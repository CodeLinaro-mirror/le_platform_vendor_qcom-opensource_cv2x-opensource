/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SplitsCodec.hpp
 *
 * @brief abstract class to encapsulate v2x codec
 */
#ifndef SPLITS_CODEC_HPP_
#define SPLITS_CODEC_HPP_

#include <memory>
#include <vector>
#include "v2xMsg.hpp"


#define RESERVE_HEADER_LEN    (32)
namespace SPLITS {

namespace Codec {

struct TxRxInfo {
    std::vector<uint32_t> rxSids;  // Rx SIDs
    std::vector<uint32_t> localTxSids; // SIDs of local generated messages
    std::vector<uint32_t> remoteTxSids; // SIDs of remote generated messages
};

/**
 * This class encapsulate v2x message codec interface
 */
class SplitsCodec {
public:
    /**
     * Initialze the codec
     *
     * @returns 0 on success or negavie on failure.
     */
    virtual int init(const TxRxInfo &info, int logLevel, bool syslog) = 0;
    /**
     * Deinitialze the frames
     *
     * @returns 0 on success or negavie on failure.
     */
    //virtual int freeFrame(std::shared_ptr<v2xMsg> msg, bool encode) = 0;

    /**
     * destructor
     */
    virtual ~SplitsCodec(){};

    /**
     * decode a v2x message, the buffer to be decoded is stored in @ref v2xMsg raw buffer pointer
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int decodeFrame(std::shared_ptr<v2xMsg> msg, bool secure, uint32_t type, uint8_t* inPayload, uint32_t inLen)= 0;

    /**
     * encode a v2x message, the message to be decoded is stored in @ref v2xMsg frame pointer
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int encodeFrame(std::shared_ptr<v2xMsg> msg) = 0;
    /** encode header only
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int encodeHeader(std::shared_ptr<v2xMsg> msg, bool secured) = 0;
    /** decode header only
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int decodeHeader(std::shared_ptr<v2xMsg> msg) = 0;
protected:
    StackType stackType_;    /**< type of the stack, e.g. SAE or C-SAE */
};

} //namespace Codec
} //namespace Codec

#endif
