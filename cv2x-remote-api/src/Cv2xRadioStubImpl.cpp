/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <telux/cv2x/Cv2xFactory.hpp>
#include "Cv2xRadioStubImpl.hpp"

using std::cout;
using std::endl;
using std::map;
using std::promise;
using std::string;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::common::ServiceStatus;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::Cv2xCauseType;
using telux::cv2x::Cv2xPoolStatus;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::ICv2xRadioManager;

Cv2xRemoteSrvRadioListener::Cv2xRemoteSrvRadioListener(RpcRadioFires fireMethods) {
    COMMONAPI_DEBUG(__FUNCTION__);
    rpcRadionFires_ = fireMethods;
}

void Cv2xRemoteSrvRadioListener::onStatusChanged(telux::cv2x::Cv2xStatusEx statusEx) {
    COMMONAPI_DEBUG("Cv2xRadioListener::onStatusChanged");
    auto rpcOverallCv2xStatus = Cv2xRemoteServerHelper::telux2capiCv2xStatus(statusEx.status);

    std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> rpcPools;
    for (auto itr : statusEx.poolStatus) {
        capicv2x::Cv2xRadioTypes::Cv2xPoolStatus rpcPool(itr.poolId,
            Cv2xRemoteServerHelper::telux2capiCv2xStatus(itr.status));
        rpcPools.emplace_back(rpcPool);
    }

    capicv2x::Cv2xRadioTypes::Cv2xStatusEx rpcStatusEx(rpcOverallCv2xStatus, rpcPools,
        statusEx.timeUncertaintyValid, statusEx.timeUncertainty);
    rpcRadionFires_.fireCv2xStatus(rpcStatusEx);
}

void Cv2xRemoteSrvRadioListener::onL2AddrChanged(uint32_t newL2Addr) {
    rpcRadionFires_.fireL2AddrEvent(newL2Addr);
}

void Cv2xRemoteSrvRadioListener::onSpsOffsetChanged(int spsId, telux::cv2x::MacDetails details) {

    rpcRadionFires_.fireSpsOffsetEvent(spsId,
            capicv2x::Cv2xRadioTypes::MacDetails(details.periodicityInUseNs,
                details.currentlyReservedPeriodicBytes,
                details.txReservationOffsetNs));
}

void Cv2xRemoteSrvRadioListener::onSpsSchedulingChanged(
    const telux::cv2x::SpsSchedulingInfo & schedulingInfo) {
    //TODO: fire event not defined
}

void Cv2xRemoteSrvRadioListener::onCapabilitiesChanged(
    const telux::cv2x::Cv2xRadioCapabilities & capabilities) {
    std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> txPoolIdsSupported;
    for (telux::cv2x::TxPoolIdInfo teluxPoolIdInfo : capabilities.txPoolIdsSupported) {
        capicv2x::Cv2xRadioTypes::TxPoolIdInfo capiPoolIdInfo(teluxPoolIdInfo.poolId,
                teluxPoolIdInfo.minFreq, teluxPoolIdInfo.maxFreq);
        txPoolIdsSupported.push_back(capiPoolIdInfo);
    }
    rpcRadionFires_.fireCapabilitiesEvent(
            capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities(capabilities.linkIpMtuBytes,
                capabilities.linkNonIpMtuBytes,
                static_cast<capicv2x::Cv2xRadioTypes::RadioConcurrencyMode::Literal>(
                    capabilities.maxSupportedConcurrency),
                capabilities.nonIpTxPayloadOffsetBytes,
                capabilities.nonIpRxPayloadOffsetBytes,
                Cv2xRemoteServerHelper::bitset2Uint8(capabilities.periodicitiesSupported),
                capabilities.periodicities,
                capabilities.maxNumAutoRetransmissions,
                capabilities.layer2MacAddressSize,
                Cv2xRemoteServerHelper::bitset2Uint8(capabilities.prioritiesSupported),
                capabilities.maxNumSpsFlows,
                capabilities.maxNumNonSpsFlows,
                capabilities.maxTxPower,
                capabilities.minTxPower,
                txPoolIdsSupported,
                capabilities.isUnicastSupported));
}

void Cv2xRemoteSrvRadioListener::onMacAddressCloneAttack(const bool detected) {
    rpcRadionFires_.fireMacAddrCloneEvent(detected);
}


