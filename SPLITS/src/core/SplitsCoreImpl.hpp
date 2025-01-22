/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SPLITS_COREIMPL_HPP_
#define SPLITS_COREIMPL_HPP_

#include "RadioReceive.h"
#include "RadioTransmit.h"
#include "SplitsCore.hpp"
#include "v2xMsg.hpp"
#include "BsmMsgGenerator.hpp"
//#include "SplitsFilter.hpp"
#include "TxScheduler.hpp"
#include "CongestCtrl.hpp"
#include "SecurityInterface.hpp"
#include "location/KinematicsReceive.hpp"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"
#include "SplitsConfigIdl.hpp"
#include "SplitsConfigFile.hpp"
#include "SplitsWra.hpp"
#include "SplitsRTCM.hpp"
#include "SplitsServiceDataPublisher.hpp"
#include "SplitsDataSubscriber.hpp"
#include "SplitsCoreDataListener.hpp"

using SPLITS::v2xMsg;
using v1::commonapi::SPLITS::SplitsConfig;
using SPLITS::Config::SplitsConfigIdl;
using SPLITS::Config::SplitsConfigFile;
using SPLITS::Codec::SplitsCodec;
using SPLITS::StackType;
using SPLITS::Interface::SplitsServiceDataPublisher;
using SPLITS::Interface::SplitsDataSubscriber;
using SPLITS::Core::SplitsCoreDataListener;

namespace CommonAPI {
    class Runtime;
}

//class SplitsDataPublisher;
//class SplitsConfigStubImpl;

namespace SPLITS {

namespace Core {

class SplitsCoreImpl : public std::enable_shared_from_this<SplitsCoreImpl>,
    public SPLITS::Core::SplitsCore {
public:
    SplitsCoreImpl(StackType st = StackType::STACK_TYPE_SAE) {
        st_ = st;
    }
    ~SplitsCoreImpl(){}

    std::shared_ptr<SplitsCoreImpl> getptr() {
        return shared_from_this();
    }
    void run(void) override;
    virtual int init(std::string cfgFileName, std::string rxpduFileName, std::string txpduFileName) override;
    virtual void deinit(void) override {};
    //virtual int send(int serviceId) override;
    virtual int send(int serviceId, std::shared_ptr<v2xMsg> msg) override;

    virtual int receive(const uint8_t index, const uint16_t bufLen) override;

    virtual void clearRadioInstance() override{};

    virtual void closeAllRadio() override {};
    friend class SplitsCoreDataListener;

protected:

    std::vector<RadioReceive> radioReceives_;
    std::map<int, RadioTransmit> eventTransmits_;
    std::map<int, RadioTransmit> spsTransmits_;

private:
    int adjustSpsPeriodicity(int intervalMs);
    int readPduFromFile(std::string file, char *buf, int bufLen);
    int initMsgs(void) override;
    int initRadio(void);
    int initSecurity(const std::string ctxName, const uint16_t countryCode,
                    const std::string lcmName);
    std::shared_ptr<v2xMsg> fillLocalMsg(int serviceId);
    void processReceive(void);

    bool useRxPduFile_ = false;
    bool useTxPduFile_ = false;
    bool stop_ = false;
    SecuredMessageGeneratorC smg_;
    std::vector<SecuredMessageParserC> smps_;
    IDChangeData idChangeData_;
    std::vector<std::thread> rxThreads_;
    std::thread processRxThread_;

    std::shared_ptr<SplitsCodec> codec_ = nullptr;
    std::shared_ptr<SplitsServiceDataPublisher> publisher_ = nullptr;
    std::shared_ptr<SplitsDataSubscriber> subscriber_ = nullptr;
    std::shared_ptr<CommonAPI::Runtime> runtime_ =nullptr;
    std::shared_ptr<SplitsConfig::SplitsConfigData> configData_ = nullptr;
    std::shared_ptr<SplitsConfigFile> configFile_ = nullptr;
    std::shared_ptr<SplitsConfigIdl> configIdl_ = nullptr;
    std::shared_ptr<BsmMsgGenerator> bsmGenerator_ = nullptr;
    std::shared_ptr<SecurityInterface> secInterface_ = nullptr;
    std::shared_ptr<KinematicsReceive> kinematic_ = nullptr;
    std::shared_ptr<ILocationInfoEx> hvLocationInfo_ = nullptr;
    std::shared_ptr<TxScheduler> txScheduler_ = nullptr;
    std::shared_ptr<CongestCtrl> congestCtrl_ = nullptr;
    std::shared_ptr<SplitsWra> wra_ = nullptr;
    std::shared_ptr<SplitsRTCM> rtcm_ = nullptr;
    std::shared_ptr<SplitsCoreDataListener> dataListener_ = nullptr;
    SPLITS::Core::Role role_;

};


}; //namespace Core
}; //namespace SPLITS

#endif  //SPLITS_CORE_HPP_
