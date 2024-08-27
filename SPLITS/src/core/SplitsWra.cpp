/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <cstring>
#include <iomanip>
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include "v2xMsgSae.hpp"
#include "SplitsWra.hpp"

namespace SPLITS {

namespace Core {

SplitsWra::~SplitsWra()
{
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
        const uint8_t *sourceMacAddr, int macAddrlen)
{
    int ret = 0;
    telux::cv2x::IPv6AddrType ipPrefix;
    telux::cv2x::GlobalIPUnicastRoutingInfo routingInfo;

    if (ipSessionActive_ == false) {
        if (wra.ipPrefix().size() > CV2X_IPV6_ADDR_ARRAY_LEN) {
            std::cerr << "invalid ip prefix length: " << wra.ipPrefix().size() << std::endl;
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
                std::cerr << "Failed to set Global IP" << std::endl;
                stop_ = true;
                return -1;
            } else {
                std::cout << "Global IP set" << std::endl;
            }
            std::memcpy(routingInfo.destMacAddr, sourceMacAddr, CV2X_MAC_ADDR_LEN);
            if ((ret = radio_->setRoutingInfo(routingInfo))) {
                std::cerr << "Failed to set routing Info" << std::endl;
                stop_ = true;
                return ret;
            } else {
                std::cout << "Routing info set" << std::endl;
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
                std::cerr << "Failed to update routing info" << std::endl;
            }
        }
    }
    return ret;
}
void SplitsWra::threadFunc(int routerLifetime)
{
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
            std::cout << "IP session stopped due to WRA timeout" << std::endl;
            return;
        }
    }
}

int SplitsWra::fillWra(std::shared_ptr<v2xMsg> msg)
{
    std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> sp;
    auto wsaMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
    if (wsaMsg == nullptr) {
        std::cerr << "No msg " << std::endl;
        return -1;
    }
    if ((sp = wsaMsg->SrvAdvMsg()) == nullptr) {
        sp = std::make_shared<IEEE1609Dot3::idl::SrvAdvMsg>();
        if (sp == nullptr) {
            std::cerr << "Failed to create wsa idl message" << std::endl;
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
#if 0
        std::cout << "ipPrefixLen=" << config_->getIpPrefixLength() << std::endl;
        std::cout << "defaultGw: " << std::endl;
        for (auto const & v : defaultGw) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)v << ":";
        }
        std::cout << std::endl;
        std::cout << "primaryDns: " << std::endl;
        for (auto const & v : primaryDns) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)v << ":";
        }
        std::cout << std::endl;
#endif
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
int SplitsWra::setGlobalIPv6Prefix(void)
{
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
            std::cout << "Global IP info set" << std::endl;
            ipSessionActive_ = true;
        } else {
            std::cerr << "Global IP info failed to " << std::endl;
        }
    }
    return ret;
}
int SplitsWra::clearGlobalIPv6Prefix(void)
{
    int ret = 0;
    ipSessionActive_ = false;
    if (radio_ != nullptr) {
        ret = radio_->clearGlobalIPInfo();
    }
    return ret;
}

} //namespace Core
} //namespace SPLITS
