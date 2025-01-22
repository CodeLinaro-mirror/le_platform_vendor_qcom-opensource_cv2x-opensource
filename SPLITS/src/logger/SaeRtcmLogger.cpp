/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeRtcmLogger.hpp"

namespace SPLITS {

void SaeRtcmLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->RTCMcorrection()));
}
void SaeRtcmLogger::logMsg(const J2735::idl::RTCMcorrection &rtcm)
{
    //place holder
}

} //namespace SPLITS
