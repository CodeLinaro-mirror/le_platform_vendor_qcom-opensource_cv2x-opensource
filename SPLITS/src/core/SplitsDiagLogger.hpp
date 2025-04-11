/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SPLITS_DIAG_LOGGER_HPP_
#define SPLITS_DIAG_LOGGER_HPP_

#include <mutex>
#include <memory>

#include "qDiagLogPacketDef.h"
#include "qUtils.hpp"
#include "v2x_diag.h"
#include "utils.h"

namespace SPLITS {
namespace Core {

struct DiagLogData {
    bool validPkt;
    uint64_t currTime;
    uint64_t txInterval;
    bool enableCongCtrl;
    double distFromRV;
    uint32_t l2SrcAddr;
};

class CongestCtrl;
enum class TransmitType;

class SplitsDiagLogger {
public:
    SplitsDiagLogger(const std::shared_ptr<SPLITS::Core::CongestCtrl>& congestCtrl);
    ~SplitsDiagLogger();
    void sendGeneralTxInfo(const TransmitType &txType, const DiagLogData *logData,
        const v2x_diag_bsm_data *bsmInfo);
    void sendGeneralRxInfo(const DiagLogData *logData,
        const v2x_diag_bsm_data *bsmInfo);
    void sendGeneralPeriodicInfo();

private:
    v2x_diag_qits_general_data generalInfo_;
    std::mutex mtxGeneralInfo_;
    std::shared_ptr<QUtils> utils_;
    std::shared_ptr<SPLITS::Core::CongestCtrl> congestCtrl_ = nullptr;

    void fillGeneralInfo(const DiagLogData *logData, const v2x_diag_event_bit_t &eventsData);
};

} //namespace Core
} //namespace SPLITS

#endif //SPLITS_DIAG_LOGGER_HPP_