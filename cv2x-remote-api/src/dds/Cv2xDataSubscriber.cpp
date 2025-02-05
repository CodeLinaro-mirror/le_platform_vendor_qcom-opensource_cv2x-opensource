/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

#include "Cv2xDataSubscriber.h"
#include "Cv2xDataPubSubTypes.h"

using namespace eprosima::fastdds::dds;

Cv2xDataSubscriber::Cv2xDataSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new Cv2xDataPubSubType())
{
}

Cv2xDataSubscriber::~Cv2xDataSubscriber()
{
    if (udSockets[UD_SOCK_SERVER_ID] >= 0) {
        close(udSockets[UD_SOCK_SERVER_ID]);
    }

    if (udSockets[UD_SOCK_CLIENT_ID] >= 0) {
        close(udSockets[UD_SOCK_CLIENT_ID]);
    }

    if (reader_ != nullptr) {
        subscriber_->delete_datareader(reader_);
    }

    if (topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }

    if (subscriber_ != nullptr) {
        participant_->delete_subscriber(subscriber_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool Cv2xDataSubscriber::init()
{
    std::string topic("Cv2xData");
    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.name("cv2x_sub");

    if (txFlow_ != nullptr) {
        //called from cv2x stub side 
        topic = topic + "Tx" + std::to_string(txFlow_->getFlowId());
    } else {
        //called from cv2x client side
        topic = topic + "Rx" + std::to_string(rxSubId_);
    }
    participant_ = DomainParticipantFactory::get_instance()->create_participant(
            CV2X_DOMAIN_ID, pqos);

    if (participant_ == nullptr) {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr) {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic(
        topic,
        type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        return false;
    }

    //CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    reader_ = subscriber_->create_datareader(topic_, rqos, this);
    if (reader_ == nullptr) {
        return false;
    }

    if (not txFlow_) {
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, udSockets) < 0) {
            udSockets[0] = udSockets[1] = -1;
            std::cout << "Failed to create socket pair " << strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}

void Cv2xDataSubscriber::on_subscription_matched(
        DataReader*,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1) {
        currentMatchedCount = info.total_count;
        std::cout << "Subscriber matched." << std::endl;
    } else if (info.current_count_change == -1) {
        currentMatchedCount = info.total_count;
        std::cout << "Subscriber unmatched." << std::endl;
    } else {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void Cv2xDataSubscriber::on_data_available(
        DataReader* reader)
{
    Cv2xData st;
    SampleInfo info;

    struct msghdr txmsg = {0};
    struct iovec iov[1] = {0};
    txmsg.msg_iov = iov;
    txmsg.msg_iovlen = 1;
    txmsg.msg_control = 0;
    txmsg.msg_controllen = 0;

    if (reader->take_next_sample(&st, &info) == ReturnCode_t::RETCODE_OK ) {
        if (info.valid_data) {
            if (not txFlow_) {
                //called from client proxy, send data to UDS
                iov[0].iov_base = st.msg().data();
                iov[0].iov_len = st.len();
                if (sendmsg(udSockets[UD_SOCK_SERVER_ID], &txmsg, 0) < 0) {
                    std::cout << "Send to uds failed " << strerror(errno) << std::endl;
                }
            } else {
                //called from server stub, send data to cv2x sock.
                iov[0].iov_base = st.msg().data();
                iov[0].iov_len = st.len();
                if (sendmsg(txFlow_->getSock(), &txmsg, 0) < 0) {
                    std::cout << "Send to cv2x sock failed " << strerror(errno) << std::endl;
                }
            }
        }
    } else {
        std::cout << "faild to read DDS data" << std::endl;
    }
}
