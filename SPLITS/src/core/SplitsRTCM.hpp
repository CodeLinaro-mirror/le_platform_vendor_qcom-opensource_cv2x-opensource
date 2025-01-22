/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITSRTCM_HPP__
#define __SPLITSRTCM_HPP__
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>
#include <cstdio>
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/DgnssManager.hpp>
#include "SplitsCore.hpp"
#include "v2xMsgSae.hpp"
#include "J2735_RTCMcorrections.h"
#include "SplitsNtrip.hpp"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"

using telux::loc::ILocationListener;
using telux::loc::DgnssStatus;
using telux::loc::IDgnssStatusListener;
using telux::common::ServiceStatus;
using telux::common::Status;
using v1::commonapi::SPLITS::SplitsConfig;
using SPLITS::v2xMsgSae;

namespace SPLITS {
namespace Core {

class SplitsRTCM : public SplitsNtrip,
    public ILocationListener,
    public IDgnssStatusListener,
    public std::enable_shared_from_this<SplitsRTCM>
{
public:
    SplitsRTCM(SPLITS::Core::Role role, std::shared_ptr<SplitsConfig::RTCMConfig> config);
    virtual ~SplitsRTCM(){
    }
    int init(void) override;
    void deinit(void) override;
    Status onReceiveRTCM(J2735::idl::RTCMcorrection &idlRTCM);
    Status fillRTCM(std::shared_ptr<v2xMsgSae> msg);
    void onMsgAvailable(uint8_t *buf, int bufLen) override;
    void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;
    void onDgnssStatusUpdate(DgnssStatus status) override;

private:

    int sendGGAString(void) override;
    std::mutex mtx_;
    std::shared_ptr<telux::loc::IDgnssManager> dgnssMgr_ = nullptr;
    std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgQueue_;
    std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgPool_;
    std::mutex msgMtx_;
    Role role_;
};
} //namespace Core
} //namespace SPLITS
#endif
