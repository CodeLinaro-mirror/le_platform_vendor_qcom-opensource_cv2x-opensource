/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SPLITS_CONFIG_FILE_HPP__
#define __SPLITS_CONFIG_FILE_HPP__
#include "SplitsCodec.hpp"
#include "v1/commonapi/SPLITS/SplitsConfig.hpp"

using v1::commonapi::SPLITS::SplitsConfig;

namespace SPLITS {

namespace Config {

class SplitsConfigFile {
public:
    SplitsConfigFile(std::shared_ptr<SplitsConfig::SplitsConfigData> config) : 
        config_(config){
        msgName2PsidMap_["BSM"] = SAE_PSID_BSM;
        msgName2PsidMap_["WSA"] = SAE_PSID_WSA;
        msgName2PsidMap_["SPAT"] = SAE_PSID_SPAT;
        msgName2PsidMap_["MAP"] = SAE_PSID_MAP;
        msgName2PsidMap_["RTCM"] = SAE_PSID_RTCM;
        msgName2PsidMap_["RSM"] = SAE_PSID_RSM;
        msgName2PsidMap_["RSM"] = SAE_PSID_RSM;
        msgName2PsidMap_["SRM"] = SAE_PSID_SRM;
        msgName2PsidMap_["SSM"] = SAE_PSID_SSM;
        msgName2PsidMap_["SDSM"] = SAE_PSID_SDSM;
    }

    virtual ~SplitsConfigFile(){}

    int loadConfigFromFile(std::string filename);

    uint32_t getPsidFromName(std::string name) {
        if (msgName2PsidMap_.find(name) == msgName2PsidMap_.end()) {
            return 0;
         } else {
            return msgName2PsidMap_[name];
         }
    }

private:
    int HexStringValue(std::string &hexStr, std::vector<uint8_t> &value, bool toValue);
    int IPv6AddrStringValue(std::string &ipv6Str, std::vector<uint8_t> & value, bool toValue);
    std::shared_ptr<SplitsConfig::SplitsConfigData> config_;
    std::map<std::string, int> msgName2PsidMap_;
};
} //namespace Config
} //namespace SPLITS

#endif //__SPLITS_CONFIG_FILE_HPP__
