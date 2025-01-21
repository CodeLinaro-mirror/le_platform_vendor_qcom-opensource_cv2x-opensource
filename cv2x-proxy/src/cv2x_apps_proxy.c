/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <linux/in6.h>
#include <stdio.h>
#include <sys/types.h>
#include <net/if.h>
#include <netdb.h>
#include <glib.h>
#include <sys/capability.h>
#include <inttypes.h>

#include "v2x_log.h"
#include "cv2x_radiotap.h"
#include "cv2x_apps_common.h"

cv2x_app_state_t state_start();
cv2x_app_state_t state_initialize();
cv2x_app_state_t state_setup_flows();
cv2x_app_state_t state_receive();
cv2x_app_state_t state_transmit(int flow_id);
cv2x_app_state_t state_pause(bool primary_flow);
cv2x_app_state_t state_deinitialize();
int air2net_forward(int rx_sock);
int net2air_forward(int flow_id);
void init_air2net_info();
void init_net2air_info();

#define DEFAULT_IF_NAME "rndis0"
#define MAX_IP_LENGTH 64
#define DEFAULT_IP_ADDR "127.0.0.1"
#define DEFAULT_LOOPBACK_IFANCE_NAME "lo"
#define DEFAULT_MUL_ADDR "ff02::1"
#define PROXY_MAX_FLOWS (1)

typedef struct proxy_air2net_info_t {
    char if_name[IFNAMSIZ];
    char remote_net_address[MAX_IP_LENGTH];
    uint16_t air2net_forward_to_port;
    int forward_sockfd;
    int rx_sockfd;
    int family;
    bool rt_flag;
    bool remove_radiotap_family;    // remove family id byte option
    struct sockaddr_in6 rx_sockaddr;
    struct sockaddr_in6 remote_sin;
    struct sockaddr_in remote_sin4;
} proxy_air2net_info;

typedef struct proxy_net2air_info_t {
    char iface[IFNAMSIZ];
    char dest_ipv6_addr_str[MAX_IP_LENGTH];
    uint16_t net2air_listen_port;
    int listen_net_socket;
    uint8_t radiotap_family_id; // by default, it will be IEEE1609 if nothing specified
    struct sockaddr_in6 proxy_listen_sin;
    bool add_radiotap_family;   // add radiotap family id byte option (requires arg)
} proxy_net2air_info;

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
    int mean_payload_len;
    int sps_event_opt;
    char* tx_buf;
} proxy_flow_info_t;

cv2x_app_state_t mode_to_state[] = {STATE_TRANSMIT, STATE_RECEIVE};
TestMode_et test_mode = ModeTx;  // net2air mode by default

/*
 * The following variables are for the radiotap parser for sniffer testing
 * radiotap hdr | ieee 80211 hdr | 8023 edp | acme message
 */
static radiotap_msg_hdr_t radiotap_msg_hdr; //includes radiotap + 80211
//air2net
int do_air2net_proxy = 0;
proxy_air2net_info air2net_control;
//net2air
int do_net2air_proxy = 0;
proxy_net2air_info net2air_control;
//statistics
int rx_count = 0; // number of packets to RX or TX. -1 = indefinite.
uint32_t g_tx_count = 0; // number of tx packets
int lost_tx_cnt = 0; // Count how many times the status indicated loss of timing precision, causin gTx suspend
int lost_rx_cnt = 0; // Count the number of times, status indicated a loss of RX timing

proxy_flow_info_t g_tx_flow_content[PROXY_MAX_FLOWS];
int dump_opt = 0;
int g_verbosity = 1;
int g_use_syslog = 0;

v2x_radio_handle_t handles[2] = {V2X_RADIO_HANDLE_BAD, V2X_RADIO_HANDLE_BAD};
v2x_iface_capabilities_t g_capabilities;
bool g_capabilities_valid = false;

v2x_radio_calls_t radio_calls;
v2x_per_sps_reservation_calls_t sps_function_calls;

pthread_mutex_t g_capability_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_sock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_status_cv;
pthread_mutex_t g_status_mutex;
pthread_cond_t g_radio_init_cv;
pthread_mutex_t g_radio_init_mutex;

/* counter flow_cnt keeps track of how many total flows (event only, sps, etc) have been set-up.
 * starts from 0, and used as an index into sps_flow_info[], sps_port[], evt_port[] etc
 * */
unsigned int flow_cnt = 0;
static char buf[MAX_DUMMY_PACKET_LEN];

v2x_service_status_t g_service_status;

static int g_terminate = 0;
static int g_terminate_pipe[2];

static int g_sid_list_len = 0;
static unsigned int g_sid_list[MAX_SUBSCRIBE_SIDS_LIST_LEN] = {0};

bool g_event_flags[NUM_EVENT_FLAGS] = { false };

uint16_t cla_portnum = 0; // CLA command line argument for port spec, 0 indicates use default
bool cla_portnum_set = false;

long long  g_reportInterval_ns = 1000000000;
double gd_reportInterval_sec = 1;
// Just a odd measurement to assume, till we hear otherwise via callback
float g_cbp_meas = 99.0;
// recent Event, from listner: effectively the status (TX/RX suspended/Active/etc)
v2x_event_t g_status = V2X_INACTIVE;
// Radio initialization status (Fail/SUCCESS/NOTREADY)
v2x_status_enum_type g_radio_init_status = V2X_STATUS_RADIO_PLACEHOLDER;
// Time uncertainty
float g_time_uncertainty = -1.0f;
static bool g_report_thr_valid = false;
// Enable print of periodic Rx report
bool g_set_rx_report = false;

void show_results(FILE *fp)
{
    fprintf(fp, "Packets Received: %d\n", rx_count);
    fprintf(fp, "Packets Transmitted: %d\n", g_tx_count);

    if (lost_tx_cnt) {
        fprintf(fp, "*** WARNING: %d ocassions of lost MAC timing good enough for TX\n",
            lost_tx_cnt);
    }

    if (lost_rx_cnt) {
        fprintf(fp, "*** WARNING: %d ocassions of lost MAC timing good enough support RX\n",
            lost_rx_cnt);
    }

}

void init_complete(v2x_status_enum_type status, void *ctx)
{
    pthread_mutex_lock(&g_radio_init_mutex);

    LOGD("Init complete callback. status=%d ctx=%08lx\n", status, *(unsigned long *)ctx);
    if (V2X_STATUS_FAIL == status) {
        LOGE("Failed to initalize radio with status=%d, bail\n", status);
    }

    g_radio_init_status = status;
    pthread_mutex_unlock(&g_radio_init_mutex);
    pthread_cond_signal(&g_radio_init_cv);
}

void radio_listener(v2x_event_t event, void *ctx)
{
    if (ctx) {
        LOGD("Radio event listener callback: status=%d (%s)  ctx=%08lx\n",
             event,
             get_v2x_event_strings(event),
             *(unsigned long *) ctx);
        pthread_mutex_lock(&g_status_mutex);
        if (event != g_status) {
            if (g_status == V2X_INACTIVE) {
                g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE]   = true;
                g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = false;
            } else if (event == V2X_INACTIVE) {
                g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE]   = false;
                g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = true;
            }

            g_status = event;
            LOGW("TX/RX Status Changed to <%s> **** \n", get_v2x_event_strings(event));
            switch (event) {
            case V2X_TXRX_SUSPENDED:
                lost_tx_cnt++;
                lost_rx_cnt++;
                break;

            case V2X_TX_SUSPENDED:
                lost_tx_cnt++;
                break;

            case V2X_RX_SUSPENDED:
                lost_rx_cnt++;
                break;
            }

            // Signal the main thread that status has changed
            pthread_cond_broadcast(&g_status_cv);
        } // Status actually changed.
        pthread_mutex_unlock(&g_status_mutex);
    } else {
        LOGE("NULL Context on radio_listener\n");
    }
}

void meas_listener(v2x_chan_measurements_t *meas_p, void *ctx)
{
    if (meas_p) {
        g_cbp_meas = meas_p->channel_busy_percentage;

        if (meas_p->time_uncertainty >= 0) {
            g_time_uncertainty = meas_p->time_uncertainty;
        }
        LOGD("Radio measurement callback, CBR=%3.0f\% TUNC=%6.8fms\n",
             g_cbp_meas, g_time_uncertainty);

    } else {
        LOGE("NULL ptr on meas_listener() callback\n");
    }
}

void service_status_listener(v2x_service_status_t status, void *ctx)
{
    LOGI("service_status_listener, service_status=%s\n",
         (status == SERVICE_AVAILABLE) ? "AVAILABLE" : "UNAVAILABLE");

    pthread_mutex_lock(&g_status_mutex);
    if (g_service_status != status) {
        g_service_status = status;
        if (status == SERVICE_AVAILABLE) {
            g_event_flags[EVENT_SERVICE_STATUS_UP] = true;
            pthread_cond_broadcast(&g_status_cv);
        } else {
            g_event_flags[EVENT_SERVICE_STATUS_DOWN] = true;
            g_status = V2X_INACTIVE;
        }
    }
    pthread_mutex_unlock(&g_status_mutex);
}

/**
 * prints warnings
 * @param[in]  pointer to the structure containing the retrieved modem capabilities
 */
