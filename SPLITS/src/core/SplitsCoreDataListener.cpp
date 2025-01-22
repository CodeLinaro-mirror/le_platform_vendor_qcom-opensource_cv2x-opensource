/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "v2xMsgTypes.hpp"
#include "SplitsCoreImpl.hpp"
#include "SplitsCoreDataListener.hpp"

using SPLITS::MsgFormat;
using SPLITS::v2xMsgType;

void SplitsCoreDataListener::onPduReady(std::shared_ptr<J2735::idl::MsgData> pdu)
{
    uint8_t signedPdu[512];
    uint32_t signedPduLen = 512;
    MsgFormat format = MsgFormat::MSG_FORMAT_UNKNOWN;

    //client sent a PDU, which is encoded message payload, we need to sign it, and add the WSMP
    //header. Messages sent from EAP are always transmitted using event flow, so we need to use a
    //message structure in eventFlow map.

    //find corresponding event flow for this type of message
    auto coreCfg = splitsCore_->configData_->getCoreConfig();
    for (auto const & txEventFlow : coreCfg.getEventFlows()) {
        std::cout << "flow sid: " << txEventFlow.getServiceId() << "pdu sid: " << pdu->serviceId() << std::endl;
        if (txEventFlow.getServiceId() == pdu->serviceId()) {
            format = static_cast<MsgFormat>(txEventFlow.getFormat());
        }
    }
    if (format == MsgFormat::MSG_FORMAT_UNKNOWN) {
        std::cerr << "Unknown msg format" << std::endl;
        return;
    }
    std::cout << "format=" << static_cast<int>(format) << std::endl;
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[pdu->serviceId()]);
    if (msg == nullptr) {
        std::cerr << "no msg for service id: "<< pdu->serviceId() << std::endl;
        return;
    }
    msg->msgType_ = psid2msgTypeMap_[pdu->serviceId()];

    if (format == MsgFormat::MSG_FORMAT_UNSIGNED) {
            std::cout << "unsigned format" << std::endl;
        //sign and add WSMP header
        if (coreCfg.getEnableSecurity() && splitsCore_->secInterface_ != nullptr) {
            //copy data to msg buffer.
            std::copy(pdu->msg().begin(), pdu->msg().end(), msg->buf);
            msg->bufLen = pdu->length();
            auto secConfig = splitsCore_->configData_->getSecConfig();
            if(!splitsCore_->secInterface_->signMsg(
                        &splitsCore_->smg_, (uint8_t*)msg->buf, msg->bufLen,
                        signedPdu, signedPduLen, secConfig.getPsidValue(),
                        secConfig.getEnableSsp(), secConfig.getSspValue().data(),secConfig.getSspLength(),
                        secConfig.getEnableSspMask(), secConfig.getSspMask().data(),
                        secConfig.getSspMaskLength(), SignerTypeOverride::STO_AUTO))
            {
                //optimize this
                std::cout << "New signed payload length is: " << signedPduLen << std::endl;
                msg->bufLen = signedPduLen;
                //copy the signed pdu to msg buffer, but reserve headroom for WSMP header.
                std::memcpy(msg->buf + RESERVE_HEADER_LEN, signedPdu, signedPduLen);
                //encode Wsmp header and prepend to begining of buffer
                if (splitsCore_->codec_->encodeWsmp(msg, true)) {
                    std::cerr << "Failed to encode WSMP header" << std::endl;
                    goto out;
                }
                //set msgFormat to blob, so core function won't do anything about it anymore
                msg->format_ = MsgFormat::MSG_FORMAT_BLOB;
            } else {
                //signing failure
            }
        } else {
            //security not enabled. add wsmp header and 1609.2 header
            std::copy(pdu->msg().begin(), pdu->msg().end(), msg->buf + RESERVE_HEADER_LEN);
            msg->buf += RESERVE_HEADER_LEN;
            msg->bufLen = pdu->length();
            //encode Wsmp header and prepend to begining of buffer
            std::cout << "calling encodeWSMP with secured false" << std::endl;
            if (splitsCore_->codec_->encodeWsmp(msg, false)) {
                std::cerr << "Failed to encode WSMP header" << std::endl;
                goto out;
            }
            //set msgFormat to blob, so core function won't do anything about it anymore
            msg->format_ = MsgFormat::MSG_FORMAT_BLOB;
        }
    } else if (format == MsgFormat::MSG_FORMAT_SIGNED) {
            std::cout << "signed format" << std::endl;
        //signed , only add WSMP header
        //copy data to msg buffer, reserved headroom for WSMP
        std::copy(pdu->msg().begin(), pdu->msg().end(), msg->buf + RESERVE_HEADER_LEN);
        msg->bufLen = pdu->length();
        //encode Wsmp header and prepend to begining of buffer
        if (splitsCore_->codec_->encodeWsmp(msg, true)) {
            std::cerr << "Failed to encode WSMP header" << std::endl;
            goto out;
        }
        //set msgFormat to blob, so core function won't do anything about it anymore
        msg->format_ = MsgFormat::MSG_FORMAT_BLOB;
    }
    // else it's BLOB, just send it out.

    splitsCore_->send(pdu->serviceId(), msg);
out:
    //reset msg
    if (msg) {
        msg->buf = msg->head;
        msg->bufLen = TX_BUF_LEN;
    }
}
void SplitsCoreDataListener::onMapReady(std::shared_ptr<J2735::idl::MapData> map)
{
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_MAP]);
    if (msg == nullptr) {
        std::cerr << "no msg for MAP msg " << std::endl;
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
    msg->Map(map);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_MAP, msg) < 0) {
        std::cerr << "tx failure" << std::endl;
    }
}
void SplitsCoreDataListener::onSpatReady(std::shared_ptr<J2735::idl::SPAT> spat)
{
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_SPAT]);
    if (msg == nullptr) {
        std::cerr << "no msg for SPAT msg " << std::endl;
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
    msg->SPAT(spat);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_SPAT, msg) < 0) {
        std::cerr << "tx failure" << std::endl;
    }

}
void SplitsCoreDataListener::onRtcmReady(std::shared_ptr<J2735::idl::RTCMcorrection> rtcm)
{
    std::cout << "got RTCM " << std::endl;
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_RTCM]);
    if (msg == nullptr) {
        std::cerr << "no msg for RTCM msg "<< std::endl;
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
    msg->RTCMcorrection(rtcm);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_RTCM, msg) < 0) {
        std::cerr << "tx failure" << std::endl;
    }
}
void SplitsCoreDataListener::onWsaReady(std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> wsa)
{
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_WSA]);
    if (msg == nullptr) {
        std::cerr << "no msg for WSA msg "<< std::endl;
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
    msg->SrvAdvMsg(wsa);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_WSA, msg) < 0) {
        std::cerr << "tx failure" << std::endl;
    }
}
