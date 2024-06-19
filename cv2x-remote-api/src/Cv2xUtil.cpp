/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "telux/cv2x/Cv2xUtil.hpp"


namespace telux {
namespace cv2x {

template <typename E>
constexpr typename std::underlying_type<E>::type getUnderlyingValue(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}
/*The offset between cv2x flows priorities and ipv6 traffic classes, cv2x flows priority min
* value(MOST_URGENT) is 0, traffic class minimum value is 1(0 means unset/default).
*/
static constexpr uint8_t PRIORITY_TCLASS_OFFSET = 1;

uint8_t Cv2xUtil::priorityToTrafficClass(Priority priority) {
    return static_cast<uint8_t>(PRIORITY_TCLASS_OFFSET + getUnderlyingValue(priority));
}

Priority Cv2xUtil::TrafficClassToPriority(uint8_t trafficClass) {
    Priority p = static_cast<Priority>(trafficClass - PRIORITY_TCLASS_OFFSET);
    if (p < Priority::MOST_URGENT || p > Priority::PRIORITY_BACKGROUND) {
        return Priority::PRIORITY_UNKNOWN;
    }
    return p;
}

}  // end of namespace cv2x
}  // end namespace telux
