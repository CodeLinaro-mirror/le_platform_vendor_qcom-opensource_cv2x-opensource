/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <ctime>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/param.h>
#include <unistd.h>
#include <string.h>

#include "Logger.hpp"

#include "SplitsCoreImpl.hpp"
#include "SplitsStatisticsStubImpl.hpp"
namespace SPLITS {
namespace Interface {

SplitsStatisticsStubImpl::SplitsStatisticsStubImpl(
    std::shared_ptr<SPLITS::Core::SplitsCoreImpl> core) {
    splitsCore_ = core;
}

void SplitsStatisticsStubImpl::getTxStatistics(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    getTxStatistics1Reply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        _reply(splitsCore_->getTxStatistics());
    }
}

void SplitsStatisticsStubImpl::getTxStatistics(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    SplitsStatistics::PSID _serviceId,
    getTxStatisticsReply_t _reply) {
    LOGD( __FUNCTION__);
    (void)_client;
    auto allTxStatistics = splitsCore_->getTxStatistics();
    auto txStatistics = allTxStatistics.find(_serviceId);
    if (txStatistics != allTxStatistics.end()) {
        _reply(txStatistics->second);
    } else {
        _reply(SplitsStatistics::TxStatistics());
    }
}

void SplitsStatisticsStubImpl::getRxStatistics(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    getRxStatistics1Reply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        _reply(splitsCore_->getRxStatistics());
    }
}

void SplitsStatisticsStubImpl::getRxStatistics(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    SplitsStatistics::Port _portNumber,
    getRxStatisticsReply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;

    auto allRxStatistics = splitsCore_->getRxStatistics();
    auto rxStatistics = allRxStatistics.find(_portNumber);
    if (rxStatistics != allRxStatistics.end()) {
        _reply(rxStatistics->second);
    } else {
        _reply(SplitsStatistics::RxStatistics());
    }
}

void SplitsStatisticsStubImpl::reportAbnormalies(
    const ::v1::commonapi::SPLITS::SplitsStatistics::AbnormalIncident& _value) {
    LOGD(__FUNCTION__);
    // TODO: need to set the value when construct statistics
    setIncidentAttribute(_value);
}

static bool processUpTime(time_t& osUptime, time_t& splitsUptime) {
    char buff[128] = {0};
    char *p = nullptr;
    struct timeval tv;
    time_t boottime = -1;
    time_t now = -1;
    int fd = ::open("/proc/uptime", O_RDONLY);

    if (fd != -1) {
        if (::read(fd, buff, sizeof buff) > 0) {
            osUptime = static_cast<time_t>(strtoul(buff, &p, 10));
            gettimeofday(&tv, 0);
            now = tv.tv_sec;
            boottime = tv.tv_sec - osUptime;
        }
        ::close(fd);

        snprintf(buff, sizeof buff, "/proc/self/stat");
        FILE *fp = ::fopen(buff, "r"); 
        if (fp != nullptr) {
            memset(buff, 0, sizeof buff);
            fgets(buff, sizeof buff, fp);

            char *rest = nullptr, *token;
            int i = 0;
            // Read the 22nd column
            for (token = strtok_r(buff, " ", &rest); token != nullptr;
                 token = strtok_r(nullptr, " ", &rest)) {
                if (i == 21) {
                    splitsUptime = (now - boottime) - (atof(token)) / HZ;
                    break;
                }
                ++i;
            }
        } else {
            ::fclose(fp);
            return false;
        }
    } else {
        ::close(fd);
        return false;
    }
    return true;
}

void SplitsStatisticsStubImpl::getSPLITSStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
                                               getSPLITSStatusReply_t _reply) {
    LOGD(__FUNCTION__);

    constexpr const char* date_format = "%Y-%m-%d %H:%M:%S";
    constexpr unsigned max_date_string_size = 20;

    (void)_client;
    time_t osUptime = {0};
    time_t splitsUptime = {0};
    std::time_t now = std::time(nullptr);
    SplitsStatistics::SPLITSStatus status;
  
    processUpTime(osUptime, splitsUptime);

    status.setOSuptime(static_cast<SplitsStatistics::TimeStamp>(osUptime));
    status.setUptime(static_cast<SplitsStatistics::TimeStamp>(splitsUptime));

    osUptime = now - osUptime;
    splitsUptime = now - splitsUptime;
    std::tm *osUptm = std::localtime(&osUptime);

    char buffer[max_date_string_size] = {0};
    std::strftime(buffer, sizeof(buffer), date_format, osUptm);
    status.setCalendarOSUptime(buffer);

    memset(buffer, 0, max_date_string_size);

    std::tm *splitsUptm = std::localtime(&splitsUptime);
    std::strftime(buffer, sizeof(buffer), date_format, splitsUptm);
    status.setCalendarUptime(buffer);

    _reply(status);
}

void SplitsStatisticsStubImpl::getSecurityReport(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                 getSecurityReportReply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        SplitsStatistics::SecurityStatistics statistics =
            splitsCore_->getSecurityStatistics();
        _reply(statistics);
    }
}

void SplitsStatisticsStubImpl::getCongestionControlStatistics(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    getCongestionControlStatisticsReply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        SplitsStatistics::CongestionControlStatistics statistics =
            splitsCore_->getCongestionControlStatistics();
        _reply(statistics);
    }
}

void SplitsStatisticsStubImpl::getMsgStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                getMsgStatisticsReply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        auto statistics = splitsCore_->getAllMessageStatistics();
        _reply(statistics);
    }
}

void SplitsStatisticsStubImpl::getMsgStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                SplitsStatistics::MessageID _msgid,
                                                getMsgStatistics1Reply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    if (splitsCore_) {
        auto allMsgStatistics = splitsCore_->getAllMessageStatistics();
        auto statistics = allMsgStatistics[_msgid];
        _reply(statistics);
    }
}

void SplitsStatisticsStubImpl::getPSIDStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                 getPSIDStatisticsReply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;
    SplitsStatistics::AllPSIDStatistics statistics;

    if (splitsCore_) {
        statistics = splitsCore_->getStatisticsofAllPSID();
    }
    _reply(statistics);
}

void SplitsStatisticsStubImpl::getPSIDStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                                                 SplitsStatistics::PSID _psid,
                                                 getPSIDStatistics1Reply_t _reply) {
    LOGD(__FUNCTION__);
    (void)_client;

    SplitsStatistics::PSIDStatistics statistics;
    if (splitsCore_) {
        auto allStatistics = splitsCore_->getStatisticsofAllPSID();
        auto st = allStatistics.find(_psid);
        if (st != allStatistics.end()) {
            statistics = st->second;
        }
    }
    _reply(statistics);
}

}
}
