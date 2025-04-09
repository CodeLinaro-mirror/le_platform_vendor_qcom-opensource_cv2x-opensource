/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_COMMON_LOGGER_HPP__
#define __SAE_COMMON_LOGGER_HPP__

#include <iostream>
#include <iomanip>
#include <vector>
#include "v2xMsgSae.hpp"
#include "J2735_Common.h"

using std::setfill;
using std::setw;
using std::ios;
using std::hex;
using std::endl;
using std::ostream;

namespace SPLITS {

class SaeCommonLogger {

public:
    SaeCommonLogger(ostream *os) : os_(os) {
    }
    virtual ~SaeCommonLogger() {}

    /**
     * log the message
     *
     * This is used by service side
     * @param[in] msg the v2x message data pointer
     * @return none.
     */
    virtual void logMsg(std::shared_ptr<v2xMsgSae> msg) {}

    /**
     * log the BSM message.
     *
     * This is used by the client side.
     * @param[in] bsm reference to the IDL bsm data
     * @return none.
     */
    virtual void logMsg(const J2735::idl::BasicSafetyMessage &bsm){}
    virtual void logMsg(const J2735::idl::MapData &map){}
    virtual void logMsg(const J2735::idl::SPAT &spat){}
    virtual void logMsg(const J2735::idl::RTCMcorrection &rtcm){}
    virtual void logMsg(const J2735::idl::RoadSafetyMessage &rsm){}
    virtual void logMsg(const J2735::idl::SensorDataSharingMessage &sdsm){}
    virtual void logMsg(const J2735::idl::SignalRequestMessage &srm){}


    template<typename idlTypeName>
    void printEnumList(std::vector<idlTypeName> idlList, int ind) {
        *os_ << indents[ind];
        for (auto elem : idlList) {
            *os_  << elem << ", ";
        }
        *os_ << endl;
    }

