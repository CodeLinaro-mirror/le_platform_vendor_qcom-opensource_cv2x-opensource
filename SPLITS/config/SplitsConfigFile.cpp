/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "json/json.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "SplitsConfigFile.hpp"

namespace SPLITS {

namespace Config {

int SplitsConfigFile::loadConfigFromFile(std::string filename)
{
    unsigned int i;
    Json::Value root;
    std::ifstream ifs;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;

    ifs.open(filename);
    builder["collectComments"] = true;

    if (!parseFromStream(builder, ifs, &root, &errs)) {
        std::cout << errs << std::endl;
        return -1;
    }
    std::cout << root << std::endl;
    Json::Value jsonCoreCfg = root["CoreConfig"];
    SplitsConfig::CoreConfig coreCfg;

    coreCfg.setProcPriority(jsonCoreCfg.get("procPriority", -20).asInt());
    coreCfg.setWildCardRx(jsonCoreCfg.get("WildCardRx", false).asBool());
    if (jsonCoreCfg.get("Role", "OBU").compare("OBU") == 0) {
        coreCfg.setRole(0);
    } else {
        coreCfg.setRole(1);
    }
    coreCfg.setEnableLocationFixes(jsonCoreCfg.get("EnableLocationFixes", true).asBool());
    coreCfg.setVehicleDataSource(jsonCoreCfg.get("VehicleDataSource", 0).asInt());
    coreCfg.setLocationInterval(jsonCoreCfg.get("LocationInterval", 100).asInt());

    //Tx Sps flows
    Json::Value jsonTxFlows = jsonCoreCfg["SpsFlows"];
    std::vector<SplitsConfig::TxFlow> TxFlows;
    for (i = 0; i < jsonTxFlows.size(); i++) {
        SplitsConfig::TxFlow spsFlow;
        auto msgType = jsonTxFlows[i].get("MessageType", "unknown").asString();
        if (msgName2PsidMap_.find(msgType) == msgName2PsidMap_.end()) {
            std::cerr << "skipping msgType: " << msgType << std::endl;
            continue;
        }
        spsFlow.setSource(jsonTxFlows[i].get("Source", 0).asUInt());
        spsFlow.setFormat(jsonTxFlows[i].get("Format", 0).asUInt());
        spsFlow.setServiceId(msgName2PsidMap_[msgType]);
        spsFlow.setPeriodicity(jsonTxFlows[i].get("Periodicity", 100).asUInt());
        spsFlow.setSourcePort(jsonTxFlows[i].get("SourcePort", 9000).asUInt());
        spsFlow.setDestPort(jsonTxFlows[i].get("DestPort", 2500).asUInt());
        spsFlow.setDestAddr(jsonTxFlows[i].get("DestAddr", "").asString());
        TxFlows.push_back(spsFlow);
    }
    coreCfg.setSpsFlows(TxFlows);

    //Tx event flows
    jsonTxFlows = jsonCoreCfg["EventFlows"];
    TxFlows.clear();
    for (i = 0; i < jsonTxFlows.size(); i++) {
        SplitsConfig::TxFlow eventFlow;
        auto msgType = jsonTxFlows[i].get("MessageType", "unknown").asString();
        if (msgName2PsidMap_.find(msgType) == msgName2PsidMap_.end()) {
            std::cerr << "skipping msgType: " << msgType << std::endl;
            continue;
        }
        eventFlow.setSource(jsonTxFlows[i].get("Source", 0).asUInt());
        eventFlow.setFormat(jsonTxFlows[i].get("Format", 0).asUInt());
        eventFlow.setServiceId(msgName2PsidMap_[msgType]);
        eventFlow.setPeriodicity(jsonTxFlows[i].get("Periodicity", 100).asUInt());
        eventFlow.setSourcePort(jsonTxFlows[i].get("SourcePort", 9000).asUInt());
        eventFlow.setDestPort(jsonTxFlows[i].get("DestPort", 2500).asUInt());
        eventFlow.setDestAddr(jsonTxFlows[i].get("DestAddr", "").asString());
        TxFlows.push_back(eventFlow);
    }
    coreCfg.setEventFlows(TxFlows);

    //Rx flows
    Json::Value jsonRxFlows = jsonCoreCfg["ReceiveFlows"];
    std::vector<SplitsConfig::RxFlow> RxFlows;
    for (i = 0; i < jsonRxFlows.size(); i++) {
        SplitsConfig::RxFlow rxFlow;
        auto msgType = jsonRxFlows[i].get("MessageType", "unknown").asString();
        if (msgName2PsidMap_.find(msgType) == msgName2PsidMap_.end()) {
            std::cerr << "skipping msgType:" << msgType << std::endl;
            return -1;
        }
        rxFlow.setServiceId(msgName2PsidMap_[msgType]);
        rxFlow.setReceivePort(jsonRxFlows[i].get("ReceivePort", 9000).asUInt());
        RxFlows.push_back(rxFlow);
    }
    coreCfg.setRxFlows(RxFlows);
    //config_->setCoreConfig(coreCfg);

    //Security
    Json::Value jsonSecCfg = root["SecurityConfig"];
    SplitsConfig::SecurityConfig secCfg;
    coreCfg.setEnableSecurity(jsonSecCfg.get("Enable", false).asBool());
    if (coreCfg.getEnableSecurity() == true) {
        secCfg.setSecurityCtxName(jsonSecCfg.get("SecurityContextName", "Unspecified").asString());
        secCfg.setCountryCode(jsonSecCfg.get("SecurityCountryCode", 0).asUInt());
        secCfg.setLocalCertMgrName(jsonSecCfg.get("LocalCertMgrName", "Unspecified").asString());
        secCfg.setEnableConsistency(jsonSecCfg.get("EnableConsistency", false).asBool());
        secCfg.setEnableRelevance(jsonSecCfg.get("EnableRelevance", false).asBool());
        secCfg.setSetGenLocation(jsonSecCfg.get("SetGenLocation", false).asBool());
        secCfg.setEnableSsp(jsonSecCfg.get("EnableSsp", false).asBool());
        auto sspValueStr = jsonSecCfg.get("SspValue", "00").asString();
        std::vector<uint8_t> sspValue;
        if (HexStringValue(sspValueStr, sspValue, true)) {
            return -1;
        }
        secCfg.setSspValue(sspValue);
        secCfg.setEnableSspMask(jsonSecCfg.get("EnableSspMask", false).asBool());
        if (secCfg.getEnableSspMask() == true) {
            auto sspMaskStr = jsonSecCfg.get("SspMask", "00").asString();
            std::vector<uint8_t> sspMaskValue;
            if (HexStringValue(sspMaskStr, sspMaskValue, true)) {
                return -1;
            }
            secCfg.setSspMask(sspMaskValue);
        }
        secCfg.setEnableL2FloodDetect(jsonSecCfg.get("EnableL2FloodDetect", false).asBool());
        secCfg.setMvmUtilThresh(jsonSecCfg.get("mvmUtilThresh", 0.5).asFloat());
    }
    config_->setSecConfig(secCfg);

    //CongCtrl
    Json::Value jsonCongCtrlCfg = root["CongCtrlConfig"];
    SplitsConfig::CongCtrlConfig congCtrlCfg;
    coreCfg.setEnableCongCtrl(jsonCongCtrlCfg.get("Enable", false).asBool());
    if (coreCfg.getEnableCongCtrl() == true) {
        congCtrlCfg.setVMinItt(jsonCongCtrlCfg.get("VMinItt", 0).asUInt());
        congCtrlCfg.setVMaxItt(jsonCongCtrlCfg.get("VMaxItt", 0).asUInt());
        congCtrlCfg.setVPerInterval(jsonCongCtrlCfg.get("VPerInterval",0).asUInt());
        congCtrlCfg.setVPerSubInterval(jsonCongCtrlCfg.get("VPerSubInterval", 0).asUInt());
        congCtrlCfg.setVDensityWeightFactor(jsonCongCtrlCfg.get("VDensityWeightFactor", 0).asFloat());
        congCtrlCfg.setVDensityCoefficient(jsonCongCtrlCfg.get("VDensityCoefficient", 0).asFloat());
        congCtrlCfg.setVDensityMinPerRange(jsonCongCtrlCfg.get("VDensityMinPerRange", 0).asFloat());
        congCtrlCfg.setEnableSpsEnhancements(jsonCongCtrlCfg.get("EnableSpsEnhancements", false).asBool());
        congCtrlCfg.setTxCtrlInterval(jsonCongCtrlCfg.get("TxCtrlInterval", 0).asUInt());
        config_->setCongCtrlConfig(congCtrlCfg);
    }
    config_->setCongCtrlConfig(congCtrlCfg);

    //WraConfig
    Json::Value jsonWraCfg = root["WraConfig"];
    SplitsConfig::WraConfig wraCfg;
    coreCfg.setEnableWra(jsonWraCfg.get("Enable", false).asBool());
    if (coreCfg.getEnableWra() == true) {
        wraCfg.setServiceId(jsonWraCfg.get("ServiceId", 4).asUInt());
        wraCfg.setWraInterval(jsonWraCfg.get("Interval", 1000).asUInt());
        wraCfg.setRouterLifetime(jsonWraCfg.get("routerLifetime", 30).asUInt());
        auto ipPrefixStr = jsonWraCfg.get("ipPrefix", "").asString();
        if (ipPrefixStr.empty() and coreCfg.getRole() == 1) {
            std::cout << "config error: WRA Role is RSU, but ipPrefix is not set" << std::endl;
            return -1;
        }
        std::vector<uint8_t> ipPrefixValue;
        if (IPv6AddrStringValue(ipPrefixStr, ipPrefixValue, true)) {
            return -1;
        }
        wraCfg.setIpPrefix(ipPrefixValue);
        wraCfg.setIpPrefixLength(jsonWraCfg.get("ipPrefixLength", wraCfg.getIpPrefix().size()*8).asUInt());
        auto defaultGwStr = jsonWraCfg.get("defaultGateway", "").asString();
        std::vector<uint8_t> defaultGwValue;
        if (false == defaultGwStr.empty()) {
            if (IPv6AddrStringValue(defaultGwStr, defaultGwValue, true)) {
                return -1;
            }
        }
        wraCfg.setDefaultGateway(defaultGwValue);
        auto primaryDnsStr = jsonWraCfg.get("primaryDns", "").asString();
        std::vector<uint8_t> primaryDnsValue;
        if (false == primaryDnsStr.empty()) {
            if (IPv6AddrStringValue(primaryDnsStr, primaryDnsValue, true)) {
                return -1;
            }
        }
        wraCfg.setPrimaryDns(primaryDnsValue);
    }
    config_->setWraConfig(wraCfg);
    //RtcmConfig
    Json::Value jsonRtcmCfg = root["RtcmConfig"];
    SplitsConfig::RTCMConfig rtcmCfg;
    coreCfg.setEnableRTCM(jsonRtcmCfg.get("Enable", false).asBool());
    if (coreCfg.getEnableRTCM() == true and coreCfg.getRole() == 1) {
        rtcmCfg.setNmeaGGAInterval(jsonRtcmCfg.get("NmeaGGAInterval", 30000).asUInt());
        auto hostNameStr = jsonRtcmCfg.get("HostName","").asString();
        auto loginStr = jsonRtcmCfg.get("UserNamePwdInBase64Format", "").asString();
        auto mountPtStr = jsonRtcmCfg.get("MountPoint","").asString();
        if (hostNameStr.empty() == true ) {
            std::cout << "config error: RTCM role is RSU, but no Ntrip hostname specified" << std::endl;
            return -1;
        }
        if (loginStr.empty() == true) {
            std::cout << "config error: RTCM role is RSU, but no login info for ntrip server specified" << std::endl;
            return -1;
        }
        if (mountPtStr.empty() == true) {
            std::cout << "config error: RTCM role is RSU, but no mount point specified for ntrip" << std::endl;
            return -1;
        }
        rtcmCfg.setHostName(hostNameStr);
        rtcmCfg.setUserNamePwdInBase64Format(loginStr);
        rtcmCfg.setMountPoint(mountPtStr);
        rtcmCfg.setPort(jsonRtcmCfg.get("Port", 0).asUInt());
        config_->setRtcmConfig(rtcmCfg);
    }
    config_->setRtcmConfig(rtcmCfg);
    config_->setCoreConfig(coreCfg);
    return 0;
}

int SplitsConfigFile::HexStringValue(std::string & hexStr, std::vector<uint8_t> & value,
    bool toValue)
{
    if (toValue) {
        size_t pos = 0, prev = 0;
        hexStr += ":";
        do {
            pos = hexStr.find(":", prev);
            if (pos == std::string::npos or pos == prev) {
                break;
            }
            std::string subStr = hexStr.substr(prev, pos - prev);
            if (subStr.size() > 2) {
                std::cerr << subStr << " is longer than one byte" << std::endl;
                return -1;
            }
            uint8_t v = std::stoi(subStr, 0, 16);
            value.push_back(v);
            prev = pos + 1;
        } while(prev < hexStr.size());
    } else {
        std::stringstream ss;
        ss << std::hex;
        for (size_t i = 0; i < value.size(); i++) {
            ss << std::setw(2) << std::setfill('0') << (unsigned int)value[i];
            if (i < (value.size() - 1)) {
                ss << ":";
            }
        }
        hexStr = ss.str();
    }
    return 0;
}
int SplitsConfigFile::IPv6AddrStringValue(std::string &ipv6Str, std::vector<uint8_t> & value,
        bool toValue)
{
    if (toValue) {
        size_t pos = 0, prev = 0;
        ipv6Str += ":";
        do {
            pos = ipv6Str.find(":", prev);
            if (pos == std::string::npos or pos == prev) {
                break;
            }
            std::string subStr = ipv6Str.substr(prev, pos - prev);
            if (subStr.size() > 4) {
                std::cerr << subStr << " is longer than two bytes" << std::endl;
                return -1;
            }
            uint16_t v = std::stoi(subStr, 0, 16);
            value.push_back(v >> 8);
            value.push_back(v & 0xFF);
            prev = pos + 1;
        } while(prev < ipv6Str.size());

    } else {
        std::stringstream ss;
        ss << std::hex;
        for (size_t i = 0; i < value.size(); i += 2 ) {
            uint16_t v = value[i] << 8 | value[i + 1];
            ss << std::setw(4) << std::setfill('0') << v;
            if (i < (value.size() - 1)) {
                ss << ":";
            }
        }
        ipv6Str = ss.str();
    }
    return 0;
}
} //namespace Config
} //namespace SPLITS
