/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "SaeBsmLogger.hpp"

namespace SPLITS {

void SaeBsmLogger::logMsg(std::shared_ptr<v2xMsgSae>msg)
{
    logMsg(*(msg->BSM()));
}
void SaeBsmLogger::logMsg(const J2735::idl::BasicSafetyMessage &idlBsm)
{
    int ind = 0;

    *os_ << indents[ind] << "BSMCoreData: " << endl;
    printBSMcoreData(idlBsm.coreData(), ind + 1);
    if (idlBsm.PartIIlist().has_value()) {
        *os_ << indents[ind] << "PartII:" << endl;
        for (auto idlPartII : idlBsm.PartIIlist().value()) {
            switch(idlPartII._d()) {
                case 0:
                    *os_ << indents[ind] << "vehicleSafetyExt: " << endl;
                    printVehicleSafetyExt(idlPartII.safetyExt(), ind + 1); 
                    break;
                case 1:
                    *os_ << indents[ind] << "specialVehicleExt: " << endl;
                    printSpecialVehicleExt(idlPartII.specialVehicleExt(), ind + 1);
                    break;
                case 2:
                    *os_ << indents[ind] << "Supplemental: " << endl;
                    printSupplementalVehicleExt(idlPartII.supplementVehicleExt(), ind + 1);
                    break;
                default:
                    *os_ << indents[ind] << "unknown partII ext! " << endl;
            }
        }
    }
}

void SaeBsmLogger::printVehicleSafetyExt(const J2735::idl::VehicleSafetyExtensions & idlSafetyExt,
        int ind)
{
    *os_ << indents[ind] << "events: " << (idlSafetyExt.events().has_value() ? 
        std::to_string(idlSafetyExt.events().value()) : "n/a") << endl;;

    *os_ << indents[ind] << "pathHistory: ";
    if (idlSafetyExt.pathHist().has_value()) {
        *os_ << endl;
        printPathHistory(idlSafetyExt.pathHist().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
    *os_ << indents[ind] << "pathPrediction: ";
    if (idlSafetyExt.pathPredict().has_value()) {
        *os_ << endl;
        printPathPrediction(idlSafetyExt.pathPredict().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
    *os_ << indents[ind] << "lights: ";
    if (idlSafetyExt.lights().has_value()) {
        *os_ << endl;
        printExteriorLights(idlSafetyExt.lights().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
}
void SaeBsmLogger::printSpecialVehicleExt(const J2735::idl::SpecialVehicleExtensions & idlSVE,
        int ind)
{
    *os_ << indents[ind] << "vehicleAlert: ";
    if (idlSVE.vehicleAlerts().has_value()) {
        *os_ << endl;
        printEmergencyDetails(idlSVE.vehicleAlerts().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
    *os_ << indents[ind] << "eventDescription: ";
    if (idlSVE.description().has_value()) {
        *os_ << endl;
        printEventDescription(idlSVE.description().value(), ind + 1);
    }
}
void SaeBsmLogger::printEventDescription(const J2735::idl::EventDescription &idlEvent, int ind) {
    *os_ << indents[ind] << "typeEvent: " << idlEvent.typeEvent() << endl;
    *os_ << indents[ind] << "desc: ";
    if (idlEvent.description().has_value()) {
        for (auto e : idlEvent.description().value()) {
            *os_ << e << ",";
        }
        *os_ << endl;
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "priority: " << (idlEvent.priority().has_value() ? 
        std::to_string(idlEvent.priority().value()) : "n/a") << endl;
    *os_ << indents[ind] << "heading: " << (idlEvent.heading().has_value() ? 
        std::to_string(idlEvent.heading().value()) : "n/a") << endl;
    *os_ << indents[ind] << "extent: " << (idlEvent.extent().has_value() ? 
        std::to_string(idlEvent.extent().value()) : "n/a") << endl;
}
void SaeBsmLogger::printSupplementalVehicleExt(const J2735::idl::SupplementalVehicleExtensions &idlSuppExt,
        int ind)
{
    *os_ << indents[ind] << "classification: " << (idlSuppExt.classification().has_value() ? 
        std::to_string(idlSuppExt.classification().value()) : "n/a") << endl;
    *os_ << indents[ind] << "classDetails: ";
    if (idlSuppExt.classDetails().has_value()) {
        *os_ << endl;
        printVehicleClassification(idlSuppExt.classDetails().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }
    *os_ << indents[ind] << "vehicleData: ";
    if (idlSuppExt.vehicleData_().has_value()){
        *os_ << endl;
        printVehicleData(idlSuppExt.vehicleData_().value(), ind + 1);
    }
    *os_ << indents[ind] << "status: ";
    if (idlSuppExt.status().has_value()) {
        printDisabledVehicle(idlSuppExt.status().value(), ind + 1);
    }
}
void SaeBsmLogger::printVehicleData(const J2735::idl::VehicleData &idlVehicleData, int ind)
{
    *os_ << indents[ind] << "height: " << (idlVehicleData.height().has_value() ? 
        std::to_string(idlVehicleData.height().value()) : "n/a") << endl;
    *os_ << indents[ind] << "bumpers: ";
    if (idlVehicleData.bumpers().has_value()) {
        *os_ << idlVehicleData.bumpers().value().front() << "," << 
        idlVehicleData.bumpers().value().rear() << endl;
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "mass: " << (idlVehicleData.mass().has_value() ? 
        std::to_string(idlVehicleData.mass().value()) : "n/a") << endl;
    *os_ << indents[ind] << "axles: ";
    if (idlVehicleData.axles_().has_value()) {
        *os_ << endl;
        printAxles(idlVehicleData.axles_().value(), ind + 1);
    } else {
        *os_ << "n/a" << endl;
    }

    *os_ << indents[ind] << "rollAngle: " << (idlVehicleData.rollAngle().has_value() ? 
        std::to_string(idlVehicleData.rollAngle().value()) : "n/a") << endl;
}
void SaeBsmLogger::printDisabledVehicle(const J2735::idl::DisabledVehicle &idlDisabledVehicle,
        int ind)
{
    *os_ << "statusDetails:" << idlDisabledVehicle.statusDetails() << endl;
    *os_ << "locationDetails: " << (idlDisabledVehicle.locationDetails().has_value() ?
        std::to_string(idlDisabledVehicle.locationDetails().value()) : "n/a");
}

} //namespace SPLITS
