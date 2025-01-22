/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_CONFIG_STUB_IMPL_HPP__
#define __SPLITS_CONFIG_STUB_IMPL_HPP__

#include <memory>
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"
#include "v1/commonapi/SPLITS/SplitsConfigStubDefault.hpp"
#include "v1/commonapi/telux/common/CommonDefines.hpp"
using v1::commonapi::SPLITS::SplitsConfig;
using v1::commonapi::SPLITS::SplitsConfigStubDefault;
using v1::commonapi::telux::common::CommonDefines;

namespace SPLITS {

namespace Config {

class SplitsConfigStubImpl : public SplitsConfigStubDefault {
public:
    SplitsConfigStubImpl(std::shared_ptr<SplitsConfig::SplitsConfigData> config): config_(config){
    }
    virtual ~SplitsConfigStubImpl(){}
    void readConfig(const std::shared_ptr<CommonAPI::ClientId> _client, readConfigReply_t _reply) {
        CommonDefines::ErrorCode err = CommonDefines::ErrorCode::SUCCESS;
        if (config_ == nullptr) {
            err =  CommonDefines::ErrorCode::INTERNAL_ERR;
            SplitsConfig::SplitsConfigData config = {};
            _reply(err, config);
        } else {
            _reply(err, *config_);
        }
    }
private:
    std::shared_ptr<SplitsConfig::SplitsConfigData> config_;
};
} //namespace Config
} //namespace SPLITS

#endif
