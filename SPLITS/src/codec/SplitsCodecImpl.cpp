/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <array>
#include <vector>
#include <iostream>
#include <memory>
#include <cstring>
#include "v2xMsgSae.hpp"
#include "SplitsCodecImpl.hpp"
#include "utils.h"
#include "Logger.hpp"

using SPLITS::v2xMsgSae;

namespace SPLITS {

namespace Codec {

SplitsCodecImpl::SplitsCodecImpl(StackType st) {
    stackType_ = st;
    if (st == SPLITS::StackType::STACK_TYPE_SAE) {
        saeCodec_ = std::unique_ptr<SPLITS::Codec::SplitsSaeCodec>(
            new SPLITS::Codec::SplitsSaeCodec());
    }
}

SplitsCodecImpl::~SplitsCodecImpl() {
    if (saeCodec_) {
        saeCodec_->deinit();
    }
}

int SplitsCodecImpl::init(int logLevel, bool syslog) {
    return saeCodec_->init(logLevel, syslog);
}
/*
int SplitsCodecImpl::decodeFrame(std::shared_ptr<v2xMsg> msg)
{
    if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data to decode");
        return -1;
    }

    if (stackType_ == StackType::STACK_TYPE_SAE) {
        std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
        DataBuf buf;

        buf.length = saeMsg->bufLen;
        buf.value = reinterpret_cast<unsigned char *>(saeMsg->buf);
        //print_buffer(payload.value, payload.length);

        //decode wsmp header
        WsmpHeader header;
        if (saeCodec_->decode(buf, header)) {
            LOGE(__FUNCTION__, " wsmp decode failed");
            return -1;
        }

#ifndef NO_DOT2_HEADER
        Ieee1609Dot2Data dot2Data;
        if (decodeIeee1609Dot2Unsecured(dot2Data, header.payload.value, &header.payload.length) < 0) {
            std::cout << "Failed to decode Ieee1609dot2 header" << std::endl;
            return -1;
        }
        msg->Ieee1609Dot2(dot2Data);
#endif

        // decode payload according to the msg type decoded from WSMP header
        switch (header.type) {
            case SaeMsgType::V2X_SAE_MSG_WSA:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
                if (saeMsg->SrvAdvMsg() == nullptr) {
                    saeMsg->SrvAdvMsg(std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SrvAdvMsg()))) {
                    LOGE(__FUNCTION__, " Failed to decode WSA message");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_BSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;
                if (saeMsg->BSM()== nullptr) {
                    saeMsg->BSM(std::make_shared<J2735::idl::BasicSafetyMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->BSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode BSM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_MAP:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
                if (saeMsg->Map() == nullptr) {
                    saeMsg->Map(std::make_shared<J2735::idl::MapData>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->Map()))) {
                    LOGE(__FUNCTION__, " Failed to decode MAP");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SPAT:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
                if (saeMsg->SPAT() == nullptr) {
                    saeMsg->SPAT(std::make_shared<J2735::idl::SPAT>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SPAT()))) {
                    LOGE(__FUNCTION__, " Failed to decode SPAT");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_RTCM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
                if (saeMsg->RTCMcorrection() == nullptr) {
                    saeMsg->RTCMcorrection(std::make_shared<J2735::idl::RTCMcorrection>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->RTCMcorrection()))) {
                    LOGE(__FUNCTION__, " Failed to decode RTCM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SRM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SRM;
                if (saeMsg->SRM() == nullptr) {
                    saeMsg->SRM(std::make_shared<J2735::idl::SignalRequestMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SRM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SRM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SSM;
#if 0
                // TODO: add SSM() in class v2xMsgSae
                if (saeMsg->SSM() == nullptr) {
                    saeMsg->SSM(std::make_shared<J2735::idl::SignalStatusMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SSM");
                    return -1;
                }
#endif
                std::cout << "not support SSM for now" << std::endl;
                return -1;
                break;
            case SaeMsgType::V2X_SAE_MSG_RSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RSM;
                if (saeMsg->RSM() == nullptr) {
                    saeMsg->RSM(std::make_shared<J2735::idl::RoadSafetyMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->RSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode RSM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SDSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SDSM;
                if (saeMsg->SDSM() == nullptr) {
                    saeMsg->SDSM(std::make_shared<J2735::idl::SensorDataSharingMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SDSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SDSM");
                    return -1;
                }
                break;
             default:
                 LOGE(__FUNCTION__, " Unsupported SAE mesage");
                return -1;
        }
    }
    return 0;
} */


/**
* Deinitialze the frames
*
* @returns 0 on success or negavie on failure.
*/
/* int SplitsCodecImpl::freeFrame(std::shared_ptr<v2xMsg> msg, bool encode){
    saeCodec_->freeFrame(
} */


int SplitsCodecImpl::decodeFrame(std::shared_ptr<v2xMsg> msg,
                                 bool secured, uint32_t type,
                                 uint8_t* inPayload, uint32_t inLen) {
    if (!secured) {
        if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
            LOGE(__FUNCTION__, " no data to decode");
            return -1;
        }
    }

