/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsDataPubSub.hpp"
#include "Logger.hpp"
#include "v2xMsg.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

std::string SplitsDataPubSub::removeNameSpace(const std::string & typeName) {
    std::string::size_type n;
    n = typeName.rfind(":");
    return typeName.substr(n + 1, std::string::npos);
}

SplitsDataPubSub::~SplitsDataPubSub()
{
    LOGD(__FUNCTION__);
    for (auto topic : topics_) {
        participant_->delete_topic(topic.second);
    }
    topics_.clear();
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool SplitsDataPubSub::init(std::string topicPrefix) {
    LOGD(__FUNCTION__);

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.name("cv2x_sub");

    participant_ = DomainParticipantFactory::get_instance()->create_participant(
            SPLITS_DDS_DOMAIN_ID, pqos);
    if (participant_ == nullptr)
    {
        LOGE(__FUNCTION__, " Failed to create dds participant");
        return false;
    }

    //REGISTER THE TYPE
    for (auto const & type : types_) {
        type.register_type(participant_);
    }
    //CREATE THE TOPIC
    LOGD(__FUNCTION__, " jie, type size:", types_.size());
    for (auto const & type : types_) {
        auto typeName = removeNameSpace(type.get_type_name());
        auto topic = participant_->create_topic(topicPrefix + typeName,
                                                type.get_type_name(),
                                                TOPIC_QOS_DEFAULT);
        if (topic == nullptr)
        {
            LOGE(__FUNCTION__, " Failed to create dds topic for type: ", type.get_type_name());
            return false;
        }
        auto topicId = msgType2IdMap_[typeName];
        LOGD(__FUNCTION__, " creating topic: ", (topicPrefix + typeName), ", id: ", topicId);
        topics_[topicId] = topic;
    }
    return true;
}
} //namespace Interface
} //namespace SPLITS
