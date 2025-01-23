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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /**
  * @file: RadioTransmit.h
  *
  * @brief: Application that handles CV2x radio transmits.
  *
  */
#ifndef __RADIO_TRANSMIT_H__
#define __RADIO_TRANSMIT_H__

#include "RadioInterface.h"
#include <vector>
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string>

using std::array;
using std::make_shared;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::ICv2xTxRxSocket;
using telux::cv2x::SocketInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::L2FilterInfo;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::Priority;
using std::vector;
using std::string;




class RadioTransmit: public RadioInterface {

private:
    // TrafficCategory category;

    Priority spsPriority;
    uint32_t spsResSize;
    struct sockaddr_in6 destSock;
    int simSock = -1;
    bool isSim = false;
    struct sockaddr_in destAddress;
    struct sockaddr_in clientAddress;
    uint16_t destPort;
    string ipv4_src;
    std::shared_ptr<SpsFlowInfo> spsFlowInfo = nullptr;
    uint64_t lastTxMonotonicTime_ = 0;
    uint64_t actualSPSTxIntervalMs_ = 0;
    string flowType;
    TrafficIpType trafficType_ = TrafficIpType::TRAFFIC_NON_IP;
    shared_ptr<ICv2xTxFlow> flow = nullptr;

public:
    uint16_t getPort() const;

    /**
    * Constructor for an Event Flow.
    * @param ipv4 an string representation of an IP version 4 address.
    * @param port a uint16_t value for the transmit port.
    */
    RadioTransmit(const string ipv4, const uint16_t port);
    RadioTransmit(const RadioOpt radioOpt, const string ipv4_dst, const uint16_t port);
    /**
    * Constructor for an Event Flow.
    * @param eventInfo a EventFlowInfo with the specifications of the flow.
    * @param category a TrafficCategory.
    * @param trafficType a TrafficIpType.
    * @param port a uint16_t value for the transmit port.
    * @param serviceId a uint32_t value that IDs the event.
    * @see TrafficCategory
    * @see TrafficIpType
    * @see EventFlowInfo
    */
    RadioTransmit(const EventFlowInfo eventInfo,
                  const TrafficCategory category,
                  const TrafficIpType trafficType,
                  const uint16_t port,
                  const uint32_t serviceId);


    /**
    * Constructor for a Sps Flow.
    * @param spsInfo SpsFlow Info with all the Sps creation information
    * @param category TrafficCategory.
    * @param trafficType TrafficIpType.
    * @param port uint16_t value for the transmit port.
    * @param serviceId uint32_t value that IDs the event.
    * @param microSleepTime uint32_t value that specifies the millisecond wait time until next send.
    * @see TrafficCategory
    * @see TrafficIpType
    */
    RadioTransmit(const SpsFlowInfo spsInfo,
                  const TrafficCategory category,
                  const TrafficIpType trafficType,
                  const uint16_t port,
                  const uint32_t serviceId);

    /**
    * Method that transmits data in a buffer based in the constructed flow.
    * @param buf a char pointer of the data buffer to be sent.
    * @param bufLen a uint16_t value representing the length of the data buffer.
    * @param priority a enum value representing the priority to be mapped to traffic class.
    * @return result value 0 on success and 1 on fail.
    */
    uint8_t transmit(const char* buf, const uint16_t bufLen, Priority priority);

    /**
    * Method that transmits data in a buffer based in the constructed flow.
    * @param buf a char pointer of the data buffer to be sent.
    * @param bufLen a uint16_t value representing the length of the data buffer.
    * @return result value 0 on success and 1 on fail.
    */
    uint8_t updateSpsFlow(const SpsFlowInfo spsInfo);

    Priority getSpsPriority();
    uint32_t getSpsResSize();
    /**
    * Method that closes flow.
    * @return result value 0 on success and 1 on fail.
    */
    uint8_t closeFlow();

    /**
    * Method that configures ipv6 destination sock for TCP/IP Simulations
    * @param port an uint16_t data holds the port.
    * @param destAddress a char* that holds the IP address of the destination
    *
    */
    void configureIpv6(const uint16_t port, const char* destAddress);

    int getTxInterval(uint64_t& periodicityMs);
    uint64_t latestTxRxTimeMonotonic() override;


};

#endif
