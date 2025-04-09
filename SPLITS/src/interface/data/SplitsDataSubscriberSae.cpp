/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SplitsDataSubscriberSae.hpp"
#include "Logger.hpp"
#include "v2xMsgTypes.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

SplitsDataSubscriberSae::SplitsDataSubscriberSae () {
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

bool SplitsDataSubscriberSae::init(std::string topicPrefix) {
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
        SplitsSubListener * listener = new SplitsSubListenerSae(topic.first, shared_from_this());
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

void SplitsSubListenerSae::on_data_available(DataReader* reader) {
    SampleInfo info;
    LOGD(__FUNCTION__, " data available: topicid = ", topicId_);
    switch(topicId_) {
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_BSM):
        {
            J2735::idl::BasicSafetyMessage Bsm;
            if (reader->take_next_sample(&Bsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onBsmReady(std::make_shared<J2735::idl::BasicSafetyMessage>(Bsm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM):
        {
            J2735::idl::RTCMcorrection Rtcm;
            LOGD(__FUNCTION__, " reading rtcm");
            if (reader->take_next_sample(&Rtcm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    LOGD(__FUNCTION__, "  Rtcm goot");
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onRtcmReady(std::make_shared<J2735::idl::RTCMcorrection>(Rtcm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_MAP):
        {
            J2735::idl::MapData Map;
            if (reader->take_next_sample(&Map, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onMapReady(std::make_shared<J2735::idl::MapData>(Map));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SPAT):
        {
            J2735::idl::SPAT Spat;
            if (reader->take_next_sample(&Spat, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onSpatReady(std::make_shared<J2735::idl::SPAT>(Spat));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RSM):
        {
            J2735::idl::RoadSafetyMessage Rsm;
            if (reader->take_next_sample(&Rsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onRsmReady(std::make_shared<J2735::idl::RoadSafetyMessage>(Rsm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SRM):
        {
            J2735::idl::SignalRequestMessage Srm;
            if (reader->take_next_sample(&Srm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onSrmReady(std::make_shared<J2735::idl::SignalRequestMessage>(Srm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SSM):
        {
            J2735::idl::SignalStatusMessage Ssm;
            if (reader->take_next_sample(&Ssm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onSsmReady(std::make_shared<J2735::idl::SignalStatusMessage>(Ssm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM):
        {
            J2735::idl::SensorDataSharingMessage Sdsm;
            if (reader->take_next_sample(&Sdsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onSdsmReady(std::make_shared<J2735::idl::SensorDataSharingMessage>(Sdsm));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_WSA):
        {
            IEEE1609Dot3::idl::SrvAdvMsg Wsa;
            if (reader->take_next_sample(&Wsa, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        listener->onWsaReady(std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>(Wsa));
                    }
                } else {
                    LOGE(__FUNCTION__, " received invalid dds data");
                }
            } else {
                LOGE(__FUNCTION__, " faild to read DDS data");
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_MSG_PDU):
        {
            J2735::idl::MsgData Pdu;
            LOGD(__FUNCTION__, " reading pdu");
            if (reader->take_next_sample(&Pdu, &info) == ReturnCode_t::RETCODE_OK ) {
            LOGD(__FUNCTION__, "  pdu good");
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto sp = item.lock();
                        if (!sp) {
                            continue;
                        }
                        auto listener = std::dynamic_pointer_cast<SplitsDataListenerSae>(sp);
                        LOGD(__FUNCTION__, " calling listener callback");
                        listener->onPduReady(std::make_shared<J2735::idl::MsgData>(Pdu));
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
