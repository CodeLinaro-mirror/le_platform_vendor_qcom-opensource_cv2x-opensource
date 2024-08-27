/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsDataPublisher.cpp
 * @description SPLITS DDS data publisher abstract class implemnentation
 */

#ifndef __SPLITSDATAPUBLISHER__
#define __SPLITSDATAPUBLISHER__
#include <map>
#include <thread>
#include "SplitsDataPubSub.hpp"

namespace SPLITS {

namespace Interface {
class SplitsDataPublisher : public SplitsDataPubSub
{
public:

    SplitsDataPublisher(){};

    virtual ~SplitsDataPublisher();

    virtual bool init(std::string topicPrefix) override;

    virtual void run(void);
    virtual void stop(void);

protected:
    virtual void publish(void) = 0;
    bool stop_;
    std::thread td_;

private:

    eprosima::fastdds::dds::Publisher* publisher_;
    std::vector<eprosima::fastdds::dds::DataWriter*> writers_;

};
} //namespace Interface
} //namespace SPLITS
#endif  //__SPLITSDATAPUBLISHER__
