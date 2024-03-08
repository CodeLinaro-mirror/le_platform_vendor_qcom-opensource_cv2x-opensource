/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef CAPICV2XRXSUBSCRIPTION_HPP
#define CAPICV2XRXSUBSCRIPTION_HPP

#include <vector>
#include <unistd.h>
#include <telux/cv2x/Cv2xRxSubscription.hpp>

namespace telux {

namespace cv2x {


class CapiCv2xRxSubscription : public ICv2xRxSubscription {
public:
    CapiCv2xRxSubscription(int sock, uint32_t subscriptionId) :
        id_(subscriptionId), sock_(sock){};

    virtual uint32_t getSubscriptionId() const { return id_;};

    virtual TrafficIpType getIpType() const { return ipType_;};

    virtual int getSock() const { return sock_;};

    virtual struct sockaddr_in6 getSockAddr() const { /* invalid sockAddr */return sockAddr_;};

    virtual uint16_t getPortNum() const { /* invalid port */return 0;};

    virtual void closeSock(){
        if (sock_>= 0){
            close(sock_);
        }
    };

    virtual std::shared_ptr<std::vector<uint32_t>> getServiceIDList() const{ return idList_;};

    virtual void setServiceIDList(const std::shared_ptr<std::vector<uint32_t>> idList){
        idList_ = idList;
    };
    virtual ~CapiCv2xRxSubscription() {}

protected:

    uint32_t id_;
    TrafficIpType ipType_;
    int sock_ = -1;
    struct sockaddr_in6 sockAddr_ = {0};
    std::shared_ptr<std::vector<uint32_t>> idList_ = nullptr;
};


} // namespace cv2x

} // namespace telux
#endif // #ifndef CAPICV2XRXSUBSCRIPTION_HPP
