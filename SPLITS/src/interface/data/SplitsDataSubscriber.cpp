/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SplitsDataSubscriber.hpp"
#include "SplitsDataSubscriberSae.hpp"
#include "SplitsDataSubscriberCsae.hpp"
#include "Logger.hpp"
#include <typeinfo>

using namespace eprosima::fastdds::dds;

namespace SPLITS {
namespace Interface {

SplitsDataSubscriber::~SplitsDataSubscriber() {
    LOGD(__FUNCTION__);

    for (auto reader : readers_) {
        if (reader != nullptr)
        {
            if (subscriber_ != nullptr) {
                subscriber_->delete_datareader(reader);
            }
        }
    }

    if (subscriber_ != nullptr) {
        participant_->delete_subscriber(subscriber_);
    }
}

bool SplitsDataSubscriber::init(std::string topicPrefix) {
    LOGD(__FUNCTION__);

    //call baseclass init function to create participant, topics and register types
    return SplitsDataPubSub::init(topicPrefix);
}

int SplitsDataSubscriber::registerListener(std::weak_ptr<SplitsDataListener> listener) {
    auto sp = listener.lock();
    if (sp == nullptr) {
        LOGE(__FUNCTION__, " SplitsDataSubscriber::registerListener: invalid param ");
        return -1;
    }
    std::lock_guard<std::mutex> lk(listenerMtx_);
    for (auto item : listeners_) {
        if (item.lock() == sp) {
            return 0;
        }
    }
    listeners_.push_back(listener);
    return 0;
}

int SplitsDataSubscriber::deregisterListener(std::weak_ptr<SplitsDataListener> listener) {
    std::lock_guard<std::mutex> lk(listenerMtx_);
    bool listenerExisted = false;

    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (!sp) {
            it = listeners_.erase(it);
        } else if (sp == listener.lock()) {
            it = listeners_.erase(it);
            listenerExisted = true;
        } else {
            ++it;
        }
    }
    if (listenerExisted) {
        return 0;
    } else  {
        LOGD(__FUNCTION__, " no such listener existed");
        return -1;
    }
}

void SplitsSubListener::on_subscription_matched(DataReader*,
                                                const SubscriptionMatchedStatus& info) {
    if (info.current_count_change == 1) {
        currentMatchedCount_ = info.total_count;
        LOGE(__FUNCTION__, " Subscriber matched.");
    }
    else if (info.current_count_change == -1) {
        currentMatchedCount_ = info.total_count;
        LOGE(__FUNCTION__, " Subscriber unmatched.");
    } else {
        LOGE(__FUNCTION__, info.current_count_change,
             " is not a valid value for SubscriptionMatchedStatus current count change");
    }
}
} //namespace Interface
} //namespace SPLITS
