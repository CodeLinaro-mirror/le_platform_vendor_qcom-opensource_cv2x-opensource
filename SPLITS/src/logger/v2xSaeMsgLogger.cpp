/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "v2xSaeMsgLogger.hpp"
#include "SaeBsmLogger.hpp"
#include "SaeMapLogger.hpp"
#include "SaeSpatLogger.hpp"
#include "SaeSdsmLogger.hpp"
#include "SaeRsmLogger.hpp"
#include "SaeSrmLogger.hpp"
#include "SaeRtcmLogger.hpp"

using SPLITS::SaeBsmLogger;
using SPLITS::SaeMapLogger;
using SPLITS::SaeSpatLogger;
using SPLITS::SaeRtcmLogger;
using SPLITS::SaeRsmLogger;
using SPLITS::SaeSdsmLogger;
using SPLITS::SaeSrmLogger;

namespace SPLITS {

v2xSaeMsgLogger::v2xSaeMsgLogger(ostream *os) : v2xMsgLogger(os) {

    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_BSM, 
                std::unique_ptr<SaeBsmLogger>(new SaeBsmLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_MAP,
                std::unique_ptr<SaeMapLogger>(new SaeMapLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_SPAT,
                std::unique_ptr<SaeSpatLogger>(new SaeSpatLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_RTCM,
                std::unique_ptr<SaeRtcmLogger>(new SaeRtcmLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_RSM,
                std::unique_ptr<SaeRsmLogger>(new SaeRsmLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_SDSM,
                std::unique_ptr<SaeSdsmLogger>(new SaeSdsmLogger(os))));
    msgLoggers_.insert(std::make_pair(v2xMsgType::V2X_SAE_MSG_SRM,
                std::unique_ptr<SaeSrmLogger>(new SaeSrmLogger(os))));

}
void v2xSaeMsgLogger::logMsg(std::shared_ptr<v2xMsg> msg) {
    if (msg != nullptr) {
        msgLoggers_[msg->msgType_]->logMsg(std::dynamic_pointer_cast<v2xMsgSae>(msg));
    }
}

} //namespace SPLITS
