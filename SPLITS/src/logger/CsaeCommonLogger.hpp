/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CSAE_COMMON_LOGGER_HPP__
#define __CSAE_COMMON_LOGGER_HPP__

#include <iostream>
#include <iomanip>
#include <vector>
#include "v2xMsgCsae.hpp"
#include "J2735_Common.h"
#include "SaeCommonLogger.hpp"

using std::setfill;
using std::setw;
using std::ios;
using std::hex;
using std::endl;
using std::ostream;

namespace SPLITS {

class CsaeCommonLogger : public SaeCommonLogger {

public:
    CsaeCommonLogger(ostream *os) : SaeCommonLogger(os) {}
    virtual ~CsaeCommonLogger() {}

    /**
     * log the message
     *
     * This is used by service side
     * @param[in] msg the v2x message data pointer
     * @return none.
     */
    virtual void logMsg(std::shared_ptr<v2xMsgCsae> msg){};

    /**
     * log the BSM message.
     *
     * This is used by the client side.
     * @param[in] bsm reference to the IDL bsm data
     * @return none.
     */
    virtual void logMsg(const Csae::idl::BasicSafetyMessage &bsm){}
    virtual void logMsg(const Csae::idl::MapData &map){}
    virtual void logMsg(const Csae::idl::SPAT &spat){}
    virtual void logMsg(const Csae::idl::RoadsideSafetyMessage &rsm){}
    virtual void logMsg(const Csae::idl::RoadSideInformation &rsi){}

