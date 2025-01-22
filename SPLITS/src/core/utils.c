/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "utils.h"

/*******************************************************************************
 * Function: print_buffer
 * Description: Print out a buffer in hex
 * Input Parameters:
 *      buffer: buffer to print
 *      buffer_len: number of bytes in buffer
 ******************************************************************************/
void print_buffer(uint8_t *buffer, int buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; i++) {
        if (i%16 == 0 && i != 0)
            printf("\n");
        printf("%02x ", *buffer++);
    }
}

/*******************************************************************************
 * return current time stamp in milliseconds
 * @return long long
 ******************************************************************************/
uint64_t timestamp_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000LL + (uint64_t)tv.tv_usec / 1000;
}
/*******************************************************************************
 * return current time stamp in microseconds
 * @return long long
 ******************************************************************************/
uint64_t timestamp_now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000LL + (uint64_t)tv.tv_usec;
}
uint64_t calc_timestamp_from_secmark(unsigned int secMark_ms)
{
    uint64_t millis = timestamp_now();
    uint64_t min = millis / 60000;
    uint64_t rem = millis % 60000;
    if (rem >= secMark_ms)
        return min * 60000 + secMark_ms;
    else {
        return (min - 1) * 60000 + secMark_ms;
    }
}
void get_ddate_time(struct ddate_time *dt)
{
    // Get the current time.
    time_t t = time(NULL);

    // Convert it to local time representation.
    struct tm *tm_info = localtime(&t);

    // Extract day, month, and year.
    dt->day = (uint8_t)tm_info->tm_mday;        // Day of the month (1-31).
    dt->month = (uint8_t)(tm_info->tm_mon + 1);   // Month (0-11, so we add 1).
    dt->year = (uint16_t)(tm_info->tm_year + 1900); // Year since 1900.
    int hour = (uint8_t)tm_info->tm_hour;
    int min = (uint8_t)tm_info->tm_min;
    int sec = (uint8_t)tm_info->tm_sec;
}
uint32_t get_minute_of_the_year(void)
{
#if 0
    struct tm year_begin;
    time_t tt_now = time(NULL);
    time_t tt_year_begin;
    //struct tm *tm_info_now = localtime(&now);

    year_begin.tm_year = tt_now.tm_year;
    year_begin.tm_mon = 0;
    year_begin.tm_mday = 0;
    year_begin.tm_hour = 0;
    year_begin.tm_min = 0;
    year_begin.tm_sec = 0;
    tt_year_begin = mktime(&year_begin);
    return (tt_now - tt_year_begin)/60;
#endif
    return 0;
}
