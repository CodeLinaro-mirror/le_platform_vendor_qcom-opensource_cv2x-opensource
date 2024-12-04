/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_REMOTE_SERVER_HELPER_HPP
#define CV2X_REMOTE_SERVER_HELPER_HPP

#include <algorithm>

#include "telux/cv2x/Cv2xRadioTypes.hpp"
#include <v1/commonapi/telux/cv2x/Cv2xRadioProxy.hpp>

namespace capi = ::v1::commonapi::telux;
namespace capicv2x = ::v1::commonapi::telux::cv2x;

using FireCv2xStatus =
    std::function<void (const capicv2x::Cv2xRadioTypes::Cv2xStatusEx &_statusEx)>;

using FireL2AddrEvent = std::function<void (const uint32_t &_newL2Addr)>;
using FireSpsOffsetEvent = std::function<void (const int32_t &_spsId,
    const capicv2x::Cv2xRadioTypes::MacDetails &_details)>;
using FireCapabilitiesEvent = std::function<void (
    const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities &_capabilities)>;
using FireMacAddrCloneEvent = std::function<void (const bool &_detected)>;

typedef struct {
    FireCv2xStatus fireCv2xStatus;
} RpcCv2xMgrFires;

typedef struct {
    FireCv2xStatus fireCv2xStatus;
    FireL2AddrEvent fireL2AddrEvent;
    FireSpsOffsetEvent fireSpsOffsetEvent;
    FireCapabilitiesEvent fireCapabilitiesEvent;
    FireMacAddrCloneEvent fireMacAddrCloneEvent;
} RpcRadioFires;

class Cv2xRemoteServerHelper {
 public:
static capicv2x::Cv2xRadioTypes::Cv2xStatus telux2capiCv2xStatus(
    const telux::cv2x::Cv2xStatus &status) {
    capicv2x::Cv2xRadioTypes::Cv2xStatus capiStatus(
        static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.rxStatus),
        static_cast<capicv2x::Cv2xRadioTypes::Cv2xStatusType::Literal>(status.txStatus),
        static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.rxCause),
        static_cast<capicv2x::Cv2xRadioTypes::Cv2xCauseType::Literal>(status.txCause),
        status.cbrValue,
        status.cbrValueValid);

    return capiStatus;
}

static uint8_t bitset2Uint8(const std::bitset<8> bits) {
    //convert std::bitset<8> to uint8_t
    uint8_t data = 0;
    std::size_t idx = 0;
    while (idx < bits.size() && bits.test(idx)) {
        data |= (1 << idx);
        ++idx;
    }

    return data;
}

};

#endif
