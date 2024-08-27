/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __TX_SCHEDULER_HPP__
#define __TX_SCHEDULER_HPP__
#include <thread>
#include <map>
#include <telux/common/CommonDefines.hpp>
#include "CongestCtrl.hpp"

using telux::common::Status;

namespace SPLITS {

namespace Core {

class SplitsCore;

class TxScheduler {
public:
    TxScheduler(std::shared_ptr<SplitsCore> core,
            std::shared_ptr<SPLITS::Core::CongestCtrl> congestCtrl = nullptr) {
        splitsCore_ = core;
        congestCtrl_ = congestCtrl;
    }
    virtual ~TxScheduler(){}
    /*
     * schecule periodic transmission that use SPS flow.
     *
     * @param[in] id message id , that core use to identify which message to send, e.g. BSM, SPAT..
     * @param[in] interval_ms periodicity of the message in mili-second.
     * @return SUCCESS/FAILED
     */
    virtual Status scheduleSpsTransmit(int id, int interval_ms);
    /*
     * Schecule transmission that uses EVENT flow.
     *
     * @param[in] id message id that the core uses to identify which message to send.
     * @param[in] interval_ms message periodicity, if value is 0, its one time transmission.
     * @return Status::SUCCESS or Status::FAILED
     */
    virtual Status scheduleEventTransmit(int id, int interval_ms = 0);

    /*
     * start the tx scheduler
     * @param[in] none
     * @return SUCCESS or FAILED
     */
    Status start(void);

    /*
     * stop the tx scheduler
     * 
     * @param[in] none
     * @return none
     */
    void stop(void);

private:
    Status calculateTimerPeriod(int id, int interval_ms,
            std::map<int, std::vector<int>> &intervalMap);
    void run(void);
    void runCongestCtrl(void);
    std::thread txThread_;
    std::thread txCongestCtrlThread_;
    bool stop_ = false;
    //Key is Tx interval, value is list of messages to send
    std::map<int, std::vector<int>> spsIntervalsMap_;
    std::map<int, std::vector<int>> eventIntervalsMap_;
    std::shared_ptr<SPLITS::Core::CongestCtrl> congestCtrl_;
    std::shared_ptr<SPLITS::Core::SplitsCore> splitsCore_;
    int timerPeriod_ = 100;
    int timerCount_ = 0;
    int timerfd_;
    int timerMisses_ = 0;
    int resetCount_ = 0;
};
}  //namespace Core
}  //namespace SPLITS

#endif //__TX_SCHEDULER_HPP__
