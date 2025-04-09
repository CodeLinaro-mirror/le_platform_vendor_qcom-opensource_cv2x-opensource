/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SplitsDataSubscriberCsae.hpp"
#include "Logger.hpp"
#include "v2xMsgTypes.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

SplitsDataSubscriberCsae::SplitsDataSubscriberCsae () {
    LOGD(__FUNCTION__);

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

bool SplitsDataSubscriberCsae::init(std::string topicPrefix) {
    LOGD(__FUNCTION__);

    //call baseclass init function to create participant, topics and register types
    if (false == SplitsDataSubscriber::init(topicPrefix)) {
        return false;
    }

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr) {
        LOGE(__FUNCTION__, " Failed to create dds subscriber");
        return false;
    }
    //CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    for (auto const & topic : topics_) {
        SplitsSubListener * listener = new SplitsSubListenerCsae(topic.first, shared_from_this());
        if (!listener ) {
            LOGE(__FUNCTION__, " Failed to create listener for topic: ",
                 (topic.second)->get_type_name());
            return false;
        }
        eprosima::fastdds::dds::DataReader* reader =
            subscriber_->create_datareader(topic.second, rqos, listener);
        if (!reader ){
            LOGE(__FUNCTION__, " Failed to create reader for topic: ",
                 (topic.second)->get_type_name());
            return false;
        }
        subListeners_.push_back(listener);
        readers_.push_back(reader);
    }
    return true;
}

void SplitsSubListenerCsae::on_data_available(DataReader* reader) {
    SampleInfo info;
    LOGD(__FUNCTION__, " data available: topicid = ", topicId_);
    switch(topicId_) {
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_BSM):
        {
            Csae::idl::BasicSafetyMessage bsm;
            if (reader->take_next_sample(&bsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerCsae>(sp);
                        listener->onBsmReady(std::make_shared<Csae::idl::BasicSafetyMessage>(bsm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_MAP):
        {
            Csae::idl::MapData map;
            if (reader->take_next_sample(&map, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerCsae>(sp);
                        listener->onMapReady(std::make_shared<Csae::idl::MapData>(map));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_SPAT):
        {
            Csae::idl::SPAT spat;
            if (reader->take_next_sample(&spat, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerCsae>(sp);
                        listener->onSpatReady(std::make_shared<Csae::idl::SPAT>(spat));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_RSM):
        {
            Csae::idl::RoadsideSafetyMessage rsm;
            if (reader->take_next_sample(&rsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerCsae>(sp);
                        listener->onRsmReady(std::make_shared<Csae::idl::RoadsideSafetyMessage>(rsm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_CSAE_MSG_RSI):
        {
            Csae::idl::RoadSideInformation rsi;
            if (reader->take_next_sample(&rsi, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerCsae>(sp);
                        listener->onRsiReady(std::make_shared<Csae::idl::RoadSideInformation>(rsi));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        default:
            LOGE(__FUNCTION__, " topicId: ", topicId_, "not supported");
            break;
    }
}
} //namespace Interface
} //namespace SPLITS