    void printDDateTime(const J2735::idl::DDateTime &idlDT, int ind) {
        if (idlDT.year().has_value()) {
            *os_ << indents[ind] << "year: " << idlDT.year().value() << endl;
        }
        if (idlDT.month().has_value()) {
            *os_ << indents[ind] << " monmth: " << idlDT.month().value() << endl;
        }
        if (idlDT.day().has_value()) {
            *os_ << indents[ind] << " day: " << idlDT.day().value() << endl;
        }
        if (idlDT.hour().has_value()) {
            *os_ << indents[ind] << " hour: " << idlDT.hour().value() << endl;
        }
        if (idlDT.minute().has_value()) {
            *os_ << indents[ind] << " min: " << idlDT.minute().value() << endl;
        }
        if (idlDT.second().has_value()) {
            *os_ << indents[ind] << " sec: " << idlDT.second().value() << endl;
        }
        if (idlDT.offset().has_value()) {
            *os_ << indents[ind] << " off: " << idlDT.offset().value() << endl;
        }
        *os_ << endl;
    }
    void printDDate(const J2735::idl::DDate &idlD, int ind) {
        *os_ << indents[ind] << "Y: " << idlD.year() << " M: " << idlD.month() << " D: " << idlD.day() << endl;
    }
    void printDTime(const J2735::idl::DTime &idlDT, int ind) {
        *os_ << indents[ind] << " H: " << idlDT.hour() << " min: " << idlDT.minute() << " sec: " << idlDT.second();
        if (idlDT.offset().has_value()) {
            *os_ << indents[ind] << " off: " << idlDT.offset().value();
        }
    }
    void printSpeedConfidence(const J2735::idl::SpeedandHeadingandThrottleConfidence &idlSHC,
            int ind) {
        *os_ << indents[ind] << "heading:" << idlSHC.heading() << endl;
        *os_ << indents[ind] << "speed:" << idlSHC.speed() << endl;
        *os_ << indents[ind] << "throttle:" << idlSHC.throttle() << endl;
    }
    void printPositionalAccuracy(const J2735::idl::PositionalAccuracy &idlPosAccu,
            int ind) {
        *os_ << indents[ind] << "semiMajor:" << static_cast<int>(idlPosAccu.semiMajor()) << endl;
        *os_ << indents[ind] << "semiMnior:" << static_cast<int>(idlPosAccu.semiMinor()) << endl;
        *os_ << indents[ind] << "orientation:" << idlPosAccu.orientation()  << endl;;
    }
    void printFullPositionVector(const J2735::idl::FullPositionVector &idlPos, int ind) {
        *os_ << indents[ind] << "utcTime: ";
        if (idlPos.utcTime().has_value()) {
            *os_ << endl;
            printDDateTime(idlPos.utcTime().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "elevation: " << (idlPos.elevation().has_value() ?
            std::to_string(idlPos.elevation().value()) : "n/a") << endl;;
        *os_ << indents[ind] << "heading: " << (idlPos.heading().has_value() ?
            std::to_string(idlPos.heading().value()) : "n/a") << endl;
        *os_ << indents[ind] << "speed: ";
        if (idlPos.speed().has_value()) {
            *os_ << endl;
            printTransmissionAndSpeed(idlPos.speed().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "posAccuracy:";
        if (idlPos.posAccuracy().has_value()) {
            *os_ << endl;
            printPositionalAccuracy(idlPos.posAccuracy().value(), ind+1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "timeConfidence:" << (idlPos.timeConfidence().has_value() ? 
            std::to_string(idlPos.timeConfidence().value()) : "n/a") << endl;
        *os_ << indents[ind] << "speedConfidence: ";
        if (idlPos.speedConfidence().has_value()) {
            *os_ << endl;
            printSpeedConfidence(idlPos.speedConfidence().value(), ind+1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "long:" << idlPos.longitude() << endl;
        *os_ << indents[ind] << "lat:" << idlPos.lat() << endl;
    }
    void printPathHistory(const J2735::idl::PathHistory &idlPH, int ind) {
        *os_ << indents[ind] << "initialPos: " << endl;
        if (idlPH.initialPosition().has_value()) {
            printFullPositionVector(idlPH.initialPosition().value(), ind+1);
        } else {
            *os_ << "n/a"<< endl;
        }
        *os_ << indents[ind] << "currGNSSstatus:" << (idlPH.currGNSSstatus().has_value() ?
            std::to_string(idlPH.currGNSSstatus().value()): "n/a") << endl;
        *os_ << indents[ind] << "crumData: " << endl;
        for (auto crumData : idlPH.crumbData()) {
            printPathHistoryPoint(crumData, ind+1);
            *os_ << endl;
        }
    }
    void printPathPrediction(const J2735::idl::PathPrediction &idlPP, int ind) {
        *os_ << indents[ind] << "radiusOfCurve:" << idlPP.radiusOfCurve() << endl;
        *os_ << indents[ind] << "confidence: " << static_cast<int>(idlPP.confidence()) << endl;
    }
    void printEmergencyDetails(const J2735::idl::EmergencyDetails &idlEmg, int ind) {
        *os_ << indents[ind] << "sirenUse:" << idlEmg.sirenUse() << endl;
        *os_ << indents[ind] << "lights: " << idlEmg.lightsUse() << endl;
        *os_ << indents[ind] << "multi: " << idlEmg.multi() << endl;
        //*os_ << indents[ind] << "events: " << (idlEmg.events().has_value() ?
        //     std::to_string(idlEmg.events().value()) : "n/a") << endl; //fix me
        *os_ << indents[ind] << "responseType: " << (idlEmg.responseType().has_value()?
            std::to_string(idlEmg.responseType().value()) : "n/a") << endl;
    }
    void printTransmissionAndSpeed(const J2735::idl::TransmissionAndSpeed &idlTS, int ind) {
        *os_ << indents[ind] << "transmission:" << idlTS.transmission() << endl;
        *os_ << indents[ind] << "speed:" << idlTS.speed() << endl;
    }
    void printVehicleClassification(const J2735::idl::VehicleClassification &idlVC, int ind) {
        *os_ << indents[ind] << "keyType:" << (idlVC.keyType().has_value() ?
            std::to_string(idlVC.keyType().value()) : "n/a") << endl;
        *os_ << indents[ind] << "role:" << (idlVC.role().has_value() ?
            std::to_string(idlVC.role().value()) : "n/a") << endl;
        *os_ << indents[ind] << "ios_3883:" << (idlVC.iso3883().has_value() ?
            std::to_string(idlVC.iso3883().value()) : "n/a") << endl;
        *os_ << indents[ind] << "hpmsType:" << (idlVC.hpmsType().has_value() ?
            std::to_string(idlVC.hpmsType().value()) : "n/a") << endl;
        *os_ << indents[ind] << "vehicleType:" << (idlVC.vehicleType().has_value() ?
            std::to_string(idlVC.vehicleType().value()) : "n/a") << endl;
        *os_ << indents[ind] << "responseEquipment:" << (idlVC.responseEquip().has_value() ?
            std::to_string(idlVC.responseEquip().value()) : "n/a") << endl;
        *os_ << indents[ind] << "responderType:" << (idlVC.responderType().has_value() ?
            std::to_string(idlVC.responderType().value()) : "n/a") << endl;
        *os_ << indents[ind] << "fuelType:" << (idlVC.fuelType().has_value() ?
            std::to_string(idlVC.fuelType().value()) : "n/a") << endl;
    }

    void printAxles(const J2735::idl::Axles & idlAxles, int ind) {
        *os_ << indents[ind] << "totalAxles:" << idlAxles.totalAxles() << endl;
        *os_ << indents[ind] << "frontAxles:" << (idlAxles.frontAxles().has_value() ?
            std::to_string(idlAxles.frontAxles().value()) : "n/a") << endl;
        *os_ << indents[ind] << "reaAxles:" << (idlAxles.rearAxles().has_value() ?
            std::to_string(idlAxles.rearAxles().value()) : "n/a") << endl;
    }
    void printAccelerationSet4Way(const J2735::idl::AccelerationSet4Way &idlAcc, int ind) {
        *os_ << indents[ind] << "longitude:" << idlAcc.longitude() << endl;
        *os_ << indents[ind] << "lat:" << idlAcc.lat() << endl;
        *os_ << indents[ind] << "vert:" << static_cast<int>(idlAcc.vert()) << endl;
        *os_ << indents[ind] << "yaw:" << idlAcc.yaw() << endl;
    }
    void printBrakeSystemStatus(const J2735::idl::BrakeSystemStatus &brakes, int ind) {
        *os_ << indents[ind] << "BrakeAppliedStatus: " << brakes.wheelBrakes() << endl;
        *os_ << indents[ind] << "TractionControlStatus: " << brakes.traction() << endl;
        *os_ << indents[ind] << "ABS: " << brakes.abs() << endl;
        *os_ << indents[ind] << "SCS: " << brakes.scs() << endl;
        *os_ << indents[ind] << "brakeBoos_t: " << brakes.brakeBoost() << endl;
        *os_ << indents[ind] << "auxBrakes: " << brakes.auxBrakes() << endl;
    }
    void printTemporaryID(const J2735::idl::TemporaryID &idlID, int ind) {
        std::ios state(NULL);
        state.copyfmt(*os_);
        *os_ << indents[ind] << "id: " << std::hex << setfill('0') << setw(2)<<
            static_cast<int>(idlID[0]) << " " <<
            static_cast<int>(idlID[1]) << " " <<
            static_cast<int>(idlID[2]) << " " <<
            static_cast<int>(idlID[3]) << endl;
        (*os_).copyfmt(state);
    }
    void printBSMcoreData(const J2735::idl::BSMCoreData &coreData, int ind) {
        *os_ << indents[ind] << "msgCnt: " << static_cast<int>(coreData.msgCnt()) << endl;
        printTemporaryID(coreData.id(), ind);
        *os_ << indents[ind] << "secMark: " << coreData.secMark() << endl;
        *os_ << indents[ind] << "lat: " << coreData.lat() << endl;
        *os_ << indents[ind] << "long: " << coreData.longitude() << endl;
        *os_ << indents[ind] << "elv: " << static_cast<int>(coreData.elev()) << endl;
        *os_ << indents[ind] << "positionalAccuracy:" << endl;
        printPositionalAccuracy(coreData.accuracy(), ind+1);
        *os_ << indents[ind] << "transmission: " << coreData.transmission() << endl;
        *os_ << indents[ind] << "speed: " << coreData.speed() << endl;
        *os_ << indents[ind] << "heading: " << coreData.heading() << endl;
        *os_ << indents[ind] << "steeringWheelAngle: " << static_cast<int>(coreData.angle()) << endl;
        *os_ << indents[ind] << "AccelSet4Way: " << endl;
        printAccelerationSet4Way(coreData.accelSet(), ind+1);
        *os_ << indents[ind] << "BrakeSystemStatus: " << endl;
        printBrakeSystemStatus(coreData.brakes(), ind+1);
        *os_ << indents[ind] << "\t size: L: " << static_cast<int>(coreData.size().length()) << " W: "
            << static_cast<int>(coreData.size().width()) << endl;
    }
    void printPosition3D(const J2735::idl::Position3D &pos_, int ind) {
        *os_ << indents[ind] << "lat: " << pos_.lat() << endl;
        *os_ << indents[ind] << "long: " << pos_.long_() << endl;
        *os_ << indents[ind] << "elevation:" << (pos_.elevation().has_value() ?
            std::to_string(pos_.elevation().value()) : "n/a") << endl;
    }
    void printRegulatorySpeedLimit(const J2735::idl::RegulatorySpeedLimit &idlSpeed, int ind) {
        *os_ << indents[ind] << "type: " << idlSpeed.type() << endl;
        *os_ << indents[ind] << "speed: " << idlSpeed.speed() << endl;
    }
    void printSpeedLimitList(const J2735::idl::SpeedLimitList idlList, int ind) {
        for (auto limit : idlList) {
            printRegulatorySpeedLimit(limit, ind+1);
            *os_ << endl;
        }
    }
    void printNodeListXY(const J2735::idl::NodeListXY &idlNodeListXY, int ind) {
        if (idlNodeListXY._d() == 0) {
            *os_ <<  indents[ind] << "node:" << endl;
            printNodeSetXY(idlNodeListXY.node(), ind + 1);
        } else {
            *os_ <<  indents[ind] << "computed:" << endl;
            printComputedLane(idlNodeListXY.computed(), ind + 1);
        }
    }
    template<typename idlTypeName>
    void printReferenceID(idlTypeName idlId, int ind) {
        std::ios  state(NULL);
        state.copyfmt(*os_);
        *os_ << indents[ind] << "ReferenceID: " << std::hex << idlId.id() << endl;
        if (idlId.region().has_value()) {
            *os_ << indents[ind] << "Regioin: " << std::hex << idlId.region().value() << endl;
        }
        (*os_).copyfmt(state);
    }
    void printFrictionInformation(const J2735::idl::FrictionInformation &idlFriction, int ind) {
        *os_ << indents[ind] << "DescriptionOfRoadSurface:" << endl;
        printtDescriptionOfRoadSurface(idlFriction.roadSurfaceDescription(), ind + 1);
        *os_ << indents[ind + 1] << "dryOrWet:" << (idlFriction.dryOrWet().has_value() ?
            std::to_string(idlFriction.dryOrWet().value()) : "n/a") ;

        *os_ << indents[ind + 1] << "roadRoughness:" << endl;
        if (idlFriction.roadRoughness().has_value()) {
            printRoadRoughness(idlFriction.roadRoughness().value(), ind + 1);
        } else {
             *os_ << indents[ind + 1] << "n/a" << endl;
        }
    }
    void printPositionConfidenceSet(const J2735::idl::PositionConfidenceSet &idlPos, int ind) {
        *os_ << indents[ind] <<"pos: " << idlPos.pos() << endl;
        *os_ << indents[ind] <<"elevation: " << idlPos.elevation() << endl;
    }
    void printExteriorLights(const J2735::idl::ExteriorLights &idlLights, int ind) {
        std::ios  state(NULL);
        state.copyfmt(*os_);
        *os_ << indents[ind] << "lights: " << setfill('0') << setw(4) << hex << idlLights << endl;
        (*os_).copyfmt(state);
    }
    void printAntennaOffsetSet(const J2735::idl::AntennaOffsetSet &idlAntennaOFS, int ind) {
        *os_ << indents[ind] << "offsetX: " << idlAntennaOFS.antOffsetX() << endl;
        *os_ << indents[ind] << "offsetY: " << idlAntennaOFS.antOffsetY() << endl;
        *os_ << indents[ind] << "offsetZ: " << idlAntennaOFS.antOffsetZ() << endl;
    }
    void printRequestorType(const J2735::idl::RequestorType &idlRT, int ind) {
        *os_ << indents[ind] << "role: " << idlRT.role() << endl;
        *os_ << indents[ind] << "subrole: " << (idlRT.subrole().has_value() ?
            std::to_string(idlRT.subrole().value()) : "n/a") << endl;
        *os_ << indents[ind] << "request: " << (idlRT.request().has_value() ?
            std::to_string(idlRT.request().value()) : "n/a") << endl;
        *os_ << indents[ind] << "iso3883: " << (idlRT.iso3883().has_value() ?
            std::to_string(idlRT.iso3883().value()) : "n/a") << endl;
    }
    void printVehicleID(const J2735::idl::VehicleID &idlID, int ind) {
        switch(idlID._d()) {
            case 0:
                printTemporaryID(idlID.entityID(), ind + 1);
                break;
            case 1:
                *os_ << indents[ind+1] << "stationId: " << idlID.stationID() << endl;
                break;
        }
    }
    void printIntersectionAccessPoint(const J2735::idl::IntersectionAccessPoint &idlAP, int ind) {
        switch(idlAP._d()) {
            case 0:
                *os_ << indents[ind] << "lane:" << idlAP.lane() << endl;
                break;
            case 1:
                *os_ << indents[ind] << "approach:" << idlAP.approach() << endl;
                break;
            case 2:
                *os_ << indents[ind] << "connection:" << idlAP.connection() << endl;
                break;
        }
    }
protected:
    std::ostream* os_ = &std::cout;
    std::string indents[6] =  {
            "",
            "\t",
            "\t\t",
            "\t\t\t",
            "\t\t\t\t",
            "\t\t\t\t\t"
        };

    void printPathHistoryPoint(const J2735::idl::PathHistoryPoint &idlPHP, int ind) {
        *os_ << indents[ind] << "speed:" << (idlPHP.speed().has_value() ?
                std::to_string(idlPHP.speed().value()) : "n/a") << endl;
        *os_ << indents[ind] << "posAccuracy:";
        if (idlPHP.posAccuracy().has_value()) {
            *os_ << endl;
            printPositionalAccuracy(idlPHP.posAccuracy().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "heading:" << (idlPHP.heading().has_value()?
                std::to_string(idlPHP.heading().value()) : "n/a") << endl;
        *os_ << indents[ind] << "latOffset: "<< idlPHP.latOffset() << endl;
        *os_ << indents[ind] << "lonOffset: "<< idlPHP.lonOffset() << endl;
        *os_ << indents[ind] << "elevationOffset: "<< idlPHP.elevationOffset() << endl;
        *os_ << indents[ind] << "timeOffset: "<< idlPHP.timeOffset() << endl;
    }
    void printtDescriptionOfRoadSurface(const J2735::idl::DescriptionOfRoadSurface &idlDoRS, int ind) {
        switch(idlDoRS._d()) {
            case 0:
                *os_ << indents[ind] << "portlandCement:" << idlDoRS.portlandCement() << endl;
                break;
            case 1:
                *os_ << indents[ind] << "asphaltOrTar:" << idlDoRS.asphaltOrTar() <<  endl;
                break;
            case 2:
                *os_ << indents[ind] << "gravel:" << idlDoRS.gravel() << endl;
                break;
            case 3:
                *os_ << indents[ind] << "grass:" << idlDoRS.grass() <<  endl;
                break;
            case 4:
                *os_ << indents[ind] << "cinders:" << idlDoRS.cinders() <<  endl;
                break;
            case 5:
                *os_ << indents[ind] << "rock:" << idlDoRS.rock() <<  endl;
                break;
            case 6:
                *os_ << indents[ind] << "ice:" << idlDoRS.ice() <<  endl;
                break;
            case 7:
                *os_ << indents[ind] << "snow:" << idlDoRS.snow() <<  endl;
                break;
        }
    }
    void printRoadRoughness(const J2735::idl::RoadRoughness &idlRoughness, int ind) {
        *os_ << indents[ind] <<"meanVerticalVariation:" << idlRoughness.meanVerticalVariation() << endl;
        *os_ << indents[ind] <<"verticalVariationStdDev:" << (idlRoughness.verticalVariationStdDev().has_value() ?
            std::to_string(idlRoughness.verticalVariationStdDev().value()) : "n/a") << endl;;
        *os_ << indents[ind] <<"meanHorizontalVariation:" << (idlRoughness.meanHorizontalVariation().has_value()?
            std::to_string(idlRoughness.meanHorizontalVariation().value()) : "n/a") << endl;;
        *os_ << indents[ind] <<"horizontalVariationStdDev:" << (idlRoughness.horizontalVariationStdDev().has_value()?
            std::to_string(idlRoughness.horizontalVariationStdDev().value()) : "n/a") << endl;;
    }
    void printLaneDataAttribute(const J2735::idl::LaneDataAttribute &idlLDA, int ind) {
        switch(idlLDA._d()) {
            case 0:
                *os_ << indents[ind] << "pathEndPointAngle: " << idlLDA.pathEndPointAngle() << endl;
                break;
            case 1:
                *os_ << indents[ind] << "laneCrownPointCenter: " << idlLDA.laneCrownPointCenter() << endl;
                break;
            case 2:
                *os_ << indents[ind] << "laneCrownPointLeft: " << idlLDA.laneCrownPointLeft() << endl;
                break;
            case 3:
                *os_ << indents[ind] << "laneCrownPointRight: " << idlLDA.laneCrownPointRight() << endl;
                break;
            case 4:
                *os_ << indents[ind] << "laneAngle: " << idlLDA.laneAngle() << endl;
                break;
            case 5:
                *os_ << indents[ind] << "speedLimits: " << endl;
                printSpeedLimitList(idlLDA.speedLimits(), ind+1);
                break;
        }
    }
    void printLaneDataAttributeList(const J2735::idl::LaneDataAttributeList &idlList, int ind) {
        for (auto elem : idlList) {
            printLaneDataAttribute(elem, ind);
        }
    }
    void printNodeAttributeSetXY(const J2735::idl::NodeAttributeSetXY &idlNodeAttrSetXY, int ind) {
        *os_ << indents[ind] << "localNode: ";
        if (idlNodeAttrSetXY.localNode().has_value()) {
            printEnumList<J2735::idl::NodeAttributeXY>(idlNodeAttrSetXY.localNode().value(),0);
        } else {
            *os_ << "n/a";
        }
        *os_ << indents[ind] << "disabled:";
        if (idlNodeAttrSetXY.disabled().has_value()) {
            printEnumList<J2735::idl::SegmentAttributeXY>(idlNodeAttrSetXY.disabled().value(), 0);
        } else {
            *os_ << "n/a";
        }
        *os_ <<  indents[ind] << "enabled:";
        if (idlNodeAttrSetXY.enabled().has_value()) {
            printEnumList<J2735::idl::SegmentAttributeXY>( idlNodeAttrSetXY.enabled().value(), 0);
        } else {
            *os_ << "n/a";
        }
        *os_ <<  indents[ind] << "data:";
        if (idlNodeAttrSetXY.data().has_value()) {
            *os_ << endl;
            printLaneDataAttributeList(idlNodeAttrSetXY.data().value(), ind+1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ <<  indents[ind] << "dWidth:" << (idlNodeAttrSetXY.dWidth().has_value() ?
            std::to_string(idlNodeAttrSetXY.dWidth().value()) : "n/a") << endl;
        *os_ <<  indents[ind] << "dElevation:" << (idlNodeAttrSetXY.dElevation().has_value() ?
            std::to_string(idlNodeAttrSetXY.dElevation().value()) : "n/a") << endl;;
    }
    void printNodeOffsetPointXY(const J2735::idl::NodeOffsetPointXY &idlOffset, int ind) {
        switch(idlOffset._d()) {
            case 0:
                *os_ <<  indents[ind] << "NodeXY1:" << idlOffset.node_XY1().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 1:
                *os_ <<  indents[ind] << "NodeXY2:" << idlOffset.node_XY2().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 2:
                *os_ <<  indents[ind] << "NodeXY3:" << idlOffset.node_XY3().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 3:
                *os_ <<  indents[ind] << "NodeXY4:" << idlOffset.node_XY4().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 4:
                *os_ <<  indents[ind] << "NodeXY5:" << idlOffset.node_XY5().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 5:
                *os_ <<  indents[ind] << "NodeXY6:" << idlOffset.node_XY6().x() << ", " <<
                    idlOffset.node_XY1().y() << endl;
                break;
            case 6:
                *os_ <<  indents[ind] << "Node_LatLon:" << idlOffset.node_LatLon().lat() << ", " <<
                    idlOffset.node_LatLon().lon() << endl;
                break;
        }
    }
    void printNodeXY(const J2735::idl::NodeXY &idlNodeXY, int ind) {
        *os_ <<  indents[ind] << "delta:" << endl;
        printNodeOffsetPointXY(idlNodeXY.delta(), ind+1);
        *os_ <<  indents[ind] << "attributes:" << endl;
        if (idlNodeXY.attributes().has_value()) {
            printNodeAttributeSetXY(idlNodeXY.attributes().value(), ind+1);
        }
    }
    void printNodeSetXY(const J2735::idl::NodeSetXY &idlNodeSetXY, int ind) {
        for (auto elem : idlNodeSetXY) {
            printNodeXY(elem, ind + 1);
        }
    }
    void printOffsetAxis(const J2735::idl::OffsetAxis &idlOffsetAxis, int ind) {
        switch(idlOffsetAxis._d()) {
            case 0:
                *os_ <<  indents[ind] << "small:" << idlOffsetAxis.small() << endl;
                break;
            case 1:
                *os_ <<  indents[ind] << "large:" << idlOffsetAxis.large() << endl;
                break;
        }
    }
    void printComputedLane(const J2735::idl::ComputedLane &idlLane, int ind) {
        printOffsetAxis(idlLane.offsetXaxis(), ind);
        printOffsetAxis(idlLane.offsetYaxis(), ind);

        *os_ <<  indents[ind] << "rotateXY:" << (idlLane.rotateXY().has_value() ? 
            std::to_string(idlLane.rotateXY().value()) : "n/a") << endl;
        *os_ <<  indents[ind] << "scaleXaxis:" << (idlLane.scaleXaxis().has_value() ? 
            std::to_string(idlLane.scaleXaxis().value()) : "n/a") << endl;;
        *os_ <<  indents[ind] << "scaleYaxis:" << (idlLane.rotateXY().has_value() ? 
            std::to_string(idlLane.scaleYaxis().value()) : "n/a") << endl;;
    }
};

} //namespace SPLITS
#endif //__SAE_COMMON_LOGGER_HPP__

