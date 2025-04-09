/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "v2xMsgTypes.hpp"
#include "v2xMsgSae.hpp"
#include "SplitsCoreImpl.hpp"
#include "SplitsCoreDataListenerSae.hpp"
#include "Logger.hpp"

using SPLITS::MsgFormat;
using SPLITS::v2xMsgType;

namespace SPLITS {

namespace Core {

void SplitsCoreDataListenerSae::onPduReady(std::shared_ptr<J2735::idl::MsgData> pdu) {
    uint8_t signedPdu[512];
    uint32_t signedPduLen = 512;
    MsgFormat format = MsgFormat::MSG_FORMAT_UNKNOWN;

    //client sent a PDU, which is encoded message payload, we need to sign it, and add the WSMP
    //header. Messages sent from EAP are always transmitted using event flow, so we need to use a
    //message structure in eventFlow map.

    //find corresponding event flow for this type of message
    auto coreCfg = splitsCore_->configData_->getCoreConfig();
    for (auto const & txEventFlow : coreCfg.getEventFlows()) {
        LOGD(__FUNCTION__, " flow sid: ", txEventFlow.getServiceId(),
             "pdu sid: ", pdu->serviceId());
        if (txEventFlow.getServiceId() == pdu->serviceId()) {
            format = static_cast<MsgFormat>(txEventFlow.getFormat());
        }
    }
    if (format == MsgFormat::MSG_FORMAT_UNKNOWN) {
        LOGE(__FUNCTION__, " Unknown msg format");
        return;
    }
    LOGD(__FUNCTION__, " format = ", static_cast<int>(format));
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[pdu->serviceId()]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for service id: ", pdu->serviceId());
        return;
    }
    msg->msgType_ = psid2msgTypeMap_[pdu->serviceId()];

    if (format == MsgFormat::MSG_FORMAT_UNSIGNED) {
            LOGD(__FUNCTION__, " unsigned format");
        //sign and add WSMP header
        if (coreCfg.getEnableSecurity() && splitsCore_->secInterface_ != nullptr) {
            //copy data to msg buffer.
            std::copy(pdu->msg().begin(), pdu->msg().end(), msg->buf);
            msg->bufLen = pdu->length();
            auto secConfig = splitsCore_->configData_->getSecConfig();
            if(!splitsCore_->secInterface_->signMsg(
                        &splitsCore_->smg_, (uint8_t*)msg->buf, msg->bufLen,
                        signedPdu, signedPduLen, secConfig.getPsidValue(),
                        secConfig.getEnableSsp(),
                        secConfig.getSspValue().data(),secConfig.getSspLength(),
                        secConfig.getEnableSspMask(), secConfig.getSspMask().data(),
                        secConfig.getSspMaskLength(), SignerTypeOverride::STO_AUTO))
            {
                //optimize this
                LOGD(__FUNCTION__, " New signed payload length is: ", signedPduLen);
                msg->bufLen = signedPduLen;
                //copy the signed pdu to msg buffer, but reserve headroom for WSMP header.
                std::memcpy(msg->buf + RESERVE_HEADER_LEN, signedPdu, signedPduLen);
                //encode Wsmp header and prepend to begining of buffer
                if (splitsCore_->codec_->encodeHeader(msg, true)) {
                    LOGE(__FUNCTION__, " Failed to encode WSMP header");
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
            LOGD(__FUNCTION__, " calling encodeHeader with secured false");
            if (splitsCore_->codec_->encodeHeader(msg, false)) {
                LOGE(__FUNCTION__, " Failed to encode WSMP header");
                goto out;
            }
            //set msgFormat to blob, so core function won't do anything about it anymore
            msg->format_ = MsgFormat::MSG_FORMAT_BLOB;
        }
    } else if (format == MsgFormat::MSG_FORMAT_SIGNED) {
        LOGD(__FUNCTION__, " signed format");
        //signed , only add WSMP header
        //copy data to msg buffer, reserved headroom for WSMP
        std::copy(pdu->msg().begin(), pdu->msg().end(), msg->buf + RESERVE_HEADER_LEN);
        msg->bufLen = pdu->length();
        //encode Wsmp header and prepend to begining of buffer
        if (splitsCore_->codec_->encodeHeader(msg, true)) {
            LOGE(__FUNCTION__, " Failed to encode WSMP header");
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
void SplitsCoreDataListenerSae::onMapReady(std::shared_ptr<J2735::idl::MapData> map) {
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_MAP]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for MAP msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
    msg->Map(map);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_MAP, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}
void SplitsCoreDataListenerSae::onSpatReady(std::shared_ptr<J2735::idl::SPAT> spat) {
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_SPAT]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for SPAT msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
    msg->SPAT(spat);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_SPAT, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }

}
void SplitsCoreDataListenerSae::onRtcmReady(std::shared_ptr<J2735::idl::RTCMcorrection> rtcm) {
    LOGD(__FUNCTION__, " got RTCM ");
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_RTCM]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for RTCM msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
    msg->RTCMcorrection(rtcm);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_RTCM, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}
void SplitsCoreDataListenerSae::onWsaReady(std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> wsa) {
    auto msg = std::dynamic_pointer_cast<v2xMsgSae>(splitsCore_->txEventMsgs_[SAE_PSID_WSA]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for WSA msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
    msg->SrvAdvMsg(wsa);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(SAE_PSID_WSA, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

} // namespace Core
} // namespace SPLITS
