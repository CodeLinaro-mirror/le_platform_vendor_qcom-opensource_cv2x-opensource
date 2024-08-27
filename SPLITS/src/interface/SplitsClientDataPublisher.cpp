/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsClientDataPublisher.hpp"

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

void SplitsClientDataPublisher::publish(void)
{
    std::shared_ptr<SPLITS::v2xMsgSae> saeMsg = nullptr;
    while(!stop_) {
        std::unique_lock<std::mutex> lk(queueMtx_);
        cv_.wait(lk);

        //dequeue msg from queue, and see if there are any DDS subscriber.
        auto msg = dequeueMsg();
        if (msg == nullptr) {
            continue;
        }

        std::cout << "got msg " << std::endl;
        for (const auto listener : pubListeners_) {
            if (listener->firstConnected_ || listener->matched_ > 0)
            {
                std::cout << "topic id: " << listener->topicId_ << std::endl;
                if (listener->topicId_ == static_cast<int>(msg->msgType_)) {
                    if (!isCSAE(msg->msgType_)){
                        saeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
                        std::cout << "type=" << static_cast<int>(msg->msgType_) << std::endl;
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
                                std::cout << "Send rtcm" << std::endl;
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
                            case SPLITS::v2xMsgType::V2X_MSG_PDU:
                                {
                                std::cout << "write pdu" << std::endl;
                                auto msgData = saeMsg->MsgData();
                                std::cout << "sid=" << msgData->serviceId() << "len=" << msgData->length() << std::endl;
                                listener->writer_->write(saeMsg->MsgData().get());
                                }
                                break;
                            default:
                                std::cerr << "Unknown sae msg type" << std::endl;
                                break;
                        }
                    } else {
                        switch(msg->msgType_) {
                            case SPLITS::v2xMsgType::V2X_CSAE_MSG_BSM:
                                break;
                            case SPLITS::v2xMsgType::V2X_CSAE_MSG_SPAT:
                                break;
                            case SPLITS::v2xMsgType::V2X_CSAE_MSG_MAP:
                                break;
                            case SPLITS::v2xMsgType::V2X_CSAE_MSG_RSM:
                                break;
                            default:
                                std::cerr << "Unknown c-sae msg type" << std::endl;
                                break;
                        }
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
