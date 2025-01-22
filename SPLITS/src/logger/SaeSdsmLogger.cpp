/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeSdsmLogger.hpp"

namespace SPLITS {

void SaeSdsmLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->SDSM()));
}
void SaeSdsmLogger::logMsg(const J2735::idl::SensorDataSharingMessage &sdsm)
{
    //place holder
}

} //namespace SPLITS
