/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsDataPubSub base class for splits data publisher/subscriber
 */

#ifndef __SPLITS_DATA_PUBSUB_HPP__
#define __SPLITS_DATA_PUBSUB_HPP__

#include <memory>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace SPLITS {

namespace Interface {

class SplitsDataSubscriber;
class SplitsSubListener : public eprosima::fastdds::dds::DataReaderListener
{
public:
    SplitsSubListener(int id, std::shared_ptr<SplitsDataSubscriber> subscriber )
        : matched_(0)
        , topicId_(id)
        , subscriber_(subscriber)
    {
    }
    ~SplitsSubListener() override {}
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {};

    void on_subscription_matched(
            eprosima::fastdds::dds::DataReader* reader,
            const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;
protected:
    int matched_ = 0;
    int32_t currentMatchedCount_ = 0;
    int topicId_;
    std::shared_ptr<SplitsDataSubscriber> subscriber_ = nullptr;
};
class SplitsPubListener : public eprosima::fastdds::dds::DataWriterListener
{
public:
    SplitsPubListener(int id)
        : matched_(0)
        , topicId_(id)
        , firstConnected_(false)
    {
    }

   ~SplitsPubListener() override
    {
    }
    void on_publication_matched(
            eprosima::fastdds::dds::DataWriter* writer,
            const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

    int matched_;
    int topicId_;
    int32_t currentMatchedCount_ = 0;
    eprosima::fastdds::dds::DataWriter* writer_;
    bool firstConnected_;
};

class SplitsDataPubSub
{
public:
    SplitsDataPubSub() {};
    virtual ~SplitsDataPubSub();

    virtual bool init(std::string topicPrefix) = 0;

    friend class SplitsSubListener;
    friend class SplitsPubListener;

protected:
    std::string removeNameSpace(const std::string &typeName);
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;

    std::map<int, eprosima::fastdds::dds::Topic*> topics_;

    std::vector<eprosima::fastdds::dds::TypeSupport > types_;
    std::vector<SplitsSubListener *> subListeners_;
    std::vector<SplitsPubListener *> pubListeners_;
    std::map<std::string, int> msgType2IdMap_;
    int currentMatchedCount = 0;
};
} //namespace Interface
} //namespace SPLITS
#endif // __SPLITS_DATA_PUBSUB_HPP__
