/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeRsmLogger.hpp"

namespace SPLITS {

void SaeRsmLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->RSM()));
}
void SaeRsmLogger::logMsg(const J2735::idl::RoadSafetyMessage &rsm)
{
    //place holder
}

} //namespace SPLITS
