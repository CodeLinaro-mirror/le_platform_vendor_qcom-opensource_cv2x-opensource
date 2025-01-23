/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CSV_IO_HPP
#define CSV_IO_HPP

#include <fstream>
#include <mutex>
#include <string>

using namespace std;

namespace SPLITS {
namespace Core {

class CsvIO {
public:
    CsvIO(const std::string dir);
    ~CsvIO();

    /**
     *  @brief: Writes to the CSV file as a fresh file.
     *
     *  @param: data          -   The content to be written to the file.
     */
    bool writeTx(const std::string &data);

    /**
     *  @brief: append the CSV file
     *
     *  @param: data          -   The content to be written to the file.
     */
    bool writeRx(const std::string &data);

private:
    std::mutex fileMutex_;
    std::ofstream txOfs_;
    std::ofstream rxOfs_;
};

}
}

#endif  // CSV_IO_HPP
