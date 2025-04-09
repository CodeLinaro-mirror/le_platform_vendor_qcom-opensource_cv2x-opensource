/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SPLITS dds data subscriber implementation.
 *
 */
#ifndef _SPLITSDATASUBSCRIBER__
#define _SPLITSDATASUBSCRIBER__

#include <memory>
#include "SplitsDataPubSub.hpp"

namespace SPLITS {

namespace Interface {

class SplitsDataListener {
public:
    virtual ~SplitsDataListener(){}
};

class SplitsDataSubscriber : public SplitsDataPubSub {
public:

    SplitsDataSubscriber() : SplitsDataPubSub() {}

    virtual ~SplitsDataSubscriber();

    bool init(std::string topicPrefix) override;

    int registerListener(std::weak_ptr<SplitsDataListener> listener);
    int deregisterListener(std::weak_ptr<SplitsDataListener> listener);

    friend class SplitsSubListenerSae;
    friend class SplitsSubListenerCsae;

protected:
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    std::vector<eprosima::fastdds::dds::DataReader*>readers_;
    std::mutex listenerMtx_;
    std::vector<std::weak_ptr<SplitsDataListener>> listeners_;
};
} //namespace Interface
} //namespace SPLITS
#endif // _SPLITSDATASUBSCRIBER__
