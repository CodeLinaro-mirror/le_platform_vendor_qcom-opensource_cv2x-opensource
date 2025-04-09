/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sys/time.h>
#include "qUtils.hpp"
#include "SecurityInterface.hpp"
#include "utils.h"
#include <sstream>
#include <iomanip>

#include "Logger.hpp"

namespace SPLITS {
namespace Core {

/* STATIC VARIABLES */
/* Asynchronous Function Variables */
//static      int secVerbosity = 0;
//static AEROLINK_RESULT completeChangeId_status;
//static bool retChangeId_status; // tells thread if callback has completed

/* Semaphores */


/* LOGGING FUNCTIONS */
// Function to set the verbosity of these security related functions
// 0   -> Quiet
// 1-4 -> Statistics and Errors
// > 4 -> Verbose Mode
void SecurityInterface::setSecVerbosity(uint8_t verbosity) {
    secVerbosity = verbosity;
}

void SecurityInterface::printBytes(char *label, uint8_t buffer[], uint32_t length) {
    uint32_t i = 0;

    printf("%s (%d bytes): ", label, length);

    for( i = 0; i < length; i++ ) {
        printf("%02x ", buffer[i] & 0xffU);
    }

    printf("\n");
}

void SecurityInterface::printErrorMsg(std::string errMsg, AEROLINK_RESULT result) {
    fprintf(stderr, "%s=%d(%s)\n", errMsg.c_str(), result, ws_errid(result));
}

int SecurityInterface::setLeapSeconds(uint32_t leapSeconds){
    int result = -1;
    /*
     * Adjust the time for the expiration of signatures and certificates
     */
    if ((result = securityServices_setTimeAdjustment(leapSeconds)) != WS_SUCCESS) {
        LOGE(__FUNCTION__, "securityServices_setTimeAdjustment failed: ", result);
    }
    return result;
}

void SecurityInterface::onDetailedLocationUpdate(
    const shared_ptr<ILocationInfoEx> &locationInfo) {
    int result = -1;
    if (locationInfo) {
        LOGD(__FUNCTION__, "Updating security position");
        {
            std::lock_guard<std::mutex> lk(locationMutex_);
            hvSecLocation_.latitude = locationInfo->getLatitude();
            hvSecLocation_.longitude = locationInfo->getLongitude();
            hvSecLocation_.elevation = locationInfo->getHeading();
            result = securityServices_setCurrentLocation(
                hvSecLocation_.latitude, hvSecLocation_.longitude,
                hvSecLocation_.elevation, countryCode_);
            if (result != WS_SUCCESS) {
                LOGE(__FUNCTION__, "Location not updated successfully");
            }
        }
    }
}

bool SecurityInterface::changeIdNow() {
    // here we need to check for two things:
    // interval has passed and distance has been covered since last id change
    uint64_t currTime = timestamp_now();
    if (currTime > lastIdChangeTime_) {
        timeSinceLastIdChange_ = currTime - lastIdChangeTime_;
    } else {
        timeSinceLastIdChange_ = 0;
    }
    if (!initIdChange_) {
        LOGE(__FUNCTION__, "Id change has not yet been initialized");
        return false;
    }
    {
        std::lock_guard<std::mutex> lk(locationMutex_);
        double hvLatNew = hvSecLocation_.latitude;
        double hvLonNew = hvSecLocation_.longitude;
        double hvLatOld = lastHvSecLocIdChange_.latitude;
        double hvLonOld = lastHvSecLocIdChange_.longitude;
        // check if there have been any fixes yet
        if (initIdChange_) {
            distSinceLastIdChange_ =
                 splitsFilter_->bsmCompute2dDistance(hvLatOld, hvLonOld, hvLatNew, hvLonNew);
        /*
          std::cout << "Curr lat and long are: " <<
              hvLatNew << ", " << hvLonNew << "\n";
          std::cout << " Last lat and long are: " <<
              hvLatOld << ", " << hvLonOld << "\n";
          std::cout << "Dist since last id change is now: " <<
              timeSinceLastIdChange_ << "\n";
          std::cout << "Time since last id change is now: " <<
              timeSinceLastIdChange_ << "\n";
        */
        } else {
            timeSinceLastIdChange_ = 0;
        }
    }
    // check if both conditions satisfied
    if (timeSinceLastIdChange_ >= idChangeInterval &&
        distSinceLastIdChange_ >= idChangeDistance) {
        // perform id change and record current position and time
        return true;
    }

    return false;
}

int SecurityInterface::lockIdChange() {
    int result = -1;
    result = securityServices_idChangeLock();
    if (result != WS_SUCCESS) {
        return -1;
    }

    return result == WS_SUCCESS ? 0 : -1;
}

int SecurityInterface::unlockIdChange() {
    int result = -1;
    result = securityServices_idChangeUnlock();
    if (result != WS_SUCCESS) {
        return -1;
    }

    return result == WS_SUCCESS ? 0 : -1;
}

int SecurityInterface::idChange() {

    int result = -1;
    result = securityServices_idChangeInit();
    if(result != WS_SUCCESS){
        return -1;
    }
    // on successful return, modify other id-related information in upper layer
    return result;
}

/* INITIALIZATION/DEINITIALIZATION FUNCTIONS */
SecurityInterface::SecurityInterface(const std::string ctxName, uint16_t countryCode) {
    if(init(ctxName, countryCode) < 0) {
        throw std::runtime_error("SecurityInterface Init Failed");
    }
    // should  perform sanitary check on the value
}


// Primary initialization function for Aerolink services
int SecurityInterface::init(const std::string ctxName, uint16_t countryCode) {
    const char *aerolinkLibVersion = securityServices_getVersion();
    if (aerolinkLibVersion != nullptr) {
        LOGD(__FUNCTION__, "Aerolink Library Version: ", aerolinkLibVersion);
    }

    int result = -1;
    if ((result = securityServices_initialize()) != WS_SUCCESS) {
        LOGE(__FUNCTION__, "securityServices initialization failed: ", ws_errid(result));
        return -1;
    }

    // May add the generator location here as well
    LOGD(__FUNCTION__, "Security context name is: ", ctxName);
    result = sc_open(ctxName.c_str(), &secContext_);
    if (result != WS_SUCCESS) {
        LOGE(__FUNCTION__, "Failed to open security context: ", ws_errid(result));
        return -1;
    }

    // make the asynchronous verification callback data pool
    asyncVerifDataPool_ = std::make_shared<AsyncVerifDataPool>();
    if (!initVerifDataPool(PP_BUFFER_MAX_SIZE)) {
        LOGE(__FUNCTION__, "Failed to initialize async verif data pool");
        return -1;
    }

    // make the asynchronous verification callback data pool
    asyncVerifDataPool_ = std::make_shared<AsyncVerifDataPool>();
    if(!initVerifDataPool(PP_BUFFER_MAX_SIZE)){
        fprintf(stderr, "Failed to initialize async verif data pool\n");
        return -1;
    }

    splitsFilter_ = std::make_shared<SplitsFilter>();
    {
        if (!initIdChange_) {
            LOGD(__FUNCTION__, "Setting init id change to true");
            lastHvSecLocIdChange_.latitude = hvSecLocation_.latitude;
            lastHvSecLocIdChange_.longitude = hvSecLocation_.longitude;
            timeSinceLastIdChange_ = timestamp_now();
            initIdChange_ = true;
        }
    }

    return result;
}

// Deinitialize aerolink services when process is finished
void SecurityInterface::deinit() {
    LOGD(__FUNCTION__, "Aerolink deinitializing");
    (void)sc_close(secContext_);
    (void)securityServices_shutdown();
}

/* UTILITY AND MULTI-THREADING FUNCTIONS */

// Set permissions struct for signing/verifying
void SecurityInterface::setPermissions(
    SigningPermissions    *p,
    uint32_t               psid,
    uint8_t const * const  ssp,
    uint8_t const * const  sspMask,
    uint32_t               sspLen) {
    p->psid = psid;
    p->ssp = ssp;
    p->sspMask = sspMask;
    p->isBitmappedSsp = (NULL == sspMask) ? 0 : 1;
    p->sspLen = sspLen;
}


// Function to create a new SecuredMessageParser object -- for verification
int SecurityInterface::createNewSmp(SecuredMessageParserC* smpPtr) {
    int result = -1;
    result = smp_new(secContext_, smpPtr);
    if (result != WS_SUCCESS){
        if (secVerbosity > 4)
            LOGE(__FUNCTION__, "Unable to create secure message parser: ", ws_errid(result));
        return -1;
    }
    return result;
}

// Function to create a new SecuredMessageGenerator object -- for signing
int SecurityInterface::createNewSmg(SecuredMessageGeneratorC* smgPtr) {
    int result = -1;
    result = smg_new(secContext_, smgPtr);
    if (result != WS_SUCCESS) {
        if (secVerbosity > 7)
            LOGE(__FUNCTION__, "Unable to create secure message generator: ", ws_errid(result));
        return -1;
    }
    return result;
}

bool SecurityInterface::initVerifDataPool(int dataPoolSize) {
    // create dataPoolSize entries for the data struct
    for (int i = 0; i < PP_BUFFER_MAX_SIZE; i++) {
        asyncCbData_t tmpData;
        if (createNewSmp((SecuredMessageParserC*)(&tmpData.smp)) <= -1) {
            LOGE(__FUNCTION__, "Error creating secure message generator for this packet.");
            return false;
        }
		tmpData.asyncState = FREE;
        asyncVerifDataPool_->add(tmpData);
    }
    return true;
}

asyncCbData_t* SecurityInterface::getFreeVerifData() {
    return asyncVerifDataPool_->getFreeVerifData();
}

int SecurityInterface::registerIdChange(const std::string lcmName, IDChangeData* idChangeData) {
    int result = -1;
    LOGD(__FUNCTION__, "Aerolink:: Lcm name is: ", lcmName);
    result =
        securityServices_idChangeRegister(
            secContext_,
            lcmName.c_str(),
            idChangeData,  // void* cb data struct - user data
            [](void *userData, uint8_t numCerts, uint8_t *certIndex) -> void {
                // on call back, this function provides the new cert index for
                // the complete id change cb fn
                static uint8_t rng_data = 0;
                int rng_ret = -1;
                auto app = static_cast<QUtils*>(userData);
                rng_ret = app->hwTRNGData(&rng_data, 1);
                if (rng_ret) {
                    LOGE(__FUNCTION__, "Failure in Random Number Generation for Cert ID");
                }
                rng_data = (rng_data % numCerts) + 1;
                LOGD(__FUNCTION__, "Random CertIndex within 1 to ", numCerts, " is : ", rng_data);
                memcpy(certIndex, &rng_data, sizeof(uint8_t));
            },
            [](AEROLINK_RESULT returnCode, void *userData,  uint8_t const *certId) -> void {
                // tells the user whether the id change was completed successfully or not.
                // completeChangeId_status = returnCode;
                // cast userdata to the user data type struct
                IDChangeData* tempIdChangePtr = (IDChangeData*)userData;
                if (returnCode == WS_SUCCESS) {
                    if (tempIdChangePtr && certId) {
                        memcpy(tempIdChangePtr->tempId, certId, sizeof(tempIdChangePtr->tempId));
                        memcpy(tempIdChangePtr->certId, certId, sizeof(tempIdChangePtr->certId));
                        // start at offset of 2 to get last 6 bytes
                        tempIdChangePtr->idChanged = true;
                    }
                } else{
                    LOGE(__FUNCTION__, "Failed to Perform ID Change");
                }

                // perform l2 src randomization
                LOGD(__FUNCTION__, "perform l2 src randomization");
            /*      for(int index = 0 ; index < spsTransmits.size(); index++){
                    this->spsTransmits[index].updateSrcL2();
                } */
                //retChangeId_status = true;
                if (tempIdChangePtr) {
                    sem_post(&tempIdChangePtr->idChangeSem);
                }
            }
            );
    if (result != WS_SUCCESS) {
        LOGE(__FUNCTION__, "Failed to register the ID change callback: ", ws_errid(result));
        return -1;
    } else {
        LOGI(__FUNCTION__, "Successful ID change callback registration");
    }
    return result;
}

void print_exception(std::exception& e) {
    LOGE(__FUNCTION__, "Exception caught : ", e.what());
}

// A function to extract the security header and cert from a signed packet
int SecurityInterface::extractMsg(SecuredMessageParserC* smp, const uint8_t * msg,
                                  uint32_t msgLen, uint8_t const *payload,
                                  uint32_t &payloadLen,
                                  uint32_t &dot2HdrLen) {
    // return nullptr if still nullptr
    if (smp == nullptr) {
        return -1;
    }

    int result = -1;
    // smp_extract
    PayloadType spduType, payloadType;
    uint8_t const *externData;
    ExternalDataHashAlg edhAlg;
    result = smp_extract(*smp, msg, msgLen,
                         &spduType, &payload, &payloadLen, &payloadType,
                         &externData, &edhAlg);
    if (result != WS_SUCCESS) {
        //TODO: replace the fprintf with LODE, for this whole source file
        if(secVerbosity > 4)
            LOGE(__FUNCTION__, " Unable to extract message: ", ws_errid(result));
        return -1;
    }
    dot2HdrLen = (uint32_t)(payload - msg);
    return result;
}

int SecurityInterface::setGenLocation(SecuredMessageParserC* smp,
                                      BsmData* rvBsmInfo) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if (smp == nullptr) {
        return -1;
    }