static void v2x_test_param_check(v2x_iface_capabilities_t *caps) {
    int i = 0;
    int largest_sps_res_bytes = -1;

    if (!caps) {
        return;
    }

    if (g_tx_flow_content[i].sps_flow_info.reservation.v2xid
        > TOP_TYPICAL_PRECONFIG_SERVICE_ID) {
        printf("** WARN ** ");
        printf("PSID/V2XID selected may not be a common preconfigured TQ service ID's\n");
    }

    // use mean or default payload len for sps reservation if not specified
    if (g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes < 0
        && ((SpsOnly == g_tx_flow_content[i].flow_type)
        || (ComboReg == g_tx_flow_content[i].flow_type))) {
        g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes =
            ((g_tx_flow_content[i].mean_payload_len > 0) ?
                g_tx_flow_content[i].mean_payload_len : DEFAULT_SPS_RES_BYTES_0);
    }

    if ((g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms < 0)
        && (g_tx_flow_content[i].interval_btw_pkts_ns > 0)) {
        g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms =
            sps_period_regulation(g_tx_flow_content[i].interval_btw_pkts_ns / 1000000);
        if (g_verbosity) {
            printf("# SPS Interval periodicity not specified, using packet-gen interval:%d ms.\n",
                    g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms);
        }
    }

    // SPS Periodicity warnings
    if ((g_tx_flow_content[i].flow_type != EventOnly)
        && (g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms > 0)) {

        if (g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms
            < WARN_MIN_PERIODICITY_MS) {
            printf("** WARN ** periodcity less than supported.\n");
        }

        if (g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes
            > largest_sps_res_bytes) {
            largest_sps_res_bytes
                = g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes;
        }
    }
    printf("Flow#%d: type=%d file-desciptors:(%2d %2d) sps_port=%d evt_port=%d, %d ms, %d bytes\n",
            i, g_tx_flow_content[i].flow_type, g_tx_flow_content[i].sps_sock,
            g_tx_flow_content[i].event_sock,
            g_tx_flow_content[i].sps_port, g_tx_flow_content[i].evt_port,
            g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms,
            g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes);
}

static void radio_query_and_print_param()
{
    v2x_iface_capabilities_t caps;

    if (V2X_STATUS_SUCCESS == v2x_radio_query_capabilities(&caps)) {
        pthread_mutex_lock(&g_capability_mutex);
        memcpy(&g_capabilities, &caps, sizeof(v2x_iface_capabilities_t));
        g_capabilities_valid = true;
        pthread_mutex_unlock(&g_capability_mutex);
        LOGI("query V2X capabilities succeeded!\n");
        if (g_verbosity) {
            print_v2x_capabilities(&caps);
        }
    } else {
        pthread_mutex_lock(&g_capability_mutex);
        g_capabilities_valid = false;
        pthread_mutex_unlock(&g_capability_mutex);
    }
}

/*
 * Serialize RadioTap message
 * Returns number of bytes in serialized radio tap buffer
 * @return uint32_t
 */
uint32_t serialize_radiotap_message(char* buf_radiotap) {
    uint8_t *buf_ptr;
    buf_ptr = buf_radiotap;  // buf is a static variable
    uint32_t msg_len = 0;

    //ieee80211_radiotap_header radiotap_hdr;
    // radio tap header is LITTLE endian.
    memcpy(buf_ptr, & radiotap_msg_hdr.radiotap_hdr,
            sizeof(struct ieee80211_radiotap_header));
    buf_ptr += sizeof(struct ieee80211_radiotap_header);
    msg_len += sizeof(struct ieee80211_radiotap_header);

    // might want to make the following variables optional with flags

    // ch_freq
    memcpy(buf_ptr, & radiotap_msg_hdr.ch_freq, sizeof(uint16_t));
    buf_ptr += sizeof(uint16_t);
    msg_len += sizeof(uint16_t);

    // ieee80211_radiotap_channel_flags
    memcpy(buf_ptr, & radiotap_msg_hdr.ch_flags,
            sizeof(radiotap_msg_hdr.ch_flags));
    buf_ptr += sizeof(radiotap_msg_hdr.ch_flags);
    msg_len += sizeof(radiotap_msg_hdr.ch_flags);

    // ieee80211_wifi_hdr_t
    memcpy(buf_ptr, & radiotap_msg_hdr.ieee80211_wifi_hdr,
         sizeof(struct ieee80211_wifi_hdr_t));
    buf_ptr += sizeof(struct ieee80211_wifi_hdr_t);
    msg_len += sizeof(struct ieee80211_wifi_hdr_t);

    //unsigned short 8023_EDP
    unsigned short epd_hdr_htons = htons(radiotap_msg_hdr.edp_hdr);
    memcpy(buf_ptr, & epd_hdr_htons, sizeof(unsigned short));
    buf_ptr += sizeof(unsigned short);
    msg_len += sizeof(unsigned short);

    return msg_len;
}

/*
 * The following variables are for the radiotap parser for sniffer testing
 * radiotap hdr | ieee 80211 hdr | 8023 edp | acme message
 */
void fill_80211_hdr (uint32_t src_l2_addr) {
    // for now maybe hardcode for simplicity..  and testing purposes
    uint8_t broadcast_mac_arr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t src_addr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    for(int i = 1; i < IEEE80211_ADDR_LEN-2; i++) {
        src_addr[i-1] = (src_l2_addr >> (i*8)) & 0x000000ff;
    }
    radiotap_msg_hdr.ieee80211_wifi_hdr.frameCtl = 0x0088;
    radiotap_msg_hdr.ieee80211_wifi_hdr.durationId = 0x0000;
    memcpy( radiotap_msg_hdr.ieee80211_wifi_hdr.addr1, broadcast_mac_arr,
        IEEE80211_ADDR_LEN);
    memcpy( radiotap_msg_hdr.ieee80211_wifi_hdr.addr2, src_addr,
        IEEE80211_ADDR_LEN);
    memcpy( radiotap_msg_hdr.ieee80211_wifi_hdr.addr3, broadcast_mac_arr,
        IEEE80211_ADDR_LEN);
    radiotap_msg_hdr.ieee80211_wifi_hdr.sequence = 0xda20;
    // Here we will need to transfer the channel priority of the received message
    radiotap_msg_hdr.ieee80211_wifi_hdr.qos = 0x0025;
}

/* This function is for the alternative sniffer proxy tests.
 * This will preprend a RadioTap Header and a 802.11 header to the packet
 * for radiotap-based tools to decode.
 * Parameters: original message msg, latitude/lontitude, source mac address,
 * Radio Tap Header must have a channel # in it
 * 802.11 header must have the source address in it and (maybe)
 * priority (traffic class)
 * Return Value: None
 */
void create_radiotap_header(unsigned short edpType,
        uint32_t src_l2_addr, uint32_t rcvMsgLen){
    // Setting up the Radio Tap Header
    //  radiotap_msg_hdr.radiotap_hdr.it_version always 0
    //  it_pad is 0
    radiotap_msg_hdr.radiotap_hdr.it_version = (uint8_t)0;
    radiotap_msg_hdr.radiotap_hdr.it_pad = (uint8_t)0;
    radiotap_msg_hdr.radiotap_hdr.it_len =
        (uint16_t)sizeof( radiotap_msg_hdr.radiotap_hdr );
    radiotap_msg_hdr.radiotap_hdr.it_len +=
          sizeof(uint16_t) + sizeof(uint16_t); // for ch flags
    radiotap_msg_hdr.radiotap_hdr.it_present  = (uint32_t)0x00000000;
    /*radiotap flags*/
    radiotap_msg_hdr.radiotap_hdr.it_present |= (uint32_t)1;
    /*radiotap Channel*/
    radiotap_msg_hdr.radiotap_hdr.it_present <<= 3;
    radiotap_msg_hdr.ch_freq = 0x00B7; // 183 in MHz for now..
    // CCK, OFDM, 2GHZ, 5GHZ(0x0100), DYN, HALF, QUARTER
    radiotap_msg_hdr.ch_flags = 0x0100;

    // Setting up the 802.11 Header
    fill_80211_hdr (src_l2_addr);

    // Setting the EDP bytes (WSMP or ETSI)
    radiotap_msg_hdr.edp_hdr = edpType;
}

