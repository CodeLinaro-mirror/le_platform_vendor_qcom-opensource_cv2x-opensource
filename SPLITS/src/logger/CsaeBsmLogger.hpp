/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CSAE_BSM_LOGGER_HPP__
#define __CSAE_BSM_LOGGER_HPP__

#include "CsaeCommonLogger.hpp"
#include "BSM.h"

namespace SPLITS {

class CsaeBsmLogger : public CsaeCommonLogger {
public:
    CsaeBsmLogger(ostream *os) : CsaeCommonLogger(os) {}
    virtual ~CsaeBsmLogger(){}

    void logMsg(std::shared_ptr<v2xMsgCsae>msg) override;
    void logMsg(const Csae::idl::BasicSafetyMessage &msg) override;

private:

    void printVehicleId(const std::array<uint8_t, 8> &idlId, int ind);
    void printBrakeSystemStatus(const Csae::idl::BrakeSystemStatus &idlBrakes, int ind);
    void printVehicleSize(const Csae::idl::VehicleSize &idlVehSize, int ind);
    void printVehicleClass(const Csae::idl::VehicleClassification &idlVehClass, int ind);
    void printVehicleSafetyExtensions(
        const Csae::idl::VehicleSafetyExtensions &idlSafetyExt, int ind);
    void printVehicleEmergencyExtensions(
        const Csae::idl::VehicleEmergencyExtensions &idlEmgExt, int ind);
};
} //namespace SPLITS
#endif //__CSAE_BSM_LOGGER_HPP__S
