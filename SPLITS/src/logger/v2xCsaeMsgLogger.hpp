/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CSAEMSG_LOGGER_HPP__
#define __CSAEMSG_LOGGER_HPP__

#include <iostream>
#include <map>
#include <memory>
#include "v2xMsgTypes.hpp"
#include "v2xMsgLogger.hpp"
#include "CsaeCommonLogger.hpp"

namespace SPLITS {

class v2xCsaeMsgLogger : public v2xMsgLogger {
public:
    v2xCsaeMsgLogger(ostream *os);
    virtual ~v2xCsaeMsgLogger(){}

    void logMsg(std::shared_ptr<SPLITS::v2xMsg> msg) override;

    void logMsg(const Csae::idl::BasicSafetyMessage &bsm) {
        msgLoggers_[v2xMsgType::V2X_CSAE_MSG_BSM]->logMsg(bsm);
    }
    void logMsg(const Csae::idl::MapData &map) {
        msgLoggers_[v2xMsgType::V2X_CSAE_MSG_MAP]->logMsg(map);
    }
    void logMsg(const Csae::idl::SPAT &spat) {
        msgLoggers_[v2xMsgType::V2X_CSAE_MSG_SPAT]->logMsg(spat);
    }
    void logMsg(const Csae::idl::RoadsideSafetyMessage &rsm) {
        msgLoggers_[v2xMsgType::V2X_CSAE_MSG_RSM]->logMsg(rsm);
    }
    void logMsg(const Csae::idl::RoadSideInformation &rsi) {
        msgLoggers_[v2xMsgType::V2X_CSAE_MSG_RSI]->logMsg(rsi);
    }

private:
    std::map<SPLITS::v2xMsgType, std::unique_ptr<CsaeCommonLogger>> msgLoggers_;

};
} //namespace SPLITS

#endif //__CSAEMSG_LOGGER_HPP__
