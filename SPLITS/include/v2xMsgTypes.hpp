/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __V2XMSGTYPES_HPP__
#define __V2XMSGTYPES_HPP__
#include <type_traits>
#include <map>

#ifndef SAE_PSID_WSA
#define SAE_PSID_WSA (0x87)
#endif
#ifndef SAE_PSID_BSM
#define SAE_PSID_BSM (0x20)
#endif
#ifndef SAE_PSID_SPAT
#define SAE_PSID_SPAT (0x82)
#endif
#ifndef SAE_PSID_MAP
#define SAE_PSID_MAP (0x204097)
#endif
#ifndef SAE_PSID_RTCM
#define SAE_PSID_RTCM (0x81)
#endif
#ifndef SAE_PSID_RSM
#define SAE_PSID_RSM (0x83)
#endif
#ifndef SAE_PSID_SRM
#define SAE_PSID_SRM (0x204096)
#endif
#ifndef SAE_PSID_SSM
#define SAE_PSID_SSM (0x204095)
#endif
#ifndef SAE_PSID_SDSM
#define SAE_PSID_SDSM (0x90)
#endif

#ifndef CSAE_AID_BSM
#define CSAE_AID_BSM (111)
#endif
#ifndef CSAE_AID_BSM_EVENT
#define CSAE_AID_BSM_EVENT (112)
#endif
#ifndef CSAE_AID_BSM_EMG
#define CSAE_AID_BSM_EMG (113)
#endif
#ifndef CSAE_AID_BSM_EMG_EVENT
#define CSAE_AID_BSM_EMG_EVENT (114)
#endif
#ifndef CSAE_AID_BSM_AM_OBU
#define CSAE_AID_BSM_AM_OBU (3617)
#endif
#ifndef CSAE_AID_MAP
#define CSAE_AID_MAP (3618)
#endif
#ifndef CSAE_AID_SPAT
#define CSAE_AID_SPAT (3619)
#endif
#ifndef CSAE_AID_RSI_STATIC
#define CSAE_AID_RSI_STATIC (3620)
#endif
#ifndef CSAE_AID_RSI_SEMI_STATIC
#define CSAE_AID_RSI_SEMI_STATIC (3621)
#endif
#ifndef CSAE_AID_RSI_DYNAMIC
#define CSAE_AID_RSI_DYNAMIC (3622)
#endif
#ifndef CSAE_AID_RSM
#define CSAE_AID_RSM (3623)
#endif

namespace SPLITS {

enum class StackType {
    STACK_TYPE_SAE = 1,
    STACK_TYPE_CSAE,
    STACK_TYPE_ETSI,
};
//enum class defined with bitmask, with operator overload.
//So we can do V2X_SAE_MSG_BSM | V2X_SAE_MSG_SPAT, etc
enum class v2xMsgType {
    V2X_MSG_UNKNOWN = 0,
    V2X_SAE_MSG_WSA = 1,
    V2X_SAE_MSG_BSM = 1 << 1,
    V2X_SAE_MSG_SPAT = 1 << 2,
    V2X_SAE_MSG_MAP = 1 << 3, 
    V2X_SAE_MSG_RTCM = 1<< 4,
    V2X_SAE_MSG_RSM = 1 << 5,
    V2X_SAE_MSG_SRM = 1 << 6,
    V2X_SAE_MSG_SSM = 1 << 7,
    V2X_SAE_MSG_SDSM = 1 << 8,
    V2X_CSAE_MSG_BSM = 1 << 9,
    V2X_CSAE_MSG_SPAT = 1 << 10,
    V2X_CSAE_MSG_MAP =  1 << 11,
    V2X_CSAE_MSG_RSM =  1 << 12,
    V2X_CSAE_MSG_RSI =  1 << 13,
    V2X_MSG_PDU = 1 << 14,
};
inline constexpr v2xMsgType operator|( v2xMsgType L, v2xMsgType R) {
    return static_cast<v2xMsgType>(
        static_cast<unsigned int>(L) |
        static_cast<unsigned int>(R));
}

inline constexpr v2xMsgType operator&( v2xMsgType L, v2xMsgType R) {
    return static_cast<v2xMsgType>(
        static_cast<unsigned int>(L) &
        static_cast<unsigned int>(R));
}
inline constexpr v2xMsgType operator~( v2xMsgType L) {
    return static_cast<v2xMsgType>(
        ~static_cast<unsigned int>(L) );
}
inline bool isCSAE(v2xMsgType type) {
    return static_cast<bool>((type &(v2xMsgType::V2X_CSAE_MSG_BSM |
                v2xMsgType::V2X_CSAE_MSG_SPAT |
                v2xMsgType::V2X_CSAE_MSG_MAP |
                v2xMsgType::V2X_CSAE_MSG_RSM |
                v2xMsgType::V2X_CSAE_MSG_RSI)));
}
} //namespace SPLITS
#endif //__V2XMSGTYPES_HPP__
