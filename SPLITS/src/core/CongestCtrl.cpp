/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <sstream>
#include "CongestCtrl.hpp"

namespace SPLITS {
namespace Core {

void CongestCtrlListener::updateSpsTransmitFlow(
        std::shared_ptr<CongestionControlUserData> congestionControlUserData)
{
    // once the user data is updated, the thread in qits
    // can now schedule a transmission
    // cast void pointer
    // if sps enhancements enabled, we should make sure that the sps flow reservation is redone
    if(spsTransmit_ != nullptr && congestionControlUserData->spsEnhancementsEnabled
        && congestionControlUserData->congestionControlCalculations->maxITT != lastPeriodicity_){
        // create a new sps flow with the rounded max ITT that congestionControl calculates
        SpsFlowInfo spsInfo;
        // congestionControl rounds it already to valid values for sps periodicity
        spsInfo.periodicityMs =
            (congestionControlUserData->congestionControlCalculations->maxITT);

        // set sps priority to same value
        spsInfo.priority = spsTransmit_->getSpsPriority();

        // set sps size to same value
        spsInfo.nbytesReserved = spsTransmit_->getSpsResSize();

        // catch future error here
        try{
            uint8_t ret = spsTransmit_->updateSpsFlow(spsInfo);
            if(ret == static_cast<uint8_t>(Status::FAILED)){
                std::cerr << "sps transmit flow update failed\n";
                std::cerr << "Max itt was: " <<
                    congestionControlUserData->congestionControlCalculations->maxITT <<"\n";
            }
        }catch(const std::future_error& e){
            std::cout << "Caught future error when updating sps flow\n";
            std::cout << "Error log is: " << e.what() << "\n";
        }
        lastPeriodicity_ = congestionControlUserData->congestionControlCalculations->maxITT;
    }
}
void CongestCtrlListener::onCongestionControlDataReady (
    std::shared_ptr<CongestionControlUserData> congestionControlUserData, bool critEvent) {

    if(congestionControlUserData){
        std::lock_guard<std::mutex> lk(cbMutex_);
        updateSpsTransmitFlow(congestionControlUserData);
        congCtrlCbData = *(congestionControlUserData->congestionControlCalculations.get());
        if(!critEvent){
            sem_post(congestionControlUserData->congestionControlSem);
        }
    }
}


void CongestCtrl::onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo)
{
    Position pos = {0};
    double speed = 0.0;

    pos.posLat = locationInfo->getLatitude();
    pos.posLong = locationInfo->getLongitude();
    pos.heading = locationInfo->getHeading();
    pos.elev = locationInfo->getAltitude();
    speed = locationInfo->getSpeed();

    if (congestCtrlMgr_ != nullptr) {
        CCErrorCode res = congestCtrlMgr_->updateHostVehicleData(pos, speed);
    }
}
Status CongestCtrl::waitForTransmit(void)
{
    if (!sem_wait(congestCtrlMgr_->getCongestionControlUserData()->congestionControlSem)) {
        return Status::SUCCESS;
    }
    return Status::FAILED;
}
void CongestCtrl::loadConfig(std::map<std::string, std::string> configs)
{
    std::cout << "Proceeding to load congestion ctrl config parameters\n";

    if(configs.find("congCtrlType") != configs.end()){
        config_.congCtrlType = stoi(configs["congCtrlType"]);
    }
    if(configs.find("enableCongCtrlLogging") != configs.end()){
        //config_.enableCongCtrlLogging = stoi(configs["enableCongCtrlLogging"]);
        config_.enableCongCtrlLogging = 10;
    }
    if(configs.find("cbpMeasInterval") != configs.end()){
        config_.cbpMeasInterval = stoi(configs["cbpMeasInterval"]);
    }
    if(configs.find("cbpWeightFactor") != configs.end()){
        config_.cbpWeightFactor = stod(configs["cbpWeightFactor"]);
    }
    if(configs.find("perInterval") != configs.end()){
        config_.perInterval = stoi(configs["perInterval"]);
    }
    if(configs.find("perSubInterval") != configs.end()){
        config_.perSubInterval = stoi(configs["perSubInterval"]);
    }
    if(configs.find("perMax") != configs.end()){
        config_.perMax = stod(configs["perMax"]);
    }
    if(configs.find("minChanQualInd") != configs.end()){
        config_.minChanQualInd = stod(configs["minChanQualInd"]);
    }
    if(configs.find("maxChanQualInd") != configs.end()){
        config_.maxChanQualInd = stod(configs["maxChanQualInd"]);
    }
    if(configs.find("vDensityWeightFactor") != configs.end()){
        config_.vDensityWeightFactor = stod(configs["vDensityWeightFactor"]);
    }
    if(configs.find("vDensityCoefficient") != configs.end()){
        config_.vDensityCoefficient = stod(configs["vDensityCoefficient"]);
    }
    if(configs.find("vDensityMinPerRange") != configs.end()){
        config_.vDensityMinPerRange = stoi(configs["vDensityMinPerRange"]);
    }
    if(configs.find("UseStaticVDensity") != configs.end()){
        config_.UseStaticVDensity = stoi(configs["UseStaticVDensity"]);
    }
    if(configs.find("vDensity") != configs.end()){
        config_.vDensity = stoi(configs["vDensity"]);
    }
    if(configs.find("txCtrlInterval") != configs.end()){
        config_.txCtrlInterval = stoi(configs["txCtrlInterval"]);
    }
    if(configs.find("hvTEMinTimeDiff") != configs.end()){
        config_.hvTEMinTimeDiff = stoi(configs["hvTEMinTimeDiff"]);
    }
    if(configs.find("hvTEMaxTimeDiff") != configs.end()){
        config_.hvTEMaxTimeDiff = stoi(configs["hvTEMaxTimeDiff"]);
    }
    if(configs.find("rvTEMinTimeDiff") != configs.end()){
        config_.rvTEMinTimeDiff = stoi(configs["rvTEMinTimeDiff"]);
    }
    if(configs.find("rvTEMaxTimeDiff") != configs.end()){
        config_.rvTEMaxTimeDiff = stoi(configs["rvTEMaxTimeDiff"]);
    }
    if(configs.find("teErrSensitivity") != configs.end()){
        config_.teErrSensitivity = stoi(configs["teErrSensitivity"]);
    }
    if(configs.find("teMinThresh") != configs.end()){
        config_.teMinThresh = stod(configs["teMinThresh"]);
    }
    if(configs.find("teMaxThresh") != configs.end()){
        config_.teMaxThresh = stod(configs["teMaxThresh"]);
    }
    if(configs.find("minItt") != configs.end()){
        config_.minItt = stoi(configs["minItt"]);
    }
    if(configs.find("txRand") != configs.end()){
        config_.txRand = stoi(configs["txRand"]);
    }
    if(configs.find("timeAccuracy") != configs.end()){
        config_.timeAccuracy = stoi(configs["timeAccuracy"]);
    }
    if(configs.find("vMax_ITT") != configs.end()){
        config_.maxItt = stoi(configs["vMax_ITT"]);
    }
    if(configs.find("vRescheduleTh") != configs.end()){
        config_.reschedThresh = stoi(configs["vRescheduleTh"]);
    }
    if(configs.find("supraGain") != configs.end()){
        config_.supraGain = stod(configs["supraGain"]);
    }
    if(configs.find("minChanUtil") != configs.end()){
        config_.minChanUtil = stoi(configs["minChanUtil"]);
    }
    if(configs.find("maxChanUtil") != configs.end()){
        config_.maxChanUtil = stoi(configs["maxChanUtil"]);
    }
    if(configs.find("minRadiPwr") != configs.end()){
        config_.minRadiPwr = stoi(configs["minRadiPwr"]);
    }
    if(configs.find("maxRadiPwr") != configs.end()){
        config_.maxRadiPwr = stoi(configs["maxRadiPwr"]);
    }

    if (configs.find("enableSpsEnhancements") != configs.end()) {
        std::istringstream is(configs["enableSpsEnhancements"]);
        is >> std::boolalpha >> config_.enableSpsEnhancements;
        if (config_.enableSpsEnhancements) {
            std::cout << "SPS Enhancements Enabled\n";
        }
        if(configs.find("cv2xMaxITTRounding") != configs.end()){
            config_.cv2xMaxITTRounding = stoi(configs["cv2xMaxITTRounding"]);
        }

        if (configs.find("spsEnhIntervalRound") != configs.end()) {
            config_.spsEnhIntervalRound = stoi(configs["spsEnhIntervalRound"]);
        }
        if (configs.find("spsEnhHysterPerc") != configs.end()) {
            config_.spsEnhHysterPerc = stoi(configs["spsEnhHysterPerc"]);
        }
        if (configs.find("spsEnhDelayPerc") != configs.end()) {
            config_.spsEnhDelayPerc = stoi(configs["spsEnhDelayPerc"]);
        }

    }
}

Status CongestCtrl::init(const CongCtrlConfig &config, std::shared_ptr<RadioTransmit> spsTransmit)
{
    std::cout << " in cong ctrl init \n";
    congestCtrlMgr_ = V2xPropFactory::getInstance().getCongestionControlManager();
    std::cout << " got instance\n";
    std::cout << " got manager\n";
    congestCtrlListener_ = std::make_shared<CongestCtrlListener>(spsTransmit);
    std::cout << " made cong ctrl listener \n";
    congestCtrlMgr_->registerListener(congestCtrlListener_);
    std::cout << " registered listener\n";
    std::cout << " got past manager creation and listener registering \n";

   // CongestionControlUtility::setLoggingLevel(10);
/*     congestCtrlMgr_->updateCongestionControlType
               ((CongestionControlType)config.congCtrlType);
    CongestionControlUtility::setLoggingLevel(config.enableCongCtrlLogging);
    congestCtrlMgr_->updateCbpConfig(config.cbpWeightFactor,
            config.cbpMeasInterval);
    congestCtrlMgr_->enableSpsEnhancements(config.enableSpsEnhancements);
    congestCtrlMgr_->updatePERConfig(config.perMax,
            config.perInterval, config.perSubInterval);
    congestCtrlMgr_->updateDensConfig(config.vDensityCoefficient, config.vDensityWeightFactor,
            config.vDensityMinPerRange);
    congestCtrlMgr_->updateTeConfig(config.txCtrlInterval, config.hvTEMinTimeDiff,
               config.hvTEMaxTimeDiff, config.rvTEMinTimeDiff,
               config.rvTEMaxTimeDiff, config.teMinThresh,
               config.teMaxThresh, config.teErrSensitivity);
    congestCtrlMgr_->updateIttConfig(config.reschedThresh,
           config.timeAccuracy,config.minItt, config.maxItt, config.txRand);

    if(config.enableSpsEnhancements){
        congestCtrlMgr_->updateSpsEnhanceConfig(config.spsEnhIntervalRound, config.spsEnhDelayPerc,
                   config.spsEnhHysterPerc);
    } */
    std::cout << "starting cong ctrl\n";
    if(CCErrorCode::SUCCESS != congestCtrlMgr_->startCongestionControl()){
        std::cerr << "Congestion control manager failed start up\n";
        return Status::FAILED;
    }
    std::cout << " started \n";
    config_ = config;
    congestCtrlCbData_ = std::make_shared<CongestionControlUserData>();
    sem_wait(congestCtrlMgr_->getCongestionControlUserData()->congestionControlSem);
   // std::cout << "sem wait on cong ctr user data sem \n";
    return Status::SUCCESS;
}

bool CongestCtrl::addCongestionControlData(uint64_t id,
    double latitude, double longitude, double heading, double speed,
    uint64_t timestamp, uint64_t msgCount){
    CCErrorCode ccErr = congestCtrlMgr_->addCongestionControlData(
                                id,
                                latitude,
                                longitude,
                                heading,
                                speed,
                                timestamp,
                                msgCount);
    if(ccErr == CCErrorCode::SUCCESS){
        return true;
    }
    return false;
}

void CongestCtrl::writeLog(char* tmpLogStr, uint32_t maxBufSize, FILE *myfp, bool validPkt,
            uint16_t eventsData )
{
    CongestionControlCalculations ccCalculations;
    if(tmpLogStr == nullptr){
        std::cerr << "Invalid input buffer\n";
        return;
    }
    if (congestCtrlListener_ != nullptr) {
        ccCalculations = congestCtrlListener_->getCbData();
        char* tmpPtr = tmpLogStr;
        char* const endBuf = tmpPtr + maxBufSize;

        if (ccCalculations.trackingError) {
            tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
                ccCalculations.trackingError);
        }
        else {
            tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "0.0,");
        }
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            ccCalculations.smoothDens);


        if (ccCalculations.channData) {
            tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
                ccCalculations.channData->channQualInd);
        }
        else {
            tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "0.0,");
        }
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%d,",
            validPkt  ? 1: 0);
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%lu,",
            ccCalculations.maxITT);

        // gps time, event, random time
        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%f,",
            0.0);

        tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%u,", eventsData);

        //sps enhancement data
        //this->config.spsEnhHysterPerc
        if (ccCalculations.spsEnhanceData) {
            // random time - todo
            tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr, "%lu,%d",
                (long unsigned int)0, 5);
        }
        else {
           tmpPtr += snprintf(tmpPtr, endBuf-tmpPtr,  "%lu,%d",
                (long unsigned int)0, 5);
        }
    }
}
} //namespace Core
} //namespace SPLITS
