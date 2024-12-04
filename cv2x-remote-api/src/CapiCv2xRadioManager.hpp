/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _CAPICV2XRADIOMANAGER_HPP__
#define _CAPICV2XRADIOMANAGER_HPP__

#include <stdint.h>

#include <vector>
#include <condition_variable>
#include <v1/commonapi/telux/cv2x/Cv2xRadioManagerProxy.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include "common/AsyncTaskQueue.hpp"

using namespace telux::cv2x;
using namespace telux::common;

namespace capitelux = v1::commonapi::telux::common;
namespace capicv2x = v1::commonapi::telux::cv2x;

namespace telux {

namespace cv2x{

extern std::shared_ptr<telux::common::AsyncTaskQueue<void>> clientTaskQ;

class CapiCv2xRadioManager : public ICv2xRadioManager ,
                             public std::enable_shared_from_this<CapiCv2xRadioManager> {
public:
    CapiCv2xRadioManager();
    ~CapiCv2xRadioManager();

    bool isReady() {
        /*deprecated */
        COMMONAPI_ERROR("deprecated API: isReady()");
        return false;
    };
    std::future<bool> onReady()
    {
        COMMONAPI_ERROR("deprecated API: onReady()");
        auto f = std::async(std::launch::async, [this] {
                      return false;});
        return f;
    };
    void init(InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus();

    std::shared_ptr<ICv2xRadio> getCv2xRadio(TrafficCategory category,
          telux::common::InitResponseCb cb = nullptr);

    telux::common::Status startCv2x(StartCv2xCallback cb);

    telux::common::Status stopCv2x(StopCv2xCallback cb);

    telux::common::Status requestCv2xStatus(RequestCv2xStatusCallback cb);

    telux::common::Status requestCv2xStatus(RequestCv2xStatusCallbackEx cb);

    telux::common::Status registerListener(std::weak_ptr<ICv2xListener> listener);

    telux::common::Status deregisterListener(std::weak_ptr<ICv2xListener> listener);

    telux::common::Status setPeakTxPower(int8_t txPower, telux::common::ResponseCallback cb);
    telux::common::Status setL2Filters(const std::vector<L2FilterInfo> &filterList,
                                              telux::common::ResponseCallback cb);
    telux::common::Status removeL2Filters(const std::vector<uint32_t> &l2IdList,
                                              telux::common::ResponseCallback cb);
    telux::common::Status getSlssRxInfo(GetSlssRxInfoCallback cb);
    telux::common::Status injectCoarseUtcTime(uint64_t utc, telux::common::ResponseCallback cb);
private:
    void initSync(InitResponseCb callback);
    bool initProxy();
    void onGetCv2xRadioResponse(telux::common::ServiceStatus status);
    void onConnectionChange(const CommonAPI::AvailabilityStatus& s);
    void getAvailableListeners(std::vector<std::weak_ptr<ICv2xListener>> &vec);
    void cv2xStatusCallback(telux::cv2x::Cv2xStatusEx status, telux::common::ErrorCode error);
    telux::common::Status onServiceStatusChange(const telux::common::ServiceStatus subsysStatus);
    telux::common::Status onCv2xStatusExChange(
        const capicv2x::Cv2xRadioTypes::Cv2xStatusEx &cv2xStatusEx);

    bool exiting_ = false;
    std::condition_variable exitCv_;
    std::mutex exitMutex_;
    std::weak_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::weak_ptr<CapiCv2xRadioManager> myself_;
    std::shared_ptr<capicv2x::Cv2xRadioManagerProxy<>> proxy_ = nullptr;
    std::vector<std::weak_ptr<ICv2xListener>> listeners_;
    std::vector<telux::common::InitResponseCb> cv2xRadioInitCallbacks_;
    std::weak_ptr<ICv2xRadio> radio_;
    ServiceStatus cv2xRadioInitStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    std::atomic<telux::common::ServiceStatus> subSysStatus_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE};
    std::mutex radioMutex_;
    std::mutex listenersMutex_;
    std::condition_variable radioCv_;
    uint32_t subscription_ = UINT32_MAX;
};

} //namespace cv2x
} //namespace telux
#endif // _CAPICV2XRADIOMANAGER_HPP__
