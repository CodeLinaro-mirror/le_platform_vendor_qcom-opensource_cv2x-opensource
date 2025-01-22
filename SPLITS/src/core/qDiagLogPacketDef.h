/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef QDIAGLOGPACKETDEF_H
#define QDIAGLOGPACKETDEF_H

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define V2X_QITS_LOG_VERSION 0x00
#define v2x_qits_general_rx_info LP_v2x_qits_general_rx_info
#define v2x_qits_general_tx_info LP_v2x_qits_general_tx_info
#define v2x_qits_general_periodic_info LP_v2x_qits_general_periodic_info

typedef struct {
    uint32_t eventHazardLights : 1;
    uint32_t eventStopLineViolation : 1;
    uint32_t eventABSactivated : 1;
    uint32_t eventTractionControlLoss : 1;
    uint32_t eventStabilityControlactivated : 1;
    uint32_t eventHazardousMaterials : 1;
    uint32_t eventReserved1 : 1;
    uint32_t eventHardBraking : 1;
    uint32_t eventLightsChanged : 1;
    uint32_t eventWipersChanged : 1;
    uint32_t eventFlatTire : 1;
    uint32_t eventDisabledVehicle : 1;
    uint32_t eventAirBagDeployment : 1;
    /// @QC_hidden
    uint32_t unused : 3;
} v2x_diag_event_bit_t;

typedef enum {
    DIAG_SPS,
    DIAG_EVENT
} v2x_diag_transmit_type_et;

