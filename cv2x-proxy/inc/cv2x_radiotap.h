/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __CV2X_RADIOTAP_H
#define __CV2X_RADIOTAP_H

/*
 * Following the address fields, IEEE 802.3 Ethernet uses a 2-byte "length"
 * field that includes the IEEE 802.2 Logical Link Control (LLC) bytes
 * and the data bytes.
 * For our purposes, it will just be a value representing WSMP and another for ETSI
 */

#define EPD_ETHER_TYPE_WSMP (0x88DC)
#define EPD_ETHER_TYPE_ETSI (0x8947)
unsigned short edp_hdr;

#define RADIOTAP_DATA_LENGTH 12

struct ieee80211_radiotap_header {
    u_int8_t        it_version;     /* set to 0 */
    u_int8_t        it_pad;
    u_int16_t       it_len;         /* entire length */
    u_int32_t       it_present;     /* fields present */
} __attribute__((__packed__));

struct radiotapdata {
    uint32_t  seconds; // seconds value when frame was captured
    uint32_t  useconds; // usec value when frame was captured
    uint16_t  length; // length of packet (radiotap + 802.11 frame)
    uint16_t  reserved; // padding used for 32 bit alignment
} __attribute__ ((packed));

// 80211 HEADER

/* Frame Control:
 * Version
 * Type
 * Subtype
 * To DS
 * From DS
 * More Frag
 * Retry
 * Power Mgmt
 * More Data
 * WEP
 * Order
 */

/* Duration/ID :
 * 4 bytes long field
 * indicates period of time in which medium is occupied
 */

/*
 * Address 1 - 4 each 6 bytes long
 * Contains IEEE 802 MAC Addresses (48 bits each)
 */

/* Sequence Control:
 * 16 bits long field consisting of
 * Sequence number (12 bits) and
 * Fragment Number (4 bits)
 * Used to filter duplicate frames.
 */

/*
 * IEEE 802.11 section 8.2.4.5, QoS Control Field
 */

#define IEEE80211_QOS_CTL_ACK_POLICY_NORMAL             0x0000
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_ACK             0x0020
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPLICIT_ACK    0x0040
#define IEEE80211_QOS_CTL_ACK_POLICY_BLOCK_ACK          0x0060
#define IEEE80211_ADDR_LEN 6

struct ieee80211_wifi_hdr_t {
    uint16_t frameCtl;
    uint16_t durationId;
    unsigned char   addr1 [IEEE80211_ADDR_LEN]; // Receiver Address
    unsigned char   addr2 [IEEE80211_ADDR_LEN]; // Sender Address
    unsigned char   addr3 [IEEE80211_ADDR_LEN]; // Other Address
    uint16_t sequence;
    uint16_t qos;
};

/* radiotap_message_t
 * A special packet structure with radiotap, 802.11 header, and EDP
 * backwards compatible with DSRC test equipement and packet sniffers.
 */
typedef struct __radiotap_msg_hdr_t {
    struct  ieee80211_radiotap_header radiotap_hdr;
    uint16_t ch_freq; // somehow need to put bsm channel frequency into here.
    uint16_t ch_flags;
    struct ieee80211_wifi_hdr_t ieee80211_wifi_hdr;
    unsigned short edp_hdr;
} radiotap_msg_hdr_t;

#endif /* __CV2X_RADIOTAP_H */
