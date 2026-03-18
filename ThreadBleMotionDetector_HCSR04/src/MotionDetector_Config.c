/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * This software is owned by Qorvo Inc
 * and protected under applicable copyright laws.
 * It is delivered under the terms of the license
 * and is intended and supplied for use solely and
 * exclusively with products manufactured by
 * Qorvo Inc.
 *
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS"
 * CONDITION. NO WARRANTIES, WHETHER EXPRESS,
 * IMPLIED OR STATUTORY, INCLUDING, BUT NOT
 * LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * QORVO INC. SHALL NOT, IN ANY
 * CIRCUMSTANCES, BE LIABLE FOR SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES,
 * FOR ANY REASON WHATSOEVER.
 *
 *
 */

/** @file "MotionDetector_Config.c"
 *
 * BLE GATT attribute table for the QPG6200 Thread+BLE HC-SR04 Motion Detector.
 *
 * Three GATT services:
 *
 *  1. Battery Service  (0x180F)   - standard, visible in nRF Connect
 *  2. Motion Detection Service    - motion status notifications and distance
 *  3. Thread Config Service       - commission Thread network via BLE
 *
 * --- Thread Config Service workflow ---
 *  a) Connect via nRF Connect / Qorvo Connect.
 *  b) Write Thread Network Name characteristic (up to 16 bytes, e.g. "MotionNet").
 *  c) Write Thread Network Key characteristic  (16-byte master key, write-only).
 *  d) Optionally write Channel (1 byte, 11-26) and PAN ID (2 bytes LE).
 *  e) Write 0x01 to the Join characteristic to start Thread network join.
 *  f) Subscribe to Thread Status notifications to watch the device role.
 *
 * --- Motion Detection Service workflow ---
 *  a) Enable notifications on the Motion Status and/or Distance characteristic.
 *  b) Object within 200 cm -> Motion Status notification value 0x01.
 *  c) Object moves away    -> Motion Status notification value 0x00.
 *  d) Distance characteristic carries the raw cm value (2 bytes big-endian).
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "MotionDetector_Config.h"

#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/* =========================================================================
 *  UUID definitions
 * ========================================================================= */

/* Motion Detection Service UUID (128-bit) : 4D4F544D-0000-1000-8000-00805F9B3400 */
#define MOTION_SERVICE_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x4E, 0x4F, 0x54, 0x4D

/* Motion Status Characteristic UUID     : 4D4F544D-0000-1000-8000-00805F9B3401 */
#define MOTION_STATUS_CHAR_UUID_128 \
    0x01, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x4E, 0x4F, 0x54, 0x4D

/* Distance Characteristic UUID          : 4D4F544D-0000-1000-8000-00805F9B3402 */
#define MOTION_DIST_CHAR_UUID_128 \
    0x02, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x4E, 0x4F, 0x54, 0x4D

/* Thread Config Service UUID (128-bit) : D00RBELL-0002-1000-8000-00805F9B3400 */
#define THREAD_CFG_SERVICE_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread Network Name Characteristic : D00RBELL-0002-1000-8000-00805F9B3401 */
#define THREAD_NET_NAME_CHAR_UUID_128 \
    0x01, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread Network Key Characteristic  : D00RBELL-0002-1000-8000-00805F9B3402 */
#define THREAD_NET_KEY_CHAR_UUID_128 \
    0x02, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread Channel Characteristic      : D00RBELL-0002-1000-8000-00805F9B3403 */
#define THREAD_CHANNEL_CHAR_UUID_128 \
    0x03, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread PAN ID Characteristic       : D00RBELL-0002-1000-8000-00805F9B3404 */
#define THREAD_PANID_CHAR_UUID_128 \
    0x04, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread Join Characteristic         : D00RBELL-0002-1000-8000-00805F9B3405 */
#define THREAD_JOIN_CHAR_UUID_128 \
    0x05, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Thread Status Characteristic       : D00RBELL-0002-1000-8000-00805F9B3406 */
#define THREAD_STATUS_CHAR_UUID_128 \
    0x06, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x02, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Standard GATT UUIDs */
static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN]  = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN]     = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/* =========================================================================
 *  Battery Service
 * ========================================================================= */

static const uint8_t  batterySvcUuid[]   = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen      = sizeof(batterySvcUuid);

static const uint8_t  batteryCh[]        = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                             UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
                                             UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t batteryChLen       = sizeof(batteryCh);

