/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <thread>
#include <CommonAPI/CommonAPI.hpp>
#include "CapiCv2xRadioManager.hpp"
#include "CapiCv2xRadio.hpp"

#define PROXY_TIMEOUT_MS 50000

namespace telux {

namespace cv2x {

std::shared_ptr<telux::common::AsyncTaskQueue<void>> clientTaskQ
    = std::make_shared<AsyncTaskQueue<void>>();

CapiCv2xRadioManager::CapiCv2xRadioManager()
{
    taskQ_ = clientTaskQ;
}

CapiCv2xRadioManager::~CapiCv2xRadioManager()
{
    COMMONAPI_DEBUG("CapiRadioManager::Descrustor");
    {
        std::lock_guard<std::mutex> lck(exitMutex_);
        exiting_ = true;
        exitCv_.notify_all();
    }

    proxy_ = nullptr;
}

void CapiCv2xRadioManager::init(InitResponseCb callback)
{
    myself_ = shared_from_this();
    auto f = [this, callback]() {
        this->initSync(callback);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        sp->add(f, myself_, std::launch::async);
    }
}
void CapiCv2xRadioManager::initSync(InitResponseCb callback)
{
    unsigned int count = PROXY_TIMEOUT_MS;
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.telux.cv2x.Cv2xRadioManager";
    proxy_ =
        runtime->buildProxy<capicv2x::Cv2xRadioManagerProxy>(domain, instance, "client-Cv2xRadioManager");

    const char *env_value = std::getenv("CV2X_REMOTE_CONNECT_TIMEOUT");
    if (!env_value) {
        COMMONAPI_INFO("Not set CV2X_REMOTE_CONNECT_TIMEOUT, use default value");
    } else {
        try {
            count = std::stoul(env_value);
        } catch (const std::invalid_argument& e) {
            COMMONAPI_ERROR("Invalid argument: ", e.what());
        } catch (const std::out_of_range& e) {
            COMMONAPI_ERROR("out of range: ", e.what());
        }
    }
    /*
     * Here we wait for the vsomeip layer client to connect to the service.
     * Default value is 500ms, which can be set by the environment variable
     * CV2X_REMOTE_CONNECT_TIMEOUT
    */
    for (unsigned int i = 0; i < count; i++) {
        std::unique_lock<std::mutex> lck(exitMutex_);
        if (not exitCv_.wait_for(lck, std::chrono::microseconds(10), [this] {
                return proxy_->isAvailable() || exiting_; })) {
            // Timeout will continue
            continue;
        } else {
            // Connection or progress exit will break
            break;
        }
    }

    if (!proxy_->isAvailable() || exiting_) {
        COMMONAPI_ERROR("Exit or connect timeout:proxy not available");
        if (callback) {
            callback(telux::common::ServiceStatus::SERVICE_FAILED);
        }
        return;
    }
    COMMONAPI_INFO("Cv2xRadioManager Proxy registration succesful");
    proxy_->getCurrentCv2xStatusEvent().subscribe([&](const capicv2x::Cv2xRadioTypes::Cv2xStatus &status) {
            Cv2xStatus st = Capi2teluxHelper::capi2teluxCv2xStatus(status);
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
    if (!proxy_->isAvailable()) {
        return teluxServiceStatus;
    }

    // sync call
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
        radio->init(taskQ_, [this](ServiceStatus status) {
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
        // sync call
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
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {
            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, fcb]() {
            proxy_->startCv2xAsync(fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
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
        // sync call
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
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, fcb]() {
            proxy_->stopCv2xAsync(fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
        }
    }

    return teluxStatus;
}
telux::common::Status CapiCv2xRadioManager::requestCv2xStatus(RequestCv2xStatusCallback cb)
{
    capitelux::CommonDefines::ErrorCode error;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        COMMONAPI_ERROR("proxy is not available");
        return Status::NOTREADY;
    } else if (cb == nullptr) {
        return Status::INVALIDPARAM;
    }
    // async only call
    auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capicv2x::Cv2xRadioTypes::Cv2xStatus &cv2xStatus) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(Capi2teluxHelper::capi2teluxCv2xStatus(cv2xStatus),
                        static_cast<ErrorCode>(err.value_));
            } else {
                cb(Capi2teluxHelper::capi2teluxCv2xStatus(cv2xStatus),
                        ErrorCode::GENERIC_FAILURE);
            }
    };
    auto f = [this, fcb]() {
        proxy_->requestCv2xStatusAsync(fcb);
    };
    auto sp = taskQ_.lock();
    if (sp) {
        teluxStatus = sp->add(f, myself_, std::launch::async);
    }
    return teluxStatus;
}

telux::common::Status CapiCv2xRadioManager::requestCv2xStatus(RequestCv2xStatusCallbackEx cb)
{
    capitelux::CommonDefines::ErrorCode error;
    Status teluxStatus = Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    } else if (cb == nullptr) {
        return Status::INVALIDPARAM;
    }
    // async only call
    auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capicv2x::Cv2xRadioTypes::Cv2xStatusEx &cv2xStatus) {

        if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
            cb(Capi2teluxHelper::capi2teluxCv2xStatusEx(cv2xStatus),
                    static_cast<ErrorCode>(err.value_));
        } else {
            cb(Capi2teluxHelper::capi2teluxCv2xStatusEx(cv2xStatus),
                    ErrorCode::GENERIC_FAILURE);
        }
    };
    auto f = [this, fcb]() {
        proxy_->requestCv2xStatusExAsync(fcb);
    };
    auto sp = taskQ_.lock();
    if (sp) {
        teluxStatus = sp->add(f, myself_, std::launch::async);
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

telux::common::Status CapiCv2xRadioManager::setPeakTxPower(int8_t txPower,
    telux::common::ResponseCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    telux::common::Status teluxStatus = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (cb == nullptr) {
        // sync call
        proxy_->setPeakTxPower(txPower, callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<telux::common::Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, txPower, fcb]() {
            proxy_->setPeakTxPowerAsync(txPower, fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
        }
    }

    return teluxStatus;
};

telux::common::Status CapiCv2xRadioManager::setL2Filters(
    const std::vector<telux::cv2x::L2FilterInfo> &filterList,
    telux::common::ResponseCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    telux::common::Status teluxStatus = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    std::vector<capicv2x::Cv2xRadioTypes::L2FilterInfo> capiL2FilterInfoList;
    for (auto info : filterList) {
        capicv2x::Cv2xRadioTypes::L2FilterInfo l2FilterInfo;
        l2FilterInfo.setSrcL2Id(info.srcL2Id);
        l2FilterInfo.setDurationMs(info.durationMs);
        l2FilterInfo.setPppp(info.pppp);
        capiL2FilterInfoList.push_back(l2FilterInfo);
    }

    if (cb == nullptr) {
        // sync call
        proxy_->setL2Filters(capiL2FilterInfoList, callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<telux::common::Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, capiL2FilterInfoList, fcb]() {
            proxy_->setL2FiltersAsync(capiL2FilterInfoList, fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
        }
    }

    return teluxStatus;
};

telux::common::Status CapiCv2xRadioManager::removeL2Filters(const std::vector<uint32_t> &l2IdList,
                                            telux::common::ResponseCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    telux::common::Status teluxStatus = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (cb == nullptr) {
        // sync call
        proxy_->removeL2Filters(l2IdList, callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<telux::common::Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, l2IdList, fcb]() {
            proxy_->removeL2FiltersAsync(l2IdList, fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
        }
    }

    return teluxStatus;
};

telux::common::Status CapiCv2xRadioManager::getSlssRxInfo(GetSlssRxInfoCallback cb)
{
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    telux::common::Status teluxStatus = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (!cb) {
        return Status::INVALIDPARAM;
    }

    // async call
    auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
            const capitelux::CommonDefines::ErrorCode &err,
            const capicv2x::Cv2xRadioTypes::SlssRxInfo &slssRxInfo,
            const capitelux::CommonDefines::Status &status) {
        telux::cv2x::SlssRxInfo teluxInfo = Capi2teluxHelper::capi2teluxSlssRxInfo(slssRxInfo);

        if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
            cb(teluxInfo, static_cast<ErrorCode>(err.value_));
        } else {
            cb(teluxInfo, ErrorCode::GENERIC_FAILURE);
        }
    };
    auto f = [this, fcb]() {
        proxy_->getSlssRxInfoAsync(fcb);
    };
    auto sp = taskQ_.lock();
    if (sp) {
        teluxStatus = sp->add(f, myself_, std::launch::async);
    }

    return teluxStatus;
};

telux::common::Status CapiCv2xRadioManager::injectCoarseUtcTime(uint64_t utc, telux::common::ResponseCallback cb)
{
    CommonAPI::CallStatus callStatus;
    capitelux::CommonDefines::ErrorCode error;
    capitelux::CommonDefines::Status status;
    telux::common::Status teluxStatus = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (cb == nullptr) {
        // sync call
        proxy_->injectCoarseUtcTime(utc, callStatus, error, status);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
                error == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
            teluxStatus = static_cast<telux::common::Status>(status.value_);
        } else {
            if (error != capitelux::CommonDefines::ErrorCode::SUCCESS) {
                COMMONAPI_ERROR("method returned error ", error.toString());
            } else {
                COMMONAPI_ERROR("RPC failure:", static_cast<int>(callStatus));
            }
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callStatusAsync,
                const capitelux::CommonDefines::ErrorCode &err,
                const capitelux::CommonDefines::Status &status) {

            if (callStatusAsync == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, utc, fcb]() {
            proxy_->injectCoarseUtcTimeAsync(utc, fcb);
        };
        auto sp = taskQ_.lock();
        if (sp) {
            teluxStatus = sp->add(f, myself_, std::launch::async);
        }
    }

    return teluxStatus;
};

}  // namespace cv2x

}  // namespace telux
