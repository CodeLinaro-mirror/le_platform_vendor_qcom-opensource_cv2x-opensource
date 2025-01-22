/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_SERVICE_DATAPUBLISHER_HPP__
#define __SPLITS_SERVICE_DATAPUBLISHER_HPP__

#include "SplitsDataPublisher.hpp"

//Forward declaration
namespace SPLITS{
    namespace Core{
         class SplitsCore;
    }
}

namespace SPLITS {
namespace Interface {

class SplitsServiceDataPublisher : public SplitsDataPublisher
{
public:
    SplitsServiceDataPublisher() {}
    virtual ~SplitsServiceDataPublisher() {}
    bool init(std::shared_ptr<SPLITS::Core::SplitsCore> splitsCore, std::string topicPrefix) {
        if (splitsCore == nullptr) {
            return false;
        }
        splitsCore_ = splitsCore;
        return SplitsDataPublisher::init(topicPrefix);
    }
    /**
     * dequeue msg from message queue and publish it via fast-dds interface.
     * @param[in] none
     * @return none
     */
    void publish(void) override;

    /**
     * publish a message via fast-dds interface.
     *
     * @param[in] msg message to be published.
     * @return none.
     */
    void publish(std::shared_ptr<v2xMsg> msg);
private:

    std::shared_ptr<SPLITS::Core::SplitsCore> splitsCore_ = nullptr;

};

} //namespace Interface
} //namespace SPLITS

#endif //__SPLITS_SERVICE_DATAPUBLISHER_HPP__
