/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

int SplitsCodecImpl::decodeFrame(std::shared_ptr<v2xMsg> msg)
{
    if (msg == nullptr || msg->buf == 0 || msg->bufLen == 0) {
        std::cout << "no data to decode" << std::endl;
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
            std::cout << "wsmp decode failed" << std::endl;
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
                    std::cout << "Failed to decode WSA message" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_BSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;
                if (saeMsg->BSM()== nullptr) {
                    saeMsg->BSM(std::make_shared<J2735::idl::BasicSafetyMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->BSM()))) {
                    std::cout << "Failed to decode BSM" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_MAP:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_MAP;
                if (saeMsg->Map() == nullptr) {
                    saeMsg->Map(std::make_shared<J2735::idl::MapData>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->Map()))) {
                    std::cout << "Failed to decode MAP" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SPAT:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SPAT;
                if (saeMsg->SPAT() == nullptr) {
                    saeMsg->SPAT(std::make_shared<J2735::idl::SPAT>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SPAT()))) {
                    std::cout << "Failed to decode SPAT" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_RTCM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
                if (saeMsg->RTCMcorrection() == nullptr) {
                    saeMsg->RTCMcorrection(std::make_shared<J2735::idl::RTCMcorrection>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->RTCMcorrection()))) {
                    std::cout << "Failed to decode RTCM" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SRM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SRM;
                if (saeMsg->SRM() == nullptr) {
                    saeMsg->SRM(std::make_shared<J2735::idl::SignalRequestMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SRM()))) {
                    std::cout << "Failed to decode SRM" << std::endl;
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
                    std::cout << "Failed to decode SSM" << std::endl;
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
                    std::cout << "Failed to decode RSM" << std::endl;
                    return -1;
                }
                break;
            case SaeMsgType::V2X_SAE_MSG_SDSM:
                saeMsg->msgType_ = v2xMsgType::V2X_SAE_MSG_SDSM;
                if (saeMsg->SDSM() == nullptr) {
                    saeMsg->SDSM(std::make_shared<J2735::idl::SensorDataSharingMessage>());
                }
                if (saeCodec_->decode(header.payload, *(saeMsg->SDSM()))) {
                    std::cout << "Failed to decode SDSM" << std::endl;
                    return -1;
                }
                break;
             default:
                std::cout << "Unsupported SAE mesage " << std::endl;
                return -1;
        }
    }
    return 0;
}


int SplitsCodecImpl::encodeFrame(std::shared_ptr<v2xMsg> msg)
{
    //sanity check
    if (msg->buf == 0 || msg->bufLen == 0) {
        std::cout << "no data buffer" << std::endl;
        return -1;
    }

    if (stackType_ == StackType::STACK_TYPE_SAE) {
        std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(msg);

        // encode payload first, reserve headroom for header
        DataBuf payload;
        payload.length = saeMsg->bufLen - RESERVE_HEADER_LEN;
        payload.value = reinterpret_cast<unsigned char *>(saeMsg->buf + RESERVE_HEADER_LEN);

        std::cout << "checking type" << std::endl;
        switch(msg->msgType_) {
            case v2xMsgType::V2X_SAE_MSG_WSA:
                {
                    IEEE1609Dot3::idl::SrvAdvMsg &idlMsg = *(saeMsg->SrvAdvMsg());
                    if (saeCodec_->encode(idlMsg, payload)) {
                        std::cout << "Failed to encode WSA" << std::endl;
                        return -1;
                    }
                }
                break;
            case v2xMsgType::V2X_SAE_MSG_BSM:
                if (saeCodec_->encode(*(saeMsg->BSM()), payload)) {
                    std::cout << "Failed to encode BSM" << std::endl;
                    return -1;
                }
                break;
            case v2xMsgType::V2X_SAE_MSG_RTCM:
                if (saeCodec_->encode(*(saeMsg->RTCMcorrection()), payload)) {
                    std::cout << "Failed to encode RTCM" << std::endl;
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
            std::cerr << "Not support msgType: " << static_cast<int>(src) << std::endl;
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
            printf("before dot2 buf=%p, len=%d\n", payload.value, payload.length);
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
            printf("after dot2 buf=%p, len=%d\n", payload.value, payload.length);
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
            std::cout << "encode WSMP failed!" << std::endl;
            return -1;
        }

        //insert wsmp header in front of msg->buf
        saeMsg->buf -= header.length;
        std::memcpy(saeMsg->buf, header.value, header.length);
        saeMsg->bufLen += header.length;
        printf("after wsmp buf=%p, len=%d\n", saeMsg->buf, saeMsg->bufLen);
        //print_buffer(reinterpret_cast<unsigned char *>(saeMsg->buf), saeMsg->bufLen);

        return 0;
    } else {
        //C-SAE part.
    }
    return 0;
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
            std::cout << "length is " << numoctets << "octets, too big!" << std::endl;
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
            std::cout << "Length > 32767? " << std::endl;
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
