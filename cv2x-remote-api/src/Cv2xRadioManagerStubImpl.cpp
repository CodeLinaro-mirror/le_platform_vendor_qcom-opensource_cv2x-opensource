/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/cv2x/Cv2xFactory.hpp>
#include "Cv2xRadioManagerStubImpl.hpp"

using std::cout;
using std::endl;
using std::map;
using std::promise;
using std::string;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::Cv2xCauseType;
using telux::cv2x::Cv2xPoolStatus;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;

namespace capi = v1::commonapi::telux;
namespace capicv2x = v1::commonapi::telux::cv2x;

Cv2xRadioManagerStubImpl::Cv2xRadioManagerStubImpl() {

    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
            telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
            std::lock_guard<std::mutex> lock(mtx);
            cv2xRadioManagerStatusUpdated = true;
            cv2xRadioManagerStatus = status;
            cv.notify_all();
    };
    // Get handle to Cv2xRadioManager
    if (cv2xRadioManager_ == nullptr) {
        auto & cv2xFactory = Cv2xFactory::getInstance();
        cv2xRadioManager_ = cv2xFactory.getCv2xRadioManager(statusCb);
    }
    if (!cv2xRadioManager_) {
        COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: Failed to get Cv2xRadioManager");
    } else {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    }
}

Cv2xRadioManagerStubImpl::~Cv2xRadioManagerStubImpl() {
}

void Cv2xRadioManagerStubImpl::GetServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetServiceStatusReply_t _reply) {
    capi::common::CommonDefines::ErrorCode error =
        capi::common::CommonDefines::ErrorCode::RADIO_NOT_AVAILABLE;
    capi::common::CommonDefines::ServiceStatus capiServiceStatus =
        capi::common::CommonDefines::ServiceStatus::SERVICE_UNAVAILABLE;

    if (cv2xRadioManager_ != nullptr) {
        auto teluxServiceStatus = cv2xRadioManager_->getServiceStatus();
        error = capi::common::CommonDefines::ErrorCode::SUCCESS;
        capiServiceStatus = static_cast<
                capi::common::CommonDefines::ServiceStatus::Literal>(teluxServiceStatus);
    }

    _reply(error, capiServiceStatus);

};

void Cv2xRadioManagerStubImpl::startCv2x(const std::shared_ptr<CommonAPI::ClientId> _client, startCv2xReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;
    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        status = cv2xRadioManager_->startCv2x([&prom](ErrorCode code) {
            prom.set_value(code);
        });
        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }
    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
            static_cast<capi::common::CommonDefines::Status::Literal>(status));

}

void Cv2xRadioManagerStubImpl::stopCv2x(const std::shared_ptr<CommonAPI::ClientId> _client, stopCv2xReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;

    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        status = cv2xRadioManager_->stopCv2x([&prom](ErrorCode code) {
            prom.set_value(code);
        });
        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }
    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
            static_cast<capi::common::CommonDefines::Status::Literal>(status));
}

capicv2x::Cv2xRadioTypes::Cv2xStatus Cv2xRadioManagerStubImpl::telux2capiCv2xStatus(const Cv2xStatus &status)
{
        capicv2x::Cv2xRadioTypes::Cv2xStatus capiStatus(
            static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.rxStatus),
            static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.txStatus),
            static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.rxCause),
            static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.txCause),
            status.cbrValue,
            status.cbrValueValid);

        return capiStatus;
}
void Cv2xRadioManagerStubImpl::requestCv2xStatusEx(const std::shared_ptr<CommonAPI::ClientId> _client, requestCv2xStatusExReply_t _reply)
{

    Cv2xStatusEx gStatusEx;
    ErrorCode error;

    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto res = cv2xRadioManager_->requestCv2xStatus([&prom, &gStatusEx, this](Cv2xStatusEx status, ErrorCode code) {
            if (ErrorCode::SUCCESS == code) {
                std::lock_guard<std::mutex> lock(m_);
                gStatusEx = status;
            }
            prom.set_value(code);
        });
        if (res == Status::SUCCESS) {
            error = prom.get_future().get();
        }
        //Meanwhile, register a Cv2x status listener if it hasn't
        if (not isListenerRegistered) {
            if (Status::SUCCESS !=  cv2xRadioManager_->registerListener(getptr())) {
                COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: failed to register listener");
            }
            isListenerRegistered = true;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    capicv2x::Cv2xRadioTypes::Cv2xStatus capiStatus = Cv2xRadioManagerStubImpl::telux2capiCv2xStatus(gStatusEx.status);
    std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> capiPoolStatus;
    for (auto ps : gStatusEx.poolStatus) {
        capicv2x::Cv2xRadioTypes::Cv2xPoolStatus capips;
        capips.setPoolId(ps.poolId);
        capips.setStatus(Cv2xRadioManagerStubImpl::telux2capiCv2xStatus(ps.status));
        capiPoolStatus.push_back(capips);
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
            capicv2x::Cv2xRadioTypes::Cv2xStatusEx(capiStatus, capiPoolStatus,
                gStatusEx.timeUncertaintyValid, gStatusEx.timeUncertainty));
}
void Cv2xRadioManagerStubImpl::requestCv2xStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCv2xStatusReply_t _reply)
{
    Cv2xStatus gStatus;
    ErrorCode error;

    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto res = cv2xRadioManager_->requestCv2xStatus([&prom, &gStatus, this](Cv2xStatus status, ErrorCode code) {
            if (ErrorCode::SUCCESS == code) {
                std::lock_guard<std::mutex> lock(m_);
                gStatus = status;
            }
            prom.set_value(code);
        });
        if (res == Status::SUCCESS) {
            error = prom.get_future().get();
        }
        //Meanwhile, register a Cv2x status listener if it hasn't
        if (not isListenerRegistered) {
            if (Status::SUCCESS !=  cv2xRadioManager_->registerListener(getptr())) {
                COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: failed to register listener");
            }
            isListenerRegistered = true;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
            capicv2x::Cv2xRadioTypes::Cv2xStatus(
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(gStatus.rxStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(gStatus.txStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(gStatus.rxCause),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(gStatus.txCause),
                gStatus.cbrValue,
                gStatus.cbrValueValid));
}
void Cv2xRadioManagerStubImpl::onStatusChanged(Cv2xStatus status)
{
    //cast from telux Cv2xStatus to CommonAPI Cv2xStatus
    std::lock_guard<std::mutex> lock(m_);
    if (status.txStatus != currentStatus_.txStatus
        or status.rxStatus != currentStatus_.rxStatus
        or status.txCause != currentStatus_.txCause
        or status.rxCause != currentStatus_.rxCause) {
            currentStatus_ = status;
            std::cout << "fire event" << std::endl;
            fireCurrentCv2xStatusEvent(
                capicv2x::Cv2xRadioTypes::Cv2xStatus(
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.rxStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.txStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.rxCause),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.txCause),
                status.cbrValue,
                status.cbrValueValid)
        );
    }
}

