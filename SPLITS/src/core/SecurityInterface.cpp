/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sys/time.h>
#include "qUtils.hpp"
#include "SecurityInterface.hpp"
#include "utils.h"

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
void SecurityInterface::setSecVerbosity(uint8_t verbosity){
    secVerbosity = verbosity;
}

void SecurityInterface::printBytes(char *label, uint8_t buffer[], uint32_t length)
{
    uint32_t i = 0;

    printf("%s (%d bytes): ", label, length);

    for( i = 0; i < length; i++ ) {
        printf("%02x ", buffer[i] & 0xffU);
    }

    printf("\n");
}

void SecurityInterface::printErrorMsg (std::string errMsg, AEROLINK_RESULT result){
    fprintf(stderr, "%s=%d(%s)\n", errMsg, result, ws_errid(result));
}

int SecurityInterface::setLeapSeconds(uint32_t leapSeconds){
    int result = -1;
    /*
     * Adjust the time for the expiration of signatures and certificates
     */
    if ((result = securityServices_setTimeAdjustment(leapSeconds)) != WS_SUCCESS) {
        std::cerr << "securityServices_setTimeAdjustment failed: " << result << std::endl;
    }
    return result;
}

void SecurityInterface::onDetailedLocationUpdate
    (const shared_ptr<ILocationInfoEx> &locationInfo)
{
    int result = -1;
    if(locationInfo){
        std::cout << "Updating security position\n";
        {
        std::lock_guard<std::mutex> lk(locationMutex_);
        hvSecLocation_.latitude = locationInfo->getLatitude();
        hvSecLocation_.longitude = locationInfo->getLongitude();
        hvSecLocation_.elevation = locationInfo->getHeading();
        result = securityServices_setCurrentLocation(
                 hvSecLocation_.latitude, hvSecLocation_.longitude,
                 hvSecLocation_.elevation, countryCode_);
        if(result != WS_SUCCESS){
            std::cerr << "Location not updated successfully\n";
        }
        }
    }
}

