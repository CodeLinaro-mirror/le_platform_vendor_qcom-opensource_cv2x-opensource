/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <cstring>
#include <iomanip>
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include "v2xMsgSae.hpp"
#include "SplitsWra.hpp"
#include "Logger.hpp"

namespace SPLITS {
namespace Core {

SplitsWra::~SplitsWra() {
    if (ipSessionActive_) {
        if (role_ == Role::OBU) {
        {
            {
               std::lock_guard<std::mutex> lk(mtx_);
               stop_ = true;
            }
            cv_.notify_all();
            if (wraThread_.joinable()) {
                wraThread_.join();
            }
        }
        (void)clearGlobalIPv6Prefix();
        }
    }
}

int SplitsWra::onReceiveWra(const IEEE1609Dot3::idl::RoutingAdvertisement &wra,
        const uint8_t *sourceMacAddr, int macAddrlen) {
    int ret = 0;
    telux::cv2x::IPv6AddrType ipPrefix;
    telux::cv2x::GlobalIPUnicastRoutingInfo routingInfo;

    if (ipSessionActive_ == false) {
        if (wra.ipPrefix().size() > CV2X_IPV6_ADDR_ARRAY_LEN) {
            LOGE(__FUNCTION__, " invalid ip prefix length: ", wra.ipPrefix().size());
            return -1;
        }
        //received first valid WRA message
        tp_now = std::chrono::high_resolution_clock::now();
        int i = 0;
        for (const auto& val : wra.ipPrefix()) {
            ipPrefix.ipv6Addr[i] = val;
            i++;
        }
        ipPrefix.prefixLen = wra.ipPrefixLength();
        std::memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
        if (radio_ != nullptr) {
            if ((ret = radio_->setGlobalIPInfo(ipPrefix, config_->getServiceId()))) {
                LOGE(__FUNCTION__, " Failed to set Global IP");
                stop_ = true;
                return -1;
            } else {
                LOGD(__FUNCTION__, " Global IP set");
            }
            std::memcpy(routingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if ((ret = radio_->setRoutingInfo(routingInfo))) {
                LOGE(__FUNCTION__, " Failed to set routing Info");
                stop_ = true;
                return ret;
            } else {
                LOGD(__FUNCTION__, " Routing info set");
            }
            auto func = [this](int routerLifetime) {
                threadFunc(routerLifetime);
            };
            //Launch a thread to monitor the received WRA message, if it timed out, we assume we
            //are going out of range of the RSU.
            if (!wraThread_.joinable()) {
                wraThread_ = std::thread(func, wra.lifetime());
                ipSessionActive_ = true;
            } else {
                if (ipSessionActive_ == false) {
                    wraThread_.join();
                    wraThread_ = std::thread(func, wra.lifetime());
                    ipSessionActive_ = true;
                } else {
                    cv_.notify_all();
                }
            }
        }
    } else {
        if (interval_ == std::chrono::milliseconds::zero()) {
            //Received 2nd WRA message, calculate the period
            auto diff = std::chrono::high_resolution_clock::now() - tp_now;
            interval_ = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        }
        //notify the thread that we have received more WRA messages.
        cv_.notify_all();
        //handle MAC address change due to t2 timer
        if (std::memcmp(sourceMacAddr, prevSourceMac, CV2X_MAC_ADDR_LEN)) {
            std::memcpy(routingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            std::memcpy(prevSourceMac, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if ((ret = radio_->setRoutingInfo(routingInfo))) {
                LOGE(__FUNCTION__, " Failed to update routing info");
            }
        }
    }
    return ret;
}

void SplitsWra::threadFunc(int routerLifetime) {
    std::unique_lock<std::mutex> lk(mtx_);
    std::cv_status status;

    while(not stop_) {
        lk.lock();
        if (interval_ == std::chrono::milliseconds::zero()) {
            //WRA message interval not yet calculated, we just received 1st message.
            //use routerLifetime as the time we wait to maintain active ip session.
            std::chrono::milliseconds wt(routerLifetime*1000);
            status = cv_.wait_for(lk, wt);
        } else {
            //we know the interval of WRA message, if we didn't receive WRA message within
            //3*interval, we deem the OBU went out of range of the RSU, stop ip session.
            status = cv_.wait_for(lk, 3*interval_);
        }
        lk.unlock();
        if (status == std::cv_status::timeout) {
            radio_->onWraTimedout();
            ipSessionActive_ = false;
            LOGD(__FUNCTION__, " IP session stopped due to WRA timeout");
            return;
        }
    }
}

int SplitsWra::fillWra(std::shared_ptr<v2xMsg> msg) {
    std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> sp;
    auto wsaMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
    if (wsaMsg == nullptr) {
        LOGE(__FUNCTION__, " No msg ");
        return -1;
    }
    if ((sp = wsaMsg->SrvAdvMsg()) == nullptr) {
        sp = std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>();
        if (sp == nullptr) {
            LOGE(__FUNCTION__, " Failed to create wsa idl message");
            return -1;
        } else {
            wsaMsg->SrvAdvMsg(sp);
        }
        //populate WSA IDL message.
        sp->version(3); //1609.3 2016
        IEEE1609Dot3::idl::SrvAdvBody body;
        IEEE1609Dot3::idl::RoutingAdvertisement routingAdvertisement;
        IEEE1609Dot3::idl::IPv6Address idlIpAddr;
        IEEE1609Dot3::idl::IpV6Prefix idlPrefix;

        auto prefix = config_->getIpPrefix();
        auto defaultGw = config_->getDefaultGateway();
        auto primaryDns = config_->getPrimaryDns();

        routingAdvertisement.lifetime(config_->getRouterLifetime());
        std::copy(prefix.begin(), prefix.end(), idlPrefix.begin());
        routingAdvertisement.ipPrefix(std::move(idlPrefix));
        routingAdvertisement.ipPrefixLength(config_->getIpPrefixLength());
        if (defaultGw.size()) {
            std::copy(defaultGw.begin(), defaultGw.end(), idlIpAddr.begin());
            routingAdvertisement.defaultGateway(std::move(idlIpAddr));
        }
        if (primaryDns.size()){
            std::copy(primaryDns.begin(), primaryDns.end(), idlIpAddr.begin());
            routingAdvertisement.primaryDns(std::move(idlIpAddr));
        }

        body.routingAdvertisement(std::move(routingAdvertisement));
        //sp->body().changeCount()
        sp->body(std::move(body));
    }
    return 0;
}
int SplitsWra::setGlobalIPv6Prefix(void) {
    int ret = 0;
    telux::cv2x::IPv6AddrType IpPrefix;
    if (ipSessionActive_ == false) {
        IpPrefix.prefixLen = config_->getIpPrefixLength();
        std::memcpy(IpPrefix.ipv6Addr, config_->getIpPrefix().data(), IpPrefix.prefixLen);
        if (radio_ != nullptr) {
            ret = radio_->setGlobalIPInfo(IpPrefix, config_->getServiceId());
        } else {
            ret = -1;
        }
        if (!ret) {
            LOGD(__FUNCTION__, " Global IP info set");
            ipSessionActive_ = true;
        } else {
            LOGE(__FUNCTION__, " Global IP info failed to ");
        }
    }
    return ret;
}
int SplitsWra::clearGlobalIPv6Prefix(void) {
    int ret = 0;
    ipSessionActive_ = false;
    if (radio_ != nullptr) {
        ret = radio_->clearGlobalIPInfo();
    }
    return ret;
}

} //namespace Core
} //namespace SPLITS
