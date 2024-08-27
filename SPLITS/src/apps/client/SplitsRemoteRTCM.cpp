/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsRemoteRTCM.hpp"
#include "utils.h"


void SplitsRemoteRTCM::onMsgAvailable(uint8_t *buf, int bufLen)
{
    std::lock_guard<std::mutex> lk(msgMtx_);
    if (rtcmMsgQueue_.size() == POOL_SIZE) {
        //queue is full, msgs are not broadcasted out, we drop the newly received msgs
        return;
    }
    auto msg = rtcmMsgPool_.front();
    rtcmMsgPool_.pop();
    msg->clear();
    std::cout << "copy data to queue: " << bufLen << std::endl;
    msg->assign(buf, buf + bufLen);
    rtcmMsgQueue_.push(msg);
}
int SplitsRemoteRTCM::fillRTCM(std::shared_ptr<v2xMsgSae> msg)
{
    std::shared_ptr<J2735::idl::RTCMcorrection> sp;
    if (msg == nullptr) {
        std::cerr << "fillRTCM: no msg to fill" << std::endl;
        return -1;
    }
    if ((sp = msg->RTCMcorrection()) == nullptr) {
        sp = std::make_shared<J2735::idl::RTCMcorrection>();
        if (sp == nullptr) {
            std::cerr << "no idl msg " << std::endl;
            return -1;
        }
        msg->RTCMcorrection(sp);
    }
    sp->rev(J2735::idl::rtcmv_rtcmRev3);
    sp->timeStamp(get_minute_of_the_year());
    {
        std::lock_guard<std::mutex> lk(msgMtx_);
        sp->msgCnt(rtcmMsgQueue_.size());
        J2735::idl::RTCMmessageList idlMsgList;
        while(rtcmMsgQueue_.empty() == false) {
            auto msg = rtcmMsgQueue_.front();
            rtcmMsgQueue_.pop();
            idlMsgList.push_back(*msg);
            std::cout << "msg size:" << msg->size() << std::endl;
            rtcmMsgPool_.push(msg);
        }
        sp->msgs(std::move(idlMsgList));
    }
    return 0;
}
