/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <CommonAPI/CommonAPI.hpp>
#include "CapiCv2xRadioManager.hpp"
#include "CapiCv2xFactory.hpp"

Cv2xFactory::Cv2xFactory() {
    COMMONAPI_DEBUG("Cv2xFactory");
}
Cv2xFactory::~Cv2xFactory() {
    COMMONAPI_DEBUG("~Cv2xFactory");
}

Cv2xFactory & Cv2xFactory::getInstance() {
    return CapiCv2xFactory::getInstance();
}

std::shared_ptr<ICv2xRadioManager> Cv2xFactory::getCv2xRadioManager(
    telux::common::InitResponseCb cb) {
    return CapiCv2xFactory::getInstance().getCv2xRadioManager(cb);
}

Cv2xFactory & CapiCv2xFactory::getInstance() {
    CommonAPI::Runtime::setProperty("LogContext", "Cv2xRemoteAPI");
    CommonAPI::Runtime::setProperty("LogApplication", "Cv2xRemoteAPI");
    CommonAPI::Runtime::setProperty("LibraryBase", "Cv2xRemoteAPI");
    return getCapiCv2xFactory();
}
std::shared_ptr<ICv2xConfig> Cv2xFactory::getCv2xConfig(
    telux::common::InitResponseCb cb) {
    COMMONAPI_ERROR("api getCv2xConfig not available");
    return nullptr;
}
std::shared_ptr<ICv2xThrottleManager> Cv2xFactory::getCv2xThrottleManager(
    telux::common::InitResponseCb cb) {
    COMMONAPI_ERROR("api ggetCv2xThrottleManager not available");
    return nullptr;
}

CapiCv2xFactory& CapiCv2xFactory::getCapiCv2xFactory() {
    static CapiCv2xFactory instance;
    return instance;
}

CapiCv2xFactory::CapiCv2xFactory() {
    COMMONAPI_DEBUG("CapiCv2xFactory");
}

void CapiCv2xFactory::onGetCv2xRadioManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;

    std::lock_guard<std::mutex> lock(mutex_);
    cv2xManagerInitStatus_ = status;
    COMMONAPI_DEBUG("onGetCv2xRadioManagerResponse: status=", static_cast<int>(status));
    if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        COMMONAPI_ERROR("Failed to initialize Cv2xRadioManager");
    }
    cbs = cv2xManagerInitCallbacks_;
    cv2xManagerInitCallbacks_.clear();
    for (auto &cb : cbs) {
        if (cb) {
            cb(status);
        }
    }
}
std::shared_ptr<ICv2xRadioManager> CapiCv2xFactory::getCv2xRadioManager(
        telux::common::InitResponseCb cb)
{
    std::shared_ptr<ICv2xRadioManager> radioMgr = radioManager_.lock();
    std::lock_guard<std::mutex> lock(mutex_);

    if (not radioMgr) {
        std::shared_ptr<CapiCv2xRadioManager> cv2xRadioMgr = nullptr;
        try {
            cv2xRadioMgr = std::make_shared<CapiCv2xRadioManager>();
        } catch (std::bad_alloc& e) {
            COMMONAPI_ERROR("Fail to get Cv2xRadioManager due to ", e.what());
            return nullptr;
        }
        cv2xRadioMgr->init([this]( telux::common::ServiceStatus status) {
            onGetCv2xRadioManagerResponse(status);
        });
        radioManager_ = cv2xRadioMgr;
        radioMgr = cv2xRadioMgr;
        cv2xManagerInitCallbacks_.push_back(cb);
    } else if (cv2xManagerInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        cv2xManagerInitCallbacks_.push_back(cb);
    } else { // AVAILABLE or FAIL
        if (cb) {
            std::thread appCallback(cb, cv2xManagerInitStatus_);
            appCallback.detach();
        } else {
            COMMONAPI_INFO("Callback is NULL");
        }
    }

    return radioMgr;
}
