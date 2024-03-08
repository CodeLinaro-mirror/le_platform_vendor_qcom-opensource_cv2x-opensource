/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2XRADIOMANAGERSTUBIMPL_HPP_
#define CV2XRADIOMANAGERSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/telux/cv2x/Cv2xRadioManagerStubDefault.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

class Cv2xRadioManagerStubImpl: public v1::commonapi::telux::cv2x::Cv2xRadioManagerStubDefault,
    public telux::cv2x::ICv2xListener,
    public std::enable_shared_from_this<Cv2xRadioManagerStubImpl> {

public:
    Cv2xRadioManagerStubImpl();
    virtual ~Cv2xRadioManagerStubImpl();
    std::shared_ptr<Cv2xRadioManagerStubImpl> getptr() {
        return shared_from_this();
    }

    virtual void GetServiceStatus(const std::shared_ptr<CommonAPI::ClientId> _client, GetServiceStatusReply_t _reply);
    virtual void startCv2x(const std::shared_ptr<CommonAPI::ClientId> _client, startCv2xReply_t _reply);
    virtual void stopCv2x(const std::shared_ptr<CommonAPI::ClientId> _client, stopCv2xReply_t _reply);
    virtual void requestCv2xStatus(const std::shared_ptr<CommonAPI::ClientId> _client, requestCv2xStatusReply_t _reply);
    virtual void requestCv2xStatusEx(const std::shared_ptr<CommonAPI::ClientId> _client, requestCv2xStatusExReply_t _reply);
    void onStatusChanged(telux::cv2x::Cv2xStatus status) override;
    v1::commonapi::telux::cv2x::Cv2xRadioTypes::Cv2xStatus telux2capiCv2xStatus(const telux::cv2x::Cv2xStatus &status);

private:
    std::shared_ptr<telux::cv2x::ICv2xRadioManager>cv2xRadioManager_ = nullptr;
    telux::cv2x::Cv2xStatus currentStatus_;
    bool isListenerRegistered = false;
    std::mutex m_;
};

#endif // CV2XRADIOMANAGERSTUBIMPL_HPP_
