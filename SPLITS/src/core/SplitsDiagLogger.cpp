/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Logger.hpp"
#include "SplitsDiagLogger.hpp"
#include "CongestCtrl.hpp"
#include "SplitsCore.hpp"

namespace SPLITS {

namespace Core {

SplitsDiagLogger::SplitsDiagLogger(const std::shared_ptr<SPLITS::Core::CongestCtrl>& congestCtrl) {
    if (!utils_) {
        utils_ = std::make_shared<QUtils>();
    }
    utils_->initDiagLog();
    generalInfo_ = {0};
    congestCtrl_ = congestCtrl;
}

SplitsDiagLogger::~SplitsDiagLogger() {
    utils_->deInitDiagLog();
}

void SplitsDiagLogger::fillGeneralInfo(const DiagLogData *logData,
    const v2x_diag_event_bit_t &eventsData) {
    std::lock_guard<std::mutex> lock(mtxGeneralInfo_);
    generalInfo_.time_stamp_log = timestamp_now();
    generalInfo_.time_stamp_msg = logData->currTime;
    generalInfo_.gnss_time = 0;
    generalInfo_.CPU_Util = 0;
    generalInfo_.GPS_mode = 0;
    generalInfo_.msg_valid = logData->validPkt;
    generalInfo_.events = eventsData;
    generalInfo_.hysterisis = 5;
    generalInfo_.L2_ID = logData->l2SrcAddr;
}

void SplitsDiagLogger::sendGeneralTxInfo(const TransmitType &txType, const DiagLogData *logData,
    const v2x_diag_bsm_data *bsmInfo) {
    if (!logData) {
        LOGE(__FUNCTION__, " logData is null");
        return;
    }

    if (!congestCtrl_) {
        LOGE(__FUNCTION__, " CongestCtrl has not been initialized");
        return;
    }

    auto ccCalculations = congestCtrl_->getCongCtrlCalculationData();
    v2x_diag_event_bit_t eventsData = {0};

    // fill other general
    fillGeneralInfo(logData, eventsData);

    v2x_diag_transmit_type_et msg_type = txType == TransmitType::SPS ? DIAG_SPS : DIAG_EVENT;
    v2x_qits_general_tx_info info = {0};
    V2X_QITS_GENERAL_TX_PKG *msg = (V2X_QITS_GENERAL_TX_PKG *)((uint32_t *)(&info) + 1);

    {
        std::lock_guard<std::mutex> lock(mtxGeneralInfo_);
        generalInfo_.tracking_error = ccCalculations.trackingError ?
            (uint32_t)(ccCalculations.trackingError * 100) : 0;
        generalInfo_.vehicle_density_in_range = (uint32_t)(ccCalculations.smoothDens * 100.0);
        msg->channel_quality_indication = ccCalculations.channData ?
            (uint32_t)(ccCalculations.channData->channQualInd * 100.0) : 0;
        generalInfo_.max_ITT = ccCalculations.maxITT;
        msg->general_data = generalInfo_;
    }

    msg->bsm_data = *bsmInfo;
    msg->tx_interval = logData->txInterval;
    msg->DCC_random_time = (long unsigned int)0;
    msg->msg_type = msg_type;

    utils_->sendLogPacket(&info, PKT_ID_QITS_TX_FLOW);
}

void SplitsDiagLogger::sendGeneralRxInfo(const DiagLogData *logData,
    const v2x_diag_bsm_data *bsmInfo) {
    if (!logData) {
        LOGE(__FUNCTION__, " logData is null");
        return;
    }

    if (!congestCtrl_) {
        LOGE(__FUNCTION__, " CongestCtrl has not been initialized");
        return;
    }

    auto ccCalculations = congestCtrl_->getCongCtrlCalculationData();
    v2x_diag_event_bit_t eventsData = {0};

    // fill other general
    fillGeneralInfo(logData, eventsData);

    v2x_qits_general_rx_info info = {0};
    V2X_QITS_GENERAL_RX_PKG *msg = (V2X_QITS_GENERAL_RX_PKG *)((uint32_t *)(&info) + 1);

    {
        std::lock_guard<std::mutex> lock(mtxGeneralInfo_);
        generalInfo_.tracking_error = ccCalculations.trackingError ?
            (uint32_t)(ccCalculations.trackingError * 100) : 0;
        generalInfo_.vehicle_density_in_range = (uint32_t)(ccCalculations.smoothDens * 100.0);
        generalInfo_.max_ITT = ccCalculations.maxITT;
        msg->general_data = generalInfo_;
    }

    msg->total_RVs = ccCalculations.totalRvsInRange;
    msg->distance_from_RV = (uint32_t)(logData->distFromRV * 1000.0);
    msg->bsm_data = *bsmInfo;

    utils_->sendLogPacket(&info, PKT_ID_QITS_RX_FLOW);
}

void SplitsDiagLogger::sendGeneralPeriodicInfo() {
    if (!congestCtrl_) {
        LOGE(__FUNCTION__, " CongestCtrl has not been initialized");
        return;
    }

    auto ccCalculations = congestCtrl_->getCongCtrlCalculationData();
    v2x_qits_general_periodic_info info = {0};
    V2X_QITS_GENERAL_PERIODIC_PKG *msg =
        (V2X_QITS_GENERAL_PERIODIC_PKG *)((uint32_t *)(&info) + 1);

    msg->max_ITT = ccCalculations.maxITT;
    msg->vehicle_density_in_range = (uint32_t)(ccCalculations.smoothDens * 100.0);
    msg->total_RVs = ccCalculations.totalRvsInRange;
    msg->tracking_error = ccCalculations.trackingError;

    {
        std::lock_guard<std::mutex> lock(mtxGeneralInfo_);
        msg->CPU_Util = generalInfo_.CPU_Util;
        msg->L2_ID = generalInfo_.L2_ID;
        msg->events = generalInfo_.events;
    }

    utils_->sendLogPacket(&info, PKT_ID_QITS_GENERIC_INFO);
}

} //namespace Core
} //namespace SPLITS