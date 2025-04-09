/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SPLITS dds data subscriber implementation.
 *
 */
#ifndef _SPLITSDATASUBSCRIBERSAE__
#define _SPLITSDATASUBSCRIBERSAE__

#include <memory>
#include "J2735_BasicSafetyMessagePubSubTypes.h"
#include "J2735_RTCMcorrectionsPubSubTypes.h"
#include "J2735_MapDataPubSubTypes.h"
#include "J2735_SPATPubSubTypes.h"
#include "J2735_PSMPubSubTypes.h"
#include "J2735_RSMPubSubTypes.h"
#include "J2735_SDSMPubSubTypes.h"
#include "J2735_SRMPubSubTypes.h"
#include "J2735_SSMPubSubTypes.h"
#include "Ieee1609Dot3WsaPubSubTypes.h"
#include "J2735_PDUPubSubTypes.h"
#include "SplitsDataSubscriber.hpp"


namespace SPLITS {

namespace Interface {

class SplitsDataListener;
class SplitsSubListener;
class SplitsDataSubscriber;

class SplitsDataListenerSae : public SplitsDataListener {
public:
    // SAE msgs
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
};

class SplitsSubListenerSae : public SplitsSubListener {
public:

    SplitsSubListenerSae(int id, std::shared_ptr<SplitsDataSubscriber> subscriber)
        : SplitsSubListener(id, subscriber) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader);
};

class SplitsDataSubscriberSae : public SplitsDataSubscriber,
    public std::enable_shared_from_this<SplitsDataSubscriberSae> {
public:

    SplitsDataSubscriberSae();
    bool init(std::string topicPrefix);
};
} //namespace Interface
} //namespace SPLITS
#endif // _SPLITSDATASUBSCRIBERSAE__
