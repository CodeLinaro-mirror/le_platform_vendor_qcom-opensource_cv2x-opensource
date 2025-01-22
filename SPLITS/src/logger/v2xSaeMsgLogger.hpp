
/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAEMSG_LOGGER_HPP__
#define __SAEMSG_LOGGER_HPP__

#include <iostream>
#include <map>
#include <memory>
#include "v2xMsgTypes.hpp"
#include "v2xMsgLogger.hpp"
#include "SaeCommonLogger.hpp"

namespace SPLITS {

class v2xSaeMsgLogger : public v2xMsgLogger {
public:
    v2xSaeMsgLogger(ostream *os);
    virtual ~v2xSaeMsgLogger(){}

    void logMsg(std::shared_ptr<SPLITS::v2xMsg> msg) override;

    void logMsg(const J2735::idl::BasicSafetyMessage &bsm) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_BSM]->logMsg(bsm);
    }
    void logMsg(const J2735::idl::MapData &map) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_MAP]->logMsg(map);
    }
    void logMsg(const J2735::idl::SPAT &spat) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_SPAT]->logMsg(spat);
    }
    void logMsg(const J2735::idl::RTCMcorrection &rtcm) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_RTCM]->logMsg(rtcm);
    }
    void logMsg(const J2735::idl::SensorDataSharingMessage &sdsm) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_SDSM]->logMsg(sdsm);
    }
    void logMsg(const J2735::idl::RoadSafetyMessage &rsm) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_RSM]->logMsg(rsm);
    }
    void logMsg(const J2735::idl::SignalRequestMessage &srm) {
        msgLoggers_[v2xMsgType::V2X_SAE_MSG_SRM]->logMsg(srm);
    }

private:
    std::map<SPLITS::v2xMsgType, std::unique_ptr<SaeCommonLogger>> msgLoggers_;

};
} //namespace SPLITS

#endif //__SAEMSG_LOGGER_HPP__