    SaeMsgType msgType;
    DataBuf payloadBuf = {0};
    DataBuf* payload = nullptr;
    if (stackType_ == StackType::STACK_TYPE_SAE) {
        std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
        DataBuf buf;

        buf.length = saeMsg->bufLen;
        buf.value = reinterpret_cast<unsigned char *>(saeMsg->buf);
        //print_buffer(buf.value, buf.length);

        //decode wsmp header if unsecured
        WsmpHeader header;
        //uint8_t* payload;
        if (!secured) {
            if (saeCodec_->decode(buf, header)) {
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
            msg->Ieee1609Dot2(dot2Data);
#endif
            msgType = header.type;
            payload = &header.payload;
        } else {
            msgType = (SaeMsgType)type;
            payloadBuf.length = inLen;
            payloadBuf.value = inPayload;
            payload = &payloadBuf;
        }
#if 0
        struct DataBuf {
            uint32_t length;      /**< Length of value in bytes. */
            unsigned char* value; /**< Pointer to the data. */
        };
#endif

        // decode payload according to the msg type decoded from WSMP header
        switch (msgType) {
            case SaeMsgType::V2X_SAE_MSG_WSA:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
                if (saeMsg->SrvAdvMsg() == nullptr) {
                    saeMsg->SrvAdvMsg(std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->SrvAdvMsg()))) {
                    LOGE(__FUNCTION__, " Failed to decode WSA message");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_BSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;
                if (saeMsg->BSM()== nullptr) {
                    saeMsg->BSM(std::make_shared<J2735::idl::BasicSafetyMessage>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->BSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode BSM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_MAP:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
                if (saeMsg->Map() == nullptr) {
                    saeMsg->Map(std::make_shared<J2735::idl::MapData>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->Map()))) {
                    LOGE(__FUNCTION__, " Failed to decode MAP");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SPAT:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
                if (saeMsg->SPAT() == nullptr) {
                    saeMsg->SPAT(std::make_shared<J2735::idl::SPAT>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->SPAT()))) {
                    LOGE(__FUNCTION__, " Failed to decode SPAT");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_RTCM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
                if (saeMsg->RTCMcorrection() == nullptr) {
                    saeMsg->RTCMcorrection(std::make_shared<J2735::idl::RTCMcorrection>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->RTCMcorrection()))) {
                    LOGE(__FUNCTION__, " Failed to decode RTCM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SRM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SRM;
                if (saeMsg->SRM() == nullptr) {
                    saeMsg->SRM(std::make_shared<J2735::idl::SignalRequestMessage>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->SRM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SRM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SSM;
#if 0
                // TODO: add SSM() in class v2xMsgSae
                if (saeMsg->SSM() == nullptr) {
                    saeMsg->SSM(std::make_shared<J2735::idl::SignalStatusMessage>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->SSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SSM");
                    return -1;
                }
#endif
                LOGE(__FUNCTION__, " not support SSM for now");
                return -1;
                break;
            case SaeMsgType::V2X_SAE_MSG_RSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RSM;
                if (saeMsg->RSM() == nullptr) {
                    saeMsg->RSM(std::make_shared<J2735::idl::RoadSafetyMessage>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->RSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode RSM");
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SDSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SDSM;
                if (saeMsg->SDSM() == nullptr) {
                    saeMsg->SDSM(std::make_shared<J2735::idl::SensorDataSharingMessage>());
                }
                if (saeCodec_->decode(*payload, *(saeMsg->SDSM()))) {
                    LOGE(__FUNCTION__, " Failed to decode SDSM");
                    return -1;
                }
                break;
             default:
                LOGE(__FUNCTION__, " Unsupported SAE mesage ");
                return -1;
        }
    }
    return 0;
}

int SplitsCodecImpl::encodeFrame(std::shared_ptr<v2xMsg> msg)
{
    //sanity check
    if (msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data buffer");
        return -1;
    }

    if (stackType_ == StackType::STACK_TYPE_SAE) {
        std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);

        // encode payload first, reserve headroom for header
        DataBuf payload;
        payload.length = saeMsg->bufLen - RESERVE_HEADER_LEN;
        payload.value = reinterpret_cast<unsigned char *>(saeMsg->buf + RESERVE_HEADER_LEN);

        switch(msg->msgType_) {
            case v2xMsgType::V2X_SAE_MSG_WSA:
                {
                    IEEE1609Dot3::idl::SrvAdvMsg &idlMsg = *(saeMsg->SrvAdvMsg());
                    if (saeCodec_->encode(idlMsg, payload)) {
                        LOGE(__FUNCTION__, " Failed to encode WSA");
                        return -1;
                    }
                }
                break;
            case v2xMsgType::V2X_SAE_MSG_BSM:
                if (saeCodec_->encode(*(saeMsg->BSM()), payload)) {
                    LOGE(__FUNCTION__, " Failed to encode BSM");
                    return -1;
                }
                break;
            case v2xMsgType::V2X_SAE_MSG_RTCM:
                if (saeCodec_->encode(*(saeMsg->RTCMcorrection()), payload)) {
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
        msg->buf += RESERVE_HEADER_LEN;
        msg->bufLen = payload.length;

        // secured, call encodeWsmp after signing
        if (saeMsg->Ieee1609Dot2().content == Ieee1609Dot2Content::signedData) {
            return 0;
        }

        // unsecured, encode wsmp
        return encodeWsmp(msg, false);
    }

    return 0;
}

void SplitsCodecImpl::convertMsgType(v2xMsgType src, SaeMsgType &dest) {
    switch(src) {
        case v2xMsgType::V2X_SAE_MSG_WSA:
            dest = SaeMsgType::V2X_SAE_MSG_WSA;
            break;
        case v2xMsgType::V2X_SAE_MSG_BSM:
            dest = SaeMsgType::V2X_SAE_MSG_BSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_RTCM:
            dest = SaeMsgType::V2X_SAE_MSG_RTCM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SPAT:
            dest = SaeMsgType::V2X_SAE_MSG_SPAT;
            break;
        case v2xMsgType::V2X_SAE_MSG_MAP:
            dest = SaeMsgType::V2X_SAE_MSG_MAP;
            break;
        case v2xMsgType::V2X_SAE_MSG_RSM:
            dest = SaeMsgType::V2X_SAE_MSG_RSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SRM:
            dest = SaeMsgType::V2X_SAE_MSG_SRM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SSM:
            dest = SaeMsgType::V2X_SAE_MSG_SSM;
            break;
        case v2xMsgType::V2X_SAE_MSG_SDSM:
            dest = SaeMsgType::V2X_SAE_MSG_SDSM;
            break;
        default:
            dest = SaeMsgType::V2X_SAE_MSG_UNKNOWN;
            LOGE(__FUNCTION__, " Not support msgType: ", static_cast<int>(src));
            break;
    }
}

int SplitsCodecImpl::encodeWsmp(std::shared_ptr<v2xMsg> msg, bool secured)
{

    if (stackType_ == StackType::STACK_TYPE_SAE) {
        std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);

#ifndef NO_DOT2_HEADER
        if (secured == false) {
            /*encode 1609.2 header first */
            DataBuf payload;
            Ieee1609Dot2Data dot2Data;
            payload.length = saeMsg->bufLen;
            payload.value = reinterpret_cast<unsigned char *>(saeMsg->buf);
            //printf("before dot2 buf=%p, len=%d\n", payload.value, payload.length);
            dot2Data.protocolVersion = 3;
            dot2Data.tagclass = Ieee1609Dot2TagClass::CONTEXT_SPECIFIC;
            dot2Data.content = Ieee1609Dot2Content::unsecuredData;
            dot2Data.payloadLen = saeMsg->bufLen;
            if (encodeIeee1609Dot2Unsecured(dot2Data, payload.value, &payload.length) < 0) {
                printf("ieee1609 dot2 failure\n");
                return -1;
            }
            saeMsg->buf = reinterpret_cast<char *>(payload.value);
            saeMsg->bufLen = payload.length;
            //printf("after dot2 buf=%p, len=%d\n", payload.value, payload.length);
        }
#endif

        //encode wsmp
        WsmpHeader wsmp;
        DataBuf header;
        unsigned char data[RESERVE_HEADER_LEN] = {0};
        convertMsgType(msg->msgType_, wsmp.type);
        wsmp.payload.length = saeMsg->bufLen;
        wsmp.payload.value = NULL; //avoid copy payload, let codec encode WSMP header only
        header.length = RESERVE_HEADER_LEN;
        header.value = data;
        if (saeCodec_->encode(wsmp, header)) {
            LOGE(__FUNCTION__, " encode WSMP failed!");
            return -1;
        }

        //insert wsmp header in front of msg->buf
        saeMsg->buf -= header.length;
        std::memcpy(saeMsg->buf, header.value, header.length);
        saeMsg->bufLen += header.length;
        //printf("after wsmp buf=%p, len=%d\n", saeMsg->buf, saeMsg->bufLen);
        //print_buffer(reinterpret_cast<unsigned char *>(saeMsg->buf), saeMsg->bufLen);

        return 0;
    } else {
        //C-SAE part.
    }
    return 0;
}

int SplitsCodecImpl::decodeWsmp(std::shared_ptr<v2xMsg> msg) {
    if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
        LOGE(__FUNCTION__, " no data to decode");
        return -1;
    }
    std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
    if (stackType_ == StackType::STACK_TYPE_SAE) {
        //no alloc solution

        WsmpHeader header;
        DataBuf buf;
        int origLen = msg->bufLen;
        buf.length = msg->bufLen;
        buf.value = (unsigned char*)(msg->buf);
        if (saeCodec_->decode(buf, header)) {
            LOGE(__FUNCTION__, " wsmp decode failed");
            //free(buf.value);
            return -1;
        }
        saeMsg->buf = saeMsg->buf + buf.length;
        //saeMsg->bufLen = origLen - buf.length;
        saeMsg->bufLen = origLen;
        return (int)(header.type);

        // alloc solution
/*         std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
        DataBuf buf;
        int origLen = saeMsg->bufLen;
        buf.length = saeMsg->bufLen;
        buf.value = (unsigned char*)calloc(saeMsg->bufLen, sizeof(unsigned char));
        memcpy(buf.value, reinterpret_cast<unsigned char *>(saeMsg->buf),saeMsg->bufLen);

        //decode wsmp header
        WsmpHeader header;
        if (saeCodec_->decode(buf, header)) {
            LOGE(__FUNCTION__, " wsmp decode failed");
            free(buf.value);
            return -1;
        }else{
            saeMsg->bufLen = buf.length;
            LOGD(__FUNCTION__, " printing buffer of wsmp after decode");
            //print_buffer(buf.value, buf.length);
            //std::cout << "buf addr of rxmsg after: " << saeMsg->buf << "\n";
            saeMsg->buf = saeMsg->buf + buf.length;
            //std::cout << "buf addr of rxmsg after + buflength: " << saeMsg->buf << "\n";
            saeMsg->bufLen = origLen - buf.length;
            //std::cout << " buf length now is: " << saeMsg->bufLen << "\n";
            print_buffer((uint8_t*)saeMsg->buf, saeMsg->bufLen);
            std::cout<< "\n";
            free(buf.value);
            return (int)(header.type);
        } */
    }
    LOGI(__FUNCTION__, " Invalid stack type");
    return -1;
 }

int SplitsCodecImpl::decodeIeee1609Dot2Unsecured(Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen) {
    int payloadLen = 0;
    if (buf == 0 || bufLen == 0 || *bufLen == 0) {
        return -1;
    }

    dot2Data.protocolVersion = *buf;
    dot2Data.payloadLen = 0;
    buf++;
    (*bufLen)--;
    dot2Data.tagclass = static_cast<Ieee1609Dot2TagClass>((*buf >> 6) & 0x03);
    dot2Data.content = static_cast<Ieee1609Dot2Content>(*buf & 0x3F);
    buf++;
    (*bufLen)--;

    //CER encoded length field
    if (*buf & 0x80) {
        int numoctets = *buf & 0x7F;
        if (numoctets > 4) {
            LOGE(__FUNCTION__, " length is ", numoctets, " octets, too big!");
            return -1;
        }
        buf++;
        (*bufLen)--;
        while(numoctets > 0) {
            dot2Data.payloadLen = (dot2Data.payloadLen << 8) | *buf;
            buf++;
            (*bufLen)--;
            numoctets--;
        }
    } else {
        dot2Data.payloadLen = *buf;
        buf ++;
        (*bufLen)--;
    }
    return 0;
}
int SplitsCodecImpl::encodeIeee1609Dot2Unsecured(const Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen) {
    if (dot2Data.content != Ieee1609Dot2Content::unsecuredData) {
        return 1;
    }
    buf--;
    if (dot2Data.payloadLen > 127) {
        //only support 2 octets for simplicity
        if (dot2Data.payloadLen > 32767) {
            return -1;
        }
        *buf = dot2Data.payloadLen & 0xFF;
        buf --;
        (*bufLen)++;
        *buf = dot2Data.payloadLen >> 8 & 0xFF;
        buf --;
        (*bufLen)++;
        *buf = 0x80 | 0x2;
        buf --;
        (*bufLen)++;
    } else {
        *buf = dot2Data.payloadLen;
        (*bufLen)++;
        buf --;
    }
    *buf = static_cast<int>(dot2Data.content) | (static_cast<int>(dot2Data.tagclass) << 6);
    buf--;
    (*bufLen)++;
    *buf = 3;  //protocol version.
    (*bufLen)++;

    return 0;
}
};
};
