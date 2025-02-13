/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Cv2xRemoteRadioHelper.hpp"

namespace telux {
namespace cv2x {

void Capi2teluxHelper::resetV2xStatusEx(Cv2xStatusEx &status) {
    status.status.rxStatus      = Cv2xStatusType::INACTIVE;
    status.status.txStatus      = Cv2xStatusType::INACTIVE;
    status.status.rxCause       = Cv2xCauseType::UNKNOWN;
    status.status.txCause       = Cv2xCauseType::UNKNOWN;
    status.status.cbrValue      = 255;
    status.status.cbrValueValid = false;
    status.poolStatus.clear();
    status.timeUncertaintyValid = false;
}

Cv2xStatus Capi2teluxHelper::capi2teluxCv2xStatus(const capicv2x::Cv2xRadioTypes::Cv2xStatus &status) {
    Cv2xStatus st;
    st.rxStatus = static_cast<Cv2xStatusType>(status.getRxStatus().value_);
    st.txStatus = static_cast<Cv2xStatusType>(status.getTxStatus().value_);
    st.rxCause = static_cast<Cv2xCauseType>(status.getRxCause().value_);
    st.txCause = static_cast<Cv2xCauseType>(status.getTxCause().value_);
    st.cbrValue = status.getCbrValue();
    st.cbrValueValid = status.getCbrValueValid();
    return st;
}
std::vector<Cv2xPoolStatus> Capi2teluxHelper::capi2teluxCv2xPoolStatus(
    const std::vector<capicv2x::Cv2xRadioTypes::Cv2xPoolStatus> &status) {
    std::vector<Cv2xPoolStatus> pv;
    for (auto st : status) {
        Cv2xPoolStatus ps;
        ps.poolId = st.getPoolId();
        ps.status = Capi2teluxHelper::capi2teluxCv2xStatus(st.getStatus());
        pv.push_back(ps);
    }
    return pv;
}

Cv2xStatusEx Capi2teluxHelper::capi2teluxCv2xStatusEx(
    const capicv2x::Cv2xRadioTypes::Cv2xStatusEx statusEx) {
    Cv2xStatusEx stex;

    stex.status = Capi2teluxHelper::capi2teluxCv2xStatus(statusEx.getStatus());
    stex.poolStatus = Capi2teluxHelper::capi2teluxCv2xPoolStatus(statusEx.getPoolStatus());
    stex.timeUncertaintyValid = statusEx.getTimeUncertaintyValid();
    stex.timeUncertainty = statusEx.getTimeUncertainty();
    return stex;
}
std::vector<TxPoolIdInfo> Capi2teluxHelper::capi2teluxSupportedPoolIds(
    const std::vector<capicv2x::Cv2xRadioTypes::TxPoolIdInfo> &idInfos) {
    std::vector<TxPoolIdInfo> pv;

    for (auto idInfo : idInfos) {
        TxPoolIdInfo info;
        info.poolId = idInfo.getPoolId();
        info.minFreq = idInfo.getMinFreq();
        info.maxFreq = idInfo.getMaxFreq();
        pv.push_back(info);
    }
    return pv;
}

telux::cv2x::Cv2xRadioCapabilities Capi2teluxHelper::capi2teluxRadioCapbilities(
    const capicv2x::Cv2xRadioTypes::Cv2xRadioCapabilities &capabilities) {
    telux::cv2x::Cv2xRadioCapabilities cap;
    cap.linkIpMtuBytes = capabilities.getLinkIpMtuBytes();
    cap.linkNonIpMtuBytes = capabilities.getLinkNonIpMtuBytes();
    cap.maxSupportedConcurrency = static_cast<RadioConcurrencyMode>(
        capabilities.getMaxSupportedConcurrency().value_);
    cap.nonIpTxPayloadOffsetBytes = capabilities.getNonIpTxPayloadOffsetBytes();
    cap.nonIpRxPayloadOffsetBytes = capabilities.getNonIpRxPayloadOffsetBytes();
    cap.periodicitiesSupported = capabilities.getPeriodicitiesSupported();
    cap.periodicities = capabilities.getPeriodicities();
    cap.maxNumAutoRetransmissions = capabilities.getMaxNumAutoRetransmissions();
    cap.layer2MacAddressSize = capabilities.getLayer2MacAddressSize();
    cap.prioritiesSupported = capabilities.getPrioritiesSupported();
    cap.maxNumSpsFlows = capabilities.getMaxNumSpsFlows();
    cap.maxNumNonSpsFlows = capabilities.getMaxNumNonSpsFlows();
    cap.maxTxPower = capabilities.getMaxTxPower();
    cap.minTxPower = capabilities.getMinTxPower();
    cap.txPoolIdsSupported = capi2teluxSupportedPoolIds(capabilities.getTxPoolIdsSupported());

    return cap;
}

std::vector<TrustedUEInfo> Capi2teluxHelper::capi2teluxTrustedUEs(
    const std::vector<capicv2x::Cv2xRadioTypes::TrustedUEInfo> &capiTrustedUEs) {
    std::vector<TrustedUEInfo> infos;
    for (auto &capiTrustedUE : capiTrustedUEs) {
        TrustedUEInfo info;
        info.sourceL2Id = capiTrustedUE.getSourceL2Id();
        info.timeUncertainty = capiTrustedUE.getTimeUncertainty();
        info.timeConfidenceLevel = capiTrustedUE.getTimeConfidenceLevel();
        info.positionConfidenceLevel = capiTrustedUE.getPositionConfidenceLevel();
        info.propagationDelay = capiTrustedUE.getPropagationDelay();
        infos.push_back(info);
    }
    return infos;
}

SpsFlowInfo Capi2teluxHelper::capi2teluxSpsFlowInfo(
        const capicv2x::Cv2xRadioTypes::SpsFlowInfo &flowInfo) {
    SpsFlowInfo spsInfo;
    spsInfo.priority = static_cast<Priority>(flowInfo.getPriority().value_);
    spsInfo.periodicity = static_cast<Periodicity>(flowInfo.getPeriodicity().value_);
    spsInfo.periodicityMs = flowInfo.getPeriodicityMs();
    spsInfo.nbytesReserved = flowInfo.getNbytesReserved();
    spsInfo.autoRetransEnabledValid = flowInfo.getAutoRetransEnabledValid();
    spsInfo.autoRetransEnabled = flowInfo.getAutoRetransEnabled();
    spsInfo.peakTxPowerValid = flowInfo.getPeakTxPowerValid();
    spsInfo.peakTxPower = flowInfo.getPeakTxPower();
    spsInfo.mcsIndexValid = flowInfo.getMcsIndexValid();
    spsInfo.mcsIndex = flowInfo.getMcsIndex();
    spsInfo.txPoolIdValid = flowInfo.getTxPoolIdValid();
    spsInfo.txPoolId = flowInfo.getTxPoolId();
    return spsInfo;
}

SlssRxInfo Capi2teluxHelper::capi2teluxSlssRxInfo(
        const capicv2x::Cv2xRadioTypes::SlssRxInfo &slssRxInfo) {
    telux::cv2x::SlssRxInfo teluxInfo;
    for (auto capiUe : slssRxInfo.getUeInfo()) {
        telux::cv2x::SyncRefUeInfo teluxUeInfo;
        teluxUeInfo.slssId = capiUe.getSlssId();
        teluxUeInfo.inCoverage = capiUe.getInCoverage();
        teluxUeInfo.pattern = static_cast<SlssSyncPattern>(capiUe.getPattern().value_);
        teluxUeInfo.rsrp = capiUe.getRsrp();
        teluxUeInfo.selected = capiUe.getSelected();
        teluxInfo.ueInfo.push_back(teluxUeInfo);
    }
    return teluxInfo;
}

}  // end of namespace cv2x
}  // end namespace telux
