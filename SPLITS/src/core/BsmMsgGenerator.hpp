/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <atomic>
#include <telux/common/CommonDefines.hpp>
#include "v2xMsgSae.hpp"
#include "v2xMsgCsae.hpp"
#include "vehicle/VehicleInterface.hpp"
#include "location/KinematicsReceive.hpp"
#include "qUtils.hpp"

using SPLITS::Vehicle::VehicleInterface;
using SPLITS::Vehicle::VehicleInterfaceListener;
using telux::common::Status;
using telux::loc::ILocationInfoEx;

namespace SPLITS {
namespace Core {

class SplitsCore;

struct BsmLocationInfo {
    int32_t lat;
    int32_t lon;
    int32_t elev;
    uint8_t semiMajorAccu;
    uint8_t semiMinorAccu;
    uint16_t orientationAccu;
    uint16_t speed;
    uint16_t heading;
    int16_t longitudeAccel;
    int16_t latitudeAccel;
    int16_t vertAccel;
    int16_t yaw;
};

class BsmMsgGenerator : public VehicleInterfaceListener,
    public ILocationListener,
    public std::enable_shared_from_this<BsmMsgGenerator> {
public:
    std::shared_ptr<BsmMsgGenerator> getptr() {
        return shared_from_this();
    }
    BsmMsgGenerator(std::shared_ptr<SplitsCore> core) {
        splitsCore_ = core;
    }
    ~BsmMsgGenerator() {}
    virtual Status init(bool useCan = true);

    virtual Status fillBsm(std::shared_ptr<v2xMsgSae> msg, bool idChanged,
        unsigned char* newTempId);

    virtual Status fillBsm(std::shared_ptr<v2xMsgCsae> msg, bool idChanged,
        unsigned char* newTempId);

private:
    void fillVehicleSafetyExt(std::shared_ptr<J2735::idl::BasicSafetyMessage> bsm);
    void onHighResMotionDataChanged(const high_resolution_motion_t &highResMotion) override;
    void onVehicleDataChanged(const current_dynamic_vehicle_state_t &vehicleState) override;
    void onVehicleEventChanged(const uint16_t &eventFlags) override;
    void onDetailedLocationUpdate(const std::shared_ptr<ILocationInfoEx> &locationInfo) override;
    std::shared_ptr<VehicleInterface> vehicle_;
    std::shared_ptr<SplitsCore> splitsCore_;
    std::shared_ptr<QUtils> utils_;
    high_resolution_motion_t highResMotion_;
    std::mutex vehStateMutex_;
    current_dynamic_vehicle_state_t vehicleState_;
    static_vehicle_parameters_t vehicleParams_;
    BsmLocationInfo bsmLocation_;
    std::mutex locationMutex_;
    int32_t lat_;
    int32_t lon_;

    uint16_t eventFlags_;
    std::atomic<bool> criticalState_;
    std::mutex eventMutex_;
    uint32_t randNumMsgCnt_ = 0;
    uint32_t randNumMsgId_ = 0;
    uint32_t msgCount_ = 0;
    std::array<uint8_t, 4> msgId_;
    std::array<uint8_t, 8> tmpVehId_; // temporary vehicle ID used for CSAE BSM
    bool avdClass_ = false;  //test
};
} //namespace Core
} //namespace SPLITS
