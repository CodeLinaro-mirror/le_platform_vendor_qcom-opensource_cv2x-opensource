/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _CAPICV2XRADIOMANAGER_HPP__
#define _CAPICV2XRADIOMANAGER_HPP__

#include <vector>
#include <condition_variable>
#include <v1/commonapi/telux/cv2x/Cv2xRadioManagerProxy.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>

using namespace telux::cv2x;
using namespace telux::common;

namespace capitelux = v1::commonapi::telux::common;
namespace capicv2x = v1::commonapi::telux::cv2x;

namespace telux {

namespace cv2x{

class CapiCv2xRadioManager : public ICv2xRadioManager {
public:
    CapiCv2xRadioManager() { };
    ~CapiCv2xRadioManager();

    bool isReady()
    { 
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

    /*telux::common::Status updateConfiguration(const std::string & configFilePath,
                                                     UpdateConfigurationCallback cb)
    {
        return Status::FAILED;
    };*/

    telux::common::Status setPeakTxPower(int8_t txPower, telux::common::ResponseCallback cb)
    {
         return Status::FAILED;
    };

    telux::common::Status setL2Filters(const std::vector<L2FilterInfo> &filterList,
                                              telux::common::ResponseCallback cb)
    {
        return Status::FAILED;
    };

    telux::common::Status removeL2Filters(const std::vector<uint32_t> &l2IdList,
                                              telux::common::ResponseCallback cb)
    {
        return Status::FAILED;
    };

    telux::common::Status getSlssRxInfo(GetSlssRxInfoCallback cb)
    {
        return Status::FAILED;
    };

    telux::common::Status injectCoarseUtcTime(uint64_t utc, telux::common::ResponseCallback cb)
    {
        return Status::FAILED;
    };
private:
    void initSync(InitResponseCb callback);
    void onGetCv2xRadioResponse(telux::common::ServiceStatus status);

    bool exiting_ = false;
    std::condition_variable exitCv_;
    std::mutex exitMutex_;
    std::shared_ptr<capicv2x::Cv2xRadioManagerProxy<>> proxy_;
    std::vector<std::weak_ptr<ICv2xListener>> listeners_;
    std::vector<telux::common::InitResponseCb> cv2xRadioInitCallbacks_;
    std::weak_ptr<ICv2xRadio> radio_;
    ServiceStatus cv2xRadioInitStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    std::mutex radioMutex_;
    std::mutex listenersMutex_;
    std::condition_variable radioCv_;
    std::shared_future<void>asyncFuture_;
};

} //namespace cv2x
} //namespace telux
#endif // _CAPICV2XRADIOMANAGER_HPP__
