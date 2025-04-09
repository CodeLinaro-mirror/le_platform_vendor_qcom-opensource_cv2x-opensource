/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SPLITS dds data subscriber implementation.
 *
 */
#ifndef _SPLITSDATASUBSCRIBERCSAE__
#define _SPLITSDATASUBSCRIBERCSAE__

#include <memory>
#include "BSMPubSubTypes.h"
#include "MapPubSubTypes.h"
#include "SignalPhaseAndTimingPubSubTypes.h"
#include "RSMPubSubTypes.h"
#include "RSIPubSubTypes.h"
#include "SplitsDataSubscriber.hpp"

namespace SPLITS {

namespace Interface {

class SplitsDataListener;
class SplitsSubListener;
class SplitsDataSubscriber;

class SplitsDataListenerCsae : public SplitsDataListener {
public:
    // CSAE msgs
    virtual void onBsmReady(std::shared_ptr<Csae::idl::BasicSafetyMessage> msg) {}
    virtual void onMapReady(std::shared_ptr<Csae::idl::MapData> msg){}
    virtual void onSpatReady(std::shared_ptr<Csae::idl::SPAT> msg){}
    virtual void onRsmReady(std::shared_ptr<Csae::idl::RoadsideSafetyMessage> msg) {}
    virtual void onRsiReady(std::shared_ptr<Csae::idl::RoadSideInformation> msg) {}
};

class SplitsSubListenerCsae : public SplitsSubListener {
public:

    SplitsSubListenerCsae(int id, std::shared_ptr<SplitsDataSubscriber> subscriber)
        : SplitsSubListener(id, subscriber) {}
    void on_data_available(eprosima::fastdds::dds::DataReader* reader);
};

class SplitsDataSubscriberCsae : public SplitsDataSubscriber,
    public std::enable_shared_from_this<SplitsDataSubscriberCsae> {
public:

    SplitsDataSubscriberCsae();
    bool init(std::string topicPrefix);
};
} //namespace Interface
} //namespace SPLITS
#endif // _SPLITSDATASUBSCRIBERCSAE__