Cv2xRadioStubImpl::Cv2xRadioStubImpl() {
    ErrorCode error;
    bool cv2xRadioManagerStatusUpdated = false;
    ServiceStatus cv2xRadioManagerStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    ServiceStatus cv2xRadioStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](ServiceStatus status) {
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
        COMMONAPI_ERROR("Cv2xRadioStubImpl: Failed to get Cv2xRadioManager");
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    } else {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        error = ErrorCode::SUCCESS;
    }

    if (error == ErrorCode::SUCCESS &&
            cv2xRadioManagerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        //get handle to Cv2xRadio
        bool cv2xRadioStatusUpdated = false;
        auto radioCb = [&](ServiceStatus status) {
            std::lock_guard<std::mutex> lock(mtx);
            cv2xRadioStatusUpdated = true;
            cv2xRadioStatus = status;
            cv.notify_all();
        };
        //TODO 
        cv2xRadio_ = cv2xRadioManager_->getCv2xRadio(telux::cv2x::TrafficCategory::SAFETY_TYPE, radioCb);
        if (cv2xRadio_ == nullptr ) {
            COMMONAPI_ERROR("Cv2xRadioStubImpl: Failed to get Cv2xRadio");
            error = ErrorCode::RADIO_NOT_AVAILABLE;
        } else {
            initListener(cv2xRadio_);
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck, [&] { return cv2xRadioStatusUpdated; });
            error = ErrorCode::SUCCESS;
        }
    }
}

Cv2xRadioStubImpl::~Cv2xRadioStubImpl() {
    //TODO: empty the FlowInfo, and RxSub
}

void Cv2xRadioStubImpl::initListener(std::shared_ptr<telux::cv2x::ICv2xRadio> radio) {
    RpcRadioFires fireMethods;
    fireMethods.fireCv2xStatus = std::bind(&capicv2x::Cv2xRadioStubDefault::fireCv2xStatusExEvent,
        this, std::placeholders::_1);
    fireMethods.fireL2AddrEvent = std::bind(&capicv2x::Cv2xRadioStubDefault::fireCurrentL2AddrEvent,
        this, std::placeholders::_1);
    fireMethods.fireSpsOffsetEvent = std::bind(
        &capicv2x::Cv2xRadioStubDefault::fireCurrentSpsOffsetEvent, this, std::placeholders::_1,
        std::placeholders::_2);
    fireMethods.fireCapabilitiesEvent = std::bind(
        &capicv2x::Cv2xRadioStubDefault::fireCurrentCapabilitiesEvent, this, std::placeholders::_1);
    fireMethods.fireMacAddrCloneEvent = std::bind(
        &capicv2x::Cv2xRadioStubDefault::fireMacAddrCloneEvent, this, std::placeholders::_1);

    radioListener_ = std::make_shared<Cv2xRemoteSrvRadioListener>(fireMethods);
    if (radioListener_ == nullptr) {
        COMMONAPI_ERROR("Cv2xRadioStubImpl: initListener Failed.");
        return;
    }
    if (radio) {
        radio->registerListener(radioListener_);
    }
}

void Cv2xRadioStubImpl::getServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        getServiceStatusReply_t _reply) {
    capiteluxcommon::CommonDefines::ErrorCode error =
        capiteluxcommon::CommonDefines::ErrorCode::RADIO_NOT_AVAILABLE;
    capiteluxcommon::CommonDefines::ServiceStatus capiServiceStatus =
        capiteluxcommon::CommonDefines::ServiceStatus::SERVICE_UNAVAILABLE;

    if (cv2xRadio_ != nullptr) {
        auto teluxServiceStatus = cv2xRadio_->getServiceStatus();
        error = capiteluxcommon::CommonDefines::ErrorCode::SUCCESS;
        capiServiceStatus = static_cast<
                capiteluxcommon::CommonDefines::ServiceStatus::Literal>(teluxServiceStatus);
    }
    _reply(error, capiServiceStatus);

};

void Cv2xRadioStubImpl::createRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
            uint16_t _port, std::vector< uint32_t > _idList, createRxSubscriptionReply_t _reply)
{
    uint32_t subscriptionId = 0;
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    std::shared_ptr<std::vector<uint32_t>> idList;

    if (_idList.size() == 0) {
        idList = nullptr;
    } else {
        idList = std::make_shared<std::vector< uint32_t >>(_idList);
    }
    auto closeRxSubCallback = [this](std::shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        COMMONAPI_DEBUG("rxsub is closed with error: ", static_cast<int>(error));
    };


    auto createRxSubscriptionCallback = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                                               ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            subscriptionId = rxSub->getSubscriptionId();
            //Create cv2x dds data publisher
            std::shared_ptr<Cv2xDataPublisher> pub = std::make_shared<Cv2xDataPublisher>();
            if (pub->init(rxSub) != true) {
                COMMONAPI_ERROR("Failed to init dds publisher");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
                cv2xRadio_->closeRxSubscription(rxSub, closeRxSubCallback);
            } else {
                pubs_[subscriptionId] = pub;
                rxSubs_[subscriptionId] = rxSub;
                COMMONAPI_DEBUG("rxSub and dds pub for sub ", subscriptionId, " created");
            }
        }
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->createRxSubscription(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                _port,
                createRxSubscriptionCallback,
                idList);

        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), 
            subscriptionId);

}

