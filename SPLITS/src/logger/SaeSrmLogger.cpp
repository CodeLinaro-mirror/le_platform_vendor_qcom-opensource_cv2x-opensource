/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeSrmLogger.hpp"

namespace SPLITS {

void SaeSrmLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->SRM()));
}
void SaeSrmLogger::logMsg(const J2735::idl::SignalRequestMessage &srm)
{
    //place holder
}

} //namespace SPLITS