bool SecurityInterface::changeIdNow(){
    //if(!criticalState){
        // here we need to check for two things:
        // interval has passed and distance has been covered since last id change
        uint64_t currTime = timestamp_now();
        if(currTime > lastIdChangeTime_) {
            timeSinceLastIdChange_ = currTime - lastIdChangeTime_;
        }else{
            timeSinceLastIdChange_ = 0;
        }
        if(!initIdChange_){
            std::cout << "Id change has not yet been initialized\n";
            return false;
        }
        {
        std::lock_guard<std::mutex> lk(locationMutex_);
        double hvLatNew = hvSecLocation_.latitude;
        double hvLonNew = hvSecLocation_.longitude;
        double hvLatOld = lastHvSecLocIdChange_.latitude;
        double hvLonOld = lastHvSecLocIdChange_.longitude;
        // check if there have been any fixes yet
        if(initIdChange_){
            distSinceLastIdChange_ =
                 splitsFilter_->bsmCompute2dDistance(hvLatOld, hvLonOld, hvLatNew, hvLonNew);
/*             std::cout << "Curr lat and long are: " <<
                hvLatNew << ", " << hvLonNew << "\n";
            std::cout << " Last lat and long are: " <<
                hvLatOld << ", " << hvLonOld << "\n";
            std::cout << "Dist since last id change is now: " <<
                timeSinceLastIdChange_ << "\n";
            std::cout << "Time since last id change is now: " <<
                timeSinceLastIdChange_ << "\n"; */
        }else{
            timeSinceLastIdChange_ = 0;
        }
        }
        // check if both conditions satisfied
        if(timeSinceLastIdChange_ >= idChangeInterval &&
            distSinceLastIdChange_ >= idChangeDistance){
            // perform id change and record current position and time
            return true;
        }
    //}
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

int SecurityInterface::idChange (){

    int result = -1;
    result = securityServices_idChangeInit();
    if(result != WS_SUCCESS){
        return -1;
    }
    // on successful return, modify other id-related information in upper layer
    return result;
}

/* INITIALIZATION/DEINITIALIZATION FUNCTIONS */
SecurityInterface::SecurityInterface(const std::string ctxName, uint16_t countryCode){
    if(init(ctxName, countryCode) < 0) {
        throw std::runtime_error("SecurityInterface Init Failed\n");
    }
    // should  perform sanitary check on the value
}


// Primary initialization function for Aerolink services
int SecurityInterface::init(const std::string ctxName, uint16_t countryCode) {
    const char *aerolinkLibVersion = securityServices_getVersion();
    if(aerolinkLibVersion != nullptr) {
        std::cout << "Aerolink Library Version: " << aerolinkLibVersion << std::endl;
    }

    int result = -1;
    if ((result = securityServices_initialize()) != WS_SUCCESS) {
        fprintf(stderr, "securityServices initialization failed (%s)\n",
            ws_errid(result));
        return -1;
    }

    // May add the generator location here as well
    std::cout << "Security context name is: " << ctxName << "\n";
    result = sc_open(ctxName.c_str(), &secContext_);
    if (result != WS_SUCCESS) {
        fprintf(stderr,
                "Failed to open security context (%s)\n",
                ws_errid(result));
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
        if(!initIdChange_){
            std::cout << "Setting init id change to true\n";
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
    fprintf(stdout,"Aerolink deinitializing\n");
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
    uint32_t               sspLen)
{
    p->psid = psid;
    p->ssp = ssp;
    p->sspMask = sspMask;
    p->isBitmappedSsp = (NULL == sspMask) ? 0 : 1;
    p->sspLen = sspLen;
}


// Function to create a new SecuredMessageParser object -- for verification
int SecurityInterface::createNewSmp(SecuredMessageParserC* smpPtr){
    int result = -1;
    result = smp_new(secContext_, smpPtr);
    if (result != WS_SUCCESS){
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to create secure message parser (%s)\n",
                     ws_errid(result));
        return -1;
    }
    return result;
}

// Function to create a new SecuredMessageGenerator object -- for signing
int SecurityInterface::createNewSmg(SecuredMessageGeneratorC* smgPtr){
    int result = -1;
    result = smg_new(secContext_, smgPtr);
    if (result != WS_SUCCESS) {
        if(secVerbosity > 7)
            fprintf(stderr,"Unable to create secure message generator (%s)\n",
                     ws_errid(result));
        return -1;
    }
    return result;
}

bool SecurityInterface::initVerifDataPool(int dataPoolSize){
    // create dataPoolSize entries for the data struct
    for (int i = 0; i < PP_BUFFER_MAX_SIZE; i++) {
        asyncCbData_t tmpData;
        if(createNewSmp(
            (SecuredMessageParserC*)(&tmpData.smp)) <= -1){
            printf("Error creating secure message generator for this packet.\n");
            return false;
        }
        tmpData.asyncState = FREE;
        asyncVerifDataPool_->add(tmpData);
    }
    return true;
}

asyncCbData_t* SecurityInterface::getFreeVerifData(){
    return asyncVerifDataPool_->getFreeVerifData();
}

int SecurityInterface::registerIdChange(const std::string lcmName, IDChangeData* idChangeData){
    int result = -1;
    fprintf(stdout, "Aerolink:: Lcm name is: %s\n", lcmName);
    result = securityServices_idChangeRegister(
                secContext_,
                lcmName.c_str(),
                idChangeData,  // void* cb data struct - user data
                [](void *userData, uint8_t numCerts, uint8_t *certIndex) -> void {
                    // on call back, this function provides the new cert index for the complete id change cb fn
                    static uint8_t rng_data = 0;
                    int rng_ret = -1;
                    auto app = static_cast<QUtils*>(userData);
                    rng_ret = app->hwTRNGChar(&rng_data);
                    if(rng_ret){
                        fprintf(stderr,"Failure in Random Number Generation for Cert ID \n");
                    }
                    rng_data = (rng_data % numCerts) + 1;
                    std::cout << "Random CertIndex within 1 to " << numCerts << " is : " << rng_data << std::endl;
                    memcpy(certIndex,&rng_data,sizeof(uint8_t));
                   },
                [](AEROLINK_RESULT returnCode, void *userData,  uint8_t const *certId) -> void {
                    // tells the user whether the id change was completed successfully or not.
                    //completeChangeId_status = returnCode;
                    // cast userdata to the user data type struct
                    IDChangeData* tempIdChangePtr = (IDChangeData*)userData;
                    if(returnCode == WS_SUCCESS){
                        if(tempIdChangePtr && certId){
                            memcpy(tempIdChangePtr->tempId, certId, sizeof(tempIdChangePtr->tempId)); // start at offset of 2 to get last 6 bytes
                            tempIdChangePtr->idChanged = true;
                        }
                    }
                    else{
                        std::cerr << "Failed to Perform ID Change" <<std::endl;
                    }

                    // perform l2 src randomization
                    std::cout << "perform l2 src randomization\n";
                /*      for(int index = 0 ; index < spsTransmits.size(); index++){
                        this->spsTransmits[index].updateSrcL2();
                    } */
                    //retChangeId_status = true;
                    if(tempIdChangePtr){
                        sem_post(&tempIdChangePtr->idChangeSem);
                    }
                   }
                );
    if(result != WS_SUCCESS) {
        std::cerr << "Failed to register the ID change callback: "
                    << ws_errid(result) << std::endl;
        return -1;
    }else{
        fprintf(stdout, "Successful ID change callback registration\n");
    }
    return result;
}

void print_exception(std::exception& e){
    fprintf(stderr, "Exception caught : %s\n", e.what());
}

// A function to extract the security header and cert from a signed packet
int SecurityInterface::extractMsg(SecuredMessageParserC* smp, const uint8_t * msg,
                    uint32_t msgLen, uint8_t const *payload, uint32_t &payloadLen,
                    uint32_t &dot2HdrLen){
    // return nullptr if still nullptr
    if(smp == nullptr){
        return -1;
    }

    int result = -1;
    // smp_extract
    PayloadType    spduType, payloadType;
    uint8_t const *externData;
    ExternalDataHashAlg edhAlg;
    result = smp_extract(
        *smp, msg, msgLen,
        &spduType, &payload, &payloadLen, &payloadType,
        &externData, &edhAlg);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to extract message (%s)\n", ws_errid(result));
        return -1;
    }
    dot2HdrLen = (uint32_t)(payload - msg);
    return result;
}

int SecurityInterface::setGenLocation
                        (SecuredMessageParserC* smp, BsmData* rvBsmInfo) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if(smp == nullptr){
        return -1;
    }

    result = smp_setGenerationLocation(*smp, rvBsmInfo->latitude,
            rvBsmInfo->longitude, rvBsmInfo->elevation);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to set the generation location (%s)\n", ws_errid(result));
        return -1;
    }
    return result;
}