    result = smp_setGenerationLocation(*smp, rvBsmInfo->latitude,
                                       rvBsmInfo->longitude,
                                       rvBsmInfo->elevation);
    if (result != WS_SUCCESS) {
        if(secVerbosity > 4)
            LOGE(__FUNCTION__, " Unable to set the generation location: ", ws_errid(result));
        return -1;
    }
    return result;
}

int SecurityInterface::checkConsistency(SecuredMessageParserC* smp) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if (smp == nullptr) {
        return -1;
    }

    if (secVerbosity > 7) {
        LOGD(__FUNCTION__, " Now checking consistency of signed message");
    }
    // smp_checkConsistency
    if (enableConsistency){
        result = smp_checkConsistency(*smp);
        if (result != WS_SUCCESS){
            if (secVerbosity > 4)
                LOGE(__FUNCTION__, " Unable to check consistency: ", ws_errid(result));
            return -1;
        }
    }
    return WS_SUCCESS;
}

// A function to check consistency and relevancy of a signed packet
int SecurityInterface::checkRelevance(SecuredMessageParserC* smp) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if (smp == nullptr) {
        return -1;
    }

    if (secVerbosity > 7) {
        LOGD(__FUNCTION__, " Now checking relevance of signed message");
    }
    // smp_checkRelevance
    if (enableRelevance) {
        result = smp_checkRelevance(*smp);
        if (result != WS_SUCCESS) {
            if(secVerbosity > 4)
                LOGE(__FUNCTION__, " Unable to check relevance: ", ws_errid(result));
            return -1;
        }
    }
    return WS_SUCCESS;
}


