/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsCodecFactory.hpp
 *
 * @brief SplitsCodecFactory is the factory that create the codec object.
 */
#include "SplitsCodec.hpp"

#ifndef _SPLITSCODEC_FACTORY_HPP_
#define _SPLITSCODEC_FACTORY_HPP_
namespace SPLITS {

namespace Codec {

/**
 * @brief SplitsCodecFactory is the factory interface that creates the codec instance.
 */
class SplitsCodecFactory {
public:
    SplitsCodecFactory();
    virtual ~SplitsCodecFactory();

    /**
     * Get the SplitsCodecFactory singleton
     *
     * @returns reference to the SplitsCodecFactory singleton
     */
    static SplitsCodecFactory & getInstance();

    /**
     * Get SplitsCodec instance
     *
     * @param[n] st type of the stack.
     * @returns shared poitner to SplitsCodec object on success or nullptr otherwise.
     */
    virtual std::shared_ptr<SplitsCodec> getCodec(StackType st);

};

} //namespace Codec
} //namespace SPLITS
#endif