static uint8_t        batteryChValue[1]  = {100};
static const uint16_t batteryChValueLen  = sizeof(batteryChValue);

static uint8_t        batteryLevelCcc[]  = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelCccLen = sizeof(batteryLevelCcc);

/* clang-format off */
static const attsAttr_t Battery_GATT_List[] = {
    { attTypePrimSvcUuid, (uint8_t*)batterySvcUuid,  (uint16_t*)&batterySvcLen,     sizeof(batterySvcUuid),    0,                    ATTS_PERMIT_READ },
    { attTypeCharUuid,    (uint8_t*)batteryCh,        (uint16_t*)&batteryChLen,      sizeof(batteryCh),         0,                    ATTS_PERMIT_READ },
    { &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], (uint8_t*)batteryChValue, (uint16_t*)&batteryChValueLen, sizeof(batteryChValue), ATTS_SET_READ_CBACK, ATTS_PERMIT_READ },
    { attTypeCliChCfgUuid, batteryLevelCcc, (uint16_t*)&batteryLevelCccLen, sizeof(batteryLevelCcc), ATTS_SET_CCC, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
};
/* clang-format on */

/* =========================================================================
 *  Motion Detection Service
 * ========================================================================= */

static const uint8_t  motionSvcUuid[]        = {MOTION_SERVICE_UUID_128};
static const uint16_t motionSvcLen           = sizeof(motionSvcUuid);

/* Motion Status characteristic */
static const uint8_t  motionStatusCh[]       = {ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_NOTIFY,
                                                  UINT16_TO_BYTES(MOTION_STATUS_HDL),
                                                  MOTION_STATUS_CHAR_UUID_128};
static const uint16_t motionStatusChLen      = sizeof(motionStatusCh);
static uint8_t        motionStatusValue[1]   = {MOTION_STATE_CLEAR};
static const uint16_t motionStatusValueLen   = sizeof(motionStatusValue);
static uint8_t        motionStatusCcc[]      = {UINT16_TO_BYTES(0x0000)};
static const uint16_t motionStatusCccLen     = sizeof(motionStatusCcc);

/* Distance characteristic */
static const uint8_t  motionDistCh[]         = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                  UINT16_TO_BYTES(MOTION_DIST_HDL),
                                                  MOTION_DIST_CHAR_UUID_128};
static const uint16_t motionDistChLen        = sizeof(motionDistCh);
static uint8_t        motionDistValue[2]     = {0x00, 0x00};
static const uint16_t motionDistValueLen     = sizeof(motionDistValue);
static uint8_t        motionDistCcc[]        = {UINT16_TO_BYTES(0x0000)};
static const uint16_t motionDistCccLen       = sizeof(motionDistCcc);

