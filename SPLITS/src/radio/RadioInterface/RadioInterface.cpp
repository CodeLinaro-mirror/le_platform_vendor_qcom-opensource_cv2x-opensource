/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


 /**
  * @file: RadioInterface.cpp
  *
  * @brief: Implementation of RadioInterface.h
  *
  */

#include <algorithm>
#include <vector>
#include <iterator>
#include "RadioInterface.h"

#define INVALID_CBR_VALUE (255)

shared_ptr<ICv2xRadioManager> RadioInterface::cv2xRadioManager_ = nullptr;

class Cv2xRadioListener : public ICv2xRadioListener {
public:
    virtual void onL2AddrChanged(uint32_t newL2Addr) {
        std::vector<v2x_src_l2_addr_update> l2Cbs;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            l2Cbs = l2Cbs_;
        }

        if (newL2Addr > 0 and l2Cbs.size() > 0) {
            for (auto cb : l2Cbs) {
                if (cb) {
                    cb(newL2Addr);
                }
            }
        }
    }

    void addL2AddrCallback(v2x_src_l2_addr_update cb) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (std::find(l2Cbs_.begin(), l2Cbs_.end(), cb) == l2Cbs_.end()) {
            l2Cbs_.emplace_back(cb);
        }
    }

    void deleteL2AddrCallback(v2x_src_l2_addr_update cb) {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = std::find(l2Cbs_.begin(), l2Cbs_.end(), cb);
        if (it != l2Cbs_.end()) {
            l2Cbs_.erase(it);
        }
    }

private:
    std::mutex mtx_;
    std::vector<v2x_src_l2_addr_update> l2Cbs_;
};

class Cv2xStatusListener : public telux::cv2x::ICv2xListener {
public:

    Cv2xStatusListener(telux::cv2x::Cv2xStatus status, int rVerbosity) {
        cv2xStatus_ = status;
        radioVerbosity = rVerbosity;
    };

    telux::cv2x::Cv2xStatus getCurrentStatus() {
        std::lock_guard<std::mutex> lock(mtx_);
        return cv2xStatus_;
    }

    uint8_t getCurrentCbr() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (cv2xStatus_.cbrValueValid) {
            return cv2xStatus_.cbrValue;
        }
        return INVALID_CBR_VALUE;
    }

    int waitForCv2xStatus(telux::cv2x::Cv2xStatusType status, bool& restartFlow) {
        // get initial status
        telux::cv2x::Cv2xStatus tmpStatus = getCurrentStatus();

        while (tmpStatus.rxStatus != status or tmpStatus.txStatus != status) {
            // return false if status is unknow
            if(tmpStatus.rxStatus == Cv2xStatusType::UNKNOWN or
               tmpStatus.txStatus == Cv2xStatusType::UNKNOWN) {
                return -1;
            }

            // if status is inactive, need to recreate flows
            if (tmpStatus.rxStatus == Cv2xStatusType::INACTIVE or
               tmpStatus.txStatus == Cv2xStatusType::INACTIVE) {
                restartFlow = true;
            }

            // wait for status change
            std::unique_lock<std::mutex> cvLock(mtx_);
            cv_.wait(cvLock);
            tmpStatus = cv2xStatus_;
        }
        return 0;
    }

    void onStatusChanged(telux::cv2x::Cv2xStatus status) override {
        telux::cv2x::Cv2xStatus preStatus;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            preStatus = cv2xStatus_;
            cv2xStatus_ = status;
        }

        if (status.rxStatus != preStatus.rxStatus or
            status.txStatus != preStatus.txStatus) {
            if (radioVerbosity) {
                cout << "Cv2x status updated, rxStatus:" << static_cast<int>(status.rxStatus);
                cout << ", txStatus:" << static_cast<int>(status.txStatus) << endl;
            }
            cv_.notify_all();
        }
    }

    void deinit() {
        // set cv2x status to unknown during exit
        std::lock_guard<std::mutex> lock(mtx_);
        cv2xStatus_.rxStatus = Cv2xStatusType::UNKNOWN;
        cv2xStatus_.txStatus = Cv2xStatusType::UNKNOWN;
        cv_.notify_all();
    }

    // avoid potential stuck in case deinit is not invoked
    ~Cv2xStatusListener() {
        deinit();
    }
private:
    std::condition_variable cv_;
    std::mutex mtx_;
    telux::cv2x::Cv2xStatus cv2xStatus_;
    int radioVerbosity = 0;
};

