/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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
    SplitsConfigFile(std::shared_ptr<SplitsConfig::SplitsConfigData> config, bool csae) : 
        config_(config){
        if (csae) {
            msgName2PsidMap_["BSM"] = CSAE_AID_BSM;
            msgName2PsidMap_["BSM_EMG"] = CSAE_AID_BSM_EMG;
            msgName2PsidMap_["BSM_EMG_EVENT"] = CSAE_AID_BSM_EMG_EVENT;
            msgName2PsidMap_["BSM_AM"] = CSAE_AID_BSM_AM_OBU;
            msgName2PsidMap_["MAP"] = CSAE_AID_MAP;
            msgName2PsidMap_["SPAT"] = CSAE_AID_SPAT;
            msgName2PsidMap_["RSM"] = CSAE_AID_RSM;
            msgName2PsidMap_["RSI"] = CSAE_AID_RSI_STATIC;
            msgName2PsidMap_["RSI_SEMI_STATIC"] = CSAE_AID_RSI_SEMI_STATIC;
            msgName2PsidMap_["RSI_DYNAMIC"] = CSAE_AID_RSI_DYNAMIC;
        } else {
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