// A function to synchronously verify a signed packet
int SecurityInterface::syncVerify(SecuredMessageParserC* smp,
                                  BsmData hvKine, BsmData rvKine,
                                  bool enableMbdCheck) {
    int result = -1;
    // set the generation position based on provided BsmData (if any)
    result = smp_setGenerationLocation(*smp, rvKine.latitude,
                                       rvKine.longitude,
                                       rvKine.elevation);
    if (result != WS_SUCCESS) {
        if (secVerbosity > 4)
            LOGE(__FUNCTION__, " Unable to set generation location: ", ws_errid(result));
    }

    if (secVerbosity > 7) {
        LOGD(__FUNCTION__, " Now checking relevance of signed message");
    }
    // smp_checkRelevance
    if (this->enableRelevance) {
        result = smp_checkRelevance(*smp);
        if (result != WS_SUCCESS) {
            if (secVerbosity > 4)
                LOGE(__FUNCTION__, " Unable to check relevance: ", ws_errid(result));
            return -1;
        }
    }

    if (secVerbosity > 7) {
        LOGD(__FUNCTION__, " Now checking consistency of signed message");
    }
    // smp_checkConsistency
    if (this->enableConsistency) {
        result = smp_checkConsistency(*smp);
        if (result != WS_SUCCESS) {
            if (secVerbosity > 4)
                LOGE(__FUNCTION__, " Unable to check consistency: ", ws_errid(result));
            return -1;
        }
    }

    // smp_verifySignatures
    result = smp_verifySignatures(*smp);
    if (result != WS_SUCCESS) {
        if (secVerbosity > 4)
            LOGE(__FUNCTION__, " Unable to call verify signature function: ", ws_errid(result));
        return -1;
    } else {
        //Misbehavior detection if enabled
        if (enableMbdCheck) {
            bsmMbdCheck(smp, &rvKine);
        }
   }
    return result;
}

