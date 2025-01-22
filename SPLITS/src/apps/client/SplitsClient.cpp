/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file SplitsClient.cpp
 *
 * @description This is a sample client program for SPLITS, it demonstrate following functinoalities
 *
 * 1. Receive verified/decoded J2735 msgs via fast-dds interface (SplitsDataSubscriber)
 *    The config file: SplitsConfig.json specifies what messages client can receive, in the section
 *    "ReceiveFlows"
 *
 * 2. Send J2735 payload (encoded, but not signed, no WSMP header) to SPLITS via fast-dds interface
 * (SplitsDataPublisher), SPLITS sign the message and transmit it out via cv2x radio.
 *
 * 3. Connects to NTRIP server (or read from rtcm data source file), send the RTCM message via
 * fast-dds interface (SplitsDataPublisher), the message is idl format, unencoded.
 * 
 */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include <CommonAPI/CommonAPI.hpp>
#include "v2xMsgTypes.hpp"
#include "SplitsCore.hpp"

#include "SplitsDataSubscriber.hpp"
#include "SplitsClientDataPublisher.hpp"
#include "SplitsNtrip.hpp"
#include "v2xSaeMsgLogger.hpp"
#include "SplitsConfigStubImpl.hpp"
#include "SplitsConfigFile.hpp"
#include "SplitsRemoteRTCM.hpp"
#include "SplitsVehicleDataService.hpp"

using SPLITS::v2xMsg;
using SPLITS::v2xSaeMsgLogger;
using SPLITS::Config::SplitsConfigFile;
using SPLITS::Config::SplitsConfigStubImpl;
using SPLITS::Core::Role;
using SPLITS::MsgSource;
using SPLITS::MsgFormat;
using SPLITS::Interface::SplitsDataSubscriber;
using SPLITS::Interface::SplitsClientDataPublisher;
using SPLITS::Interface::SplitsDataListener;

static char pduBuf[1024];
int pduBufLen = 1024;
v2xSaeMsgLogger logger(&std::cout);

class SampleDataListener : public SplitsDataListener {
    void onBsmReady(std::shared_ptr<J2735::idl::BasicSafetyMessage> msg) {
        logger.logMsg(*msg);
    }
    void onPduReady(const J2735::idl::MsgData &pdu) {
    }
};

int readPduFromFile(std::string file, char *buf, int bufLen)
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

