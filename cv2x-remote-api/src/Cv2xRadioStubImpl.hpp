/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2XRADIOSTUBIMPL_HPP_
#define CV2XRADIOSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/telux/cv2x/Cv2xRadioStubDefault.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include "Cv2xDataPublisher.h"
#include "Cv2xDataSubscriber.h"

namespace capiteluxcommon = v1::commonapi::telux::common;
namespace capicv2x = v1::commonapi::telux::cv2x;

class Cv2xRadioStubImpl: public v1::commonapi::telux::cv2x::Cv2xRadioStubDefault,
    public telux::cv2x::ICv2xRadioListener,
    public std::enable_shared_from_this<Cv2xRadioStubImpl> {

public:
    Cv2xRadioStubImpl();
    virtual ~Cv2xRadioStubImpl();
    std::shared_ptr<Cv2xRadioStubImpl> getptr() {
        return shared_from_this();
    }

    virtual void createRxSubscription(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
            uint16_t _port, std::vector< uint32_t > _idList, createRxSubscriptionReply_t _reply);

    virtual void getServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client, getServiceStatusReply_t _reply);

    virtual void enableRxMetaDataReport(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipType,
            bool _enable, std::vector< uint32_t > _idList, enableRxMetaDataReportReply_t _reply);

    virtual void createTxSpsFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType     _ipType,
            uint32_t _serviceId, ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::SpsFlowInfo _spsInfo, 
            uint16_t _spsSrcPort, bool _eventSrcPortValid, uint16_t _eventSrcPort, createTxSpsFlowReply_t _reply);

    virtual void createTxEventFlow(const std::shared_ptr<CommonAPI::ClientId> _client,
            ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::TrafficIpType _ipTyp,
            uint32_t _serviceId, uint16_t _eventSrcPort, createTxEventFlowReply_t _reply);

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

     void onStatusChanged(telux::cv2x::Cv2xStatus status) override;
     void onStatusChanged(telux::cv2x::Cv2xStatusEx status) override;
     void onL2AddrChanged(uint32_t newL2Addr) override;
     void onSpsOffsetChanged(int spsId, telux::cv2x::MacDetails details) override;
     void onSpsSchedulingChanged(const telux::cv2x::SpsSchedulingInfo & schedulingInfo) override;
     void onCapabilitiesChanged(const telux::cv2x::Cv2xRadioCapabilities & capabilities) override;
     void onMacAddressCloneAttack(const bool detected) override;

     static capicv2x::Cv2xRadioTypes::Cv2xStatus teluxCv2xStatus2capi(const telux::cv2x::Cv2xStatus status);

private:
     inline uint8_t bitset2Uint8(const std::bitset<8> bits);
     bool cv2xStatusChanged(const telux::cv2x::Cv2xStatus status);


     std::shared_ptr<telux::cv2x::ICv2xRadioManager>cv2xRadioManager_ = nullptr;
     std::shared_ptr<telux::cv2x::ICv2xRadio>cv2xRadio_ = nullptr;
     std::map<uint32_t, std::shared_ptr<telux::cv2x::ICv2xRxSubscription>> rxSubs_;
     std::map<uint32_t, std::shared_ptr<telux::cv2x::ICv2xTxFlow>> txFlows_;
     std::map<uint32_t, std::shared_ptr<Cv2xDataPublisher>> pubs_;
     std::map<uint32_t, std::shared_ptr<Cv2xDataSubscriber>> subs_;

     telux::cv2x::Cv2xStatusEx currentCv2xStatusEx_;
     std::promise<telux::common::ErrorCode> CallbackPromise_;
     std::mutex m_;
};

#endif // CV2XRADIOSTUBIMPL_HPP_
