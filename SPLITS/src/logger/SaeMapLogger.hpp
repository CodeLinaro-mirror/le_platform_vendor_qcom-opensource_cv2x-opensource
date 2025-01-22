/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_MAP_LOGGER_HPP__
#define __SAE_MAP_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_MapData.h"

namespace SPLITS {

class SaeMapLogger : public SaeCommonLogger {
public:
    SaeMapLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeMapLogger(){};

    void logMsg(std::shared_ptr<v2xMsgSae>msg) override;
    void logMsg(const J2735::idl::MapData &map) override;
};
} //namespace SPLITS
#endif //__SAE_MAP_LOGGER_HPP__
