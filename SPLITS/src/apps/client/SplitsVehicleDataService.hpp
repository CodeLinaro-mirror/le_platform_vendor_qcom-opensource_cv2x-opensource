/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
    int readEventFlags(uint16_t &flags);
    std::shared_ptr<VehicleDataStubImpl> vehStub_ = nullptr;
};

#endif //__SPLITS_VEHICLE_DATA_SERVICE_HPP__

