/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef CAPICV2XTXRXSOCKET_HPP
#define CAPICV2XTXRXSOCKET_HPP

#include <telux/cv2x/Cv2xTxRxSocket.hpp>

namespace telux {

namespace cv2x {

class CapiCv2xTxRxSocket : public ICv2xTxRxSocket {
public:
    CapiCv2xTxRxSocket(uint32_t serviceId, int sock)
        : serviceId_(serviceId), sock_(sock) {}

    uint32_t getId() const { return id_; }

    uint32_t getServiceId() const { return serviceId_; }

    int getSocket() const { return sock_; }

    struct sockaddr_in6 getSocketAddr() const { return sockAddr_; }

    uint16_t getPortNum() const { return ntohs(sockAddr_.sin6_port); }

    virtual ~CapiCv2xTxRxSocket() {}

protected:
    uint32_t id_;

    uint32_t serviceId_;

    int sock_ = -1;

    struct sockaddr_in6 sockAddr_ = {0};
};


} // namespace cv2x

} // namespace telux
#endif // #ifndef CAPICV2XTXRXSOCKET_HPP
