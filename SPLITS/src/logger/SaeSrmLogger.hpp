/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_SRM_LOGGER_HPP__
#define __SAE_SRM_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_SRM.h"

namespace SPLITS {

class SaeSrmLogger : public SaeCommonLogger {
public:
    SaeSrmLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeSrmLogger(){};
    void logMsg(std::shared_ptr<v2xMsgSae>msg);
    void logMsg(const J2735::idl::SignalRequestMessage &srm);
};
} //namespace SPLITS
#endif //__SAE_SRM_LOGGER_HPP__
