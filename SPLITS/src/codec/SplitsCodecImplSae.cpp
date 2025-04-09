/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <array>
#include <vector>
#include <iostream>
#include <memory>
#include <cstring>
#include "SplitsCodecImplSae.hpp"
#include "utils.h"
#include "Logger.hpp"

using SPLITS::v2xMsgSae;

namespace SPLITS {

namespace Codec {

SplitsCodecImplSae::SplitsCodecImplSae() {
    LOGD(__FUNCTION__);

    codecWrapper_ = std::make_shared<SPLITS::Codec::SplitsCodecSae>();
}

int SplitsCodecImplSae::decodeFrame(std::shared_ptr<v2xMsg> msg,
                                 bool secured, uint32_t psid,
                                 uint8_t* inPayload, uint32_t inLen) {
    uint32_t tmpId = psid;
    DataBuf payloadBuf = {0};
    DataBuf* payload = nullptr;
    DataBuf buf;

    if (!secured) {
        if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
            LOGE(__FUNCTION__, " no data to decode");
            return -1;
        }
    }

    auto saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
    auto saeCodec = std::dynamic_pointer_cast<SplitsCodecSae>(codecWrapper_);

    buf.length = saeMsg->bufLen;
    buf.value = reinterpret_cast<unsigned char *>(saeMsg->buf);
    //print_buffer(buf.value, buf.length);

    //decode wsmp header if unsecured
    WsmpHeader header;
    if (!secured) {
        if (saeCodec->decode(buf, header)) {
            LOGE(__FUNCTION__, " wsmp decode failed");
            return -1;
        }

#ifndef NO_DOT2_HEADER
        Ieee1609Dot2Data dot2Data;
        if (decodeIeee1609Dot2Unsecured(dot2Data,
                                        header.payload.value,
                                        &header.payload.length) < 0) {
            LOGE(__FUNCTION__, " Failed to decode Ieee1609dot2 header");
            return -1;
        }
        saeMsg->Ieee1609Dot2(dot2Data);
#endif

        payload = &header.payload;
        tmpId = header.psid;
    } else {
        payloadBuf.length = inLen;
        payloadBuf.value = inPayload;
        payload = &payloadBuf;
    }

    // decode payload according to the msg type decoded from WSMP header
    switch (tmpId) {
        case SAE_PSID_WSA:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
            if (saeMsg->SrvAdvMsg() == nullptr) {
                saeMsg->SrvAdvMsg(std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->SrvAdvMsg()))) {
                LOGE(__FUNCTION__, " Failed to decode WSA message");
                return -1;
            }
            break;
        case SAE_PSID_BSM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;
            if (saeMsg->BSM()== nullptr) {
                saeMsg->BSM(std::make_shared<J2735::idl::BasicSafetyMessage>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->BSM()))) {
                LOGE(__FUNCTION__, " Failed to decode BSM");
                return -1;
            }
            break;
        case SAE_PSID_MAP:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
            if (saeMsg->Map() == nullptr) {
                saeMsg->Map(std::make_shared<J2735::idl::MapData>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->Map()))) {
                LOGE(__FUNCTION__, " Failed to decode MAP");
                return -1;
            }
            break;
        case SAE_PSID_SPAT:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
            if (saeMsg->SPAT() == nullptr) {
                saeMsg->SPAT(std::make_shared<J2735::idl::SPAT>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->SPAT()))) {
                LOGE(__FUNCTION__, " Failed to decode SPAT");
                return -1;
            }
            break;
        case SAE_PSID_RTCM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
            if (saeMsg->RTCMcorrection() == nullptr) {
                saeMsg->RTCMcorrection(std::make_shared<J2735::idl::RTCMcorrection>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->RTCMcorrection()))) {
                LOGE(__FUNCTION__, " Failed to decode RTCM");
                return -1;
            }
            break;
        case SAE_PSID_SRM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SRM;
            if (saeMsg->SRM() == nullptr) {
                saeMsg->SRM(std::make_shared<J2735::idl::SignalRequestMessage>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->SRM()))) {
                LOGE(__FUNCTION__, " Failed to decode SRM");
                return -1;
            }
            break;
        case SAE_PSID_SSM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SSM;
#if 0
            // TODO: add SSM() in class v2xMsgSae
            if (saeMsg->SSM() == nullptr) {
                saeMsg->SSM(std::make_shared<J2735::idl::SignalStatusMessage>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->SSM()))) {
                LOGE(__FUNCTION__, " Failed to decode SSM");
                return -1;
            }
#endif
            LOGE(__FUNCTION__, " not support SSM for now");
            return -1;
            break;
        case SAE_PSID_RSM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RSM;
            if (saeMsg->RSM() == nullptr) {
                saeMsg->RSM(std::make_shared<J2735::idl::RoadSafetyMessage>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->RSM()))) {
                LOGE(__FUNCTION__, " Failed to decode RSM");
                return -1;
            }
            break;
        case SAE_PSID_SDSM:
            saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SDSM;
            if (saeMsg->SDSM() == nullptr) {
                saeMsg->SDSM(std::make_shared<J2735::idl::SensorDataSharingMessage>());
            }
            if (saeCodec->decode(*payload, *(saeMsg->SDSM()))) {
                LOGE(__FUNCTION__, " Failed to decode SDSM");
                return -1;
            }
            break;
         default:
            LOGE(__FUNCTION__, " Unsupported SAE mesage ");
            return -1;
    }

    return 0;
}

