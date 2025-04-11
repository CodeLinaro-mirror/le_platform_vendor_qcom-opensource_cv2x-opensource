/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /**
  * @file: qUtils.cpp
  *
  * @brief: Implementation of Utilities for qApplication.
  *
  */

#include <cstring>
#include <telux/sec/RandomNumberManager.hpp>
#include <telux/sec/SecurityFactory.hpp>

#include "Logger.hpp"
#include "qUtils.hpp"

void QUtils::initDiagLog() {
    v2x_diag_log_init();
}

void QUtils::deInitDiagLog() {
    v2x_diag_log_deinit();
}

int QUtils::hwTRNGInt(uint32_t& randomNumber) {
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr;

    /* Get SecurityFactory instance */
    auto &secFact = telux::sec::SecurityFactory::getInstance();
    /* Get CryptoManager instance with TRNG as source */
    rngMgr = secFact.getRandomNumberManager(telux::sec::RNGSource::QTI_HW_TRNG, ec);
    if (!rngMgr) {
        LOGD(__FUNCTION__, " Can't allocate IRandomNumberManager, err: ",
                static_cast<int>(ec));
        return -1;
    }
     /* Generate 32 bit random number */
    ec = rngMgr->getRandomNumber(randomNumber);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOGD(__FUNCTION__, " failed 32 bit number generation, err: ", static_cast<int>(ec));
        return -1;
    }
    return 0;
}

int QUtils::hwTRNGData(uint8_t *data, size_t len) {
    telux::common::ErrorCode ec;
    std::vector<uint8_t> generatedData(len, 0);
    size_t numBytes = 0;
    /* Get SecurityFactory instance */
    auto &secFact = telux::sec::SecurityFactory::getInstance();
    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr;
    /* Get CryptoManager instance with TRNG as source */
    rngMgr = secFact.getRandomNumberManager(telux::sec::RNGSource::QTI_HW_TRNG, ec);
    if (!rngMgr) {
        LOGE(__FUNCTION__, " Can't allocate IRandomNumberManager, err: ",
                static_cast<int>(ec));
        return -1;
    }
    ec = rngMgr->getRandomData(generatedData, numBytes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOGE(__FUNCTION__, " failed data generation, err: ", static_cast<int>(ec));
        return -1;
    }
    memcpy(data, generatedData.data(), sizeof(generatedData.data()));
    return 0;
}

void QUtils::fillVersion(uint32_t *version)
{
    if (version != NULL) {
        *version = V2X_QITS_LOG_VERSION;
    }
}