asyncCbData_t* AsyncVerifDataPool::getFreeVerifData(){
    if (verifDataPoolFull) {
        LOGE(__FUNCTION__, " No more verif data available. Buffer full");
        return nullptr;
    }
    // here we technically need to find the next free slot
    int startIdx = freeVerifDataIndex;
	asyncCbData_t* tmpPtr = &asyncVerifDataPool_[freeVerifDataIndex];
	asyncVerifDataPool_[freeVerifDataIndex].index = freeVerifDataIndex;

    while (asyncVerifDataPool_[freeVerifDataIndex].asyncState != FREE &&
           freeVerifDataIndex < asyncVerifDataPoolSize) {
        freeVerifDataIndex = freeVerifDataIndex + 1;
        freeVerifDataIndex %= asyncVerifDataPoolSize;
		tmpPtr = &asyncVerifDataPool_[freeVerifDataIndex];
        // we have completed a full loop, alert caller that buffer is full
        if (freeVerifDataIndex == startIdx) {
            verifDataPoolFull = true;
            return nullptr;
        }
    }
	asyncVerifDataPool_[freeVerifDataIndex].index = freeVerifDataIndex;

	freeVerifDataIndex = (freeVerifDataIndex + 1) % asyncVerifDataPoolSize;
	tmpPtr->asyncState = VERIFYING;
    return tmpPtr;
}

