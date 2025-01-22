/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsNtrip.hpp
 *
 * @brief Ntrip client class for RTCM injection.
 *
 * This implementation does NOT support the ntrip server that requires sending NMEA GGA string,
 * because this class can runs on EAP, with no location service available.
 */
#ifndef __SPLITNTRIP_HPP__
#define __SPLITNTRIP_HPP__
#include <mutex>
#include <memory>
#include <chrono>
#include <queue>
#include <thread>
#include <cstdio>
//#include <telux/loc/LocationDefines.hpp>
//#include <telux/loc/LocationManager.hpp>
//#include <telux/loc/DgnssManager.hpp>
//#include "SplitsCore.hpp"
#include "v2xMsgSae.hpp"
#include "J2735_RTCMcorrections.h"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"

#if 0
using telux::loc::ILocationListener;
using telux::loc::DgnssStatus;
using telux::loc::IDgnssStatusListener;
using telux::common::ServiceStatus;
using telux::common::Status;
using SPLITS::v2xMsgSae;
#endif
using v1::commonapi::SPLITS::SplitsConfig;
namespace SPLITS {
namespace Core {

class SplitsNtrip
{
public:
    SplitsNtrip(std::shared_ptr<SplitsConfig::RTCMConfig> config);
    virtual ~SplitsNtrip(){
    }
    virtual int init(void);
    virtual void deinit(void);
//    Status onReceiveRTCM(J2735::idl::RTCMcorrection &idlRTCM);
//    int fillRTCM(std::shared_ptr<v2xMsgSae> msg);
//    void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;
//    void onDgnssStatusUpdate(DgnssStatus status) override;

    virtual void onMsgAvailable(uint8_t *buffer, int bufLen) = 0;

protected:
    std::shared_ptr<SplitsConfig::RTCMConfig> config_;
    std::string lastNmeaGGA_;
    std::chrono::time_point<std::chrono::system_clock> lastNmeaSentTime_;
    int ntcSocketFd_;

private:
    int connectNtrip(void);
    virtual int sendGGAString(void) {
        return 0;
    }
    int waitForSock(int fd);
    int processRtcmFromServer(void);
    //reading RTCM data from file and create J2735 RTCM msg, this is for test only.
    int processRtcmFromFile(void);
    std::mutex mtx_;
    //std::shared_ptr<telux::loc::IDgnssManager> dgnssMgr_ = nullptr;
    FILE *sourceFp_;
    bool stop_ = false;
    bool reconnect_ = false;
    //std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgQueue_;
    //std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgPool_;
    //std::mutex msgMtx_;
    std::thread ntripThread_;
};
} //namespace Core
} //namespace SPLITS
#endif
