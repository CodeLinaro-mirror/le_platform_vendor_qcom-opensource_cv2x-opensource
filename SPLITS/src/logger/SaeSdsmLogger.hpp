/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_SDSM_LOGGER_HPP__
#define __SAE_SDSM_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_SDSM.h"

namespace SPLITS {

class SaeSdsmLogger : public SaeCommonLogger {
public:
    SaeSdsmLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeSdsmLogger(){};
    void logMsg(std::shared_ptr<v2xMsgSae>msg);
    void logMsg(const J2735::idl::SensorDataSharingMessage &sdsm);
};
} //namespace SPLITS
#endif //__SAE_SDSM_LOGGER_HPP__
