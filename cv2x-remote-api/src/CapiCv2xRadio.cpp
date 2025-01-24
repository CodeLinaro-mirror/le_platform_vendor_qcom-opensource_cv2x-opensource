/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <CommonAPI/CommonAPI.hpp>
#include "CapiCv2xRadio.hpp"
#include "CapiCv2xRxSubscription.hpp"
#include "CapiCv2xTxFlow.hpp"
#include "CapiCv2xTxRxSocket.hpp"

#define PROXY_TIMEOUT_MS 50000

namespace telux {

namespace cv2x {

Cv2xStatus Capi2teluxHelper::capi2teluxCv2xStatus(const capicv2x::Cv2xRadioTypes::Cv2xStatus &status) {
    Cv2xStatus st;
    st.rxStatus = static_cast<Cv2xStatusType>(status.getRxStatus().value_);
    st.txStatus = static_cast<Cv2xStatusType>(status.getTxStatus().value_);
    st.rxCause = static_cast<Cv2xCauseType>(status.getRxCause().value_);
    st.txCause = static_cast<Cv2xCauseType>(status.getTxCause().value_);
    st.cbrValue = status.getCbrValue();
    st.cbrValueValid = status.getCbrValueValid();
    return st;
}
std::vector<Cv2xPoolStatus> Capi2teluxHelper::capi2teluxCv2xPoolStatus(
    const std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> &status) {
    std::vector<Cv2xPoolStatus> pv;
    for (auto st : status) {
        Cv2xPoolStatus ps;
        ps.poolId = st.getPoolId();
        ps.status = Capi2teluxHelper::capi2teluxCv2xStatus(st.getStatus());
        pv.push_back(ps);
    }
    return pv;
}

Cv2xStatusEx Capi2teluxHelper::capi2teluxCv2xStatusEx(
    const capicv2x::Cv2xRadioTypes::Cv2xStatusEx statusEx) {
    Cv2xStatusEx stex;

    stex.status = Capi2teluxHelper::capi2teluxCv2xStatus(statusEx.getStatus());
    stex.poolStatus = Capi2teluxHelper::capi2teluxCv2xPoolStatus(statusEx.getPoolStatus());
    stex.timeUncertaintyValid = statusEx.getTimeUncertaintyValid();
    stex.timeUncertainty = statusEx.getTimeUncertainty();
    return stex;
}
std::vector<TxPoolIdInfo> Capi2teluxHelper::capi2teluxSupportedPoolIds(
    const std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> &idInfos) {
    std::vector<TxPoolIdInfo> pv;

    for (auto idInfo : idInfos) {
        TxPoolIdInfo info;
        info.poolId = idInfo.getPoolId();
        info.minFreq = idInfo.getMinFreq();
        info.maxFreq = idInfo.getMaxFreq();
        pv.push_back(info);
    }
    return pv;
}

telux::cv2x::Cv2xRadioCapabilities Capi2teluxHelper::capi2teluxRadioCapbilities(
    const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities &capabilities) {
    telux::cv2x::Cv2xRadioCapabilities cap;
    cap.linkIpMtuBytes = capabilities.getLinkIpMtuBytes();
    cap.linkNonIpMtuBytes = capabilities.getLinkNonIpMtuBytes();
    cap.maxSupportedConcurrency = static_cast<RadioConcurrencyMode>(
        capabilities.getMaxSupportedConcurrency().value_);
    cap.nonIpTxPayloadOffsetBytes = capabilities.getNonIpTxPayloadOffsetBytes();
    cap.nonIpRxPayloadOffsetBytes = capabilities.getNonIpRxPayloadOffsetBytes();
    cap.periodicitiesSupported = capabilities.getPeriodicitiesSupported();
    cap.periodicities = capabilities.getPeriodicities();
    cap.maxNumAutoRetransmissions = capabilities.getMaxNumAutoRetransmissions();
    cap.layer2MacAddressSize = capabilities.getLayer2MacAddressSize();
    cap.prioritiesSupported = capabilities.getPrioritiesSupported();
    cap.maxNumSpsFlows = capabilities.getMaxNumSpsFlows();
    cap.maxNumNonSpsFlows = capabilities.getMaxNumNonSpsFlows();
    cap.maxTxPower = capabilities.getMaxTxPower();
    cap.minTxPower = capabilities.getMinTxPower();
    cap.txPoolIdsSupported = capi2teluxSupportedPoolIds(capabilities.getTxPoolIdsSupported());

    return cap;
}

std::vector<TrustedUEInfo> Capi2teluxHelper::capi2teluxTrustedUEs(
    const std::vector<capicv2x::Cv2xRadioTypes::TrustedUEInfo> &capiTrustedUEs) {
    std::vector<TrustedUEInfo> infos;
    for (auto &capiTrustedUE : capiTrustedUEs) {
        TrustedUEInfo info;
        info.sourceL2Id = capiTrustedUE.getSourceL2Id();
        info.timeUncertainty = capiTrustedUE.getTimeUncertainty();
        info.timeConfidenceLevel = capiTrustedUE.getTimeConfidenceLevel();
        info.positionConfidenceLevel = capiTrustedUE.getPositionConfidenceLevel();
        info.propagationDelay = capiTrustedUE.getPropagationDelay();
        infos.push_back(info);
    }
    return infos;
}

SpsFlowInfo Capi2teluxHelper::capi2teluxSpsFlowInfo(
        const capicv2x::Cv2xRadioTypes::SpsFlowInfo &flowInfo) {
    SpsFlowInfo spsInfo;
    spsInfo.priority = static_cast<Priority>(flowInfo.getPriority().value_);
    spsInfo.periodicity = static_cast<Periodicity>(flowInfo.getPeriodicity().value_);
    spsInfo.periodicityMs = flowInfo.getPeriodicityMs();
    spsInfo.nbytesReserved = flowInfo.getNbytesReserved();
    spsInfo.autoRetransEnabledValid = flowInfo.getAutoRetransEnabledValid();
    spsInfo.autoRetransEnabled = flowInfo.getAutoRetransEnabled();
    spsInfo.peakTxPowerValid = flowInfo.getPeakTxPowerValid();
    spsInfo.peakTxPower = flowInfo.getPeakTxPower();
    spsInfo.mcsIndexValid = flowInfo.getMcsIndexValid();
    spsInfo.mcsIndex = flowInfo.getMcsIndex();
    spsInfo.txPoolIdValid = flowInfo.getTxPoolIdValid();
    spsInfo.txPoolId = flowInfo.getTxPoolId();
    return spsInfo;
}

SlssRxInfo Capi2teluxHelper::capi2teluxSlssRxInfo(
        const capicv2x::Cv2xRadioTypes::SlssRxInfo &slssRxInfo) {
    telux::cv2x::SlssRxInfo teluxInfo;
    for (auto capiUe : slssRxInfo.getUeInfo()) {
        telux::cv2x::SyncRefUeInfo teluxUeInfo;
        teluxUeInfo.slssId = capiUe.getSlssId();
        teluxUeInfo.inCoverage = capiUe.getInCoverage();
        teluxUeInfo.pattern = static_cast<SlssSyncPattern>(capiUe.getPattern().value_);
        teluxUeInfo.rsrp = capiUe.getRsrp();
        teluxUeInfo.selected = capiUe.getSelected();
        teluxInfo.ueInfo.push_back(teluxUeInfo);
    }
    return teluxInfo;
}

CapiCv2xRadio::CapiCv2xRadio()
{
    isInitialized_ = false;
}

CapiCv2xRadio::~CapiCv2xRadio()
{
    COMMONAPI_DEBUG("CapiCv2xRadio destructor");
    {
        std::lock_guard<std::mutex> lck(exitMutex_);
        exiting_ = true;
        exitCv_.notify_all();
    }

    proxy_ = nullptr;
}

void CapiCv2xRadio::init(std::weak_ptr<telux::common::AsyncTaskQueue<void>> taskQ,
    InitResponseCb callback)
{
    myself_ = shared_from_this();
    taskQ_ = taskQ;
    auto f = [this, callback]() {
        this->initSync(callback);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        sp->add(f, myself_, std::launch::async);
    }
}

void CapiCv2xRadio::initSync(InitResponseCb callback)
{
    unsigned int count = PROXY_TIMEOUT_MS;
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.telux.cv2x.Cv2xRadio";
    proxy_ =
        runtime->buildProxy<capicv2x::Cv2xRadioProxy>(domain, instance, "client-Cv2xRadio");

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
    COMMONAPI_INFO("Cv2xRadio Proxy registration succesful");
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
    proxy_->getCurrentCv2xStatusExEvent().subscribe([&](const capicv2x::Cv2xRadioTypes::Cv2xStatusEx &statusEx) {
            Cv2xStatusEx stex;
            stex.status = Capi2teluxHelper::capi2teluxCv2xStatus(statusEx.getStatus());
            stex.poolStatus = Capi2teluxHelper::capi2teluxCv2xPoolStatus(statusEx.getPoolStatus());
            stex.timeUncertaintyValid = statusEx.getTimeUncertaintyValid();
            stex.timeUncertainty = statusEx.getTimeUncertainty();
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onStatusChanged(stex);
                }
            }
    });
    proxy_->getCurrentL2AddrEvent().subscribe([&](const uint32_t newL2Addr) {
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onL2AddrChanged(newL2Addr);
                }
            }
    });
    proxy_->getCurrentSpsOffsetEvent().subscribe([&](const int spsId,
                                const capicv2x::Cv2xRadioTypes::MacDetails details) {
            MacDetails mc;
            mc.periodicityInUseNs = details.getPeriodicityInUseNs();
            mc.currentlyReservedPeriodicBytes = details.getCurrentlyReservedPeriodicBytes();
            mc.txReservationOffsetNs = details.getTxReservationOffsetNs();
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onSpsOffsetChanged(spsId, mc);
                }
            }
    });
    proxy_->getCurrentCapabilitiesEvent().subscribe([&](
                            const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities & capabilities) {
            Cv2xRadioCapabilities cap = Capi2teluxHelper::capi2teluxRadioCapbilities(capabilities);
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onCapabilitiesChanged(cap);
                }
            }
    });

    proxy_->getMacAddrCloneEvent().subscribe([&](const bool detected) {
            std::lock_guard<std::mutex> lock(listenersMutex_);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onMacAddressCloneAttack(detected);
                }
            }
    });
    isInitialized_ = true;
    if (callback) {
        COMMONAPI_DEBUG("calling init callback");
        callback(ServiceStatus::SERVICE_AVAILABLE);
    }

}

