/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "v2xCsaeMsgLogger.hpp"
#include "CsaeBsmLogger.hpp"

using SPLITS::CsaeBsmLogger;

namespace SPLITS {

v2xCsaeMsgLogger::v2xCsaeMsgLogger(ostream *os) : v2xMsgLogger(os) {
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_CSAE_MSG_BSM, 
                std::unique_ptr<CsaeBsmLogger>(new CsaeBsmLogger(os))));
    // add logger for the other msgs
#if 0
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_CSAE_MSG_MAP,
                std::unique_ptr<CsaeMapLogger>(new CsaeMapLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_CSAE_MSG_SPAT,
                std::unique_ptr<CsaeSpatLogger>(new CsaeSpatLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_CSAE_MSG_RSM,
                std::unique_ptr<CsaeRsmLogger>(new CsaeRsmLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_CSAE_MSG_RSI,
                std::unique_ptr<CsaeRsiLogger>(new CsaeRsiLogger(os))));
#endif
}

void v2xCsaeMsgLogger::logMsg(std::shared_ptr<v2xMsg> msg) {
    if (msg != nullptr) {
        msgLoggers_[msg->msgType_]->logMsg(std::dynamic_pointer_cast<v2xMsgCsae>(msg));
    }
}

} //namespace SPLITS
