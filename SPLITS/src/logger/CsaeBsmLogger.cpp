/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "CsaeBsmLogger.hpp"

namespace SPLITS {

void CsaeBsmLogger::logMsg(std::shared_ptr<v2xMsgCsae>msg) {
    logMsg(*(msg->BSM()));
}
void CsaeBsmLogger::logMsg(const Csae::idl::BasicSafetyMessage &idlBsm) {
    int ind = 0;

    *os_ << indents[ind] << "BSM: " << endl;
    ++ind;
    *os_ << indents[ind] << "msgCnt: " << static_cast<int>(idlBsm.msgCnt()) << endl;
    CsaeBsmLogger::printVehicleId(idlBsm.id(), ind);
    *os_ << indents[ind] << "secMark: " << idlBsm.secMark() << endl;
    *os_ << indents[ind] << "timeConfidence:" << (idlBsm.timeConfidence().has_value() ?
        std::to_string(idlBsm.timeConfidence().value()) : "n/a") << endl;
    printPosition3D(idlBsm.pos(), ind + 1);

    *os_ << indents[ind] << "PosAccuracy: ";
    if (idlBsm.posAccuracy().has_value()) {
        *os_ << endl;
        printPositionalAccuracy(idlBsm.posAccuracy().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "PosConfidence: ";
    if (idlBsm.posConfidence().has_value()) {
        *os_ << endl;
        printPositionConfidenceSet(idlBsm.posConfidence().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "TransmissionState:" <<
        static_cast<int>(idlBsm.transmission()) << endl;
    *os_ << indents[ind] << "Speed:" << static_cast<int>(idlBsm.speed()) << endl;
    *os_ << indents[ind] << "Heading:" << static_cast<int>(idlBsm.heading()) << endl;
    *os_ << indents[ind] << "SteeringWheelAngle:" << (idlBsm.angle().has_value() ?
        std::to_string(static_cast<int8_t>(idlBsm.angle().value())) : "n/a") << endl;
    *os_ << indents[ind] << "MotionConfidenceSet: ";
    if (idlBsm.motionCfd().has_value()) {
        *os_ << endl;
        printMotionConfidenceSet(idlBsm.motionCfd().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "AccelSet4Way: " << endl;
    printAccelerationSet4Way(idlBsm.accelSet(), ind + 1);

    *os_ << indents[ind] << "BrakeSystemStatus: " << endl;
    printBrakeSystemStatus(idlBsm.brakes(), ind + 1);

    *os_ << indents[ind] << "VehicleSize: " << endl;
    printVehicleSize(idlBsm.size(), ind + 1);

    *os_ << indents[ind] << "VehicleClassification: " << endl;
    printVehicleClass(idlBsm.vehicleClass(), ind + 1);

    *os_ << indents[ind] << "VehicleSafetyExtensions: ";
    if (idlBsm.safetyExt().has_value()) {
        *os_ << endl;
        printVehicleSafetyExtensions(idlBsm.safetyExt().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "VehicleEmergencyExtensions: ";
    if (idlBsm.emergencyExt().has_value()) {
        *os_ << endl;
        printVehicleEmergencyExtensions(idlBsm.emergencyExt().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
}
void CsaeBsmLogger::printVehicleId(const std::array<uint8_t, 8> &idlId, int ind) {
    std::ios state(NULL);
    state.copyfmt(*os_);
    *os_ << indents[ind] << "id: " << std::hex << setfill('0');
    for (int i = 0; i < idlId.size(); ++i) {
        *os_ << std::setw(2) << static_cast<int>(idlId[i]) << " ";
    }
    *os_ << std::endl;
    (*os_).copyfmt(state);
}
void CsaeBsmLogger::printBrakeSystemStatus(const Csae::idl::BrakeSystemStatus &idlBrakes, int ind) {
    *os_ << indents[ind] << "BrakePedalStatus:" << (idlBrakes.brakePadel().has_value() ?
        std::to_string(idlBrakes.brakePadel().value()) : "n/a") << endl;
    *os_ << indents[ind] << "BrakeAppliedStatus:" << (idlBrakes.wheelBrakes().has_value() ?
        std::to_string(idlBrakes.wheelBrakes().value()) : "n/a") << endl;
    *os_ << indents[ind] << "TractionControlStatus:" << (idlBrakes.traction().has_value() ?
        std::to_string(idlBrakes.traction().value()) : "n/a") << endl;
    *os_ << indents[ind] << "AntiLockBrakeStatus:" << (idlBrakes.abs().has_value() ?
        std::to_string(idlBrakes.abs().value()) : "n/a") << endl;
    *os_ << indents[ind] << "StabilityControlStatus:" << (idlBrakes.scs().has_value() ?
        std::to_string(idlBrakes.scs().value()) : "n/a") << endl;
    *os_ << indents[ind] << "BrakeBoostApplied:" << (idlBrakes.brakeBoost().has_value() ?
        std::to_string(idlBrakes.brakeBoost().value()) : "n/a") << endl;
    *os_ << indents[ind] << "AuxiliaryBrakeStatus:" << (idlBrakes.auxBrakes().has_value() ?
        std::to_string(idlBrakes.auxBrakes().value()) : "n/a") << endl;
}
void CsaeBsmLogger::printVehicleSize(const Csae::idl::VehicleSize &idlVehSize, int ind) {
    *os_ << indents[ind] << "VehicleWidth:" <<
        std::to_string(idlVehSize.width()) << endl;
    *os_ << indents[ind] << "VehicleLength:" <<
        std::to_string(idlVehSize.length()) << endl;
    *os_ << indents[ind] << "VehicleHeight:" << (idlVehSize.height().has_value() ?
        std::to_string(idlVehSize.height().value()) : "n/a") << endl;
}
void CsaeBsmLogger::printVehicleClass(
    const Csae::idl::VehicleClassification &idlVehClass, int ind) {
    *os_ << indents[ind] << "BasicVehicleClass:" <<
        std::to_string(idlVehClass.classification()) << endl;
    *os_ << indents[ind] << "FuelType:" << (idlVehClass.fuelType().has_value() ?
        std::to_string(idlVehClass.fuelType().value()) : "n/a") << endl;
}
void CsaeBsmLogger::printVehicleSafetyExtensions(
    const Csae::idl::VehicleSafetyExtensions &idlSafetyExt, int ind) {
    *os_ << indents[ind] << "VehicleEventFlags:" << (idlSafetyExt.events().has_value() ?
        std::to_string(idlSafetyExt.events().value()) : "n/a") << endl;

    *os_ << indents[ind] << "PathHistory: ";
    if (idlSafetyExt.pathHistory().has_value()) {
        *os_ << endl;
        CsaeCommonLogger::printPathHistory(idlSafetyExt.pathHistory().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "PathPrediction: ";
    if (idlSafetyExt.pathPrediction().has_value()) {
        *os_ << endl;
        printPathPrediction(idlSafetyExt.pathPrediction().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "ExteriorLights: ";
    if (idlSafetyExt.lights().has_value()) {
        *os_ << endl;
        printExteriorLights(idlSafetyExt.lights().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
}
void CsaeBsmLogger::printVehicleEmergencyExtensions(
    const Csae::idl::VehicleEmergencyExtensions &idlEmgExt, int ind) {
    *os_ << indents[ind] << "ResponseType:" << (idlEmgExt.responseType().has_value() ?
        std::to_string(idlEmgExt.responseType().value()) : "n/a") << endl;
    *os_ << indents[ind] << "SirenInUse:" << (idlEmgExt.sirenUse().has_value() ?
        std::to_string(idlEmgExt.sirenUse().value()) : "n/a") << endl;
    *os_ << indents[ind] << "LightbarInUse:" << (idlEmgExt.lightsUse().has_value() ?
        std::to_string(idlEmgExt.lightsUse().value()) : "n/a") << endl;
}
} //namespace SPLITS