void Cv2xRadioStubImpl::enableRxMetaDataReport(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
            bool _enable, std::vector< uint32_t > _idList, enableRxMetaDataReportReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    auto enableRxMetaDataCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->enableRxMetaDataReport(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                _enable, std::make_shared<std::vector<uint32_t>>(_idList), enableRxMetaDataCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));

}

void Cv2xRadioStubImpl::createTxSpsFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType     _ipType,
            uint32_t _serviceId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SpsFlowInfo _spsInfo, 
            uint16_t _spsSrcPort, bool _eventSrcPortValid, uint16_t _eventSrcPort, createTxSpsFlowReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    uint32_t spsFlowId, eventFlowId;

    auto closeTxFlowCallback = [this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow, ErrorCode error) {
        COMMONAPI_DEBUG("close flow with error : ", static_cast<int>(error));
    };

    if (cv2xRadio_ != nullptr) {
        auto createTxSpsFlowCallback = [&](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txSpsFlow,
                                            std::shared_ptr<telux::cv2x::ICv2xTxFlow> txEventFlow,
                                            ErrorCode spsError,
                                            ErrorCode eventError) {
            if (ErrorCode::SUCCESS == spsError) {
                spsFlowId = txSpsFlow->getFlowId();
                //create data subscriber to receive data from client.
                std::shared_ptr<Cv2xDataSubscriber> spsSub = std::make_shared<Cv2xDataSubscriber>();
                 if (spsSub->init(txSpsFlow) != true) {
                     COMMONAPI_ERROR("Failed to init dds subscriber for sps flow");
                     error = telux::common::ErrorCode::GENERIC_FAILURE;
                     auto status = cv2xRadio_->closeTxFlow(txSpsFlow, closeTxFlowCallback);
                 } else {
                     subs_[spsFlowId] = spsSub;
                     txFlows_[spsFlowId] = txSpsFlow;
                 }
            }
            if (_eventSrcPortValid ==true && ErrorCode::SUCCESS == eventError) {
                eventFlowId = txEventFlow->getFlowId();
                std::shared_ptr<Cv2xDataSubscriber> eventSub = std::make_shared<Cv2xDataSubscriber>();
                 if (eventSub->init(txEventFlow) != true) {
                     COMMONAPI_ERROR("failed to init dds subscriber for sps flow");
                     error = telux::common::ErrorCode::GENERIC_FAILURE;
                     auto status = cv2xRadio_->closeTxFlow(txEventFlow, closeTxFlowCallback);
                 } else {
                     subs_[eventFlowId] = eventSub;
                     txFlows_[eventFlowId] = txEventFlow;
                 }
            }
            if (_eventSrcPortValid == false) {
                prom.set_value(spsError);
            } else {
                if (spsError != ErrorCode::SUCCESS) {
                    prom.set_value(spsError);
                } else if (eventError != ErrorCode::SUCCESS) {
                    prom.set_value(eventError);
                } else {
                    prom.set_value(ErrorCode::SUCCESS);
                }
            }
        };
        telux::cv2x::SpsFlowInfo spsInfo;
        spsInfo.priority = static_cast<telux::cv2x::Priority>(_spsInfo.getPriority().value_);
        spsInfo.periodicity = static_cast<telux::cv2x::Periodicity>(_spsInfo.getPeriodicity().value_);
        spsInfo.periodicityMs = _spsInfo.getPeriodicityMs();
        spsInfo.nbytesReserved = _spsInfo.getNbytesReserved();
        spsInfo.autoRetransEnabledValid = _spsInfo.getAutoRetransEnabledValid();
        spsInfo.autoRetransEnabled = _spsInfo.getAutoRetransEnabled();
        spsInfo.peakTxPowerValid = _spsInfo.getPeakTxPowerValid();
        spsInfo.peakTxPower = _spsInfo.getPeakTxPower();
        spsInfo.mcsIndexValid = _spsInfo.getMcsIndexValid();
        spsInfo.mcsIndex = _spsInfo.getMcsIndex();
        spsInfo.txPoolIdValid = _spsInfo.getTxPoolIdValid();
        spsInfo.txPoolId = _spsInfo.getTxPoolId();
        auto status = cv2xRadio_->createTxSpsFlow(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                                    _serviceId,
                                    spsInfo,
                                    _spsSrcPort,
                                    _eventSrcPortValid,
                                    _eventSrcPort,
                                    createTxSpsFlowCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    //TODO: txSpsError and txEventError in reply
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), spsFlowId, eventFlowId,
            static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error),
            static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::createTxEventFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
            uint32_t _serviceId,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _flowInfo,
            uint16_t _eventSrcPort, createTxEventFlowReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    uint32_t eventFlowId;

    auto closeTxFlowCallback = [this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow, ErrorCode error) {
        COMMONAPI_DEBUG("close flow with error : ", static_cast<int>(error));
    };

    if (cv2xRadio_ != nullptr) {
        auto createTxEventFlowCallback = [&](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txEventFlow,
                                            ErrorCode error) {
            if (ErrorCode::SUCCESS == error) {
                eventFlowId = txEventFlow->getFlowId();
                //create data subscriber to receive data from client.
                std::shared_ptr<Cv2xDataSubscriber> sub = std::make_shared<Cv2xDataSubscriber>();
                 if (sub->init(txEventFlow) != true) {
                     COMMONAPI_ERROR("failed to init dds subscriber");
                     error = telux::common::ErrorCode::GENERIC_FAILURE;
                     auto status = cv2xRadio_->closeTxFlow(txEventFlow, closeTxFlowCallback);
                 } else {
                     subs_[eventFlowId] = sub;
                     txFlows_[eventFlowId] = txEventFlow;
                 }
            }
            prom.set_value(error);
        };
        telux::cv2x::EventFlowInfo flowInfo;
        flowInfo.autoRetransEnabledValid = _flowInfo.getAutoRetransEnabledValid();
        flowInfo.autoRetransEnabled = _flowInfo.getAutoRetransEnabled();
        flowInfo.peakTxPowerValid = _flowInfo.getPeakTxPowerValid();
        flowInfo.peakTxPower = _flowInfo.getPeakTxPower();
        flowInfo.mcsIndexValid = _flowInfo.getMcsIndexValid();
        flowInfo.mcsIndex = _flowInfo.getMcsIndex();
        flowInfo.txPoolIdValid = _flowInfo.getTxPoolIdValid();
        flowInfo.txPoolId = _flowInfo.getTxPoolId();
        flowInfo.isUnicast = _flowInfo.getIsUnicast();
        auto status = cv2xRadio_->createTxEventFlow(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                                    _serviceId,
                                    flowInfo,
                                    _eventSrcPort,
                                    createTxEventFlowCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), eventFlowId);
}

