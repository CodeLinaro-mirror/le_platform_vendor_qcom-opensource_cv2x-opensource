/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck, [&] { return cv2xRadioStatusUpdated; });
            error = ErrorCode::SUCCESS;
        }
    }
}

Cv2xRadioStubImpl::~Cv2xRadioStubImpl() {
    //TODO: empty the FlowInfo, and RxSub
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
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->createRxSubscription(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                _port,
                createRxSubscriptionCallback,
                idList);

        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = CallbackPromise_.get_future().get();
        }
        //reset callback promise
        CallbackPromise_ =  promise<ErrorCode>();
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
    auto enableRxMetaDataCallback = [this](ErrorCode error) {
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->enableRxMetaDataReport(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                _enable, std::make_shared<std::vector<uint32_t>>(_idList), enableRxMetaDataCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = CallbackPromise_.get_future().get();
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
                CallbackPromise_.set_value(spsError);
            } else {
                if (spsError != ErrorCode::SUCCESS) {
                    CallbackPromise_.set_value(spsError);
                } else if (eventError != ErrorCode::SUCCESS) {
                    CallbackPromise_.set_value(eventError);
                } else {
                    CallbackPromise_.set_value(ErrorCode::SUCCESS);
                }
            }
        };
        //TODO convert _spsInfo to spsInfo
        telux::cv2x::SpsFlowInfo spsInfo;
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
            error = CallbackPromise_.get_future().get();
        }
        CallbackPromise_ =  promise<ErrorCode>();
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
            uint32_t _serviceId, uint16_t _eventSrcPort, createTxEventFlowReply_t _reply)
{
    ErrorCode error;
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
            CallbackPromise_.set_value(error);
        };
        telux::cv2x::EventFlowInfo flowInfo;
        auto status = cv2xRadio_->createTxEventFlow(static_cast<telux::cv2x::TrafficIpType>((uint8_t)_ipType),
                                    _serviceId,
                                    flowInfo,
                                    _eventSrcPort,
                                    createTxEventFlowCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = CallbackPromise_.get_future().get();
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }
    CallbackPromise_ =  promise<ErrorCode>();

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error), eventFlowId);
}

