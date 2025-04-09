/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_SERVICE_DATAPUBLISHER_HPP__
#define __SPLITS_SERVICE_DATAPUBLISHER_HPP__

#include "SplitsCore.hpp"
#include "SplitsDataPublisher.hpp"
#include "v2xMsg.hpp"
#include "Logger.hpp"

//Forward declaration
namespace SPLITS {
namespace Core {
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
    void publish(void) override {
        LOGD(__FUNCTION__, " jie");
        while (!stop_) {
            //dequeue msg from rx queue, and see if there are any DDS subscriber.
            auto msg = splitsCore_->dequeueMsg();

            if (msg != nullptr) {
            LOGD(__FUNCTION__, " jie valid msg");

                publish(msg);
                //put the msg back to rx pool
                splitsCore_->putFreeMsg(msg);
            } else {
            LOGD(__FUNCTION__, " jie invalid msg");

            }
        }
    }

    /**
     * publish a message via fast-dds interface.
     *
     * @param[in] msg message to be published.
     * @return none.
     */
    virtual void publish(std::shared_ptr<v2xMsg> msg) = 0;

protected:
    std::shared_ptr<SPLITS::Core::SplitsCore> splitsCore_ = nullptr;
};

} //namespace Interface
} //namespace SPLITS

#endif //__SPLITS_SERVICE_DATAPUBLISHER_HPP__
