/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SplitsCore.hpp"
#include "v2xMsgSae.hpp"
#include "SplitsServiceDataPublisher.hpp"

namespace SPLITS {
namespace Interface {

using namespace eprosima::fastdds::dds;

void SplitsServiceDataPublisher::publish(std::shared_ptr<v2xMsg> msg)
{
    std::shared_ptr<SPLITS::v2xMsgSae> saeMsg = nullptr;

    for (const auto listener : pubListeners_) {
        if (listener->firstConnected_ || listener->matched_ > 0) {
            if (listener->topicId_ == static_cast<int>(msg->msgType_)) {
                if (!isCSAE(msg->msgType_)){
                    saeMsg = std::dynamic_pointer_cast<SPLITS::v2xMsgSae>(msg);
                    switch(msg->msgType_) {
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_BSM:
                            std::cout << "writting BSM" << std::endl;
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
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_SDSM:
                            listener->writer_->write(saeMsg->SDSM().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_SAE_MSG_WSA:
                            listener->writer_->write(saeMsg->SrvAdvMsg().get());
                            break;
                        case SPLITS::v2xMsgType::V2X_MSG_PDU:
                            listener->writer_->write(saeMsg->MsgData().get());
                            break;
                        default:
                            std::cerr << "unknown msg type" << std::endl;
                            break;
                    }
                } else {
                    //define v2xMsgCsae 
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
                            break;
                    }
                }
            }
        }
    }
}
void SplitsServiceDataPublisher::publish(void)
{
    while(!stop_) {
        //dequeue msg from rx queue, and see if there are any DDS subscriber.
        auto msg = splitsCore_->dequeueMsg();

        if (msg != nullptr) {
            publish(msg);
            //put the msg back to rx pool
            splitsCore_->putFreeMsg(msg);
        }
    }
}
} //namespace Interface
} //namespace SPLITS
