/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __VEHICLE_DATA_STUBIMPL_HPP__
#define __VEHICLE_DATA_STUBIMPL_HPP__

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/SPLITS/VehicleDataStubDefault.hpp>

using v1::commonapi::SPLITS::VehicleDataStubDefault;
using v1::commonapi::SPLITS::VehicleData;

class VehicleDataStubImpl : public VehicleDataStubDefault {
public:
    VehicleDataStubImpl(VehicleData::staticVehicleParameters vehicleParams){
        vehicleParams_ = vehicleParams;
    }
    virtual ~VehicleDataStubImpl(){}

    void onVehicleStateChanged(const VehicleData::dynamicVehicleState &state) {
        fireDynamicVehicleStateEvent(state);
    }
    void getStaticVehicleParameters(const std::shared_ptr<CommonAPI::ClientId> _client,
            getStaticVehicleParametersReply_t _reply) {
        (void)_client;
        _reply(v1::commonapi::telux::common::CommonDefines::ErrorCode::SUCCESS, vehicleParams_);
    }
private:
    VehicleData::staticVehicleParameters vehicleParams_;
};


#endif //__VEHICLE_DATA_STUBIMPL_HPP__
