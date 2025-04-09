/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <array>
#include <vector>
#include <iostream>
#include <memory>
#include <cstring>
#include "SplitsCodecImpl.hpp"
#include "utils.h"
#include "Logger.hpp"


namespace SPLITS {

namespace Codec {

SplitsCodecImpl::~SplitsCodecImpl() {
    LOGD(__FUNCTION__);

    if (codecWrapper_) {
        codecWrapper_->deinit();
    }
}

void SplitsCodecImpl::getCodecCfg(const TxRxInfo &info, CodecConfig &codecCfg) {
    // initialize
    codecCfg.ctxtNum = 0;
    codecCfg.dftDecCtxt = 0;
    codecCfg.dftEncCtxt = 0;

    // add a ctxt for decoding, assuming all decoding is done in a single thread
    // Note: if each SID has a Rx thread, need to add a ctxt for each Rx SID and
    // specify the ctxt ID for each Rx SID in codecCfg.decCtxts
    if (not info.rxSids.empty()) {
        codecCfg.dftDecCtxt = codecCfg.ctxtNum;
        ++codecCfg.ctxtNum;
    }

    if (not info.localTxSids.empty() or not info.remoteTxSids.empty()) {
        // add a default Tx ctxt
        codecCfg.dftEncCtxt = codecCfg.ctxtNum;
        ++codecCfg.ctxtNum;

        // local Tx and remote Tx are done in different threads, add another ctxt
        // for remote Tx and specify SIDs in codecCfg.encCtxts
        if (not info.localTxSids.empty() and not info.remoteTxSids.empty()) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(info.remoteTxSids.size()); ++i) {
                codecCfg.encCtxts[info.remoteTxSids[i]] = codecCfg.ctxtNum;
            }
            ++codecCfg.ctxtNum;
        }
    }

    LOGD(__FUNCTION__, " num:", +codecCfg.ctxtNum, ", dec:", +codecCfg.dftDecCtxt,
         ", enc:", +codecCfg.dftEncCtxt, ", extEnc:", codecCfg.encCtxts.size(),
         ", extDec:", codecCfg.encCtxts.size());
}

int SplitsCodecImpl::init(const TxRxInfo &info, int logLevel, bool syslog) {
    LOGD(__FUNCTION__);

    if (!codecWrapper_) {
        LOGE(__FUNCTION__, " invalid codec");
        return -1;
    }

    CodecConfig cfg;
    getCodecCfg(info, cfg);
    auto ret = codecWrapper_->init(cfg, logLevel, syslog);
    if (ret < 0) {
        LOGE(__FUNCTION__, " codec init failed:", ret);
    }

    return ret;
}

int SplitsCodecImpl::decodeIeee1609Dot2Unsecured(Ieee1609Dot2Data& dot2Data,
    uint8_t* & buf, uint32_t *bufLen) {
    int payloadLen = 0;
    if (buf == 0 || bufLen == 0 || *bufLen == 0) {
        return -1;
    }

    dot2Data.protocolVersion = *buf;
    dot2Data.payloadLen = 0;
    buf++;
    (*bufLen)--;
    dot2Data.tagclass = static_cast<Ieee1609Dot2TagClass>((*buf >> 6) & 0x03);
    dot2Data.content = static_cast<Ieee1609Dot2Content>(*buf & 0x3F);
    buf++;
    (*bufLen)--;

    //CER encoded length field
    if (*buf & 0x80) {
        int numoctets = *buf & 0x7F;
        if (numoctets > 4) {
            LOGE(__FUNCTION__, "length is ", numoctets, "octets, too big!");
            return -1;
        }
        buf++;
        (*bufLen)--;
        while(numoctets > 0) {
            dot2Data.payloadLen = (dot2Data.payloadLen << 8) | *buf;
            buf++;
            (*bufLen)--;
            numoctets--;
        }
    } else {
        dot2Data.payloadLen = *buf;
        buf ++;
        (*bufLen)--;
    }
    return 0;
}
int SplitsCodecImpl::encodeIeee1609Dot2Unsecured(const Ieee1609Dot2Data& dot2Data,
    uint8_t* & buf, uint32_t *bufLen) {
    buf--;
    if (dot2Data.payloadLen > 127) {
        //only support 2 octets for simplicity
        if (dot2Data.payloadLen > 32767) {
            return -1;
        }
        *buf = dot2Data.payloadLen & 0xFF;
        buf --;
        (*bufLen)++;
        *buf = dot2Data.payloadLen >> 8 & 0xFF;
        buf --;
        (*bufLen)++;
        *buf = 0x80 | 0x2;
        buf --;
        (*bufLen)++;
    } else {
        *buf = dot2Data.payloadLen;
        (*bufLen)++;
        buf --;
    }
    *buf = static_cast<int>(dot2Data.content) | (static_cast<int>(dot2Data.tagclass) << 6);
    buf--;
    (*bufLen)++;
    *buf = 3;  //protocol version.
    (*bufLen)++;

    return 0;
}
};
};
