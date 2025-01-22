/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SAE_BSM_LOGGER_HPP__
#define __SAE_BSM_LOGGER_HPP__

#include "SaeCommonLogger.hpp"
#include "J2735_BasicSafetyMessage.h"

namespace SPLITS {

class SaeBsmLogger : public SaeCommonLogger {
public:
    SaeBsmLogger(ostream *os) : SaeCommonLogger(os) {
    }
    virtual ~SaeBsmLogger(){}

    void logMsg(std::shared_ptr<v2xMsgSae>msg) override;
    void logMsg(const J2735::idl::BasicSafetyMessage &msg) override;

private:
    void printVehicleSafetyExt(const J2735::idl::VehicleSafetyExtensions & idlSafetyExt, int ind);
    void printSpecialVehicleExt(const J2735::idl::SpecialVehicleExtensions & idlSVE,
        int ind);
    void printEventDescription(const J2735::idl::EventDescription &idlEvent, int ind);
    void printSupplementalVehicleExt(const J2735::idl::SupplementalVehicleExtensions &idlSuppExt,
            int ind);
    void printVehicleData(const J2735::idl::VehicleData &idlVehicleData, int ind);
    void printDisabledVehicle(const J2735::idl::DisabledVehicle &idlDisabledVehicle, int ind);
};
} //namespace SPLITS
#endif //__SAE_BSM_LOGGER_HPP__
