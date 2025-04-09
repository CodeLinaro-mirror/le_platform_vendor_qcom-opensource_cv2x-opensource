/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <map>
#include <vector>
#include <string>
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

    return 0;
}

int SplitsVehicleDataService::getEventFlags(VehicleData::dynamicVehicleState &state)
{
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
    std::cout <<"(13) eventJackKnife(SAE only)" << std::endl;
    std::cout <<"(14) clear all events" << std::endl;

    int n;
    std::string str;
    std::vector<int> inputs;
    uint16_t flags = 0;
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
            break;
        } else {
            flags |= (1 << n);
        }
    }
    std::cout << "event flag:" << flags << std::endl;
    state.setEvents(flags);
    return 0;
}

int SplitsVehicleDataService::getTransmission(VehicleData::dynamicVehicleState &state) {
    std::cout << "Enter a number for Transmission State: " << std::endl;
    std::cout << "(0) neutral" << std::endl;
    std::cout << "(1) park" << std::endl;
    std::cout << "(2) forwardGears" << std::endl;
    std::cout <<"(3) reverseGears" << std::endl;
    std::cout <<"(7) unavailable" << std::endl;

    int value = 0;
    std::cin >> value;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (value < 0 or (value > 3 and value != 7)) {
        std::cerr << "invalid input" << std::endl;
        return -1;
    }
    state.setTransmission(static_cast<VehicleData::v2x_TransmissionState_enum::Literal>(value));
    return 0;
}

int SplitsVehicleDataService::getAngle(VehicleData::dynamicVehicleState &state) {
    std::cout << "Enter a number for steering wheel angle [-126, 127]: " << std::endl;
    std::cout << "(-126) = -189 deg and beyond" << std::endl;
    std::cout << "(+126) = 189 deg and beyond" << std::endl;
    std::cout << "(+127) unavailable" << std::endl;

    int value = 0;
    std::cin >> value;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (value < -126 or value > 127) {
        std::cerr << "invalid input" << std::endl;
        return -1;
    }
    state.setSteeringWheelAngle(value);
    return 0;
}

int SplitsVehicleDataService::getWheelBrakes(uint8_t value, VehicleBrakes &brakes) {
    switch (value) {
        case 0:
            brakes.bits.leftFront = 0;
            brakes.bits.leftRear = 0;
            brakes.bits.rightFront = 0;
            brakes.bits.rightRear = 0;
            break;
        case 1:
            brakes.bits.leftFront = 1;
            break;
        case 2:
            brakes.bits.leftRear = 1;
            break;
        case 3:
            brakes.bits.rightFront = 1;
            break;
        case 4:
            brakes.bits.rightRear = 1;
            break;
        default:
            return -1;
    }
    return 0;
}

int SplitsVehicleDataService::getBrakes(VehicleData::dynamicVehicleState &state) {
    std::cout << "Enter 0 to clear all brakes or <type1>:<value1>,<type2>:<value2>...: "
              << std::endl;
    std::cout <<"(0) reset all brakes" << std::endl;
    std::cout << "(1) wheel brakes: 0-una 1-leftFront, 2-leftRear, 3-rightFront, 4-rightRear"
              << std::endl;
    std::cout << "(2) traction: 0-una, 1-off, 2-on, 3-engaged" << std::endl;
    std::cout <<"(3) abs: 0-una, 1-off, 2-on, 3-engaged" << std::endl;
    std::cout <<"(4) scs: 0-una, 1-off, 2-on, 3-engaged" << std::endl;
    std::cout <<"(5) brake boost: 0-una, 1-off, 2-on" << std::endl;
    std::cout <<"(6) aux brakes: 0-una, 1-off, 2-on" << std::endl;

    std::string input;
    std::getline(std::cin, input);
    if ("0" == input) {
        // reset brakes
        std::cout << "reset brakes" << std::endl;
        state.setBrakeStatus(0);
        return 0;
    }

    // parse input string
    std::map<uint8_t, uint8_t> typeValueMap;
    std::stringstream ss(input);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        std::stringstream pairStr(pair);
        std::string typeStr, valueStr;
        if (std::getline(pairStr, typeStr, ':') and std::getline(pairStr, valueStr, ':')) {
            std::cout << "type:" << typeStr << ", value:" << valueStr << std::endl;
            uint8_t type = static_cast<uint8_t>(std::stoi(typeStr));
            uint8_t value = static_cast<uint8_t>(std::stoi(valueStr));
            typeValueMap[type] = value;
        } else {
            std::cerr << "invalid type and value:" << pair << std::endl;
            return -1;
        }
    }

    // get brake status
    int ret = 0;
    VehicleBrakes brakes = {.word = 0};
    for (const auto &kv : typeValueMap) {
        uint8_t value = kv.second;
        switch (kv.first) {
            case 1:
                ret = getWheelBrakes(value, brakes);
                break;
            case 2:
                if (value > 3) {
                    ret = -1;
                } else {
                    brakes.bits.tcs = value;
                }
                break;
            case 3:
                if (value > 3) {
                    ret = -1;
                } else {
                    brakes.bits.abs = value;
                }
                break;
            case 4:
                if (value > 3) {
                    ret = -1;
                } else {
                    brakes.bits.scs = value;
                }
                break;
            case 5:
                if (value > 2) {
                    ret = -1;
                } else {
                    brakes.bits.boostApplied = value;
                }
                break;
            case 6:
                if (value > 2) {
                    ret = -1;
                } else {
                    brakes.bits.aux = value;
                }
                break;
            default:
                ret = -1;
                break;
        }
        if (ret) {
            std::cerr << "invalid type:" << kv.first << " value:" << value << std::endl;
            break;
        }
    }

    // update brake status
    if (0 == ret) {
        state.setBrakeStatus(brakes.word);
    }

    return ret;
}

int SplitsVehicleDataService::getDynamicVehState(VehicleData::dynamicVehicleState &state) {
    int ret = 0;
    std::cout << "Enter a number to select dynamic data type: " << std::endl;
    std::cout << "(0) events" << std::endl;
    std::cout << "(1) TransmissionState" << std::endl;
    std::cout << "(2) SteeringWheelAngle" << std::endl;
    std::cout <<"(3) BrakeStatus" << std::endl;

    int option = 0;
    std::cin >> option;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    switch (option) {
        case 0:
            ret = getEventFlags(state);
            break;
        case 1:
            ret = getTransmission(state);
            break;
        case 2:
            ret = getAngle(state);
            break;
        case 3:
            ret = getBrakes(state);
            break;
        default:
            std::cerr << "invalid input" << std::endl;
            return -1;
    }

    return ret;
}

void SplitsVehicleDataService::run(void)
{
    th_ = std::thread([&]() {
            VehicleData::dynamicVehicleState state;
            while(stop_ == false) {
                if (getDynamicVehState(state)) {
                    continue;
                } else {
                    std::cout << "calling onVehicleStateChanged" << std::endl;
                    vehStub_->onVehicleStateChanged(state);
                }
            }
        });
}
