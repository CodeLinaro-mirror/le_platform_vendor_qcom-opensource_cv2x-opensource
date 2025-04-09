/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cmath>
#include "BsmMsgGenerator.hpp"
#include "vehicle/VehicleInterfaceLocal.hpp"
#include "vehicle/VehicleInterfaceIdl.hpp"
#include "utils.h"
#include "Logger.hpp"

using SPLITS::Vehicle::VehicleInterfaceLocal;
using SPLITS::Vehicle::VehicleInterfaceIdl;
namespace SPLITS {

namespace Core {

Status BsmMsgGenerator::init(bool useCan) {
    LOGD(__FUNCTION__, " useCan:", useCan);

    if (useCan) {
        vehicle_ = std::make_shared<VehicleInterfaceLocal>();
    } else {
        vehicle_ = std::make_shared<VehicleInterfaceIdl>();
    }
    if (vehicle_->init() != Status::SUCCESS) {
        LOGE(__FUNCTION__, " Failed to init vehicle data interface");
        return Status::FAILED;
    }
    if (vehicle_->getStaticVehicleParameters(vehicleParams_) != Status::SUCCESS) {
        LOGE(__FUNCTION__, " Failed to get static vehicle parameters");
        return Status::FAILED;
    }

    if (Status::SUCCESS != vehicle_->registerListener(getptr())
        or Status::SUCCESS != vehicle_->enableVehicleDynamicData() ) {
        LOGE(__FUNCTION__, " Failed to register for veh dynamic data");
        return Status::FAILED;
    }

    utils_ = std::make_shared<QUtils>();
    //utils_->initDiagLog();
    if (utils_->hwTRNGInt(randNumMsgCnt_)) {
        //LOG ERROR
        return Status::FAILED;
    }
    if (utils_->hwTRNGInt(randNumMsgId_)) {
        //LOG ERROR
        return Status::FAILED;
    }
    msgCount_ = randNumMsgCnt_ % 128;
    msgId_ = {randNumMsgId_ & 0xFF,
        (randNumMsgId_ >> 8 ) & 0xFF,
        (randNumMsgId_ >> 16) & 0xFF,
        (randNumMsgId_ >> 24) & 0xFF
    };

    if (utils_->hwTRNGData(tmpVehId_.data(), tmpVehId_.size())) {
        LOGE(__FUNCTION__, " get random veh id failed");
        return Status::FAILED;
    }

    criticalState_ = false;
    return Status::SUCCESS;
}

Status BsmMsgGenerator::fillBsm(std::shared_ptr<v2xMsgCsae> msg, bool idChanged,
    unsigned char* newTempId) {
    std::shared_ptr<Csae::idl::BasicSafetyMessage> sp = nullptr;
    if (msg == nullptr) {
        LOGE(__FUNCTION__, " no msg");
        return Status::INVALIDPARAM;
    }
    if ((sp = msg->BSM()) == nullptr) {
        sp = std::make_shared<Csae::idl::BasicSafetyMessage>();
        if (sp == nullptr) {
            LOGE(__FUNCTION__, " allocate BSM failed");
            return Status::NOMEMORY;
        } else {
            msg->BSM(sp);
        }
    }

    if (idChanged) {
        if(utils_){
            if (utils_->hwTRNGInt(randNumMsgCnt_)) {
                LOGE(__FUNCTION__, " get random number failed");
                return Status::FAILED;
            }
        }
        msgCount_ = randNumMsgCnt_ % 128;

        //tmp vehicle ID
        if (newTempId == nullptr) {
            LOGE(__FUNCTION__, " no temp ID");
            return Status::INVALIDPARAM;
        }

        for (int i = 0; i < tmpVehId_.size(); ++i) {
            tmpVehId_[i] = newTempId[i];
        }
    } else {
        msgCount_ = (msgCount_ + 1) % 128;
    }
    sp->msgCnt(msgCount_);
    sp->id(tmpVehId_);
    sp->secMark(timestamp_now() % 60000);

    // location data
    BsmLocationInfo locData;
    {
        std::lock_guard<std::mutex> lk(locationMutex_);
        locData = bsmLocation_;
    }

    // TODO: use c-shift pos data come from 3rd party
    sp->pos().lat(locData.lat);
    sp->pos().long_(locData.lon);

    J2735::idl::PositionalAccuracy posAccu;
    posAccu.semiMajor(locData.semiMajorAccu);
    posAccu.semiMinor(locData.semiMinorAccu);
    posAccu.orientation(locData.orientationAccu);
    sp->posAccuracy(std::move(posAccu));

    sp->speed(locData.speed);
    sp->heading(locData.heading);

    sp->accelSet().longitude(locData.longitudeAccel);
    sp->accelSet().lat(locData.latitudeAccel);
    sp->accelSet().vert(locData.vertAccel);
    sp->accelSet().yaw(locData.yaw);

    // dynamic vehicle data, come from local vehicle lib(CAN) or IVSS vehicle interface
    current_dynamic_vehicle_state_t dynmVehData;
    {
        std::lock_guard<std::mutex> lk(vehStateMutex_);
        dynmVehData = vehicleState_;
    }
    sp->transmission(static_cast<J2735::idl::TransmissionState>(dynmVehData.prndl));
    // the IDL angle type char is unsigned, must cast it to int8_t first
    int8_t angle = static_cast<int8_t>(dynmVehData.steering_wheel_angle);
    sp->angle(angle);
    Csae::idl::BrakeSystemStatus brake;
    if (!dynmVehData.brake_status.bits.unavailable) {
        J2735::idl::BrakeAppliedStatus wheelBrakes = 0;
        wheelBrakes |= dynmVehData.brake_status.bits.leftFront ? (1 << 1) : 0;
        wheelBrakes |= dynmVehData.brake_status.bits.leftRear ? ( 1 << 2) : 0;
        wheelBrakes |= dynmVehData.brake_status.bits.rightFront ? ( 1 << 3) : 0;
        wheelBrakes |= dynmVehData.brake_status.bits.rightRear ? ( 1 << 4) : 0;
        brake.wheelBrakes(std::move(wheelBrakes));
        brake.traction(std::move(static_cast<J2735::idl::TractionControlStatus>(
            dynmVehData.brake_status.bits.traction_control_status)));
        brake.abs(static_cast<J2735::idl::AntiLockBrakeStatus>(
            dynmVehData.brake_status.bits.antilock_brake_status));
        brake.scs(static_cast<J2735::idl::StabilityControlStatus>(
            dynmVehData.brake_status.bits.stability_control_status));
        brake.brakeBoost(static_cast<J2735::idl::BrakeBoostApplied>(
            dynmVehData.brake_status.bits.brake_boost_applied));
        brake.auxBrakes(static_cast<J2735::idl::AuxiliaryBrakeStatus>(
            dynmVehData.brake_status.bits.aux_brake_status));
    }
    sp->brakes(std::move(brake));

    //static vehicle data from client
    sp->size().width(vehicleParams_.vehicle_width_cm);
    sp->size().length(vehicleParams_.vehicle_length_cm);
    sp->size().height(vehicleParams_.vehicle_height_cm);
    //TODO: expand veh interface to include the two params
    sp->vehicleClass().classification(0);
    sp->vehicleClass().fuelType(0);

    // VehicleSafetyExtensions
    Csae::idl::VehicleSafetyExtensions safetyExt;
    {
        std::lock_guard<std::mutex> lk(eventMutex_);
        if (criticalState_) {
            safetyExt.events(eventFlags_);
        }
    }
    if (0 != dynmVehData.exterior_lights.data) {
        safetyExt.lights(static_cast<J2735::idl::ExteriorLights>(dynmVehData.exterior_lights.data));
    }
    // TODO: add PH and PD
    sp->safetyExt(std::move(safetyExt));

    // TODO: add VehicleEmergencyExtensions
    return Status::SUCCESS;
}

Status BsmMsgGenerator::fillBsm(std::shared_ptr<v2xMsgSae> msg, bool idChanged,
    unsigned char* newTempId) {
    std::shared_ptr<J2735::idl::BasicSafetyMessage> sp = nullptr;
    if (msg == nullptr) {
        return Status::INVALIDPARAM;
    }
    if ((sp = msg->BSM()) == nullptr) {
        sp = std::make_shared<J2735::idl::BasicSafetyMessage>();
        if (sp == nullptr) {
            return Status::NOMEMORY;
        } else {
            msg->BSM(sp);
        }
    }
    //TODO: call security object's API to check if idChanged
    if (idChanged) {
        if(utils_){
            if (utils_->hwTRNGInt(randNumMsgCnt_)) {
                LOGE(__FUNCTION__, " generate ID failed");
                return Status::FAILED;
            }
        }
        msgCount_ = randNumMsgCnt_ % 128;
        msgId_ = {(uint32_t)newTempId[0] << 24 |
                  (uint32_t)newTempId[1] << 16 |
                  (uint32_t)newTempId[2] << 8  |
                  (uint32_t)newTempId[3]};
        //TODO: get new 4 bytes ID from security object. and assign it to msgId_
    } else {
        msgCount_ = (msgCount_ + 1) % 128;
    }

    //BsmCoreData
    J2735::idl::BSMCoreData& coreData = sp->coreData();
    coreData.msgCnt(msgCount_);
    coreData.id(msgId_);
    coreData.secMark(timestamp_now() % 60000);

    //Location
    {
        std::lock_guard<std::mutex> lk(locationMutex_);
        coreData.lat(bsmLocation_.lat);
        coreData.longitude(bsmLocation_.lon);
        coreData.elev(bsmLocation_.elev);
        coreData.accuracy().semiMajor(bsmLocation_.semiMajorAccu);
        coreData.accuracy().semiMinor(bsmLocation_.semiMinorAccu);
        coreData.accuracy().orientation(bsmLocation_.orientationAccu);

        //speed can also come from vehicle high resolution motion data
        coreData.speed(bsmLocation_.speed);
        coreData.heading(bsmLocation_.heading);
        //acceleration can either come from vehicle high resolution motion data, or GNSS
        //Following code use GNSS
        coreData.accelSet().longitude(bsmLocation_.longitudeAccel);
        coreData.accelSet().lat(bsmLocation_.latitudeAccel);
        coreData.accelSet().vert(bsmLocation_.vertAccel);
        coreData.accelSet().yaw(bsmLocation_.yaw);
    }

    //Vehicle state, come from local vehicle lib(CAN) or IVSS vehicle interface
    {
        std::lock_guard<std::mutex> lk(vehStateMutex_);

        coreData.transmission(static_cast<J2735::idl::TransmissionState>(vehicleState_.prndl));
        coreData.angle(static_cast<int8_t>(vehicleState_.steering_wheel_angle));
        uint8_t brake = 0;
        if (!vehicleState_.brake_status.bits.unavailable) {
            brake |= vehicleState_.brake_status.bits.leftFront ? (1 << 1) : 0;
            brake |= vehicleState_.brake_status.bits.leftRear ? ( 1 << 2) : 0;
            brake |= vehicleState_.brake_status.bits.rightFront ? ( 1 << 3) : 0;
            brake |= vehicleState_.brake_status.bits.rightRear ? ( 1 << 4) : 0;
        }
        coreData.brakes().wheelBrakes(brake);
        coreData.brakes().traction(static_cast<J2735::idl::TractionControlStatus>(
                    vehicleState_.brake_status.bits.traction_control_status));
        coreData.brakes().abs((static_cast<J2735::idl::AntiLockBrakeStatus>(
                        vehicleState_.brake_status.bits.antilock_brake_status)));
        coreData.brakes().scs(static_cast<J2735::idl::StabilityControlStatus>(
                    vehicleState_.brake_status.bits.stability_control_status));
        coreData.brakes().brakeBoost(static_cast<J2735::idl::BrakeBoostApplied>(
                    vehicleState_.brake_status.bits.brake_boost_applied));
        coreData.brakes().auxBrakes(static_cast<J2735::idl::AuxilaryBrakesStatus>(
                    vehicleState_.brake_status.bits.aux_brake_status));
    }

    //static data: vehicle size
    //coreData.size().width(vehicleParams_.vehicle_width_cm);
    //missing heigh in VehicleSize in IDL file?
    //coreData.size().height(vehicleParams_.vehicle_height_cm);
    coreData.size().width(99.99);
    coreData.size().length(55.55);

    {
        std::lock_guard<std::mutex> lk(eventMutex_);
        if (criticalState_ == true) {
             fillVehicleSafetyExt(sp);
        }
    }
    //alternatively send safetyExt with AVDclass.
    std::vector<J2735::idl::PartIIcontent> idlPartIIlist;
    if (avdClass_ == false) {
        idlPartIIlist.clear();
        avdClass_ = true;
    } else {
        J2735::idl::VehicleSafetyExtensions safetyExt;
        safetyExt.avdClass(J2735::idl::AVDClass::AVDClass_lightAVD);
        J2735::idl::PartIIcontent idlPartII;
        idlPartII._d(0);
        idlPartII.safetyExt(std::move(safetyExt));
        idlPartIIlist.push_back(idlPartII);
        avdClass_ = false;
    }
    msg->BSM()->PartIIlist(std::move(idlPartIIlist));
    return Status::SUCCESS;
}

void BsmMsgGenerator::fillVehicleSafetyExt(std::shared_ptr<J2735::idl::BasicSafetyMessage> bsm) {
    J2735::idl::VehicleSafetyExtensions safetyExt;
    safetyExt.events(eventFlags_);
    //no pathHistory
    //no pathPrediction
    //no lights
    J2735::idl::PartIIcontent idlPartII;
    idlPartII._d(0);
    idlPartII.safetyExt(std::move(safetyExt));
    std::vector<J2735::idl::PartIIcontent> idlPartIIlist;
    idlPartIIlist.push_back(idlPartII);
    bsm->PartIIlist(std::move(idlPartIIlist));
}
void BsmMsgGenerator::onHighResMotionDataChanged(const high_resolution_motion_t &highResMotion) {
    highResMotion_ = highResMotion;
}
void BsmMsgGenerator::onVehicleDataChanged(const current_dynamic_vehicle_state_t &vehicleState) {
    std::lock_guard<std::mutex> lk(vehStateMutex_);
    vehicleState_ = vehicleState;
}
void BsmMsgGenerator::onVehicleEventChanged(const uint16_t &eventFlags) {
    LOGD(__FUNCTION__, " eventFlags:", eventFlags, ", cur msg count:", msgCount_);

    std::lock_guard<std::mutex> lk(eventMutex_);
    if (eventFlags) {
        //critical event occured
        criticalState_ = true;
        eventFlags_ = eventFlags;
    } else {
        criticalState_ = false;
        eventFlags_ = 0;
    }
}
void  BsmMsgGenerator::onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo) {
    std::lock_guard<std::mutex> lk(locationMutex_);
    bsmLocation_.lat = static_cast<int32_t>(locationInfo->getLatitude() * 10000000);
    bsmLocation_.lon = static_cast<int32_t>(locationInfo->getLongitude() * 10000000);
    bsmLocation_.elev = static_cast<int32_t>(locationInfo->getAltitude() * 10);

    auto semiMajorAccu = locationInfo->getHorizontalUncertaintySemiMajor();
    if (std::isnan(semiMajorAccu)) {
        bsmLocation_.semiMajorAccu = 255;
    } else {
        bsmLocation_.semiMajorAccu = static_cast<uint8_t>(semiMajorAccu * 20);
    }
    auto semiMinorAccu = locationInfo->getHorizontalUncertaintySemiMinor();
    if (std::isnan(semiMinorAccu)) {
        bsmLocation_.semiMinorAccu = 255;
    } else {
        bsmLocation_.semiMinorAccu = static_cast<uint8_t>(semiMinorAccu * 20);
    }
    auto orientationAccu = locationInfo->getHorizontalUncertaintyAzimuth();
    if (std::isnan(orientationAccu)) {
        bsmLocation_.orientationAccu = 65535;
    } else {
        bsmLocation_.orientationAccu = static_cast<uint16_t>(orientationAccu/0.0054932479);
    }
    //speed can also come from vehicle high resolution motion data
    bsmLocation_.speed = static_cast<uint16_t>(locationInfo->getSpeed() * 50);
    bsmLocation_.heading = static_cast<uint16_t>(locationInfo->getHeading()/0.0125);
    //acceleration can come from vehicle too
    bsmLocation_.longitudeAccel = static_cast<int16_t>(100 * locationInfo->getBodyFrameData().latAccel);
    bsmLocation_.latitudeAccel = static_cast<int16_t>(100 * locationInfo->getBodyFrameData().longAccel);
    bsmLocation_.vertAccel = static_cast<char>(50* locationInfo->getBodyFrameData().latAccel);
    bsmLocation_.yaw = static_cast<int16_t>(100 * locationInfo->getBodyFrameData().yawRate);
}
} //namespace Core
} //namespace SPLITS
