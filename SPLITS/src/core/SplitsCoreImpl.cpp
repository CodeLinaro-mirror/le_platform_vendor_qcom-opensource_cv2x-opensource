/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <syslog.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include "SplitsCoreFactory.hpp"
#include "SplitsCodecFactory.hpp"
#include "SplitsCoreImpl.hpp"

#include <CommonAPI/CommonAPI.hpp>
#include "SplitsConfigStubImpl.hpp"
#include "SplitsDataPublisher.hpp"
#include "utils.h"
#include "v2xMsgSae.hpp"
#include "CsvIO.hpp"
#include "Logger.hpp"

#define RX_POOL_SIZE    6000
#define DEFAULT_BSM_PRIORITY    5

using SPLITS::Codec::SplitsCodecFactory;
using SPLITS::Codec::SplitsCodec;
using SPLITS::Core::Role;
using SPLITS::Interface::SplitsStatisticsStubImpl;
using SPLITS::MsgSource;
using SPLITS::MsgFormat;
using v1::commonapi::SPLITS::SplitsStatistics;
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
SplitsCoreImpl::SplitsCoreImpl(StackType st) {
    LOGD(__FUNCTION__);
    st_ = st;
}

SplitsCoreImpl::~SplitsCoreImpl() {
    LOGD(__FUNCTION__);
}

int SplitsCoreImpl::readPduFromFile(std::string file, char *buf, int bufLen) {
     int ret = 0;
     size_t len;

    if (file.empty()) {
        return -1;
    }

    FILE* fp = fopen(file.c_str(), "r");
    if (!fp) {
        LOGE(__FUNCTION__, " failed to open file", file);
        return -1;
    }

    len = fread(buf, 1, bufLen, fp);

    fclose(fp);

    ret = len > 0 ? len : -1;
    return ret;
}

int SplitsCoreImpl::initRadio() {
    EventFlowInfo eventInfo;
    SpsFlowInfo spsInfo;
    const SplitsConfig::CoreConfig & coreCfg = configData_->getCoreConfig();

    ::v1::commonapi::SPLITS::SplitsStatistics::TxStatistics txStatistics;
    //Initialize TxSps
    for (auto const & txSpsFlow : coreCfg.getSpsFlows()) {
        spsInfo.periodicityMs = adjustSpsPeriodicity(txSpsFlow.getPeriodicity());
        //use default priority and reservation size
        RadioTransmit tx(spsInfo, TrafficCategory::SAFETY_TYPE,  TrafficIpType::TRAFFIC_NON_IP,
                txSpsFlow.getSourcePort(), txSpsFlow.getServiceId());
        if (-1 != tx.getPort()) {
            tx.configureIpv6(txSpsFlow.getDestPort(), txSpsFlow.getDestAddr().c_str());
            spsTransmits_.emplace(txSpsFlow.getServiceId(), std::move(tx));

            txStatistics.setType(
                ::v1::commonapi::SPLITS::SplitsCommon::FlowType::Literal::SPS);
            allTxStatistics_[static_cast<SplitsStatistics::PSID>(txSpsFlow.getServiceId())] =
                txStatistics;

            LOGD(__FUNCTION__, " allTxStatistics_[", txSpsFlow.getServiceId(), "], type = ",
                 static_cast<int>(::v1::commonapi::SPLITS::SplitsCommon::FlowType::Literal::SPS));
        } else {
            LOGE(__FUNCTION__, " SplitsCoreImpl::init: init error in creating Tx SPS flow! with spsServiceId: ",
                 txSpsFlow.getServiceId());
            return -1;
        }
    }
    //Initialize TxEvent
    for (auto const &txEventFlow : coreCfg.getEventFlows()) {
        RadioTransmit tx(eventInfo, TrafficCategory::SAFETY_TYPE, TrafficIpType::TRAFFIC_NON_IP,
                         txEventFlow.getSourcePort(), txEventFlow.getServiceId());
        if (-1 != tx.getPort()) {
            tx.configureIpv6(txEventFlow.getDestPort(), txEventFlow.getDestAddr().c_str());
            eventTransmits_.emplace(txEventFlow.getServiceId(), std::move(tx));

            txStatistics.setType(
                ::v1::commonapi::SPLITS::SplitsCommon::FlowType::Literal::EVENT);
            allTxStatistics_[static_cast<SplitsStatistics::PSID>(txEventFlow.getServiceId())] =
                txStatistics;

            LOGD("allTxStatistics_[", txEventFlow.getServiceId(), "], type = ",
                 static_cast<int>(::v1::commonapi::SPLITS::SplitsCommon::FlowType::Literal::EVENT));
        } else {
            LOGE(__FUNCTION__, " SplitsCoreImpl::init error in creating Tx event flow! with serviceID ",
                 txEventFlow.getServiceId());
            return -1;
        }
    }

    //Initialize Rx
    ::v1::commonapi::SPLITS::SplitsStatistics::RxStatistics rxStatistics;
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
            if (-1 != rx.getPort()) {
                radioReceives_.push_back(rx);
                std::vector<SplitsStatistics::PSID> serviceIds;
                for (auto id : rxPortId.second) {
                    serviceIds.push_back(static_cast<SplitsStatistics::PSID>(id));
                }
                rxStatistics.setPsids(serviceIds);
                allRxStatistics_[rxPortId.first] = rxStatistics;
            } else {
                LOGE(__FUNCTION__, " SplitsCoreImpl::init error in creating Rx subscription!");
                LOGE(__FUNCTION__, " with receiveSubIds: ");
                for (auto id : rxPortId.second) {
                    LOGE(__FUNCTION__, " ", id, ", ");
                }

                return -1;
            }
        }
    } else {
        uint16_t port = 9000;
        LOGD(__FUNCTION__,
             " Creating wildcard Rx subscription on port 9000");
        RadioReceive rx(TrafficCategory::SAFETY_TYPE,
                        TrafficIpType::TRAFFIC_NON_IP,
                        port, nullptr);
        if (-1 != rx.getPort()) {
            radioReceives_.push_back(rx);
        } else {
            LOGE(__FUNCTION__,
                 " SplitsCoreImpl::init error in creating wildcard Rx subscription!");
            return -1;
        }
    }

    return 0;
}

