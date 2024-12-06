/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <syslog.h>
#include <cstdlib>
#include <cstring>
#include "SplitsCoreFactory.hpp"
#include "SplitsCodecFactory.hpp"
#include "SplitsCoreImpl.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include "SplitsConfigStubImpl.hpp"
#include "SplitsDataPublisher.hpp"
#include "utils.h"
#include "v2xMsgSae.hpp"

#define RX_POOL_SIZE    6000
#define DEFAULT_BSM_PRIORITY    5

using SPLITS::Codec::SplitsCodecFactory;
using SPLITS::Codec::SplitsCodec;
using SPLITS::Core::Role;
using SPLITS::MsgSource;
using SPLITS::MsgFormat;
using namespace telux::cv2x::prop;
//Testing variables
#include <cstdio>
char gRxBuf[512];
char gTxBuf[512];
int gRxBufLen;
int gTxBufLen;
std::atomic<uint64_t> totalVerifSuccess {0};
std::atomic<uint64_t> totalExtracted {0};
std::atomic<uint64_t> lastTimeStamp {0};
std::atomic<uint64_t> numPublished {0};
std::atomic<uint64_t> numRxed {0};
namespace SPLITS {

namespace Core {

int SplitsCoreImpl::readPduFromFile(std::string file, char *buf, int bufLen)
{
     int ret = 0;
     size_t len;

    if (file.empty()) {
        return -1;
    }

    FILE* fp = fopen(file.c_str(), "r");
    if (!fp) {
        std::cout <<"failed to open file" << file << std::endl;
        return -1;
    }

    len = fread(buf, 1, bufLen, fp);

    fclose(fp);

    ret = len > 0 ? len : -1;
    return ret;
}
int SplitsCoreImpl::initRadio()
{
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;
    const SplitsConfig::CoreConfig & coreCfg = configData_->getCoreConfig();

    //Initialize TxSps
    for (auto const & txSpsFlow : coreCfg.getSpsFlows()) {
        spsInfo.periodicityMs = adjustSpsPeriodicity(txSpsFlow.getPeriodicity());
        //use default priority and reservation size
        RadioTransmit tx(spsInfo, TrafficCategory::SAFETY_TYPE,  TrafficIpType::TRAFFIC_NON_IP,
                txSpsFlow.getSourcePort(), txSpsFlow.getServiceId());
        if (tx.flow) {
            tx.configureIpv6(txSpsFlow.getDestPort(), txSpsFlow.getDestAddr().c_str());
            spsTransmits_.emplace(txSpsFlow.getServiceId(), std::move(tx));
        } else {
            cerr << "SplitsCoreImpl::init: init error in creating Tx SPS flow!" <<
                    " with spsServiceId: " << txSpsFlow.getServiceId() << endl;
            return -1;
        }
    }
    //Initialize TxEvent
    for (auto const &txEventFlow : coreCfg.getEventFlows()) {
        RadioTransmit tx(eventInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         txEventFlow.getSourcePort(), txEventFlow.getServiceId());
        if (tx.flow) {
            tx.configureIpv6(txEventFlow.getDestPort(), txEventFlow.getDestAddr().c_str());
            eventTransmits_.emplace(txEventFlow.getServiceId(), std::move(tx));
        } else {
            cerr << "SplitsCoreImpl::init error in creating Tx event flow!" <<
                    " with serviceID " << txEventFlow.getServiceId() << endl;
            return -1;
        }
    }
    //Initialize Rx
    if (coreCfg.getWildCardRx() == false) {
        //multiple Sid may/may not use same port for Rx.
        std::map<uint16_t, std::vector<uint32_t>> rxPortsIds;
        for (auto const & rxFlow : coreCfg.getRxFlows()) {
            std::vector<uint32_t> ids;
            auto port = rxFlow.getReceivePort();
            if (rxPortsIds.find(port) != rxPortsIds.end()) {
                rxPortsIds[port].push_back(rxFlow.getServiceId());
            } else {
                ids.push_back(rxFlow.getServiceId());
                rxPortsIds[port] = ids;
            }
        }
        for (auto const & rxPortId : rxPortsIds) {
            RadioReceive rx(TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                    rxPortId.first, std::make_shared<std::vector<uint32_t>>(rxPortId.second));
            if (rx.gRxSub != nullptr) {
                radioReceives_.push_back(rx);
            } else {
                cerr << "SplitsCoreImpl::init error in creating Rx subscription!";
                cerr << " with receiveSubIds: ";
                for (auto id : rxPortId.second) {
                    std::cerr << "" << id << ", ";
                }
                std::cerr << "" << std::endl;
                return -1;
            }
        }
    } else {
        uint16_t port = 9000;
        cout << "Creating wildcard Rx subscription on port 9000" << endl;
        RadioReceive rx(TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP, port, nullptr);
        if (rx.gRxSub != nullptr) {
            radioReceives_.push_back(rx);
        } else {
            cerr << "SplitsCoreImpl::init error in creating wildcard Rx subscription!";
            return -1;
        }
    }

    return 0;
}

int SplitsCoreImpl::initSecurity(const std::string ctxName,
    const uint16_t countryCode, const std::string lcmName){
    try{
        secInterface_ = std::make_shared<SecurityInterface>
            (ctxName, countryCode);
    }catch(const std::runtime_error& error){
        std::cerr << "Failure in creating security interface" << std::endl;
        return -1;
    }

    sem_init(&idChangeData_.idChangeSem, 0, 1);
    if(secInterface_->registerIdChange(lcmName, &idChangeData_)){
        std::cerr << "Failure in registering id change" << std::endl;
        return -1;
    }

    if(secInterface_->createNewSmg(&smg_) < 0){
        std::cerr << "Failure in creating a secure message generator" << std::endl;
        return -1;
    }
    return 0;

}

int SplitsCoreImpl::initMsgs(void) {
    int i = 0;
    std::shared_ptr<v2xMsg> msg = nullptr;
    const SplitsConfig::CoreConfig &coreCfg = configData_->getCoreConfig();

    //rx msgs
    for (i = 0; i < RX_POOL_SIZE; i++) {
        if (st_ == StackType::STACK_TYPE_SAE) {
            msg = std::make_shared<v2xMsgSae>();
        } else {
            //TODO: Add CSAE message object
        }
        msg->head = msg->buf = (char *)malloc(RX_BUF_LEN);
        msg->bufLen = RX_BUF_LEN;
        putFreeMsg(msg);
    }
    //tx sps msgs
    for (auto const &spsTx : coreCfg.getSpsFlows()) {
        if (st_ == StackType::STACK_TYPE_SAE) {
            msg = std::make_shared<v2xMsgSae>();
            msg->head = msg->buf = (char *)malloc(TX_BUF_LEN);
            msg->bufLen = TX_BUF_LEN;
            txSpsMsgs_[spsTx.getServiceId()] = msg;
        } else {
            //TODO: Add CSAE message objects
        }
    }
    //tx event msgs
    for (auto const &eventTx : coreCfg.getEventFlows()) {
        if (st_ == StackType::STACK_TYPE_SAE) {
            msg = std::make_shared<v2xMsgSae>();
            msg->head = msg->buf = (char *)malloc(TX_BUF_LEN);
            msg->bufLen = TX_BUF_LEN;
            txEventMsgs_[eventTx.getServiceId()] = msg;
        } else {
            //TODO: Add CSAE message objects
        }
    }

    return 0;
}
int SplitsCoreImpl::adjustSpsPeriodicity(int intervalMs) {
    if (intervalMs < 50) {
        return 20;
    } else if (intervalMs < 100) {
        return 50;
    }
    int ret = intervalMs / 100;
    if (ret >= 10) {
        return 1000;
    }
    return (ret * 100);
}
int SplitsCoreImpl::init(std::string cfgFileName, std::string rxpduFileName, std::string txpduFileName)
{
    bool useLocalConfig = true;
    configData_ = std::make_shared<SplitsConfig::SplitsConfigData>();
    configIdl_ = std::make_shared<SplitsConfigIdl>(configData_);
#if 0
    //Initialize proxy to read config from client side.
    if (!configIdl_->init()) {
        if (configIdl_->loadConfigFromIdl() < 0) {
            useLocalConfig = true;
        }
    } else {
        useLocalConfig = true;
    }
#endif
    if (useLocalConfig == true) {
        configFile_ = std::make_shared<SplitsConfigFile>(configData_);
        if (cfgFileName.empty()) {
            std::cerr << "config file name not specified" << std::endl;
            return -1;
        } else if (configFile_->loadConfigFromFile(cfgFileName) < 0) {
            std::cerr << "Faild to load or parse config file" << std::endl;
            return -1;
        }
    }
    const SplitsConfig::CoreConfig &coreCfg = configData_->getCoreConfig();
    role_ = static_cast<Role>(coreCfg.getRole());

    //pdu file is for testing of codec.
    if (rxpduFileName.empty() == false) {
         if ((gRxBufLen = readPduFromFile(rxpduFileName, gRxBuf, 512)) < 0) {
                 std::cerr << "Failed to read file" << std::endl;
                 return -1;
             }
         useRxPduFile_ = true;
         } else {
             if (initRadio() < 0) {
                 std::cerr << "Faled to init radio" << std::endl;
                 return -1;
             }
         }
    if (txpduFileName.empty() == false) {
        if ((gTxBufLen = readPduFromFile(txpduFileName, gTxBuf, 512)) < 0) {
            std::cerr << "Failed to read file" << std::endl;
            return -1;
        }
        useTxPduFile_ = true;
    }

    // Initialize Tx/Rx messages
    initMsgs();

    // Codec
    SplitsCodecFactory codecFactory;

    if (st_ == StackType::STACK_TYPE_SAE) {
        codec_ = codecFactory.getCodec(SPLITS::StackType::STACK_TYPE_SAE);
    } else {
        codec_ = codecFactory.getCodec(SPLITS::StackType::STACK_TYPE_CSAE);
    }

    if (codec_->init(LOG_DEBUG, false)) {
        std::cerr << "Failed to init codec" << std::endl;
        return -1;
    }

    std::vector<std::shared_ptr<ILocationListener>> locListeners;

    //message generator(BSM)
    bsmGenerator_ = std::make_shared<BsmMsgGenerator>(getptr());
    bool useCan = true;
    if (coreCfg.getVehicleDataSource() == 1) {
        useCan = false;
    }
    if (bsmGenerator_->init(useCan) != Status::SUCCESS) {
        std::cerr << "Faild to init bsm generator" << std::endl;
        return -1;
    }
    locListeners.push_back(bsmGenerator_);

   // if (configData_->getCoreConfig().getEnableCongCtrl() == true) {
       std::cout << "making cong ctrl\n";
        congestCtrl_ = std::make_shared<CongestCtrl>();
        locListeners.push_back(congestCtrl_);
   // }
    //location
    kinematic_ = std::make_shared<KinematicsReceive>(locListeners, 100);
    //tx Scheduler
    txScheduler_ = std::make_shared<TxScheduler>(getptr());

    //security interface
    if(coreCfg.getEnableSecurity()){
        auto secConfig = configData_->getSecConfig();
        if(initSecurity(secConfig.getSecurityCtxName(),
                secConfig.getCountryCode(), secConfig.getLocalCertMgrName()) < 0){
            return -1;
        }
    }

    //WRA
    if (coreCfg.getEnableWra()) {
        std::cout << "WRA enabled" << std::endl;
        wra_ = std::make_shared<SplitsWra>(role_,
                std::make_shared<RadioInterface>(radioReceives_[0]),
                std::make_shared<SplitsConfig::WraConfig>(configData_->getWraConfig()));
        if (wra_ == nullptr) {
            std::cerr << "failed to init wra object" << std::endl;
            return -1;
        }
    } else {
        std::cout << "WRA not enabled" << std::endl;
    }
    //RTCM
    if (coreCfg.getEnableRTCM()) {
        std::cout << "role: " << static_cast<std::underlying_type<SPLITS::Core::Role>::type>(role_) << std::endl;
        rtcm_ = std::make_shared<SplitsRTCM>(role_,
                std::make_shared<SplitsConfig::RTCMConfig>(configData_->getRtcmConfig()));
        if (rtcm_->init()) {
            std::cerr << "Failed to init RTCM" << std::endl;
            return -1;
        }
    }
    //dds
    publisher_ = std::make_shared<SplitsServiceDataPublisher>();
    if (publisher_->init(getptr(), std::string("SERVICE")) == false) {
        std::cerr << "Failed to init data publisher" << std::endl;
        return -1;
    }
    subscriber_ = std::make_shared<SplitsDataSubscriber>();
    if (subscriber_->init(std::string("CLIENT")) == false) {
        std::cerr << "Failed to init data subscriber" << std::endl;
        return -1;
    }
    //register a listener for receiving message from EAP
    dataListener_ = std::make_shared<SplitsCoreDataListener>(getptr());
    subscriber_->registerListener(dataListener_);

    return 0;
}
int SplitsCoreImpl::receive(const uint8_t rxid, const uint16_t bufLen)
{
    int ret = 0;
    auto rxMsg = getFreeMsg();

    if (rxMsg == nullptr) {
        return -1;
    }
    rxMsg->macAddrLen_ = CV2X_MAC_ADDR_LEN;
    if (useRxPduFile_ == true) {
        rxMsg->buf = gRxBuf;
        rxMsg->bufLen = gRxBufLen;
    } else {
         ret = radioReceives_[rxid].receive(rxMsg->buf, MAX_PACKET_LEN, rxMsg->srcMac_, rxMsg->macAddrLen_);
         if (ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN ) {
            goto exit;
        }
        rxMsg->bufLen = ret;
    }
    enqueueMsg(rxMsg);
    numRxed++;
    return 0;
exit:

    putFreeMsg(rxMsg);
    // only queue a message structure if it was actually valid
    // a return value of 0 means that polling just expired
    if(ret != 0){
        enqueueMsg(rxMsg);
    }
    return ret;
}
int SplitsCoreImpl::processReceive(void)
{
    int ret = 0;
    char* bufStart;
    uint64_t time_start_ms, time_finish_ms, time_start_us, time_finish_us;
    // process all queued rxed messages
    while(stop_ == false) {

        auto rxMsg = dequeueMsg();
        if(rxMsg == nullptr){
            ret = -1;
            goto handleMsg;
        }

        // enum class MsgFormat {
        /*     MSG_FORMAT_BLOB,     //encoded, signed, with network and transport header.
        MSG_FORMAT_SIGNED,   //encoded, signed, no network and transport header.
        MSG_FORMAT_UNSIGNED, //encoded, but not signed, no network and transport header.
        MSG_FORMAT_UNENCODED, //unencoded, unsigned, no network and transport header.
        MSG_FORMAT_UNKNOWN   //don't know */

        // todo... detect that this is a secured frame
        // or just assume by default that it is secured and if not secured, reject it
        asyncCbData_t* asyncCbData;
        bufStart = rxMsg->buf;
        if(configData_->getCoreConfig().getEnableSecurity() && secInterface_ != nullptr){
            // find an available callback data struct for this verification
            asyncCbData = secInterface_->getFreeVerifData();
            // extract the ieee 1609.2 header
            if(asyncCbData == nullptr){
                std::cout << "Invalid async cb for this msg\n";
                ret = -1;
                goto handleMsg;
            }
            uint8_t const *payload = NULL;
            uint32_t payloadLen = 0;
            uint32_t dot2HdrLen = 0;

            // just decode wsmp
            int msgType = 0;
            int origLen = rxMsg->bufLen;
            msgType = codec_->decodeWsmp(rxMsg);
            int offset = origLen - rxMsg->bufLen;
            if(msgType == -1){
                std::cerr <<" Error decoding wsmp header of signed pdu\n";
                ret = -1;
                goto handleMsg;
            }
            uint32_t sizeBeforeExtract = rxMsg->bufLen;
            // provide the details of the message to aerolink
            // buffer length needs to be large enough...
            // to do... somehow when an SPDU is signed with cert digest and not with full cert, 
            // the SPDU gets rejected by aerolink due to smaller received packet size.
            // to do.. use proper method for the offset (6 bytes) to bypass the original wsmp header
            ret = secInterface_->extractMsg(&asyncCbData->smp,
                (uint8_t*)rxMsg->buf, rxMsg->bufLen-6,
                payload, payloadLen, dot2HdrLen);
            if(ret == -1){
                std::cerr << "Error extracting security header\n";
                // error in extracting the payload and secured information
                goto handleMsg;
            }else{
                totalExtracted++;
            }
            asyncCbData->asyncState=FREE;

            // NOTE: following may change based on the packet type
            rxMsg->buf = (char*)rxMsg->buf+dot2HdrLen;
            rxMsg->bufLen = sizeBeforeExtract-dot2HdrLen;
            ret = codec_->decodeFrame(rxMsg, true, msgType, (uint8_t*)rxMsg->buf, rxMsg->bufLen);
            if(ret == -1){
                std::cout << "Decode failure \n";
                goto handleMsg;
            }

            //handle locally consumed messages
            if (st_ == StackType::STACK_TYPE_SAE) {
                std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(rxMsg);
                if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_WSA) {
                    std::cout << "Received WSA" << std::endl;
                    auto wsaBody = saeMsg->SrvAdvMsg()->body();
                    if (role_ == Role::OBU and wsaBody.routingAdvertisement().has_value() and
                            wra_ != nullptr) {
                        std::cout << "settting IP" << std::endl;
                        ret = wra_->onReceiveWra(wsaBody.routingAdvertisement().value(),
                                    rxMsg->srcMac_, rxMsg->macAddrLen_);
                        enqueueMsg(rxMsg);
                        goto handleMsg;
                    }
                } else if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_RTCM) {
                    std::cout << "Received RTCM" << std::endl;
                    if (role_ == Role::OBU and rtcm_ != nullptr) {
                        if(rtcm_->onReceiveRTCM(*(saeMsg->RTCMcorrection())) == Status::SUCCESS) {
                            ret = 0;
                        } else {
                            ret = -1;
                        }
                        goto handleMsg;
                    }
                } else if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_BSM) {
                    // add more message types up here for any packets that are expected
                    // to be signed
                    // verify .. continue
                    // extract the specific data from the RX msg struct and copy to
                    // data type that aerolink expects
                    auto bsm = saeMsg->BSM();
                    auto coreData = bsm->coreData();
                    asyncCbData->rvBsmData.version = 1;
                    asyncCbData->rvBsmData.dataType = (uint32_t)SAE_PSID_BSM; // should be 0x20 or 32
                    asyncCbData->rvBsmData.id = 0;
                    for(int i =7; i >= 0 ; i--){
                        asyncCbData->rvBsmData.id |= ((coreData.id()[i] << 8*i) & 0x00);
                    }
                    asyncCbData->rvBsmData.msgCount = coreData.msgCnt();
                    asyncCbData->rvBsmData.latitude = coreData.lat();
                    asyncCbData->rvBsmData.longitude = coreData.longitude();
                    asyncCbData->rvBsmData.elevation =  coreData.elev();
                    asyncCbData->rvBsmData.speed =  coreData.speed();
                    asyncCbData->rvBsmData.heading =  coreData.heading();
                    //AccelerationSet4Way
                    asyncCbData->rvBsmData.longitudeAcceleration =  coreData.accelSet().longitude();
                    asyncCbData->rvBsmData.latitudeAcceleration =  coreData.accelSet().lat();
                    asyncCbData->rvBsmData.verticalAcceleration =  coreData.accelSet().vert();
                    asyncCbData->rvBsmData.yawAcceleration =  coreData.accelSet().yaw();
                    // J2735::idl::BrakeSystemStatus
                    int i = 0;
                    for(; i < sizeof(coreData.brakes().wheelBrakes()); i++){
                        asyncCbData->rvBsmData.brakes = ((coreData.brakes().wheelBrakes() << i) & (0x01 << i));
                    }
                    for(; i < sizeof(coreData.brakes().traction()); i++){
                        asyncCbData->rvBsmData.brakes = ((coreData.brakes().traction() << i) & (0x01 << i));
                    }
                    for(; i < sizeof(coreData.brakes().abs()); i++){
                        asyncCbData->rvBsmData.brakes = ((coreData.brakes().abs() << i) & (0x01 << i));
                    }
                    for(; i < sizeof(coreData.brakes().brakeBoost()); i++){
                        asyncCbData->rvBsmData.brakes = ((coreData.brakes().brakeBoost() << i) & (0x01 << i));
                    }
                    for(; i < sizeof(coreData.brakes().auxBrakes()); i++){
                        asyncCbData->rvBsmData.brakes = ((coreData.brakes().auxBrakes() << i) & (0x01 << i));
                    }

                    // this function will check the consistency, relevancy, signature, and mbd
                    // the async verify function will then call your callback
                    // checkMsgSecurity(asyncCbData, priority, callbackFn);
                    // provide aerolink the exact location given by sender in its BSM
                    if(secInterface_->setGenLocation(&asyncCbData->smp, &asyncCbData->rvBsmData) < 0){
                        ret = -1;
                    }
                    // check consistency
                    if(secInterface_->checkConsistency(&asyncCbData->smp) < 0){
                        ret = -1;
                    }
                    // check relevancy
                    if(secInterface_->checkRelevance(&asyncCbData->smp) < 0){
                        ret = -1;
                    }
                    // verify the packet
                    // extract priority from traffic class of packet
                    uint8_t priority = (uint8_t)TrafficIpType::TRAFFIC_NON_IP;
                    asyncCbData->rxMsg = std::dynamic_pointer_cast<void>(saeMsg);
                    if(congCtrlInit_){
                        asyncCbData->congCtrl = static_cast<void*>(congestCtrl_.get());
                    }
                    asyncCbData->asyncState = VERIFYING;
                    secInterface_->asyncVerify(
                        asyncCbData,
                        priority,
                        &(asyncCbData->smp),
                        [] (
                            AEROLINK_RESULT returnCode,
                            void           *userData)  -> void
                        {
                            asyncCbData_t* cbData = (asyncCbData_t*) userData;
                            if(cbData == nullptr){
                                printf("cb_data is a null pointer\n");
                                return;
                            }
                            std::shared_ptr<v2xMsgSae> rxV2xMsgSae =
                                std::static_pointer_cast<v2xMsgSae>(cbData->rxMsg);
                            if(returnCode != WS_SUCCESS){
                                cbData->verifSuccess = false;
                                cbData->asyncState = VERIF_DONE;
                                cbData->asyncState = PP_DONE;
                                cbData->asyncState = FREE;
                                return;
                            }
                            else
                            {
                                totalVerifSuccess++;
                                if(lastTimeStamp == 0){
                                    lastTimeStamp = timestamp_now();
                                }
                                if(totalVerifSuccess % 1000 == 0){
                                    uint64_t currTime = timestamp_now();
                                    double rate = (1000.0 / (double)(currTime - lastTimeStamp));
                                    std::cout << "Average rate is: " << rate << "k Vhz\n";
                                    lastTimeStamp = currTime;
                                }
                                cbData->verifSuccess = true;
                                cbData->asyncState = VERIF_DONE;
                            }

                            // misbehavior detection
                            AEROLINK_RESULT result;
                            MisbehaviorDetectedType misbehaviorResult;
                            result = smp_checkMisbehavior(cbData->smp,
                                    static_cast<void*>(&cbData->rvBsmData),
                                    &misbehaviorResult);
                            if (result != WS_SUCCESS){
                                if(result == WS_ERR_MISBEHAVIOR_DETECTED){
                                    fprintf(stdout, "Detected Misbehavior Class is %llu\n",
                                            misbehaviorResult.detectedMisbehavior);
                                }else{
                                    fprintf(stderr, "Error in checking misbehavior\n");
                                }
                                // don't forward if mbd fail
                            }
                            // filtering
                                // don't forward if filtered out
                            // postprocessing
                            // congestion control
                            uint64_t timestamp_ms = timestamp_now();
                            CongestCtrl* congCtrl = static_cast<CongestCtrl*>(cbData->congCtrl);
                            if(congCtrl != nullptr){
                                congCtrl->addCongestionControlData(
                                    cbData->rvBsmData.id,
                                    (cbData->rvBsmData.latitude)/10000000.0,
                                    (cbData->rvBsmData.longitude)/10000000.0,
                                    cbData->rvBsmData.heading,
                                    cbData->rvBsmData.speed,
                                    timestamp_ms,
                                    cbData->rvBsmData.msgCount);
                            }
                            // everything else (e.g., logging, statistics...)
                            cbData->asyncState = PP_DONE;
                            cbData->asyncState = FREE;
                        });
                    ret = 0;
                    // technically dont want to free or enqueue until postprocess
                } else {
                    // enqueue rest of message types, if successful
                    std::cout << "queued and freeing decoded unsigned msg" << std::endl;
                    ret = 0;
                }

            }
        }else{
            //unsecured case

            time_start_ms = timestamp_now();
            time_start_us = timestamp_now_us();
            ret = codec_->decodeFrame(rxMsg, false, 0, nullptr, 0);

            time_finish_us = timestamp_now_us();
            time_finish_ms = timestamp_now();
            if (time_start_ms == time_finish_ms) {
                std::cout << "ts: " << time_finish_us - time_start_us << " (us)" << std::endl;
            } else {
                std::cout << "ts: > 1ms" << std::endl;
            }

            //handle locally consumed messages
            if (st_ == StackType::STACK_TYPE_SAE) {
                std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(rxMsg);
                if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_WSA) {
                    std::cout << "Received WSA" << std::endl;
                    auto wsaBody = saeMsg->SrvAdvMsg()->body();
                    if (role_ == Role::OBU and wsaBody.routingAdvertisement().has_value() and
                            wra_ != nullptr) {
                        std::cout << "setting IP" << std::endl;
                        ret = wra_->onReceiveWra(wsaBody.routingAdvertisement().value(),
                                    saeMsg->srcMac_, saeMsg->macAddrLen_);
                        putFreeMsg(rxMsg);
                        continue;
                    }
                } else if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_RTCM) {
                    std::cout << "Received RTCM" << std::endl;
                    if (role_ == Role::OBU and rtcm_ != nullptr) {
                        if(rtcm_->onReceiveRTCM(*(saeMsg->RTCMcorrection())) != Status::SUCCESS) {
                            std::cout << "failed to inject RTCM" << std::endl;
                        }
                        putFreeMsg(rxMsg);
                        continue;
                    }
                }
            }

            if (ret != 0) {
                std::cerr << "decode error" << std::endl;
                ret = -1;
                goto handleMsg;
            }
        }