void CommonCallback::onResponse(ErrorCode error) {
    std::unique_lock<std::mutex> cvLock(cbMtx_);
    cbRecvd_ = true;
    err_ = error;
    cbCv_.notify_all();
}

ErrorCode CommonCallback::getResponse() {
    std::unique_lock<std::mutex> cvLock(cbMtx_);
    while (not cbRecvd_) {
        cbCv_.wait(cvLock);
    }
    return err_;
}

void RadioInterface::set_radio_verbosity(int value) {
    if(value)
        printf("Radio flow verbosity will be set to: %d\n", value);
    rVerbosity = value;
}

map<Cv2xStatusType, string> RadioInterface:: gCv2xStatusToString = {
    {Cv2xStatusType::INACTIVE, "INACTIVE"},
    {Cv2xStatusType::ACTIVE, "ACTIVE"},
    {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
    {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
};

bool RadioInterface::updateSrcL2(){
    auto cb = std::make_shared<CommonCallback>();
    auto respCb = [&cb](ErrorCode error) {
        cb->onResponse(error);
    };

    auto cv2xRadio = getCv2xRadio();
    if(cv2xRadio and Status::SUCCESS == cv2xRadio->updateSrcL2Info(respCb)){
        if(ErrorCode::SUCCESS == cb->getResponse()){
            return true;
        }
    }

    return false;
};

Cv2xStatusType RadioInterface::statusCheck(RadioType type) {
    Cv2xStatusType status;
    auto cb = std::make_shared<CommonCallback>();
    auto respCb = [&](Cv2xStatusEx status, ErrorCode error) {
        if (ErrorCode::SUCCESS == error) {
            this->gCv2xStatus = status;
        }
        cb->onResponse(error);
    };

    auto cv2xRadioMgr = getCv2xRadioManager();
    if (!cv2xRadioMgr or
        Status::SUCCESS != cv2xRadioMgr->requestCv2xStatus(respCb)
        or ErrorCode::SUCCESS != cb->getResponse()) {
        cerr << "Error : request for C-V2X status failed." << endl;
        gCv2xStatus.status.rxStatus = Cv2xStatusType::UNKNOWN;
        gCv2xStatus.status.txStatus = Cv2xStatusType::UNKNOWN;
    }

    if (RadioType::RX == type) {
        if (rVerbosity) {
            cout << "C-V2X RX is ";
        }
        status = gCv2xStatus.status.rxStatus;
    }
    else {
        if (rVerbosity) {
            cout << "C-V2X TX is ";
        }
        status = gCv2xStatus.status.txStatus;
    }

    if (rVerbosity) {
        cout << gCv2xStatusToString[status] << endl;
    }

    if (Cv2xStatusType::ACTIVE != status) {
        cerr << "C-V2X RX or TX status " << gCv2xStatusToString[status] << endl;
    }

    return status;
}

telux::cv2x::Cv2xStatus RadioInterface::getCurrentStatus() {
    telux::cv2x::Cv2xStatus status;
    if (cv2xStatusListener_) {
        status = cv2xStatusListener_->getCurrentStatus();
    }

    return status;
}

int RadioInterface::waitForCv2xToActivate(bool& restartFlow) {
    if (cv2xStatusListener_) {
        return cv2xStatusListener_->waitForCv2xStatus(Cv2xStatusType::ACTIVE, restartFlow);
    }
    return -1;
}

bool RadioInterface::ready(TrafficCategory category, RadioType type) {
    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };

    if (!cv2xRadioManager_) {
        auto &cv2xFactory = Cv2xFactory::getInstance();
        auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
        if (!cv2xRadioMgr) {
            std::cout << "Fail to get cv2xRadioMgr" << std::endl;
            return false;
        }

        {
            std::unique_lock<std::mutex> lck(mtx);
            cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
            /* Check that V2X radio is initialized */
            if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
                cv2xRadioManagerStatus) {
                std::cout << "V2X cv2xRadioMgr initialization failed" << std::endl;
                return false;
            }
        }

        cv2xRadioManager_ = cv2xRadioMgr;
    }

    // Get C-V2X status and make sure requested radio(Tx or Rx) is enabled
    if (statusCheck(type) != Cv2xStatusType::ACTIVE) {
        return false;
    }

    // register listener for cv2x status change
    cv2xStatusListener_ = std::make_shared<Cv2xStatusListener>(gCv2xStatus.status,rVerbosity);
    if (Status::SUCCESS != cv2xRadioManager_->registerListener(cv2xStatusListener_)) {
        cerr << "Error : register Cv2x status listener failed!" << endl;
        return false;
    }

    // Get handle to Cv2xRadio
    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    auto cb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus = status;
        cv.notify_all();
    };

    // Wait for radio to complete initialization
    auto cv2xRadio = cv2xRadioManager_->getCv2xRadio(category, cb);
    if (not cv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return false;
    }

    {
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return false;
        }
    }

    // register listener for src L2 addr updates
    radioListener_ = std::make_shared<Cv2xRadioListener>();
    if (Status::SUCCESS != cv2xRadio->registerListener(radioListener_)) {
        cerr << "Error : register Cv2x radio listener failed!" << endl;
        return false;
    }

    cv2xRadio_ = cv2xRadio;

    return true;
}

