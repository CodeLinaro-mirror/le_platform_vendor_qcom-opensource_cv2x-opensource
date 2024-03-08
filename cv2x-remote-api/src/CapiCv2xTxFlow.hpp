/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef CAPICV2XTXFLOW_HPP__
#define CAPICV2XTXFLOW_HPP__

#include <vector>
#include <unistd.h>
#include <telux/cv2x/Cv2xTxFlow.hpp>

namespace telux {

namespace cv2x {

enum class Cv2xTxFlowType {
    EVENT,
    SPS,
};

/**
 * This class encapsulates a V2X Tx Event Flow and is returned
 * to the client.
 */
class CapiCv2xTxEventFlow : public ICv2xTxFlow {
public:

    CapiCv2xTxEventFlow(TrafficIpType ipType, uint32_t serviceId, int sock,
            uint32_t flowId) :
        id_(flowId),  ipType_(ipType), serviceId_(serviceId), sock_(sock)
    {
    };

    virtual uint32_t getFlowId() const {return id_;};

    virtual TrafficIpType getIpType() const {return ipType_;};

    virtual uint32_t getServiceId() const {return serviceId_;};

    virtual int getSock() const { return sock_;};

    virtual struct sockaddr_in6 getSockAddr() const {/* invalid sock addr */return sockAddr_;};
    virtual uint16_t getPortNum() const { /* invalid port num */ return 0;};

    virtual Cv2xTxFlowType getFlowType() const { return Cv2xTxFlowType::EVENT;};

    virtual void closeSock(){
        if (sock_ >= 0) {
            close(sock_);
        }
        sock_ = -1;
    };

    virtual ~CapiCv2xTxEventFlow() {}

protected:

    uint32_t id_;
    TrafficIpType ipType_;
    uint32_t serviceId_;
    int sock_ = -1;
    struct sockaddr_in6 sockAddr_ = {0};
};


/**
 * This class encapsulates a V2X Tx SPS Flow and is returned
 * to the client.
 */
class CapiCv2xTxSpsFlow : public CapiCv2xTxEventFlow {
public:
    CapiCv2xTxSpsFlow(uint32_t id, TrafficIpType ipType, uint32_t serviceId, int sock,
                  const SpsFlowInfo & spsInfo) :
        CapiCv2xTxEventFlow(ipType, serviceId, sock, id),  spsInfo_(spsInfo) {};

    virtual Cv2xTxFlowType getFlowType() const { return  Cv2xTxFlowType::SPS;};

    virtual ~CapiCv2xTxSpsFlow() {}

    SpsFlowInfo getSpsFlowInfo(void) { return spsInfo_;};

    void setSpsFlowInfo(const SpsFlowInfo & spsInfo) {spsInfo_ = spsInfo;};

protected:

    uint32_t id_;
    TrafficIpType ipType_;
    uint32_t serviceId_;
    int sock_ = -1;
    struct sockaddr_in6 sockAddr_ = {0};
    SpsFlowInfo spsInfo_;
};


} // namespace cv2x

} // namespace telux

#endif // #ifndef CAPICV2XTXFLOW_HPP__

