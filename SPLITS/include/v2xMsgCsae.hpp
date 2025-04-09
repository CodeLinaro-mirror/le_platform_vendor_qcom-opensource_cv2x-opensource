/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __V2XMSGCSAE_HPP__
#define __V2XMSGCSAE_HPP__

#include "v2xMsg.hpp"
#include "splits-dds-gen/BSM.h"
#include "splits-dds-gen/Map.h"
#include "splits-dds-gen/RSI.h"
#include "splits-dds-gen/RSM.h"
#include "splits-dds-gen/SignalPhaseAndTiming.h"

namespace SPLITS {

class v2xMsgCsae : public v2xMsg {
public:
    v2xMsgCsae() {};
    virtual ~v2xMsgCsae(){};
    /**
     * get BSM message in IDL format
     *
     * @returns A shared pointer to the idl BSM message stored in this message data.
     */
    const inline std::shared_ptr<Csae::idl::BasicSafetyMessage> BSM(void) {
        return idlBsm_;
    }
    /**
     * set BSM message data pointer.
     *
     * @param[in] bsm A shared pointer to IDL BSM data.
     * @returns none.
     */
    inline void BSM(const std::shared_ptr<Csae::idl::BasicSafetyMessage> bsm) {
        idlBsm_ = bsm;
    }
    /**
     * Return MapData in IDL format
     *
     * @returns A shared pointer to the idl MapData stored in this message data.
     */
    const inline std::shared_ptr<Csae::idl::MapData> Map(void) {
        return idlMap_;
    }
    /**
     * set MapData message data pointer.
     *
     * @param[in] mapData A shared pointer to IDL MapData data.
     * @returns none.
     */
    inline void Map(const std::shared_ptr<Csae::idl::MapData> map) {
        idlMap_ = map;
    }
    /**
     * Return SPAT message in IDL format
     *
     * @returns A shared pointer to the idl SPAT stored in this message data.
     */
    const inline std::shared_ptr<Csae::idl::SPAT> SPAT(void) {
        return idlSPAT_;
    }
    /**
     * set SPAT message data pointer.
     *
     * @param[in] SPAT A shared pointer to IDL SPAT message.
     * @returns none.
     */
    inline void SPAT(const std::shared_ptr<Csae::idl::SPAT> spat) {
        idlSPAT_ = spat;
    }
    /**
     * Return RoadsideSafetyMessage in IDL format
     *
     * @returns A shared pointer to the idl RoadsideSafetyMessage stored in this message data.
     */
    const inline std::shared_ptr<Csae::idl::RoadsideSafetyMessage> RSM(void) {
        return idlRSM_;
    }
    /**
     * set RoadsideSafetyMessage message data pointer.
     *
     * @param[in]  rsm A shared pointer to RoadsideSafetyMessage.
     * @returns none.
     */
    inline void RSM(const std::shared_ptr<Csae::idl::RoadsideSafetyMessage> rsm) {
        idlRSM_ = rsm;
    }
    /**
     * Return RoadSideInformation in IDL format
     *
     * @returns A shared pointer to the idl RoadSideInformation stored in this message data.
     */
    const inline std::shared_ptr<Csae::idl::RoadSideInformation> RSI(void) {
        return idlRSI_;
    }
    /**
     * set RoadSideInformation message data pointer.
     *
     * @param[in]  rsm A shared pointer to RoadSideInformation.
     * @returns none.
     */
    inline void RSI(const std::shared_ptr<Csae::idl::RoadSideInformation> rsi) {
        idlRSI_ = rsi;
    }

private:
    std::shared_ptr<Csae::idl::BasicSafetyMessage> idlBsm_ = nullptr;    /** <message in IDL format */
    std::shared_ptr<Csae::idl::MapData> idlMap_ = nullptr;
    std::shared_ptr<Csae::idl::SPAT> idlSPAT_ = nullptr;
    std::shared_ptr<Csae::idl::RoadsideSafetyMessage> idlRSM_ = nullptr;
    std::shared_ptr<Csae::idl::RoadSideInformation> idlRSI_ = nullptr;
};
}  //namespace SPLITS
#endif //__V2XMSGCSAE_HPP__