int RadioInterface::registerL2AddrCallback(v2x_src_l2_addr_update cb) {
    if (!radioListener_) {
        cerr << "Radio listener not ready!" << endl;
        return -1;
    }

    auto sp = std::dynamic_pointer_cast<Cv2xRadioListener>(radioListener_);
    if (sp) {
        sp->addL2AddrCallback(cb);
        return 0;
    }

    return -1;
}

int RadioInterface::deregisterL2AddrCallback(v2x_src_l2_addr_update cb) {
    if (!radioListener_) {
        cerr << "Radio listener not ready!" << endl;
        return -1;
    }

    auto sp = std::dynamic_pointer_cast<Cv2xRadioListener>(radioListener_);
    if (sp) {
        sp->deleteL2AddrCallback(cb);
        return 0;
    }

    return -1;
}

int RadioInterface::getV2xIfaceName(TrafficIpType type, string& ifName) {
    auto cv2xRadio = getCv2xRadio();
    if (!cv2xRadio) {
        return -1;
    }

    ifName = cv2xRadio->getIfaceNameFromIpType(type);

    if (rVerbosity > 3) {
        cout << "Get V2X-Iface Name:" << ifName << endl;
    }
    return 0;
}

uint8_t RadioInterface::getCBRValue() {
    uint8_t cbr = INVALID_CBR_VALUE;
    if (cv2xStatusListener_) {
        cbr = cv2xStatusListener_->getCurrentCbr();
    }
    return cbr;
}

uint64_t RadioInterface::latestTxRxTimeMonotonic() {
    return 0;
}

void RadioInterface::enableCsvLog(bool enable) {
    enableCsvLog_ = enable;
}

/* set the Global IP addres prefix */
int RadioInterface::setGlobalIPInfo(const telux::cv2x::IPv6AddrType &ipv6Addr,
    const uint32_t serviceId) {
    int ret = 0;

    SocketInfo tcpInfo;
    EventFlowInfo eventInfo;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return -1;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCb = [&](ErrorCode error){
            cb->onResponse(error);
        };
    if (Status::SUCCESS == cv2xRadio->setGlobalIPInfo(ipv6Addr, respCb)) {
        ErrorCode error = cb->getResponse();
        if (ErrorCode::SUCCESS == error) {
            cout<<"setGlobalIPInfo succeeds." << endl;;
            ret = 0;
        } else {
            if(rVerbosity)
                cerr<<"setGlobalIPInfo fails:" << static_cast<int>(error) << endl;;
            ret = -1;
        }
    } else {
        if (rVerbosity)
            cerr<<"setGlobalIPInfo sync fails." << endl;
        ret = -1;
    }

    /* Create IP unicast flow on port 0 */
    tcpInfo.serviceId = serviceId;
    tcpInfo.localPort = 0;
    eventInfo.isUnicast = true;
    auto commCb = std::make_shared<CommonCallback>();
    auto sockRespCb = [&](shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
                if (ErrorCode::SUCCESS == error) {
                    this->tcpSockInfo = sock;
                }
                commCb->onResponse(error);
        };
    if (Status::SUCCESS ==
        cv2xRadio->createCv2xTcpSocket(eventInfo, tcpInfo, sockRespCb)) {
        auto error = commCb->getResponse();
        if (ErrorCode::SUCCESS == error) {
            if(rVerbosity)
                cout<<"createCv2xTcpSocket succeeds." << endl;;
            ret = 0;
        } else {
            if (rVerbosity)
                cerr<<"createCv2xTcpSocket fails: ." <<
                    static_cast<int>(error) << endl;;
            ret = -1;
        }
    } else {
        if (rVerbosity)
            cerr << "createCv2xTcpSocket sync fails" << endl;
        ret = -1;
    }

    if (rVerbosity)
        cout << "Global IP Info Set" << endl;

    return ret;
}