int sample_tx(int flow_id, int n_bytes)
{
    int bytes_sent = 0;
    struct msghdr message = {0};
    struct iovec iov[1] = {0};
    struct cmsghdr *cmsghp = NULL;
    char control[CMSG_SPACE(sizeof(int))];
    int socket = g_tx_flow_content[flow_id].tx_sock;
    char* buf = g_tx_flow_content[flow_id].tx_buf;
    struct sockaddr_in6 sockaddr = g_tx_flow_content[flow_id].dest_sockaddr;
    v2x_priority_et tx_prio = g_tx_flow_content[flow_id].sps_flow_info.reservation.priority;
    uint32_t tx_count = 0;
    if (socket < 0) {
        return 0;
    }

    if (cla_portnum_set) {
        sockaddr.sin6_port = htons((uint16_t)cla_portnum);
    }

    /* AIR2NET PROXY OPTION - Add Family ID Byte */
    if(net2air_control.add_radiotap_family){
        printf("Adding family ID to the packet\n");
        memcpy(buf + 1, buf, n_bytes);
        memset(buf, '\0', 1);
        buf[0] = net2air_control.radiotap_family_id;
        n_bytes++;
    }

    /* Send data using sendmsg to provide IPV6_TCLASS per packet */
    iov[0].iov_base = buf;
    iov[0].iov_len = n_bytes;
    message.msg_name = &sockaddr;
    message.msg_namelen = sizeof(sockaddr);
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    /* Fill ancillary data */
    cmsghp = CMSG_FIRSTHDR(&message);
    cmsghp->cmsg_level = IPPROTO_IPV6;
    cmsghp->cmsg_type = IPV6_TCLASS;
    cmsghp->cmsg_len = CMSG_LEN(sizeof(int));
    *((int *)CMSG_DATA(cmsghp)) = v2x_convert_priority_to_traffic_class(tx_prio);

    bytes_sent = sendmsg(socket, &message, 0);

    if (bytes_sent < 0) {
        fprintf(stderr, "Error sending message: %d\n", bytes_sent);
        bytes_sent = -1;
    } else if (bytes_sent != n_bytes) {
        printf("TX bytes sent were short\n");
    } else {
        tx_count = ++g_tx_count;
        if (g_verbosity || dump_opt) {
            printf("%s Flow #%d: TX count: %d, len = %d\n",
                   (g_tx_flow_content[flow_id].flow_type == EventOnly) ? "Event" : "SPS",
                   flow_id, tx_count, bytes_sent);
            if (dump_opt) {
                print_buffer(buf, bytes_sent);
            }
        }
    }

    return bytes_sent;
}

void close_all_v2x_sockets(void)
{
    int i;

    if (g_verbosity) {
        printf("closing all V2X Sockets \n");
    }

    if (test_mode != ModeTx && air2net_control.rx_sockfd >= 0) {
        v2x_radio_sock_close(&air2net_control.rx_sockfd);
        proxy_flow_info_t* rx_info = &g_tx_flow_content[0];

        // close additional event flow created for specific service ID subscription
        if (g_sid_list_len > 0 && rx_info->event_sock >= 0) {
            printf("closing event flow for specific SID subscription: evt_port=%d, evt_sock=%d\n",
                    rx_info->evt_port, rx_info->event_sock);
            v2x_radio_sock_close(&rx_info->event_sock);
        }
    }

    if (test_mode != ModeRx) {
        for (i = 0; i <= flow_cnt &&
             (g_tx_flow_content[i].sps_sock >= 0 || g_tx_flow_content[i].event_sock >= 0); i++) {
            g_tx_flow_content[i].tx_sock = -1;
            if (g_tx_flow_content[i].flow_type) {
                printf("closing Flow#%d: type=%d %d %d sps_port=%d evt_port=%d, %d ms, %d bytes\n",
                       i,
                       g_tx_flow_content[i].flow_type,
                       g_tx_flow_content[i].sps_sock,
                       g_tx_flow_content[i].event_sock,
                       g_tx_flow_content[i].sps_port,
                       g_tx_flow_content[i].evt_port,
                       g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms,
                       g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes
                      );

                if (g_tx_flow_content[i].sps_sock >= 0) {
                    v2x_radio_sock_close(&g_tx_flow_content[i].sps_sock);
                }

                if (g_tx_flow_content[i].event_sock >= 0) {
                    v2x_radio_sock_close(&g_tx_flow_content[i].event_sock);
                }
            }
        }
    }
}

static void termination_handler(int signum)
{
    unsigned terminate = 1;
    ssize_t s = write(g_terminate_pipe[1], &terminate, sizeof(int));
}

static void termination_cleanup()
{
    // Need to wait for the main thread to finish
    // before attempting to cleanup and exit
    printf("tearing down all services\n");

    pthread_mutex_lock(&g_sock_mutex);
    close_all_v2x_sockets();

    if (handles[TRAFFIC_IP] != V2X_RADIO_HANDLE_BAD) {
        v2x_radio_deinit(handles[TRAFFIC_IP]);
    }
    if (handles[TRAFFIC_NON_IP] != V2X_RADIO_HANDLE_BAD) {
        v2x_radio_deinit(handles[TRAFFIC_NON_IP]);
    }

    show_results(stdout);

    pthread_mutex_unlock(&g_sock_mutex);
}

void *terminate_cleanup_task(void *arg)
{
    int terminate = 0;
    ssize_t s;
    s = read(g_terminate_pipe[0], &terminate, sizeof(int));
    LOGD("Read terminate %d\n", terminate);

    pthread_mutex_lock(&g_status_mutex);
    g_terminate = 1;
    pthread_cond_broadcast(&g_status_cv);
    pthread_mutex_unlock(&g_status_mutex);

    return NULL;
}

void print_periodic_rx_report(FILE *print_fp,
                              bool print_banner,
                              int delta,
                              int active_RV_cnt,
                              uint64_t t_now_us,
                              double pps)
{
    if (NULL == print_fp) {
        return;
    }

    if (print_banner) {
        fprintf(print_fp, " Epoch-us     |  Tot-pkts   | New-pkts  |   PPS  | RV's | CBP %%\n");
    }

    fprintf(print_fp, "<%" PRIu64 "> | %4d | +%4d packets | %6.2f packets per second (PPS)|\
RV Count=%d | CBP=%3.0f%%\n",
        t_now_us,
        rx_count,
        delta,
        pps,
        active_RV_cnt,
        g_cbp_meas);
    fflush(print_fp);

    return;
}

/*
   print_rx_deltas  to allow exteral triggering of packet count printing
   at an interval (1 sec default) specified on commandline
*/
static void print_rx_deltas(void)
{
    int i;
    int delta;
    double delta_t, pps;
    static int prev_count = 0;
    static uint64_t prev_time = 0;
    static bool print_banner = true;
    uint64_t t_now_us = 0;
    int active_RV_cnt = 0;
    v2x_event_t recent_status;
    uint64_t age_usec;

    if (g_verbosity > 3) {
        recent_status = cv2x_status_poll(&age_usec);
        printf("|status=%dm %d ms ago|", recent_status, (int)(age_usec / 1000));
    }

    // Only bother with the RX reports, if actually receiving
    if (g_status == V2X_ACTIVE || g_status == V2X_TX_SUSPENDED) {
        t_now_us = timestamp_now();
        if (prev_time > 0) {
            delta = rx_count - prev_count;
            delta_t = t_now_us - prev_time;
            pps = ((double)delta * 1000000.0) / delta_t;
            if (g_verbosity || g_set_rx_report) {
                print_periodic_rx_report(stdout, print_banner,
                                         delta, active_RV_cnt, t_now_us, pps);
                print_banner = false;
            }
            prev_count = rx_count;
        }
        prev_time = t_now_us;
    }
}

/*
   Setup a timer with specified interval,
   and call function to print recevied packets periodically.
*/
void *periodic_reports_task(void *arg)
{
    int timerfd;
    uint64_t exp;
    ssize_t s;

    timerfd = cv2x_app_start_timer(g_reportInterval_ns);
    if ( timerfd < 0) {
        LOGE("cv2x_app_start_timer() failed\n");
        return NULL;
    }

    while (1) {
        print_rx_deltas();
        s = read(timerfd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            printf("periodic_reports_task error read from timerfd\n");
        }
    }

    close(timerfd);
    return NULL;
}

// return false if specified tx pool exceeds num of supported tx pools in CV2X capablities
static bool v2x_validate_tx_pools() {
    bool ret = true;
    pthread_mutex_lock(&g_capability_mutex);

    if (!g_capabilities_valid) {
        LOGI("capabilities invalid\n");
        pthread_mutex_unlock(&g_capability_mutex);
        return ret;
    }
    uint8_t pool_num = g_capabilities.tx_pool_ids_supported_len;
    for (uint8_t flow_id = 0; flow_id <= flow_cnt; ++flow_id) {
        if (g_tx_flow_content[flow_id].sps_flow_info.flow_info.tx_pool_id_valid &&
            (g_tx_flow_content[flow_id].sps_flow_info.flow_info.tx_pool_id >= pool_num)) {
            LOGW("Flow#%d tx pool id %d exceeds tx pool num %d!\n",
                 flow_id, g_tx_flow_content[flow_id].sps_flow_info.flow_info.tx_pool_id,
                 pool_num);
            ret = false;
            break;
        }
    }
    pthread_mutex_unlock(&g_capability_mutex);

    return ret;
}

static bool v2x_is_ready() {

    bool res = true;

    if (g_service_status == SERVICE_UNAVAILABLE) {
        res = false;
    } else {
        switch (test_mode) {
            case ModeTx:
                // for Tx mode, pause Tx if specified tx pool is not supported
                res = ((g_status == V2X_ACTIVE || g_status == V2X_RX_SUSPENDED)
                       && v2x_validate_tx_pools());
                break;
            case ModeRx:
                res = (g_status == V2X_ACTIVE || g_status == V2X_TX_SUSPENDED);
                break;
        }
    }

    return res;
}

