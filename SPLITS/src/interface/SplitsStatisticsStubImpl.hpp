/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SPLITS_STATISTICS_SUBIMPL_HPP_
#define SPLITS_STATISTICS_SUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/SPLITS/SplitsStatisticsStubDefault.hpp>

namespace SPLITS {
    namespace Core {
        class SplitsCoreImpl;
    }
}

using v1::commonapi::SPLITS::SplitsStatistics;

namespace SPLITS {
namespace Interface {

class SplitsStatisticsStubImpl : public v1::commonapi::SPLITS::SplitsStatisticsStubDefault {
public:
    SplitsStatisticsStubImpl(std::shared_ptr<SPLITS::Core::SplitsCoreImpl> SplitsCore);

    virtual ~SplitsStatisticsStubImpl() {
    }

    void getRxStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                         SplitsStatistics::Port _portNumber,
                         getRxStatisticsReply_t _reply) override;
    void getRxStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                         getRxStatistics1Reply_t _reply) override;
    void getTxStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                         SplitsStatistics::PSID _psId,
                         getTxStatisticsReply_t _reply) override;
    void getTxStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                         getTxStatistics1Reply_t _reply) override;

    void reportAbnormalies(
        const ::v1::commonapi::SPLITS::SplitsStatistics::AbnormalIncident& _value);


    void getSPLITSStatus(const std::shared_ptr<CommonAPI::ClientId> _client,
                         getSPLITSStatusReply_t _reply) override;
    void getSecurityReport(const std::shared_ptr<CommonAPI::ClientId> _client,
                           getSecurityReportReply_t _reply) override;
    void getCongestionControlStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                                        getCongestionControlStatisticsReply_t _reply) override;
    void getMsgStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                          getMsgStatisticsReply_t _reply) override;
    void getMsgStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                          SplitsStatistics::MessageID _msgid,
                          getMsgStatistics1Reply_t _reply) override;
    void getPSIDStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                           getPSIDStatisticsReply_t _reply) override;
    void getPSIDStatistics(const std::shared_ptr<CommonAPI::ClientId> _client,
                           SplitsStatistics::PSID _psid,
                           getPSIDStatistics1Reply_t _reply) override;

private:
    std::shared_ptr<SPLITS::Core::SplitsCoreImpl> splitsCore_;

};


}
}
#endif