int SplitsCodecImplSae::encodeFrame(std::shared_ptr<v2xMsg> msg) {
    //sanity check
    if (msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data buffer");
        return -1;
    }

    auto saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
    auto saeCodec = std::dynamic_pointer_cast<SplitsCodecSae>(codecWrapper_);

    // encode payload first, reserve headroom for header
    DataBuf payload;
    payload.length = saeMsg->bufLen - RESERVE_HEADER_LEN;
    payload.value = reinterpret_cast<unsigned char *>(saeMsg->buf + RESERVE_HEADER_LEN);

    switch(saeMsg->msgType_) {
        case v2xMsgType::V2X_SAE_MSG_WSA:
            {
                IEEE1609Dot3::idl::SrvAdvMsg &idlMsg = *(saeMsg->SrvAdvMsg());
                if (saeCodec->encode(idlMsg, payload)) {
                    LOGE(__FUNCTION__, " Failed to encode WSA");
                    return -1;
                }
            }
            break;
        case v2xMsgType::V2X_SAE_MSG_BSM:
            if (saeCodec->encode(*(saeMsg->BSM()), payload)) {
                LOGE(__FUNCTION__, " Failed to encode BSM");
                return -1;
            }
            break;
        case v2xMsgType::V2X_SAE_MSG_RTCM:
            if (saeCodec->encode(*(saeMsg->RTCMcorrection()), payload)) {
                LOGE(__FUNCTION__, " Failed to encode RTCM");
                return -1;
            }
            break;
        case v2xMsgType::V2X_SAE_MSG_SPAT:
        case v2xMsgType::V2X_SAE_MSG_MAP:
        case v2xMsgType::V2X_SAE_MSG_RSM:
        case v2xMsgType::V2X_SAE_MSG_SRM:
        case v2xMsgType::V2X_SAE_MSG_SSM:
        case v2xMsgType::V2X_SAE_MSG_SDSM:
        default:
            return -1;
    }

    // set encoded payload to msg buf
    saeMsg->buf += RESERVE_HEADER_LEN;
    saeMsg->bufLen = payload.length;

    // secured, call encodeHeader after signing
    if (saeMsg->Ieee1609Dot2().content == Ieee1609Dot2Content::signedData) {
        return 0;
    }

    // unsecured, encode wsmp
    return encodeHeader(saeMsg, false);
}

void SplitsCodecImplSae::msgType2Psid(v2xMsgType type, uint32_t &psid) {
    switch(type) {
        case v2xMsgType::V2X_SAE_MSG_WSA:
            psid = SAE_PSID_WSA;
            break;
        case v2xMsgType::V2X_SAE_MSG_BSM:
            psid = SAE_PSID_BSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_RTCM:
            psid = SAE_PSID_RTCM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SPAT:
            psid = SAE_PSID_SPAT;
            break;
        case v2xMsgType::V2X_SAE_MSG_MAP:
            psid = SAE_PSID_MAP;
            break;
        case v2xMsgType::V2X_SAE_MSG_RSM:
            psid = SAE_PSID_RSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SRM:
            psid = SAE_PSID_SRM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SSM:
            psid = SAE_PSID_SSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SDSM:
            psid = SAE_PSID_SDSM;
            break;
        default:
            psid = 0;
            LOGE(__FUNCTION__, " Not support msgType: ", static_cast<int>(type));
            break;
    }
}

int SplitsCodecImplSae::encodeHeader(std::shared_ptr<v2xMsg> msg, bool secured)
{
#ifndef NO_DOT2_HEADER
    if (secured == false) {
        /*encode 1609.2 header first */
        DataBuf payload;
        Ieee1609Dot2Data dot2Data;
        payload.length = msg->bufLen;
        payload.value = reinterpret_cast<unsigned char *>(msg->buf);
        dot2Data.protocolVersion = 3;
        dot2Data.tagclass = Ieee1609Dot2TagClass::CONTEXT_SPECIFIC;
        dot2Data.content = Ieee1609Dot2Content::unsecuredData;
        dot2Data.payloadLen = msg->bufLen;
        if (encodeIeee1609Dot2Unsecured(dot2Data, payload.value, &payload.length) < 0) {
            printf("ieee1609 dot2 failure\n");
            return -1;
        }
        msg->buf = reinterpret_cast<char *>(payload.value);
        msg->bufLen = payload.length;
    }
#endif

    unsigned char data[RESERVE_HEADER_LEN] = {0};
    DataBuf header;
    header.length = RESERVE_HEADER_LEN;
    header.value = data;

    //encode wsmp
    WsmpHeader wsmp;
    msgType2Psid(msg->msgType_, wsmp.psid);
    wsmp.payload.length = msg->bufLen;
    wsmp.payload.value = NULL; //avoid copy payload, let codec encode WSMP header only
    auto saeCodec = std::dynamic_pointer_cast<SplitsCodecSae>(codecWrapper_);
    int ret = saeCodec->encode(wsmp, header);
    if (ret < 0) {
        LOGE(__FUNCTION__, " encode header failed:", ret);
        return ret;
    }

    //insert encoded header in front of msg->buf
    msg->buf -= header.length;
    std::memcpy(msg->buf, header.value, header.length);
    msg->bufLen += header.length;

    return 0;
}

int SplitsCodecImplSae::decodeHeader(std::shared_ptr<v2xMsg> msg) {
    if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data to decode");
        return -1;
    }

    DataBuf buf;
    int origLen = msg->bufLen;
    buf.length = msg->bufLen;
    buf.value = (unsigned char*)(msg->buf);
    WsmpHeader header;
    auto saeCodec = std::dynamic_pointer_cast<SplitsCodecSae>(codecWrapper_);
    if (saeCodec->decode(buf, header)) {
        LOGE(__FUNCTION__, " wsmp decode failed");
        //free(buf.value);
        return -1;
    }
    msg->buf = msg->buf + buf.length;
    msg->bufLen = origLen;

    return (int)(header.psid);
}
};
};