// A function to asynchronously verify a signed packet
int SecurityInterface::asyncVerify(asyncCbData_t *asyncCbData,
                                   uint8_t priority,
                                   SecuredMessageParserC* smp,
                                   ValidateCallback callBackFunction) {

    if (smp == nullptr) {
        return -1;
    }

    int result = -1;
    uint8_t aerolinkPriority = (priority <= 4) ? 0 : 1;
    if (secVerbosity > 6){
        LOGD(__FUNCTION__, " Aerolink Verification Priority will be ", aerolinkPriority);
    }

    // async verification
    asyncCbData->asyncState = VERIFYING;
    result = smp_verifySignaturesAsyncPriority(*smp,
                                               aerolinkPriority,
                                               static_cast<void*>(asyncCbData),
                                               callBackFunction);
    if (result != WS_SUCCESS) {
        if (secVerbosity > 4) {
            LOGE(__FUNCTION__, " Unable to call verify signature function ", ws_errid(result));
        }
        return -1;
    }
    return WS_SUCCESS;
}

AEROLINK_RESULT SecurityInterface::bsmMbdCheck(SecuredMessageParserC* smp,
    BsmData* rvBsmInfo) {
    AEROLINK_RESULT result = WS_ERR_BAD_ARGS;

    MisbehaviorDetectedType misbehaviorResult;
    if(smp != nullptr){
        result = smp_checkMisbehavior(*smp, static_cast<void*>(rvBsmInfo),
                &misbehaviorResult);

        if (result != WS_SUCCESS && result != WS_ERR_MISBEHAVIOR_DETECTED){
            if(secVerbosity > 4)
                LOGE(__FUNCTION__, " Error in checking misbehavior");
        } else {
            if (secVerbosity > 4) {
                LOGE(__FUNCTION__, " Detected Misbehavior Class is ",
                     misbehaviorResult.detectedMisbehavior);
            }
        }
    }
    return result;
}

