/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <array>
#include <vector>
#include <iostream>
#include <memory>
#include <cstring>
#include "SplitsCodecImplCsae.hpp"
#include "utils.h"
#include "Logger.hpp"

using SPLITS::v2xMsgCsae;

namespace SPLITS {

namespace Codec {

SplitsCodecImplCsae::SplitsCodecImplCsae() {
    LOGD(__FUNCTION__);

    codecWrapper_ = std::make_shared<SPLITS::Codec::SplitsCodecCsae>();
}

int SplitsCodecImplCsae::decodeFrame(std::shared_ptr<v2xMsg> msg,
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
    auto csaeMsg = std::dynamic_pointer_cast<v2xMsgCsae>(msg);
    auto csaeCodec = std::dynamic_pointer_cast<SplitsCodecCsae>(codecWrapper_);

    buf.length = csaeMsg->bufLen;
    buf.value = reinterpret_cast<unsigned char *>(csaeMsg->buf);

    //decode dsmp header if unsecured
    DsmpHeader header;
    if (!secured) {
        if (csaeCodec->decode(buf, header)) {
            LOGE(__FUNCTION__, " dsmp decode failed");
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
        csaeMsg->Ieee1609Dot2(dot2Data);
#endif

        payload = &header.payload;
        tmpId = header.aid;
    } else {
        payloadBuf.length = inLen;
        payloadBuf.value = inPayload;
        payload = &payloadBuf;
    }

    // decode payload according to the msg type decoded from DSMP header
    switch (tmpId) {
        case CSAE_AID_BSM:
        case CSAE_AID_BSM_EVENT:
        case CSAE_AID_BSM_EMG:
        case CSAE_AID_BSM_EMG_EVENT:
            csaeMsg->msgType_ = v2xMsgType::V2X_CSAE_MSG_BSM;
            if (csaeMsg->BSM()== nullptr) {
                csaeMsg->BSM(std::make_shared<Csae::idl::BasicSafetyMessage>());
            }
            if (csaeCodec->decode(*payload, *(csaeMsg->BSM()))) {
                LOGE(__FUNCTION__, " Failed to decode BSM");
                return -1;
            }
            break;
        case CSAE_AID_MAP:
            csaeMsg->msgType_ = v2xMsgType::V2X_CSAE_MSG_MAP;
            if (csaeMsg->Map() == nullptr) {
                csaeMsg->Map(std::make_shared<Csae::idl::MapData>());
            }
            if (csaeCodec->decode(*payload, *(csaeMsg->Map()))) {
                LOGE(__FUNCTION__, " Failed to decode MAP");
                return -1;
            }
            break;
        case CSAE_AID_SPAT:
            csaeMsg->msgType_ = v2xMsgType::V2X_CSAE_MSG_SPAT;
            if (csaeMsg->SPAT() == nullptr) {
                csaeMsg->SPAT(std::make_shared<Csae::idl::SPAT>());
            }
            if (csaeCodec->decode(*payload, *(csaeMsg->SPAT()))) {
                LOGE(__FUNCTION__, " Failed to decode SPAT");
                return -1;
            }
            break;
        case CSAE_AID_RSM:
            csaeMsg->msgType_ = v2xMsgType::V2X_CSAE_MSG_RSM;
            if (csaeMsg->RSM() == nullptr) {
                csaeMsg->RSM(std::make_shared<Csae::idl::RoadsideSafetyMessage>());
            }
            if (csaeCodec->decode(*payload, *(csaeMsg->RSM()))) {
                LOGE(__FUNCTION__, " Failed to decode RSM");
                return -1;
            }
            break;
        case CSAE_AID_RSI_STATIC:
        case CSAE_AID_RSI_SEMI_STATIC:
        case CSAE_AID_RSI_DYNAMIC:
            csaeMsg->msgType_ = v2xMsgType::V2X_CSAE_MSG_RSI;
            if (csaeMsg->RSI() == nullptr) {
                csaeMsg->RSI(std::make_shared<Csae::idl::RoadSideInformation>());
            }
            if (csaeCodec->decode(*payload, *(csaeMsg->RSI()))) {
                LOGE(__FUNCTION__, " Failed to decode RSI");
                return -1;
            }
            break;
         default:
            LOGE(__FUNCTION__, " Unsupported CSAE mesage ");
            return -1;
    }

    return 0;
}

int SplitsCodecImplCsae::encodeFrame(std::shared_ptr<v2xMsg> msg) {
    //sanity check
    if (msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data buffer");
        return -1;
    }

    auto csaeMsg = std::dynamic_pointer_cast<v2xMsgCsae>(msg);
    auto csaeCodec = std::dynamic_pointer_cast<SplitsCodecCsae>(codecWrapper_);

    // encode payload first, reserve headroom for header
    DataBuf payload;
    payload.length = csaeMsg->bufLen - RESERVE_HEADER_LEN;
    payload.value = reinterpret_cast<unsigned char *>(csaeMsg->buf + RESERVE_HEADER_LEN);

    switch(csaeMsg->msgType_) {
        case v2xMsgType::V2X_CSAE_MSG_BSM:
            if (csaeCodec->encode(*(csaeMsg->BSM()), payload)) {
                LOGE(__FUNCTION__, " Failed to encode BSM");
                return -1;
            }
            break;
        case v2xMsgType::V2X_CSAE_MSG_SPAT:
        case v2xMsgType::V2X_CSAE_MSG_MAP:
        case v2xMsgType::V2X_CSAE_MSG_RSM:
        case v2xMsgType::V2X_CSAE_MSG_RSI:
        default:
            return -1;
    }

    // set encoded payload to msg buf
    csaeMsg->buf += RESERVE_HEADER_LEN;
    csaeMsg->bufLen = payload.length;

    // secured, call encodeHeader after signing
    if (csaeMsg->Ieee1609Dot2().content == Ieee1609Dot2Content::signedData) {
        return 0;
    }

    // unsecured, encode wsmp
    return encodeHeader(csaeMsg, false);
}

void SplitsCodecImplCsae::msgType2Psid(v2xMsgType type, uint32_t &psid) {
    switch(type) {
        case v2xMsgType::V2X_CSAE_MSG_BSM:
            psid = CSAE_AID_BSM;
            break;
        case v2xMsgType::V2X_CSAE_MSG_MAP:
            psid = CSAE_AID_MAP;
            break;
        case v2xMsgType::V2X_CSAE_MSG_SPAT:
            psid = CSAE_AID_SPAT;
            break;
        case v2xMsgType::V2X_CSAE_MSG_RSM:
            psid = CSAE_AID_RSM;
            break;
        case v2xMsgType::V2X_CSAE_MSG_RSI:
            psid = CSAE_AID_RSI_STATIC;
            break;
        default:
            psid = 0;
            LOGE(__FUNCTION__, " Not support msgType: ", static_cast<int>(type));
            break;
    }
}

int SplitsCodecImplCsae::encodeHeader(std::shared_ptr<v2xMsg> msg, bool secured)
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

    //encode dsmp
    DsmpHeader dsmp;
    msgType2Psid(msg->msgType_, dsmp.aid);
    dsmp.payload.length = msg->bufLen;
    dsmp.payload.value = NULL; //avoid copy payload, let codec encode WSMP header only
    auto csaeCodec = std::dynamic_pointer_cast<SplitsCodecCsae>(codecWrapper_);
    int ret = csaeCodec->encode(dsmp, header);
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

int SplitsCodecImplCsae::decodeHeader(std::shared_ptr<v2xMsg> msg) {
    if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data to decode");
        return -1;
    }

    DataBuf buf;
    int origLen = msg->bufLen;
    buf.length = msg->bufLen;
    buf.value = (unsigned char*)(msg->buf);
    DsmpHeader header;
    auto csaeCodec = std::dynamic_pointer_cast<SplitsCodecCsae>(codecWrapper_);
    if (csaeCodec->decode(buf, header)) {
        LOGE(__FUNCTION__, " dsmp decode failed");
        return -1;
    }
    msg->buf = msg->buf + buf.length;
    msg->bufLen = origLen;

    return (int)(header.aid);
}
};
};
