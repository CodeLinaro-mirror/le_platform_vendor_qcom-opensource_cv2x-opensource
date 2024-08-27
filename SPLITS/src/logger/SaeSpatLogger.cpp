/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeSpatLogger.hpp"

namespace SPLITS {

void SaeSpatLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->SPAT()));
}
void SaeSpatLogger::logMsg(const J2735::idl::SPAT &spat)
{
    //place holder
}

} //namespace SPLITS
