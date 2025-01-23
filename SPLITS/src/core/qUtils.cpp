/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    //v2x_diag_log_init();
}

void QUtils::deInitDiagLog() {
    //v2x_diag_log_deinit();
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

int QUtils::hwTRNGChar(uint8_t *randomNumber) {
    telux::common::ErrorCode ec;
    std::vector<uint8_t> generatedData(1, 0);
    size_t numBytes = 0;
    /* Get SecurityFactory instance */
    auto &secFact = telux::sec::SecurityFactory::getInstance();
    std::shared_ptr<telux::sec::IRandomNumberManager> rngMgr;
    /* Get CryptoManager instance with TRNG as source */
    rngMgr = secFact.getRandomNumberManager(telux::sec::RNGSource::QTI_HW_TRNG, ec);
    if (!rngMgr) {
        LOGD(__FUNCTION__, " Can't allocate IRandomNumberManager, err: ",
                static_cast<int>(ec));
        return -1;
    }
    ec = rngMgr->getRandomData(generatedData, numBytes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOGD(__FUNCTION__, " failed data generation, err: ", static_cast<int>(ec));
        return -1;
    }
    memcpy(randomNumber,generatedData.data(),sizeof(generatedData.data()));
    return 0;
}

void QUtils::fillVersion(uint32_t *version)
{
    if (version != NULL) {
        *version = V2X_QITS_LOG_VERSION;
    }
}