//For RSU use case, clear Global IP info and unregister catch all flow
int RadioInterface::clearGlobalIPInfo(void) {
    if (not this->tcpSockInfo) {
        return 0;
    }

    int ret = 0;
    telux::cv2x::IPv6AddrType ipv6Prefix;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return -1;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto closeSockCb = [&](shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
        cb->onResponse(error);
    };
    if (Status::SUCCESS !=
        cv2xRadio->closeCv2xTcpSocket(tcpSockInfo, closeSockCb) ||
        ErrorCode::SUCCESS != cb->getResponse()) {
        if(rVerbosity)
            cerr << "close tcp socket err or already closed" << endl;
        ret = -1;
    } else {
        this->tcpSockInfo = nullptr;
    }

    if (ret)
        return ret;

    ipv6Prefix.prefixLen = 64;
    memset(&ipv6Prefix.ipv6Addr[0], 0, CV2X_IPV6_ADDR_ARRAY_LEN);
    auto commCb = std::make_shared<CommonCallback>();
    auto respCb = [&](ErrorCode error) {
        commCb->onResponse(error);
    };
    if (Status::SUCCESS == cv2xRadio->setGlobalIPInfo(ipv6Prefix, respCb)) {
        if (ErrorCode::SUCCESS == commCb->getResponse()) {
            if(rVerbosity)
                cout<<"setGlobalIPInfo succeeds." << endl;;
            ret = 0;
        } else {
            if(rVerbosity)
                cerr<<"setGlobalIPInfo fails." << endl;;
            ret = -1;
        }
    } else {
        if (rVerbosity)
            cout<<"setGlobalIPInfo sync fails." << endl;
        ret = -1;
    }

    if(rVerbosity)
        cout << "Global IP session stopped" << endl;

    return ret;
}

int RadioInterface::setRoutingInfo(const telux::cv2x::GlobalIPUnicastRoutingInfo &destL2Addr) {
    int ret = 0;
    auto cv2xRadio = this->getCv2xRadio();
    if (nullptr == cv2xRadio) {
        return -1;
    }
    auto cb = std::make_shared<CommonCallback>();
    auto respCb = [&](ErrorCode error){
                cb->onResponse(error);
            };

    if (Status::SUCCESS == cv2xRadio->setGlobalIPUnicastRoutingInfo(destL2Addr, respCb)) {
        if (ErrorCode::SUCCESS == cb->getResponse())
        {
            ret = 0;
            if(rVerbosity)
                cout<<"setGlobalIPUnicastRoutingInfo succeeds." << endl;;
        } else {
            if(rVerbosity)
                cerr<<"setGlobalIPUnicastRoutingInfo fails." << endl;;
            ret = -1;
        }
    } else {
        if(rVerbosity)
            cerr<< "setGlobalIPUnicastRoutingInfo sync fails." << endl;
        ret = -1;
    }

    return ret;
}

int RadioInterface::onWraTimedout(void) {
    return clearGlobalIPInfo();
}

shared_ptr<ICv2xRadioManager> RadioInterface::getCv2xRadioManager() {
    if (nullptr == cv2xRadioManager_ or not cv2xRadioManager_->isReady()) {
        cout << "cv2x radio manager is not ready." << endl;
        return nullptr;
    }
    return cv2xRadioManager_;
}

shared_ptr<ICv2xRadio> RadioInterface::getCv2xRadio() {
    if (nullptr == cv2xRadio_ or not cv2xRadio_->isReady()) {
        cout << "cv2x radio is not ready." << endl;
        return nullptr;
    }
    return cv2xRadio_;
}

void RadioInterface::prepareForExit() {
    // deregister listeners
    if (cv2xRadioManager_ and cv2xStatusListener_) {
        cv2xRadioManager_->deregisterListener(cv2xStatusListener_);
        // stop waiting for status change
        cv2xStatusListener_->deinit();
    }

    if (cv2xRadio_ and radioListener_) {
        cv2xRadio_->deregisterListener(radioListener_);
    }
}
