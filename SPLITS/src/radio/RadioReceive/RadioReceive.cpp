/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
 /**
  * @file: RadioReceive.cpp
  *
  * @brief: Implementation of RadioReceive
  *
  */
#include <sstream>

#include "RadioReceive.h"
#include "Logger.hpp"

static int gRxCount = 0;
void RadioReceive::rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        this->gRxSub = rxSub;
    }
};

RadioReceive::RadioReceive(const TrafficCategory category,
                           const TrafficIpType trafficIpType, const uint16_t port,
                           std::shared_ptr<std::vector<uint32_t>> idList) {

    if (!this->ready(category, RadioType::RX)) {
        LOGD(__FUNCTION__, " Radio Checks on RadioReceive creation fail");
    }
    this->category = category;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCb =
        [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
            ErrorCode error) {
            rxSubCallback(rxSub, error);
            cb->onResponse(error);
        };
    if (Status::SUCCESS == cv2xRadio->createRxSubscription(trafficIpType, port, respCb, idList)) {
        if (ErrorCode::SUCCESS == cb->getResponse()) {
            std::stringstream ss;            
            ss << "Rx Subscription creation succeeds";
            if (nullptr != idList) {
                ss << " for SID: ";
                auto idp = idList.get();
                for (int i = 0; i < idp->size(); i++)
                    ss << idp->at(i) << ' ';
            }
            LOGD(__FUNCTION__, " ", ss.str());
        } else {
            LOGD(__FUNCTION__, " Rx Subscription creation fails.");
        }
    } else {
        LOGE(__FUNCTION__, " Rx Subscription creation fails.");
    }
}

/*
 * RadioReceive ctor for only simulation purposes. Communication over Ethernet.
 */
RadioReceive::RadioReceive(RadioOpt radioOpt, const string ipv4_dst,
                           const uint16_t port) {
    isSim = true;
    this->ipv4_src = radioOpt.ipv4_src;
    this->srcAddress = {0};
    this->serverAddress = {0};
    logTag = "SIMULATION:UDP:";

    // Creating socket file descriptor
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOGE(__FUNCTION__, logTag, " Error Creating Socket");
        return;
    }
    do {
        // setting up network parameters for sender and receiving devices
        this->srcAddress.sin_family = AF_INET;
        this->srcAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_dst.data(), &(this->srcAddress.sin_addr)) <= 0) {
            LOGE(__FUNCTION__, logTag, " Invalid ip address of other device ", ipv4_dst);
            LOGE(__FUNCTION__, logTag, " Will attempt accepting from any ip address now ");
            this->srcAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        this->serverAddress.sin_family = AF_INET;
        this->serverAddress.sin_port = htons(port);
        if(inet_pton(AF_INET, ipv4_src.data(), &(this->serverAddress.sin_addr)) <= 0){
            LOGE(__FUNCTION__, " Invalid ip address for this device: ", ipv4_src);
            break;
        }
        if (bind(sock, (struct sockaddr*) &(this->serverAddress),
            sizeof(this->serverAddress)) < 0) {
            LOGE(__FUNCTION__,  logTag, " Socket ", sock,  " with IP: ", ipv4_dst,
                 " and port: ", port, " failed binding");
            break;
        }
        simRxSock = sock;
        return;
    } while(0);

    //Something wrong, do cleanup
    close(sock);
    return;
}


uint32_t RadioReceive::receive(const char* buf, int len) {
    uint8_t sourceMac[CV2X_MAC_ADDR_LEN];
    int cv2x_mac_addr_len = CV2X_MAC_ADDR_LEN;
    return receive(buf, len, sourceMac, cv2x_mac_addr_len);
}