ServiceStatus CapiCv2xRadio::getServiceStatus(void)
{
    ServiceStatus teluxServiceStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    capitelux::CommonDefines::ErrorCode err;
    capitelux::CommonDefines::ServiceStatus serviceStatus;

    // sync call only.
    proxy_->getServiceStatus(callStatus, err, serviceStatus);
    if (callStatus == CommonAPI::CallStatus::SUCCESS &&
            err == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
        teluxServiceStatus = static_cast<ServiceStatus>(serviceStatus.value_);
    } else {
        if (err != capitelux::CommonDefines::ErrorCode::SUCCESS) {
            COMMONAPI_ERROR(" getServiceStatus returned error");
        } else {
            COMMONAPI_ERROR("CapiCv2xRadio::getServiceStatus: RPC failure");
        }
    }
    return teluxServiceStatus;
}
Status CapiCv2xRadio::registerListener(std::weak_ptr<ICv2xRadioListener> listener)
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
Status CapiCv2xRadio::deregisterListener(std::weak_ptr<ICv2xRadioListener> listener)
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

Status CapiCv2xRadio::createRxSubscription(
        TrafficIpType ipType, uint16_t port, CreateRxSubscriptionCallback cb,
        std::shared_ptr<std::vector<uint32_t>> idList)
{
    Status status = Status::FAILED;

    if (!proxy_->isAvailable()) {
        COMMONAPI_DEBUG("proxy is not available");
        return Status::NOTREADY;
    }
    // async call
    auto fcb = [this, cb](const CommonAPI::CallStatus &callstatus,
            const capitelux::CommonDefines::ErrorCode &err,
            const uint32_t &rxSubId){
        ErrorCode teluxError = ErrorCode::GENERIC_FAILURE;
        std::shared_ptr<ICv2xRxSubscription> rxSub = nullptr;

        if (callstatus == CommonAPI::CallStatus::SUCCESS &&
                err == capitelux::CommonDefines::ErrorCode::SUCCESS) {
            // creae DDS subscriber and rxSubscription objects
            std::shared_ptr<Cv2xDataSubscriber> ddsSub = std::make_shared<Cv2xDataSubscriber>();

            if (ddsSub->init(rxSubId) != false) {
                rxSub = std::make_shared<CapiCv2xRxSubscription>(ddsSub->getClientSock(),
                        rxSubId);
                subs_.push_back(ddsSub);
                teluxError = ErrorCode::SUCCESS;
            } else {
                COMMONAPI_ERROR("failed to init dds subscriber");
                teluxError = ErrorCode::GENERIC_FAILURE;
            }
        } else {
            COMMONAPI_ERROR("rpc call failure");
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                teluxError = static_cast<ErrorCode>(err.value_);
            } else {
                teluxError = ErrorCode::GENERIC_FAILURE;
            }
        }
        if (cb) {
            cb(rxSub, teluxError);
        }
    };
    std::vector<uint32_t> idv;
    if (idList != nullptr) {
        idv = *idList;
    }
    auto f = [this, ipType, port, idv, fcb]() {
        proxy_->createRxSubscriptionAsync(
        static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
        port,
        idv,
        fcb);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        status = sp->add(f, myself_, std::launch::async);
    }

    return status;
}