/* State functions */
cv2x_app_state_t state_start() {
    int log_level;
    pthread_t tid;
    pthread_attr_t attr;

    install_signal_handler(termination_handler);

    if (g_verbosity == 0) {
        log_level = LOG_ERR;
    } else if (g_verbosity == 1) {
        log_level = LOG_NOTICE;
    } else if (g_verbosity == 2) {
        log_level = LOG_INFO;
    } else {
        log_level = LOG_DEBUG;
    }
    v2x_radio_set_log_level(log_level, g_use_syslog);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, terminate_cleanup_task, NULL);
    pthread_attr_destroy(&attr);

    if (pthread_mutex_init(&g_status_mutex, NULL)) {
        LOGE("Error initializing status mutex");
        errExit("initialize_mutex");
    }

    if (pthread_cond_init(&g_status_cv, NULL)) {
        LOGE("Error initializing status condition variable");
        errExit("initialize_condition_variable");
    }

    if (pthread_mutex_init(&g_radio_init_mutex, NULL)) {
        LOGE("Error initializing radio init mutex");
        errExit("initialize_mutex");
    }

    if (pthread_cond_init(&g_radio_init_cv, NULL)) {
        LOGE("Error initializing radio init condition variable");
        errExit("initialize_condition_variable");
    }

    return STATE_INITIALIZE;
}

void capability_listener(v2x_iface_capabilities_t* capability, void *ctx) {
    pthread_mutex_lock(&g_capability_mutex);
    if (!g_capabilities_valid ||
        memcmp(&g_capabilities, capability, sizeof(v2x_iface_capabilities_t))) {
        LOGI("V2X capabilities updated!\n");
        memcpy(&g_capabilities, capability, sizeof(v2x_iface_capabilities_t));
        g_capabilities_valid = true;
    }
    pthread_mutex_unlock(&g_capability_mutex);

    if (g_verbosity) {
        print_v2x_capabilities(capability);
    }
}

cv2x_app_state_t state_initialize() {
    int i;
    pthread_t tid;
    pthread_attr_t attr;
    // Just a dummy test context to make sure is maintained properly
    unsigned long  test_ctx = 0xfeedbeef;
    v2x_concurrency_sel_t mode = V2X_WWAN_NONCONCURRENT;
    v2x_api_ver_t  verinfo;
    static bool is_reinit = false;

    for (i = 0; i < PROXY_MAX_FLOWS; i++) {
        g_tx_flow_content[i].sps_sock = -1;
        g_tx_flow_content[i].event_sock = -1;
        g_tx_flow_content[i].tx_sock = -1;
    }

    pthread_mutex_lock(&g_radio_init_mutex);
    g_radio_init_status = V2X_STATUS_RADIO_PLACEHOLDER;
    pthread_mutex_unlock(&g_radio_init_mutex);

    radio_calls.v2x_radio_init_complete = init_complete;
    radio_calls.v2x_radio_status_listener = radio_listener;
    radio_calls.v2x_radio_chan_meas_listener =  meas_listener;
    radio_calls.v2x_service_status_listener = service_status_listener;
    radio_calls.v2x_radio_capabilities_listener = capability_listener;
    sps_function_calls.v2x_radio_sps_offset_changed = NULL;
    radio_calls.v2x_radio_l2_addr_changed_listener = NULL;

    verinfo = v2x_radio_api_version();
    LOGI("API Version#%d, built on <%s> @ %s \n", verinfo.version_num,
         verinfo.build_date_str,
         verinfo.build_time_str);

    if (test_mode != ModeTx) {
        if (g_verbosity > 2) {
            LOGI("Skipping set-up of tx interval timer, since RX or Echo mode.\n");
        }

        if (g_reportInterval_ns && !g_report_thr_valid) {
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            if (pthread_create(&tid, &attr, periodic_reports_task, NULL)){
                LOGE("Failed to create periodic reports thread\n");
                exit(2);
            }
            pthread_attr_destroy(&attr);
            g_report_thr_valid = true;
        }
    }
    if (do_air2net_proxy) {
        init_air2net_info();
    } else if (do_net2air_proxy) {
        init_net2air_info();
    } else {
        return STATE_DEINITIALIZE;
    }
    int res = v2x_radio_init_v3(mode, &radio_calls, &test_ctx,
        &handles[TRAFFIC_IP], &handles[TRAFFIC_NON_IP]);
    if (res != 0) {
        if (is_reinit == false) {
            LOGE("Failed to initalize radio in cv2x inactive state, bail\n");
            exit(2);
        } else if (v2x_is_ready()) {
            LOGE("Failed to re-initalize radio in cv2x active/suspend state, retry\n");
            usleep(READ_SOCKET_TIMEOUT_US);
            return STATE_INITIALIZE;
        } else {
            LOGE("Failed to re-initalize radio in cv2x nonactive state, pause\n");
            return STATE_PAUSE;
        }
    }
    g_service_status = SERVICE_AVAILABLE;

    pthread_mutex_lock(&g_radio_init_mutex);
    while (V2X_STATUS_RADIO_PLACEHOLDER == g_radio_init_status) {
        printf("Waiting on radio init_complete callback\n");
        pthread_cond_wait(&g_radio_init_cv, &g_radio_init_mutex);
    }
    pthread_mutex_unlock(&g_radio_init_mutex);

    // If radio isn't ready due to v2x status wait on status update
    if (V2X_STATUS_RADIO_NOT_READY == g_radio_init_status) {
        g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = true;
        return STATE_PAUSE;
    }

    radio_query_and_print_param();
    if (test_mode == ModeTx) {
        v2x_test_param_check(&g_capabilities);
    }

    if (cla_portnum_set && test_mode != ModeTx) {
        v2x_set_rx_port(cla_portnum);
    }

    is_reinit = true;

    pthread_mutex_lock(&g_status_mutex);
    if (!v2x_is_ready()) {
        pthread_mutex_unlock(&g_status_mutex);
        return STATE_PAUSE;
    }
    // This gets set on the initial status callback
    // How will we recover if SSR or EVENT occurs on initialization
    g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE] = false;
    g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = false;
    pthread_mutex_unlock(&g_status_mutex);

    return STATE_SETUP_FLOWS;
}

cv2x_app_state_t state_receive() {
    int rc = 0;

    pthread_mutex_lock(&g_status_mutex);
    if (!v2x_is_ready() || g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE]) {
        pthread_mutex_unlock(&g_status_mutex);
        return STATE_PAUSE;
    }
    pthread_mutex_unlock(&g_status_mutex);

    rc = air2net_forward(air2net_control.rx_sockfd);

    if (rc > 0 && g_tx_flow_content[0].qty > 0) {
        // Only reduce qty if sample_rx wasn't timed out.
        g_tx_flow_content[0].qty--;
    }

    if (g_tx_flow_content[0].qty == 0) {
        return STATE_DEINITIALIZE;
    }

    return STATE_RECEIVE;
}

cv2x_app_state_t state_transmit(int flow_id) {
    int ret = 1;
    if (!v2x_is_ready() || g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE]) {
        return STATE_PAUSE;
    }
    ret = net2air_forward(flow_id);

    return ret;
}

/* Pause state handles SSR and V2X Status change logic
 * Only primary flow take care of close sockets and reset flags */
cv2x_app_state_t state_pause(bool primary_flow) {
    cv2x_app_state_t res = mode_to_state[test_mode];
    printf("V2X not ready, pausing actions.\n");
    bool close_socks = false;

    {
        pthread_mutex_lock(&g_status_mutex);
        /*If cv2x status inactive, primary task close all the socks to avoid fds leak*/
        if (primary_flow && g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE]) {
           close_socks = true;
        }
        pthread_mutex_unlock(&g_status_mutex);
    }
    if (close_socks) {
        pthread_mutex_lock(&g_sock_mutex);
        close_all_v2x_sockets();
        pthread_mutex_unlock(&g_sock_mutex);
    }

    pthread_mutex_lock(&g_status_mutex);
    while (!v2x_is_ready() && !g_terminate) {
        pthread_cond_wait(&g_status_cv, &g_status_mutex);

        // if status goes to inactive, need to set capabilities to invalid,
        // otherwise it might get stuck in pause state because the former
        // cv2x capabilites are being used for checking cv2x readiness
        if (g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE]) {
            pthread_mutex_lock(&g_capability_mutex);
            g_capabilities_valid = false;
            pthread_mutex_unlock(&g_capability_mutex);
        }
    }

    if (g_terminate) {
        pthread_mutex_unlock(&g_status_mutex);
        return STATE_DEINITIALIZE;
    }

    if (primary_flow) {
       /* Clear event flags for service status up and status transition,
        *In multple flows scenarios, only 1 task shall take care of the flags,
        *otherwise other tasks may get incorrect events if reset by others*/
       if (g_event_flags[EVENT_SERVICE_STATUS_UP]) {
           g_event_flags[EVENT_SERVICE_STATUS_UP] = false;
           g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE] = false;
           g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = false;
           res = STATE_INITIALIZE;
        } else if (g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE] ||
               g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE]) {
            g_event_flags[EVENT_STATUS_TRANSITION_TO_ACTIVE] = false;
            g_event_flags[EVENT_STATUS_TRANSITION_TO_INACTIVE] = false;
            res = STATE_INITIALIZE;
        }
    }
    pthread_mutex_unlock(&g_status_mutex);

    printf("V2X ready, resuming actions.\n");
    return res;
}

cv2x_app_state_t state_deinitialize() {
    g_terminate = 1;
    termination_cleanup();
    return STATE_EXIT;
}

static void log_state_transition(cv2x_app_state_t curr, cv2x_app_state_t next) {
    if (curr != next) {
        LOGI("Transitioning from state %s to %s\n",
              get_state_strings(curr),
              get_state_strings(next));
    }
}

