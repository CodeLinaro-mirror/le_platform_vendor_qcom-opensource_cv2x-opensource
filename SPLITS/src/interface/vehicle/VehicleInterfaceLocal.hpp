/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
 /**
  * @file: VehicleInterfaceLocal.hpp
  *
  * @brief: Public API and definitions of the VehicleData interface. It uses
  * the v2x vehicle library
  *
  */
#ifndef __VEHICLE_INTERFACE_LOCAL_HPP__
#define __VEHICLE_INTERFACE_LOCAL_HPP__

#include "VehicleInterface.hpp"
#include <telux/cv2x/legacy/v2x_vehicle_api.h>

using telux::common::Status;
#define V2X_DATA_HANDLE_BAD -1

namespace SPLITS {
namespace Vehicle {

class VehicleInterfaceLocal : public VehicleInterface {
public:
    VehicleInterfaceLocal(){};
    virtual ~VehicleInterfaceLocal(){};
    Status init() {
        return Status::SUCCESS;
    };
    Status enableVehicleDynamicData(void) override;
    Status disableVehicleDynamicData(void) override;
    Status enableHighResMotionData(void) override;
    Status disableHighResMotionData(void) override;
    Status getStaticVehicleParameters(static_vehicle_parameters_t &params) override;

private:
    v2x_vehicle_handle_t vehicleDynamicHandle_ = V2X_VDATA_HANDLE_BAD;
    v2x_motion_data_handle_t motionDataHandle_ = V2X_VDATA_HANDLE_BAD;
    static void onVehicleDynamicDataChanged(current_dynamic_vehicle_state_t* vehicleData, void* context);
    static void onHighResMotionDataChanged(high_resolution_motion_t *motionData);
};
} //namespace Vehicle
} //namespace SPLITS
#endif
