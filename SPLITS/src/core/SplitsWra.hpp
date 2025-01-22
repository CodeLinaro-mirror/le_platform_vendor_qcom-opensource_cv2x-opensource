/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITSWRA_HPP__
#define __SPLITSWRA_HPP__
#include <chrono>
#include <mutex>
#include <condition_variable>
#include "SplitsCore.hpp"
#include "Ieee1609Dot3Wsa.h"
#include "RadioInterface.h"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"

using v1::commonapi::SPLITS::SplitsConfig;

namespace SPLITS {
namespace Core {

class SplitsCore;

class SplitsWra{
public:
    SplitsWra(SPLITS::Core::Role role, std::shared_ptr<RadioInterface> radio,
            std::shared_ptr<SplitsConfig::WraConfig> config) :
        role_(role),
        radio_(radio),
        config_(config)
    {
        interval_ = std::chrono::milliseconds::zero();
    }
    ~SplitsWra();

    int onReceiveWra(const IEEE1609Dot3::idl::RoutingAdvertisement &wra,
            const uint8_t *sourceMac, int macAddrLen);
    int fillWra(std::shared_ptr<v2xMsg> msg);
private:
    int setGlobalIPv6Prefix(void);
    int clearGlobalIPv6Prefix(void);
    void threadFunc(int routerLifetime);
    std::thread wraThread_;
    bool ipSessionActive_ = false;
    bool stop_ = false;
    std::chrono::milliseconds interval_;
    std::chrono::time_point<std::chrono::high_resolution_clock> tp_now;
    uint8_t prevSourceMac[CV2X_MAC_ADDR_LEN];
    Role role_;
    std::shared_ptr<RadioInterface> radio_;
    std::shared_ptr<SplitsConfig::WraConfig> config_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
} //namespace Core
} //namespace SPLITS
#endif
