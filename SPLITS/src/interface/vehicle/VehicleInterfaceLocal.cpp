/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "VehicleInterfaceLocal.hpp"
#include "J2735_Common.h"
#include <functional>

using J2735::idl::VehicleEventFlagsBits;

namespace SPLITS {
namespace Vehicle {

Status VehicleInterfaceLocal::enableVehicleDynamicData(void)
{
    vehicleDynamicHandle_ = v2x_vehicle_register_listener(
            VehicleInterfaceLocal::onVehicleDynamicDataChanged, this);
    if (vehicleDynamicHandle_ == V2X_DATA_HANDLE_BAD) {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}
Status VehicleInterfaceLocal::disableVehicleDynamicData(void)
{
    if (vehicleDynamicHandle_ == V2X_VDATA_HANDLE_BAD) {
        return Status::INVALIDPARAM;
    }
    if (v2x_vehicle_deregister_for_callback(vehicleDynamicHandle_)) {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}
Status VehicleInterfaceLocal::enableHighResMotionData(void)
{
    motionDataHandle_ = v2x_high_res_motion_register_listener(
            VehicleInterfaceLocal::onHighResMotionDataChanged);
    if (motionDataHandle_ == V2X_DATA_HANDLE_BAD) {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status VehicleInterfaceLocal::disableHighResMotionData(void)
{
    if (motionDataHandle_ == V2X_VDATA_HANDLE_BAD) {
        return Status::FAILED;
    }
    if (v2x_vehicle_deregister_for_callback(motionDataHandle_)) {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}
void VehicleInterfaceLocal::onVehicleDynamicDataChanged(current_dynamic_vehicle_state_t *vehicleData,
                                          void *context)
{
    if (context == nullptr) {
        std::cout << "onVehicleEventChanged: null ctx" << std::endl;
    }
    if (vehicleData == nullptr) {
        std::cout << "onVehicleEventChanged: vehicleData nullptr" << std::endl;
    }
    VehicleInterfaceLocal * veh = static_cast<VehicleInterfaceLocal*>(context);

    auto critical = veh->eventFlags_;

    vehicleData->events.bits.eventAirBagDeployment ? veh->eventFlags_ |= VehicleEventFlagsBits::eventAirBagDeployment :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventAirBagDeployment);
    vehicleData->events.bits.eventDisabledVehicle ? veh->eventFlags_ |=  VehicleEventFlagsBits::eventDisabledVehicle :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventDisabledVehicle);
    vehicleData->events.bits.eventFlatTire ? veh->eventFlags_ |= VehicleEventFlagsBits::eventFlatTire :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventFlatTire);
    vehicleData->events.bits.eventHardBraking ? veh->eventFlags_ |= VehicleEventFlagsBits::eventHardBraking :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventHardBraking);
    vehicleData->events.bits.eventStabilityControlactivated ?
        veh->eventFlags_ |= VehicleEventFlagsBits::eventStabilityControlactivated :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventStabilityControlactivated);
    vehicleData->events.bits.eventTractionControlLoss ? veh->eventFlags_ |= VehicleEventFlagsBits::eventTractionControlLoss :
        veh->eventFlags_ &=(~VehicleEventFlagsBits::eventTractionControlLoss);
    vehicleData->events.bits.eventABSactivated ? veh->eventFlags_ |= VehicleEventFlagsBits::eventABSactivated :
        veh->eventFlags_ &= (~VehicleEventFlagsBits::eventABSactivated);

    std::lock_guard<std::mutex> lock(veh->listenerMutex_);
    for (auto it = veh->listeners_.begin() ; it != veh->listeners_.end();) {
        auto sp = (*it).lock();
        if (sp) {
            if (critical && veh->eventFlags_ == 0) {
                //critical event cleared
                sp->onVehicleEventChanged(veh->eventFlags_);
            }else if (veh->eventFlags_) {
                //critical event appeared
                sp->onVehicleEventChanged(veh->eventFlags_);
            } else {
                sp->onVehicleDataChanged(*vehicleData);
            }
        }
    }
}
void VehicleInterfaceLocal::onHighResMotionDataChanged(high_resolution_motion_t *motionData)
{
#if 0
    VehicleInterfaceLocal * veh = static_cast<VehicleInterfaceLocal*>(context);

    for (auto it = veh->listeners_.begin() ; it != veh->listeners_.end();) {
        auto sp = (*it).lock();
        if (sp) {
            sp->onHighResMotionDataChanged(*motionData);
        }
    }
#endif
}
Status VehicleInterfaceLocal::getStaticVehicleParameters(static_vehicle_parameters_t &params)
{
    static_vehicle_parameters_t sparams;
#if 0
    if (v2x_vehicle_get_static_params(&sparams) != V2X_STATUS_SUCCESS) {
        return Status::FAILED;
    }
    params = sparams;
#endif
    return Status::SUCCESS;
}
} //namespace Vehicle
} //namespace SPLITS