int SecurityInterface::checkConsistency
                        (SecuredMessageParserC* smp) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if(smp == nullptr){
        return -1;
    }

    if(secVerbosity > 7) {
        fprintf(stdout, "Now checking consistency of signed message\n");
    }
    // smp_checkConsistency
    if(enableConsistency){
        result = smp_checkConsistency(*smp);
        if(result != WS_SUCCESS){
            if(secVerbosity > 4)
                fprintf(stderr,"Unable to check consistency (%s)\n", ws_errid(result));
            return -1;
        }
    }
    return WS_SUCCESS;
}

// A function to check consistency and relevancy of a signed packet
int SecurityInterface::checkRelevance
                        (SecuredMessageParserC* smp) {
    // Add new smp (if none exists) for this thread
    int result = -1;
    if(smp == nullptr){
        return -1;
    }

    if(secVerbosity > 7) {
        fprintf(stdout, "Now checking relevance of signed message\n");
    }
    // smp_checkRelevance
    if(enableRelevance){
        result = smp_checkRelevance(*smp);
        if (result != WS_SUCCESS)
        {
            if(secVerbosity > 4)
                fprintf(stderr,"Unable to check relevance (%s)\n", ws_errid(result));
            return -1;
        }
    }
    return WS_SUCCESS;
}


// A function to synchronously verify a signed packet
int SecurityInterface::syncVerify(
    SecuredMessageParserC* smp,
    BsmData hvKine, BsmData rvKine,
    bool enableMbdCheck
    )
{
    int result = -1;

    // set the generation position based on provided BsmData (if any)
    result = smp_setGenerationLocation(*smp, rvKine.latitude, rvKine.longitude,
            rvKine.elevation);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to set generation location (%s)\n", ws_errid(result));
    }

    if(secVerbosity > 7) {
        fprintf(stdout, "Now checking relevance of signed message\n");
    }
    // smp_checkRelevance
    if(this->enableRelevance){
        result = smp_checkRelevance(*smp);
        if (result != WS_SUCCESS)
        {
            if(secVerbosity > 4)
                fprintf(stderr,"Unable to check relevance (%s)\n", ws_errid(result));
            return -1;
        }
    }

    if(secVerbosity > 7) {
        fprintf(stdout, "Now checking consistency of signed message\n");
    }
    // smp_checkConsistency
    if(this->enableConsistency){
        result = smp_checkConsistency(*smp);
        if(result != WS_SUCCESS){
            if(secVerbosity > 4)
                fprintf(stderr,"Unable to check consistency (%s)\n", ws_errid(result));
            return -1;
        }
    }

    // smp_verifySignatures
    result = smp_verifySignatures(*smp);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4)
            fprintf(stderr,"Unable to call verify signature function (%s)\n",
                     ws_errid(result));
        return -1;
    }else{
        //Misbehavior detection if enabled
        if(enableMbdCheck){
            bsmMbdCheck(smp, &rvKine);
        }
   }
    return result;
}

