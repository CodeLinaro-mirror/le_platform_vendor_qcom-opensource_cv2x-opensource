/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_RTCM_LOGGER_HPP__
#define __SAE_RTCM_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_RTCMcorrections.h"

namespace SPLITS {

class SaeRtcmLogger : public SaeCommonLogger {
public:
    SaeRtcmLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeRtcmLogger(){};
    void logMsg(std::shared_ptr<v2xMsgSae>msg);
    void logMsg(const J2735::idl::RTCMcorrection &rtcm);
};
} //namespace SPLITS
#endif //__SAE_RTCM_LOGGER_HPP__