void Cv2xRadioStubImpl::closeRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _subscriptionId, closeRxSubscriptionReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    auto closeRxSubCallback = [&prom, this](std::shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (rxSubs_.find(_subscriptionId) != rxSubs_.end()) {
            if (shutdown(rxSubs_[_subscriptionId]->getSock(), SHUT_RD) == -1) {
                int temp_error = errno;
                if (temp_error != ENOTCONN) {
                    COMMONAPI_ERROR("Failed to shutdown read direction ", temp_error);
                }
            }
            auto status = cv2xRadio_->closeRxSubscription(rxSubs_[_subscriptionId], closeRxSubCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    COMMONAPI_DEBUG("deleting rxsub ", _subscriptionId);
                    rxSubs_.erase(_subscriptionId);
                    pubs_.erase(_subscriptionId);
                }
            }
        } else {
            COMMONAPI_ERROR(":closeRxSubscription subId ", _subscriptionId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }

    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}
void Cv2xRadioStubImpl::closeTxFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _txFlowId, closeTxFlowReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    auto closeTxFlowCallback = [&prom, this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow, ErrorCode error) {
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->closeTxFlow(txFlows_[_txFlowId], closeTxFlowCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    COMMONAPI_DEBUG("deleting txflow ", _txFlowId);
                    txFlows_.erase(_txFlowId);
                    subs_.erase(_txFlowId);
                }
            }
        } else {
            COMMONAPI_ERROR("closeTxFlow: flowId ", _txFlowId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl:: changeSpsFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _txFlowId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SpsFlowInfo _spsInfo,
             changeSpsFlowInfoReply_t _reply)

{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::SpsFlowInfo teluxSpsInfo;

    teluxSpsInfo.priority = static_cast<telux::cv2x::Priority>((uint8_t)_spsInfo.getPriority());
    teluxSpsInfo.periodicity = static_cast<telux::cv2x::Periodicity>((uint8_t)_spsInfo.getPeriodicity());
    teluxSpsInfo.periodicityMs = _spsInfo.getPeriodicityMs();
    teluxSpsInfo.nbytesReserved = _spsInfo.getNbytesReserved();
    teluxSpsInfo.autoRetransEnabledValid = _spsInfo.getAutoRetransEnabledValid();
    teluxSpsInfo.autoRetransEnabled = _spsInfo.getAutoRetransEnabled();
    teluxSpsInfo.peakTxPowerValid = _spsInfo.getPeakTxPowerValid();
    teluxSpsInfo.peakTxPower = _spsInfo.getPeakTxPower();
    teluxSpsInfo.mcsIndexValid = _spsInfo.getMcsIndexValid();
    teluxSpsInfo.mcsIndex = _spsInfo.getMcsIndex();
    teluxSpsInfo.txPoolIdValid = _spsInfo.getTxPoolIdValid();
    teluxSpsInfo.txPoolId = _spsInfo.getTxPoolId();

    auto changeSpsFlowInfoCallback = [&prom, this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txSpsFlow, ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->changeSpsFlowInfo(txFlows_[_txFlowId], teluxSpsInfo,
                    changeSpsFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
            }
        } else {
            COMMONAPI_ERROR("changeSpsFlowInfo: flowId ", _txFlowId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl:: requestSpsFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _txFlowId, requestSpsFlowInfoReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    capicv2x::Cv2xRadioTypes::SpsFlowInfo capiSpsInfo;
    auto requestSpsFlowInfoCallback = [&prom, &capiSpsInfo, this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txSpsFlow,
            const telux::cv2x::SpsFlowInfo& spsInfo,
            ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            capiSpsInfo.setPriority(static_cast<capicv2x::Cv2xRadioTypes::Priority::Literal>(spsInfo.priority));
            capiSpsInfo.setPeriodicity(static_cast<capicv2x::Cv2xRadioTypes::Periodicity::Literal>(spsInfo.periodicity));
            capiSpsInfo.setPeriodicityMs(spsInfo.periodicityMs);
            capiSpsInfo.setNbytesReserved(spsInfo.nbytesReserved);
            capiSpsInfo.setAutoRetransEnabledValid(spsInfo.autoRetransEnabledValid);
            capiSpsInfo.setAutoRetransEnabled(spsInfo.autoRetransEnabled);
            capiSpsInfo.setPeakTxPowerValid(spsInfo.peakTxPowerValid);
            capiSpsInfo.setPeakTxPower(spsInfo.peakTxPower);
            capiSpsInfo.setMcsIndexValid(spsInfo.mcsIndexValid);
            capiSpsInfo.setMcsIndex(spsInfo.mcsIndex);
            capiSpsInfo.setTxPoolIdValid(spsInfo.txPoolIdValid);
            capiSpsInfo.setTxPoolId(spsInfo.txPoolId);
        }
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            COMMONAPI_DEBUG("requestSpsFlowInfo: flowId ", _txFlowId);
            auto status = cv2xRadio_->requestSpsFlowInfo(txFlows_[_txFlowId], requestSpsFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
                COMMONAPI_DEBUG("requestSpsFlowInfo: ERROR ", static_cast<int>(error));
            }
        } else {
            COMMONAPI_ERROR("requestSpsFlowInfo: flowId ", _txFlowId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), capiSpsInfo);
}

void Cv2xRadioStubImpl:: changeEventFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _txFlowId,
             ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _eventInfo,
             changeEventFlowInfoReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::EventFlowInfo teluxEventInfo;

    teluxEventInfo.autoRetransEnabledValid = _eventInfo.getAutoRetransEnabledValid();
    teluxEventInfo.autoRetransEnabled = _eventInfo.getAutoRetransEnabled();
    teluxEventInfo.peakTxPowerValid = _eventInfo.getPeakTxPowerValid();
    teluxEventInfo.peakTxPower = _eventInfo.getPeakTxPower();
    teluxEventInfo.mcsIndexValid = _eventInfo.getMcsIndexValid();
    teluxEventInfo.mcsIndex = _eventInfo.getMcsIndex();
    teluxEventInfo.txPoolIdValid = _eventInfo.getTxPoolIdValid();
    teluxEventInfo.txPoolId = _eventInfo.getTxPoolId();
    teluxEventInfo.isUnicast = _eventInfo.getIsUnicast();

    auto changeEventFlowInfoCallback = [&prom, this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txEventFlow, ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->changeEventFlowInfo(txFlows_[_txFlowId], teluxEventInfo,
                    changeEventFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
            }
        } else {
            COMMONAPI_ERROR("changeEventFlow: flowId ", _txFlowId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::requestCapability(const std::shared_ptr<CommonAPI::ClientId> _client,
             requestCapabilityReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities capiCapabilities;
    auto requestCapabilitiesCallback = [&prom, &capiCapabilities, this](const telux::cv2x::Cv2xRadioCapabilities & capabilities,
            ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            capiCapabilities.setLinkIpMtuBytes(capabilities.linkIpMtuBytes);
            capiCapabilities.setLinkNonIpMtuBytes(capabilities.linkNonIpMtuBytes);
            capiCapabilities.setMaxSupportedConcurrency(
                    static_cast<capicv2x::Cv2xRadioTypes::RadioConcurrencyMode::Literal>(
                        capabilities.maxSupportedConcurrency));

            capiCapabilities.setNonIpTxPayloadOffsetBytes(capabilities.nonIpTxPayloadOffsetBytes);
            capiCapabilities.setNonIpRxPayloadOffsetBytes(capabilities.nonIpRxPayloadOffsetBytes);
            capiCapabilities.setPeriodicitiesSupported(Cv2xRemoteServerHelper::bitset2Uint8(
                capabilities.periodicitiesSupported));
            capiCapabilities.setPeriodicities(capabilities.periodicities);
            capiCapabilities.setMaxNumAutoRetransmissions(capabilities.maxNumAutoRetransmissions);
            capiCapabilities.setLayer2MacAddressSize(capabilities.layer2MacAddressSize);
            capiCapabilities.setPrioritiesSupported(Cv2xRemoteServerHelper::bitset2Uint8(
                capabilities.prioritiesSupported));
            capiCapabilities.setMaxNumSpsFlows(capabilities.maxNumSpsFlows);
            capiCapabilities.setMaxNumNonSpsFlows(capabilities.maxNumNonSpsFlows);
            capiCapabilities.setMaxTxPower(capabilities.maxTxPower);
            capiCapabilities.setMinTxPower(capabilities.minTxPower);

            std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> txPoolIdsSupported;
            for (telux::cv2x::TxPoolIdInfo teluxPoolIdInfo : capabilities.txPoolIdsSupported) {
                capicv2x::Cv2xRadioTypes::TxPoolIdInfo capiPoolIdInfo(teluxPoolIdInfo.poolId,
                        teluxPoolIdInfo.minFreq, teluxPoolIdInfo.maxFreq);
                txPoolIdsSupported.push_back(capiPoolIdInfo);
            }
            capiCapabilities.setTxPoolIdsSupported(txPoolIdsSupported);
            capiCapabilities.setIsUnicastSupported(capabilities.isUnicastSupported);
        }
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->requestCapabilities(requestCapabilitiesCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), capiCapabilities);
}

capicv2x::Cv2xRadioTypes::Cv2xStatus Cv2xRadioStubImpl::teluxCv2xStatus2capi(
        const telux::cv2x::Cv2xStatus status)
{
    return capicv2x::Cv2xRadioTypes::Cv2xStatus(
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.rxStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.txStatus),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.rxCause),
                static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.txCause),
                status.cbrValue,
                status.cbrValueValid
    );
}

