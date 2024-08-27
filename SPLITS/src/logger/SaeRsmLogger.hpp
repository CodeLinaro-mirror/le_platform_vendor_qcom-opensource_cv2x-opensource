/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_RSM_LOGGER_HPP__
#define __SAE_RSM_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_RSM.h"

namespace SPLITS {

class SaeRsmLogger : public SaeCommonLogger {
public:
    SaeRsmLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeRsmLogger(){};
    void logMsg(std::shared_ptr<v2xMsgSae>msg);
    void logMsg(const J2735::idl::RoadSafetyMessage &rsm);
};
} //namespace SPLITS
#endif //__SAE_RSM_LOGGER_HPP__
