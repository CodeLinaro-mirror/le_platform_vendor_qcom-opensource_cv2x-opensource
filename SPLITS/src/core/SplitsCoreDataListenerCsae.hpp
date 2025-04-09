/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsCoreDataListenerCsae.hpp
 *
 * @description this class implements SplitsDataListenerCsae interface, it receives data sent from
 * SPLITS client via DDS interface, and transmit the data out by calling core->send() method.
 */
#ifndef SPLITS_CORE_DATALISTENER_CSAE_HPP__
#define SPLITS_CORE_DATALISTENER_CSAE_HPP__

#include <memory>

#include "data/SplitsDataSubscriberCsae.hpp"

using SPLITS::Core::SplitsCore;

namespace SPLITS {

namespace Core {

class SplitsCoreImpl;

class SplitsCoreDataListenerCsae : public SPLITS::Interface::SplitsDataListenerCsae {
public:
    SplitsCoreDataListenerCsae(std::shared_ptr<SplitsCoreImpl> core) :
        splitsCore_(core) {
        psid2msgTypeMap_[CSAE_AID_RSM] = v2xMsgType::V2X_CSAE_MSG_RSM;
        psid2msgTypeMap_[CSAE_AID_RSI_STATIC] = v2xMsgType::V2X_CSAE_MSG_RSI;
        psid2msgTypeMap_[CSAE_AID_RSI_SEMI_STATIC] = v2xMsgType::V2X_CSAE_MSG_RSI;
        psid2msgTypeMap_[CSAE_AID_RSI_DYNAMIC] = v2xMsgType::V2X_CSAE_MSG_RSI;
    }
    ~SplitsCoreDataListenerCsae(){}

    // CSAE msgs
    void onBsmReady(std::shared_ptr<Csae::idl::BasicSafetyMessage> msg) override;
    void onMapReady(std::shared_ptr<Csae::idl::MapData> msg) override;
    void onSpatReady(std::shared_ptr<Csae::idl::SPAT> msg) override;
    void onRsmReady(std::shared_ptr<Csae::idl::RoadsideSafetyMessage> msg) override;
    void onRsiReady(std::shared_ptr<Csae::idl::RoadSideInformation> msg) override;

private:
    std::shared_ptr<SplitsCoreImpl> splitsCore_;
    std::map<int, v2xMsgType> psid2msgTypeMap_;
};

} //namespace Core
} //namespace SPLITS

#endif //SPLITS_CORE_DATALISTENER_CSAE_HPP__
