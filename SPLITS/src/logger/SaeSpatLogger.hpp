/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_SPAT_LOGGER_HPP__
#define __SAE_SPAT_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_SPAT.h"

namespace SPLITS {

class SaeSpatLogger : public SaeCommonLogger {
public:
    SaeSpatLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeSpatLogger(){};
    void logMsg(std::shared_ptr<v2xMsgSae>msg) override;
    void logMsg(const J2735::idl::SPAT &spat) override;
};
} //namespace SPLITS
#endif //__SAE_SPAT_LOGGER_HPP__
