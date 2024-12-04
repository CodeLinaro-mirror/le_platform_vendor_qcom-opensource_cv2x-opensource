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

Cv2xMgrListener::Cv2xMgrListener(RpcCv2xMgrFires fireMethods) {
    rpcFireMethods_ = fireMethods;
}

void Cv2xMgrListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    COMMONAPI_DEBUG("Cv2xMgrListener::onServiceStatusChange ", static_cast<int>(status));
}

void Cv2xMgrListener::onStatusChanged(telux::cv2x::Cv2xStatusEx statusEx) {
    COMMONAPI_DEBUG("Cv2xMgrListener::onStatusChanged");
    auto rpcOverallCv2xStatus = Cv2xRemoteServerHelper::telux2capiCv2xStatus(statusEx.status);

    std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> rpcPools;
    for (auto itr : statusEx.poolStatus) {
        capicv2x::Cv2xRadioTypes::Cv2xPoolStatus rpcPool(itr.poolId,
            Cv2xRemoteServerHelper::telux2capiCv2xStatus(itr.status));
        rpcPools.emplace_back(rpcPool);
    }

    capicv2x::Cv2xRadioTypes::Cv2xStatusEx rpcStatusEx(rpcOverallCv2xStatus, rpcPools,
        statusEx.timeUncertaintyValid, statusEx.timeUncertainty);
    rpcFireMethods_.fireCv2xStatus(rpcStatusEx);
}

void Cv2xMgrListener::onSlssRxInfoChanged(const telux::cv2x::SlssRxInfo& slssInfo) {
    COMMONAPI_DEBUG("Cv2xMgrListener::onSlssRxInfoChanged");
}


Cv2xRadioManagerStubImpl::Cv2xRadioManagerStubImpl() {
    RpcCv2xMgrFires fireMethods;
    fireMethods.fireCv2xStatus = std::bind(
        &capicv2x::Cv2xRadioManagerStubDefault::fireCv2xStatusExEvent, this,
        std::placeholders::_1);

    cv2xMgrListener_ = std::make_shared<Cv2xMgrListener>(fireMethods);
    if (!cv2xMgrListener_) {
        COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: Failed to make cv2xMgrListener_");
        return;
    }

    // Get handle to Cv2xRadioManager
    if (cv2xRadioManager_ == nullptr) {
        auto & cv2xFactory = Cv2xFactory::getInstance();
        cv2xRadioManager_ = cv2xFactory.getCv2xRadioManager(std::bind(
            &Cv2xMgrListener::onServiceStatusChange, cv2xMgrListener_, std::placeholders::_1));
    }
    if (!cv2xRadioManager_) {
        COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: Failed to get Cv2xRadioManager");
        return;
    }

    if (Status::SUCCESS != cv2xRadioManager_->registerListener(cv2xMgrListener_)) {
        COMMONAPI_ERROR("Cv2xRadioManagerStubImpl: failed to register listener");
    }
}

Cv2xRadioManagerStubImpl::~Cv2xRadioManagerStubImpl() {
    COMMONAPI_DEBUG("~Cv2xRadioManagerStubImpl");
    cv2xMgrListener_ = nullptr;
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
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    capicv2x::Cv2xRadioTypes::Cv2xStatus capiStatus =
        Cv2xRemoteServerHelper::telux2capiCv2xStatus(gStatusEx.status);
    std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> capiPoolStatus;
    for (auto ps : gStatusEx.poolStatus) {
        capicv2x::Cv2xRadioTypes::Cv2xPoolStatus capips;
        capips.setPoolId(ps.poolId);
        capips.setStatus(Cv2xRemoteServerHelper::telux2capiCv2xStatus(ps.status));
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
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    auto rpcCv2xStatus = Cv2xRemoteServerHelper::telux2capiCv2xStatus(gStatus);
    _reply(static_cast<capi::common::CommonDefines::ErrorCode::Literal>(error), rpcCv2xStatus);
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