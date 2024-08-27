/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_CONFIG_IDL_HPP__
#define __SPLITS_CONFIG_IDL_HPP__
#include "v1/commonapi/SPLITS/SplitsConfigProxy.hpp"
using v1::commonapi::SPLITS::SplitsConfig;
using v1::commonapi::SPLITS::SplitsConfigProxy;
namespace capisplits = v1::commonapi::SPLITS;

namespace SPLITS {
namespace Config {

class SplitsConfigIdl {
public:
    SplitsConfigIdl(std::shared_ptr<SplitsConfig::SplitsConfigData> config) :
        config_(config){
    }
    virtual ~SplitsConfigIdl(){}

    int init(void);
    int loadConfigFromIdl(void);

private:
    std::shared_ptr<SplitsConfigProxy<>> proxy_ = nullptr;
    std::shared_ptr<SplitsConfig::SplitsConfigData> config_;
};
} //namespace Config
} //namespace SPLITS

#endif //__SPLITS_CONFIG_IDL_HPP__

