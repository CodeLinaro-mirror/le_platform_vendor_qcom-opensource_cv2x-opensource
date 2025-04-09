/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
 /**
  * @file: VehicleInterface.hpp
  *
  * @brief: Public API and definitions of the VehicleData abstract interface.
  *
  */
#ifndef __VEHICLE_INTERFACE_HPP__
#define __VEHICLE_INTERFACE_HPP__

#include <telux/cv2x/legacy/v2x_vehicle_api.h>
#include <telux/common/CommonDefines.hpp>
#include <iostream>
#include <bitset>
#include <memory>
#include <vector>
#include <mutex>

using telux::common::Status;
using telux::common::ErrorCode;
using std::weak_ptr;

namespace SPLITS {
namespace Vehicle {

class VehicleInterfaceListener {
public:
    VehicleInterfaceListener(){}
    virtual void onVehicleDataChanged(const current_dynamic_vehicle_state_t &vehicleState){}
    virtual void onHighResMotionDataChanged(const high_resolution_motion_t &highResMotion){}
    virtual void onVehicleEventChanged(const uint16_t &eventFlags){};
    virtual ~VehicleInterfaceListener(){}
};

class VehicleInterface {

public:
    virtual Status init(void) = 0;

    virtual ~VehicleInterface(){};

    virtual Status enableVehicleDynamicData(void) = 0;
    virtual Status disableVehicleDynamicData(void) = 0;
    virtual Status enableHighResMotionData(void) = 0;
    virtual Status disableHighResMotionData(void) = 0;
    virtual Status getStaticVehicleParameters(static_vehicle_parameters_t &params) = 0;

    virtual Status registerListener(std::weak_ptr<VehicleInterfaceListener> listener){
        auto sp = listener.lock();
        if (sp == nullptr) {
            return Status::INVALIDPARAM;
        }
        std::lock_guard<std::mutex> lock(listenerMutex_);
        for (auto it : listeners_) {
            if (it.lock() == sp) {
                return Status::ALREADY;
            }
        }
        listeners_.push_back(listener);
        return Status::SUCCESS;
    }

    virtual Status deregisterListener(weak_ptr<VehicleInterfaceListener> listener) {
        std::lock_guard<std::mutex> lock(listenerMutex_);
        bool listenerExisted = false;
        for (auto it = listeners_.begin(); it != listeners_.end();) {
            auto sp = (*it).lock();
            if (!sp) {
                it = listeners_.erase(it);
            } else if (sp == listener.lock()) {
                it = listeners_.erase(it);
                listenerExisted = true;
            } else {
                ++it;
            }
        }
        if (listenerExisted) {
            return Status::SUCCESS;
        } else  {
            return Status::NOSUCH;
        }
    }

protected:
    uint16_t eventFlags_ = 0;
    std::vector<std::weak_ptr<VehicleInterfaceListener>> listeners_;
    std::mutex listenerMutex_;

};
}  //namespace Vehicle
}  //namespace SPLITS
#endif
