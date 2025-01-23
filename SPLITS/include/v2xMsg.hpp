/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef V2XMSG_HPP_
#define V2XMSG_HPP_

#include <cstdint>
#include <memory>
#include <iostream>
#include "v2xMsgTypes.hpp"

#define SPLITS_DDS_DOMAIN_ID    2
namespace SPLITS {


enum class MsgSource {
    LOCAL,
    REMOTE
};
enum class MsgFormat {
    MSG_FORMAT_BLOB,     //encoded, signed, with network and transport header.
    MSG_FORMAT_SIGNED,   //encoded, signed, no network and transport header.
    MSG_FORMAT_UNSIGNED, //encoded, but not signed, no network and transport header.
    MSG_FORMAT_UNENCODED, //unencoded, unsigned, no network and transport header.
    MSG_FORMAT_UNKNOWN   //don't know
};

enum class Ieee1609Dot2Content{
    unsecuredData,
    signedData,
    encryptedData,
    signedCertificateRequest
};

enum class Ieee1609Dot2TagClass {
    UNIVERSAL = 0,
    APPLICATION = 1,
    CONTEXT_SPECIFIC = 2,
    PRIVATE = 3,
};

struct Ieee1609Dot2Data {
    uint8_t protocolVersion;
    Ieee1609Dot2TagClass tagclass;
    Ieee1609Dot2Content content;
    int payloadLen;
};

/**
 * Encapsulates all information about v2x message
 */
class v2xMsg {
public:
    v2xMsg() {
        buf = 0;
        bufLen = 0;
        msgType_ = v2xMsgType::V2X_MSG_UNKNOWN;
    }
    virtual ~v2xMsg() {
    }

    //L2
    uint8_t srcMac_[6];    /**< source's MAC address */
    int macAddrLen_;
    v2xMsgType msgType_;   /**< Type of the message */
    MsgSource source_;     /**< message is from Local or Remote */
    MsgFormat format_;     /**< format of the message, signed/unsigned, encoded/unencoded*/
    uint16_t port_;        /**< from which port the message is transceived*/

    /**
     * get const reference to the decoded IEEE1609.2 header
     *
     * @return reference to the IEEE1609.2 header content.
     */
    inline const Ieee1609Dot2Data & Ieee1609Dot2() {
        return Ieee1609Dot2_;
    }
    /**
     * set the IEEE1609.2 header content (for encoding)
     *
     * @param[in] dot2 reference to the IEEE1609.2 data.
     * @returns none.
     */
    inline void Ieee1609Dot2(Ieee1609Dot2Data & dot2) {
        Ieee1609Dot2_ = dot2;
    }

    friend class SplitsCodec;

    char *buf;    /**< points to valid payload after encoding/sign */
    char *head;   /**< raw data buffer */
    int bufLen;   /**< length of the raw data buffer */

private:
    //Security
    Ieee1609Dot2Data Ieee1609Dot2_; /**< IEEE1609.2 header content */
};
} //namespace SPLITS

#endif
