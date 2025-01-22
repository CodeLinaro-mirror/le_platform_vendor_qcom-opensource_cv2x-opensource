/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __V2XMSGSAE_HPP__
#define __V2XMSGSAE_HPP__
#include "v2xMsg.hpp"
#include "J2735_BasicSafetyMessage.h"
#include "J2735_MapData.h"
#include "J2735_SPAT.h"
#include "J2735_RTCMcorrections.h"
#include "J2735_RSM.h"
#include "J2735_SRM.h"
#include "J2735_SSM.h"
#include "J2735_SDSM.h"
#include "J2735_PDU.h"
#include "Ieee1609Dot3Wsa.h"

namespace SPLITS {

class v2xMsgSae : public v2xMsg {
public:
    v2xMsgSae() {};
    virtual ~v2xMsgSae(){};
    /**
     * get BSM message in IDL format
     *
     * @returns A shared pointer to the idl BSM message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::BasicSafetyMessage> BSM(void) {
        return idlBsm_;
    }
    /**
     * set BSM message data pointer.
     * 
     * @param[in] bsm A shared pointer to IDL BSM data.
     * @returns none.
     */
    inline void BSM(const std::shared_ptr<J2735::idl::BasicSafetyMessage> bsm) {
        idlBsm_ = bsm;
    }
    /**
     * Return MapData in IDL format
     *
     * @returns A shared pointer to the idl MapData stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::MapData> Map(void) {
        return idlMap_;
    }
    /**
     * set MapData message data pointer.
     * 
     * @param[in] mapData A shared pointer to IDL MapData data.
     * @returns none.
     */
    inline void Map(const std::shared_ptr<J2735::idl::MapData> map) {
        idlMap_ = map;
    }
    /**
     * Return SPAT message in IDL format
     *
     * @returns A shared pointer to the idl SPAT stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::SPAT> SPAT(void) {
        return idlSPAT_;
    }
    /**
     * set SPAT message data pointer.
     * 
     * @param[in] SPAT A shared pointer to IDL SPAT message.
     * @returns none.
     */
    inline void SPAT(const std::shared_ptr<J2735::idl::SPAT> spat) {
        idlSPAT_ = spat;
    }
    /**
     * Return RTCMcorrection message in IDL format
     *
     * @returns A shared pointer to the idl RTCMcorrection message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::RTCMcorrection> RTCMcorrection(void) {
        return idlRTCMcorrection_;
    }
    /**
     * set RTCMcorrection message data pointer.
     * 
     * @param[in] rtcm A shared pointer to IDL RTCMcorrection message.
     * @returns none.
     */
    inline void RTCMcorrection(const std::shared_ptr<J2735::idl::RTCMcorrection> rtcm) {
        idlRTCMcorrection_ = rtcm;
    }
    /**
     * Return SensorDataSharing message in IDL format
     *
     * @returns A shared pointer to the idl SDSM message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::SensorDataSharingMessage> SDSM(void) {
        return idlSDSM_;
    }
    /**
     * set SensorDataSharing message data pointer.
     * 
     * @param[in]  sdsm A shared pointer to IDL RTCMcorrection message.
     * @returns none.
     */
    inline void SDSM(const std::shared_ptr<J2735::idl::SensorDataSharingMessage> sdsm) {
        idlSDSM_ = sdsm;
    }
    /**
     * Return RoadSafetyMessage in IDL format
     *
     * @returns A shared pointer to the idl RoadSafety message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::RoadSafetyMessage> RSM(void) {
        return idlRSM_;
    }
    /**
     * set RoadSafety message data pointer.
     * 
     * @param[in]  rsm A shared pointer to IDL RTCMcorrection message.
     * @returns none.
     */
    inline void RSM(const std::shared_ptr<J2735::idl::RoadSafetyMessage> rsm) {
        idlRSM_ = rsm;
    }
    /**
     * Return SignalRequestMessage in IDL format
     *
     * @returns A shared pointer to the idl RoadSafety message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::SignalRequestMessage> SRM(void) {
        return idlSRM_;
    }
    /**
     * set SignalRequest message data pointer.
     * 
     * @param[in]  srm A shared pointer to IDL SignalRequest message.
     * @returns none.
     */
    inline void SRM(const std::shared_ptr<J2735::idl::SignalRequestMessage> srm) {
        idlSRM_ = srm;
    }
    /**
     * Return SignalStatusMessage in IDL format
     *
     * @returns A shared pointer to the idl SignalStatus message stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::SignalStatusMessage> SSM(void) {
        return idlSSM_;
    }
    /**
     * set SignalStatus message data pointer.
     * 
     * @param[in]  ssm A shared pointer to IDL SignalRequest message.
     * @returns none.
     */
    inline void SRM(const std::shared_ptr<J2735::idl::SignalStatusMessage> ssm) {
        idlSSM_ = ssm;
    }
    /**
     * get WSA message in IDL format
     *
     * @returns A shared pointer to the idl WSA message stored in this message data.
     */
    const inline std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> SrvAdvMsg(void) {
        return idlSrvAdvMsg_;
    }
    /**
     * set WSA message data pointer.
     * 
     * @param[in] srvAdvMsg A shared pointer to IDL WSA data.
     * @returns none.
     */
    inline void SrvAdvMsg(const std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> srvAdvMsg) {
        idlSrvAdvMsg_ = srvAdvMsg;
    }
    /**
     * get customized pdu data in IDL format
     *
     * @returns A shared pointer to the idl pdu stored in this message data.
     */
    const inline std::shared_ptr<J2735::idl::MsgData> MsgData(void) {
        return idlMsgData_;
    }
    /**
     * set pdu message data pointer.
     * 
     * @param[in] msgData A shared pointer to IDL pdu msg data.
     * @returns none.
     */
    inline void MsgData(const std::shared_ptr<J2735::idl::MsgData> msgData) {
        idlMsgData_ = msgData;
    }

private:
    std::shared_ptr<J2735::idl::BasicSafetyMessage> idlBsm_ = nullptr;    /** <message in IDL format */
    std::shared_ptr<J2735::idl::MapData> idlMap_ = nullptr;
    std::shared_ptr<J2735::idl::SPAT> idlSPAT_ = nullptr;
    std::shared_ptr<J2735::idl::RTCMcorrection> idlRTCMcorrection_ = nullptr;
    std::shared_ptr<J2735::idl::SensorDataSharingMessage> idlSDSM_ = nullptr;
    std::shared_ptr<J2735::idl::RoadSafetyMessage> idlRSM_ = nullptr;
    std::shared_ptr<J2735::idl::SignalRequestMessage> idlSRM_ = nullptr;
    std::shared_ptr<J2735::idl::SignalStatusMessage> idlSSM_ = nullptr;
    std::shared_ptr<IEEE1609Dot3::idl::SrvAdvMsg> idlSrvAdvMsg_ = nullptr;
    std::shared_ptr<J2735::idl::MsgData> idlMsgData_ = nullptr;
};
}  //namespace SPLITS
#endif //__V2XMSGSAE_HPP__
