/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __V2XMSG_LOGGER_HPP__
#define __V2XMSG_LOGGER_HPP__

#include <iostream>
#include <iomanip>
#include "v2xMsg.hpp"

using std::ostream ;

namespace SPLITS {

class v2xMsgLogger {
public:
    v2xMsgLogger(ostream *os) : os_(os) {}
    virtual ~v2xMsgLogger(){}

    virtual void logMsg(std::shared_ptr<SPLITS::v2xMsg> msg) = 0;

private:
    ostream *os_;

};
} //namespace SPLITS
#endif //__V2XMSG_LOGGER_HPP__