void add_default_flow(int idx)
{
    memset(&air2net_control, 0, sizeof(air2net_control));
    g_strlcpy(air2net_control.if_name, DEFAULT_IF_NAME,
              sizeof(air2net_control.if_name));
    g_strlcpy(air2net_control.remote_net_address, DEFAULT_IP_ADDR,
              sizeof(air2net_control.remote_net_address));
    air2net_control.air2net_forward_to_port = 2499;
    air2net_control.rx_sockfd = -1;
    air2net_control.forward_sockfd = -1;
    air2net_control.rt_flag = false;
    air2net_control.remove_radiotap_family = false;
    air2net_control.family = 0;

    memset(&net2air_control, 0, sizeof(net2air_control));
#ifdef SIM_BUILD
    g_strlcpy(net2air_control.iface, DEFAULT_LOOPBACK_IFANCE_NAME,
              sizeof(net2air_control.iface));
#endif
    g_strlcpy(net2air_control.dest_ipv6_addr_str, DEFAULT_MUL_ADDR,
              sizeof(net2air_control.dest_ipv6_addr_str));
    net2air_control.net2air_listen_port = 2498;
    net2air_control.listen_net_socket = -1;
    net2air_control.radiotap_family_id = 1;

    if ((idx >= 0) && (idx < PROXY_MAX_FLOWS)) {
        g_tx_flow_content[idx].ip_type = TRAFFIC_NON_IP;
        // Must be 1, 2, 3, 4 for default config in most pre-configured units
        g_tx_flow_content[idx].sps_flow_info.reservation.v2xid = DEFAULT_SERVICE_ID;
        // 2 = lowest priority supported in TQ.
        g_tx_flow_content[idx].sps_flow_info.reservation.priority = V2X_PRIO_2;
        // Invalid Periodicity signaling to use Interval if not specified
        g_tx_flow_content[idx].sps_flow_info.reservation.period_interval_ms = -1;
        g_tx_flow_content[idx].sps_flow_info.reservation.tx_reservation_size_bytes = -1;

        g_tx_flow_content[idx].sps_flow_info.flow_info.default_tx_power_valid = false;
        g_tx_flow_content[idx].sps_flow_info.flow_info.mcs_index_valid = false;
        g_tx_flow_content[idx].sps_flow_info.flow_info.retransmit_policy =
                V2X_AUTO_RETRANSMIT_DONT_CARE;
        g_tx_flow_content[idx].sps_flow_info.flow_info.tx_pool_id_valid = false;
        g_tx_flow_content[idx].sps_flow_info.flow_info.tx_pool_id = 0;
        // By default we register an event with each SPS
        g_tx_flow_content[idx].flow_type = ComboReg;
        g_tx_flow_content[idx].sps_port = SAMPLE_SPS_PORT + idx;
        g_tx_flow_content[idx].evt_port = SAMPLE_EVENT_PORT + idx;
        /* Timer interval, used for TX packet timing, derrived from reservation */
        g_tx_flow_content[idx].interval_btw_pkts_ns = DEFAULT_PERIODICITY_MS * 1000000;
        g_tx_flow_content[idx].qty = -1;
        g_tx_flow_content[idx].sps_event_opt = 0;
        g_tx_flow_content[idx].tx_buf = NULL;
        g_tx_flow_content[idx].mean_payload_len = 0;
    } else {
        LOGE("Max number of compile time supported flows exceeded.\n");
        exit(-1);
    }
}

int proxy2air_setup_flows() {
    int i = 0;
    int f = 0;
    int socknum = -1;
    int esocknum = -1;
    int traffic_class = -1;
    static bool been_success = false;
    struct sockaddr_in6 sockaddr = {0};
    struct sockaddr_in6 esockaddr = {0};
    v2x_radio_handle_t hndl;
    cv2x_app_state_t ret = mode_to_state[test_mode];

    for (f = 0; f <= flow_cnt; f++) {
        socknum  = -1;
        esocknum = -1;
        hndl = g_tx_flow_content[f].ip_type == TRAFFIC_IP ?
                handles[TRAFFIC_IP] : handles[TRAFFIC_NON_IP];
        // get interface name if we are not using loopback testing
#ifndef SIM_BUILD
        if (V2X_STATUS_SUCCESS != get_iface_name(g_tx_flow_content[f].ip_type,
                                                 net2air_control.iface, IFNAMSIZ)) {
            return STATE_DEINITIALIZE;
        }
#endif
        traffic_class = v2x_convert_priority_to_traffic_class(
                            g_tx_flow_content[f].sps_flow_info.reservation.priority);
        if (g_verbosity) {
            fprintf(stdout, "# traffic_class=%d\n", traffic_class);
        }
        /* Set destination information */
        inet_pton(AF_INET6,
                  net2air_control.dest_ipv6_addr_str,
                  (void *)&g_tx_flow_content[f].dest_sockaddr.sin6_addr);
        g_tx_flow_content[f].dest_sockaddr.sin6_family = AF_INET6;
        g_tx_flow_content[f].dest_sockaddr.sin6_scope_id = if_nametoindex(net2air_control.iface);

        if (0 != pthread_mutex_trylock(&g_sock_mutex)) {
            return STATE_DEINITIALIZE;
        }
        switch (g_tx_flow_content[f].flow_type) {
            case EventOnly:
                if (v2x_radio_tx_event_sock_create_and_bind_v3(g_tx_flow_content[f].ip_type,
                                   g_tx_flow_content[f].sps_flow_info.reservation.v2xid,
                                   g_tx_flow_content[f].evt_port,
                                   &g_tx_flow_content[f].sps_flow_info.flow_info,
                                   &esockaddr,
                                   &esocknum) != V2X_STATUS_SUCCESS) {
                    LOGE("Error creating Event socket\n");
                    ret = STATE_DEINITIALIZE;
                }
                g_tx_flow_content[f].event_sock = esocknum;
                g_tx_flow_content[f].event_sockaddr = esockaddr;
                break;
            case SpsOnly:
                if (v2x_radio_tx_sps_sock_create_and_bind_v2(hndl,
                                                             &g_tx_flow_content[f].sps_flow_info,
                                                             &sps_function_calls,
                                                             g_tx_flow_content[f].sps_port,
                                                             -1,
                                                             &socknum,
                                                             &sockaddr,
                                                             NULL,
                                                             NULL) != V2X_STATUS_SUCCESS) {
                    fprintf(stderr, "Error creating SPS socket\n");
                    ret = STATE_DEINITIALIZE;
                }
                g_tx_flow_content[f].sps_sock = socknum;
                g_tx_flow_content[f].sps_sockaddr = sockaddr; //Shallow struc copy
                break;
            case ComboReg:
                if (v2x_radio_tx_sps_sock_create_and_bind_v2(hndl,
                                                             &g_tx_flow_content[f].sps_flow_info,
                                                             &sps_function_calls,
                                                             g_tx_flow_content[f].sps_port,
                                                             g_tx_flow_content[f].evt_port,
                                                             &socknum,
                                                             &sockaddr,
                                                             &esocknum,
                                                             &esockaddr) != V2X_STATUS_SUCCESS) {
                    fprintf(stderr, "Error creating SPS socket\n");
                    ret = STATE_DEINITIALIZE;
                }
                g_tx_flow_content[f].sps_sock = socknum;
                g_tx_flow_content[f].sps_sockaddr = sockaddr;
                g_tx_flow_content[f].event_sock = esocknum;
                g_tx_flow_content[f].event_sockaddr = esockaddr;
                break;
            case NoReg:
            default:
                fprintf(stderr, "WARNING:  unprocessed TX Flow Type");
        }

        pthread_mutex_unlock(&g_sock_mutex);
        if (STATE_DEINITIALIZE == ret) {
            break;
        }
        if (esocknum >= 0) {
            if (setsockopt(esocknum,
                           IPPROTO_IPV6,
                           IPV6_TCLASS, (void *)&traffic_class, sizeof(traffic_class)) < 0) {
                    fprintf(stderr, "setsockopt(IPV6_TCLASS) on event socket failed err=%d\n",
                            errno);
                }
            if (g_verbosity) {
                printf("Setup traffic class=%d on the event socket completed.\n", traffic_class);
            }
        }
        if (socknum >= 0) {
            if (setsockopt(socknum,
                           IPPROTO_IPV6,
                           IPV6_TCLASS, (void *)&traffic_class, sizeof(traffic_class)) < 0) {
                fprintf(stderr, "setsockopt(IPV6_TCLASS) on SPS socket failed err=%d\n", errno);
            }
            if (g_verbosity) {
                printf("Setup traffic class =%d completed the SPS flow socket.\n", traffic_class);
            }
        }
        g_tx_flow_content[0].tx_buf = buf;

        for (i = 0; STATE_DEINITIALIZE != ret && i <= flow_cnt; i++) {
            int tx_buf_size = 0;
            printf("Flow#%d: type=%d %d %d sps_port=%d evt_port=%d, %d ms, %d bytes\n",
                   i,
                   g_tx_flow_content[i].flow_type,
                   g_tx_flow_content[i].sps_sock,
                   g_tx_flow_content[i].event_sock,
                   g_tx_flow_content[i].sps_port,
                   g_tx_flow_content[i].evt_port,
                   g_tx_flow_content[i].sps_flow_info.reservation.period_interval_ms,
                   g_tx_flow_content[i].sps_flow_info.reservation.tx_reservation_size_bytes
                  );

            if ((g_tx_flow_content[i].flow_type == EventOnly) ||
                    g_tx_flow_content[i].sps_event_opt) {
                g_tx_flow_content[i].tx_sock = g_tx_flow_content[i].event_sock;
                g_tx_flow_content[i].dest_sockaddr.sin6_port =
                    htons((uint16_t)g_tx_flow_content[i].evt_port);
            } else {
                g_tx_flow_content[i].tx_sock = g_tx_flow_content[i].sps_sock;
                g_tx_flow_content[i].dest_sockaddr.sin6_port =
                    htons((uint16_t)g_tx_flow_content[i].sps_port);
            }
        }
    }
    if (ret == STATE_DEINITIALIZE) {
        if (been_success) {
            /*If ever been success, we have confidence the setup is good;
             *This case if failure occur due to status changed to inactive or modem SSR,
             *we do not have to bail out, simply wait cv2x status back to active*/
            if (!v2x_is_ready()) {
                ret = STATE_PAUSE;
            }
        }
    } else {
        been_success = true;
    }
    return ret;
}

