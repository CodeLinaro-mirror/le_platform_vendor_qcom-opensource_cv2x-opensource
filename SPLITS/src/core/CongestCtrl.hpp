/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __CONGESTCTRL_HPP__
#define __CONGESTCTRL_HPP__

#include <semaphore.h>
#include <mutex>
#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/prop/CongestionControlManager.hpp>
#include <telux/cv2x/prop/V2xPropFactory.hpp>
#include "location/KinematicsReceive.hpp"
#include "RadioTransmit.h"

using telux::common::Status;
using namespace telux::cv2x::prop;
using telux::cv2x::prop::ICongestionControlManager;
using telux::cv2x::prop::V2xPropFactory;

namespace SPLITS {
namespace Core {

struct CongCtrlConfig {
    /*
        SQUISH CONFIGURATION ITEMS
    */
    uint8_t congCtrlType = 1; // for now just SAE j3161/1
    uint32_t enableCongCtrlLogging = 0;
    /*
        # j2945/1 ; j3161 ; SAE
    */
    uint32_t cbpMeasInterval = 100;
    double cbpWeightFactor = 0.5;
    /*
        packet error rate related
    */
    uint32_t perInterval = 5000;
    uint32_t perSubInterval = 100;
    double perMax = 0.3;

    // ITT related
    uint32_t vMax_ITT = 600;
    uint32_t vRescheduleTh = 25;
    uint8_t cv2xMaxITTRounding = 0; // 0, 2
    /*
     * channel quality indication related
     */
    double minChanQualInd = 0.0;
    double maxChanQualInd = 0.3;
    /*
        density related
    */
    double vDensityWeightFactor = 0.05;
    double vDensityCoefficient = 25; // lambda
    uint32_t vDensityWindow = 1;
    uint32_t vDensityMinPerRange = 100;
    uint32_t vMaxSuccessiveFail = 3; //0, 10000
    uint8_t UseStaticVDensity = 0;// 0,1 # Will cause vDensity to be used instead of calculated density
    uint32_t vDensity = 10; //0, 100 # Only used if UseStaticDensity is set

    /*
        Tracking Error related
    */
    uint32_t txCtrlInterval = 100;
    uint32_t hvTEMinTimeDiff = 0;
    uint32_t hvTEMaxTimeDiff = 200;
    uint32_t rvTEMinTimeDiff = 0;
    uint32_t rvTEMaxTimeDiff = 3000;
    uint8_t teErrSensitivity = 75;
    double teMinThresh = 0.2;
    double teMaxThresh = 0.5;
    /*
        Inter Transmit Time and Tx Decision related
    */
    uint32_t minItt = 100;
    uint32_t maxItt = 600;
    uint8_t txRand = 5;
    uint32_t timeAccuracy = 1000;
    uint8_t reschedThresh = 25;

    /*
        power related
    */

    uint8_t supraGain = 0.5;
    uint8_t minChanUtil = 50;
    uint8_t maxChanUtil = 80;
    uint8_t minRadiPwr = 10;
    uint8_t maxRadiPwr = 20;

    /*
        sps enhancements parameters
    */
    bool enableSpsEnhancements = false;
    uint8_t spsEnhIntervalRound = 100; //#CV2XPeriodicityHz     = 100; 20, 1000
    uint8_t spsEnhHysterPerc = 5; //#cv2xSPSHysterisis = 5;
    uint8_t spsEnhDelayPerc = 20; //#cv2xMaxITTChangeFreq = 5;
};

class CongestCtrlListener : public ICongestionControlListener {
public:
    CongestCtrlListener(std::shared_ptr<RadioTransmit> transmit) :
        spsTransmit_(transmit) {
    }
    virtual ~CongestCtrlListener() {
    }
    void onCongestionControlDataReady(
            std::shared_ptr<CongestionControlUserData> congestionControlUserData,
            bool critEvent) override;

    const CongestionControlCalculations getCbData(void) {
        std::lock_guard<std::mutex> lk(cbMutex_);
        return congCtrlCbData;
    }

private:
    uint64_t lastPeriodicity_ = 100;
    CongestionControlCalculations congCtrlCbData;
    std::mutex cbMutex_;
    std::shared_ptr<RadioTransmit> spsTransmit_;
    void updateSpsTransmitFlow(
            std::shared_ptr<CongestionControlUserData> congestionControlUserData);
};

class CongestCtrl : public ILocationListener,
    public std::enable_shared_from_this<CongestCtrl> {
public:
    CongestCtrl(){}
    virtual ~CongestCtrl(){}
    Status init(const CongCtrlConfig & config, std::shared_ptr<RadioTransmit> spsTransmit);

    std::shared_ptr<CongestCtrl> getptr() {
        return shared_from_this();
    }
    Status waitForTransmit(void);
    void loadConfig(std::map<std::string, std::string> configs);
    void writeLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp, bool validPkt,
            uint16_t eventsData );
    const std::shared_ptr<ICongestionControlManager> getCurrCongestionCtrlMgr() {
        return congestCtrlMgr_;
    }
    bool addCongestionControlData(uint64_t id,
    double latitude, double longitude, double heading, double speed,
    uint64_t timestamp, uint64_t msgCount);

private:
    void onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo) override;
    std::shared_ptr<CongestCtrlListener> congestCtrlListener_;
    std::shared_ptr<ICongestionControlManager> congestCtrlMgr_;
    std::shared_ptr<CongestionControlUserData> congestCtrlCbData_;
    CongCtrlConfig config_;

};
} //namespace Core
} //namespace SPLITS
#endif
