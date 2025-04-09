/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsVehicleDataService.hpp
 *
 * @description This file demonstrate a vehicle data service implementation via
 * commonapi/vsomeip interface. It provides vehicle static and dynamic data to SPLITS core.
 */
#ifndef __SPLITS_VEHICLE_DATA_SERVICE_HPP__
#define __SPLITS_VEHICLE_DATA_SERVICE_HPP__

#include <thread>
#include <iostream>
#include <vector>
#include <memory>
#include <CommonAPI/CommonAPI.hpp>
#include "vehicle/VehicleDataStubImpl.hpp"

using v1::commonapi::SPLITS::VehicleData;

typedef union {
    struct {
        unsigned padding                    : 1;
        unsigned aux                        : 2;
        unsigned boostApplied               : 2;
        unsigned scs                        : 2;
        unsigned abs                        : 2;
        unsigned tcs                        : 2;
        unsigned rightRear                  : 1;
        unsigned rightFront                 : 1;
        unsigned leftRear                   : 1;
        unsigned leftFront                  : 1;
        unsigned unavailable                : 1;
    } bits;
    unsigned short word;
} VehicleBrakes;

class SplitsVehicleDataService
{
public:
    SplitsVehicleDataService() {}
    ~SplitsVehicleDataService(){}
    int init();
    void run();
    void stop() {
        stop_ = true;
        th_.join();
    }
private:
    bool stop_ = false;
    std::thread th_;
    std::shared_ptr<VehicleDataStubImpl> vehStub_ = nullptr;

    int getDynamicVehState(VehicleData::dynamicVehicleState &state);
    int getBrakes(VehicleData::dynamicVehicleState &state);
    int getWheelBrakes(uint8_t value, VehicleBrakes &brakes);
    int getAngle(VehicleData::dynamicVehicleState &state);
    int getTransmission(VehicleData::dynamicVehicleState &state);
    int getEventFlags(VehicleData::dynamicVehicleState &state);
};

#endif //__SPLITS_VEHICLE_DATA_SERVICE_HPP__

