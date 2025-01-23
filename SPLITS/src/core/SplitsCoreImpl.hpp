/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SPLITS_COREIMPL_HPP_
#define SPLITS_COREIMPL_HPP_

#include <mutex>

#include "RadioReceive.h"
#include "RadioTransmit.h"
#include "SplitsCore.hpp"
#include "v2xMsg.hpp"
#include "BsmMsgGenerator.hpp"
#include "TxScheduler.hpp"
#include "CongestCtrl.hpp"
#include "SecurityInterface.hpp"
#include "location/KinematicsReceive.hpp"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"
#include "v1/commonapi/SPLITS/SplitsStatistics.hpp"
#include "SplitsConfigIdl.hpp"
#include "SplitsConfigFile.hpp"
#include "SplitsWra.hpp"
#include "SplitsRTCM.hpp"
#include "SplitsServiceDataPublisher.hpp"
#include "SplitsDataSubscriber.hpp"
#include "SplitsCoreDataListener.hpp"
#include "SplitsStatisticsStubImpl.hpp"

using SPLITS::v2xMsg;
using v1::commonapi::SPLITS::SplitsConfig;
using v1::commonapi::SPLITS::SplitsStatistics;
using SPLITS::Config::SplitsConfigIdl;
using SPLITS::Config::SplitsConfigFile;
using SPLITS::Codec::SplitsCodec;
using SPLITS::StackType;
using SPLITS::Interface::SplitsServiceDataPublisher;
using SPLITS::Interface::SplitsDataSubscriber;
using SPLITS::Interface::SplitsStatisticsStubImpl;
using SPLITS::Core::SplitsCoreDataListener;

namespace CommonAPI {
    class Runtime;
}

namespace SPLITS {

namespace Core {

class CsvIO;

class SplitsCoreImpl : public std::enable_shared_from_this<SplitsCoreImpl>,
    public SPLITS::Core::SplitsCore {
public:
    SplitsCoreImpl(StackType st = StackType::STACK_TYPE_SAE);
    ~SplitsCoreImpl();

    std::shared_ptr<SplitsCoreImpl> getptr() {
        return shared_from_this();
    }
    void run(void) override;
    virtual int init(std::string cfgFileName,
                     std::string rxpduFileName,
                     std::string txpduFileName,
                     std::string csvFileDir) override;
    virtual void deinit(void) override {};

    virtual int send(int msgId, std::shared_ptr<v2xMsg> msg) override;

    virtual int receive(const uint8_t index, const uint16_t bufLen) override;

    virtual void clearRadioInstance() override{};

    virtual void closeAllRadio() override {};
    friend class SplitsCoreDataListener;

    void recordBSM(const std::shared_ptr<v2xMsg> msg, bool tx = true);
    
    const SplitsStatistics::AllTxStatistics& getTxStatistics() const {
        return allTxStatistics_;
    }

    const SplitsStatistics::AllRxStatistics& getRxStatistics() const {
        return allRxStatistics_;
    }

    const SplitsStatistics::AllMessageStatistics& getAllMessageStatistics() const {
        return allMessageStatistics_;
    }

    const SplitsStatistics::AllPSIDStatistics& getStatisticsofAllPSID() const {
        return allPSIDStatistics_;
    }

    const SplitsStatistics::CongestionControlStatistics& getCongestionControlStatistics();

    const SplitsStatistics::SecurityStatistics& getSecurityStatistics();

    void updateVerificationResult(v2xMsgType msgType, uint16_t port,
                                  bool verifSuccess, bool misbehavir);

protected:

    std::vector<RadioReceive> radioReceives_;
    std::map<int, RadioTransmit> eventTransmits_;
    std::map<int, RadioTransmit> spsTransmits_;
    SplitsStatistics::AllTxStatistics allTxStatistics_;
    SplitsStatistics::AllRxStatistics allRxStatistics_;
    SplitsStatistics::AllMessageStatistics allMessageStatistics_;
    SplitsStatistics::AllPSIDStatistics allPSIDStatistics_;
    SplitsStatistics::CongestionControlStatistics congestionControlstatistics_;
    SplitsStatistics::SecurityStatistics securityStatistics_;

private:
    int adjustSpsPeriodicity(int intervalMs);
    int readPduFromFile(std::string file, char *buf, int bufLen);
    int initMsgs(void) override;
    int initRadio(void);
    int initSecurity(const std::string ctxName, const uint16_t countryCode,
                    const std::string lcmName);
    std::shared_ptr<v2xMsg> fillLocalMsg(int serviceId);
    int processReceive(void);

    bool useRxPduFile_ = false;
    bool useTxPduFile_ = false;
    bool stop_ = false;
    SecuredMessageGeneratorC smg_;
    std::vector<SecuredMessageParserC> smps_;
    IDChangeData idChangeData_;
    std::vector<std::thread> rxThreads_;
    std::thread processRxThread_;
    CongCtrlConfig congCtrlConfig_;
    bool congCtrlInit_ = false;
    std::mutex statisticsMutex_;
    std::shared_ptr<SplitsCodec> codec_ = nullptr;
    std::shared_ptr<SplitsServiceDataPublisher> publisher_ = nullptr;
    std::shared_ptr<SplitsDataSubscriber> subscriber_ = nullptr;
    std::shared_ptr<CommonAPI::Runtime> runtime_ =nullptr;
    std::shared_ptr<SplitsStatisticsStubImpl> splitsStatisticsService_;
    std::shared_ptr<SplitsConfig::SplitsConfigData> configData_ = nullptr;
    std::shared_ptr<SplitsConfigFile> configFile_ = nullptr;
    std::shared_ptr<SplitsConfigIdl> configIdl_ = nullptr;
    std::shared_ptr<BsmMsgGenerator> bsmGenerator_ = nullptr;
    std::shared_ptr<SecurityInterface> secInterface_ = nullptr;
    std::shared_ptr<KinematicsReceive> kinematic_ = nullptr;
    std::shared_ptr<ILocationInfoEx> hvLocationInfo_ = nullptr;
    std::shared_ptr<TxScheduler> txScheduler_ = nullptr;
    std::shared_ptr<CongestCtrl> congestCtrl_ = nullptr;
    std::shared_ptr<CsvIO> csv_ = nullptr;
    std::shared_ptr<SplitsWra> wra_ = nullptr;
    std::shared_ptr<SplitsRTCM> rtcm_ = nullptr;
    std::shared_ptr<SplitsCoreDataListener> dataListener_ = nullptr;
    SPLITS::Core::Role role_;
};


}; //namespace Core
}; //namespace SPLITS

#endif  //SPLITS_CORE_HPP_
