/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SPLITS_CODEC_IMPL_HPP_
#define _SPLITS_CODEC_IMPL_HPP_

#include <memory>

#include "SplitsCodec.hpp"
#include "splits_codec/SplitsCodecWrapper.hpp"

namespace SPLITS {

namespace Codec {

class SplitsCodecImpl : public SplitsCodec {
public:
    SplitsCodecImpl() {};
    virtual ~SplitsCodecImpl() = 0;

    int init(const TxRxInfo &info, int logLevel, bool syslog) override;
    int decodeFrame(std::shared_ptr<v2xMsg> msg, bool secured, uint32_t psid,
        uint8_t* inPayload, uint32_t inLen) override { return 0; }
    int encodeFrame(std::shared_ptr<v2xMsg> msg) override { return 0; }
    int encodeHeader(std::shared_ptr<v2xMsg> msg, bool secured) override { return 0; }
    int decodeHeader(std::shared_ptr<v2xMsg> msg) override { return 0; }

protected:
    int decodeIeee1609Dot2Unsecured(Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen);
    int encodeIeee1609Dot2Unsecured(const Ieee1609Dot2Data& dot2Data, uint8_t* & buf, uint32_t *bufLen);

    std::shared_ptr<SPLITS::Codec::SplitsCodecWrapper> codecWrapper_ = nullptr;

private:
    void getCodecCfg(const TxRxInfo &info, CodecConfig &codecCfg);
};
} //namespace Codec
} //namespace SPLITS
#endif //_SPLITS_CODEC_IMPL_HPP_
