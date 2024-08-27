/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
 /**
  * @file: VehicleInterfaceIdl.hpp
  *
  * @brief: Public API and definitions of the VehicleData interface. It uses
  * the vsomeip, get the data from remote EAP.
  *
  */
#ifndef __VEHICLE_INTERFACE_IDL_HPP__
#define __VEHICLE_INTERFACE_IDL_HPP__

#include "VehicleInterface.hpp"
#include "v1/commonapi/SPLITS/VehicleDataProxy.hpp"

namespace capisplits = v1::commonapi::SPLITS;

namespace SPLITS {
namespace Vehicle {

class VehicleInterfaceIdl : public VehicleInterface {
public:
    VehicleInterfaceIdl(){};
    virtual ~VehicleInterfaceIdl(){};
    Status init(void) override;
    Status enableVehicleDynamicData(void) override;
    Status disableVehicleDynamicData(void) override;
    Status enableHighResMotionData(void) override;
    Status disableHighResMotionData(void) override;
    Status getStaticVehicleParameters(static_vehicle_parameters_t &params) override;

private:
    void capi2lteluxVehicleState(current_dynamic_vehicle_state_t &teluxDVS,
        const capisplits::VehicleData::dynamicVehicleState &capiDVS);

    void capi2teluxHighResMotion(high_resolution_motion_t &teluxHRM,
        const capisplits::VehicleData::highResolutionMotion &capiHRM);
    void capi2teluxStaticParams(static_vehicle_parameters_t &teluxSVP,
        const capisplits::VehicleData::staticVehicleParameters &idlSVP);

    std::shared_ptr<capisplits::VehicleDataProxy<>> proxy_ = nullptr;
    uint32_t dynamicDataSub_;
    uint32_t highResolutionMotionSub_;
};
} //namespace Vehicle
} //namespace SPLITS
#endif //__VEHICLE_INTERFACE_IDL_HPP__
