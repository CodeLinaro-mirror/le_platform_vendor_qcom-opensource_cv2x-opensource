/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "Cv2xDataPublisher.h"
#include "Cv2xDataPubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>

#include <thread>
#include <chrono>

using namespace eprosima::fastdds::dds;

Cv2xDataPublisher::Cv2xDataPublisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new Cv2xDataPubSubType())
{
}

Cv2xDataPublisher::~Cv2xDataPublisher()
{
    if (udSockets[UD_SOCK_CLIENT_ID] >= 0) {
        if (shutdown(udSockets[UD_SOCK_CLIENT_ID], SHUT_RD) == -1) {
            int temp_error = errno;
            std::cout << "Failed to shutdown read direction " << temp_error << std::endl;
        }
    }

    if (udSockets[UD_SOCK_SERVER_ID] >= 0) {
        close(udSockets[UD_SOCK_SERVER_ID]);
    }

    if (td.joinable()) {
        td.join();
    }

    if (writer_ != nullptr) {
        publisher_->delete_datawriter(writer_);
    }

    if (publisher_ != nullptr) {
        participant_->delete_publisher(publisher_);
    }

    if (topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}
bool Cv2xDataPublisher::init()
{
    /* Initialize data_ here */
    std::string topic("Cv2xData");

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.name("cv2x_pub");
    if (rxSub_ != nullptr) {
        //called from cv2x stub side
        topic = topic + "Rx" + std::to_string(rxSub_->getSubscriptionId());
    } else {
        //called from cv2x client side
        topic = topic + "Tx" + std::to_string(txFlowId_);
    }
    participant_ = DomainParticipantFactory::get_instance()->create_participant(
            CV2X_DOMAIN_ID, pqos);
    if (participant_ == nullptr) {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE PUBLISHER
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    if (publisher_ == nullptr) {
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

    // CREATE THE WRITER
    writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, this);
    if (writer_ == nullptr) {
        return false;
    }

    std::cout << "Cv2xData DataWriter created." << std::endl;

    if (not rxSub_) {
        //create Unix domain socket if it's called from client side
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, udSockets) < 0) {
            udSockets[0] = udSockets[1] = -1;
            std::cout << "Failed to create socket pair " << strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}

void Cv2xDataPublisher::on_publication_matched(
        eprosima::fastdds::dds::DataWriter*,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    auto FuncHandleRxDDS = [&](void) {
        this->handleRxDDS();
    };
    auto FuncHandleTxDDS = [&](void) {
        this->handleTxDDS();
    };

    if (info.current_count_change == 1) {
        currentMatchedCount = info.current_count;
        std::cout << "DataWriter matched." << std::endl;
        if (rxSub_) {
            td = std::thread(FuncHandleRxDDS);
        } else {
            td = std::thread(FuncHandleTxDDS);
        }
    } else if (info.current_count_change == -1) {
        currentMatchedCount = info.current_count;
        std::cout << "DataWriter unmatched." << std::endl;
    } else {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

//Handle Rx from socket and publish it, called from vsomeip stub side
void Cv2xDataPublisher::handleRxDDS()
{
    ssize_t rlen;
    struct sockaddr_in6 from;
    socklen_t fromLen = sizeof(from);
    char control[CMSG_SPACE(sizeof(int))];
    struct msghdr rxmsg = {0};
    struct iovec iov[1] = {0};
    int sock = rxSub_->getSock();
    uint8_t buf[CV2X_BUFFER_SIZE];
    uint8_t *p = &buf[0];

    if (sock < 0) {
        std::cout << "sock does not exist, the child thread exits" << std::endl;
        return;
    }

    iov[0].iov_base = buf;
    iov[0].iov_len = CV2X_BUFFER_SIZE;

    rxmsg.msg_name = &from;
    rxmsg.msg_namelen = fromLen;
    rxmsg.msg_iov = iov;
    rxmsg.msg_iovlen = 1;
    rxmsg.msg_control = control;
    rxmsg.msg_controllen = sizeof(control);
    std::cout << "Cv2xData DataWriter waiting for DataReaders." << std::endl;

    while (currentMatchedCount) {
        if ((rlen = recvmsg(sock, &rxmsg, 0)) <= 0) {
            std::cout << "Failed to rx message for rxsub id "
                      << rxSub_->getSubscriptionId() << std::endl;
            return;
        } else {
            Cv2xData data;
            std::array<uint8_t, 3> src_id{from.sin6_addr.s6_addr[13],
                    from.sin6_addr.s6_addr[14],
                    from.sin6_addr.s6_addr[15]};
            data.src_id(src_id);
            data.len(rlen);
            std::vector<uint8_t> msg(p, p + rlen);
            data.msg(std::move(msg));
            writer_->write(&data);
        }
    }
}

//handle tx, read packets from UDS socket and publish it, called from vsomeip client wrapper side
void Cv2xDataPublisher::handleTxDDS(void)
{
    ssize_t rlen;
    struct msghdr msg = {};
    struct iovec iov[1] = {0};
    uint8_t buf[CV2X_BUFFER_SIZE];
    uint8_t *p = &buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = CV2X_BUFFER_SIZE;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;

    while (currentMatchedCount) {
        //receive packets from UDS
        if ((rlen = recvmsg(udSockets[UD_SOCK_CLIENT_ID], &msg, 0)) <= 0) {
            close(udSockets[UD_SOCK_CLIENT_ID]);
            std::cout << "uds rx failed " << strerror(errno) << std::endl;
            return;
        }

        Cv2xData data;
        data.len(rlen);
        std::vector<uint8_t> msg(p, p + rlen);
        data.msg(std::move(msg));
        writer_->write(&data);
    }
}
