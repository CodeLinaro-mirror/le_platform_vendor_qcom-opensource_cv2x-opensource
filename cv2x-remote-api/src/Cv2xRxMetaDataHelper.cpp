/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sstream>
#include "telux/cv2x/Cv2xRxMetaDataHelper.hpp"
#include "telux/cv2x/Cv2xRadioTypes.hpp"

namespace telux {
namespace cv2x {

static constexpr uint8_t TYPE_LEN = 1; // 1 byte for the type, type should be 0 ~ 255
static constexpr uint8_t LENGTH_INFO_SIZE = 1; // 1 byte encoding of the Length info

// type definitions
static constexpr uint8_t TLV_MD_PADDING_TYPE = 0x0;  // used when some meta data are missing
static constexpr uint8_t TLV_MD_START_TYPE = 0xFF; // START
static constexpr uint8_t TLV_MD_END_TYPE = 0x1;  // END
static constexpr uint8_t TLV_MD_SFN_TYPE = 0x2;
static constexpr uint8_t TLV_MD_SUBCH_IDX_TYPE = 0x3;
static constexpr uint8_t TLV_MD_DST_ID_TYPE = 0x4;
static constexpr uint8_t TLV_MD_RSSI_TYPE = 0x5;
static constexpr uint8_t TLV_MD_SCI_TYPE = 0x6;
static constexpr uint8_t TLV_MD_PKT_DELAY_EST_TYPE = 0x7;
static constexpr uint8_t TLV_MD_SUBCH_NUM_TYPE = 0x8;
// bytes used by each meta data information
static constexpr unsigned TLV_MD_START_LEN = 1;
static constexpr unsigned TLV_MD_END_LEN = 1;
static constexpr unsigned TLV_MD_SFN_LEN = 2;
static constexpr unsigned TLV_MD_SUBCH_IDX_LEN = 1;
static constexpr unsigned TLV_MD_DST_ID_LEN = 4;
static constexpr unsigned TLV_MD_RSSI_LEN = 2;
static constexpr unsigned TLV_MD_SCI_LEN = 4;
static constexpr unsigned TLV_MD_PKT_DELAY_EST_LEN = 4;
static constexpr unsigned TLV_MD_SUBCH_NUM_LEN = 1;

// The minimum meta data should consist the START, END markers,
// and the time and frequency information: SFN, SubChannelIndex.
static constexpr unsigned MIN_MD_LEN = TLV_MD_START_LEN + TLV_MD_END_LEN +
    TLV_MD_SFN_LEN + TLV_MD_SUBCH_IDX_LEN + 2 * (TYPE_LEN + LENGTH_INFO_SIZE);

// For just 1 TLV, 3 bytes is needed for type, length, and value
static constexpr unsigned MIN_TLV_LEN = 3;

/*
 * getFullRxMetaDataReport - get the received packet's meta data, this is used for
 *                           the packet which only have meta data
 *
 * @payload       - the pointer to the meta data payload
 * @payloadLength - length
 * @metaData      - value resulted, it contains the rx meta data information decoded
 *
 * Return the length of meta data, or 0 if no meta data presented
 */
static unsigned getFullRxMetaDataReport(const uint8_t* payload, uint32_t length,
                                        RxPacketMetaDataReport& metaData) {
    unsigned metaDataLen = 0;
    if (nullptr == payload || length < MIN_TLV_LEN) {
        std::cout << " Invalid parameter, length: " << 
            static_cast<int>(length);
        return metaDataLen;
    }

    auto pl = payload;
    auto pEnd = pl + length - 1;
    bool found = false;
    bool parse = true;

    while (parse && pl <= pEnd) {
        switch (*pl) {
        case TLV_MD_PADDING_TYPE:
             pl += TYPE_LEN;
             break;
        case TLV_MD_END_TYPE:
             // END marker found, a valid full meta data is parsed out
             found = true;
             parse = false;
             pEnd = pl;
             break;
        case TLV_MD_DST_ID_TYPE:
             pl += TYPE_LEN;
             if (pl + LENGTH_INFO_SIZE + TLV_MD_DST_ID_LEN <= pEnd &&
                 *pl == TLV_MD_DST_ID_LEN) {
                 pl += LENGTH_INFO_SIZE;
                 // 4 bytes for L2 Destination ID
                 metaData.l2DestinationId = *(uint32_t *)pl;
                 metaData.metaDataMask |= RX_L2_DEST_ID;

                 pl += TLV_MD_DST_ID_LEN;
             } else {
                 parse = false;
             }
             break;
        case TLV_MD_RSSI_TYPE:
             pl += TYPE_LEN;
             if (pl + LENGTH_INFO_SIZE + TLV_MD_RSSI_LEN <= pEnd &&
                 *pl == TLV_MD_RSSI_LEN) {
                 pl += LENGTH_INFO_SIZE;
                 // 1 byte for both RSSI value
                 metaData.prxRssi = *pl;
                 metaData.drxRssi = *(pl + 1);
                 metaData.metaDataMask |= RX_PRX_RSSI;
                 metaData.metaDataMask |= RX_DRX_RSSI;

                 pl += TLV_MD_RSSI_LEN;
             } else {
                 parse = false;
             }
             break;
        case TLV_MD_SCI_TYPE:
             pl += TYPE_LEN;
             if (pl + LENGTH_INFO_SIZE + TLV_MD_SCI_LEN <= pEnd &&
                 *pl == TLV_MD_SCI_LEN) {
                 pl += LENGTH_INFO_SIZE;
                 // 4 bytes for SCI format1
                 metaData.sciFormat1Info = *(uint32_t *)pl;
                 pl += TLV_MD_SCI_LEN;
                 metaData.metaDataMask |= RX_SCI_FORMAT1;
             } else {
                 parse = false;
             }
             break;
        case TLV_MD_PKT_DELAY_EST_TYPE:
             pl += TYPE_LEN;
             if (pl + LENGTH_INFO_SIZE + TLV_MD_PKT_DELAY_EST_LEN <= pEnd &&
                 *pl == TLV_MD_PKT_DELAY_EST_LEN) {
                 pl += LENGTH_INFO_SIZE;
                 // 4 bytes for packets delay estimation
                 metaData.delayEstimation = *(uint32_t *)pl;
                 pl += TLV_MD_PKT_DELAY_EST_LEN;
                 metaData.metaDataMask |= RX_DELAY_ESTIMATION;

             } else {
                 parse = false;
             }
             break;
        case TLV_MD_SUBCH_NUM_TYPE:
             pl += TYPE_LEN;
             if (pl + LENGTH_INFO_SIZE + TLV_MD_SUBCH_NUM_LEN <= pEnd &&
                 *pl == TLV_MD_SUBCH_NUM_LEN) {
                 pl += LENGTH_INFO_SIZE;
                 // 1 byte for subchannel number
                 metaData.subChannelNum = *pl;
                 pl += TLV_MD_SUBCH_NUM_LEN;
                 metaData.metaDataMask |= RX_SUBCHANNEL_NUMBER;
             } else {
                 parse = false;
             }
             break;
        default:
             std::cout << " Non recognized type" << std::endl;
             parse= false;
        }
    }

    if (found) {
        metaDataLen = pEnd - payload + 1;
    }

    return metaDataLen;
}
/*
 * getTimeFrequency - try to decode the subframe number and subchannel index
 *
 * @payload - the pointer to the received packet's data
 * @payloadLength - received packets length
 * @metaData - value resulted, it contains the rx meta data information parsed
 *
 * Return the length of meta data, or 0 if no meta data presented
 */
static unsigned getTimeFrequency(const uint8_t* payload, uint32_t payloadLength,
                                 RxPacketMetaDataReport& metaData) {
    unsigned metaDataLen = 0;
    if (nullptr == payload || payloadLength < MIN_MD_LEN) {
        std::cout << " Invalid parameter, payloadLength: " <<
            static_cast<int>(payloadLength) << std::endl;
        return metaDataLen;
    }

    const uint8_t* pl = payload;
    uint16_t sfn = -1;
    uint8_t subChannelIndex= -1;
    // Meta head contains two TLVs: SFN, SubChannelIndex, in between START(0xFF) and END(0x1)
    if (*pl == TLV_MD_START_TYPE) {
        pl += TYPE_LEN;
        // get subframe number
        if (*pl == TLV_MD_SFN_TYPE) {
            pl += TYPE_LEN;
            if (*pl == TLV_MD_SFN_LEN) {
                pl += LENGTH_INFO_SIZE;
                sfn = *(uint16_t *)pl;
                pl += TLV_MD_SFN_LEN;
                // get subchannel index
                if (*pl == TLV_MD_SUBCH_IDX_TYPE) {
                    pl += TYPE_LEN;
                    if (*pl == TLV_MD_SUBCH_IDX_LEN) {
                        pl += LENGTH_INFO_SIZE;
                        subChannelIndex = *pl++;
                        metaData.sfn = sfn;
                        metaData.subChannelIndex = subChannelIndex;

                        // check if encounter the END
                        if (*pl == TLV_MD_END_TYPE) {
                            metaDataLen = MIN_MD_LEN;
                        } else {
                            metaDataLen = MIN_MD_LEN - TLV_MD_END_LEN;
                        }
                    }
                }
            }
        }
    }

    // set the validity for SFN and SubChannelIndex together, lack of either
    // one makes the meta data useless. Both items are needed to match the meta data
    // to the packet.
    if (metaDataLen > 0) {
        metaData.metaDataMask |= RX_SUBFRAME_NUMBER;
        metaData.metaDataMask |= RX_SUBCHANNEL_INDEX;
    }
    return metaDataLen;
}

void logMetaDataReport(RxPacketMetaDataReport& metaData) {
    std::stringstream metaStr;
    if (metaData.metaDataMask & RX_SUBFRAME_NUMBER) {
        metaStr << " OTA subframe :" << static_cast<int>(metaData.sfn);
    }
    if (metaData.metaDataMask & RX_SUBCHANNEL_INDEX) {
        metaStr << " Subchannel Index:"
                << static_cast<int>(metaData.subChannelIndex);
    }
    if (metaData.metaDataMask & RX_SUBCHANNEL_NUMBER) {
        metaStr << " subchannel number:"
                << static_cast<int>(metaData.subChannelNum);
    }
    if (metaData.metaDataMask & RX_DELAY_ESTIMATION) {
        metaStr << " packets delay estimation:"
                 << static_cast<int>(metaData.delayEstimation);
    }
    if (metaData.metaDataMask & RX_PRX_RSSI) {
        metaStr << " RSSI of PRx:" << static_cast<int>(metaData.prxRssi);
    }
    if (metaData.metaDataMask & RX_DRX_RSSI) {
        metaStr << " RSSI of DRx:" << static_cast<int>(metaData.drxRssi);
    }
    if (metaData.metaDataMask & RX_L2_DEST_ID) {
        metaStr << " L2 Destination ID:0x" << std::hex << metaData.l2DestinationId;
    }
    if (metaData.metaDataMask & RX_SCI_FORMAT1) {
        metaStr << " SCI format1:0x" << std::hex << static_cast<int>(metaData.sciFormat1Info);
    }
    //COMMONAPI_DEBUG(metaStr.str());
}

telux::common::Status Cv2xRxMetaDataHelper::getRxMetaDataInfo(
        const uint8_t* payload, uint32_t payloadLength,
        size_t& metaDataLen, std::shared_ptr<std::vector<RxPacketMetaDataReport>> metaDatas) {
    metaDataLen = 0;
    if (nullptr == payload || !metaDatas) {
        std::cout << " Invalid parameter" << std::endl;
        return telux::common::Status::INVALIDPARAM;
    }

    auto pl = payload;
    auto plen = payloadLength;
    RxPacketMetaDataReport metaData = {0};

    do {
        // parse the OTA timing(SFN) and frequency location(SubChannel index) info,
        // SFN and SubChannel index TLVs are mandatory for every meta data reports.
        auto tfLen = getTimeFrequency(pl, plen, metaData);

        if ((metaData.metaDataMask & RX_SUBFRAME_NUMBER) &&
            (metaData.metaDataMask & RX_SUBCHANNEL_INDEX)) {
            pl += tfLen;
            plen -= tfLen;

            if (tfLen != MIN_MD_LEN) { // in case no "END" found, continue the parsing
                auto reportLen = getFullRxMetaDataReport(pl, plen, metaData);
                if (reportLen == 0) { // Wrong TLV format, cease parsing
                    // Not a valid meta data
                    break;
                }
                metaDataLen += reportLen;
                pl += reportLen;
                plen -= reportLen;
            }
            metaDataLen += tfLen;
            (*metaDatas).push_back(metaData);
            logMetaDataReport(metaData);
        } else {
            // real payload encountered, not meta data TLVs
            break;
        }
    } while (plen > MIN_MD_LEN);

    return telux::common::Status::SUCCESS;
}

}
}