int main(int argc, char **argv)
{
    bool exit = false;
    int c;
    std::string cfgFileName;
    std::string pduFileName;
    std::string msgName;
    uint32_t pduPsid = 0;
    CommonAPI::Runtime::setProperty("LogContext", "SplitsService");
    CommonAPI::Runtime::setProperty("LogApplication", "SplitsService");
    CommonAPI::Runtime::setProperty("LibraryBase", "SplitsService");

    while (( c = getopt(argc, argv, "hf:p:m:")) != -1 ) {
        switch(c) {
            case 'f':
                if (optarg) {
                    cfgFileName = optarg;
                } else {
                    std::cout << "Must followed by a config file name if '-f' supplied" << std::endl;
                    return -1;
                }
                break;
            case 'p':
                if (optarg) {
                    pduFileName = optarg;
                } else {
                    std::cout << "Must provide pdu file name with -p" << std::endl;
                    return -1;
                }
                break;
            case 'm':
                if (optarg) {
                    msgName = optarg;
                } else {
                    std::cout << "Must provide msg name, e.g. MAP, SPAT, etc" << std::endl;
                    return -1;
                }
                break;
            case 'h':
            default:
                std::cout << "Usage: splits_client -f cfgFileName" << std::endl;
                return -1;
        }
    }
    if (!pduFileName.empty()) {
    }

    std::shared_ptr<SplitsConfig::SplitsConfigData> cfg = std::make_shared<SplitsConfig::SplitsConfigData>();
    SplitsConfigFile cfgFile(cfg);
    if (cfgFile.loadConfigFromFile(cfgFileName)) {
        std::cerr << "Failed to load config file" << std::endl;
        return -1;
    }
    const SplitsConfig::CoreConfig &coreCfg = cfg->getCoreConfig();
    auto cfgStub = std::make_shared<SplitsConfigStubImpl>(cfg);
    auto runtime = CommonAPI::Runtime::get();
    std::string domain = "local";
    std::string instance = "commonapi.SPLITS.SplitsConfig";
    std::string connection = "SPLITS";

    while (!runtime->registerService(domain,
                instance,
                cfgStub,
                connection)) {
        std::cout << "registing configService " << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "registed configService " << std::endl;


    //Receive msg via subscriber interface. only showing bsm message
    std::shared_ptr<SplitsDataSubscriber> sub = std::make_shared<SplitsDataSubscriber>();
    if (sub->init(std::string("SERVICE")) == false) {
        COMMONAPI_ERROR("Failed to init dds subscriber");
        return -1;
    }
    auto listener = std::make_shared<SampleDataListener>();
    if (sub->registerListener(listener) == 0 ) {
        COMMONAPI_INFO("data listener registered");
    }

    //Transmit message via client data publisher interface.
    std::shared_ptr<SplitsClientDataPublisher> pub = std::make_shared<SplitsClientDataPublisher>();
    if (pub->init(std::string("CLIENT")) == false) {
        COMMONAPI_ERROR("Failed to init dds publisher");
        return -1;
    }
    pub->run();
    std::cout << "pub is running" << std::endl;

    // Initialize RTCM if we are an RSU, and asked to generate RTCM from Splits Client side(EAP)
    std::thread rtcmThread;
    std::shared_ptr<SplitsRemoteRTCM> rtcm = nullptr;
    auto rtcmCfg = std::make_shared<SplitsConfig::RTCMConfig>(cfg->getRtcmConfig());
    if (coreCfg.getEnableRTCM() && static_cast<Role>(coreCfg.getRole()) == Role::RSU ) {
        //Initialize the RTCM object, only if the corresponding TxFlow for RTCM message has the
        //message source as "Remote".
        for (auto const& txEventFlow : coreCfg.getEventFlows()) {
            if (txEventFlow.getServiceId() == SAE_PSID_RTCM &&
                    static_cast<MsgSource>(txEventFlow.getSource()) == MsgSource::REMOTE) {

                rtcm = std::make_shared<SplitsRemoteRTCM>(rtcmCfg);
                if (rtcm->init()) {
                    std::cerr << "failed to init rtcm " << std::endl;
                    return -1;
                }

                rtcmThread = std::thread([&]() {
                    while(!exit) {
                        auto msg = std::dynamic_pointer_cast<v2xMsgSae>(pub->getFreeMsg());
                        if (msg == nullptr) {
                            std::cerr << "no msg " << std::endl;
                        } else {
                            msg->msgType_ = SPLITS::v2xMsgType::V2X_SAE_MSG_RTCM;
                            if (rtcm->fillRTCM(msg)) {
                                std::cerr << "failed to fill RTCM msg" << std::endl;
                                break;
                            }
                            std::cout << "queued rtcm msg" << std::endl;
                            pub->enqueueMsg(msg);
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(txEventFlow.getPeriodicity()*1000));
                    }
                });
            }
        }
    }

    std::thread pduThread;
    auto msgData = std::make_shared<J2735::idl::MsgData>();
    std::vector<uint8_t> pdu;
    if (!pduFileName.empty()) {
        //read pdu from file and transmit it via client data publisher interface.
        //the config file must have corresponding flow for this message.
        std::cout << "reading from pdu file"  << std::endl;
        pduBufLen = readPduFromFile(pduFileName, pduBuf, pduBufLen);
        if (pduBufLen < 0) {
            std::cerr << "Failed to read file " << pduFileName << std::endl;
            return -1;
        }
        msgData->length(pduBufLen);
        pdu.assign(pduBuf, pduBuf + pduBufLen);
        msgData->msg(std::move(pdu));
        if ((pduPsid = cfgFile.getPsidFromName(msgName)) < 0) {
            std::cerr << "Msg name: " << msgName << " is not valid" << std::endl;
            return -1;
        }
        msgData->serviceId(pduPsid);
        msgData->format(J2735::idl::IDL_MSG_FORMAT_UNSIGNED);

        for (auto const& txEventFlow : coreCfg.getEventFlows()) {
            if (static_cast<MsgSource>(txEventFlow.getSource()) == MsgSource::REMOTE &&
                    pduPsid == txEventFlow.getServiceId()) {

                pduThread = std::thread([&]() {
                        while(!exit) {
                            auto msg = std::dynamic_pointer_cast<v2xMsgSae>(pub->getFreeMsg());
                            if (msg == nullptr) {
                                std::cerr << "no tx msg to use" << std::endl;
                            } else {
                                msg->MsgData(msgData);
                                msg->msgType_ = SPLITS::v2xMsgType::V2X_MSG_PDU;
                                pub->enqueueMsg(msg);
                            std::cout << "queued msg" << std::endl;
                            }
                            std::this_thread::sleep_for(std::chrono::microseconds(txEventFlow.getPeriodicity()*1000));
                        }

                });

            }
        }

    }

    auto vehicleDataService = std::make_shared<SplitsVehicleDataService>();
    if (vehicleDataService->init()) {
        std::cerr << "Failed to init vehicle data service, continue running..." << std::endl;
    } else {
        vehicleDataService->run();
    }

    do {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    } while(1);

    return 0;
}
