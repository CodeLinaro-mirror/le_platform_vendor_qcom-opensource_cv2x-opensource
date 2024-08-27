/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeMapLogger.hpp"

namespace SPLITS {

void SaeMapLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->Map()));
}
void SaeMapLogger::logMsg(const J2735::idl::MapData &map)
{
    int ind = 0;
    if (map.timeStamp().has_value()) {
        *os_ << indents[ind] << "timestamp:" << map.timeStamp().value() << endl;
    }
    *os_ << indents[ind] << "messageIssueRevision: " << map.msgIssueRevision() << endl;
    if (map.layerType().has_value()) {
        *os_ << indents[ind] << "layerType:" <<  static_cast<int>(map.layerType().value()) << endl;
    }
    if (map.layerID().has_value()) {
        *os_ << indents[ind] << "layerID:" <<  static_cast<int>(map.layerID().value()) << endl;
    }
#if 0
    if (map.intersections().has_value()) {
        printIntersectionGeometryList(map.intersections().value(), ind);
    }
    if (map.dataParameters.has_value()) {
        printDataParameters();
    }
    if (map.restrictionList.has_value()) {
        printRestrictionList();
    }
#endif
}
#if 0
void SaeMapLogger::printIntersectionGeometryList(J2735::idl::IntersectionGeometryList &idlList, int ind)
{
    int i = 0;
    for (auto item : idlList) {
        *os_ << indents[ind] << "item: " <<  i << endl;
        ind++;
        if (item.name().has_value()) {
            *os_ << indents[ind] << "name: " <<  item.name().value() << endl;
        }
        printReferenceID<J2735::idl::intersectionReferenceID>(item.id(), ind);
        *os_ << indents[ind] << "revision: " <<  static_cast<int>(item.revision()) << endl;
        *os_ << indents[ind] << "refPoint:" << endl;
        printPosition3D(item.refPoint(), ind + 1);
        if (item.laneWidth().has_value()) {
            *os_ << indents[ind] << "laneWidth: " << item.laneWidth().value() << endl;
        }
        if (item.speedLimits().has_value()) {
            *os_ << indents[ind] << "speedLimits:" << endl;
            printSpeedLimitList(item.speedLimits().value(), ind+1);
        }
        *os_ << indents[ind] << "laneSet:" << endl;
        printLaneList(item.laneSet(), ind + 1);
        if (item.preemptPriorityData().has_value()) {
            *os_ << indents[ind] << "preemptPriorityData:" << endl;
        }
        i++;
        ind--;
    }
}
#endif
} //namespace SPLITS