/* clang-format off */
static const attsAttr_t Motion_GATT_List[] = {
    { attTypePrimSvcUuid, (uint8_t*)motionSvcUuid, (uint16_t*)&motionSvcLen, sizeof(motionSvcUuid), ATTS_SET_UUID_128, ATTS_PERMIT_READ },
    { attTypeCharUuid,    (uint8_t*)motionStatusCh, (uint16_t*)&motionStatusChLen, sizeof(motionStatusCh), 0, ATTS_PERMIT_READ },
    { &motionStatusCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], motionStatusValue, (uint16_t*)&motionStatusValueLen, sizeof(motionStatusValue), ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
    { attTypeCliChCfgUuid, motionStatusCcc, (uint16_t*)&motionStatusCccLen, sizeof(motionStatusCcc), ATTS_SET_CCC, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
    { attTypeCharUuid,    (uint8_t*)motionDistCh, (uint16_t*)&motionDistChLen, sizeof(motionDistCh), 0, ATTS_PERMIT_READ },
    { &motionDistCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], motionDistValue, (uint16_t*)&motionDistValueLen, sizeof(motionDistValue), ATTS_SET_UUID_128, ATTS_PERMIT_READ },
    { attTypeCliChCfgUuid, motionDistCcc, (uint16_t*)&motionDistCccLen, sizeof(motionDistCcc), ATTS_SET_CCC, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
};
/* clang-format on */

/* =========================================================================
 *  Thread Config Service
 * ========================================================================= */

/* Service declaration */
static const uint8_t  threadCfgSvcUuid[]    = {THREAD_CFG_SERVICE_UUID_128};
static const uint16_t threadCfgSvcLen       = sizeof(threadCfgSvcUuid);

/* Network Name characteristic */
static const uint8_t  threadNetNameCh[]     = {ATT_PROP_READ | ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(THREAD_NET_NAME_HDL),
                                                THREAD_NET_NAME_CHAR_UUID_128};
static const uint16_t threadNetNameChLen    = sizeof(threadNetNameCh);
static uint8_t        threadNetNameValue[THREAD_NET_NAME_MAX_LEN] = "MotionNet";
static uint16_t       threadNetNameValueLen = 9; /* strlen("MotionNet") */

/* Network Key characteristic (write-only) */
static const uint8_t  threadNetKeyCh[]      = {ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(THREAD_NET_KEY_HDL),
                                                THREAD_NET_KEY_CHAR_UUID_128};
static const uint16_t threadNetKeyChLen     = sizeof(threadNetKeyCh);
static uint8_t        threadNetKeyValue[THREAD_NET_KEY_LEN] = {0};
static uint16_t       threadNetKeyValueLen  = THREAD_NET_KEY_LEN;

/* Channel characteristic */
static const uint8_t  threadChannelCh[]     = {ATT_PROP_READ | ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(THREAD_CHANNEL_HDL),
                                                THREAD_CHANNEL_CHAR_UUID_128};
static const uint16_t threadChannelChLen    = sizeof(threadChannelCh);
static uint8_t        threadChannelValue[1] = {15};  /* Default channel 15 */
static uint16_t       threadChannelValueLen = 1;

/* PAN ID characteristic */
static const uint8_t  threadPanIdCh[]       = {ATT_PROP_READ | ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(THREAD_PANID_HDL),
                                                THREAD_PANID_CHAR_UUID_128};
static const uint16_t threadPanIdChLen      = sizeof(threadPanIdCh);
static uint8_t        threadPanIdValue[2]   = {0xCD, 0xAB};  /* Default 0xABCD (LE) */
static uint16_t       threadPanIdValueLen   = 2;

/* Join characteristic (write-only) */
static const uint8_t  threadJoinCh[]        = {ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(THREAD_JOIN_HDL),
                                                THREAD_JOIN_CHAR_UUID_128};
static const uint16_t threadJoinChLen       = sizeof(threadJoinCh);
static uint8_t        threadJoinValue[1]    = {0x00};
static uint16_t       threadJoinValueLen    = 1;

/* Thread Status characteristic (read + notify) */
static const uint8_t  threadStatusCh[]      = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(THREAD_STATUS_HDL),
                                                THREAD_STATUS_CHAR_UUID_128};
static const uint16_t threadStatusChLen     = sizeof(threadStatusCh);
static uint8_t        threadStatusValue[1]  = {THREAD_STATUS_DISABLED};
static uint16_t       threadStatusValueLen  = 1;
static uint8_t        threadStatusCcc[]     = {UINT16_TO_BYTES(0x0000)};
static const uint16_t threadStatusCccLen    = sizeof(threadStatusCcc);

/* clang-format off */
static const attsAttr_t ThreadCfg_GATT_List[] = {
    /* Service declaration */
    { attTypePrimSvcUuid, (uint8_t*)threadCfgSvcUuid, (uint16_t*)&threadCfgSvcLen, sizeof(threadCfgSvcUuid), ATTS_SET_UUID_128, ATTS_PERMIT_READ },

    /* Network Name: read + write */
    { attTypeCharUuid, (uint8_t*)threadNetNameCh, (uint16_t*)&threadNetNameChLen, sizeof(threadNetNameCh), 0, ATTS_PERMIT_READ },
    { &threadNetNameCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadNetNameValue, &threadNetNameValueLen, THREAD_NET_NAME_MAX_LEN, ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },

    /* Network Key: write-only */
    { attTypeCharUuid, (uint8_t*)threadNetKeyCh, (uint16_t*)&threadNetKeyChLen, sizeof(threadNetKeyCh), 0, ATTS_PERMIT_READ },
    { &threadNetKeyCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadNetKeyValue, &threadNetKeyValueLen, THREAD_NET_KEY_LEN, ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_WRITE },

    /* Channel: read + write */
    { attTypeCharUuid, (uint8_t*)threadChannelCh, (uint16_t*)&threadChannelChLen, sizeof(threadChannelCh), 0, ATTS_PERMIT_READ },
    { &threadChannelCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadChannelValue, &threadChannelValueLen, THREAD_CHANNEL_LEN, ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },

    /* PAN ID: read + write */
    { attTypeCharUuid, (uint8_t*)threadPanIdCh, (uint16_t*)&threadPanIdChLen, sizeof(threadPanIdCh), 0, ATTS_PERMIT_READ },
    { &threadPanIdCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadPanIdValue, &threadPanIdValueLen, THREAD_PANID_LEN, ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },

    /* Join: write-only */
    { attTypeCharUuid, (uint8_t*)threadJoinCh, (uint16_t*)&threadJoinChLen, sizeof(threadJoinCh), 0, ATTS_PERMIT_READ },
    { &threadJoinCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadJoinValue, &threadJoinValueLen, THREAD_JOIN_LEN, ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_WRITE },

    /* Thread Status: read + notify */
    { attTypeCharUuid, (uint8_t*)threadStatusCh, (uint16_t*)&threadStatusChLen, sizeof(threadStatusCh), 0, ATTS_PERMIT_READ },
    { &threadStatusCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], threadStatusValue, &threadStatusValueLen, THREAD_STATUS_LEN, ATTS_SET_UUID_128, ATTS_PERMIT_READ },
    { attTypeCliChCfgUuid, threadStatusCcc, (uint16_t*)&threadStatusCccLen, sizeof(threadStatusCcc), ATTS_SET_CCC, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
};
/* clang-format on */