uint32_t RadioReceive::receive(const char* buf, int len,
                               uint8_t *sourceMacAddr,
                               int& macAdrLen) {
    int socket = -1;
    if (!buf || !sourceMacAddr ||
        macAdrLen < CV2X_MAC_ADDR_LEN) {
        LOGE(__FUNCTION__, " Invalid input params");
        return -1;
    }
    // check if this receive is for simulation and/or for UDP
    if (isSim) {
        if ((socket = simRxSock) < 0) {
            return -1;
        }
    } else {
        if (nullptr == this->gRxSub) {
            LOGE(__FUNCTION__, " Invalid Rx subs");
            return -1;
        }
        socket = this->gRxSub->getSock();
    }

    struct pollfd fd;
    int ret;
    fd.fd = socket;
    fd.events = POLLIN;
    fd.revents = 0;
    ret = poll(&fd, 1, 100); // 100 milisec timeout
    // timed out or had error receiving
    if (ret <= 0) {
        if (rVerbosity && ret < 0) {
            LOGE(__FUNCTION__, " ", strerror(errno));
        }
        return ret;
    }
    struct timespec ts;
    uint32_t srcAddressSize = sizeof(this->srcAddress);
    uint32_t bytesReceived = 0;

    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);
    struct msghdr message = {0};
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov[1] = {0};
    iov[0].iov_base = (char*)buf;
    iov[0].iov_len = len;
    message.msg_name = &from;
    message.msg_namelen = fromLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    // udp connection over eth or radio
    bytesReceived = recvmsg(socket, &message, 0);

    msgL2SrcAdrr = ntohl(from.sin6_addr.s6_addr32[3]);

    if (bytesReceived > 0) {
        if (enableCsvLog_) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            lastRxMonotonicTime_ = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
        }
        sourceMacAddr[0] = 0;
        sourceMacAddr[1] = 0;
        sourceMacAddr[2] = 0;
        sourceMacAddr[3] = from.sin6_addr.s6_addr[13];
        sourceMacAddr[4] = from.sin6_addr.s6_addr[14];
        sourceMacAddr[5] = from.sin6_addr.s6_addr[15];
        gRxCount++;
        if (rVerbosity) {
            LOGD(__FUNCTION__, "#", std::dec, gRxCount, " Source MAC: ");
            for (int i = 0; i < CV2X_MAC_ADDR_LEN; i++) {
                LOGD(__FUNCTION__, std::hex, static_cast<int>(sourceMacAddr[i]), " ");
            }
        }
    } else {
        if (rVerbosity) {
            LOGE(__FUNCTION__, " Invalid message");
        }
        return -1;
    }

    return bytesReceived;
}

int RadioReceive::setL2Filters(std::vector<L2FilterInfo> filterList){
    promise<ErrorCode> p;
    auto cv2xRadioMgr = this->getCv2xRadioManager();
    if (nullptr == cv2xRadioMgr) {
        return -1;
    }
    cv2xRadioMgr->setL2Filters(filterList, [&p](ErrorCode error) {p.set_value(error);});
    if(rVerbosity) {
        LOGD(__FUNCTION__, " Setting l2 filters for flooding attack addresses");
    }
    if (ErrorCode::SUCCESS == p.get_future().get()) {
        if(rVerbosity) {
            LOGD(__FUNCTION__, " success to setL2Filters") ;
        }
        return 0;
    }
    LOGE(__FUNCTION__, " Failed to setL2Filters");
    return -1;
}

uint8_t RadioReceive::closeFlow(){
    if (isSim) {
        if (simRxSock >= 0) {
            if (close(simRxSock) >= 0) {
                LOGD(__FUNCTION__, logTag, " Receives socket closed succesfully.");
                simRxSock = -1;
            } else {
                LOGD(__FUNCTION__, logTag, "Problem closing Sockets in RadioReceive.");
            }
        }
    }

    if (rVerbosity)
        LOGD(__FUNCTION__, "Attempting to close wra-related subscriptions");
    clearGlobalIPInfo();

    if (this->gRxSub) {
        auto resp = -1;
        auto cv2xRadio = this->getCv2xRadio();
        if (nullptr == cv2xRadio) {
            resp = static_cast<uint8_t>(Status::FAILED);
        } else {
            auto cb = std::make_shared<CommonCallback>();
            auto respCb = [&](std::shared_ptr<ICv2xRxSubscription> rxSub,
                                    ErrorCode error){
                                        rxSubCallback(rxSub, error);
                                        cb->onResponse(error);
                                    };
            if (Status::SUCCESS == cv2xRadio->closeRxSubscription(this->gRxSub, respCb)){
                if (ErrorCode::SUCCESS == cb->getResponse()) {
                    resp = static_cast<uint8_t>(Status::SUCCESS);
                } else {
                    resp = static_cast<uint8_t>(Status::FAILED);
                }
            }else{
                resp = static_cast<uint8_t>(Status::FAILED);
            }
        }

        this->gRxSub = nullptr;
        LOGD(__FUNCTION__, "Rx subscription closed.");
        return resp;
    }
    return 0;
}

uint64_t RadioReceive::latestTxRxTimeMonotonic() {
    return lastRxMonotonicTime_;
}


int RadioReceive::getPort() const {
    if (gRxSub) {
        return gRxSub->getPortNum();
    } else {
        return -1;
    }
}