void Cv2xRadioStubImpl::requestDataSessionSettings(const std::shared_ptr<CommonAPI::ClientId> _client,
    requestDataSessionSettingsReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    capicv2x::Cv2xRadioTypes::DataSessionSettings capiDataSessionSettings;
    auto requestDataSessionSettingCallback = [&prom, &capiDataSessionSettings, this](const telux::cv2x::DataSessionSettings & dataSessionSettings,
            ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            capiDataSessionSettings.setMtuValid(dataSessionSettings.mtuValid);
            capiDataSessionSettings.setMtu(dataSessionSettings.mtu);
            capiDataSessionSettings.setIpv6AddrValid(dataSessionSettings.ipv6AddrValid);
            capicv2x::Cv2xRadioTypes::IPv6Address ipv6Address;
            std::vector< uint8_t > ipv6str;
            for (auto s : dataSessionSettings.ipv6Addr.addr) {
                ipv6str.push_back(s);
            }
            ipv6Address.setAddr(ipv6str);
            capiDataSessionSettings.setIpv6Addr(ipv6Address);
        }
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->requestDataSessionSettings(requestDataSessionSettingCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), capiDataSessionSettings);
}

void Cv2xRadioStubImpl::updateSrcL2Info(const std::shared_ptr<CommonAPI::ClientId> _client,
    updateSrcL2InfoReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    auto updateSrcL2InfoCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->updateSrcL2Info(updateSrcL2InfoCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::updateTrustedUEList(const std::shared_ptr<CommonAPI::ClientId> _client,
    ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrustedUEInfoList _infoList,
    updateTrustedUEListReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::TrustedUEInfoList teluxTrustedUEInfoList;

    teluxTrustedUEInfoList.maliciousIdsValid = _infoList.getMaliciousIdsValid();
    auto maliciousIdList = _infoList.getMaliciousIds();
    for (auto id : maliciousIdList) {
        teluxTrustedUEInfoList.maliciousIds.push_back(id);
    }
    teluxTrustedUEInfoList.trustedUEsValid = _infoList.getTrustedUEsValid();
    auto trustedUEList = _infoList.getTrustedUEs();
    for (auto trustedUE : trustedUEList) {
        telux::cv2x::TrustedUEInfo teluxTrustedUE;
        teluxTrustedUE.sourceL2Id = trustedUE.getSourceL2Id();
        teluxTrustedUE.timeUncertainty = trustedUE.getTimeUncertainty();
        teluxTrustedUE.timeConfidenceLevel = trustedUE.getTimeConfidenceLevel();
        teluxTrustedUE.positionConfidenceLevel = trustedUE.getPositionConfidenceLevel();
        teluxTrustedUE.propagationDelay = trustedUE.getPropagationDelay();
        teluxTrustedUEInfoList.trustedUEs.push_back(teluxTrustedUE);
    }

    auto updateTrustedUEListCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->updateTrustedUEList(teluxTrustedUEInfoList, updateTrustedUEListCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::getIfaceNameFromIpType(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
        getIfaceNameFromIpTypeReply_t _reply)
{

    ErrorCode error;
    telux::cv2x::TrafficIpType teluxTrafficIpType;
    std::string ifaceName = "";

    switch (_ipType)
    {
    case ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType::TRAFFIC_IP:
        teluxTrafficIpType = telux::cv2x::TrafficIpType::TRAFFIC_IP;
        break;
    case ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType::TRAFFIC_NON_IP:
        teluxTrafficIpType = telux::cv2x::TrafficIpType::TRAFFIC_NON_IP;
        break;
    default:
        teluxTrafficIpType = telux::cv2x::TrafficIpType::TRAFFIC_IP;
        break;
    }

    if (cv2xRadio_ != nullptr) {
        ifaceName = cv2xRadio_->getIfaceNameFromIpType(teluxTrafficIpType);
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), ifaceName);
}

void Cv2xRadioStubImpl::createCv2xTcpSocket(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _eventFlowInfo,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SocketInfo _socketInfo,
        createCv2xTcpSocketReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::EventFlowInfo teluxEventFlowInfo;
    telux::cv2x::SocketInfo teluxSocketInfo;

    teluxEventFlowInfo.autoRetransEnabledValid = _eventFlowInfo.getAutoRetransEnabledValid();
    teluxEventFlowInfo.autoRetransEnabled = _eventFlowInfo.getAutoRetransEnabled();
    teluxEventFlowInfo.peakTxPowerValid = _eventFlowInfo.getPeakTxPowerValid();
    teluxEventFlowInfo.peakTxPower = _eventFlowInfo.getPeakTxPower();
    teluxEventFlowInfo.mcsIndexValid = _eventFlowInfo.getMcsIndexValid();
    teluxEventFlowInfo.mcsIndex = _eventFlowInfo.getMcsIndex();
    teluxEventFlowInfo.txPoolIdValid = _eventFlowInfo.getTxPoolIdValid();
    teluxEventFlowInfo.txPoolId = _eventFlowInfo.getTxPoolId();
    teluxEventFlowInfo.isUnicast = _eventFlowInfo.getIsUnicast();

    teluxSocketInfo.serviceId = _socketInfo.getServiceId();
    teluxSocketInfo.localPort = _socketInfo.getLocalPort();

    auto createCv2xTcpSocketCallback = [&prom, this](std::shared_ptr<telux::cv2x::ICv2xTxRxSocket> sock, ErrorCode error) {
        sockets_[sock->getId()] = sock;
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->createCv2xTcpSocket(teluxEventFlowInfo, teluxSocketInfo, createCv2xTcpSocketCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::closeCv2xTcpSocket(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _sockId, closeCv2xTcpSocketReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;

    auto closeCv2xTcpSocketCallback = [&prom, this](std::shared_ptr<telux::cv2x::ICv2xTxRxSocket> sock, ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        if (sockets_.find(_sockId) != sockets_.end()) {
            auto status = cv2xRadio_->closeCv2xTcpSocket(sockets_[_sockId], closeCv2xTcpSocketCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = prom.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    COMMONAPI_DEBUG("deleting socket ", _sockId);
                    sockets_.erase(_sockId);
                }
            }
        } else {
            COMMONAPI_ERROR("closeTxFlow: flowId ", _sockId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::setGlobalIPInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::IPv6AddrType _ipv6Addr,
        setGlobalIPInfoReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::IPv6AddrType teluxIpv6Addr = {0};

    teluxIpv6Addr.prefixLen = _ipv6Addr.getPrefixLen();
    auto ipv6Address = _ipv6Addr.getIpv6Addr();
    for (int index = 0; index < teluxIpv6Addr.prefixLen; index++) {
        teluxIpv6Addr.ipv6Addr[index] = ipv6Address[index];
    }

    auto setGlobalIPInfoCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->setGlobalIPInfo(teluxIpv6Addr, setGlobalIPInfoCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::setGlobalIPUnicastRoutingInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::GlobalIPUnicastRoutingInfo _destL2Addr,
        setGlobalIPUnicastRoutingInfoReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;
    telux::cv2x::GlobalIPUnicastRoutingInfo teluxDestL2Addr = {0};

    std::vector< uint8_t > destMacAddr = _destL2Addr.getDestMacAddr();
    std::copy(std::begin(destMacAddr), std::end(destMacAddr),
        std::begin(teluxDestL2Addr.destMacAddr));

    auto setGlobalIPUnicastRoutingInfoCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->setGlobalIPUnicastRoutingInfo(teluxDestL2Addr, setGlobalIPUnicastRoutingInfoCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl::getCapabilities(const std::shared_ptr<CommonAPI::ClientId> _client,
        getCapabilitiesReply_t _reply)
{
    ErrorCode error;

    capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities capiCv2xRadioCapabilities;
    if (cv2xRadio_ != nullptr) {
        capabilities_ = cv2xRadio_->getCapabilities();
        capiCv2xRadioCapabilities.setLinkIpMtuBytes(capabilities_.linkIpMtuBytes);
        capiCv2xRadioCapabilities.setLinkNonIpMtuBytes(capabilities_.linkNonIpMtuBytes);
        capiCv2xRadioCapabilities.setMaxSupportedConcurrency(
            static_cast<capicv2x::Cv2xRadioTypes::RadioConcurrencyMode::Literal>(capabilities_.maxSupportedConcurrency));
        capiCv2xRadioCapabilities.setNonIpTxPayloadOffsetBytes(capabilities_.nonIpTxPayloadOffsetBytes);
        capiCv2xRadioCapabilities.setNonIpRxPayloadOffsetBytes(capabilities_.nonIpRxPayloadOffsetBytes);
        capiCv2xRadioCapabilities.setPeriodicitiesSupported(static_cast<uint8_t>(capabilities_.periodicitiesSupported.to_ulong()));
        capiCv2xRadioCapabilities.setPeriodicities(capabilities_.periodicities);
        capiCv2xRadioCapabilities.setMaxNumAutoRetransmissions(capabilities_.maxNumAutoRetransmissions);
        capiCv2xRadioCapabilities.setLayer2MacAddressSize(capabilities_.layer2MacAddressSize);
        capiCv2xRadioCapabilities.setPrioritiesSupported(static_cast<uint8_t>(capabilities_.prioritiesSupported.to_ulong()));
        capiCv2xRadioCapabilities.setMaxNumSpsFlows(capabilities_.maxNumSpsFlows);
        capiCv2xRadioCapabilities.setMaxNumNonSpsFlows(capabilities_.maxNumNonSpsFlows);
        capiCv2xRadioCapabilities.setMaxTxPower(capabilities_.maxTxPower);
        capiCv2xRadioCapabilities.setMinTxPower(capabilities_.minTxPower);
        std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> capiTxPoolIdsSupported;
        for (auto info : capabilities_.txPoolIdsSupported) {
            capicv2x::Cv2xRadioTypes::TxPoolIdInfo capiInfo;
            capiInfo = capicv2x::Cv2xRadioTypes::TxPoolIdInfo(info.poolId, info.minFreq, info.maxFreq);
            capiTxPoolIdsSupported.push_back(capiInfo);

        }
        capiCv2xRadioCapabilities.setTxPoolIdsSupported(capiTxPoolIdsSupported);
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), capiCv2xRadioCapabilities);
}

void Cv2xRadioStubImpl::injectVehicleSpeed(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _speed, injectVehicleSpeedReply_t _reply)
{
    ErrorCode error;
    std::promise<telux::common::ErrorCode> prom;

    auto injectVehicleSpeedCallback = [&prom, this](ErrorCode error) {
        prom.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->injectVehicleSpeed(_speed, injectVehicleSpeedCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = prom.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}
