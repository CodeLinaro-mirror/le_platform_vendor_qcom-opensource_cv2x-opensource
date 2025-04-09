/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "v2xMsgSae.hpp"
#include "SplitsServiceDataPublisherSae.hpp"
#include "Logger.hpp"
#include "SplitsCore.hpp"

#include "J2735_BasicSafetyMessagePubSubTypes.h"
#include "J2735_RTCMcorrectionsPubSubTypes.h"
#include "J2735_MapDataPubSubTypes.h"
#include "J2735_SPATPubSubTypes.h"
#include "J2735_PSMPubSubTypes.h"
#include "J2735_RSMPubSubTypes.h"
#include "J2735_SDSMPubSubTypes.h"
#include "J2735_SRMPubSubTypes.h"
#include "J2735_SSMPubSubTypes.h"
#include "Ieee1609Dot3WsaPubSubTypes.h"
#include "J2735_PDUPubSubTypes.h"

namespace SPLITS {
namespace Interface {

SplitsServiceDataPublisherSae::SplitsServiceDataPublisherSae () {
    LOGD(__FUNCTION__);

    std::string typeName;
    eprosima::fastdds::dds::TypeSupport BsmType(new J2735::idl::BasicSafetyMessagePubSubType);
    types_.push_back(BsmType);
    typeName = removeNameSpace(BsmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_BSM);

    eprosima::fastdds::dds::TypeSupport RtcmType(new J2735::idl::RTCMcorrectionPubSubType);
    types_.push_back(RtcmType);
    typeName = removeNameSpace(RtcmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM);

    eprosima::fastdds::dds::TypeSupport MapType(new J2735::idl::MapDataPubSubType);
    types_.push_back(MapType);
    typeName = removeNameSpace(MapType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_MAP);

    eprosima::fastdds::dds::TypeSupport SpatType(new J2735::idl::SPATPubSubType);
    types_.push_back(SpatType);
    typeName = removeNameSpace(SpatType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SPAT);

    eprosima::fastdds::dds::TypeSupport RsmType(new J2735::idl::RoadSafetyMessagePubSubType);
    types_.push_back(RsmType);
    typeName = removeNameSpace(RsmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RSM);

    eprosima::fastdds::dds::TypeSupport SrmType(new J2735::idl::SignalRequestMessagePubSubType);
    types_.push_back(SrmType);
    typeName = removeNameSpace(SrmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SRM);

    eprosima::fastdds::dds::TypeSupport SsmType(new J2735::idl::SignalStatusMessagePubSubType);
    types_.push_back(SsmType);
    typeName = removeNameSpace(SsmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SSM);

    eprosima::fastdds::dds::TypeSupport SdsmType(
        new J2735::idl::SensorDataSharingMessagePubSubType);
    types_.push_back(SdsmType);
    typeName = removeNameSpace(SdsmType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM);

    eprosima::fastdds::dds::TypeSupport WsaType(new IEEE1609Dot3::idl::SrvAdvMsgPubSubType);
    types_.push_back(WsaType);
    typeName = removeNameSpace(WsaType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_WSA);

    eprosima::fastdds::dds::TypeSupport pduType(new J2735::idl::MsgDataPubSubType);
    types_.push_back(pduType);
    typeName = removeNameSpace(pduType.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_MSG_PDU);
}

void SplitsServiceDataPublisherSae::publish(std::shared_ptr<v2xMsg> msg) {
    for (const auto listener : pubListeners_) {
        if (listener->firstConnected_ || listener->matched_ > 0) {
            if (listener->topicId_ == static_cast<int>(msg->msgType_)) {
                auto saeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
                switch(msg->msgType_) {
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_BSM:
                        listener->writer_->write(saeMsg->BSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_SPAT:
                        listener->writer_->write(saeMsg->SPAT().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_MAP:
                        listener->writer_->write(saeMsg->Map().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM:
                        listener->writer_->write(saeMsg->RTCMcorrection().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_RSM:
                        listener->writer_->write(saeMsg->RSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_SRM:
                        listener->writer_->write(saeMsg->SRM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_SSM:
                        listener->writer_->write(saeMsg->SSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM:
                        listener->writer_->write(saeMsg->SDSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_SAE_MSG_WSA:
                        listener->writer_->write(saeMsg->SrvAdvMsg().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_MSG_PDU:
                        listener->writer_->write(saeMsg->MsgData().get());
                        break;
                    default:
                        LOGE(__FUNCTION__, " unknown msg type");
                        break;
                }
            }
        }
    }
}
} //namespace Interface
} //namespace SPLITS
