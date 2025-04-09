/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "v2xMsgCsae.hpp"
#include "SplitsServiceDataPublisherCsae.hpp"
#include "Logger.hpp"
#include "SplitsCore.hpp"

#include "BSMPubSubTypes.h"
#include "MapPubSubTypes.h"
#include "SignalPhaseAndTimingPubSubTypes.h"
#include "RSMPubSubTypes.h"
#include "RSIPubSubTypes.h"

namespace SPLITS {
namespace Interface {

SplitsServiceDataPublisherCsae::SplitsServiceDataPublisherCsae() {
    LOGD(__FUNCTION__, " jie");

    std::string typeName;
    eprosima::fastdds::dds::TypeSupport cBsm(new Csae::idl::BasicSafetyMessagePubSubType);
    types_.push_back(cBsm);
    typeName = removeNameSpace(cBsm.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_BSM);

    eprosima::fastdds::dds::TypeSupport cMap(new Csae::idl::MapDataPubSubType);
    types_.push_back(cMap);
    typeName = removeNameSpace(cMap.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_MAP);

    eprosima::fastdds::dds::TypeSupport cSpat(new Csae::idl::SPATPubSubType);
    types_.push_back(cSpat);
    typeName = removeNameSpace(cSpat.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_SPAT);

    eprosima::fastdds::dds::TypeSupport cRsi(new Csae::idl::RoadSideInformationPubSubType);
    types_.push_back(cRsi);
    typeName = removeNameSpace(cRsi.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_RSI);

    eprosima::fastdds::dds::TypeSupport cRsm(new Csae::idl::RoadsideSafetyMessagePubSubType);
    types_.push_back(cRsm);
    typeName = removeNameSpace(cRsm.get_type_name());
    msgType2IdMap_[typeName] = static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_RSM);
}

void SplitsServiceDataPublisherCsae::publish(std::shared_ptr<v2xMsg> msg) {
    LOGD(__FUNCTION__, " jie listener size:", pubListeners_.size(),
         ", publish type:", static_cast<int>(msg->msgType_));

    for (const auto listener : pubListeners_) {
        LOGD(__FUNCTION__, " jie firstConnected_:", listener->firstConnected_,
             ", matched:", listener->matched_, " topic:", listener->topicId_);

        if (listener->firstConnected_ || listener->matched_ > 0) {
            if (listener->topicId_ == static_cast<int>(msg->msgType_)) {
                auto csaeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgCsae>(msg);
                LOGI(__FUNCTION__,  " csae type = ", static_cast<int>(msg->msgType_));
                switch(msg->msgType_) {
                    case SPLITS::v2xMsgType::V2X_CSAE_MSG_BSM:
                        listener->writer_->write(csaeMsg->BSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_CSAE_MSG_SPAT:
                        listener->writer_->write(csaeMsg->SPAT().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_CSAE_MSG_MAP:
                        listener->writer_->write(csaeMsg->Map().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_CSAE_MSG_RSM:
                        listener->writer_->write(csaeMsg->RSM().get());
                        break;
                    case SPLITS::v2xMsgType::V2X_CSAE_MSG_RSI:
                        listener->writer_->write(csaeMsg->RSI().get());
                        break;
                    default:
                        LOGE(__FUNCTION__, " Unknown c-sae msg type");
                        break;
                }
            }
        }
    }
}
} //namespace Interface
} //namespace SPLITS