int air2proxy_setup_flows()
{
    int esocknum = -1;
    static bool been_success = false;
    // get Rx port number, default is 9000
    uint16_t rx_port = V2X_RX_WILDCARD_PORTNUM;
    v2x_radio_handle_t hndl;
    cv2x_app_state_t ret = mode_to_state[test_mode];
    proxy_flow_info_t* rx_info = &g_tx_flow_content[0];
    // Adding 100ms timeout for SSR support. Remove the possibility for indefinite wait
    struct timeval tv;
    struct sockaddr_in6 esockaddr = {0};
    tv.tv_sec = 0;
    tv.tv_usec = READ_SOCKET_TIMEOUT_US;

    if (cla_portnum_set) {
        rx_port = cla_portnum;
    }
    if (0 != pthread_mutex_trylock(&g_sock_mutex)) {
        return STATE_DEINITIALIZE;
    }
    // To subscribe a list of service IDs, an Tx event flow must be pre-created with the first
    // service ID in the service ID list and the same port number as the Rx port. In this way,
    // multiple APPs can Rx with different service IDs on different port number.
    // If no service ID specified when Rx, wildcard will be enabled, all Rx traffic will be
    // directed to the wildcard port number.
    if (g_sid_list_len > 0) {
        // set the same Tx port as Rx port
        rx_info->evt_port = rx_port;
        // set the Tx flow servcie ID to the first service ID in the Rx service ID list
        if (v2x_radio_tx_event_sock_create_and_bind_v3(rx_info->ip_type,
                                                   g_sid_list[0],
                                                   rx_info->evt_port,
                                                   &rx_info->sps_flow_info.flow_info,
                                                   &esockaddr,
                                                   &esocknum) != V2X_STATUS_SUCCESS) {
            LOGE("Error creating event flow for specific SID subscription.\n");
            ret = STATE_DEINITIALIZE;
        }
        rx_info->event_sock = esocknum;
        rx_info->event_sockaddr = esockaddr;
    }
    hndl = rx_info->ip_type == TRAFFIC_IP ? handles[TRAFFIC_IP] : handles[TRAFFIC_NON_IP];
    // subscribe specific service IDs or wildcard
    if (v2x_radio_rx_sock_create_and_bind_v3(hndl,
                                             rx_port,
                                             g_sid_list_len,
                                             g_sid_list,
                                             &air2net_control.rx_sockfd,
                                             &air2net_control.rx_sockaddr)) {
        fprintf(stderr, "Error creating RX socket");
        ret = STATE_DEINITIALIZE;
    }
    pthread_mutex_unlock(&g_sock_mutex);

    if (ret != STATE_DEINITIALIZE) {
        if (setsockopt(air2net_control.rx_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            fprintf(stderr, "Setsockopt(SO_RCVTIMEO) failed\n");
            ret = STATE_DEINITIALIZE;
        } else {
            // enable recv traffic class
            int flag = 1;
            if (setsockopt(air2net_control.rx_sockfd,
                           IPPROTO_IPV6, IPV6_RECVTCLASS, &flag, sizeof(flag)) < 0) {
                fprintf(stderr, "Setsockopt(IPV6_RECVTCLASS) failed\n");
                ret = STATE_DEINITIALIZE;
            }
            int val = 1;
            if (setsockopt(air2net_control.rx_sockfd,
                           IPPROTO_IPV6, IPV6_RECVERR, &val, sizeof(flag)) < 0) {
                fprintf(stderr, "Setsockopt(IPV6_RECVERR) failed\n");
                ret = STATE_DEINITIALIZE;
            }
        }
    }
    if (ret == STATE_DEINITIALIZE) {
        if (been_success) {
            /*If ever been success, we have confidence the setup is good;
             *This case if failure occur due to status changed to inactive or modem SSR,
             *we do not have to bail out, simply wait cv2x status back to active*/
            if (!v2x_is_ready()) {
                ret = STATE_PAUSE;
            }
        }
    } else {
        been_success = true;
    }
    return ret;
}

int net2air_forward(int flow_id) {
    int rc;
    int flags = 0;
    int len = sizeof(buf);
    bool txRes = true;

    rc = recv(net2air_control.listen_net_socket, buf, len, flags);
    if (rc > 0) {
        if (sample_tx(0, rc) <= 0) {
            txRes = false;
        }
    }

    if (txRes == false || g_tx_flow_content[flow_id].qty == -1) {
        return STATE_TRANSMIT;
    } else if (g_tx_flow_content[flow_id].qty > 1) {
        g_tx_flow_content[flow_id].qty --;
        return STATE_TRANSMIT;
    } else if (g_tx_flow_content[flow_id].qty == 1) {
        g_tx_flow_content[flow_id].qty --;
        if (g_verbosity) {
            printf("Flow %d transmission completed\n", flow_id);
        }
    }
    return STATE_DEINITIALIZE;
}

int air2net_forward(int rx_sock) {
    int rc = 0;
    int bytes_proxied = 0;
    int len = sizeof(buf);
    unsigned char *buf_p;
    // create new buf for radiotap
    char buf_rt[MAX_DUMMY_PACKET_LEN];
    char control[CMSG_SPACE(sizeof(int))];
    struct sockaddr_in6 from;
    struct msghdr message = {0};
    struct msghdr forward_msg = {0};
    struct iovec iov[1] = {0};
    struct iovec forward_iov[1] = {0};
    socklen_t fromLen = sizeof(from);

    if (g_verbosity > 1) {
        printf("RX...");
    }
    if (rx_sock <= 0) {
        printf("rx_sock is failed : %d", rx_sock);
        return -1;
    }

    iov[0].iov_base = buf;
    iov[0].iov_len = len;
    message.msg_name = &from;
    message.msg_namelen = fromLen;
    message.msg_iov = iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    rc = recvmsg(rx_sock, &message, 0);
    if (rc < 0) {
        /* EAGAIN and EWOULDBLOCK are possible return values when Rx socket has timed out.
         This can be an indication that cv2x status has changed and we need to recheck v2x status
         before continuing to wait for a message over the socket.
         EINTR is another possible return value when the L2 ID changed, which indicate this
         system call was interrupted.*/
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOGE("Error receiving message! errno %d\n", errno);
        }
        goto bail;
    }

    buf_p = buf;
    rx_count++;
    if(dump_opt) {
        print_buffer(buf_p, rc);
    }

    /* We will be prepending radiotap header
     * to this proxied packet if flag is true
    */
    if(air2net_control.rt_flag){ /* START RADIOTAP HEADER ADD */
        // we will need to get the source mac address L2 address-6 bytes/48bits
        // this will be added to the 802.11 header
        uint32_t src_id = from.sin6_addr.s6_addr32[3];

        // make sure the edp is set properly according to etsi or sae
        // here we need to check the packet to see if it is etsi or sae/wsmp
        unsigned char edp_type = buf[0];
        printf("The first byte of this packet (fam id) is : %02x\n", edp_type);
        if(edp_type == 0x01){
            edp_hdr = EPD_ETHER_TYPE_WSMP;
        }
        if(edp_type == 0x00){
            edp_hdr = EPD_ETHER_TYPE_ETSI;
        }
        create_radiotap_header(edp_hdr, src_id, rc);

        // add radiotap_message_hdr (including radiotap, 80211, edp)
        uint32_t serial_bytes = serialize_radiotap_message(buf_rt);

        // remove family id flag check
        if(air2net_control.remove_radiotap_family){
            rc -= 1;
            memcpy(buf_rt + serial_bytes, (buf + 1), rc-1);
            printf("Removing family ID byte from orig packet\n");
        }else{
            memcpy(buf_rt + serial_bytes, buf, rc);
            printf("Not removing family ID byte from orig packet\n");
        }

        buf_p = buf_rt;
        rc = serial_bytes + rc;
        /* END RADIOTAP HEADER ADD */
    }
    if (!air2net_control.rt_flag) {
        // remove family id flag check
        if(air2net_control.remove_radiotap_family){
            rc = rc-1;
            buf_p += 1;
            printf("Removing family ID byte from orig packet\n");
        }
    }
    if (g_verbosity > 1) {
        printf("Air2Net Proxy: FINAL proxied message with radiotap of size %d is: \n", rc);
        print_buffer(buf_p, rc);
    }
    switch (air2net_control.family) {
        case AF_INET:
            forward_msg.msg_name = &air2net_control.remote_sin4;
            forward_msg.msg_namelen = sizeof(air2net_control.remote_sin4);
            break;
        case AF_INET6:
            forward_msg.msg_name = &air2net_control.remote_sin;
            forward_msg.msg_namelen = sizeof(air2net_control.remote_sin);
            break;
    }
    // Following is for Normal case
    forward_iov[0].iov_base = buf_p;
    forward_iov[0].iov_len = rc;
    // Storing the buf and rc into the msg object for ipv6 sending
    forward_msg.msg_iov = forward_iov;
    forward_msg.msg_iovlen = 1;
    forward_msg.msg_control = 0;
    forward_msg.msg_controllen = 0;
    /* On MDM9150, SA415M,  We have to connect to tcp kinematics server and ask server
     * to send packet locally to avoid any UDP header or
     * Ethernet header issues.
     * Important to not use the same port as kinematics, and to use 127.0.0.1.
     * Above comment NOT applicable to SA515M
     */
    /* Send data using sendmsg to provide IPV6_TCLASS per packet */
    bytes_proxied = sendmsg(air2net_control.forward_sockfd, &forward_msg, 0);
    if (bytes_proxied < 0) {
        LOGE("Air2Net Proxy: error on air2net sendmsg() error is %d \n", errno);
    } else {
        printf("bytes_proxied is %d\n", bytes_proxied);
    }
bail:
    return rc;
}