void Cv2xRadioManagerStubImpl::setPeakTxPower(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    int8_t _txPower, setPeakTxPowerReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;

    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto setPeakTxPowerCallback = [&prom, this](ErrorCode code) {
            prom.set_value(code);
        };
        auto status = cv2xRadioManager_->setPeakTxPower(_txPower, setPeakTxPowerCallback);

        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
        static_cast<capi::common::CommonDefines::Status::Literal>(status));
}

void Cv2xRadioManagerStubImpl::setL2Filters(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    std::vector< ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::L2FilterInfo > _filterList,
    setL2FiltersReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;
    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto setL2FiltersCallback = [&prom, this](ErrorCode code) {
            prom.set_value(code);
        };
        std::vector<telux::cv2x::L2FilterInfo > teluxFilterList;
        for (auto info : _filterList) {
            telux::cv2x::L2FilterInfo l2FilterInfo;
            l2FilterInfo.srcL2Id = info.getSrcL2Id();
            l2FilterInfo.durationMs = info.getDurationMs();
            l2FilterInfo.pppp = info.getPppp();
            teluxFilterList.push_back(l2FilterInfo);
        }

        auto status = cv2xRadioManager_->setL2Filters(teluxFilterList, setL2FiltersCallback);
        
        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
        static_cast<capi::common::CommonDefines::Status::Literal>(status));
}

void Cv2xRadioManagerStubImpl::removeL2Filters(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    std::vector< uint32_t > _l2IdList,
    removeL2FiltersReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;
    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto removeL2FiltersCallback = [&prom, this](ErrorCode code) {
            prom.set_value(code);
        };

        auto status = cv2xRadioManager_->removeL2Filters(_l2IdList, removeL2FiltersCallback);

        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
        static_cast<capi::common::CommonDefines::Status::Literal>(status));
}

void Cv2xRadioManagerStubImpl::getSlssRxInfo(const std::shared_ptr<CommonAPI::ClientId> _client, getSlssRxInfoReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;

    capicv2x::Cv2xRadioTypes::SlssRxInfo capiSlssRxInfo;
    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;

        auto getSlssRxInfoCallback = [&capiSlssRxInfo, &prom, this](telux::cv2x::SlssRxInfo info, ErrorCode code) {
            std::vector<capicv2x::Cv2xRadioTypes::SyncRefUeInfo> capiUeInfo;
            for (auto ue: info.ueInfo) {
                capicv2x::Cv2xRadioTypes::SyncRefUeInfo capiSyncRefUeInfo;
                capiSyncRefUeInfo.setSlssId(ue.slssId);
                capiSyncRefUeInfo.setInCoverage(ue.inCoverage);
                capiSyncRefUeInfo.setPattern(static_cast<capicv2x::Cv2xRadioTypes::SlssSyncPattern::Literal>(ue.pattern));
                capiSyncRefUeInfo.setRsrp(ue.rsrp);
                capiSyncRefUeInfo.setSelected(ue.selected);
                capiUeInfo.push_back(capiSyncRefUeInfo);
            }
            capiSlssRxInfo.setUeInfo(capiUeInfo);
            prom.set_value(code);
        };

        auto status = cv2xRadioManager_->getSlssRxInfo(getSlssRxInfoCallback);

        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
        capiSlssRxInfo,
        static_cast<capi::common::CommonDefines::Status::Literal>(status));
}

void Cv2xRadioManagerStubImpl::injectCoarseUtcTime(const std::shared_ptr<CommonAPI::ClientId> _client, uint64_t _utc, injectCoarseUtcTimeReply_t _reply)
{
    ErrorCode error = ErrorCode::UNKNOWN;
    Status status = Status::FAILED;
    capi::common::CommonDefines::ServiceStatus capiServiceStatus =
        capi::common::CommonDefines::ServiceStatus::SERVICE_UNAVAILABLE;
    if (cv2xRadioManager_ != nullptr) {
        std::promise<ErrorCode> prom;
        auto injectCoarseUtcTimeCallback = [&prom, this](ErrorCode code) {
            prom.set_value(code);
        };

        auto teluxServiceStatus = cv2xRadioManager_->injectCoarseUtcTime(_utc, injectCoarseUtcTimeCallback);

        if (status == Status::SUCCESS) {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
        status = Status::INVALIDSTATE;
    }

    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error),
        static_cast<capi::common::CommonDefines::Status::Literal>(status));
}