/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_REMOTE_RADIO_HELPER_HPP
#define CV2X_REMOTE_RADIO_HELPER_HPP

#include "telux/cv2x/Cv2xRadioTypes.hpp"
#include <v1/commonapi/telux/cv2x/Cv2xRadioProxy.hpp>

#define PROXY_TIMEOUT_MS 50000

namespace capicv2x = v1::commonapi::telux::cv2x;
namespace capi = v1::commonapi::telux;

namespace telux {
namespace cv2x {

// Helper class for Cv2xRadio
class Capi2teluxHelper {
public:
    static void resetV2xStatusEx(Cv2xStatusEx &status);
    static Cv2xStatus capi2teluxCv2xStatus(const capicv2x::Cv2xRadioTypes::Cv2xStatus &status);
    static Cv2xStatusEx capi2teluxCv2xStatusEx(
        const capicv2x::Cv2xRadioTypes::Cv2xStatusEx statusEx);
    static std::vector<Cv2xPoolStatus> capi2teluxCv2xPoolStatus(
        const std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> &status);
    static std::vector<TxPoolIdInfo> capi2teluxSupportedPoolIds(
        const std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> &idInfos);
    static Cv2xRadioCapabilities capi2teluxRadioCapbilities(
        const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities &capbilities);
    static std::vector<TrustedUEInfo> capi2teluxTrustedUEs(
        const std::vector<capicv2x::Cv2xRadioTypes::TrustedUEInfo> &capiTrustedUEs);
    static SpsFlowInfo capi2teluxSpsFlowInfo(
        const capicv2x::Cv2xRadioTypes::SpsFlowInfo &flowInfo);
    static SlssRxInfo capi2teluxSlssRxInfo(
        const capicv2x::Cv2xRadioTypes::SlssRxInfo &flowInfo);
};

}  // end namespace cv2x
}  // end namespace telux
#endif
