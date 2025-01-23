/*
 *  Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CsvIO.hpp"
#include "Logger.hpp"

#define DELIMETER ','

namespace SPLITS {
namespace Core {

constexpr auto TX_FILENAME = "splits_tx.csv";
constexpr auto RX_FILENAME = "splits_rx.csv";

CsvIO::CsvIO(const std::string csvFileDir) {
    LOGD("csv file path: ", csvFileDir);
    txOfs_ = std::ofstream((csvFileDir + "/" + TX_FILENAME).c_str(), std::ofstream::trunc);
    rxOfs_ = std::ofstream((csvFileDir + "/" + RX_FILENAME).c_str(), std::ofstream::trunc);
}

CsvIO::~CsvIO() {
}

bool CsvIO::writeTx(const std::string &data) {
    std::lock_guard<std::mutex> lck(CsvIO::fileMutex_);
    txOfs_ << data;
    return true;
}

bool CsvIO::writeRx(const std::string &data) {
    std::lock_guard<std::mutex> lck(CsvIO::fileMutex_);
    rxOfs_ << data;
    return true;
}

} // end of namespace common

} // end of namespace telux