telux::common::Status CapiCv2xRadio::enableRxMetaDataReport(TrafficIpType ipType,
    bool enable,
    std::shared_ptr<std::vector<std::uint32_t>> idList,
    telux::common::ResponseCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    std::vector<uint32_t> idv;
    if (idList != nullptr) {
        idv = *idList;
    }
    if (not cb) {
        // sync call
        proxy_->enableRxMetaDataReport(
            static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
            enable,
            idv,
            callStatus,
            error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, ipType, enable, idv, fcb]() {
            proxy_->enableRxMetaDataReportAsync(
                static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
                enable,
                idv,
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::createTxSpsFlow(TrafficIpType ipType,
                                          uint32_t serviceId,
                                          const SpsFlowInfo& flowInfo,
                                          uint16_t spsSrcPort,
                                          bool eventSrcPortValid,
                                          uint16_t eventSrcPort,
                                          CreateTxSpsFlowCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error, spsError, eventError;
    uint32_t txSpsFlowId, txEventFlowId;
    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    capicv2x::Cv2xRadioTypes::SpsFlowInfo capiSpsInfo(
            static_cast<capicv2x::Cv2xRadioTypes::Priority::Literal>(flowInfo.priority),
            static_cast<capicv2x::Cv2xRadioTypes::Periodicity::Literal>(flowInfo.periodicity),
            flowInfo.periodicityMs,
            flowInfo.nbytesReserved,
            flowInfo.autoRetransEnabledValid,
            flowInfo.autoRetransEnabled,
            flowInfo.peakTxPowerValid,
            flowInfo.peakTxPower,
            flowInfo.mcsIndexValid,
            flowInfo.mcsIndex,
            flowInfo.txPoolIdValid,
            flowInfo.txPoolId);
    if (not cb) {
        // sync call
        proxy_->createTxSpsFlow(static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
                serviceId,
                capiSpsInfo,
                spsSrcPort,
                eventSrcPortValid,
                eventSrcPort,
                callStatus,
                error,
                txSpsFlowId,
                txEventFlowId,
                spsError,
                eventError);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        }
    } else {
        // async call
        auto fcb = [this, ipType, serviceId, flowInfo, eventSrcPortValid, cb](
                const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err,
                const uint32_t spsFlowId,
                const uint32_t eventFlowId,
                const capitelux::CommonDefines::ErrorCode spsError,
                const capitelux::CommonDefines::ErrorCode eventError) {
            ErrorCode teluxError, teluxEventError;
            std::shared_ptr<ICv2xTxFlow> txSpsFlow = nullptr;
            std::shared_ptr<ICv2xTxFlow> txEventFlow = nullptr;

            if (callstatus == CommonAPI::CallStatus::SUCCESS &&
                    spsError == capitelux::CommonDefines::ErrorCode::SUCCESS) {
                // creae DDS publishers and txFlow objects
                std::shared_ptr<Cv2xDataPublisher> ddsSpsPub =
                    std::make_shared<Cv2xDataPublisher>();
                if (ddsSpsPub->init(spsFlowId) != false ) {
                    txSpsFlow = std::make_shared<CapiCv2xTxSpsFlow>(spsFlowId, ipType, serviceId,
                            ddsSpsPub->getServerSock(), flowInfo);
                    pubs_[spsFlowId]= ddsSpsPub;
                    teluxError = ErrorCode::SUCCESS;
                    // create event flow and publisher
                    if (eventSrcPortValid &&
                            eventError == capitelux::CommonDefines::ErrorCode::SUCCESS) {
                        std::shared_ptr<Cv2xDataPublisher> ddsEventPub =
                            std::make_shared<Cv2xDataPublisher>();
                        if (ddsEventPub->init(eventFlowId) != false) {
                            txEventFlow = std::make_shared<CapiCv2xTxEventFlow>(ipType,
                                    serviceId, ddsEventPub->getServerSock(), eventFlowId);
                            pubs_[eventFlowId] = ddsEventPub;
                            teluxEventError = ErrorCode::SUCCESS;
                        } else {
                            COMMONAPI_ERROR("Failed to init dds publisher for event");
                            teluxEventError = ErrorCode::GENERIC_FAILURE;
                        }
                    }
                } else {
                    COMMONAPI_ERROR("Failed to init dds publisher for sps");
                    teluxError = ErrorCode::GENERIC_FAILURE;
                }
            } else {
                if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                    teluxError = static_cast<ErrorCode>(spsError.value_);
                    teluxEventError = static_cast<ErrorCode>(eventError.value_);
                } else {
                    teluxError = ErrorCode::GENERIC_FAILURE;
                }
            }
            if (cb) {
                cb(txSpsFlow, txEventFlow, teluxError, teluxEventError);
            }
        };
        auto f = [this, ipType, serviceId, capiSpsInfo, spsSrcPort,
            eventSrcPortValid, eventSrcPort, fcb]() {
            proxy_->createTxSpsFlowAsync(
                static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
                serviceId,
                capiSpsInfo,
                spsSrcPort,
                eventSrcPortValid,
                eventSrcPort,
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::createTxEventFlow(TrafficIpType ipType,
                                            uint32_t serviceId,
                                            uint16_t eventSrcPort,
                                            CreateTxEventFlowCallback cb)
{

    EventFlowInfo flowInfo;
    flowInfo.autoRetransEnabledValid = false;
    flowInfo.peakTxPowerValid        = false;
    flowInfo.mcsIndexValid           = false;
    flowInfo.txPoolIdValid           = false;

    return this->createTxEventFlow(ipType, serviceId, flowInfo, eventSrcPort, cb);
}

telux::common::Status CapiCv2xRadio::createTxEventFlow(TrafficIpType ipType,
                                            uint32_t serviceId,
                                            const EventFlowInfo& flowInfo,
                                            uint16_t eventSrcPort,
                                            CreateTxEventFlowCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;
    uint32_t txEventFlowId;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    capicv2x::Cv2xRadioTypes::EventFlowInfo capiEventInfo(
        flowInfo.autoRetransEnabledValid,
        flowInfo.autoRetransEnabled,
        flowInfo.peakTxPowerValid,
        flowInfo.peakTxPower,
        flowInfo.mcsIndexValid,
        flowInfo.mcsIndex,
        flowInfo.txPoolIdValid,
        flowInfo.txPoolId,
        flowInfo.isUnicast);

    if (not cb) {
        // sync call
        proxy_->createTxEventFlow(
                static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
                serviceId,
                capiEventInfo,
                eventSrcPort,
                callStatus,
                error,
                txEventFlowId);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = Status::FAILED;
        }
    } else {
        // async call
        auto fcb = [this, ipType, serviceId, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err,
                const uint32_t flowId) {
            ErrorCode teluxError;
            std::shared_ptr<ICv2xTxFlow> txEventFlow = nullptr;
            if (callstatus == CommonAPI::CallStatus::SUCCESS &&
                    err == capitelux::CommonDefines::ErrorCode::SUCCESS) {
                //creae DDS publisher and txFlow object.
                std::shared_ptr<Cv2xDataPublisher> ddsPub = std::make_shared<Cv2xDataPublisher>();
                if (ddsPub->init(flowId) != true) {
                    teluxError = ErrorCode::GENERIC_FAILURE;
                } else {
                    txEventFlow = std::make_shared<CapiCv2xTxEventFlow>(ipType, serviceId,
                            ddsPub->getServerSock(), flowId);
                    pubs_[flowId] = ddsPub;
                    teluxError = ErrorCode::SUCCESS;
                }
            } else {
                if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                    teluxError = static_cast<ErrorCode>(err.value_);
                }
            }
            if (cb) {
                    cb(txEventFlow, teluxError);
            }
        };
        auto f = [this, ipType, serviceId, capiEventInfo, eventSrcPort, fcb]() {
            proxy_->createTxEventFlowAsync(
                static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
                serviceId,
                capiEventInfo,
                eventSrcPort,
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::closeRxSubscription(std::shared_ptr<ICv2xRxSubscription> rxSub,
                                              CloseRxSubscriptionCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (not cb) {
        // sync call
        proxy_->closeRxSubscription(
                rxSub->getSubscriptionId(),
                callStatus,
                error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = Status::FAILED;
        } else {
            status = Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [rxSub, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(rxSub, static_cast<ErrorCode>(err.value_));
            } else {
                cb(rxSub, ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, rxSub, fcb]() {
            proxy_->closeRxSubscriptionAsync(
                rxSub->getSubscriptionId(),
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::closeTxFlow(std::shared_ptr<ICv2xTxFlow> txFlow,
                                      CloseTxFlowCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (not cb) {
        // sync call
        proxy_->closeTxFlow(
                txFlow->getFlowId(),
                callStatus,
                error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = Status::FAILED;
        } else {
            if (pubs_.find(txFlow->getFlowId()) != pubs_.end()) {
                pubs_.erase(txFlow->getFlowId());
            }
            status = Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [txFlow, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(txFlow, static_cast<ErrorCode>(err.value_));
            } else {
                cb(txFlow, ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, txFlow, fcb]() {
            proxy_->closeTxFlowAsync(txFlow->getFlowId(), fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
        if (status == Status::SUCCESS) {
            if (pubs_.find(txFlow->getFlowId()) != pubs_.end()) {
                pubs_.erase(txFlow->getFlowId());
            }
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::changeSpsFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                            const SpsFlowInfo& flowInfo,
                                            ChangeSpsFlowInfoCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    telux::common::Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    capicv2x::Cv2xRadioTypes::SpsFlowInfo capiSpsInfo(
            static_cast<capicv2x::Cv2xRadioTypes::Priority::Literal>(flowInfo.priority),
            static_cast<capicv2x::Cv2xRadioTypes::Periodicity::Literal>(flowInfo.periodicity),
            flowInfo.periodicityMs,
            flowInfo.nbytesReserved,
            flowInfo.autoRetransEnabledValid,
            flowInfo.autoRetransEnabled,
            flowInfo.peakTxPowerValid,
            flowInfo.peakTxPower,
            flowInfo.mcsIndexValid,
            flowInfo.mcsIndex,
            flowInfo.txPoolIdValid,
            flowInfo.txPoolId);
    if (not cb) {
        // sync call
        proxy_->changeSpsFlowInfo(
                txFlow->getFlowId(),
                capiSpsInfo,
                callStatus,
                error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [txFlow, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(txFlow, static_cast<ErrorCode>(err.value_));
            } else {
                cb(txFlow, ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, txFlow, capiSpsInfo, fcb]() {
            proxy_->changeSpsFlowInfoAsync(
                txFlow->getFlowId(),
                capiSpsInfo,
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::requestSpsFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                             RequestSpsFlowInfoCallback cb)
{
    // async only call
    Status status = telux::common::Status::FAILED;;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (!cb) {
        return Status::INVALIDPARAM;
    }

    // async call
    auto fcb = [txFlow, cb](const CommonAPI::CallStatus &callstatus,
            const capitelux::CommonDefines::ErrorCode &err,
            const capicv2x::Cv2xRadioTypes::SpsFlowInfo &flowInfo) {
        SpsFlowInfo spsInfo = Capi2teluxHelper::capi2teluxSpsFlowInfo(flowInfo);
        if (callstatus == CommonAPI::CallStatus::SUCCESS) {
            cb(txFlow, spsInfo, static_cast<ErrorCode>(err.value_));
        } else {
            cb(txFlow, spsInfo, ErrorCode::GENERIC_FAILURE);
        }
    };

    auto f = [this, txFlow, fcb]() {
        proxy_->requestSpsFlowInfoAsync(
            txFlow->getFlowId(),
            fcb);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        status = sp->add(f, myself_, std::launch::async);
    }

    return status;
}
telux::common::Status CapiCv2xRadio::changeEventFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                              const EventFlowInfo &flowInfo,
                                              ChangeEventFlowInfoCallback cb)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    telux::common::Status status = telux::common::Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    capicv2x::Cv2xRadioTypes::EventFlowInfo capiEventFlowInfo(
            flowInfo.autoRetransEnabledValid,
            flowInfo.autoRetransEnabled,
            flowInfo.peakTxPowerValid,
            flowInfo.peakTxPower,
            flowInfo.mcsIndexValid,
            flowInfo.mcsIndex,
            flowInfo.txPoolIdValid,
            flowInfo.txPoolId,
            flowInfo.isUnicast);
    if (not cb) {
        // sync call
        proxy_->changeEventFlowInfo(
                txFlow->getFlowId(),
                capiEventFlowInfo,
                callStatus,
                error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [txFlow, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(txFlow, static_cast<ErrorCode>(err.value_));
            } else {
                cb(txFlow, ErrorCode::GENERIC_FAILURE);
            }
        };
        auto f = [this, txFlow, capiEventFlowInfo, fcb]() {
            proxy_->changeEventFlowInfoAsync(
                    txFlow->getFlowId(),
                    capiEventFlowInfo,
                    fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::requestCapabilities(RequestCapabilitiesCallback cb)
{
    // async only call
    Status status = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (!cb) {
        return Status::INVALIDPARAM;
    }

    // async call
    auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
            const capitelux::CommonDefines::ErrorCode &err,
            const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities& capabilities) {
        Cv2xRadioCapabilities cap = Capi2teluxHelper::capi2teluxRadioCapbilities(capabilities);
        ErrorCode teluxError = ErrorCode::GENERIC_FAILURE;
        if (callstatus == CommonAPI::CallStatus::SUCCESS &&
                err == capitelux::CommonDefines::ErrorCode::SUCCESS) {
            teluxError = ErrorCode::SUCCESS;
        } else {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                teluxError = static_cast<ErrorCode>(err.value_);
            } else {
                teluxError = ErrorCode::GENERIC_FAILURE;
            }
        }
        cb(cap, teluxError);
    };
    auto f = [this, fcb]() {
        proxy_->requestCapabilityAsync(fcb);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        status = sp->add(f, myself_, std::launch::async);
    }
    return status;
}

telux::common::Status CapiCv2xRadio::requestDataSessionSettings(
        RequestDataSessionSettingsCallback cb) {
    return Status::NOTALLOWED;
}

telux::common::Status CapiCv2xRadio::updateSrcL2Info(UpdateSrcL2InfoCallback cb) {
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (not cb) {
        // sync call
        proxy_->updateSrcL2Info(
                callStatus,
                error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, fcb]() {
            proxy_->updateSrcL2InfoAsync(fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::updateTrustedUEList (
    const TrustedUEInfoList &infoList,
    UpdateTrustedUEListCallback cb) {
    return Status::NOTALLOWED;
}

std::string CapiCv2xRadio::getIfaceNameFromIpType(TrafficIpType ipType)
{
    ServiceStatus teluxServiceStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    capitelux::CommonDefines::ErrorCode err;
    capitelux::CommonDefines::ServiceStatus serviceStatus;
    std::string ifaceNameIp("");

    proxy_->getIfaceNameFromIpType(static_cast<capicv2x::Cv2xRadioTypes::TrafficIpType::Literal>(ipType),
        callStatus, err, ifaceNameIp);
    if (callStatus == CommonAPI::CallStatus::SUCCESS &&
            err == capitelux::CommonDefines::ErrorCode::SUCCESS ) {
        teluxServiceStatus = static_cast<ServiceStatus>(serviceStatus.value_);
    } else {
        if (err != capitelux::CommonDefines::ErrorCode::SUCCESS) {
            COMMONAPI_ERROR("getIfaceNameFromIpType returned error");
        } else {
            COMMONAPI_ERROR("CapiCv2xRadio::getIfaceNameFromIpType: RPC failure");
        }
    }
    return ifaceNameIp;
};

telux::common::Status CapiCv2xRadio::createCv2xTcpSocket(
        const EventFlowInfo &eventInfo, const SocketInfo &sockInfo, CreateTcpSocketCallback cb) {
    Status status = telux::common::Status::FAILED;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    if (!cb) {
        return Status::INVALIDPARAM;
    }

    capicv2x::Cv2xRadioTypes::EventFlowInfo capiEventInfo(
        eventInfo.autoRetransEnabledValid,
        eventInfo.autoRetransEnabled,
        eventInfo.peakTxPowerValid,
        eventInfo.peakTxPower,
        eventInfo.mcsIndexValid,
        eventInfo.mcsIndex,
        eventInfo.txPoolIdValid,
        eventInfo.txPoolId,
        eventInfo.isUnicast);

    capicv2x::Cv2xRadioTypes::SocketInfo capiSockInfo(
        sockInfo.serviceId,
        sockInfo.localPort);

    // async call
    auto fcb = [sockInfo, cb](const CommonAPI::CallStatus &callstatus,
        const capitelux::CommonDefines::ErrorCode &err) {
        ErrorCode teluxError = ErrorCode::GENERIC_FAILURE;
        std::shared_ptr<ICv2xTxRxSocket> socket = nullptr;

        if (callstatus == CommonAPI::CallStatus::SUCCESS &&
                err == capitelux::CommonDefines::ErrorCode::SUCCESS) {
            std::shared_ptr<Cv2xDataSubscriber> ddsSub = std::make_shared<Cv2xDataSubscriber>();

            if (ddsSub->init() != false) {
                socket = std::make_shared<CapiCv2xTxRxSocket>(sockInfo.serviceId,
                    ddsSub->getClientSock());
                teluxError = ErrorCode::SUCCESS;
            } else {
                COMMONAPI_ERROR("failed to init dds subscriber");
                teluxError = ErrorCode::GENERIC_FAILURE;
            }
        } else {
            COMMONAPI_ERROR("rpc call failure");
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                teluxError = static_cast<ErrorCode>(err.value_);
            } else {
                teluxError = ErrorCode::GENERIC_FAILURE;
            }
        }
        if (cb) {
            cb(socket, teluxError);
        }
    };

    auto f = [this, capiEventInfo, capiSockInfo, fcb]() {
        proxy_->createCv2xTcpSocketAsync(capiEventInfo, capiSockInfo, fcb);
    };
    auto sp = taskQ_.lock();
    if(sp) {
        status = sp->add(f, myself_, std::launch::async);
    }

    return status;
}

telux::common::Status CapiCv2xRadio::closeCv2xTcpSocket(std::shared_ptr<ICv2xTxRxSocket> sock,
                                                    CloseTcpSocketCallback cb) {
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (not cb) {
        // sync call
        proxy_->closeCv2xTcpSocket(
            sock->getId(),
            callStatus,
            error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [sock, cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(sock, static_cast<ErrorCode>(err.value_));
            } else {
                cb(sock, ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, sock, fcb]() {
            proxy_->closeCv2xTcpSocketAsync(
                sock->getId(),
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::registerTxStatusReportListener(uint16_t port,
        std::shared_ptr<ICv2xTxStatusReportListener> listener,
        telux::common::ResponseCallback cb) {

    return Status::NOTALLOWED;
}

telux::common::Status CapiCv2xRadio::deregisterTxStatusReportListener(uint16_t port,
                        telux::common::ResponseCallback cb) {
    return Status::NOTALLOWED;
}

telux::common::Status CapiCv2xRadio::setGlobalIPInfo(const IPv6AddrType &ipv6Addr,
                        telux::common::ResponseCallback cb){

    Status status = Status::FAILED;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    std::vector< uint8_t > vecIpv6Addr(std::begin(ipv6Addr.ipv6Addr), std::end(ipv6Addr.ipv6Addr));
    std::shared_ptr<capicv2x::Cv2xRadioTypes::IPv6AddrType> capiIpv6Addr =
        std::make_shared<capicv2x::Cv2xRadioTypes::IPv6AddrType>(
        ipv6Addr.prefixLen,
        vecIpv6Addr);

    if (not cb) {
        // sync call
        proxy_->setGlobalIPInfo(
            (capicv2x::Cv2xRadioTypes::IPv6AddrType)*(capiIpv6Addr.get()),
            callStatus,
            error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, capiIpv6Addr, fcb]() {
            proxy_->setGlobalIPInfoAsync(
                (capicv2x::Cv2xRadioTypes::IPv6AddrType)*(capiIpv6Addr.get()),
                fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::setGlobalIPUnicastRoutingInfo(
        const GlobalIPUnicastRoutingInfo &destL2Addr,
        telux::common::ResponseCallback cb) {

    Status status = Status::FAILED;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    capicv2x::Cv2xRadioTypes::GlobalIPUnicastRoutingInfo capiDestL2Addr;
    std::vector<uint8_t> addrs = std::vector<uint8_t>();
    for (auto addr : destL2Addr.destMacAddr) {
        addrs.push_back(addr);
    }
    capiDestL2Addr.setDestMacAddr(addrs);

    if (not cb) {
        // sync call
        proxy_->setGlobalIPUnicastRoutingInfo(
            capiDestL2Addr,
            callStatus,
            error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, capiDestL2Addr, fcb]() {
            proxy_->setGlobalIPUnicastRoutingInfoAsync(capiDestL2Addr, fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

telux::common::Status CapiCv2xRadio::injectVehicleSpeed(uint32_t speed, telux::common::ResponseCallback cb) {
    Status status = Status::FAILED;
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    capitelux::CommonDefines::ErrorCode error;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }

    if (not cb) {
        // sync call
        proxy_->injectVehicleSpeed(
            speed,
            callStatus,
            error);
        if (callStatus != CommonAPI::CallStatus::SUCCESS) {
            status = telux::common::Status::FAILED;
        } else {
            status = telux::common::Status::SUCCESS;
        }
    } else {
        // async call
        auto fcb = [cb](const CommonAPI::CallStatus &callstatus,
                const capitelux::CommonDefines::ErrorCode err) {
            if (callstatus == CommonAPI::CallStatus::SUCCESS) {
                cb(static_cast<ErrorCode>(err.value_));
            } else {
                cb(ErrorCode::GENERIC_FAILURE);
            }
        };

        auto f = [this, speed, fcb]() {
            proxy_->injectVehicleSpeedAsync(speed, fcb);
        };
        auto sp = taskQ_.lock();
        if(sp) {
            status = sp->add(f, myself_, std::launch::async);
        }
    }

    return status;
}

bool CapiCv2xRadio::isInitialized() const
{
    return isInitialized_;
}

bool CapiCv2xRadio::isReady() const
{
    return false;
}

std::future<telux::common::Status> CapiCv2xRadio::onReady()
{
    return std::future<telux::common::Status>();
}

}  // namespace cv2x

}  // namespace telux
