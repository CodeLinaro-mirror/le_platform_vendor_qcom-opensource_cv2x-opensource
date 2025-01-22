/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsDataPubSub.hpp"


using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

SplitsDataPubSub::SplitsDataPubSub()
    : participant_(nullptr)
{
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

    eprosima::fastdds::dds::TypeSupport SdsmType(new J2735::idl::SensorDataSharingMessagePubSubType);
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
std::string SplitsDataPubSub::removeNameSpace(const std::string & typeName)
{
    std::string::size_type n;
    n = typeName.rfind(":");
    return typeName.substr(n + 1, std::string::npos);
}

SplitsDataPubSub::~SplitsDataPubSub()
{
    for (auto topic : topics_) {
        participant_->delete_topic(topic.second);
    }
    topics_.clear();
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool SplitsDataPubSub::init(std::string topicPrefix)
{

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.name("cv2x_sub");

    participant_ = DomainParticipantFactory::get_instance()->create_participant(
            SPLITS_DDS_DOMAIN_ID, pqos);
    if (participant_ == nullptr)
    {
        std::cerr << "Failed to create dds participant" << std::endl;
        return false;
    }

    //REGISTER THE TYPE
    for (auto const & type : types_) {
        type.register_type(participant_);
    }
    //CREATE THE TOPIC
    for (auto const & type : types_) {
        auto typeName = removeNameSpace(type.get_type_name());
        auto topic = participant_->create_topic(topicPrefix + typeName, type.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic == nullptr)
        {
            std::cerr << "Failed to create dds topic for type: " << type.get_type_name() << std::endl;
            return false;
        }
        auto topicId = msgType2IdMap_[typeName];
        std::cout << "creating topic: " << topicPrefix + typeName <<"id: " << topicId <<  std::endl;
        topics_[topicId] = topic;
    }
    return true;
}
} //namespace Interface
} //namespace SPLITS