asyncCbData_t* AsyncVerifDataPool::getFreeVerifData(){
    if(verifDataPoolFull){
        std::cerr << "No more verif data available. Buffer full\n";
        return nullptr;
    }
    // here we technically need to find the next free slot
    int startIdx = freeVerifDataIndex;
//  std::cout << " free verif data index before getfreeverif is: " << freeVerifDataIndex << "\n";
    asyncCbData_t* tmpPtr = &asyncVerifDataPool_[freeVerifDataIndex];
    //std::cout << "Size of verif data pool : " << asyncVerifDataPoolSize << "\n";
    asyncVerifDataPool_[freeVerifDataIndex].index = freeVerifDataIndex;
    while(asyncVerifDataPool_[freeVerifDataIndex].asyncState != FREE &&
        freeVerifDataIndex < asyncVerifDataPoolSize){
        freeVerifDataIndex = freeVerifDataIndex + 1;
        freeVerifDataIndex%=asyncVerifDataPoolSize;
        tmpPtr = &asyncVerifDataPool_[freeVerifDataIndex];
        // we have completed a full loop, alert caller that buffer is full
        if(freeVerifDataIndex == startIdx){
            verifDataPoolFull = true;
            return nullptr;
        }
    }
    asyncVerifDataPool_[freeVerifDataIndex].index = freeVerifDataIndex;
    //std::cout << " free verif data index from getfreeverif is: " << freeVerifDataIndex << "\n";
    freeVerifDataIndex = (freeVerifDataIndex + 1) % asyncVerifDataPoolSize;
    tmpPtr->asyncState = VERIFYING;
    return tmpPtr;
}

// A function to asynchronously verify a signed packet
int SecurityInterface::asyncVerify(asyncCbData_t *asyncCbData,
                uint8_t priority, SecuredMessageParserC* smp,
                ValidateCallback callBackFunction) {

    if(smp == nullptr){
        return -1;
    }

    int result = -1;
    uint8_t aerolinkPriority = (priority <= 4) ? 0 : 1;
    if (secVerbosity > 6){
        printf("Aerolink Verification Priority will be %d \n", aerolinkPriority);
    }

    // async verification
    asyncCbData->asyncState = VERIFYING;
    result = smp_verifySignaturesAsyncPriority
        (*smp, aerolinkPriority, static_cast<void*>(asyncCbData), callBackFunction);
    if (result != WS_SUCCESS)
    {
        if(secVerbosity > 4){
            fprintf(stderr,"Unable to call verify signature function (%s)\n",
                     ws_errid(result));
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
                fprintf(stderr, "Error in checking misbehavior\n");
        }else{
            if(secVerbosity > 4){
                fprintf(stdout, "Detected Misbehavior Class is %llu\n",
                    misbehaviorResult.detectedMisbehavior);
            }
        }
    }
    return result;
}

/* SIGNING FUNCTIONS */
// Sign and return signed message
int SecurityInterface::signMsg(SecuredMessageParserC* smg,  const uint8_t *msg, uint32_t msgLen,
                              uint8_t *signedSpdu, uint32_t &signedSpduLen, uint32_t psidValue, bool enableSsp,
                              const uint8_t* sspValue, uint32_t sspLength, bool enableSspMask,
                              const uint8_t* sspMaskValue, uint32_t sspMaskLength, SignerTypeOverride signType) {
    int result = -1;
    if(smg == nullptr){
        if(secVerbosity > 7)
            fprintf(stderr,
                "Unable to retreive smg for this thread\n");
        return -1;
    }

    if(secVerbosity > 7) {
        fprintf(stdout, "Now setting the provider service id and permissions\n");
        fprintf(stdout, "User provided psid is: %02x\n", psidValue);
        fprintf(stdout, "User provided ssp length is %d\n", sspLength);
        fprintf(stdout, "User provided ssp mask length is %d\n", sspMaskLength);
        fprintf(stdout, "\nSetting the signing permissions\n");
        fprintf(stdout, "Signing permissions: psid = 0x%02x\n", psidValue);
        if(sspLength>0){
            printf("SSP Length: %d\n", sspLength);
            printf("SSP = ");
            for (size_t j = 0; j < sspLength; ++j)
            {
                fprintf(stdout,"%02x", sspValue[j]);
            }
            std::cout << std::endl;
        }
        if(sspMaskLength>0)
        {
            printf("SSP Mask Length: %d\n", sspMaskLength);
            printf("SSPMask = ");
            for (size_t k = 0; k < sspMaskLength; ++k)
            {
                fprintf(stdout,"%02x", sspMaskValue[k]);
            }
            std::cout << std::endl;
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
            fprintf(stderr, "Failed to sign the message, error=%d(%s)\n",
                result, ws_errid(result));
        return -1;
    }
    if(secVerbosity > 7){
        fprintf(stdout,"Signing successful\n");
    }
    return WS_SUCCESS;
}

} //namespace Core
} //namespace SPLITS