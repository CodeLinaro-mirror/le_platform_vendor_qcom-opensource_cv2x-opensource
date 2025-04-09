/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
  * @file: RadioTransmit.cpp
  *
  * @brief: Implementation of RadioTransmit
  *
  */

#include "RadioTransmit.h"
#include "Logger.hpp"
#include <cerrno>

RadioTransmit::RadioTransmit(const SpsFlowInfo spsInfo, const TrafficCategory category,
                             const TrafficIpType trafficType,
                             const uint16_t port, const uint32_t serviceId) {

    if (!ready(category, RadioType::TX)) {
        LOGD(__FUNCTION__, " Radio Checks on Sps Transmit Event Fail");
        //return static_cast<uint8_t>(Status::FAILED);
    }
    this->category = category;
    this->flowType = "spsFlow";
    this->trafficType_ = trafficType;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txSpsFlow,
                            std::shared_ptr<ICv2xTxFlow> txEventFlow,
                            ErrorCode spsError, ErrorCode eventError){
                                if (ErrorCode::SUCCESS == spsError) {
                                    this->flow = txSpsFlow;
                                }
                                cb->onResponse(spsError);
                            };
    if(Status::SUCCESS == cv2xRadio->createTxSpsFlow(trafficType, serviceId, spsInfo,
                port, false, 0, respCallback)){
        if(ErrorCode::SUCCESS == cb->getResponse()){
            LOGD(__FUNCTION__, " Sps flow created succesfully sid = ", serviceId);

            spsFlowInfo = std::make_shared<SpsFlowInfo>();
            if (spsFlowInfo) {
                memcpy(spsFlowInfo.get(), &spsInfo, sizeof(SpsFlowInfo));
                this->spsPriority = spsInfo.priority;
                this->spsResSize = spsInfo.nbytesReserved;
            }
        }
        else{
            LOGE(__FUNCTION__, " Sps Flow creation fails, future.get");
            //return static_cast<uint8_t>(Status::FAILED);
        }
    }
    else {
        LOGD(__FUNCTION__, " Sps Flow creation fails");
        //return static_cast<uint8_t>(Status::FAILED);
    }
}

RadioTransmit::RadioTransmit(const EventFlowInfo eventInfo,
                            const TrafficCategory category,
                            const TrafficIpType trafficType, const uint16_t port,
                            const uint32_t serviceId){
    if (!this->ready(category, RadioType::TX)) {
        LOGD(__FUNCTION__, " Radio Checks on Transmit Event fail");
        //return static_cast<uint8_t>(Status::FAILED);;
    }
    this->category = category;
    this->flowType = "eventFlow";
    this->trafficType_ = trafficType;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txEventFlow,
                            ErrorCode eventError){
                                if (ErrorCode::SUCCESS == eventError) {
                                    this->flow = txEventFlow;
                                }
                                cb->onResponse(eventError);
                            };
    EventFlowInfo testEventInfo;
    if(Status::SUCCESS == cv2xRadio->createTxEventFlow(trafficType, serviceId, testEventInfo,
                port, respCallback)){
        if(ErrorCode::SUCCESS == cb->getResponse()) {
            LOGD(__FUNCTION__, " Event Flow created succesfully");
        } else {
            LOGE(__FUNCTION__, " Event Flow creation fails, future.get");
        }
    } else {
        LOGE(__FUNCTION__, " Event Flow creation fails");
    }
}

RadioTransmit::RadioTransmit(const RadioOpt radioOpt, const string ipv4_dst, const uint16_t port) {
    LOGD(__FUNCTION__, " Now simulating transmission of messages...");
    isSim = true;
    this->flowType = "simFlow";

    this->ipv4_src = radioOpt.ipv4_src;
    this->clientAddress = {0};
    this->destAddress = {0};

    // udp
    this->simSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (simSock < 0) {
        LOGD(__FUNCTION__, " Error Creating Socket");
        return;
    }

    this->destAddress.sin_family = AF_INET;
    this->destAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ipv4_dst.data(), &(this->destAddress.sin_addr)) <= 0) {
        LOGD(__FUNCTION__, " Invalid ip address: ", ipv4_dst);
    }

    this->clientAddress.sin_family = AF_INET;
    this->clientAddress.sin_port = htons(port);
    if(inet_pton(AF_INET, this->ipv4_src.data(), &(this->clientAddress.sin_addr)) <= 0) {
        LOGD(__FUNCTION__, " Invalid ip address for client: ", ipv4_src.data());
    }
}

void RadioTransmit::configureIpv6(const uint16_t port, const char* destAddress) {
    string ifName;
    this->destSock.sin6_family = AF_INET6;
    this->destSock.sin6_port = htons((uint16_t)port);
    inet_pton(AF_INET6, destAddress, (void*) &this->destSock.sin6_addr);
    if (0 == getV2xIfaceName(trafficType_, ifName) && (not ifName.empty())) {
        this->destSock.sin6_scope_id = if_nametoindex(ifName.c_str());
    }
}