    void printMotionConfidenceSet(const Csae::idl::MotionConfidenceSet &idlMoCfd, int ind) {
        *os_ << indents[ind] << "SpeedConfidence:" << (idlMoCfd.speedCfd().has_value() ?
            std::to_string(idlMoCfd.speedCfd().value()) : "n/a") << endl;
        *os_ << indents[ind] << "HeadingConfidence:" << (idlMoCfd.headingCfd().has_value() ?
            std::to_string(idlMoCfd.headingCfd().value()) : "n/a") << endl;
        *os_ << indents[ind] << "SteeringWheelAngleConfidence:" << (idlMoCfd.steerCfd().has_value() ?
            std::to_string(idlMoCfd.steerCfd().value()) : "n/a") << endl;
    }
    void printPosition3D(const Csae::idl::Position3D &idlPos, int ind) {
        *os_ << indents[ind] << "lat: " << idlPos.lat() << endl;
        *os_ << indents[ind] << "long: " << idlPos.long_() << endl;
        *os_ << indents[ind] << "elevation:" << (idlPos.elevation().has_value() ?
            std::to_string(idlPos.elevation().value()) : "n/a") << endl;
    }
    void printPositionConfidenceSet(const Csae::idl::PositionConfidenceSet &idlPosCfd, int ind) {
        *os_ << indents[ind] << "PositionConfidence: " << idlPosCfd.pos() << endl;
        *os_ << indents[ind] << "ElevationConfidence:" << (idlPosCfd.elevation().has_value() ?
            std::to_string(idlPosCfd.elevation().value()) : "n/a") << endl;
    }
    void printFullPositionVector(const Csae::idl::FullPositionVector &idlPos, int ind) {
        *os_ << indents[ind] << "utcTime: ";
        if (idlPos.utcTime().has_value()) {
            *os_ << endl;
            printDDateTime(idlPos.utcTime().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }

        *os_ << indents[ind] << "Position3D: " << endl;
        printPosition3D(idlPos.pos(), ind + 1);

        *os_ << indents[ind] << "heading:" << (idlPos.heading().has_value() ?
            std::to_string(idlPos.heading().value()) : "n/a") << endl;

        *os_ << indents[ind] << "transmission:" << (idlPos.transmission().has_value() ?
            std::to_string(idlPos.transmission().value()) : "n/a") << endl;

        *os_ << indents[ind] << "speed:" << (idlPos.speed().has_value() ?
            std::to_string(idlPos.speed().value()) : "n/a") << endl;

        *os_ << indents[ind] << "posAccuracy: ";
        if (idlPos.posAccuracy().has_value()) {
            *os_ << endl;
            printPositionConfidenceSet(idlPos.posAccuracy().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }

        *os_ << indents[ind] << "timeConfidence:" << (idlPos.timeConfidence().has_value() ?
            std::to_string(idlPos.timeConfidence().value()) : "n/a") << endl;

        *os_ << indents[ind] << "motionCfd: ";
        if (idlPos.motionCfd().has_value()) {
            *os_ << endl;
            printMotionConfidenceSet(idlPos.motionCfd().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
    }
    void printPositionOffsetLL(const Csae::idl::PositionOffsetLL &idlPosOff, int ind) {
        switch (idlPosOff._d()) {
            case 0:
                *os_ << indents[ind] << "Position_LL_24B: " << idlPosOff.position_LL1().lon()
                    << ", " << idlPosOff.position_LL1().lat() <<  endl;
                break;
            case 1:
                *os_ << indents[ind] << "Position_LL_28B: " << idlPosOff.position_LL2().lon()
                    << ", " << idlPosOff.position_LL2().lat() <<  endl;
                break;
            case 2:
                *os_ << indents[ind] << "Position_LL_32B: " << idlPosOff.position_LL3().lon()
                    << ", " << idlPosOff.position_LL3().lat() <<  endl;
                break;
            case 3:
                *os_ << indents[ind] << "Position_LL_36B: " << idlPosOff.position_LL4().lon()
                    << ", " << idlPosOff.position_LL4().lat() <<  endl;
                break;
            case 4:
                *os_ << indents[ind] << "Position_LL_44B: " << idlPosOff.position_LL5().lon()
                    << ", " << idlPosOff.position_LL5().lat() <<  endl;
                break;
            case 5:
                *os_ << indents[ind] << "Position_LL_48B: " << idlPosOff.position_LL6().lon()
                    << ", " << idlPosOff.position_LL6().lat() <<  endl;
                break;
            case 6:
                *os_ << indents[ind] << "Position_LLmD_64b: " << idlPosOff.position_LatLon().lon()
                    << ", " << idlPosOff.position_LatLon().lat() <<  endl;
                break;
            default:
                break;
        }
    }
    void printVerticalOffset(const Csae::idl::VerticalOffset &idlOff, int ind) {
        switch (idlOff._d()) {
            case 0:
                *os_ << indents[ind] << "offset1: " << idlOff.offset1() <<  endl;
                break;
            case 1:
                *os_ << indents[ind] << "offset2: " << idlOff.offset2() <<  endl;
                break;
            case 2:
                *os_ << indents[ind] << "offset3: " << idlOff.offset3() <<  endl;
                break;
            case 3:
                *os_ << indents[ind] << "offset4: " << idlOff.offset4() <<  endl;
                break;
            case 4:
                *os_ << indents[ind] << "offset5: " << idlOff.offset5() <<  endl;
                break;
            case 5:
                *os_ << indents[ind] << "offset6: " << idlOff.offset6() <<  endl;
                break;
            case 6:
                *os_ << indents[ind] << "elevation: " << idlOff.elevation() <<  endl;
                break;
            default:
                break;
        }
    }
    void printPositionOffsetLLV(const Csae::idl::PositionOffsetLLV &idlPosOff, int ind) {
        *os_ << indents[ind] << "PositionOffsetLL: " << endl;
        printPositionOffsetLL(idlPosOff.offsetLL(), ind + 1);

        *os_ << indents[ind] << "VerticalOffset: ";
        if (idlPosOff.offsetV().has_value()) {
            *os_ << endl;
            printVerticalOffset(idlPosOff.offsetV().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
    }
    void printPathHistoryPoint(const Csae::idl::PathHistoryPoint &idlPhp, int ind) {
        *os_ << indents[ind] << "PositionOffsetLLV: " << endl;
        printPositionOffsetLLV(idlPhp.llvOffset(), ind + 1);
        *os_ << indents[ind] << "TimeOffset: " << idlPhp.timeOffset() << endl;
        *os_ << indents[ind] << "Speed:" << (idlPhp.speed().has_value() ?
            std::to_string(idlPhp.speed().value()) : "n/a") << endl;
        *os_ << indents[ind] << "posAccuracy: ";
        if (idlPhp.posAccuracy().has_value()) {
            *os_ << endl;
            printPositionConfidenceSet(idlPhp.posAccuracy().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }
        *os_ << indents[ind] << "heading:" << (idlPhp.heading().has_value() ?
            std::to_string(idlPhp.heading().value()) : "n/a") << endl;
    }
    void printPathHistoryPointList(const Csae::idl::PathHistoryPointList &idlList, int ind) {
        for (int i = 0; i < idlList.size(); ++i) {
            *os_ << indents[ind] << "PathHistoryPoint[" << i << "]: " << endl;
            printPathHistoryPoint(idlList[i], ind + 1);
        }
    }

    void printPathHistory(const Csae::idl::PathHistory &idlPh, int ind) {
        *os_ << indents[ind] << "initialPosition: ";
        if (idlPh.initialPosition().has_value()) {
            *os_ << endl;
            printFullPositionVector(idlPh.initialPosition().value(), ind + 1);
        } else {
            *os_ << "n/a" << endl;
        }

        *os_ << indents[ind] << "currGNSSstatus:" << (idlPh.currGNSSstatus().has_value() ?
            std::to_string(idlPh.currGNSSstatus().value()) : "n/a") << endl;

        *os_ << indents[ind] << "crumbData: " << endl;
        printPathHistoryPointList(idlPh.crumbData(), ind + 1);
    }

};

} //namespace SPLITS
#endif //__CSAE_COMMON_LOGGER_HPP__

