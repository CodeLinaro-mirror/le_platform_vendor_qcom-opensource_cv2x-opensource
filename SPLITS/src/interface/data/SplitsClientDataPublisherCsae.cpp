/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsClientDataPublisherCsae.hpp"
#include "Logger.hpp"
#include "v2xMsgCsae.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

bool SplitsClientDataPublisherCsae::init(std::string topicPrefix) {
    LOGD(__FUNCTION__);

    for (int i = 0; i < MSG_POOL_SIZE; i++) {
        auto msg = std::make_shared<v2xMsgCsae>();
        //null the unused buffers
        msg->head = msg->buf = 0;
        putFreeMsg(msg);
    }
    return SplitsClientDataPublisher::init(topicPrefix);
}

void SplitsClientDataPublisherCsae::publish(void) {
    while(!stop_) {
        std::unique_lock<std::mutex> lk(queueMtx_);
        cv_.wait(lk);

        //dequeue msg from queue, and see if there are any DDS subscriber.
        auto msg = dequeueMsg();
        if (msg == nullptr) {
            continue;
        }

        LOGI(__FUNCTION__, " got msg ");
        for (const auto listener : pubListeners_) {
            if (listener->firstConnected_ || listener->matched_ > 0) {
                LOGD(__FUNCTION__, "topic id: ", listener->topicId_);
                if (listener->topicId_ == static_cast<int>(msg->msgType_)) {
                    auto csaeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgCsae>(msg);
                    LOGI(__FUNCTION__,  " type = ", static_cast<int>(msg->msgType_));
                    switch(msg->msgType_) {
                        case SPLITS::v2xMsgType::V2X_CSAE_MSG_BSM:
                            listener->writer_->write(csaeMsg->BSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_CSAE_MSG_SPAT:
                            listener->writer_->write(csaeMsg->SPAT().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_CSAE_MSG_MAP:
                            listener->writer_->write(csaeMsg->Map().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_CSAE_MSG_RSM:
                            listener->writer_->write(csaeMsg->RSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_CSAE_MSG_RSI:
                            listener->writer_->write(csaeMsg->RSI().get());
                            break;
                        default:
                            LOGE(__FUNCTION__, " Unknown c-sae msg type");
                            break;
                    }
                }
            }
        }
        //put the msg back to pool
        putFreeMsg(msg);
    } // !stop
}
} //namespace Interface
} //namespace SPLITS