uint8_t RadioTransmit::transmit(const char* buf, const uint16_t bufLen, Priority priority) {
    struct timespec ts;
    if (isSim)
    {
        //udp
        return sendto(this->simSock, buf, bufLen,  0,
                        (const struct sockaddr *) &(this->destAddress),
                         sizeof(this->destAddress));
    }

    if (nullptr == this->flow) {
        LOGE(__FUNCTION__, " Invalid flow");
        return 1;
    }
    // Radio-based Communication
    auto resp = 0;
    auto sock = this->flow->getSock();

    if (sock == -1) {
        LOGD(__FUNCTION__, " Error on transmit, with socket value -1");
        return 1;
    }

    struct msghdr message = { 0 };
    struct iovec iov[1] = { 0 };
    struct cmsghdr* cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];

    iov[0].iov_base = (char*)buf;
    iov[0].iov_len = bufLen;
    message.msg_name = &this->destSock;
    message.msg_namelen = sizeof(this->destSock);
    message.msg_iov = iov;
    message.msg_iovlen = 1;

    if (Priority::PRIORITY_UNKNOWN > priority) {
        // map pppp to traffic class if the priority is valid
        // note that pppp is only used for one-shot transmission
        message.msg_control = control;
        message.msg_controllen = sizeof(control);
        cmsghp = CMSG_FIRSTHDR(&message);
        cmsghp->cmsg_level = IPPROTO_IPV6;
        cmsghp->cmsg_type = IPV6_TCLASS;
        cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
        *((int *)CMSG_DATA(cmsghp)) = static_cast<int>(priority) + 1;
        //int msg_prior = static_cast<int>(priority) + 1;
        //int msg_prior = 3;
        //memcpy(CMSG_DATA(cmsghp), &msg_prior, sizeof(int));
    }

    auto bytes_sent = sendmsg(sock, &message, 0);
    if(bytes_sent == bufLen){
        //resp = bytes_sent;
        resp = 0;
    }else{
        LOGE(__FUNCTION__, " Error Sending Data.");
        LOGE(__FUNCTION__, " Error is: ", strerror(errno));

        resp = 1;
    }
    if (!resp && enableCsvLog_) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        auto nowMonotonicTime = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
        if (spsFlowInfo) {
            /*calculate sps tx interval only if it is SPS flow*/
            if (lastTxMonotonicTime_ > 0) {
                actualSPSTxIntervalMs_ = nowMonotonicTime - lastTxMonotonicTime_;
            } else {
                /*Fist time tx message*/
                actualSPSTxIntervalMs_ = spsFlowInfo->periodicityMs;
            }
        }
        lastTxMonotonicTime_ = nowMonotonicTime;
    }

    return resp;
}

uint8_t RadioTransmit::updateSpsFlow(const SpsFlowInfo spsInfo) {
    auto resp = -1;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return -1;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> txSpsFlow,
                            ErrorCode spsError){
                                if (ErrorCode::SUCCESS == spsError) {
                                    this->flow = txSpsFlow;
                                }
                                cb->onResponse(spsError);
                            };
    if(Status::SUCCESS == cv2xRadio->changeSpsFlowInfo(this->flow, spsInfo, respCallback)){
        if(ErrorCode::SUCCESS == cb->getResponse()) {
            resp = static_cast<uint8_t>(Status::SUCCESS);
            if (spsFlowInfo) {
                memcpy(spsFlowInfo.get(), &spsInfo, sizeof(SpsFlowInfo));
            }
        } else {
            resp =  static_cast<uint8_t>(Status::FAILED);
        }
    } else {
        resp =  static_cast<uint8_t>(Status::FAILED);
    }
    return resp;
}

uint8_t RadioTransmit::closeFlow() {

    if (isSim)
    {
        const auto ans = close(simSock);
        if (ans < 0)
        {
            LOGD(__FUNCTION__, " Simulation socket failed to close.");
            return ans;
        }
        else {
            LOGD(__FUNCTION__, " Simulation socket closed succesfully.");
            return ans;
        }
    }

    if (this->flow) {
        auto resp = -1;
        auto cv2xRadio = this->getCv2xRadio();
        if (nullptr == cv2xRadio) {
            resp = static_cast<uint8_t>(Status::FAILED);
        } else {
            auto cb = std::make_shared<CommonCallback>();
            auto respCallback = [&](std::shared_ptr<ICv2xTxFlow> flow,
                                    ErrorCode error){
                                        cb->onResponse(error);
                                    };
            if(Status::SUCCESS == cv2xRadio->closeTxFlow(this->flow, respCallback)){
                if (ErrorCode::SUCCESS == cb->getResponse()){
                    resp = static_cast<uint8_t>(Status::SUCCESS);
                    if (spsFlowInfo) {
                        memset(spsFlowInfo.get(), 0, sizeof(SpsFlowInfo));
                    }
                }
                else{
                    resp = static_cast<uint8_t>(Status::FAILED);
                }
            }else{
                    resp = static_cast<uint8_t>(Status::FAILED);
            }
        }
        this->flow = nullptr;
        LOGD(__FUNCTION__, " Closing flow of type: ", flowType);
        if(resp != static_cast<uint8_t>(Status::FAILED)){
            LOGD(__FUNCTION__, " Tx flow closed.");
        }else{
            LOGD(__FUNCTION__, " Tx flow not closed correctly.");
        }
        return resp;
    }

    return 0;
}

int RadioTransmit::getTxInterval(uint64_t& periodicityMs) {
    int res = -1;
    if (enableCsvLog_ && spsFlowInfo) {
        periodicityMs = actualSPSTxIntervalMs_;
        res = 1;
    }
    return res;
}

Priority RadioTransmit::getSpsPriority(){
    return this->spsPriority;
}

uint32_t RadioTransmit::getSpsResSize(){
    return this->spsResSize;
}


uint64_t RadioTransmit::latestTxRxTimeMonotonic() {
    return lastTxMonotonicTime_;
}

int RadioTransmit::getPort() const {
    if (nullptr != this->flow) {
        return flow->getPortNum();
    } else {
        return -1;
    }
}