void init_net2air_info()
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = READ_SOCKET_TIMEOUT_US;

    memset(&net2air_control.proxy_listen_sin, 0, sizeof(struct sockaddr_in6));
    net2air_control.proxy_listen_sin.sin6_addr = in6addr_any;
    net2air_control.proxy_listen_sin.sin6_family = AF_INET6;
    net2air_control.proxy_listen_sin.sin6_scope_id = 0;
    net2air_control.proxy_listen_sin.sin6_port =
                                        htons((uint16_t)net2air_control.net2air_listen_port);

    if (flow_cnt) {
        LOGE("Unsupported Network-to-Air proxy mode for multiple tx flows.\n");
        exit(0);
    }
    net2air_control.listen_net_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (net2air_control.listen_net_socket < 0 ) {
        fprintf(stderr, "%s() cannot create socket", __FUNCTION__);
        exit(3);
    }
    if (bind(net2air_control.listen_net_socket,
             (struct sockaddr *)&net2air_control.proxy_listen_sin,
             sizeof(struct sockaddr_in6)) < 0) {
        fprintf(stderr, "Error binding proxy listen socket,  err = %s\n", strerror(errno));
        exit(3);
    }
    if (setsockopt(net2air_control.listen_net_socket,
                   SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        fprintf(stderr, "Setsockopt(SO_RCVTIMEO) failed\n");
        exit(3);
    }
}

void init_air2net_info()
{
    int family;
    struct addrinfo hint;
    struct addrinfo *details_p = NULL;

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_NUMERICHOST;
    hint.ai_family = PF_UNSPEC;

    if (getaddrinfo(air2net_control.remote_net_address, NULL, &hint, &details_p)) {
        fprintf(stderr, "Bad Air2net remote IP address %s.\n",
                air2net_control.remote_net_address);
        exit(3);
    }
    family = details_p->ai_family;
    air2net_control.forward_sockfd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (air2net_control.forward_sockfd < 0) {
        fprintf(stderr, "Proxy to network requested, socket failed.\n");
        exit(3);
    }
    air2net_control.family = family;
    /* vehicle => network */
    switch (family) {
    case AF_INET:
        printf("Destination address is IPv4\n");
        memset((void *)&air2net_control.remote_sin4, 0, sizeof(air2net_control.remote_sin4));
        air2net_control.remote_sin4.sin_family = family;
        air2net_control.remote_sin4.sin_addr.s_addr = htonl(INADDR_ANY);
        air2net_control.remote_sin4.sin_port =
            htons((uint16_t)air2net_control.air2net_forward_to_port);

        if (setsockopt(air2net_control.forward_sockfd,
                       SOL_SOCKET, SO_BINDTODEVICE,
                       (void *)air2net_control.if_name,
                       strlen(air2net_control.if_name)) < 0) {
            fprintf(stderr, "Error setting socket options, err = %s\n", strerror(errno));
            exit(0);
        }
        /* Sending received packets to: proxy_ip_addr air2net_control.air2net_forward_to_port */
        if (bind(air2net_control.forward_sockfd,
                 (struct sockaddr *)&air2net_control.remote_sin4,
                 sizeof(air2net_control.remote_sin4)) < 0) {
            fprintf(stderr, "Error binding sps socket, err = %s\n", strerror(errno));
        }
        printf("SPS Socket setup success fd=%d, port=%d\n",
                air2net_control.forward_sockfd,
                ntohs(air2net_control.remote_sin.sin6_port));

        inet_pton(family, air2net_control.remote_net_address,
                  &air2net_control.remote_sin4.sin_addr.s_addr);
        printf("Successfully setup default destination for sps socket | Errno = %d\n", errno);
        break;
    case AF_INET6:
        printf("Destination address is IPv6\n");
        memset((void *)&air2net_control.remote_sin, 0, sizeof(air2net_control.remote_sin));
        air2net_control.remote_sin.sin6_addr = in6addr_any;
        air2net_control.remote_sin.sin6_port =
            htons((uint16_t)air2net_control.air2net_forward_to_port);
        air2net_control.remote_sin.sin6_family = AF_INET6;
        air2net_control.remote_sin.sin6_scope_id = if_nametoindex(air2net_control.if_name);

        if (setsockopt(air2net_control.forward_sockfd,
                       SOL_SOCKET,
                       SO_BINDTODEVICE,
                       (void *)air2net_control.if_name, strlen(air2net_control.if_name)) < 0) {
            fprintf(stderr, "Error setting socket options, err = %s\n", strerror(errno));
            exit(0);
        }
        /* Sending received packets to: proxy_ip_addr air2net_control.air2net_forward_to_port */
        if (bind(air2net_control.forward_sockfd,
                 (struct sockaddr *)&air2net_control.remote_sin,
                 sizeof(struct sockaddr_in6)) < 0) {
            fprintf(stderr, "Error binding sps socket, err = %s\n", strerror(errno));
        }
        printf("SPS Socket setup success fd=%d, port=%d\n",
                air2net_control.forward_sockfd, ntohs(air2net_control.remote_sin.sin6_port));

        inet_pton(family, air2net_control.remote_net_address,
                  &air2net_control.remote_sin.sin6_addr);
        LOGI("Successfully setup default destination for sps socket | Errno = %d\n", errno);
        break;
    default:
        LOGE("WARNING. address family of specified address not recognized.\n");
    }
}

static void print_usage(const char *prog_name)
{
    printf("Usage: %s ... \n", prog_name);
    printf("\t A cv2x test tool is used for data forwarding.\n");
    printf("\t Can also optionally be used as a UDP -> PC5 proxy or vice-versa. \n");
    printf("\t air2net options: \n");
    printf("\t-b<interval sec>  Reporting interval for receive stats.");
    printf(" set to 0 to disable, use default interval if not specified\n");
    printf("\t-B AIR2NET Proxy will contain radiotap for Wireshark\n ");
    printf("\t-C NET2AIR Proxy option to remove Family ID byte of received packet\n");
    printf("\t-p <RX port>    RX port number[Default=%d]\n", (int)V2X_RX_WILDCARD_PORTNUM);
    printf("\t-R<SID 1>,<SID 2>...   air2net mode, default is net2air.\n");
    printf("\t                       If service IDs are specified, will only Rx packets with\n");
    printf("\t                       the specified service IDs.\n");
    printf("\t                       If service IDs are not specified, wildcard will be used\n");
    printf("\t                       and will Rx all service IDs, default is wildcard.\n");
    printf("\t-x <interface> Proxy interface name.\n");
    printf("\t-X <IP addr>   Enable PC5 -> UDP forwarding using destination IP: <IP addr>\n");
    printf("\t-Y <dest port> Enable PC5 -> UDP forwarding using destination port: <dest port>\n");
    printf("\n");

    printf("\t net2air options: \n");
    printf("\t-c<FAMILY_ID> AIR2NET Proxy option to add Family ID byte");
    printf(" specified by user (either 00 or 01)\n");
    printf("\t-D <dest ip6>   Destination IPV6 address[ Default=%s]\n",
                              net2air_control.dest_ipv6_addr_str);
    printf("\t-E              Event flow mode instead of SPS\n");
    printf("\t-F              Use event port of SPS flow registration\n");
    printf("\t-L <port>       Enable UDP -> PC5 forwarding and listen on <port>,");
    printf(" and not support the multiple tx flows\n");
    printf("\t-O              Setup TX sps flow only without creating Tx event flow\n");
    printf("\t-P <V2X ID>     V2X session ID to be used\n");
    printf("\t-s <sps_port>   TX SPS port#. Overrides the default of %d.\n", SAMPLE_SPS_PORT);
    printf("\t-t <evt_port>   TX Event driven port#. [default=%d]\n", SAMPLE_EVENT_PORT);
    printf("\n");

    printf("\t common options: \n");
    printf("\t-i ip type, defalut is TRAFFIC_NON_IP \n");
    printf("\t-d              dump raw packet\n");
    printf("\t-V              output level, the larger the value,");
    printf(" the more content will be output \n");
    printf("\t-k <qty>        Quit after <qty> packets received or transmitted\n");
    printf("\t-Z              Use syslog instead of stdout for SDK debug/info/errors\n");
    printf("\n");

    printf("Examples: \n");
    printf("\t This UDP Air2Net proxy command will receive port 9001 date and");
    printf(" reforward the messages caught OTA to the device \n");
    printf("\t                  with IP address 10.150.34.14 via network interface eth0 and port 9000. \n");
    printf("\t option -B will make proxy encapsulate it with a RadioTap header with SAE EDP (all by default)\n");
    printf("\t %s -R -B -x eth0 -X 10.150.34.14 -Y 9000 -p 9001\n", prog_name);
    printf("\t This will be a UDP Air2Net Proxy which");
    printf(" will remove the first byte / family ID that is in the original packet.\n");
    printf("\t %s -R -C -x eth0 -X 10.150.34.14 -Y 9000\n", prog_name);
    printf("\t This will be a UDP Net2Air Proxy which ");
    printf(" will add the first byte / family ID that is in the original packet.\n");
    printf("\t %s -c 1 -L 9000 \n", prog_name);

}

