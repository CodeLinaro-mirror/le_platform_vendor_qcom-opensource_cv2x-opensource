
/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SplitsCodec.hpp
 * 
 * @brief abstract class to encapsulate v2x codec 
 */
#ifndef SPLITS_CODEC_HPP_
#define SPLITS_CODEC_HPP_

#include <memory>
#include "v2xMsg.hpp"


#define RESERVE_HEADER_LEN    (32)
namespace SPLITS {

namespace Codec {

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
    virtual int init(int logLevel, bool syslog) = 0;

    /**
     * destructor
     */
    virtual ~SplitsCodec(){};

    /**
     * decode a v2x message, the buffer to be decoded is stored in @ref v2xMsg raw buffer pointer
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int decodeFrame(std::shared_ptr<v2xMsg> msg) = 0;

    /**
     * encode a v2x message, the message to be decoded is stored in @ref v2xMsg frame pointer
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int encodeFrame(std::shared_ptr<v2xMsg> msg) = 0;
    /** encode WSMP header only
     *
     * @returns 0 on success or negative on failure.
     */
    virtual int encodeWsmp(std::shared_ptr<v2xMsg> msg, bool secured) = 0;

protected:
    StackType stackType_;    /**< type of the stack, e.g. SAE or C-SAE */
};

} //namespace Codec
} //namespace Codec

#endif
