/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <chrono>
#include "SplitsDataPublisher.hpp"
#include "Logger.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {

namespace Interface {
SplitsDataPublisher::~SplitsDataPublisher() {
    for (auto writer : writers_) {
        publisher_->delete_datawriter(writer);
    }
    writers_.clear();
    for (auto listener : pubListeners_) {
        delete listener;
    }
    pubListeners_.clear();
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
    stop_ = true;
    td_.join();
}

bool SplitsDataPublisher::init(std::string topicPrefix) {
    //call base class init method to create participant, topics and types
    SplitsDataPubSub::init(topicPrefix);
    //CREATE THE PUBLISHER
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    if (publisher_ == nullptr)
    {
        LOGD(__FUNCTION__, " Failed to create publisher");
        return false;
    }
    // CREATE THE WRITER

    for (const auto topic : topics_) {
        SplitsPubListener *listener = new SplitsPubListener(topic.first);
        eprosima::fastdds::dds::DataWriter * writer =
            publisher_->create_datawriter(topic.second, DATAWRITER_QOS_DEFAULT, listener);
        if (writer == nullptr)
        {
            LOGD(__FUNCTION__, " Failed to create writer");
            return false;
        }
        writers_.push_back(writer);
        pubListeners_.push_back(listener);
    }

    return true;
}

void SplitsPubListener::on_publication_matched(
        eprosima::fastdds::dds::DataWriter* writer,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) {
    writer_ = writer;
    if (info.current_count_change == 1) {
        currentMatchedCount_ = info.current_count;
        matched_ = currentMatchedCount_;
        LOGD(__FUNCTION__, "  Datawriter matched: ", currentMatchedCount_);
    }
    else if (info.current_count_change == -1) {
        currentMatchedCount_ = info.current_count;
        LOGD(__FUNCTION__, " DataWriter unmatched.");
    } else {
        LOGD(__FUNCTION__, info.current_count_change,
             " is not a valid value for PublicationMatchedStatus current count change");
    }
}
void SplitsDataPublisher::run(void) {
    stop_ = false;
    td_ = std::thread(&SplitsDataPublisher::publish, this);
}

void SplitsDataPublisher::stop(void) {
    stop_ = true;
    td_.join();
}
} //namespace Interface
} //namespace SPLITS
