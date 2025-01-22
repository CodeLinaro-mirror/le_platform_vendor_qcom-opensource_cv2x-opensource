/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <cstring>
#include <thread>
#include <CommonAPI/CommonAPI.hpp>
#include "VehicleInterfaceIdl.hpp"
#include "v1/commonapi/telux/common/CommonDefines.hpp"
using v1::commonapi::telux::common::CommonDefines;

namespace SPLITS {
namespace Vehicle {

#define PROXY_TIMEOUT    60 //seconds

Status VehicleInterfaceIdl::init(void)
{
    int count = 0;
    CommonAPI::Runtime::setProperty("LogContext", "SplitsService");
    CommonAPI::Runtime::setProperty("LogApplication", "SplitsService");
    CommonAPI::Runtime::setProperty("LibraryBase", "SplitsService");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.VehicleData";
    std::string connection = "SPLITS";

    proxy_ =
        runtime->buildProxy<capisplits::VehicleDataProxy>(domain, instance, connection);

    while(!proxy_->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
        if (count == PROXY_TIMEOUT)
            break;
        else
            count++;
    }
    if (count < PROXY_TIMEOUT) {
        COMMONAPI_INFO("VehicleData proxy registration succesful");
    } else {
        COMMONAPI_ERROR("VehicleData Proxy not available");
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status VehicleInterfaceIdl::enableVehicleDynamicData(void)
{
    dynamicDataSub_ = proxy_->getDynamicVehicleStateEvent().subscribe([&](const capisplits::VehicleData::dynamicVehicleState &state) {
            current_dynamic_vehicle_state_t teluxDVS;
            capi2lteluxVehicleState(teluxDVS, state);
            auto critical = eventFlags_;
            eventFlags_ = state.getEvents();

            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    if (critical && eventFlags_ == 0) {
                        //critical event cleared.
                        sp->onVehicleEventChanged(eventFlags_);
                    } else if (eventFlags_) {
                        //critical event appeared
                        sp->onVehicleEventChanged(eventFlags_);
                    }  else {
                        sp->onVehicleDataChanged(teluxDVS);
                    }
                }
            }
    });
}
Status VehicleInterfaceIdl::enableHighResMotionData(void)
{
    highResolutionMotionSub_ = proxy_->getHighResolutionMotionEvent().subscribe([&](const capisplits::VehicleData::highResolutionMotion &motion) {
            high_resolution_motion_t teluxHRM;
            capi2teluxHighResMotion(teluxHRM, motion);
            for (auto listener : listeners_) {
                auto sp = listener.lock();
                if (sp) {
                    sp->onHighResMotionDataChanged(teluxHRM);
                }
            }
    });
    return Status::SUCCESS;
}
Status VehicleInterfaceIdl::disableVehicleDynamicData(void)
{
    if (proxy_) {
        proxy_->getDynamicVehicleStateEvent().unsubscribe(dynamicDataSub_);
    }
    return Status::SUCCESS;
}
Status VehicleInterfaceIdl::disableHighResMotionData(void)
{
    if (proxy_) {
        proxy_->getHighResolutionMotionEvent().unsubscribe(highResolutionMotionSub_);
    }
    return Status::SUCCESS;
}
Status VehicleInterfaceIdl::getStaticVehicleParameters(static_vehicle_parameters_t &params)
{
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    Status status = Status::FAILED;
    CommonDefines::ErrorCode error;
    capisplits::VehicleData::staticVehicleParameters idlParams;

    if (!proxy_->isAvailable()) {
        return Status::NOTREADY;
    }
    //sync call
    COMMONAPI_INFO("calling RPC");
    proxy_->getStaticVehicleParameters(callStatus, error, idlParams);
    COMMONAPI_INFO("returned from calling RPC");
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        return Status::FAILED;
    } else {
    COMMONAPI_INFO("convert");
        capi2teluxStaticParams(params, idlParams);
    }
    COMMONAPI_INFO("return success");
    return Status::SUCCESS;

}
void VehicleInterfaceIdl::capi2lteluxVehicleState(current_dynamic_vehicle_state_t &teluxDVS,
        const capisplits::VehicleData::dynamicVehicleState &capiDVS)
{
    teluxDVS.prndl = static_cast<v2x_transmission_state_enum_type>(capiDVS.getTransmission().value_);
    teluxDVS.events.data = capiDVS.getEvents();
    teluxDVS.throttle_position = static_cast<double>(capiDVS.getThrottlePosition());
    teluxDVS.throttle_confidence = static_cast<double>(capiDVS.getThrottleConfidenc());
    teluxDVS.steering_wheel_angle = static_cast<double>(capiDVS.getSteeringWheelAngle());
    teluxDVS.brake_status.word = capiDVS.getBrakeStatus();
    teluxDVS.exterior_lights.data = capiDVS.getLights();
    teluxDVS.front_wiper_status = capiDVS.getFrontWiperStatus();
    teluxDVS.rear_wiper_status = capiDVS.getRearWiperStatus();
}
void VehicleInterfaceIdl::capi2teluxHighResMotion(high_resolution_motion_t &teluxHRM,
        const capisplits::VehicleData::highResolutionMotion &capiHRM)
{
    teluxHRM.vehicle_speed = capiHRM.getSpeed();
    teluxHRM.longitudinal_acceleration = capiHRM.getLongitudinalAcceleration();
    teluxHRM.yaw_rate = capiHRM.getYawRate();
}

void VehicleInterfaceIdl::capi2teluxStaticParams(static_vehicle_parameters_t &teluxSVP,
        const capisplits::VehicleData::staticVehicleParameters &idlSVP)
{
    teluxSVP.vehicle_height_cm = (double)idlSVP.getVehicle_height_cm();
    COMMONAPI_INFO("1");
    teluxSVP.vehicle_width_cm = (double)idlSVP.getVehicle_width_cm();
    COMMONAPI_INFO("1");
    teluxSVP.vehicle_length_cm = (double)idlSVP.getVehicle_length_cm();
    COMMONAPI_INFO("1");
    teluxSVP.front_bumper_height_cm = (double)idlSVP.getFront_bumper_height_cm();
    COMMONAPI_INFO("1");
    teluxSVP.rear_bumper_height_cm = (double)idlSVP.getRear_bumper_height_cm();
    COMMONAPI_INFO("1");
    teluxSVP.vehicle_mass_kg = (double)idlSVP.getVehicle_mass_kg();
    COMMONAPI_INFO("1");
    teluxSVP.trailer_weight_kg = (double)idlSVP.getTrailer_weight_kg();
    COMMONAPI_INFO("1");
    size_t strSize = idlSVP.getMake().size();
    COMMONAPI_INFO(strSize);
    if (teluxSVP.make == 0) {
        teluxSVP.make = new char[strSize];
    COMMONAPI_INFO("2");
        //if (teluxSVP.make == 0) {
        //    COMMONAPI_ERROR("Malloc failed");
        //    return;
        //}
    }
    COMMONAPI_INFO("2");
    std::memcpy(teluxSVP.make, idlSVP.getMake().c_str(), strSize);
    COMMONAPI_INFO("2");
    strSize = idlSVP.getModel_name().size();
    if (teluxSVP.model == 0) {
        teluxSVP.model = new char[strSize];
        if (teluxSVP.model == 0) {
            COMMONAPI_ERROR("Malloc failed");
            return;
        }
    }
    COMMONAPI_INFO("2");
    std::memcpy(teluxSVP.model, idlSVP.getModel_name().c_str(), strSize);
    COMMONAPI_INFO("2");
    teluxSVP.begin_model_year = idlSVP.getBegin_model_year();
    teluxSVP.end_model_year = idlSVP.getEnd_model_year();
}
} //namespace Vehicle
} //namespace SPLITS