/* SIGNING FUNCTIONS */
// Sign and return signed message
int SecurityInterface::signMsg(SecuredMessageParserC* smg,  const uint8_t *msg, uint32_t msgLen,
                               uint8_t *signedSpdu, uint32_t &signedSpduLen,
                               uint32_t psidValue, bool enableSsp,
                               const uint8_t* sspValue, uint32_t sspLength, bool enableSspMask,
                               const uint8_t* sspMaskValue,
                               uint32_t sspMaskLength, SignerTypeOverride signType) {
    int result = -1;
    if (smg == nullptr) {
        if (secVerbosity > 7)
            LOGE(__FUNCTION__, " Unable to retreive smg for this thread");
        return -1;
    }

    if (secVerbosity > 7) {
        LOGD(__FUNCTION__, " Now setting the provider service id and permissions");
        LOGD(__FUNCTION__, " User provided psid is: ", psidValue);
        LOGD(__FUNCTION__, " User provided ssp length is ", sspLength);
        LOGD(__FUNCTION__, " User provided ssp mask length is ", sspMaskLength);
        LOGD(__FUNCTION__, " Setting the signing permissions");
        LOGD(__FUNCTION__, " Signing permissions: psid = ", psidValue);
        if (sspLength > 0) {

            std::stringstream ss;
            ss << " SSP Length: " << sspLength;
            ss << "SSP = ";
            ss << std::hex;
            for (size_t j = 0; j < sspLength; ++j) {
                ss << std::setw(2) << sspValue[j];
            }
            LOGD(__FUNCTION__, ss.str());
        }
        if (sspMaskLength > 0) {
            std::stringstream ss;
            ss << " SSP Mask Length: " << sspMaskLength;
            ss << "SSPMask = ";
            ss << std::hex;
            for (size_t j = 0; j < sspMaskLength; ++j) {
                ss << std::setw(2) << sspMaskValue[j];
            }
            LOGD(__FUNCTION__, ss.str());
        }
    }

    // Set signing permissions
    uint8_t externalDataHash[31];
    const AerolinkEncryptionKey* pubEncryptKey = NULL;
    SigningPermissions permissions;
    setPermissions(&permissions, psidValue, (enableSsp) ? sspValue : NULL,
                   (enableSspMask) ? sspMaskValue : NULL, sspLength);
    //synchronous signing
    result = smg_sign(*smg,
                      permissions,
                      signType,
                      0,
                      msg,
                      msgLen,
                      0,
                      externalDataHash,
                      EDHA_NONE,
                      pubEncryptKey, // for receiver to reply with encrypted msg
                      signedSpdu,
                      &signedSpduLen);
    // Check if the sign function successfully ran
    if (result != WS_SUCCESS) {
        LOGE(__FUNCTION__, " Failed to sign the message, error= ", result, "(", ws_errid(result), ")");
        return -1;
    }
    if (secVerbosity > 7) {
        LOGI("Signing successful");
    }
    return WS_SUCCESS;
}

} //namespace Core
} //namespace SPLITS
