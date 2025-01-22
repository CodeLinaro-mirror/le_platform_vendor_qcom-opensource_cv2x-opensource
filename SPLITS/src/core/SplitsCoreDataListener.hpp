/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsCoreDataListener.hpp
 *
 * @description this class implements SplitsDataListener interface, it receives data sent from
 * SPLITS client via DDS interface, and transmit the data out by calling core->send() method.
 */
#ifndef SPLITS_CORE_DATALISTENER_HPP__
#define SPLITS_CORE_DATALISTENER_HPP__
#include <memory>
#include "SplitsDataSubscriber.hpp"

using SPLITS::Core::SplitsCore;
using SPLITS::Interface::SplitsDataListener;
namespace SPLITS {
namespace Core {

class SplitsCoreImpl;

class SplitsCoreDataListener : public SplitsDataListener
{
public:
    SplitsCoreDataListener(std::shared_ptr<SplitsCoreImpl> core) :
        splitsCore_(core) {
            psid2msgTypeMap_[SAE_PSID_BSM] = v2xMsgType::V2X_SAE_MSG_BSM;
            psid2msgTypeMap_[SAE_PSID_SPAT] = v2xMsgType::V2X_SAE_MSG_SPAT;
            psid2msgTypeMap_[SAE_PSID_MAP] = v2xMsgType::V2X_SAE_MSG_MAP;
            psid2msgTypeMap_[SAE_PSID_SRM] = v2xMsgType::V2X_SAE_MSG_SRM;
            psid2msgTypeMap_[SAE_PSID_SSM] = v2xMsgType::V2X_SAE_MSG_SSM;
            psid2msgTypeMap_[SAE_PSID_SDSM] = v2xMsgType::V2X_SAE_MSG_SDSM;
            psid2msgTypeMap_[SAE_PSID_RTCM] = v2xMsgType::V2X_SAE_MSG_RTCM;
            psid2msgTypeMap_[SAE_PSID_WSA] = v2xMsgType::V2X_SAE_MSG_WSA;
        }
    virtual ~SplitsCoreDataListener(){}

    void onPduReady(std::shared_ptr<J2735::idl::MsgData> pdu) override;
    void onSpatReady(std::shared_ptr<J2735::idl::SPAT> spat) override;
    void onRtcmReady(std::shared_ptr<J2735::idl::RTCMcorrection> rtcm) override;
    void onWsaReady(std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> wsa) override;
    void onMapReady(std::shared_ptr<J2735::idl::MapData> map) override;

private:
    std::shared_ptr<SplitsCoreImpl> splitsCore_;
    std::map<int, v2xMsgType> psid2msgTypeMap_;
};

} //namespace Core
} //namespace SPLITS

#endif //SPLITS_CORE_DATALISTENER_HPP__