handleMsg:
        if(ret != -1){
            numPublished++;
            publisher_->publish(rxMsg);
        }
        if(rxMsg != nullptr){
            rxMsg->buf = bufStart;
            putFreeMsg(rxMsg);
        }
    }
    return ret;
}
std::shared_ptr<v2xMsg> SplitsCoreImpl::fillLocalMsg(int serviceId)
{
    std::shared_ptr<v2xMsgSae> msg = nullptr;
    switch(serviceId) {
        case SAE_PSID_BSM:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txSpsMsgs_[serviceId]);
            if (msg == nullptr) {
                std::cerr << "No sps msg!" << std::endl;
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;

            // we need to check for distance and time since last id change to determine
            // if a change should happen
            if(configData_->getCoreConfig().getEnableSecurity() && secInterface_){
                if(secInterface_->changeIdNow()){
                    if(secInterface_->idChange() < 0){
                        std::cerr << "Id Change Failure" << std::endl;
                    }
                }
            }

            if(bsmGenerator_->fillBsm(msg, idChangeData_.idChanged,
                idChangeData_.tempId) != Status::SUCCESS) {
                std::cerr << "Failed to init bsm msg" << std::endl;
                msg = nullptr;
                goto out;
            }
            // reset here
            idChangeData_.idChanged = false;
            break;
        case SAE_PSID_WSA:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txSpsMsgs_[serviceId]);
            if (msg == nullptr) {
                std::cerr << "No sps msg!" << std::endl;
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
            if (wra_ != nullptr) {
                if(wra_->fillWra(msg)) {
                    std::cerr << "Failed to init WSA msg" << std::endl;
                    msg = nullptr;
                    goto out;
                }
            } else {
                std::cerr << "wsa not initialized" << std::endl;
                msg = nullptr;
                goto out;
            }
            break;
        case SAE_PSID_RTCM:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txEventMsgs_[serviceId]);
            if (msg == nullptr) {
                std::cerr << "No event msg!" << std::endl;
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
            if (rtcm_ != nullptr) {
                if(rtcm_->fillRTCM(msg) != Status::SUCCESS) {
                    std::cerr << "failed to initialize RTCM msg" << std::endl;
                    goto out;
                }
            } else {
                std::cerr << "RTCM object not initialized" << std::endl;
                msg = nullptr;
                goto out;
            }
            break;
        default:
            std::cerr << "not supporting local generation of message with serviceId" << serviceId << std::endl;
            break;
    }
out:
    return msg;
}
int SplitsCoreImpl::send(int serviceId, std::shared_ptr<v2xMsg> msg) {
    int ret = -1;
    uint8_t signedPdu[512];
    uint32_t signedPduLen = 512;
    Priority priority;
    std::map<int, RadioTransmit>::iterator it;

    if (msg == nullptr) {
        //called by txScheduler for local generation
        msg = fillLocalMsg(serviceId);
        if (msg == nullptr) {
            std::cerr << "No message " << std::endl;
        }
        msg->format_ = MsgFormat::MSG_FORMAT_UNENCODED;
    }
    if (msg->format_ == MsgFormat::MSG_FORMAT_UNENCODED) {
        Ieee1609Dot2Data dot2Data;
        if (configData_->getCoreConfig().getEnableSecurity() && secInterface_) {
            dot2Data.content = Ieee1609Dot2Content::signedData;
        } else {
            dot2Data.content = Ieee1609Dot2Content::unsecuredData;
        }
        msg->Ieee1609Dot2(dot2Data);
        // this would fully encode if the packet was not to be signed
        if ((ret = codec_->encodeFrame(msg))) {
            std::cerr << "Failed to encode msg" << std::endl;
            goto out;
        }

        //sign message based on psid and ssp
        if(configData_->getCoreConfig().getEnableSecurity() && secInterface_){
            auto secConfig = configData_->getSecConfig();
            //std::cout << "Psid and ssp for sign are: " << secConfig.getPsidValue()<< ", " << secConfig.getSspValue().data() <<"\n";
            if((ret = secInterface_->signMsg(&smg_, (uint8_t*)msg->buf, msg->bufLen,
                signedPdu, signedPduLen, secConfig.getPsidValue(),
                secConfig.getEnableSsp(), secConfig.getSspValue().data(),secConfig.getSspLength(),
                secConfig.getEnableSspMask(), secConfig.getSspMask().data(), secConfig.getSspMaskLength(),
                SignerTypeOverride::STO_AUTO))  == 0){
                //optimize this
               // std::cout << "New signed payload length is: " << signedPduLen << "\n";
                msg->bufLen = signedPduLen;
                std::memcpy(msg->buf, signedPdu, signedPduLen);
                //encode Wsmp header and prepend to begining of buffer
                if (codec_->encodeWsmp(msg, true)) {
                     std::cerr << "Failed to encode WSMP header" << std::endl;
                    goto out;
                }
            } else {
                std::cerr << "fail to sign msg" << std::endl;
                goto out;
            }
        }
    }
    //TODO: use proper priority for different messages.
    priority = static_cast<Priority>(DEFAULT_BSM_PRIORITY);
    it = spsTransmits_.find(serviceId);
    if (it != spsTransmits_.end()) {
        // following initialization should only occur once and for the BSM PSID
        if(!congCtrlInit_ && serviceId == SAE_PSID_BSM){
            congestCtrl_->init(congCtrlConfig_,
                std::make_shared<RadioTransmit>(it->second));
            congCtrlInit_ = true;
        }
        ret = it->second.transmit(msg->buf, msg->bufLen, priority);
        if (ret) {
            std::cerr << "Transmit failure" << std::endl;
            goto out;
        }
    } else {
        it = eventTransmits_.find(serviceId);
        if (it == eventTransmits_.end()) {
            std::cerr << "No corresponding radio to send msg for serviceId: "<< serviceId << std::endl;
            ret = -1;
            goto out;
        }
        ret = it->second.transmit(msg->buf, msg->bufLen, priority);
        if (ret) {
            std::cerr << "Transmit failure" << std::endl;
            goto out;
        }
    }

    ret = 0;
 out:
     //reset msg
    if(msg) {
        msg->buf = msg->head;
        msg->bufLen = TX_BUF_LEN;
        //TODO: properly delete and reallocate structure
        // perhaps even make a cache per msg type
    }
    return ret;
}
void SplitsCoreImpl::run(void)
{
     //publisher_->run();
     const SplitsConfig::CoreConfig & coreCfg = configData_->getCoreConfig();

     if (useTxPduFile_ == true) {
         //send MAP (msg index = 1)
         txScheduler_->scheduleSpsTransmit(1, 1000);
     }else{
         for (auto const & txSpsFlow : coreCfg.getSpsFlows()) {
             txScheduler_->scheduleSpsTransmit(txSpsFlow.getServiceId(), txSpsFlow.getPeriodicity());
         }
         for (auto const & txEventFlow : coreCfg.getEventFlows()) {
             if (static_cast<MsgSource>(txEventFlow.getSource()) == MsgSource::LOCAL) {
                 txScheduler_->scheduleEventTransmit(txEventFlow.getServiceId(), txEventFlow.getPeriodicity());
             }
         }
     }
     txScheduler_->start();

     processRxThread_ = std::thread(&SplitsCoreImpl::processReceive, this);

     int rxid = 0;
     while(!congCtrlInit_){}
     for (auto const & radioRx : radioReceives_) {
         auto rxThread = std::thread([&, rxid]() {
                 while(!stop_) {
                     receive(rxid, RX_BUF_LEN);
                 }

         });
         rxThreads_.push_back(std::move(rxThread));
         rxid++;
     }
     while(stop_ == false) {
        if(congCtrlInit_){
            receive(0, RX_BUF_LEN);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
}
}; //namespace Core
}; //namespace SPLITS
