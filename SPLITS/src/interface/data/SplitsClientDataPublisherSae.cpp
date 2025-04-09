/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsClientDataPublisherSae.hpp"
#include "Logger.hpp"
#include "v2xMsgSae.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

bool SplitsClientDataPublisherSae::init(std::string topicPrefix) {
    LOGD(__FUNCTION__);

    for (int i = 0; i < MSG_POOL_SIZE; i++) {
        auto msg = std::make_shared<v2xMsgSae>();
        //null the unused buffers
        msg->head = msg->buf = 0;
        putFreeMsg(msg);
    }
    return SplitsClientDataPublisher::init(topicPrefix);
}

void SplitsClientDataPublisherSae::publish(void) {
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
                    auto saeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
                    LOGI(__FUNCTION__,  " type = ", static_cast<int>(msg->msgType_));
                    switch(msg->msgType_) {
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_BSM:
                            listener->writer_->write(saeMsg->BSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_SPAT:
                            listener->writer_->write(saeMsg->SPAT().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_MAP:
                            listener->writer_->write(saeMsg->Map().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM:
                            listener->writer_->write(saeMsg->RTCMcorrection().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_RSM:
                            listener->writer_->write(saeMsg->RSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_SRM:
                            listener->writer_->write(saeMsg->SRM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_SSM:
                            listener->writer_->write(saeMsg->SSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM:
                            listener->writer_->write(saeMsg->SDSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_WSA:
                            listener->writer_->write(saeMsg->SrvAdvMsg().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_MSG_PDU: {
                            LOGD(__FUNCTION__, " write pdu");
                            auto msgData = saeMsg->MsgData();
                            LOGD(__FUNCTION__, " sid = ", msgData->serviceId(),
                                 ", len = ", msgData->length());
                            listener->writer_->write(saeMsg->MsgData().get());
                            }
                            break;
                        default:
                            LOGE(__FUNCTION__, " Unknown sae msg type");
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