int SplitsCoreImpl::initSecurity(const std::string ctxName,
    const uint16_t countryCode, const std::string lcmName) {
    try {
        secInterface_ = std::make_shared<SecurityInterface>(ctxName, countryCode);
    } catch (const std::runtime_error& error) {
        LOGE(__FUNCTION__, " Failure in creating security interface");
        return -1;
    }

    sem_init(&idChangeData_.idChangeSem, 0, 1);
    if (secInterface_->registerIdChange(lcmName, &idChangeData_)) {
        LOGE(__FUNCTION__, " Failure in registering id change");
        return -1;
    }

    if (secInterface_->createNewSmg(&smg_) < 0) {
        LOGE(__FUNCTION__, " Failure in creating a secure message generator");
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
            // TODO: Service ID to Message ID map,
            //       for now we are just using the service ID
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
            // TODO: Service ID to Message ID map,
            //       for now we are just using the service ID
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
int SplitsCoreImpl::init(std::string cfgFileName, std::string rxpduFileName,
                         std::string txpduFileName, std::string csvFileDir) {
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
            LOGE(__FUNCTION__, " config file name not specified");
            return -1;
        } else if (configFile_->loadConfigFromFile(cfgFileName) < 0) {
            LOGE(__FUNCTION__, " Faild to load or parse config file");
            return -1;
        }
    }
    const SplitsConfig::CoreConfig &coreCfg = configData_->getCoreConfig();
    role_ = static_cast<Role>(coreCfg.getRole());

    //pdu file is for testing of codec.
    if (rxpduFileName.empty() == false) {
         if ((gRxBufLen = readPduFromFile(rxpduFileName, gRxBuf, 512)) < 0) {
             LOGE(__FUNCTION__, " Failed to read file");
             return -1;
         }
         useRxPduFile_ = true;
    } else {
        if (initRadio() < 0) {
            LOGE(__FUNCTION__, " Faled to init radio");
            return -1;
        }
    }
    if (txpduFileName.empty() == false) {
        if ((gTxBufLen = readPduFromFile(txpduFileName, gTxBuf, 512)) < 0) {
            LOGE(__FUNCTION__, " Failed to read file");
            return -1;
        }
        useTxPduFile_ = true;
    }

    // csv file creation
    LOGD(__FUNCTION__, " csvFileDir = ", csvFileDir);
    if (false == csvFileDir.empty()) {
        csv_ = std::make_shared<CsvIO>(csvFileDir);
        // TODO: Write csv header
        std::string header = "count,id,second,latitude,longitude,elevation,semiMajor,semiMinor,orientation,transmission,speed,heading,angle,accel_longitude,accel_lat,accel_vert,yaw,brake,traction,abs,scs,brakeboost,auxBrake,length,width,\n";
        csv_->writeTx(header);
        csv_->writeRx(header);
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
        LOGE(__FUNCTION__, " Failed to init codec");
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
        LOGE(__FUNCTION__, " Faild to init bsm generator");
        return -1;
    }
    locListeners.push_back(bsmGenerator_);

    // if (configData_->getCoreConfig().getEnableCongCtrl() == true) {
    LOGD(__FUNCTION__, " making cong ctrl");
    congestCtrl_ = std::make_shared<CongestCtrl>();
    locListeners.push_back(congestCtrl_);
    // }
    //location
    kinematic_ = std::make_shared<KinematicsReceive>(locListeners, 100);
    //tx Scheduler
    txScheduler_ = std::make_shared<TxScheduler>(getptr());

    CommonAPI::Runtime::setProperty("LogContext", "SplitsService");
    CommonAPI::Runtime::setProperty("LogApplication", "SplitsService");
    CommonAPI::Runtime::setProperty("LibraryBase", "SplitsService");
    runtime_ = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.SplitsStatistics";
    std::string connection = "service-SPLITS";
    splitsStatisticsService_ = std::make_shared<SplitsStatisticsStubImpl>(this->getptr());
    while (!runtime_->registerService(domain,
                                      instance,
                                      splitsStatisticsService_,
                                      connection)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //security interface
    if (coreCfg.getEnableSecurity()) {
        auto secConfig = configData_->getSecConfig();
        if (initSecurity(secConfig.getSecurityCtxName(),
                         secConfig.getCountryCode(),
                         secConfig.getLocalCertMgrName()) < 0) {
            return -1;
        }
    }

    //WRA
    if (coreCfg.getEnableWra()) {
        LOGD(__FUNCTION__, " WRA enabled");
        wra_ = std::make_shared<SplitsWra>(role_,
                std::make_shared<RadioInterface>(radioReceives_[0]),
                std::make_shared<SplitsConfig::WraConfig>(configData_->getWraConfig()));
        if (wra_ == nullptr) {
            LOGE(__FUNCTION__, " failed to init wra object");
            return -1;
        }
    } else {
        LOGD(__FUNCTION__, " WRA not enabled");
    }
    //RTCM
    if (coreCfg.getEnableRTCM()) {
        LOGD(__FUNCTION__, " role: ",
             static_cast<std::underlying_type<SPLITS::Core::Role>::type>(role_));
        rtcm_ = std::make_shared<SplitsRTCM>(role_,
                std::make_shared<SplitsConfig::RTCMConfig>(configData_->getRtcmConfig()));
        if (rtcm_->init()) {
            LOGE(__FUNCTION__, " Failed to init RTCM");
            return -1;
        }
    }
    //dds
    publisher_ = std::make_shared<SplitsServiceDataPublisher>();
    if (publisher_->init(getptr(), std::string("SERVICE")) == false) {
        LOGE(__FUNCTION__, " Failed to init data publisher");
        return -1;
    }
    subscriber_ = std::make_shared<SplitsDataSubscriber>();
    if (subscriber_->init(std::string("CLIENT")) == false) {
        LOGE(__FUNCTION__, " Failed to init data subscriber");
        return -1;
    }
    //register a listener for receiving message from EAP
    dataListener_ = std::make_shared<SplitsCoreDataListener>(getptr());
    subscriber_->registerListener(dataListener_);

    return 0;
}

int SplitsCoreImpl::receive(const uint8_t rxid, const uint16_t bufLen) {
    int ret = 0;
    auto rxMsg = getFreeMsg();

    if (rxMsg == nullptr) {
        LOGD(__FUNCTION__, " rxpool overrun!");
        return -1;
    }
    rxMsg->macAddrLen_ = CV2X_MAC_ADDR_LEN;
    rxMsg->port_ = radioReceives_[rxid].getPort();

    if (useRxPduFile_ == true) {
        rxMsg->buf = gRxBuf;
        rxMsg->bufLen = gRxBufLen;
    } else {
         ret = radioReceives_[rxid].receive(rxMsg->buf,
                                            MAX_PACKET_LEN,
                                            rxMsg->srcMac_, rxMsg->macAddrLen_);
        if (ret < MIN_PACKET_LEN || ret > MAX_PACKET_LEN) {
            if (0 != ret) {
                LOGE(__FUNCTION__, " Rx error, ret = ", ret);
                allRxStatistics_[rxMsg->port_].setRxFailed(
                    allRxStatistics_[rxMsg->port_].getRxFailed() + 1);
            }
            goto exit;
        }
        allRxStatistics_[rxMsg->port_].setRxNumber(
            allRxStatistics_[rxMsg->port_].getRxNumber() + 1);
        rxMsg->bufLen = ret;
    }
    enqueueMsg(rxMsg);
    ++numRxed;
    return 0;
exit:
    putFreeMsg(rxMsg);
    // only queue a message structure if it was actually valid
    // a return value of 0 means that polling just expired
    if (ret != 0) {
        enqueueMsg(rxMsg);
    }
    return ret;
}

void SplitsCoreImpl::updateVerificationResult(v2xMsgType msgType, uint16_t port,
                                              bool verifSuccess, bool misbehavir) {
    LOGD(__FUNCTION__);
    // TODO: check existence of the key-value in the map
    auto& rxStatistics = allRxStatistics_[port];
    auto& msgStatistics =
        allMessageStatistics_[static_cast<SplitsStatistics::MessageID>(msgType)];

    std::lock_guard<std::mutex> lock(statisticsMutex_);
    msgStatistics.setVerifFailed(msgStatistics.getVerifFailed() + 1);
    msgStatistics.setMisbehaviorsDetected(msgStatistics.getMisbehaviorsDetected() + 1);
    rxStatistics.setVerifFailed(rxStatistics.getVerifFailed() + 1);
    rxStatistics.setMisbehaviorsDetected(rxStatistics.getMisbehaviorsDetected() + 1);
}

int SplitsCoreImpl::processReceive(void) {
    LOGD(__FUNCTION__);
    int ret = 0;
    char* bufStart = nullptr;
    uint64_t time_start_ms, time_finish_ms, time_start_us, time_finish_us = 0;
    // process all queued rxed messages
    while (stop_ == false) {

        auto rxMsg = dequeueMsg();
        if (rxMsg == nullptr) {
            // sanity check, should not happen
            ret = -1;
            return ret;
        }

        recordBSM(rxMsg, false);
        
        auto& rxStatistics = allRxStatistics_[rxMsg->port_];
        /*
           enum class MsgFormat {
               MSG_FORMAT_BLOB,     //encoded, signed, with network and transport header.
               MSG_FORMAT_SIGNED,   //encoded, signed, no network and transport header.
               MSG_FORMAT_UNSIGNED, //encoded, but not signed, no network and transport header.
               MSG_FORMAT_UNENCODED, //unencoded, unsigned, no network and transport header.
               MSG_FORMAT_UNKNOWN   //don't know
           }
         */

        // detect that this is a secured frame
        // or just assume by default that it is secured and if not secured, reject it

        asyncCbData_t* asyncCbData;
        bufStart = rxMsg->buf;
        if (configData_->getCoreConfig().getEnableSecurity() &&
            secInterface_ != nullptr) {
            // find an available callback data struct for this verification
            asyncCbData = secInterface_->getFreeVerifData();
            if (asyncCbData == nullptr) {
                rxStatistics.setDecodeFailed(rxStatistics.getDecodeFailed() + 1);
                LOGE(__FUNCTION__, " Invalid async cb for this msg");
                ret = -1;
                goto handleMsg;
            }
            uint8_t const *payload = NULL;
            uint32_t payloadLen = 0;
            uint32_t dot2HdrLen = 0;

            // just decode wsmp
            int msgType = 0;
            msgType = codec_->decodeWsmp(rxMsg);

            if (msgType == -1) {
                rxStatistics.setDecodeFailed(rxStatistics.getDecodeFailed() + 1);
                LOGE(__FUNCTION__, " Error decoding wsmp header of signed pdu");
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
                                            (uint8_t*)rxMsg->buf, rxMsg->bufLen - 6,
                                            payload, payloadLen, dot2HdrLen);
            if (ret == -1) {
                rxStatistics.setDecodeFailed(rxStatistics.getDecodeFailed() + 1);
                LOGE(__FUNCTION__, " Error extracting security header");
                // error in extracting the payload and secured information
                goto handleMsg;
            } else {
                ++totalExtracted;
            }
            asyncCbData->asyncState = FREE;
            asyncCbData->splitsCore = shared_from_this();

            // NOTE: following may change based on the packet type
            rxMsg->buf = (char*)rxMsg->buf + dot2HdrLen;
            // rxMsg->buf = payload ?
            rxMsg->bufLen = sizeBeforeExtract - dot2HdrLen;
            ret = codec_->decodeFrame(rxMsg, true, msgType, (uint8_t*)rxMsg->buf, rxMsg->bufLen);
            if (ret == -1) {
                LOGE(__FUNCTION__, " Decode failure");
                rxStatistics.setDecodeFailed(rxStatistics.getDecodeFailed() + 1);
                goto handleMsg;
            }

            // TODO: check existence of the message type in the map
            auto& msgStatistics =
                allMessageStatistics_[static_cast<SplitsStatistics::MessageID>(rxMsg->msgType_)];
            msgStatistics.setRxSignedCount(msgStatistics.getRxSignedCount() + 1);

            //handle locally consumed messages
            if (st_ == StackType::STACK_TYPE_SAE) {
                std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(rxMsg);
                if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_WSA) {
                    LOGD(__FUNCTION__, " Received WSA");
                    auto wsaBody = saeMsg->SrvAdvMsg()->body();
                    if (role_ == Role::OBU and wsaBody.routingAdvertisement().has_value() and
                        wra_ != nullptr) {
                        LOGD(__FUNCTION__, " settting IP");
                        ret = wra_->onReceiveWra(wsaBody.routingAdvertisement().value(),
                                                 rxMsg->srcMac_, rxMsg->macAddrLen_);
                        enqueueMsg(rxMsg);
                        goto handleMsg;
                    }
                } else if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_RTCM) {
                    LOGD(__FUNCTION__, " Received RTCM");
                    if (role_ == Role::OBU and rtcm_ != nullptr) {
                        if (rtcm_->onReceiveRTCM(*(saeMsg->RTCMcorrection())) == Status::SUCCESS) {
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
                    asyncCbData->rvBsmData.dataType = (uint32_t)32; // should be 0x20 or 32
                    asyncCbData->rvBsmData.id = 0;
                    for (int i = 7; i >= 0 ; i--) {
                        asyncCbData->rvBsmData.id |= ((coreData.id()[i] << 8*i) & 0x00);
                    }

                    asyncCbData->rvBsmData.msgCount = coreData.msgCnt();
                    asyncCbData->rvBsmData.latitude = coreData.lat();
                    asyncCbData->rvBsmData.longitude = coreData.longitude();
                    asyncCbData->rvBsmData.elevation =  coreData.elev();
                    asyncCbData->rvBsmData.speed =  coreData.speed();
                    asyncCbData->rvBsmData.heading =  coreData.heading();

                    // AccelerationSet4Way
                    // same definitions as in DefAcceleration.asn
                    asyncCbData->rvBsmData.longitudeAcceleration =  coreData.accelSet().longitude();
                    asyncCbData->rvBsmData.latitudeAcceleration =  coreData.accelSet().lat();
                    asyncCbData->rvBsmData.verticalAcceleration =  coreData.accelSet().vert();
                    asyncCbData->rvBsmData.yawAcceleration =  coreData.accelSet().yaw();
                    // J2735::idl::BrakeSystemStatus

                    int i = 0;
                    for (; i < sizeof(coreData.brakes().wheelBrakes()); i++) {
                        asyncCbData->rvBsmData.brakes =
                            ((coreData.brakes().wheelBrakes() << i) & (0x01 << i));
                    }
                    for (; i < sizeof(coreData.brakes().traction()); i++) {
                        asyncCbData->rvBsmData.brakes =
                            ((coreData.brakes().traction() << i) & (0x01 << i));
                    }
                    for (; i < sizeof(coreData.brakes().abs()); i++) {
                        asyncCbData->rvBsmData.brakes =
                            ((coreData.brakes().abs() << i) & (0x01 << i));
                    }
                    for (; i < sizeof(coreData.brakes().brakeBoost()); i++) {
                        asyncCbData->rvBsmData.brakes =
                            ((coreData.brakes().brakeBoost() << i) & (0x01 << i));
                    }
                    for (; i < sizeof(coreData.brakes().auxBrakes()); i++) {
                        asyncCbData->rvBsmData.brakes =
                            ((coreData.brakes().auxBrakes() << i) & (0x01 << i));
                    }

                    // this function will check the consistency, relevancy, signature, and mbd
                    // the async verify function will then call your callback
                    // checkMsgSecurity(asyncCbData, priority, callbackFn);
                    // provide aerolink the exact location given by sender in its BSM
                    if (secInterface_->setGenLocation(&asyncCbData->smp,
                                                      &asyncCbData->rvBsmData) < 0) {
                        ret = -1;
                    }
                    // check consistency
                    if (secInterface_->checkConsistency(&asyncCbData->smp) < 0) {
                        msgStatistics.setConsistCheckFailed(
                            msgStatistics.getConsistCheckFailed() + 1);
                        rxStatistics.setConsistCheckFailed(
                            rxStatistics.getConsistCheckFailed() + 1);
                        ret = -1;
                    }
                    // check relevancy
                    if (secInterface_->checkRelevance(&asyncCbData->smp) < 0) {
                        msgStatistics.setRelevanceCheckFailed(
                            msgStatistics.getRelevanceCheckFailed() + 1);
                        rxStatistics.setRelevanceCheckFailed(
                            rxStatistics.getRelevanceCheckFailed() + 1);
                        ret = -1;
                    }
                    // verify the packet
                    // extract priority from traffic class of packet
                    uint8_t priority = (uint8_t)TrafficIpType::TRAFFIC_NON_IP;
                    asyncCbData->rxMsg = std::dynamic_pointer_cast<void>(saeMsg);
                    // if (congCtrlInit_) {
                    //     asyncCbData->congCtrl = static_cast<void*>(congestCtrl_.get());
                    // }
                    asyncCbData->asyncState = VERIFYING;
                    secInterface_->asyncVerify(
                        asyncCbData,
                        priority,
                        &(asyncCbData->smp),
                        [] (AEROLINK_RESULT returnCode, void *userData) -> void {
                            asyncCbData_t* cbData = (asyncCbData_t*) userData;
                            if (cbData == nullptr) {
                                LOGE(__FUNCTION__, " cb_data is a null pointer");
                                return;
                            }
                            std::shared_ptr<SplitsCore> core = cbData->splitsCore.lock();
                            if (nullptr == core) {
                                LOGE(__FUNCTION__, " core is null");
                                return;
                            }
                            auto coreImpl = std::dynamic_pointer_cast<SplitsCoreImpl>(core);
                            if (nullptr == coreImpl) {
                                LOGE(__FUNCTION__, " coreImpl is null");
                                return;
                            }

                            std::shared_ptr<v2xMsgSae> rxV2xMsgSae =
                                std::static_pointer_cast<v2xMsgSae>(cbData->rxMsg);
                            if (returnCode != WS_SUCCESS) {
                                cbData->verifSuccess = false;
                                cbData->asyncState = VERIF_DONE;
                                cbData->asyncState = PP_DONE;
                                cbData->asyncState = FREE;
                                return;
                            } else {
                                totalVerifSuccess++;
                                if (lastTimeStamp == 0) {
                                    lastTimeStamp = timestamp_now();
                                }
                                if (totalVerifSuccess % 1000 == 0) {
                                    uint64_t currTime = timestamp_now();
                                    double rate = (1000.0 / (double)(currTime - lastTimeStamp));
                                    LOGI(__FUNCTION__, " Average rate is: ", rate, "k Vhz");
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
                            if (result != WS_SUCCESS) {
                                if (result == WS_ERR_MISBEHAVIOR_DETECTED) {
                                    LOGD(__FUNCTION__, " Detected Misbehavior Class is ",
                                         misbehaviorResult.detectedMisbehavior);
                                } else {
                                    LOGE(__FUNCTION__, " Error in checking misbehavior");
                                }
                                // don't forward if mbd fail
                            }

                            coreImpl->updateVerificationResult(rxV2xMsgSae->msgType_,
                                                               rxV2xMsgSae->port_,
                                                               cbData->verifSuccess,
                                                               result == WS_ERR_MISBEHAVIOR_DETECTED);

                            // filtering
                            // don't forward if filtered out
                            // postprocessing
                            // congestion control
                            uint64_t timestamp_ms = timestamp_now();
                            // CongestCtrl* congCtrl = static_cast<CongestCtrl*>(cbData->congCtrl);
                            auto congCtrl = coreImpl->congestCtrl_;

                            if (congCtrl != nullptr) {
                                // adding cong ctrl data
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
                    LOGD(__FUNCTION__, " queued and freeing decoded unsigned msg");
                    ret = 0;
                }
            }
        } else {
            //unsecured case
            time_start_ms = timestamp_now();
            time_start_us = timestamp_now_us();
            ret = codec_->decodeFrame(rxMsg, false, 0, nullptr, 0);
            time_finish_us = timestamp_now_us();
            time_finish_ms = timestamp_now();
            if (time_start_ms == time_finish_ms) {
                LOGD(__FUNCTION__, " ts: ", time_finish_us - time_start_us, " (us)");
            } else {
                LOGD(__FUNCTION__, " ts: > 1ms");
            }

            //handle locally consumed messages
            if (st_ == StackType::STACK_TYPE_SAE) {
                std::shared_ptr<v2xMsgSae> saeMsg = std::dynamic_pointer_cast<v2xMsgSae>(rxMsg);
                if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_WSA) {
                    LOGD(__FUNCTION__, " Received WSA");
                    auto wsaBody = saeMsg->SrvAdvMsg()->body();
                    if (role_ == Role::OBU and wsaBody.routingAdvertisement().has_value() and
                        wra_ != nullptr) {
                        LOGD(__FUNCTION__, " setting IP");
                        wra_->onReceiveWra(wsaBody.routingAdvertisement().value(),
                                           saeMsg->srcMac_, saeMsg->macAddrLen_);
                        putFreeMsg(rxMsg);
                        continue;
                    }
                } else if (saeMsg->msgType_ == v2xMsgType::V2X_SAE_MSG_RTCM) {
                    LOGD(__FUNCTION__, " Received RTCM");
                    if (role_ == Role::OBU and rtcm_ != nullptr) {
                        if (rtcm_->onReceiveRTCM(*(saeMsg->RTCMcorrection())) != Status::SUCCESS) {
                            LOGE(__FUNCTION__, " failed to inject RTCM");
                        }
                        putFreeMsg(rxMsg);
                        continue;
                    }
                }
            }

            if (ret != 0) {
                LOGE(__FUNCTION__, " decode error");
                ret = -1;
                goto handleMsg;
            }

            auto& msgStatistics =
                allMessageStatistics_[static_cast<SplitsStatistics::MessageID>(rxMsg->msgType_)];
            msgStatistics.setRxUnsignedCount(msgStatistics.getRxUnsignedCount() + 1);
        }
handleMsg:
        if (ret != -1) {
            numPublished++;
            publisher_->publish(rxMsg);
        }
        if (rxMsg != nullptr) {
            rxMsg->buf = bufStart;
            putFreeMsg(rxMsg);
        }
    }
    return ret;
}

const SplitsStatistics::CongestionControlStatistics&
    SplitsCoreImpl::getCongestionControlStatistics() {
    LOGD(__FUNCTION__);
    // TODO, better to add mutex here.
    if (congCtrlInit_ && congestCtrl_) {
        auto data = congestCtrl_->getCongCtrlCalculationData();
        congestionControlstatistics_.setNumRVsInRange(data.totalRvsInRange);
        congestionControlstatistics_.setTxMaxITTInterval(data.updateMaxITT);
        congestionControlstatistics_.setVehicleDensity(data.smoothDens);
        if (data.channData) {
            congestionControlstatistics_.setCQI(data.channData->channQualInd);
        }
    }
    
    return congestionControlstatistics_;
}

const SplitsStatistics::SecurityStatistics& SplitsCoreImpl::getSecurityStatistics() {
    LOGD(__FUNCTION__);
    if (configData_->getCoreConfig().getEnableSecurity() && secInterface_) {
        secInterface_->changeIdNow();
        ::v1::commonapi::SPLITS::SplitsStatistics::HvBSMSpecific value;
        value.setPseudonymCertChangePassed(secInterface_->timeSinceLastIdChange());
        value.setPseudonymCertChangeDistance(secInterface_->distSinceLastIdChange());

        uint32_t id =
            (uint32_t)idChangeData_.tempId[0] << 24 |
            (uint32_t)idChangeData_.tempId[1] << 16 |
            (uint32_t)idChangeData_.tempId[2] << 8  |
            (uint32_t)idChangeData_.tempId[3];

        value.setTempID(id);
        securityStatistics_.setBsm(value);
    }
    return securityStatistics_;
}

std::shared_ptr<v2xMsg> SplitsCoreImpl::fillLocalMsg(int id) {
    std::shared_ptr<v2xMsgSae> msg = nullptr;
    switch (id) {
        case SAE_PSID_BSM:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txSpsMsgs_[id]);
            if (msg == nullptr) {
                LOGE(__FUNCTION__, " No sps msg!");
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_BSM;

            // we need to check for distance and time since last id change to determine
            // if a change should happen
            if (configData_->getCoreConfig().getEnableSecurity() && secInterface_) {
                if (secInterface_->changeIdNow()) {
                    if (secInterface_->idChange() < 0) {
                        LOGE(__FUNCTION__, " Id Change Failure");
                    }
                }
            }

            if (bsmGenerator_->fillBsm(msg, idChangeData_.idChanged,
                                       idChangeData_.tempId) != Status::SUCCESS) {
                LOGE(__FUNCTION__, " Failed to init bsm msg");
                msg = nullptr;
                goto out;
            }
            // reset here
            idChangeData_.idChanged = false;
            break;
        case SAE_PSID_WSA:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txSpsMsgs_[id]);
            if (msg == nullptr) {
                LOGE(__FUNCTION__, " No sps msg!");
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_WSA;
            if (wra_ != nullptr) {
                if (wra_->fillWra(msg)) {
                    LOGE(__FUNCTION__, " Failed to init WSA msg");
                    msg = nullptr;
                    goto out;
                }
            } else {
                LOGE(__FUNCTION__, " wsa not initialized");
                msg = nullptr;
                goto out;
            }
            break;
        case SAE_PSID_RTCM:
            msg = std::dynamic_pointer_cast<v2xMsgSae>(txEventMsgs_[id]);
            if (msg == nullptr) {
                LOGE(__FUNCTION__, " No event msg!");
                goto out;
            }
            msg->msgType_ = v2xMsgType::V2X_SAE_MSG_RTCM;
            if (rtcm_ != nullptr) {
                if (rtcm_->fillRTCM(msg) != Status::SUCCESS) {
                    LOGE(__FUNCTION__, " failed to initialize RTCM msg");
                    goto out;
                }
            } else {
                LOGE(__FUNCTION__, " RTCM object not initialized");
                msg = nullptr;
                goto out;
            }
            break;
        default:
            LOGE(__FUNCTION__,
                 " not supporting local generation of message with Message Id: ", id);
            break;
    }
out:
    return msg;
}

void SplitsCoreImpl::recordBSM(const std::shared_ptr<v2xMsg> msg, bool tx) {
    // write BSM messages to csv
    if (v2xMsgType::V2X_SAE_MSG_BSM == msg->msgType_) {
        std::shared_ptr<v2xMsgSae> bsm_msg = std::dynamic_pointer_cast<v2xMsgSae>(msg);
        std::shared_ptr<J2735::idl::BasicSafetyMessage> sp = bsm_msg->BSM();
        //BsmCoreData
        J2735::idl::BSMCoreData& coreData = sp->coreData();
        std::stringstream line;
        J2735::idl::TemporaryID idlID = coreData.id();
        if (csv_) {
            line <<
                static_cast<int>(coreData.msgCnt()) << "," <<
                std::hex << std::setfill('0') << std::setw(2) <<
                idlID[0] << idlID[1] << idlID[2] << idlID[3] << ",";
            line <<
                std::dec << coreData.secMark() << "," <<
                coreData.lat() << "," <<
                coreData.longitude() << "," <<
                static_cast<int>(coreData.elev()) << "," <<
                coreData.accuracy().semiMajor() << "," <<
                coreData.accuracy().semiMinor() << "," <<
                coreData.accuracy().orientation() << "," <<
                coreData.transmission() << "," <<
                coreData.speed() << "," <<
                coreData.heading() << "," <<
                static_cast<int>(coreData.angle()) << "," <<
                coreData.accelSet().longitude() << "," <<
                coreData.accelSet().lat() << "," <<
                static_cast<int>(coreData.accelSet().vert()) << "," <<
                coreData.accelSet().yaw() << "," <<
                coreData.brakes().wheelBrakes() << "," <<
                coreData.brakes().traction() << "," <<
                coreData.brakes().abs() << "," <<
                coreData.brakes().scs() << "," <<
                coreData.brakes().brakeBoost() << "," <<
                coreData.brakes().auxBrakes() << "," <<
                static_cast<int>(coreData.size().length()) << "," <<
                static_cast<int>(coreData.size().width()) << ",";
            line << std::endl;
            tx ? csv_->writeTx(line.str()) : csv_->writeRx(line.str());
        }
    }
}

int SplitsCoreImpl::send(int msgId, std::shared_ptr<v2xMsg> msg) {
    int ret = -1;
    uint8_t signedPdu[512];
    uint32_t signedPduLen = 512;

    // TODO: use proper priority for different messages.
    Priority priority = static_cast<Priority>(DEFAULT_BSM_PRIORITY);
    // TODO: message ID map to service ID, spsTransmits_ key is service ID actually
    std::map<int, RadioTransmit>::iterator it = spsTransmits_.find(msgId);
    if (spsTransmits_.end() == it) {
        it = eventTransmits_.find(msgId);
        if (eventTransmits_.end() == it) {
            LOGE(__FUNCTION__, " No corresponding radio to send msg for msgId: ", msgId);
            ret = -1;
            return ret;
        }
    } else {
        if (!congCtrlInit_ && SAE_PSID_BSM == msgId) {
            LOGI(__FUNCTION__, " Init Congestion Ctrl");
            congestCtrl_->init(congCtrlConfig_, std::make_shared<RadioTransmit>(it->second));
            congCtrlInit_ = true;
        }
    }

    // TODO: message ID map to service ID
    auto& statistics = allTxStatistics_[static_cast<SplitsStatistics::PSID>(msgId)];
    auto& msgStatistics = allMessageStatistics_[static_cast<SplitsStatistics::MessageID>(msgId)];

    if (msg == nullptr) {
        //called by txScheduler for local generation
        msg = fillLocalMsg(msgId);
        if (msg == nullptr) {
            LOGE(__FUNCTION__, " No message");
            statistics.setTxFailed(statistics.getTxFailed() + 1);
            return ret;
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
            LOGE(__FUNCTION__, " Failed to encode msg");

            statistics.setEncodeFailed(statistics.getEncodeFailed() + 1);
            statistics.setTxFailed(statistics.getTxFailed() + 1);

            ::v1::commonapi::SPLITS::SplitsStatistics::AbnormalIncident value;
            value.setTimestamp(timestamp_now());
            value.setErr(::v1::commonapi::SPLITS::SplitsCommon::ErrorCode::ENCODE_FAIL);
            value.setDetail("Failed to encode msg");
            splitsStatisticsService_->reportAbnormalies(value);
            goto out;
        }

        //sign message based on psid and ssp
        if(configData_->getCoreConfig().getEnableSecurity() && secInterface_){
            auto secConfig = configData_->getSecConfig();
            if ((ret = secInterface_->signMsg(&smg_,
                                              (uint8_t*)msg->buf, msg->bufLen,
                                              signedPdu, signedPduLen, secConfig.getPsidValue(),
                                              secConfig.getEnableSsp(), secConfig.getSspValue().data(),
                                              secConfig.getSspLength(),
                                              secConfig.getEnableSspMask(),
                                              secConfig.getSspMask().data(),
                                              secConfig.getSspMaskLength(),
                                              SignerTypeOverride::STO_AUTO))  == 0) {
                //optimize this
                LOGD(__FUNCTION__, " New signed payload length is: ", signedPduLen);
                msg->bufLen = signedPduLen;
                std::memcpy(msg->buf, signedPdu, signedPduLen);
                //encode Wsmp header and prepend to begining of buffer
                if (codec_->encodeWsmp(msg, true)) {
                    LOGE(__FUNCTION__, " Failed to encode WSMP header");
                    statistics.setEncodeFailed(statistics.getEncodeFailed() + 1);
                    statistics.setTxFailed(statistics.getTxFailed() + 1);
                    goto out;
                }
            } else {
                statistics.setSignFailed(statistics.getSignFailed() + 1);
                statistics.setTxFailed(statistics.getTxFailed() + 1);
                LOGE(__FUNCTION__, " fail to sign msg");
                goto out;
            }
        }
    }

    recordBSM(msg);

    ret = it->second.transmit(msg->buf, msg->bufLen, priority);
    if (ret) {
        LOGE(__FUNCTION__, " Transmit failure");
        statistics.setTxFailed(statistics.getTxFailed() + 1);
        goto out;
    }
    statistics.setTxNumber(statistics.getTxNumber() + 1);
    msgStatistics.setTxNumber(msgStatistics.getTxNumber() + 1);
    LOGI(__FUNCTION__, " msgId = ", msgId, " TxNumber = ", statistics.getTxNumber());
    ret = 0;
 out:
     //reset msg
    if (msg) {
        msg->buf = msg->head;
        msg->bufLen = TX_BUF_LEN;
        //TODO: properly delete and reallocate structure
        // perhaps even make a cache per msg type
    }
    return ret;
}

void SplitsCoreImpl::run(void) {
    LOGD(__FUNCTION__);
    const SplitsConfig::CoreConfig & coreCfg = configData_->getCoreConfig();

    if (useTxPduFile_ == true) {
        // send MAP (msg index = 1)
        txScheduler_->scheduleSpsTransmit(1, 1000);
    } else {
        // TODO: should do a map between service PSID and message ID
        for (auto const & txSpsFlow : coreCfg.getSpsFlows()) {
            txScheduler_->scheduleSpsTransmit(txSpsFlow.getServiceId(),
                                              txSpsFlow.getPeriodicity());
        }
        for (auto const & txEventFlow : coreCfg.getEventFlows()) {
            if (static_cast<MsgSource>(txEventFlow.getSource()) == MsgSource::LOCAL) {
                txScheduler_->scheduleEventTransmit(txEventFlow.getServiceId(),
                                                    txEventFlow.getPeriodicity());
            }
        }
    }
    txScheduler_->start();

    processRxThread_ = std::thread(&SplitsCoreImpl::processReceive, this);

    int rxid = 0;
    // TODO: Fix this empty while-loop
    while (!congCtrlInit_) {}
    for (auto const & radioRx : radioReceives_) {
        auto rxThread =
            std::thread([&, rxid]() {
                while (!stop_) {
                    receive(rxid, RX_BUF_LEN);
                }

            });
        rxThreads_.push_back(std::move(rxThread));
        rxid++;
    }
    while (stop_ == false) {
       if (congCtrlInit_) {
           receive(0, RX_BUF_LEN);
       }
       std::this_thread::sleep_for(std::chrono::microseconds(10000));
   }
}
}; //namespace Core
}; //namespace SPLITS
