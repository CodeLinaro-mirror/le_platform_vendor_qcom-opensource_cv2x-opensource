/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <CommonAPI/CommonAPI.hpp>
#include "SplitsVehicleDataService.hpp"

int SplitsVehicleDataService::init()
{
    auto runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.VehicleData";
    std::string connection = "SPLITS";

    VehicleData::staticVehicleParameters params;
    params.setVehicle_height_cm(163);
    params.setVehicle_width_cm(193);
    params.setVehicle_length_cm(490);
    params.setFront_bumper_height_cm(20);
    params.setVehicle_mass_kg(2595);
    params.setMake(std::string("Audi"));
    params.setModel_name(std::string("e-tron"));
    params.setBegin_model_year(2022);
    params.setEnd_model_year(2024);

    vehStub_ = std::make_shared<VehicleDataStubImpl>(params);
    while (!runtime->registerService(domain,
                instance,
                vehStub_,
                connection)) {
        std::cout << "registing VehicleDataService " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "Successfully Registered vehicleDataService\n" << std::endl;
    std::cout << "Enter one or more numbers to trigger events: " << std::endl;
    std::cout << "(0) eventHazardLights" << std::endl;
    std::cout << "(1) eventStopLineViolation" << std::endl;
    std::cout << "(2) eventABSactivated" << std::endl;
    std::cout <<"(3) eventTractionControlLoss" << std::endl;
    std::cout <<"(4) eventStabilityControlactivated" << std::endl;
    std::cout <<"(5) eventHazardousMaterials" << std::endl;
    std::cout <<"(6) eventReserved1" << std::endl;
    std::cout <<"(7) eventHardBraking" << std::endl;
    std::cout <<"(8) eventLightsChanged" << std::endl;
    std::cout <<"(9) eventWipersChanged" << std::endl;
    std::cout <<"(10) eventFlatTire" << std::endl;
    std::cout <<"(11) eventDisabledVehicle" << std::endl;
    std::cout <<"(12) eventAirBagDeployment" << std::endl;
    std::cout <<"(13) eventJackKnife" << std::endl;
    std::cout <<"(14) clear all events" << std::endl;

    return 0;
}
int SplitsVehicleDataService::readEventFlags(uint16_t &flags)
{
    int n;
    std::string str;
    std::vector<int> inputs;
    flags = 0;
    std::getline(std::cin, str);
    std::istringstream sstr(str);
    while(sstr >> n) {
        inputs.push_back(n);
    }
    for (auto i : inputs) {
        if (i > 14) {
           std::cout << "invalid inputs: " << i << std::endl;
           return -1;
        } else if (i == 14) {
            flags = 0;
            return 0;
        } else {
            flags |= (1 << n);
        }
    }
    return 0;
}

void SplitsVehicleDataService::run(void)
{
    uint16_t eventFlags;
    VehicleData::dynamicVehicleState state;
    th_ = std::thread([&]() {
            while(stop_ == false) {
                if (readEventFlags(eventFlags)) {
                    continue;
                } else {
                    state.setEvents(eventFlags);
                    std::cout << "calling onVehicleStateChanged" << std::endl;
                    vehStub_->onVehicleStateChanged(state);
                }
            }
        });
}
