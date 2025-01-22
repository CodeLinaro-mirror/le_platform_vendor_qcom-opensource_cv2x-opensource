/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SPLITS_CODEC_IMPL_HPP_
#define _SPLITS_CODEC_IMPL_HPP_

#include "SplitsCodec.hpp"
#include "splits_codec/SplitsSaeCodec.hpp"

#include <memory>

namespace SPLITS {

namespace Codec {

class SplitsCodecImpl : public SplitsCodec {
public:
    SplitsCodecImpl(StackType st);
    virtual ~SplitsCodecImpl();

    virtual int init(int logLevel, bool syslog) override;
    virtual int decodeFrame(std::shared_ptr<v2xMsg> msg) override;
    virtual int encodeFrame(std::shared_ptr<v2xMsg> msg) override;
    virtual int encodeWsmp(std::shared_ptr<v2xMsg> msg, bool secured) override;
private:

    void convertMsgType(v2xMsgType src, SaeMsgType &dest);
    int decodeIeee1609Dot2Unsecured(Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen);
    int encodeIeee1609Dot2Unsecured(const Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen);
    std::unique_ptr<SPLITS::Codec::SplitsSaeCodec> saeCodec_ = nullptr;
};
} //namespace Codec
} //namespace SPLITS
#endif //_SPLITS_CODEC_IMPL_HPP_
