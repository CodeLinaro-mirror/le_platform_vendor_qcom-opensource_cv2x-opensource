/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "v2xMsgTypes.hpp"
#include "v2xMsgCsae.hpp"
#include "SplitsCoreImpl.hpp"
#include "SplitsCoreDataListenerCsae.hpp"
#include "Logger.hpp"

using SPLITS::MsgFormat;
using SPLITS::v2xMsgType;

namespace SPLITS {

namespace Core {

void SplitsCoreDataListenerCsae::onBsmReady(std::shared_ptr<Csae::idl::BasicSafetyMessage> bsm) {
    auto msg = std::dynamic_pointer_cast<v2xMsgCsae>(splitsCore_->txEventMsgs_[CSAE_AID_BSM]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for BSM msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_CSAE_MSG_BSM;
    msg->BSM(bsm);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(CSAE_AID_BSM, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

void SplitsCoreDataListenerCsae::onMapReady(std::shared_ptr<Csae::idl::MapData> map) {
    auto msg = std::dynamic_pointer_cast<v2xMsgCsae>(splitsCore_->txEventMsgs_[CSAE_AID_MAP]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for MAP msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_CSAE_MSG_MAP;
    msg->Map(map);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(CSAE_AID_MAP, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

void SplitsCoreDataListenerCsae::onSpatReady(std::shared_ptr<Csae::idl::SPAT> spat) {
    auto msg = std::dynamic_pointer_cast<v2xMsgCsae>(splitsCore_->txEventMsgs_[CSAE_AID_SPAT]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for SPAT msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_CSAE_MSG_SPAT;
    msg->SPAT(spat);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(CSAE_AID_SPAT, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

void SplitsCoreDataListenerCsae::onRsmReady(std::shared_ptr<Csae::idl::RoadsideSafetyMessage> rsm) {
    auto msg = std::dynamic_pointer_cast<v2xMsgCsae>(splitsCore_->txEventMsgs_[CSAE_AID_RSM]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for RSM msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_CSAE_MSG_RSM;
    msg->RSM(rsm);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(CSAE_AID_RSM, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

void SplitsCoreDataListenerCsae::onRsiReady(std::shared_ptr<Csae::idl::RoadSideInformation> rsi) {
    auto msg = std::dynamic_pointer_cast<v2xMsgCsae>(splitsCore_->txEventMsgs_[CSAE_AID_RSI_STATIC]);
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg for RSI msg ");
        return;
    }
    msg->msgType_ = v2xMsgType::V2X_CSAE_MSG_RSI;
    msg->RSI(rsi);
    msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    if (splitsCore_->send(CSAE_AID_RSI_STATIC, msg) < 0) {
        LOGE(__FUNCTION__, " tx failure");
    }
}

} // namespace Core
} // namespace SPLITS
