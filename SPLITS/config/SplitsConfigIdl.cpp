/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <thread>
#include <CommonAPI/CommonAPI.hpp>
#include "SplitsConfigIdl.hpp"
#include "v1/commonapi/telux/common/CommonDefines.hpp"
using v1::commonapi::telux::common::CommonDefines;

namespace SPLITS {
namespace Config {

#define PROXY_TIMEOUT     5 // seconds
int SplitsConfigIdl::init(void)
{
    int count = 0;
    CommonAPI::Runtime::setProperty("LogContext", "SplitsService");
    CommonAPI::Runtime::setProperty("LogApplication", "SplitsService");
    CommonAPI::Runtime::setProperty("LibraryBase", "SplitsService");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.SplitsConfig";
    std::string connection = "SPLITS";

    proxy_ =
        runtime->buildProxy<capisplits::SplitsConfigProxy>(domain, instance, connection);

    while(!proxy_->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
        if (count == PROXY_TIMEOUT)
            break;
        else
            count++;
    }
    if (count < PROXY_TIMEOUT) {
        COMMONAPI_INFO("SplitsConfig proxy registration succesful");
    } else {
        COMMONAPI_ERROR("SplitsConfig Proxy not available");
        return -1;
    }
    return 0;
}
int SplitsConfigIdl::loadConfigFromIdl(void)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    CommonAPI::CallInfo info(1000);
    if (!proxy_->isAvailable()) {
        COMMONAPI_ERROR("Proxy not available");
        return -1;
    }
    CommonDefines::ErrorCode error;
    proxy_->readConfig(callStatus, error, *config_);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        COMMONAPI_ERROR("remote call failed");
        return -1;
    } else if (error != CommonDefines::ErrorCode::SUCCESS) {
        return -1;
    }
    return 0;
}
} //namespace Config
} //namespace SPLITS
