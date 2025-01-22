/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsRemoteRTCM.hpp
 *
 * @description This file demonstrate client side connects to NTRIP server(or read form RTCM data
 * file),and fill RTCM message defined via Fast-DDS IDL, and send the message to SPLITS service, 
 * the service program broadcast the J2375 RTCM message to OBU.
 */
#ifndef __SPLITS_REMOTE_RTCM_HPP__
#define __SPLITS_REMOTE_RTCM_HPP__

#include <memory>
#include <mutex>
#include "v2xMsgSae.hpp"
#include "SplitsNtrip.hpp"

using SPLITS::Core::SplitsNtrip;
using SPLITS::v2xMsgSae;

#define POOL_SIZE  3

class SplitsRemoteRTCM : public SplitsNtrip
{
public:
    SplitsRemoteRTCM(std::shared_ptr<SplitsConfig::RTCMConfig> config) :
        SplitsNtrip(config) {
            for (int i = 0; i < POOL_SIZE; i++) {
                rtcmMsgPool_.push(std::make_shared<J2735::idl::RTCMmessage>());
            }
    }
    virtual ~SplitsRemoteRTCM() {}
    void onMsgAvailable(uint8_t *buf, int bufLen) override;
    int fillRTCM(std::shared_ptr<v2xMsgSae> msg);

private:
    std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgQueue_;
    std::queue<std::shared_ptr<J2735::idl::RTCMmessage>> rtcmMsgPool_;
    std::mutex msgMtx_;

};

#endif
