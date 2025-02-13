/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2XRADIOMANAGERSTUBIMPL_HPP_
#define CV2XRADIOMANAGERSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/telux/cv2x/Cv2xRadioManagerStubDefault.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

#include "Cv2xRemoteServerHelper.hpp"

namespace capi = v1::commonapi::telux;
namespace capicv2x = v1::commonapi::telux::cv2x;

class Cv2xMgrListener : public telux::cv2x::ICv2xListener {
 public:
    Cv2xMgrListener(RpcCv2xMgrFires fireMethods);
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onStatusChanged(telux::cv2x::Cv2xStatusEx statusEx) override;
    void onSlssRxInfoChanged(const telux::cv2x::SlssRxInfo& slssInfo) override;

 private:
    RpcCv2xMgrFires rpcFireMethods_;
};

class Cv2xRadioManagerStubImpl: public telux::cv2x::ICv2xListener,
    public capicv2x::Cv2xRadioManagerStubDefault,
    public std::enable_shared_from_this<Cv2xRadioManagerStubImpl> {

public:
    Cv2xRadioManagerStubImpl();
    virtual ~Cv2xRadioManagerStubImpl();

    virtual void GetServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        GetServiceStatusReply_t _reply);
    virtual void startCv2x(const std::shared_ptr<CommonAPI::ClientId> _client,
        startCv2xReply_t _reply);
    virtual void stopCv2x(const std::shared_ptr<CommonAPI::ClientId> _client,
        stopCv2xReply_t _reply);
    virtual void requestCv2xStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCv2xStatusReply_t _reply);
    virtual void requestCv2xStatusEx(const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCv2xStatusExReply_t _reply);

    virtual void setPeakTxPower(const std::shared_ptr<CommonAPI::ClientId> _client,
        int8_t _txPower, setPeakTxPowerReply_t _reply);
    virtual void setL2Filters(const std::shared_ptr<CommonAPI::ClientId> _client,
        std::vector< ::v1::commonapi::telux::cv2x::Cv2xRadioTypes::L2FilterInfo > _filterList,
        setL2FiltersReply_t _reply);
    virtual void removeL2Filters(const std::shared_ptr<CommonAPI::ClientId> _client,
        std::vector< uint32_t > _l2IdList, removeL2FiltersReply_t _reply);
    virtual void getSlssRxInfo(const std::shared_ptr<CommonAPI::ClientId> _client,
        getSlssRxInfoReply_t _reply);
    virtual void injectCoarseUtcTime(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint64_t _utc, injectCoarseUtcTimeReply_t _reply);

private:
    std::shared_ptr<telux::cv2x::ICv2xRadioManager>cv2xRadioManager_ = nullptr;
    std::mutex m_;
    std::shared_ptr<Cv2xMgrListener>  cv2xMgrListener_ = nullptr;
};

#endif // CV2XRADIOMANAGERSTUBIMPL_HPP_
