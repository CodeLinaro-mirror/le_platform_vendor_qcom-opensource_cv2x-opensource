/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include "Cv2xRadioManagerStubImpl.hpp"
#include "Cv2xRadioStubImpl.hpp"
#include "Utils.hpp"

int main() {

    std::vector<std::string> groups{"system", "diag", "radio", "logd", "dlt"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        std::cout << "Adding supplementary group failed!" << std::endl;
    }

    CommonAPI::Runtime::setProperty("LogContext", "Cv2xRemoteAPI");
    CommonAPI::Runtime::setProperty("LogApplication", "Cv2xRemoteAPI");
    CommonAPI::Runtime::setProperty("LibraryBase", "Cv2xRemoteAPI");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.telux.cv2x.Cv2xRadioManager";
    std::string connection = "service-Cv2xRadioManager";

    std::shared_ptr<Cv2xRadioStubImpl> radioService = std::make_shared<Cv2xRadioStubImpl>();
    while (!runtime->registerService(domain,
                std::string("commonapi.telux.cv2x.Cv2xRadio"),
                radioService,
                std::string("service-Cv2xRadio"))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Successfully Registered Service-Cv2xRadio!" << std::endl;
#if 1
    std::shared_ptr<Cv2xRadioManagerStubImpl> radioManagerService = std::make_shared<Cv2xRadioManagerStubImpl>();
    while (!runtime->registerService(domain, instance, radioManagerService, connection)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Successfully Registered Service-Cv2xRadioManager!" << std::endl;
#endif
    while (true) {
        //myService->incCounter(); // Change value of attribute, see stub implementation
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
