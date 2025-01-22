/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted (subject to the limitations in the
 *disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /**
  * @file: qUtils.hpp
  *
  * @brief: Implementation for Utilities for qApplication
  */
#ifndef QUTILS_HPP_
#define QUTILS_HPP_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include "v2x_diag.h"
#include "qDiagLogPacketDef.h"

#define V2X_APPS_DIAG_LOG_PKT(type, pbuf, buf_size)                            \
    v2x_diag_log_state_et ec = v2x_diag_log_packet(type, pbuf, buf_size);      \
    if (ERR_SUCCESS != ec && ERR_STATUS_FAIL != ec) {                          \
        printf("%s: send type[0x%x], errcode: %d \n", __FUNCTION__, type, ec); \
    }

class QUtils
{
private:
    void fillVersion(uint32_t *version);

public:
    void initDiagLog();
    void deInitDiagLog();
    int hwTRNGInt(uint32_t &randomNumber);
    int hwTRNGChar(uint8_t* randomNumber);

    template<typename InfoType>
    void sendLogPacket(InfoType *info, v2x_diag_log_packet_et type)
    {
        this->fillVersion(&info->Version);

        V2X_APPS_DIAG_LOG_PKT(type, info, sizeof(InfoType));
    }
};
#endif
