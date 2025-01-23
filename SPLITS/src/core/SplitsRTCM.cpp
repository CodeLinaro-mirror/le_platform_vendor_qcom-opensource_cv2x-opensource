/*
 *Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <telux/loc/LocationFactory.hpp>
#include "SplitsRTCM.hpp"
#include "J2735_RTCMcorrections.h"
#include "utils.h"
#include "Logger.hpp"

using telux::loc::LocationFactory;
using telux::loc::DgnssDataFormat;

#define RTCM_MSGPOOL_SIZE   5
namespace SPLITS {

namespace Core {

SplitsRTCM::SplitsRTCM(SPLITS::Core::Role role, std::shared_ptr<SplitsConfig::RTCMConfig> config)
    : SplitsNtrip(config), role_(role) {
    //role_ = role;
    //config_ = config;
    if (role_ == Role::RSU) {
        for (int i = 0; i < RTCM_MSGPOOL_SIZE; i++) {
            rtcmMsgPool_.push(std::make_shared<J2735::idl::RTCMmessage>());
        }
    }
}

int SplitsRTCM::init(void) {
    if (role_ == Role::OBU) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        dgnssMgr_ = locationFactory.getDgnssManager(DgnssDataFormat::DATA_FORMAT_RTCM_3,
                [&] (ServiceStatus status) {
                    if (status == ServiceStatus::SERVICE_AVAILABLE) {
                        prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
                    } else {
                        prom.set_value(ServiceStatus::SERVICE_FAILED);
                    }
                });
        if (dgnssMgr_ == nullptr) {
            LOGE(__FUNCTION__, " Failed to get dgnss manager object");
            return -1;
        }
        ServiceStatus dgnssMgrStatus = dgnssMgr_->getServiceStatus();
        if (dgnssMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LOGD(__FUNCTION__, " waiting for dgnss subsystem ready...");
        }
        dgnssMgrStatus = prom.get_future().get();
        if(dgnssMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            LOGD(__FUNCTION__, " Dgnss subsystem is ready ");
        } else {
            LOGE(__FUNCTION__, " Unable to initialize Dgnss subsystem");
            return -1;
        }
    } else {
        return SplitsNtrip::init();
    }
    return 0;
}

void SplitsRTCM::deinit(void) {
    if (role_ == Role::RSU) {
        SplitsNtrip::deinit();
    }
}

void SplitsRTCM::onDgnssStatusUpdate(DgnssStatus status) {
   switch(status) {
       case DgnssStatus::DATA_SOURCE_NOT_SUPPORTED:
         LOGD(__FUNCTION__, " RTCM data soure is not supported");
         break;
       case DgnssStatus::DATA_FORMAT_NOT_SUPPORTED:
         LOGD(__FUNCTION__, " RTCM data format is not supported");
         break;
       case DgnssStatus::OTHER_SOURCE_IN_USE:
         LOGD(__FUNCTION__, " RTCM other source is in use");
         break;
       case DgnssStatus::MESSAGE_PARSE_ERROR:
         LOGD(__FUNCTION__, " RTCM message parsing error");
         break;
       case DgnssStatus::DATA_SOURCE_NOT_USABLE:
         LOGE(__FUNCTION__, " RTCM data source is not usable");
         // Demonstrate "source switching" requirement for v2x use case. If current source's data
         // is not usable anymore, another source is picked, but we must call releaseSource()
         // to release current source and createSource() to create a new one.
         //dgnssManager_->releaseSource();
         //if (dgnssManager_->createSource(DgnssDataFormat::DATA_FORMAT_RTCM_3) !=
         //       telux::common::Status::SUCCESS) {
         //   LOGD(__FUNCTION__, " Failed to create RTCM source");
         //}
         break;
      default:
         LOGE(__FUNCTION__, " Unknown RTCM status");
   }
}

Status SplitsRTCM::onReceiveRTCM(J2735::idl::RTCMcorrection &idlRTCM) {
    Status status;
    if (idlRTCM.rev() != J2735::idl::rtcmv_rtcmRev3) {
        LOGE(__FUNCTION__, " Unsupported RTCM version: ", idlRTCM.rev());
        return Status::NOTSUPPORTED;
    }
    auto msgList = idlRTCM.msgs();
    LOGE(__FUNCTION__, " msgList size: ", msgList.size());
    for (const auto &rtcmMsg : msgList) {
        if (dgnssMgr_ != nullptr) {
            LOGD(__FUNCTION__, "injecting ", rtcmMsg.size(), "bytes data");
            if ((status = dgnssMgr_->injectCorrectionData(rtcmMsg.data(), rtcmMsg.size()))
                != Status::SUCCESS) {
                return status;
            }
        }
    }
    return Status::SUCCESS;
}

Status SplitsRTCM::fillRTCM(std::shared_ptr<v2xMsgSae> msg) {
    std::shared_ptr<J2735::idl::RTCMcorrection> sp;
    if (msg == nullptr) {
        return Status::INVALIDPARAM;
    }
    if ((sp = msg->RTCMcorrection()) == nullptr) {
        sp = std::make_shared<J2735::idl::RTCMcorrection>();
        if (sp == nullptr) {
            return Status::NOMEMORY;
        }
        msg->RTCMcorrection(sp);
    }
    sp->rev(J2735::idl::rtcmv_rtcmRev3);
    sp->timeStamp(get_minute_of_the_year());
    {
        std::lock_guard<std::mutex> lk(msgMtx_);
        sp->msgCnt(rtcmMsgQueue_.size());
        J2735::idl::RTCMmessageList idlMsgList;
        while (rtcmMsgQueue_.empty() == false) {
            auto msg = rtcmMsgQueue_.front();
            rtcmMsgQueue_.pop();
            idlMsgList.push_back(*msg);
            LOGD(__FUNCTION__, " msg size:", msg->size());
            rtcmMsgPool_.push(msg);
        }
        sp->msgs(std::move(idlMsgList));
    }
    return Status::SUCCESS;
}
void SplitsRTCM::onMsgAvailable(uint8_t *buf, int bufLen)
{
    std::lock_guard<std::mutex> lk(msgMtx_);
    if (rtcmMsgQueue_.size() == RTCM_MSGPOOL_SIZE) {
        //queue is full, msgs are not broadcasted out, we drop the newly received msgs
        return;
    }
    auto msg = rtcmMsgPool_.front();
    rtcmMsgPool_.pop();
    msg->clear();
    LOGD(__FUNCTION__, " copy data to queue: ", bufLen);
    msg->assign(buf, buf + bufLen);
    rtcmMsgQueue_.push(msg);
}
void SplitsRTCM::onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {
    if (nmea.find("GNGGA", 0) != std::string::npos) {
        std::lock_guard<std::mutex> lk(mtx_);
        lastNmeaGGA_.assign(nmea);
    }
}

int SplitsRTCM::sendGGAString(void) {
    int ret;
    std::lock_guard<std::mutex> lk(mtx_);
    if (not lastNmeaGGA_.empty()) {
        //LOGD(__FUNCTION__, " Send NMEA: " << nmeaGGA);
        ret = send(ntcSocketFd_, lastNmeaGGA_.c_str(), lastNmeaGGA_.size(), 0);
        if (ret < 0) {
            LOGE(__FUNCTION__, " Failed to send GGA string to server");
        } else {
           lastNmeaSentTime_ = std::chrono::system_clock::now();
        }
    } else {
        ret = -1;
        LOGE(__FUNCTION__, " No NMEA GGA string to send");
    }

    return ret;
}
} //namespace Core
} //namespace SPLITS
