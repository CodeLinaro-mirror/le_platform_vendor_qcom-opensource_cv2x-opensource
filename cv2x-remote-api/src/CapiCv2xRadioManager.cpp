/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <thread>
#include <CommonAPI/CommonAPI.hpp>
#include "CapiCv2xRadioManager.hpp"
#include "CapiCv2xRadio.hpp"

void CapiCv2xRadioManager::init(InitResponseCb callback)
{
    asyncFuture_ = std::async(std::launch::async, [this, callback]() {
            this->initSync(callback);}).share();
}
void CapiCv2xRadioManager::initSync(InitResponseCb callback)
{
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.telux.cv2x.Cv2xRadioManager";
    proxy_ = 
        runtime->buildProxy<capicv2x::Cv2xRadioManagerProxy>(domain, instance, "client-Cv2xRadioManager");

    while (!proxy_->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    COMMONAPI_INFO("Cv2xRadioManager Proxy registration succesful");
    proxy_->getCurrentCv2xStatusEvent().subscribe([&](const capicv2x::Cv2xRadioTypes::Cv2xStatus &status) {
            Cv2xStatus st = CapiCv2xRadio::capi2teluxCv2xStatus(status);
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onStatusChanged(st);
                }
            }
    });

    if (callback) {
        //SERVICE_AWAILABLE only means we created proxy and found service on the service
        //provider, the caller still need to call "getServiceStatus" to find out if remote
        //RadioManager is in SERVICE_AWAILABLE state or not.
        //callback(ServiceStatus::SERVICE_AVAILABLE);
        callback(getServiceStatus());
    }
}
ServiceStatus CapiCv2xRadioManager::getServiceStatus(void)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::ServiceStatus status;
    ServiceStatus teluxServiceStatus = ServiceStatus::SERVICE_UNAVAILABLE;

    //sync call
    COMMONAPI_DEBUG("CapiCv2xRadioManager::getServiceStatus");
    proxy_->GetServiceStatus(callStatus, error, status);
    if (callStatus == CommonAPI::CallStatus::SUCCESS &&
            error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
        teluxServiceStatus = static_cast<ServiceStatus>(status.value_);
    } else {
        if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
            COMMONAPI_ERROR("method returned error ", error.toString());
        } else {
            COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
        }
    }
    COMMONAPI_DEBUG("CapiCv2xRadioManager::getServiceStatus:", static_cast<int>(teluxServiceStatus));
    return teluxServiceStatus;
}
void CapiCv2xRadioManager::onGetCv2xRadioResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::unique_lock<std::mutex> lock(radioMutex_);
        cv2xRadioInitStatus_ = status;
        if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            COMMONAPI_ERROR("Fail to initialize Cv2xRadio");
            radio_.reset(); // FAILED, radio_ reset to nullptr
        }
        cbs = cv2xRadioInitCallbacks_;
        cv2xRadioInitCallbacks_.clear();
        radioCv_.notify_all();
    }
    for (auto &cb : cbs) {
        if (cb) {
            COMMONAPI_DEBUG("calling callback vectors");
            cb(status);
        }
    }
}

std::shared_ptr<ICv2xRadio> CapiCv2xRadioManager::getCv2xRadio(TrafficCategory category,
          telux::common::InitResponseCb cb)
{
    std::shared_ptr<CapiCv2xRadio> radio = nullptr;

    std::lock_guard<std::mutex> lock(radioMutex_);
    radio = std::dynamic_pointer_cast<CapiCv2xRadio>(radio_.lock());
    if (radio &&  telux::common::ServiceStatus::SERVICE_FAILED != radio->getServiceStatus()) {
        // Radio has completed initialization and success, or has not complete initialization,
        // but not FAILED.
        if (cv2xRadioInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            // SERVICE_UNAVAILABLE is initial status
            COMMONAPI_DEBUG(" Cv2xRadio status is SERVICE_UNAVAILABLE");
            // radio initialization is triggered, wait for init result
            cv2xRadioInitCallbacks_.push_back(cb);
        } else {
            if (cb) {
                COMMONAPI_DEBUG(" Cv2xRadio status is SERVICE_AVAILABLE");
                auto f = [cb]() {
                         cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                };
                std::thread th(f);
                th.detach();
            }
            COMMONAPI_DEBUG(" returns existing radio");
            return radio;
        }
    }
    //no radio or radio in bad state. make a new instance
    COMMONAPI_DEBUG("creating new cv2x radio instance");
    try {
        radio = std::make_shared<CapiCv2xRadio>();
    } catch (std::bad_alloc &e) {
        COMMONAPI_ERROR("Failed to create radio: ", e.what());
        return nullptr;
    }
    cv2xRadioInitCallbacks_.push_back(cb);
    radio_ = radio;

    if (radio) {
        radio->init([this](ServiceStatus status) {
                onGetCv2xRadioResponse(status);
        });
    }
    return radio;
}

