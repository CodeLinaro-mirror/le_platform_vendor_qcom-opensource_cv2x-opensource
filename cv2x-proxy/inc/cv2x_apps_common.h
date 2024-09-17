/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_APP_COMMON_H
#define CV2X_APP_COMMON_H

#include <telux/cv2x/legacy/v2x_radio_api.h>

//Error_code
#define SUCCESS 0
#define ERROR_NULL_POUIINTER -5
#define ERROR_GETPWNAM_FAILED -6
#define ERROR_SETRESUID_FAILED -7
#define ERROR_CAPSET_FAILED -8
#define ERROR_CAP_INVALID -9

#define MAX_DUMMY_PACKET_LEN (10000)
#define MAX_QTY_PAYLOADS (100)
#define MAX_SPS_FLOWS (2)
#define MAX_FLOWS (5)
/* Regarding these sample port#: any unused ports could be selected, and additional ports
 * could also be set-up for additional reservations and near unlimited number of event flows. */
#define SAMPLE_SPS_PORT   (2500)
#define SAMPLE_EVENT_PORT (2600)
#define DEFAULT_PAYLOAD_QTY (5)

#define DEFAULT_SERVICE_ID (1)
/* Simulated secured BSM lengths with 5 path history points */
#define PAYLOAD_SIZE_CERT (287)
#define PAYLOAD_SIZE_DIGEST (187)

/* Two possible reservation sizes, if only one made, it uses the largest of the two */
#define DEFAULT_SPS_RES_BYTES_0 (PAYLOAD_SIZE_CERT)
#define DEFAULT_SPS_RES_BYTES_1 (PAYLOAD_SIZE_DIGEST)

#define DEFAULT_PERIODICITY_MS (100)

// Some limits beyond which warnings will be generated, based on current known support
#define TOP_TYPICAL_PRECONFIG_SERVICE_ID (10)
#define WARN_MIN_PERIODICITY_MS (100)

// Microseconds to wait before timing out on read socket
#define READ_SOCKET_TIMEOUT_US (100000)

#define errExit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef enum {
    NoReg = 0,
    SpsOnly = 1,
    EventOnly = 2,
    ComboReg = 3
} FlowType_t;

// Application states for state machine
typedef enum {
    STATE_START = 0,
    STATE_INITIALIZE,
    STATE_SETUP_FLOWS,
    STATE_RECEIVE,
    STATE_TRANSMIT,
    STATE_ECHO,
    STATE_PAUSE,
    STATE_DEINITIALIZE,
    STATE_EXIT,
    NUM_STATES
} cv2x_app_state_t;

// Event flags set by asynchronous listeners/callbacks
typedef enum {
    EVENT_STATUS_TRANSITION_TO_INACTIVE = 0,
    EVENT_STATUS_TRANSITION_TO_ACTIVE,
    EVENT_SERVICE_STATUS_DOWN,
    EVENT_SERVICE_STATUS_UP,
    NUM_EVENT_FLAGS
} event_flags_t;

typedef enum {
    ModeTx = 0,
    ModeRx,
    ModeEcho
} TestMode_et;

typedef enum {
    tx_test_name_sps   = 0,
    tx_test_name_event = 1,
    tx_test_name_combo = 2
} tx_test_name_et;

typedef struct {
    traffic_ip_type_t ip_type;
    v2x_tx_sps_flow_info_t sps_flow_info;
    uint16_t sps_port;
    uint16_t evt_port;
    FlowType_t flow_type;
    int sps_sock;
    int event_sock;
    struct sockaddr_in6 sps_sockaddr;
    struct sockaddr_in6 event_sockaddr;
    struct sockaddr_in6 dest_sockaddr;
    int qty; // number of packets after which to quit, either tx/rx or echo. -1 = indefinite.
    unsigned long interval_btw_pkts_ns;
    int tx_sock;
    int tx_timer_fd;
    bool tx_thr_valid;
    pthread_t flow_tid;
    int payload_qty;
    int payload_len[MAX_QTY_PAYLOADS];
    int mean_payload_len;
    unsigned char ueid; // UE
    uint16_t  tx_seq_num;
    int sps_event_opt;
    char* tx_buf;
} v2x_tx_flow_content_t;

__inline uint64_t timestamp_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

const char* get_v2x_event_strings(v2x_event_t event);
const char* get_state_strings(cv2x_app_state_t state);
int cv2x_app_start_timer(long long interval_ns);
gid_t get_gid_byname(const char* names);
int set_supplementary_groups(char** grps, int size);
int change_user(char *userName, int8_t *caps, const int size);
unsigned int sps_period_regulation(unsigned int period);
void print_v2x_capabilities(v2x_iface_capabilities_t* capabilities);
void print_buffer(uint8_t *buffer, int buffer_len);
int parse_service_id_list(unsigned int *sid_list, char* param, const int output_level);
bool sps_period_validation_check(unsigned int period);
void install_signal_handler(void (*handler)(int));
#endif