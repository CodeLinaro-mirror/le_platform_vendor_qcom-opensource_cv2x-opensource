/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

class SplitsDataListener
{
public:
    virtual void onBsmReady(std::shared_ptr<J2735::idl::BasicSafetyMessage> msg) {}
    virtual void onMapReady(std::shared_ptr<J2735::idl::MapData> msg){}
    virtual void onSpatReady(std::shared_ptr<J2735::idl::SPAT> msg){}
    virtual void onRtcmReady(std::shared_ptr<J2735::idl::RTCMcorrection> msg) {}
    virtual void onSdsmReady(std::shared_ptr<J2735::idl::SensorDataSharingMessage> msg) {}
    virtual void onRsmReady(std::shared_ptr<J2735::idl::RoadSafetyMessage> msg) {}
    virtual void onSrmReady(std::shared_ptr<J2735::idl::SignalRequestMessage> msg) {}
    virtual void onSsmReady(std::shared_ptr<J2735::idl::SignalStatusMessage> msg) {}
    virtual void onWsaReady(std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> msg) {}
    virtual void onPduReady(std::shared_ptr<J2735::idl::MsgData> msg) {}
    virtual ~SplitsDataListener(){}
};

class SplitsDataSubscriber : public SplitsDataPubSub,
    public std::enable_shared_from_this<SplitsDataSubscriber>
{
public:

    SplitsDataSubscriber(){}

    virtual ~SplitsDataSubscriber();

    bool init(std::string topicPrefix) override;

    int registerListener(std::weak_ptr<SplitsDataListener> listener);
    int deregisterListener(std::weak_ptr<SplitsDataListener> listener);
    friend class SplitsSubListener;

private:
    eprosima::fastdds::dds::Subscriber* subscriber_;
    std::vector<eprosima::fastdds::dds::DataReader*>readers_;
    std::vector<std::weak_ptr<SplitsDataListener>> listeners_;

    std::mutex listenerMtx_;


};
} //namespace Interface
} //namespace SPLITS
#endif // _SPLITSDATASUBSCRIBER__