typedef struct {
    uint32_t msg_count;
    uint32_t temp_id;
    /// @QC_synthetic_Unit ms
    /// @QC_synthetic_internal
    /// @QC_internal
    /// @QC_hidden
    uint32_t secmark_ms;
    /// @QC_synthetic_Unit degree
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.7f
    /// @qc_synthetic_formula double longitude, (."longitude"/10000000.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t longitude;
    /// @QC_synthetic_Unit degree
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.7f
    /// @qc_synthetic_formula double latitude, (."latitude"/10000000.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t latitude;
    /// @QC_synthetic_Unit meter
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double semi_major_dev, (."semi_major_dev"/20.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t semi_major_dev;
    /// @QC_synthetic_Unit km/h
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double speed, (."speed"*0.072)
    /// @QC_internal
    /// @QC_hidden
    uint32_t speed;
    /// @QC_synthetic_Unit degree/
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.4f
    /// @qc_synthetic_formula double heading, (."heading"*0.0125)
    /// @QC_internal
    /// @QC_hidden
    uint32_t heading;
    /// @QC_synthetic_Unit m/s2
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double long_accel, (."long_accel"/100.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t long_accel;
    /// @QC_synthetic_Unit m/s2
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double lat_accel, (."lat_accel"/100.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t lat_accel;
} v2x_diag_bsm_data;

typedef struct {
    /// @QC_display_name LOG-Timestamp
    uint64_t time_stamp_log;
    /// @QC_display_name MSG-Timestamp
    uint64_t time_stamp_msg;
    /// @QC_display_name GPS-Timestamp
    uint64_t gnss_time;
    /// @QC_synthetic_Unit %
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double CPU_Util, (."CPU_Util"/100.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t CPU_Util;
    uint32_t GPS_mode;
    /// @QC_synthetic_Unit meter
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double tracking_error, (."tracking_error"/100.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t tracking_error;
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double vehicle_density_in_range, (."vehicle_density_in_range"/100.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t vehicle_density_in_range;
    /// @QC_synthetic_Unit ns
    uint64_t max_ITT;
    uint32_t hysterisis;
    uint32_t L2_ID;
    v2x_diag_event_bit_t events;
    /// @QC_display_name Is Msg Valid
    bool msg_valid;
} v2x_diag_qits_general_data;

#pragma region PKT_ID_QITS_RX_FLOW

typedef struct {
    v2x_diag_bsm_data bsm_data;
    v2x_diag_qits_general_data general_data;
    uint32_t total_RVs;
    uint32_t distance_from_RV;
    /// @QC_ENUM_TYPE v2x_diag_transmit_type_et
    v2x_diag_transmit_type_et msg_type;
} V2X_QITS_GENERAL_RX_PKG;

typedef union {
    /// @QC_discriminator_condition 0x0
    /// @QC_inline
    /// @QC_hide_bracket
    /// @QC_display_name Version 0x0
    V2X_QITS_GENERAL_RX_PKG version_0x0;
} v2x_qits_general_rx_versions_u;

/*! @QC_brief This is description
    \struct LP_v2x_qits_general_rx_info
    @QC_log_id 0x3372
    @QC_log_name V2X HLOS qits general rx info
    @QC_log_category Known Log Items$V2X$HLOS$CV2X
    @QC_external
*/
typedef struct {
    /// @QC_SYNTHETIC_FORMULA Major Version, ."Version" >> 16 & 0xffff
    /// @QC_SYNTHETIC_FORMULA Minor Version, ."Version" & 0xffff
    /// @QC_HIDDEN
    uint32_t Version;

    /// @QC_discriminator Version
    v2x_qits_general_rx_versions_u Versions;
} LP_v2x_qits_general_rx_info;

#pragma endregion

#pragma region PKT_ID_QITS_TX_FLOW

typedef struct {
    v2x_diag_bsm_data bsm_data;
    v2x_diag_qits_general_data general_data;
    uint64_t tx_interval;
    /// @QC_synthetic_Unit %
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double channel_quality_indication, (."channel_quality_indication"/100.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t channel_quality_indication;
    uint32_t DCC_random_time;
    /// @QC_ENUM_TYPE v2x_diag_transmit_type_et
    v2x_diag_transmit_type_et msg_type;
} V2X_QITS_GENERAL_TX_PKG;

typedef union {
    /// @QC_discriminator_condition 0x0
    /// @QC_inline
    /// @QC_hide_bracket
    /// @QC_display_name Version 0x0
    V2X_QITS_GENERAL_TX_PKG version_0x0;
} v2x_qits_general_tx_versions_u;

/*! @QC_brief This is description
    \struct LP_v2x_qits_general_tx_info
    @QC_log_id 0x3371
    @QC_log_name V2X HLOS qits general tx info
    @QC_log_category Known Log Items$V2X$HLOS$CV2X
    @QC_external
*/
typedef struct {
    /// @QC_SYNTHETIC_FORMULA Major Version, ."Version" >> 16 & 0xffff
    /// @QC_SYNTHETIC_FORMULA Minor Version, ."Version" & 0xffff
    /// @QC_HIDDEN
    uint32_t Version;

    /// @QC_discriminator Version
    v2x_qits_general_tx_versions_u Versions;
} LP_v2x_qits_general_tx_info;

#pragma endregion

#pragma region PKT_ID_QITS_GENERIC_INFO

typedef struct {
    /// @QC_synthetic_Unit ns
    uint64_t max_ITT;
    /// @QC_synthetic_Unit %
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double CPU_Util, (."CPU_Util"/100.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t CPU_Util;
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double vehicle_density_in_range, (."vehicle_density_in_range"/100.0)
    /// @QC_internal
    /// @QC_hidden
    uint32_t vehicle_density_in_range;
    uint32_t total_RVs;
    /// @QC_synthetic_Unit meter
    /// @QC_synthetic_internal
    /// @QC_synthetic_format %.2f
    /// @qc_synthetic_formula double tracking_error, (."tracking_error"/100.0)
    /// @QC_internal
    /// @QC_hidden
    int32_t tracking_error;
    uint32_t L2_ID;
    v2x_diag_event_bit_t events;
} V2X_QITS_GENERAL_PERIODIC_PKG;

typedef union {
    /// @QC_discriminator_condition 0x0
    /// @QC_inline
    /// @QC_hide_bracket
    /// @QC_display_name Version 0x0
    V2X_QITS_GENERAL_PERIODIC_PKG version_0x0;
} v2x_qits_general_periodic_versions_u;

/*! @QC_brief This is description
    \struct LP_v2x_qits_general_periodic_info
    @QC_log_id 0x3373
    @QC_log_name V2X HLOS qits general periodic info
    @QC_log_category Known Log Items$V2X$HLOS$CV2X
    @QC_external
*/
typedef struct {
    /// @QC_SYNTHETIC_FORMULA Major Version, ."Version" >> 16 & 0xffff
    /// @QC_SYNTHETIC_FORMULA Minor Version, ."Version" & 0xffff
    /// @QC_HIDDEN
    uint32_t Version;

    /// @QC_discriminator Version
    v2x_qits_general_periodic_versions_u Versions;
} LP_v2x_qits_general_periodic_info;

#pragma endregion

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif