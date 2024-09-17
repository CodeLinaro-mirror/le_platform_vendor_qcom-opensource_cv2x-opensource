/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <stdbool.h>
#include <sys/types.h>
#include <grp.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <time.h>
#include <inttypes.h>
#include <v2x_log.h>
#include <errno.h>
#include <limits.h>

#include "cv2x_apps_common.h"

const char v2x_event_strings[V2X_TXRX_SUSPENDED + 1][20] = {
    "Inactive",
    "Active",
    "TX Suspended",
    "RX Suspended",
    "TX + RX Suspended"
};

const char g_state_names[NUM_STATES][20] = {
    "START",
    "INITIALIZE",
    "SETUP_FLOWS",
    "RECEIVE",
    "TRANSMIT",
    "ECHO",
    "PAUSE",
    "DEINITIALIZE",
    "EXIT"
};

const char* get_v2x_event_strings(v2x_event_t event) {
    if (event <= V2X_TXRX_SUSPENDED)
        return v2x_event_strings[event];
    else
        return "ERR: Unkonwn event.";
}

const char* get_state_strings(cv2x_app_state_t state) {
    if (state < NUM_STATES)
        return g_state_names[state];
    else
        return "ERR: Unkonwn state.";
}

int parse_service_id_list(unsigned int *sid_list, char* param, const int output_level) {
    char* saveptr = NULL;
    char* tok = param;
    char* endptr = NULL;
    int i = 0;
    int temp_len = 0;
    unsigned int temp_sid = 0;

    tok = strtok_r(tok, ",", &saveptr);
    while ((tok != NULL) && (i < MAX_SUBSCRIBE_SIDS_LIST_LEN)) {
        temp_sid = (unsigned int)strtoul(tok, &endptr, 0);

        if (errno == ERANGE && temp_sid == ULONG_MAX) {
            printf("range err for sid\n");
            return 0;
        } else if (endptr == tok || *endptr != '\0') {
            printf("sid convert fail\n");
            return 0;
        }

        sid_list[i] = temp_sid;
        i++;
        tok = strtok_r(NULL, ",", &saveptr);
    }

    if (i == 0) {
        printf("Invalid parameters for SID subsrprition\n");
        return 0;
    }

    temp_len = i;

    if (output_level) {
        for (i = 0; i < temp_len; ++i) {
            printf("Subscribe service ID %u\n", sid_list[i]);
        }
    }

    return temp_len;
}

/**
 * return true if sps period is one of 20, 50, 100, 200, 300,
 * 400, 600, 700, 800, 900, 1000ms.
 *
 * return false if the sps period is not supported
 */

bool sps_period_validation_check(unsigned int period) {
    if (0 == period || period > 1000) {
        return false;
    }

    if (20 == period || 50 == period || 0 == period % 100) {
        return true;
    }

    return false;
}

void install_signal_handler(void (*handler)(int))
{
    struct sigaction sig_action;

    sig_action.sa_handler = handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

int cv2x_app_start_timer(long long interval_ns)
{
    int timerfd;
    struct itimerspec its = {0};

    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) {
        return -1;
    }

    /* Start the timer */
    its.it_value.tv_sec = interval_ns / 1000000000;
    its.it_value.tv_nsec = interval_ns % 1000000000;
    its.it_interval = its.it_value;

    if (timerfd_settime(timerfd, 0, &its, NULL) < 0) {
        close(timerfd);
        return -1;
    }

    return timerfd;
}

/**
 * calculate a supported sps interval
 */
unsigned int sps_period_regulation(unsigned int period) {
    unsigned int ret = 0;
    if (period < 50) {
        ret = 20;
    } else if (period < 100) {
        ret = 50;
    } else {
        ret = period / 100;
        if (ret > 10) {
            ret = 1000;
        } else {
            ret *= 100;
        }
    }
    return ret;
}

void print_v2x_capabilities(v2x_iface_capabilities_t* capabilities) {
    if (!capabilities) {
        return;
    }
    printf("Modem capabilities:\n");
    printf("\tnon IP MTU: %d \n", capabilities->link_non_ip_MTU_bytes);
    printf("\tIP MTU: %d \n", capabilities->link_ip_MTU_bytes);
    printf("\tmin periodicity: %d ms\n", capabilities->int_min_periodicity_multiplier_ms);
    printf("\tmax periodicity (lowest reserved Freq): %d ms\n",
           capabilities->int_maximum_periodicity_ms);
    printf("\thighest priority number supported: %d (lower # is more urgent)\n",
           capabilities->highest_priority_value);
    printf("\tlowest priority number supported: %d (lower # is more urgent)\n",
           capabilities->lowest_priority_value);
    if (capabilities->tx_pool_ids_supported_len > 0u) {
        printf("\ttx pool ids supported:\n");
        for (uint32_t i = 0u; i < capabilities->tx_pool_ids_supported_len; i++) {
            printf("\t\tID: %u, min_freq: %u, max_freq: %u\n",
                   capabilities->tx_pool_ids_supported[i].pool_id,
                   capabilities->tx_pool_ids_supported[i].min_freq,
                   capabilities->tx_pool_ids_supported[i].max_freq);
        }
    }
}

/*******************************************************************************
 * Function: print_buffer
 * Description: Print out a buffer in hex
 * Input Parameters:
 *      buffer: buffer to print
 *      buffer_len: number of bytes in buffer
 ******************************************************************************/
void print_buffer(uint8_t *buffer, int buffer_len)
{
    uint8_t *pkt_buf_ptr;
    int items_printed = 0;

    pkt_buf_ptr = buffer;

    while (items_printed < buffer_len) {
        if (items_printed  && items_printed % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", *pkt_buf_ptr);
        pkt_buf_ptr++;
        items_printed++;
    }
    printf("\n");
}

gid_t get_gid_byname(const char* names) {
    gid_t tmp_gid = 0;
    struct group* temp_grp = NULL ;

    if ((temp_grp = getgrnam(names)) != NULL) {
        tmp_gid = temp_grp->gr_gid;
    }
    return tmp_gid;
}

/* Utility to add supplementary groups */
int set_supplementary_groups(char** grps, int size) {
    int ret = 0;
    gid_t group_id[size];
    int num_groups = getgroups(0, NULL);

    for (int i = 0; i < size; i++) {
        group_id[i] = get_gid_byname(*(grps + i));
    }

    gid_t gid[num_groups + size];
    ret = getgroups(num_groups, gid);
    for (int i = 0; i < size; i++) {
        gid[num_groups + i] = group_id[i];
    }
    ret = setgroups(num_groups + size, gid);
    return ret;
}

int change_user(char *user_name, int8_t *caps, const int size) {
    bool has_valid_caps = false;
    int i = 0;
    struct passwd *p;
    /*Retain necessary capabilities for the new user*/
    struct __user_cap_header_struct header = {0};
    /* V3 supported since Linux 2.6.26 */
    header.version = _LINUX_CAPABILITY_VERSION_3;
    struct __user_cap_data_struct cap_set[CAP_TO_INDEX(CAP_LAST_CAP) + 1] = {0};

    if (!user_name) {
        printf("user_name is NULL!!\n");
        return ERROR_NULL_POUIINTER;
    }
    /*Get user id by supplied user name*/
    if ((p = getpwnam(user_name)) == NULL) {
        printf("get passwd fialed!\n");
        return ERROR_GETPWNAM_FAILED;
    }

    /*Allow retain capabilities in its permitted set when switches to non root user*/
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    /*Switch to specified user*/
    if (setresuid(p->pw_uid, p->pw_uid, p->pw_uid) == -1) {
        printf("setresuid fialed!\n");
        return ERROR_SETRESUID_FAILED;
    }

    if (!caps) {
        printf("capbilitys is NULL!!\n");
        return SUCCESS;
    }

    for (i = 0; i < size; i++) {
        if (cap_valid(caps[i])) {
            has_valid_caps = true;
            cap_set[CAP_TO_INDEX(caps[i])].effective |= CAP_TO_MASK(caps[i]);
            cap_set[CAP_TO_INDEX(caps[i])].permitted |= CAP_TO_MASK(caps[i]);
        }
    }

    if (has_valid_caps) {
        if (capset(&header, &cap_set[0]) == -1) {
            printf("capset is failed!!\n");
            return ERROR_CAPSET_FAILED;
        }
    } else {
        printf("capbilitys is not valid\n");
        return ERROR_CAP_INVALID ;
    }
    return SUCCESS;
}