telux::common::Status CapiCv2xRadioManager::startCv2x(StartCv2xCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (cb == nullptr) {
        //sync call
        proxy_->startCv2x(callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        //Async call
        auto fcb = [&](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto rpcStatus = proxy_->startCv2xAsync(fcb);
        if (rpcStatus.get() != CommonAPI::CallStatus::SUCCESS) {
            //COMMONAPI_ERROR("RPC failure ", rpcStatus.get());
            teluxStatus =  Status::FAILED;
        } else {
            teluxStatus = Status::SUCCESS;
        }
    }

    return teluxStatus;
}

telux::common::Status CapiCv2xRadioManager::stopCv2x(StopCv2xCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (cb == nullptr) {
        //sync call
        proxy_->stopCv2x(callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        //Async call
        auto fcb = [&](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto rpcStatus = proxy_->stopCv2xAsync(fcb);
        callStatus = rpcStatus.get();
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            COMMONAPI_ERROR("RPC failure ", static_cast<int>(callStatus));
            teluxStatus =  Status::FAILED;
        } else {
            teluxStatus = Status::SUCCESS;
        }
    }

    return teluxStatus;
}
telux::common::Status CapiCv2xRadioManager::requestCv2xStatus(RequestCv2xStatusCallback cb)
{
    CommonAPI::CallStatus gCallStatus;
    capitelux::CommonDefines::ErrorCode error;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        COMMONAPI_ERROR("proxy is not available");
        return Status::NOTREADY;
    } else if (cb == nullptr) {
        return Status::INVALIDPARAM;
    }
    //Async only call
    auto fcb = [&](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode &err,
                const capicv2x::Cv2xRadioTypes::Cv2xStatus &cv2xStatus) {

            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(CapiCv2xRadio::capi2teluxCv2xStatus(cv2xStatus),
                        static_cast<ErrorCode>(err.value_));
            } else {
                cb(CapiCv2xRadio::capi2teluxCv2xStatus(cv2xStatus), 
                        ErrorCode::GENERIC_FAILURE);
            }
    };
    auto rpcStatus = proxy_->requestCv2xStatusAsync(fcb);
    gCallStatus = rpcStatus.get();
    if (gCallStatus != CommonAPI::CallStatus::SUCCESS) {
        COMMONAPI_ERROR("RPC failure ", static_cast<int>(gCallStatus));
        teluxStatus =  Status::FAILED;
    } else {
        teluxStatus = Status::SUCCESS;
    }
    return teluxStatus;
}

telux::common::Status CapiCv2xRadioManager::requestCv2xStatus(RequestCv2xStatusCallbackEx cb)
{
    CommonAPI::CallStatus gCallStatus;
    capitelux::CommonDefines::ErrorCode error;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    } else if (cb == nullptr) {
        return Status::INVALIDPARAM;
    }
    //Async only call
    auto fcb = [&](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode &err,
                const capicv2x::Cv2xRadioTypes::Cv2xStatusEx &cv2xStatus) {

            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(CapiCv2xRadio::capi2teluxCv2xStatusEx(cv2xStatus),
                        static_cast<ErrorCode>(err.value_));
            } else {
                cb(CapiCv2xRadio::capi2teluxCv2xStatusEx(cv2xStatus), 
                        ErrorCode::GENERIC_FAILURE);
            }
    };
    auto rpcStatus = proxy_->requestCv2xStatusExAsync(fcb);
    gCallStatus = rpcStatus.get();
    if (gCallStatus != CommonAPI::CallStatus::SUCCESS) {
        COMMONAPI_ERROR("RPC failure ", static_cast<int>(gCallStatus));
        teluxStatus =  Status::FAILED;
    } else {
        teluxStatus = Status::SUCCESS;
    }

    return teluxStatus;
}
telux::common::Status CapiCv2xRadioManager::registerListener(std::weak_ptr<ICv2xListener> listener)
{
    auto sp = listener.lock();

    if (sp == nullptr) {
        return Status::INVALIDPARAM;
    }
    std::lock_guard<std::mutex> lock(listenersMutex_);
    for (auto item : listeners_) {
        if (item.lock() == sp) {
            return Status::ALREADY;
        }
    }
    listeners_.push_back(listener);

    return Status::SUCCESS;
}

telux::common::Status CapiCv2xRadioManager::deregisterListener(std::weak_ptr<ICv2xListener> listener)
{
    std::lock_guard<std::mutex> lock(listenersMutex_);
    bool listenerExisted = false;
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (!sp) {
            it = listeners_.erase(it);
        } else if (sp == listener.lock()) {
            it = listeners_.erase(it);
            listenerExisted = true;
        } else {
            ++it;
        }
    }
    if (listenerExisted) {
        return Status::SUCCESS;
    } else  {
        return Status::NOSUCH;
    }
}
