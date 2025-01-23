/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/time.h>
#include <sys/timerfd.h>
#include "TxScheduler.hpp"
#include "SplitsCore.hpp"

#include "Logger.hpp"

namespace SPLITS {
namespace Core {

Status TxScheduler::scheduleSpsTransmit(int id, int interval_ms) {
    if (congestCtrl_ != nullptr) {
        //when congestion control enabled, tx scheduler for SPS depends on congestion control
        //library's timer value, which is 100ms.
        if (interval_ms != 100) {
            LOGE(__FUNCTION__, " Can only schedule 100ms interval with congestion control enabled");
            return Status::INVALIDPARAM;
        }
    }
    return calculateTimerPeriod(id, interval_ms, spsIntervalsMap_);
}

Status TxScheduler::scheduleEventTransmit(int id, int interval_ms) {
    Status ret = Status::SUCCESS;
    if (interval_ms > 0) {
        //schedule periodic event msg
        ret = calculateTimerPeriod(id, interval_ms, eventIntervalsMap_);
    } else {
        //one time event msg, send immediately
        splitsCore_->send(id, nullptr);
    }
    return ret;
}

Status TxScheduler::calculateTimerPeriod(int id, int interval_ms,
                                         std::map<int, std::vector<int>> &intervalsMap) {
    //interval should be in SPS values range: 20, 50, 100...
    if (interval_ms < 20) {
        return Status::INVALIDPARAM;
    }
    auto msgList = intervalsMap.find(interval_ms);
    if (msgList != intervalsMap.end()) {
        msgList->second.push_back(id);
    } else {
        std::vector<int> msgList;
        msgList.push_back(id);
        intervalsMap.emplace(interval_ms, std::move(msgList));
    }
    //set the timer value to be least common denominator of all the intervals.
    auto min = intervalsMap.begin()->first;
    auto max = intervalsMap.rbegin()->first;
    if (min == 20) {
        timerPeriod_ = 10;
    } else if (min == 50 ) {
        timerPeriod_ = 50;
    } else {
        timerPeriod_ = 100;
    }
    resetCount_ = max / timerPeriod_;
    LOGD(__FUNCTION__, " timerPeriod = ", timerPeriod_, " resetCount = ", resetCount_);
    return Status::SUCCESS;
}

Status TxScheduler::start(void) {
    LOGD(__FUNCTION__);
    struct itimerspec its = {0};

    timerfd_ = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd_ < 0) {
        return Status::FAILED;
    }
    its.it_value.tv_sec = timerPeriod_ / 1000;
    its.it_value.tv_nsec = (timerPeriod_%1000) * 1000000;
    its.it_interval = its.it_value;

    if (timerfd_settime(timerfd_, 0, &its, NULL) < 0) {
        close(timerfd_);
        return Status::FAILED;
    }
    txThread_ = std::thread(&TxScheduler::run, this);
    if (congestCtrl_ != nullptr) {
        txCongestCtrlThread_ = std::thread(&TxScheduler::runCongestCtrl, this);
    }

    return Status::SUCCESS;
}

void TxScheduler::stop(void) {
    LOGD(__FUNCTION__);
    stop_ = true;
    txThread_.join();
    if (congestCtrl_ != nullptr) {
        txCongestCtrlThread_.join();
    }
}

void TxScheduler::runCongestCtrl(void) {
    LOGD(__FUNCTION__);
    while (congestCtrl_->getCurrCongestionCtrlMgr() == nullptr) {
    }
    while (stop_ == false) {
       sem_wait(congestCtrl_->
                getCurrCongestionCtrlMgr()->
                getCongestionControlUserData()->congestionControlSem);
       //only support 100ms sps msgs
       auto msgList = spsIntervalsMap_.find(100);
       if (msgList != spsIntervalsMap_.end()) {
           for (auto msgId : msgList->second) {
               splitsCore_->send(msgId, nullptr);
           }
       }
    }
}

void TxScheduler::run(void) {
    LOGD(__FUNCTION__);
    uint64_t exp;
    int ret;
    while (stop_ == false) {
        ret = read(timerfd_, &exp, sizeof(exp));
        if (ret == sizeof(uint64_t) && exp > 1) {
            timerMisses_ += 1;
        } else {
            timerCount_ += 1;
        }
        if (congestCtrl_ == nullptr) {
            //use timer for SPS transmit if congestion control not enabled, 
            for (auto const & spsInt : spsIntervalsMap_) {
                if (!((timerCount_ * timerPeriod_) % spsInt.first)) {
                    for (auto const & msgId : spsInt.second) {
                        splitsCore_->send(msgId, nullptr);
                    }
                }
            }
        }
        //always use timer for periodic event transmit, regardless of congestion control enabled
        //or not.
        for (auto const & eventInt : eventIntervalsMap_) {
            if (!((timerCount_ * timerPeriod_) % eventInt.first)) {
                for (auto const & msgId : eventInt.second) {
                    splitsCore_->send(msgId, nullptr);
                }
            }
        }
        if (timerCount_ == resetCount_) {
            timerCount_ = 0;
        }
    }
}

} //namespace Core
} //namespace SPLITS
