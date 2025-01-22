/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


 /**
  * @file: KinematicsReceive.cpp
  *
  * @brief: Implementation of the KinematicsReceive.h
  *
  */
#include "KinematicsReceive.hpp"
#include <chrono>
#include <ctime>

using telux::common::ServiceStatus;

shared_ptr<KinematicsReceive> KinematicsReceive::instance = nullptr;

mutex KinematicsReceive::sync;

shared_ptr<ILocationInfoEx> LocListener::getLocation() {
    std::unique_lock<std::mutex> lck(locInfoMtx_);
    /*if no locationInfo, wait at most 1 sec unless locationInfo update or exit occur*/
    if (locationInfo_ == nullptr && (!exit_) &&
        !locInfoCv_.wait_for(lck, std::chrono::seconds(1),[this]{
            return (locationInfo_!= nullptr || exit_ == true);
        })) {
        cout<<"request for location too fast. " << +exit_ << std::endl;
    }
    return locationInfo_;
};

void LocListener::close() {
    std::lock_guard<std::mutex> lock(locInfoMtx_);
    exit_ = true;
    locInfoCv_.notify_all();
}

void LocListener::setLocCbFn(void(*locCbFn_)(shared_ptr<ILocationInfoEx> &locationInfo)){
    locCbFunction_ = locCbFn_;
}

void LocListener::onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo) {
    static bool locInfoAvailable = false;
    lock_guard<mutex> lk(locInfoMtx_);
    locationInfo_ = locationInfo;
    if(locCbFunction_){
        locCbFunction_(locationInfo_);
    }
    if (not locInfoAvailable) {
        locInfoAvailable = true;
        locInfoCv_.notify_all();
    }
}

void KinematicsReceive::startDetailsCallback(ErrorCode error){
    if (ErrorCode::SUCCESS != error) {
        cout << "Error starting Details Report on Location.\n";
    }
}

KinematicsReceive::KinematicsReceive(){}

shared_ptr<ILocationInfoEx> KinematicsReceive::getLocation(){
    if(!KinematicsReceive::instance){
        KinematicsReceive(this->interval);
    }

    if (locListener_) {
        return locListener_->getLocation();
    }
    return nullptr;
}

KinematicsReceive::KinematicsReceive(uint16_t interval){
    {
        lock_guard<mutex> lk(sync);
        if (!KinematicsReceive::instance) {
            KinematicsReceive::instance = make_shared<KinematicsReceive>();
        } else {
            return;
        }
    }
    auto &locationFactory = LocationFactory::getInstance();

    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    locationManager_ = locationFactory.getLocationManager([&prom](ServiceStatus status) {
          if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
    if (locationManager_ and prom.get_future().get() == ServiceStatus::SERVICE_AVAILABLE) {
        locListener_ = make_shared<LocListener>();
        // Registering a listener to get location fixes
        locationManager_->registerListenerEx(locListener_);
        // Starting the reports for fixes
        printf("Creating callback for gnss fixes\n");
        auto respCallback = [&](ErrorCode error){
                            startDetailsCallback(error); };
        locationManager_->startDetailedReports(interval, respCallback);
    } else {
        // release location manager if it's created but service unavailable
        if (locationManager_) {
            locationManager_ == nullptr;
        }
        cout << "Error on Location Create.\n";
    }
    this->interval = interval;
}

KinematicsReceive::KinematicsReceive(std::vector<std::shared_ptr<ILocationListener>> locListeners, uint16_t interval){
    {
        lock_guard<mutex> lk(sync);
        if (!KinematicsReceive::instance) {
            KinematicsReceive::instance = make_shared<KinematicsReceive>();
        } else {
            return;
        }
    }
    auto &locationFactory = LocationFactory::getInstance();

    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    locationManager_ = locationFactory.getLocationManager([&prom](ServiceStatus status) {
          if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
    if (locationManager_ and prom.get_future().get() == ServiceStatus::SERVICE_AVAILABLE) {
        for (auto &locListener_ :locListeners) {
            // Registering a listener to get location fixes
            locationManager_->registerListenerEx(locListener_);
            locListeners_.push_back(locListener_);
        }
        // Starting the reports for fixes
        printf("Creating callback for gnss fixes\n");
        auto respCallback = [&](ErrorCode error){
                            startDetailsCallback(error); };
        locationManager_->startDetailedReports(interval, respCallback);
    } else {
        // release location manager if it's created but service unavailable
        if (locationManager_) {
            locationManager_ == nullptr;
        }
        cout << "Error on Location Create.\n";
    }
    this->interval = interval;
}

void KinematicsReceive::close(){
    if (locationManager_) {
        if (locListener_) {
            locListener_->close();
            locationManager_->deRegisterListenerEx(locListener_);
        }
    }

    for (auto listener : locListeners_) {
        // Registering a listener to get location fixes
        LocListener* locListener = dynamic_cast<LocListener*>(listener.get());
        locListener->close();
        locationManager_->deRegisterListenerEx(listener);
    }

    cout << "Location Listeners closed.\n";
}