static void parse_opts(int argc, char *argv[])
{
    int c;
    add_default_flow(flow_cnt);
    while ((c = getopt(argc, argv, "?b::Bc::CdD:EFhik:L:Op:P:R::s:t:v:Vx:X:Y:Z")) != -1) {
        switch (c) {
            case 'b':
                g_set_rx_report = true;
                if (optarg) {
                    gd_reportInterval_sec = atof(optarg);
                    g_reportInterval_ns = (double)gd_reportInterval_sec * 1000000000.0;
                }
                if (g_verbosity) {
                    if (g_reportInterval_ns) {
                        printf("\tPeriodic reporting every %f sec \n", gd_reportInterval_sec);
                    } else {
                        printf("\tPeriodic packet count reporting disabled\n");
                    }
                }
                break;
            case 'B':
                // by default, the packets will be Radiotap +
                // EDP header will be based on the received packet family ID
                printf("AIR2NET Proxy will contain radiotap header\n");
                air2net_control.remove_radiotap_family = true;
                air2net_control.rt_flag = true;
                break;
            case 'c':
                printf("NET2AIR Proxy: Add new family byte on proxied packet\n");
                net2air_control.add_radiotap_family = true;
                if (optarg) {
                    // the byte that user passes
                    net2air_control.radiotap_family_id = (uint8_t)atoi(optarg);
                }
                break;
                // Family byte strip or addition
            case 'C': // family byte strip
                printf("AIR2NET Proxy: ");
                printf("Stripping family byte (first byte) of the original packet\n");
                air2net_control.remove_radiotap_family = true;
                break;
            case 'd':
                dump_opt = 1;
                break;
            case 'D':
                if (optarg) {
                    g_strlcpy(net2air_control.dest_ipv6_addr_str, optarg,
                              sizeof(net2air_control.dest_ipv6_addr_str));
                    printf("\tDesination IPV6 address set to %s\n",
                            net2air_control.dest_ipv6_addr_str);
                }
                break;
            case 'E':
                g_tx_flow_content[flow_cnt].flow_type = EventOnly;
                if (g_verbosity) {
                    printf("Event only flow\n");
                }
                break;
            case 'F':
                g_tx_flow_content[flow_cnt].sps_event_opt = 1;
                if (g_verbosity) {
                    printf("Event port #%d of SPS flow registration used for all TX\n",
                            g_tx_flow_content[flow_cnt].evt_port);
                }
                break;
            case 'i':
                g_tx_flow_content[flow_cnt].ip_type = TRAFFIC_IP;
                break;
            case 'k':
                if (optarg) {
                    g_tx_flow_content[flow_cnt].qty = atoi(optarg);
                    if (g_tx_flow_content[flow_cnt].qty <= 0) {
                        printf(" Invalid Tx quantity: %d\n", g_tx_flow_content[flow_cnt].qty);
                        exit(0);
                    }
                    if (g_verbosity) {
                        printf("Will exit after %d packets processed (TX or RX)\n",
                            g_tx_flow_content[flow_cnt].qty);
                    }
                }
                break;
            case 'L':
                if (optarg) {
                    net2air_control.net2air_listen_port = atoi(optarg);
                    printf("*** Doing wire to air proxy, lisen port %d\n",
                            net2air_control.net2air_listen_port);
                    do_net2air_proxy = 1; // take network packets and broadcast on v2x link
                }
                break;
            case 'O':
                g_tx_flow_content[flow_cnt].flow_type = SpsOnly;
                break;
            case 'p':
                if (optarg) {
                    cla_portnum = atoi(optarg);
                    cla_portnum_set = true;
                    printf("Portnum set to %d, for RX port when receving or dest port when transmitting\n",
                            cla_portnum);
                }
                break;
            case 'P':
                if (optarg) {
                    g_tx_flow_content[flow_cnt].sps_flow_info.reservation.v2xid = atoi(optarg);
                    if (g_verbosity) {
                        printf("PSID/Service ID set to %d\n",
                            g_tx_flow_content[flow_cnt].sps_flow_info.reservation.v2xid);
                    }
                }
                break;
            case 'R':
                test_mode = ModeRx;
                if (optarg) {
                    g_sid_list_len = parse_service_id_list(g_sid_list, optarg, g_verbosity);
                }
                break;
            case 's':
                if (optarg) {
                    g_tx_flow_content[flow_cnt].sps_port = atoi(optarg);
                }
                break;
            case 't':
                if (optarg) {
                    g_tx_flow_content[flow_cnt].evt_port = atoi(optarg);
                }
                break;
            case 'V':
                g_verbosity++;
                break;
            case 'v':
                if (optarg) {
                    g_verbosity = atoi(optarg);
                }
                break;
            case 'x':
                if (optarg) {
                    g_strlcpy(air2net_control.if_name, optarg, sizeof(air2net_control.if_name));
                    printf("air2net_control.if_name is %s\n", air2net_control.if_name);
                }
                break;
            case 'X':
                if (optarg) {
                    g_strlcpy(air2net_control.remote_net_address, optarg,
                              sizeof(air2net_control.remote_net_address));
                    printf("air2net_control.remote_net_address is %s\n",
                            air2net_control.remote_net_address);
                    do_air2net_proxy = 1;
                    printf(" Enabling PC5 air link to UDP downlink proxy\n");
                }
                break;
            case 'Y':
                if (optarg) {
                    air2net_control.air2net_forward_to_port = atoi(optarg);
                }
                break;
            case 'Z':
                g_use_syslog = 1;
                break;
            case 'h':
            case '?':
            default:
                print_usage(argv[0]);
                exit(0);
        }
    }
}

cv2x_app_state_t state_setup_flows()
{
    cv2x_app_state_t ret = mode_to_state[test_mode];
    if (do_air2net_proxy) {
        ret = air2proxy_setup_flows();
    } else if (do_net2air_proxy) {
        ret = proxy2air_setup_flows();
    }
    return ret;
}

int main(int argc, char **argv)
{
    // frist state is STATE_START
    cv2x_app_state_t g_app_state = STATE_START;
    cv2x_app_state_t next_state = g_app_state;
    char* groups[] = { "system", "diag", "radio"};
    set_supplementary_groups(groups, 3);

    uid_t uid = getuid();
    if (uid == 0) {
        /*Change running as non-root user*/
        int8_t new_user_caps[] = {CAP_NET_ADMIN};
        int ret  = change_user("its", new_user_caps, sizeof(new_user_caps) / sizeof(int8_t));
        if (0 != ret) {
            printf("change user failed ");
            //continue even if change to non-root user fail;
        }
    }

    // parse argv, maybe is conf global value
    parse_opts(argc, argv);

    if (pipe(g_terminate_pipe) == -1) {
        LOGE("Error creating pipe.\n");
        return 0;
    }

    /* Application state machine */
    while (1) {
        switch (g_app_state) {
            case STATE_START:
                next_state = state_start();
                break;
            case STATE_INITIALIZE:
                next_state = state_initialize();
                break;
            case STATE_SETUP_FLOWS:
                next_state = state_setup_flows();
                break;
            case STATE_RECEIVE:
                next_state = state_receive();
                break;
            case STATE_TRANSMIT:
                next_state = state_transmit(0);
                break;
            case STATE_PAUSE:
                next_state = state_pause(true);
                break;
            case STATE_DEINITIALIZE:
                next_state = state_deinitialize();
                break;
            default:
                break;
        }
        if (next_state != g_app_state) {
            log_state_transition(g_app_state, next_state);
            g_app_state = next_state;
        }
        if (g_app_state != STATE_EXIT) {
            g_app_state = g_terminate ? STATE_DEINITIALIZE : next_state;
        } else {
            break;
        }
    }
    printf("terminating\n");
    return 0;
}
