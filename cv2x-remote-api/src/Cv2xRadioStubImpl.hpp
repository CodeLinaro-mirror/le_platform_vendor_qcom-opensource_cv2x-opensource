/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2XRADIOSTUBIMPL_HPP_
#define CV2XRADIOSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/telux/cv2x/Cv2xRadioStubDefault.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include "Cv2xDataPublisher.h"
#include "Cv2xDataSubscriber.h"

#include "Cv2xRemoteServerHelper.hpp"

namespace capiteluxcommon = v1::commonapi::telux::common;
namespace capicv2x = v1::commonapi::telux::cv2x;

class Cv2xRemoteSrvRadioListener : public telux::cv2x::ICv2xRadioListener {
 public:
    Cv2xRemoteSrvRadioListener(RpcRadioFires fireMethods);
    void onStatusChanged(telux::cv2x::Cv2xStatusEx statusEx) override;
    void onL2AddrChanged(uint32_t newL2Address) override;
    void onSpsOffsetChanged(int spsId, telux::cv2x::MacDetails details) override;
    void onSpsSchedulingChanged(const telux::cv2x::SpsSchedulingInfo & schedulingInfo) override;
    void onCapabilitiesChanged(const telux::cv2x::Cv2xRadioCapabilities & capabilities) override;
    void onMacAddressCloneAttack(const bool detected) override;

 private:
    RpcRadioFires rpcRadionFires_;
};


class Cv2xRadioStubImpl: public v1::commonapi::telux::cv2x::Cv2xRadioStubDefault {

public:
    Cv2xRadioStubImpl();
    virtual ~Cv2xRadioStubImpl();

    virtual void createRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
        uint16_t _port, std::vector< uint32_t > _idList, createRxSubscriptionReply_t _reply);

    virtual void getServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        getServiceStatusReply_t _reply);

    virtual void enableRxMetaDataReport(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
        bool _enable, std::vector< uint32_t > _idList, enableRxMetaDataReportReply_t _reply);

    virtual void createTxSpsFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType     _ipType,
        uint32_t _serviceId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SpsFlowInfo _spsInfo,
        uint16_t _spsSrcPort, bool _eventSrcPortValid, uint16_t _eventSrcPort,
        createTxSpsFlowReply_t _reply);

    virtual void createTxEventFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipTyp,
        uint32_t _serviceId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _flowInfo,
        uint16_t _eventSrcPort, createTxEventFlowReply_t _reply);

    virtual void closeRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _subscriptionId, closeRxSubscriptionReply_t reply);

    virtual void closeTxFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
            uint32_t _txFlowId, closeTxFlowReply_t _reply);

    virtual void changeSpsFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _txFlowId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SpsFlowInfo _spsInfo,
        changeSpsFlowInfoReply_t _reply);

    virtual void requestSpsFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _txFlowId, requestSpsFlowInfoReply_t _reply);

    virtual void changeEventFlowInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _txFlowId,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _eventInfo,
        changeEventFlowInfoReply_t _reply);

    virtual void requestCapability(const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCapabilityReply_t _reply);

    virtual void requestDataSessionSettings(const std::shared_ptr<CommonAPI::ClientId> _client,
        requestDataSessionSettingsReply_t _reply);

    virtual void updateSrcL2Info(const std::shared_ptr<CommonAPI::ClientId> _client,
        updateSrcL2InfoReply_t _reply);

    virtual void updateTrustedUEList(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrustedUEInfoList _infoList,
        updateTrustedUEListReply_t _reply);

    virtual void getIfaceNameFromIpType(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
        getIfaceNameFromIpTypeReply_t _reply);

    virtual void createCv2xTcpSocket(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::EventFlowInfo _eventFlowInfo,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SocketInfo _socketInfo,
        createCv2xTcpSocketReply_t _reply);

    virtual void closeCv2xTcpSocket(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _sockId, closeCv2xTcpSocketReply_t _reply);

    virtual void setGlobalIPInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::IPv6AddrType _ipv6Addr,
        setGlobalIPInfoReply_t _reply);

    virtual void setGlobalIPUnicastRoutingInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::GlobalIPUnicastRoutingInfo _destL2Addr,
        setGlobalIPUnicastRoutingInfoReply_t _reply);

    virtual void getCapabilities(const std::shared_ptr<CommonAPI::ClientId> _client,
        getCapabilitiesReply_t _reply);

    virtual void injectVehicleSpeed(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint32_t _speed, injectVehicleSpeedReply_t _reply);

    static capicv2x::Cv2xRadioTypes::Cv2xStatus teluxCv2xStatus2capi(
        const telux::cv2x::Cv2xStatus status);

private:
     void initListener(std::shared_ptr<telux::cv2x::ICv2xRadio> radio);

    std::shared_ptr<telux::cv2x::ICv2xRadioManager>cv2xRadioManager_ = nullptr;
    std::shared_ptr<telux::cv2x::ICv2xRadio>cv2xRadio_ = nullptr;
    std::map<uint32_t, std::shared_ptr<telux::cv2x::ICv2xRxSubscription>> rxSubs_;
    std::map<uint32_t, std::shared_ptr<telux::cv2x::ICv2xTxFlow>> txFlows_;
    std::map<uint32_t, std::shared_ptr<Cv2xDataPublisher>> pubs_;
    std::map<uint32_t, std::shared_ptr<Cv2xDataSubscriber>> subs_;

    std::shared_ptr<Cv2xRemoteSrvRadioListener> radioListener_ = nullptr;
    std::mutex m_;
    std::map<uint32_t, std::shared_ptr<telux::cv2x::ICv2xTxRxSocket>> sockets_;
    telux::cv2x::Cv2xRadioCapabilities capabilities_;
};

#endif // CV2XRADIOSTUBIMPL_HPP_
