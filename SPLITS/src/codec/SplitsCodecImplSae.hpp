/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SPLITS_CODEC_IMPL_SAE_HPP_
#define _SPLITS_CODEC_IMPL_SAE_HPP_

#include "v2xMsgSae.hpp"
#include "SplitsCodecImpl.hpp"
#include "splits_codec/SplitsCodecSae.hpp"

#include <memory>

namespace SPLITS {

namespace Codec {

class SplitsCodecImplSae : public SplitsCodecImpl {
public:
    SplitsCodecImplSae();

    int decodeFrame(std::shared_ptr<v2xMsg> msg, bool secured, uint32_t psid,
        uint8_t* inPayload, uint32_t inLen) override;
    int encodeFrame(std::shared_ptr<v2xMsg> msg) override;
    int encodeHeader(std::shared_ptr<v2xMsg> msg, bool secured) override;
    int decodeHeader(std::shared_ptr<v2xMsg> msg) override;
private:
    void msgType2Psid(v2xMsgType type, uint32_t &psid);
};
} //namespace Codec
} //namespace SPLITS
#endif //_SPLITS_CODEC_IMPL_SAE_HPP_
