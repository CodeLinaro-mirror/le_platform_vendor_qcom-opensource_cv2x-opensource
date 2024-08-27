/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: SecurityInterface.hpp
 *
 * @brief: Security Service wrapper for aerolink
 */

#ifndef SECURITY_INTERFACE_HPP_
#define SECURITY_INTERFACE_HPP_

#include <stdexcept>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <semaphore.h>
#include <atomic>
#include <thread>
#include <iterator>
#include <chrono>
#include <sstream>
#include <cstdint>
#include <string>
#include "viicsec.h"
#include "MisbehaviorData.h"
#include "location/KinematicsReceive.hpp"
#include "SplitsFilter.hpp"

using SPLITS::Core::SplitsFilter;


namespace SPLITS {
namespace Core {

const uint8_t NO_KEY_GEN=0;
const uint8_t ASYMMETRIC_KEY_GEN=1;
const uint8_t SYMMETRIC_KEY_GEN=2;
const uint8_t IMPORT_SYMMETRIC_KEY=3;

struct VerifStats
{
    double timestamp;
    double verifLatency;
};

struct SignStats
{
    double timestamp;
    double signLatency;
};

struct MisbehaviorStats{
    double timestamp = 0.0;
    double misbehaviorLatency = 0.0;
};

struct ResultLoggingStats{
    int tid;
    int asyncVerifSuccess;
    double currTimeStamp;
    double prevBatchTimeStamp;
    double rate;
    double dur;
};

struct SecLocation {
    int32_t latitude;
    int32_t longitude;
    uint16_t elevation;
};

/*
 * Fields related to ID change operation
 */
typedef struct IDChangeData {
    unsigned char tempId [4]; // 32 bits
    unsigned char certId [8]; // last 8 bytes of cert id
    bool idChanged;
    sem_t idChangeSem;
} IDChangeData_t;

static size_t chunksize = 10000;

class SecurityInterface : public ILocationListener,
                          public std::enable_shared_from_this<SecurityInterface> {
    public:
        // ctor
        SecurityInterface(const std::string ctxName, uint16_t countryCode);
        // dtor
        ~SecurityInterface() {
            deinit();
        }
        int createNewSmp(SecuredMessageParserC* smpPtr);
        int createNewSmg(SecuredMessageGeneratorC* smgPtr);
        int setLeapSeconds(uint32_t leapSeconds);
        int signMsg(SecuredMessageParserC* smg,  const uint8_t *msg, uint32_t msgLen,
                    uint8_t *signedSpdu, uint32_t &signedSpduLen, uint32_t psidValue,
                    bool enableSsp, const uint8_t* sspValue, uint32_t sspLength, bool enableSspMask,
                    const uint8_t* sspMaskValue, uint32_t sspMaskLength,
                    SignerTypeOverride signType = SignerTypeOverride::STO_AUTO);
        int extractMsg(SecuredMessageParserC* smp, const uint8_t * msg, uint32_t msgLen,
                    uint8_t const *payload, uint32_t payloadLen, uint32_t &dot2HdrLen);
        int checkConsistencyandRelevancy(SecuredMessageParserC* smp, BsmData* rvBsmInfo);
        int syncVerify(SecuredMessageParserC* smp, BsmData hvKine, BsmData rvKine,
                    bool enableMbdCheck);
        int asyncVerify(BsmData rvKine, void* asyncCbData,
                    uint8_t priority, ValidateCallback callBackFunction,
                    SecuredMessageParserC* smp);
        int registerIdChange(const std::string lcmName, IDChangeData* idChangeData);
        static void initIdChangeCbFn(void *userData, uint8_t numCerts, uint8_t *certIndex);
        static void completeIdChangeCbFn(AEROLINK_RESULT returnCode, void *userData,  uint8_t const *certId);

        bool changeIdNow();
        int idChange();
        AEROLINK_RESULT bsmMbdCheck(SecuredMessageParserC* smp, BsmData* rvBsmInfo);
        int lockIdChange();
        int unlockIdChange();
        std::shared_ptr<SecurityInterface> getptr() {
            return shared_from_this();
        }

    private:
        int init(const std::string ctxName, uint16_t countryCode);
        void deinit();

        // from ilocationlistener
        void onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo) override;
        void setPermissions(
            SigningPermissions    *p,
            uint32_t               psid,
            uint8_t const * const  ssp,
            uint8_t const * const  sspMask,
            uint32_t               sspLen);
        void setSecVerbosity(uint8_t verbosity);
        void printBytes(char *label, uint8_t buffer[], uint32_t length);
        void printErrorMsg (std::string errMsg, AEROLINK_RESULT result);

        // private variables
        bool enableMisbehavior;
        bool enableConsistency;
        bool enableRelevance;
        std::shared_ptr<BsmData> misbehaviorAppDataPtr_ = nullptr;
        std::shared_ptr<SplitsFilter> splitsFilter_ = nullptr;
        std::string securityCtxName_;
        SecLocation hvSecLocation_;
        SecLocation lastHvSecLocIdChange_;
        bool initIdChange_ = false;
        uint64_t lastIdChangeTime_ = 0;
        uint64_t timeSinceLastIdChange_ = 0;
        double distSinceLastIdChange_ = 0.0;
        uint64_t idChangeInterval = 300000;
        double idChangeDistance = 2100.0;
        uint16_t countryCode_;
        SecurityContextC secContext_;
        std::mutex locationMutex_;
};
} //namespace Core
} //namespace SPLITS
#endif

