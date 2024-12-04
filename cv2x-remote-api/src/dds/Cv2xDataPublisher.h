/*
 * Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _CV2XDATAPUBLISHER_HPP__
#define _CV2XDATAPUBLISHER_HPP__

#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "Cv2xDataCommon.h"

class Cv2xDataPublisher : public eprosima::fastdds::dds::DataWriterListener,
    public std::enable_shared_from_this<Cv2xDataPublisher>
{
public:

    Cv2xDataPublisher();

    virtual ~Cv2xDataPublisher();

    bool init(std::shared_ptr<telux::cv2x::ICv2xRxSubscription> rxSub) {
        rxSub_ = rxSub;
        return init();
    };
    bool init(uint32_t txFlowId) {
        txFlowId_ = txFlowId;
        return init();
    }

    int getServerSock(void) { return udSockets[UD_SOCK_SERVER_ID];};
    void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* writer,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;
    void handleRxDDS(void);
    void handleTxDDS(void);

    int getSock(void){
        return udSockets[UD_SOCK_SERVER_ID];
    }

private:
    bool init();
    void shutdownUds();

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataWriter* writer_;
    eprosima::fastdds::dds::TypeSupport type_;
    std::shared_ptr<telux::cv2x::ICv2xRxSubscription>rxSub_ = nullptr;
    uint32_t txFlowId_;
    int udSockets[2] = {-1};
    std::mutex udsMutex_;
    std::condition_variable udsCv_;
    std::thread td;
    int32_t currentMatchedCount = 0;
};

#endif  //_CV2XDATAPUBLISHER_HPP__