void Cv2xRadioStubImpl::closeRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
             uint32_t _subscriptionId, closeRxSubscriptionReply_t _reply)
{
    ErrorCode error;
    auto closeRxSubCallback = [this](std::shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (rxSubs_.find(_subscriptionId) != rxSubs_.end()) {
            if (shutdown(rxSubs_[_subscriptionId]->getSock(), SHUT_RD) == -1) {
                int temp_error = errno;
                COMMONAPI_ERROR("Failed to shutdown read direction ", temp_error);
            }
            auto status = cv2xRadio_->closeRxSubscription(rxSubs_[_subscriptionId], closeRxSubCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = CallbackPromise_.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    COMMONAPI_DEBUG("deleting rxsub ", _subscriptionId);
                    rxSubs_.erase(_subscriptionId);
                    pubs_.erase(_subscriptionId);
                }
            }
            CallbackPromise_ =  promise<ErrorCode>();
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
    auto closeTxFlowCallback = [this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow, ErrorCode error) {
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->closeTxFlow(txFlows_[_txFlowId], closeTxFlowCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = CallbackPromise_.get_future().get();
                if (error == ErrorCode::SUCCESS) {
                    COMMONAPI_DEBUG("deleting txflow ", _txFlowId);
                    txFlows_.erase(_txFlowId);
                    subs_.erase(_txFlowId);
                }
            }
            CallbackPromise_ =  promise<ErrorCode>();
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

    auto changeSpsFlowInfoCallback = [this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txSpsFlow, ErrorCode error) {
        //TODO update flow in the map?
        CallbackPromise_.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        //for (std::shared_ptr<telux::cv2x::ICv2xTxFlow> TxFlow : txFlows_) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->changeSpsFlowInfo(txFlows_[_txFlowId], teluxSpsInfo,
                    changeSpsFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = CallbackPromise_.get_future().get();
            }
            CallbackPromise_ =  promise<ErrorCode>();
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
    capicv2x::Cv2xRadioTypes::SpsFlowInfo capiSpsInfo;
    auto requestSpsFlowInfoCallback = [&capiSpsInfo, this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txSpsFlow,
            const telux::cv2x::SpsFlowInfo & spsInfo,
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
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            COMMONAPI_DEBUG("requestSpsFlowInfo: flowId ", _txFlowId);
            auto status = cv2xRadio_->requestSpsFlowInfo(txFlows_[_txFlowId], requestSpsFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = CallbackPromise_.get_future().get();
                COMMONAPI_DEBUG("requestSpsFlowInfo: ERROR ", static_cast<int>(error));
            }
            CallbackPromise_ =  promise<ErrorCode>();
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

    auto changeEventFlowInfoCallback = [this](std::shared_ptr<telux::cv2x::ICv2xTxFlow> txEventFlow, ErrorCode error) {
        //TODO: update txEventFlow?
        CallbackPromise_.set_value(error);
    };

    if (cv2xRadio_ != nullptr) {
        if (txFlows_.find(_txFlowId) != txFlows_.end()) {
            auto status = cv2xRadio_->changeEventFlowInfo(txFlows_[_txFlowId], teluxEventInfo,
                    changeEventFlowInfoCallback);
            if (status != Status::SUCCESS) {
                error = ErrorCode::GENERIC_FAILURE;
            } else {
                error = CallbackPromise_.get_future().get();
            }
            CallbackPromise_ =  promise<ErrorCode>();
        } else {
            COMMONAPI_ERROR("changeEventFlow: flowId ", _txFlowId, " Not found");
            error = ErrorCode::GENERIC_FAILURE;
        }
    } else {
        error = ErrorCode::RADIO_NOT_AVAILABLE;
    }

    _reply(static_cast<capiteluxcommon::CommonDefines::ErrorCode::Literal>(error));
}

void Cv2xRadioStubImpl:: requestCapability(const std::shared_ptr<CommonAPI::ClientId> _client,
             requestCapabilityReply_t _reply)
{
    ErrorCode error;
    capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities capiCapabilities;
    auto requestCapabilitiesCallback = [&capiCapabilities, this](const telux::cv2x::Cv2xRadioCapabilities & capabilities,
            ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            capiCapabilities.setLinkIpMtuBytes(capabilities.linkIpMtuBytes);
            capiCapabilities.setLinkNonIpMtuBytes(capabilities.linkNonIpMtuBytes);
            capiCapabilities.setMaxSupportedConcurrency(
                    static_cast<capicv2x::Cv2xRadioTypes::RadioConcurrencyMode::Literal>(
                        capabilities.maxSupportedConcurrency));

            capiCapabilities.setNonIpTxPayloadOffsetBytes(capabilities.nonIpTxPayloadOffsetBytes);
            capiCapabilities.setNonIpRxPayloadOffsetBytes(capabilities.nonIpRxPayloadOffsetBytes);
            capiCapabilities.setPeriodicitiesSupported(bitset2Uint8(capabilities.periodicitiesSupported));
            capiCapabilities.setPeriodicities(capabilities.periodicities);
            capiCapabilities.setMaxNumAutoRetransmissions(capabilities.maxNumAutoRetransmissions);
            capiCapabilities.setLayer2MacAddressSize(capabilities.layer2MacAddressSize);
            capiCapabilities.setPrioritiesSupported(bitset2Uint8(capabilities.prioritiesSupported));
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
        CallbackPromise_.set_value(error);
    };
    if (cv2xRadio_ != nullptr) {
        auto status = cv2xRadio_->requestCapabilities(requestCapabilitiesCallback);
        if (status != Status::SUCCESS) {
            error = ErrorCode::GENERIC_FAILURE;
        } else {
            error = CallbackPromise_.get_future().get();
        }
        CallbackPromise_ =  promise<ErrorCode>();
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
void Cv2xRadioStubImpl::onStatusChanged(Cv2xStatus status)
{
    std::lock_guard<std::mutex> lock(m_);
#if 0
    std::cout << "onStatusChanged tx=" << static_cast<int32_t>(status.txStatus) <<
        "rx=" << static_cast<int32_t>(status.rxStatus) <<
        "tx_cause=" << static_cast<int32_t>(status.txCause) <<
        "rx_cause=" << static_cast<int32_t>(status.rxCause) << std::endl;
    
    std::cout << "onStatusChanged current tx=" << static_cast<int32_t>(currentStatus_.txStatus) <<
        "rx=" << static_cast<int32_t>(currentStatus_.rxStatus) <<
        "tx_cause=" << static_cast<int32_t>(currentStatus_.txCause) <<
        "rx_cause=" << static_cast<int32_t>(currentStatus_.rxCause) << std::endl;
#endif
    if (cv2xStatusChanged(status)) {
            currentCv2xStatusEx_.status = status;
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

void Cv2xRadioStubImpl::onStatusChanged(Cv2xStatusEx statusEx)
{
#if 0
    std::lock_guard<std::mutex> lock(m_);
    if (cv2xStatusChanged(statusEx.status)) {
            currentCv2xStatusEx_.status = status;
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
#endif
}


void Cv2xRadioStubImpl::onL2AddrChanged(uint32_t newL2Addr) 
{
    std::lock_guard<std::mutex> lock(m_);
    fireCurrentL2AddrEvent(newL2Addr);
}

void Cv2xRadioStubImpl::onSpsOffsetChanged(int spsId, telux::cv2x::MacDetails details) {
    std::lock_guard<std::mutex> lock(m_);
    fireCurrentSpsOffsetEvent(spsId,
            capicv2x::Cv2xRadioTypes::MacDetails(details.periodicityInUseNs,
                details.currentlyReservedPeriodicBytes,
                details.txReservationOffsetNs));
}

void Cv2xRadioStubImpl::onSpsSchedulingChanged(const telux::cv2x::SpsSchedulingInfo & schedulingInfo)
{
    std::lock_guard<std::mutex> lock(m_);
    //TODO: fire event not defined
}

void Cv2xRadioStubImpl::onCapabilitiesChanged(const telux::cv2x::Cv2xRadioCapabilities & capabilities)
{
    std::lock_guard<std::mutex> lock(m_);
    std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> txPoolIdsSupported;
    for (telux::cv2x::TxPoolIdInfo teluxPoolIdInfo : capabilities.txPoolIdsSupported) {
        capicv2x::Cv2xRadioTypes::TxPoolIdInfo capiPoolIdInfo(teluxPoolIdInfo.poolId,
                teluxPoolIdInfo.minFreq, teluxPoolIdInfo.maxFreq);
        txPoolIdsSupported.push_back(capiPoolIdInfo);
    }
    fireCurrentCapabilitiesEvent(
            capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities(capabilities.linkIpMtuBytes,
                capabilities.linkNonIpMtuBytes,
                static_cast<capicv2x::Cv2xRadioTypes::RadioConcurrencyMode::Literal>(capabilities.maxSupportedConcurrency),
                capabilities.nonIpTxPayloadOffsetBytes,
                capabilities.nonIpRxPayloadOffsetBytes,
                bitset2Uint8(capabilities.periodicitiesSupported),
                capabilities.periodicities,
                capabilities.maxNumAutoRetransmissions,
                capabilities.layer2MacAddressSize,
                bitset2Uint8(capabilities.prioritiesSupported),
                capabilities.maxNumSpsFlows,
                capabilities.maxNumNonSpsFlows,
                capabilities.maxTxPower,
                capabilities.minTxPower,
                txPoolIdsSupported,
                capabilities.isUnicastSupported));
}

void Cv2xRadioStubImpl::onMacAddressCloneAttack(const bool detected)
{
    std::lock_guard<std::mutex> lock(m_);
    fireMacAddrCloneEvent(detected);
}

bool Cv2xRadioStubImpl::cv2xStatusChanged(telux::cv2x::Cv2xStatus status)
{
    if (status.txStatus != currentCv2xStatusEx_.status.txStatus
        or status.rxStatus != currentCv2xStatusEx_.status.rxStatus
        or status.txCause != currentCv2xStatusEx_.status.txCause
        or status.rxCause != currentCv2xStatusEx_.status.rxCause) {
        return true;
    }
    return false;
}

uint8_t Cv2xRadioStubImpl::bitset2Uint8(const std::bitset<8> bits)
{
    //convert std::bitset<8> to uint8_t
    uint8_t data = 0;
    std::size_t idx = 0;
    while (idx < bits.size() && bits.test(idx)) {
        data |= (1 << idx);
        ++idx;
    }

    return data;
}
