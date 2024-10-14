/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _CV2XDATASUBSCRIBER_HPP__
#define _CV2XDATASUBSCRIBER_HPP__

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include "Cv2xDataCommon.h"

class Cv2xDataSubscriber : public eprosima::fastdds::dds::DataReaderListener
{
public:

    Cv2xDataSubscriber();

    virtual ~Cv2xDataSubscriber();

    bool init(uint32_t rxSubId) {
        rxSubId_ = rxSubId;
        return init();
    };
    bool init(std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow ) {
        txFlow_ = txFlow;
        return init();
    }
    bool init();

    int getClientSock(void) { return udSockets[UD_SOCK_CLIENT_ID];};

    //void run();
    void on_data_available(
            eprosima::fastdds::dds::DataReader* reader) override;

    void on_subscription_matched(
            eprosima::fastdds::dds::DataReader* reader,
            const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

private:

    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataReader* reader_;
    eprosima::fastdds::dds::TypeSupport type_;

    int currentMatchedCount = 0;
    uint32_t rxSubId_ = 0;
    std::shared_ptr<telux::cv2x::ICv2xTxFlow> txFlow_ = nullptr;
    int udSockets[2];
};

#endif // _CV2XDATASUBSCRIBER_HPP__
