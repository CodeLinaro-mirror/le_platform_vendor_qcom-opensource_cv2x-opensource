/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _CAPICV2XRADIO_HPP__
#define _CAPICV2XRADIO_HPP__

#include <v1/commonapi/telux/cv2x/Cv2xRadioProxy.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <Cv2xDataPublisher.h>
#include <Cv2xDataSubscriber.h>
#include "common/AsyncTaskQueue.hpp"

#include <mutex>

using namespace telux::cv2x;
using namespace telux::common;

namespace capitelux = v1::commonapi::telux::common;
namespace capicv2x = v1::commonapi::telux::cv2x;

namespace telux{
namespace cv2x{

class Capi2teluxHelper {
public:
    static Cv2xStatus capi2teluxCv2xStatus(const capicv2x::Cv2xRadioTypes::Cv2xStatus &status);
    static Cv2xStatusEx capi2teluxCv2xStatusEx(
        const capicv2x::Cv2xRadioTypes::Cv2xStatusEx statusEx);
    static std::vector<Cv2xPoolStatus> capi2teluxCv2xPoolStatus(
        const std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> &status);
    static std::vector<TxPoolIdInfo> capi2teluxSupportedPoolIds(
        const std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> &idInfos);
    static Cv2xRadioCapabilities capi2teluxRadioCapbilities(
        const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities &capbilities);
    static std::vector<TrustedUEInfo> capi2teluxTrustedUEs(
        const std::vector<capicv2x::Cv2xRadioTypes::TrustedUEInfo> &capiTrustedUEs);
    static SpsFlowInfo capi2teluxSpsFlowInfo(
        const capicv2x::Cv2xRadioTypes::SpsFlowInfo &flowInfo);
    static SlssRxInfo capi2teluxSlssRxInfo(
        const capicv2x::Cv2xRadioTypes::SlssRxInfo &flowInfo);
};

class CapiCv2xRadio : public ICv2xRadio ,
                      public std::enable_shared_from_this<CapiCv2xRadio> {
public:
    CapiCv2xRadio();
    ~CapiCv2xRadio();

    void init(std::weak_ptr<telux::common::AsyncTaskQueue<void>> taskQ,
        InitResponseCb callback = nullptr);

    bool isInitialized() const;

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<ICv2xRadioListener> listener);

    telux::common::Status deregisterListener(std::weak_ptr<ICv2xRadioListener> listener);

    telux::common::Status createRxSubscription(
        TrafficIpType ipType, uint16_t port, CreateRxSubscriptionCallback cb,
        std::shared_ptr<std::vector<uint32_t>> idList = nullptr);
    telux::common::Status enableRxMetaDataReport(TrafficIpType ipType,
                                                 bool enable,
                                                 std::shared_ptr<std::vector<std::uint32_t>> idList,
                                                 telux::common::ResponseCallback cb) override;
    telux::common::Status createTxSpsFlow(TrafficIpType ipType,
                                          uint32_t serviceId,
                                          const SpsFlowInfo& spsInfo,
                                          uint16_t spsSrcPort,
                                          bool eventSrcPortValid,
                                          uint16_t eventSrcPort,
                                          CreateTxSpsFlowCallback cb);

    telux::common::Status createTxEventFlow(TrafficIpType ipType,
                                            uint32_t serviceId,
                                            uint16_t eventSrcPort,
                                            CreateTxEventFlowCallback cb);

    telux::common::Status createTxEventFlow(TrafficIpType ipType,
                                            uint32_t serviceId,
                                            const EventFlowInfo& flowInfo,
                                            uint16_t eventSrcPort,
                                            CreateTxEventFlowCallback cb);

    telux::common::Status closeRxSubscription(std::shared_ptr<ICv2xRxSubscription> rxSub,
                                              CloseRxSubscriptionCallback cb);

    telux::common::Status closeTxFlow(std::shared_ptr<ICv2xTxFlow> txFlow,
                                      CloseTxFlowCallback cb);

    telux::common::Status changeSpsFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                            const SpsFlowInfo& spsInfo,
                                            ChangeSpsFlowInfoCallback cb);

    telux::common::Status requestSpsFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                             RequestSpsFlowInfoCallback cb);
    telux::common::Status changeEventFlowInfo(std::shared_ptr<ICv2xTxFlow> txFlow,
                                              const EventFlowInfo &flowInfo,
                                              ChangeEventFlowInfoCallback cb);

    telux::common::Status requestCapabilities(RequestCapabilitiesCallback cb);

    //Unimplemented
    telux::common::Status requestDataSessionSettings(
        RequestDataSessionSettingsCallback cb);
    telux::common::Status updateSrcL2Info(UpdateSrcL2InfoCallback cb);
    telux::common::Status updateTrustedUEList(
        const TrustedUEInfoList & infoList,
        UpdateTrustedUEListCallback cb);

    std::string getIfaceNameFromIpType(TrafficIpType ipType);
    telux::common::Status createCv2xTcpSocket(
        const EventFlowInfo &eventInfo, const SocketInfo &sockInfo, CreateTcpSocketCallback cb);
    telux::common::Status closeCv2xTcpSocket(std::shared_ptr<ICv2xTxRxSocket> sock,
        CloseTcpSocketCallback cb);
    telux::common::Status registerTxStatusReportListener( uint16_t port,
        std::shared_ptr<ICv2xTxStatusReportListener> listener,
        telux::common::ResponseCallback cb);
    telux::common::Status deregisterTxStatusReportListener( uint16_t port,
                           telux::common::ResponseCallback cb);
    telux::common::Status setGlobalIPInfo(const IPv6AddrType &ipv6Addr,
                           telux::common::ResponseCallback cb);
    telux::common::Status setGlobalIPUnicastRoutingInfo( const GlobalIPUnicastRoutingInfo &destL2Addr,
            telux::common::ResponseCallback cb);
    telux::common::Status injectVehicleSpeed(uint32_t speed, telux::common::ResponseCallback cb);

    Cv2xRadioCapabilities getCapabilities() const {
        return Cv2xRadioCapabilities();
    };
    bool isReady() const;
    std::future<telux::common::Status> onReady();

private:
    void initSync(InitResponseCb callback);

    bool exiting_ = false;
    std::vector<std::weak_ptr<ICv2xRadioListener>> listeners_;
    std::map<uint32_t, std::shared_ptr<Cv2xDataSubscriber>> subs_;
    std::map<uint32_t, std::shared_ptr<Cv2xDataPublisher>> pubs_;
    std::shared_ptr<capicv2x::Cv2xRadioProxy<>> proxy_;
    ServiceStatus serviceStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable exitCv_;
    std::mutex exitMutex_;
    std::mutex mutex_;
    std::mutex listenersMutex_;
    std::atomic<bool> isInitialized_;
    std::weak_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::weak_ptr<CapiCv2xRadio> myself_;
};

} //namespace cv2x
} //namespace telux
#endif //_CAPICV2XRADIO_HPP__
