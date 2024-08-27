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

#define RX_POOL_SIZE    16
#define DEFAULT_BSM_PRIORITY    5

using SPLITS::Codec::SplitsCodecFactory;
using SPLITS::Codec::SplitsCodec;
using SPLITS::Core::Role;
using SPLITS::MsgSource;
using SPLITS::MsgFormat;

//Testing variables
#include <cstdio>
char gRxBuf[512];
char gTxBuf[512];
int gRxBufLen;
int gTxBufLen;

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

    if (coreCfg.getEnableCongCtrl() == true) {
        congestCtrl_ = std::make_shared<CongestCtrl>();
        locListeners.push_back(congestCtrl_);
    }
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
    int ret;
    auto rxMsg = getFreeMsg();

    if (rxMsg == nullptr) {
        std::cout << "rxpool overrun!" << std::endl;
        return -1;
    }
    rxMsg->macAddrLen_ = CV2X_MAC_ADDR_LEN;
    if (useRxPduFile_ == true) {
        rxMsg->buf = gRxBuf;
        rxMsg->bufLen = gRxBufLen;
    } else {
         ret = radioReceives_[rxid].receive(rxMsg->buf, rxMsg->bufLen, rxMsg->srcMac_, rxMsg->macAddrLen_);
         if (ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN ) {
            //std::cout << "Rx error" << std::endl;
            goto exit;
        }
        rxMsg->bufLen = ret;
    }
    enqueueMsg(rxMsg);
    return 0;
exit:
    putFreeMsg(rxMsg);

    return ret;
}
void SplitsCoreImpl::processReceive(void)
{
    int ret;
    uint64_t time_start_ms, time_finish_ms, time_start_us, time_finish_us;
    while(stop_ == false) {
        auto rxMsg = dequeueMsg();
        time_start_ms = timestamp_now();
        time_start_us = timestamp_now_us();

        ret = codec_->decodeFrame(rxMsg);

        time_finish_us = timestamp_now_us();
        time_finish_ms = timestamp_now();
        if (time_start_ms == time_finish_ms) {
            std::cout << "ts: " << time_finish_us - time_start_us << " (us)" << std::endl;
        } else {
            std::cout << "ts: > 1ms" << std::endl;
        }

        if (ret != 0) {
            std::cerr << "decode error" << std::endl;
            putFreeMsg(rxMsg);
            continue;
        }
        //TODO: Security Verification.
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
        std::cout << "queued msg" << std::endl;
        publisher_->publish(rxMsg);
        putFreeMsg(rxMsg);
    }
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

//    std::shared_ptr<v2xMsgSae> msg = nullptr;

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
            //std::cout << "current message buffer size: " << msg->bufLen << "\n";
            //std::cout << "size of output signed msg buffer: " << signedPduLen << "\n";
            auto secConfig = configData_->getSecConfig();
            if((ret = secInterface_->signMsg(&smg_, (uint8_t*)msg->buf, msg->bufLen,
                signedPdu, signedPduLen, secConfig.getPsidValue(),
                secConfig.getEnableSsp(), secConfig.getSspValue().data(),secConfig.getSspLength(),
                secConfig.getEnableSspMask(), secConfig.getSspMask().data(), secConfig.getSspMaskLength(),
                SignerTypeOverride::STO_AUTO))  == 0){
                //optimize this
                std::cout << "New signed payload length is: " << signedPduLen << "\n";
                msg->bufLen = signedPduLen;
                std::memcpy(msg->buf, signedPdu, signedPduLen);
                //encode Wsmp header and prepend to begining of buffer
                if (codec_->encodeFrame(msg)) {
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
        //codec_->freeFrame(msg);
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
        //receive(0, RX_BUF_LEN);
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
}
}; //namespace Core
}; //namespace SPLITS