/* =========================================================================
 *  Global variables read by BleIf.c
 * ========================================================================= */

attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    {NULL, (attsAttr_t*)Battery_GATT_List, NULL, NULL, BATTERY_SVC_HDL,    BATTERY_LEVEL_HDL_MAX - 1  },
    {NULL, (attsAttr_t*)Motion_GATT_List,  NULL, NULL, MOTION_SVC_HDL,     MOTION_SVC_HDL_MAX - 1     },
    {NULL, (attsAttr_t*)ThreadCfg_GATT_List, NULL, NULL, THREAD_CFG_SVC_HDL, THREAD_CFG_SVC_HDL_MAX - 1 },
};

/* CCC descriptor table: GATT SC, Battery Level, Motion Status, Distance, Thread Status */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    {GATT_SC_CH_CCC_HDL,       ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {MOTION_STATUS_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {MOTION_DIST_CCC_HDL,      ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {THREAD_STATUS_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
};

/* =========================================================================
 *  Advertising data
 * ========================================================================= */

static const uint8_t DefaultAdvDataFrame[] = {
    0x02,
    BLEIF_ADV_DATA_TYPE_FLAGS,
    0x06,                           /* General Discoverable + BLE only */
    0x03,
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE,
    batterySvcUuid[0], batterySvcUuid[1],
};

static const uint8_t ScanRespFrame[] = {
    0x12,                               /* length: 17 chars + type = 18 */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE,  /* type: Complete Local Name */
    'Q', 'P', 'G', ' ', 'H', 'C', '-', 'S', 'R', '0',
    '4', ' ', 'M', 'o', 't', 'i', 'o', 'n'
};

/* =========================================================================
 *  Public functions called by BleIf
 * ========================================================================= */

uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer)
{
    Q_STATIC_ASSERT(BLEIF_ADV_DATASET_MAX_SIZE >= sizeof(DefaultAdvDataFrame));
    memcpy(buffer, DefaultAdvDataFrame, sizeof(DefaultAdvDataFrame));
    return sizeof(DefaultAdvDataFrame);
}

uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer)
{
    Q_STATIC_ASSERT(BLEIF_ADV_DATASET_MAX_SIZE >= sizeof(ScanRespFrame));
    memcpy(buffer, ScanRespFrame, sizeof(ScanRespFrame));
    return sizeof(ScanRespFrame);
}

/* =========================================================================
 *  Accessor functions for AppManager (Thread characteristic values)
 * ========================================================================= */

uint8_t* ThreadCfg_GetNetworkName(uint16_t* pLen)
{
    *pLen = threadNetNameValueLen;
    return threadNetNameValue;
}

uint8_t* ThreadCfg_GetNetworkKey(void)
{
    return threadNetKeyValue;
}

uint8_t ThreadCfg_GetChannel(void)
{
    return threadChannelValue[0];
}

uint16_t ThreadCfg_GetPanId(void)
{
    /* PAN ID stored little-endian */
    return (uint16_t)((threadPanIdValue[1] << 8) | threadPanIdValue[0]);
}

void ThreadCfg_SetStatus(uint8_t status)
{
    threadStatusValue[0] = status;
}

uint8_t ThreadCfg_GetStatus(void)
{
    return threadStatusValue[0];
}
