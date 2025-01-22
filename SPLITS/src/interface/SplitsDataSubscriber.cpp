/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SplitsDataSubscriber.hpp"


using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {
SplitsDataSubscriber::~SplitsDataSubscriber()
{
    for (auto reader : readers_) {
        if (reader != nullptr)
        {
            if (subscriber_ != nullptr) {
                subscriber_->delete_datareader(reader);
            }
        }
    }

    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
}

bool SplitsDataSubscriber::init(std::string topicPrefix)
{
    //call baseclass init function to create participant, topics and register types
    SplitsDataPubSub::init(topicPrefix);
    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr)
    {
        std::cerr << "Failed to create dds subscriber" << std::endl;
        return false;
    }
    //CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    for (auto const & topic : topics_) {
        SplitsSubListener * listener = new SplitsSubListener(topic.first, shared_from_this());
        if (!listener ){
            std::cerr << "Failed to create listener for topic: " << (topic.second)->get_type_name() << std::endl;
            return false;
        }
        eprosima::fastdds::dds::DataReader* reader = subscriber_->create_datareader(topic.second, rqos, listener);
        if (!reader ){
            std::cerr << "Failed to create reader for topic: " << (topic.second)->get_type_name() << std::endl;
            return false;
        }
        subListeners_.push_back(listener);
        readers_.push_back(reader);
    }
    return true;
}
int SplitsDataSubscriber::registerListener(std::weak_ptr<SplitsDataListener> listener)
{
    auto sp = listener.lock();
    if (sp == nullptr) {
        std::cout << "SplitsDataSubscriber::registerListener: invalid param " << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lk(listenerMtx_);
    for (auto item : listeners_) {
        if (item.lock() == sp) {
            return 0;
        }
    }
    listeners_.push_back(listener);
    return 0;
}
int SplitsDataSubscriber::deregisterListener(std::weak_ptr<SplitsDataListener> listener)
{
    std::lock_guard<std::mutex> lk(listenerMtx_);
    bool listenerExisted = false;

    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (!sp) {
            it = listeners_.erase(it);
        } else if (sp == listener.lock()) {
            it = listeners_.erase(it);
            listenerExisted = true;
        } else {
            ++it;
        }
    }
    if (listenerExisted) {
        return 0;
    } else  {
        std::cout << "no such listener existed" << std::endl;
        return -1;
    }
}

void SplitsSubListener::on_subscription_matched(
        DataReader*,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        currentMatchedCount_ = info.total_count;
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        currentMatchedCount_ = info.total_count;
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void SplitsSubListener::on_data_available(
        DataReader* reader)
{
    SampleInfo info;
    std::cout << "data available: topicid="<< topicId_ << std::endl;
    switch(topicId_) {
        case  static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_BSM):
        {
            J2735::idl::BasicSafetyMessage Bsm;
            if (reader->take_next_sample(&Bsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onBsmReady(std::make_shared<J2735::idl::BasicSafetyMessage>(Bsm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM):
        {
            J2735::idl::RTCMcorrection Rtcm;
            std::cout << "reading rtcm" << std::endl;
            if (reader->take_next_sample(&Rtcm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
            std::cout << " Rtcm goot" << std::endl;
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onRtcmReady(std::make_shared<J2735::idl::RTCMcorrection>(Rtcm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_MAP):
        {
            J2735::idl::MapData Map;
            if (reader->take_next_sample(&Map, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onMapReady(std::make_shared<J2735::idl::MapData>(Map));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SPAT):
        {
            J2735::idl::SPAT Spat;
            if (reader->take_next_sample(&Spat, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onSpatReady(std::make_shared<J2735::idl::SPAT>(Spat));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_RSM):
        {
            J2735::idl::RoadSafetyMessage Rsm;
            if (reader->take_next_sample(&Rsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onRsmReady(std::make_shared<J2735::idl::RoadSafetyMessage>(Rsm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SRM):
        {
            J2735::idl::SignalRequestMessage Srm;
            if (reader->take_next_sample(&Srm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onSrmReady(std::make_shared<J2735::idl::SignalRequestMessage>(Srm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SSM):
        {
            J2735::idl::SignalStatusMessage Ssm;
            if (reader->take_next_sample(&Ssm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onSsmReady(std::make_shared<J2735::idl::SignalStatusMessage>(Ssm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM):
        {
            J2735::idl::SensorDataSharingMessage Sdsm;
            if (reader->take_next_sample(&Sdsm, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onSdsmReady(std::make_shared<J2735::idl::SensorDataSharingMessage>(Sdsm));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_SAE_MSG_WSA):
        {
            IEEE1609Dot3::idl::SrvAdvMsg Wsa;
            if (reader->take_next_sample(&Wsa, &info) == ReturnCode_t::RETCODE_OK ) {
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        listener->onWsaReady(std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>(Wsa));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        case static_cast<int>(SPLITS::v2xMsgType::V2X_MSG_PDU):
        {
            J2735::idl::MsgData Pdu;
            std::cout << "reading pdu" << std::endl;
            if (reader->take_next_sample(&Pdu, &info) == ReturnCode_t::RETCODE_OK ) {
            std::cout << " pdu good" << std::endl;
                if (info.valid_data) {
                    for (auto item : subscriber_->listeners_) {
                        auto listener = item.lock();
                        std::cout << "calling listener callback" << std::endl;
                        listener->onPduReady(std::make_shared<J2735::idl::MsgData>(Pdu));
                    }
                } else {
                    std::cerr << "received invalid dds data" << std::endl;
                }
            } else {
                std::cerr << "faild to read DDS data" << std::endl;
            }
        }
            break;
        default:
            std::cerr << "topicId: " << topicId_ << "not supported" << std::endl;
            break;

    }
}
} //namespace Interface
} //namespace SPLITS
