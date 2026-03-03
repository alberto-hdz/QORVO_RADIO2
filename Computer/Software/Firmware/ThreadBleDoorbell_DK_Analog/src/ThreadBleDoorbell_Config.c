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

/** @file "ThreadBleDoorbell_Config.c"
 *
 * BLE GATT attribute table for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Three GATT services:
 *
 *  1. Battery Service  (0x180F)   - standard, visible in nRF Connect
 *  2. Doorbell Ring Service       - ring notifications / remote trigger
 *  3. Thread Config Service       - commission Thread network via BLE
 *
 * --- Thread Config Service workflow ---
 *  a) Connect via nRF Connect / Qorvo Connect.
 *  b) Write Thread Network Name characteristic (up to 16 bytes, e.g. "DoorbellNet").
 *  c) Write Thread Network Key characteristic  (16-byte master key, write-only).
 *  d) Optionally write Channel (1 byte, 11-26) and PAN ID (2 bytes LE).
 *  e) Write 0x01 to the Join characteristic to start Thread network join.
 *  f) Subscribe to Thread Status notifications to watch the device role.
 *
 * --- Doorbell Ring Service workflow ---
 *  a) Enable notifications on the Doorbell Ring characteristic.
 *  b) Press the analog button (GPIO 28 / ANIO0) → notification value 0x01.
 *  c) Write 0x01 remotely → board rings locally.
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "ThreadBleDoorbell_Config.h"

#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/* =========================================================================
 *  UUID definitions
 * ========================================================================= */

/* Doorbell Ring Service UUID (128-bit) : D00RBELL-0000-1000-8000-00805F9B3400 */
#define DOORBELL_SERVICE_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Doorbell Ring Characteristic UUID  : D00RBELL-0000-1000-8000-00805F9B3401 */
#define DOORBELL_RING_CHAR_UUID_128 \
    0x01, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

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
 *  Doorbell Ring Service
 * ========================================================================= */

static const uint8_t  doorbellSvcUuid[]     = {DOORBELL_SERVICE_UUID_128};
static const uint16_t doorbellSvcLen        = sizeof(doorbellSvcUuid);

static const uint8_t  doorbellRingCh[]      = {ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(DOORBELL_RING_HDL),
                                                DOORBELL_RING_CHAR_UUID_128};
static const uint16_t doorbellRingChLen     = sizeof(doorbellRingCh);

static uint8_t        doorbellRingValue[1]  = {DOORBELL_STATE_IDLE};
static const uint16_t doorbellRingValueLen  = sizeof(doorbellRingValue);

static uint8_t        doorbellRingCcc[]     = {UINT16_TO_BYTES(0x0000)};
static const uint16_t doorbellRingCccLen    = sizeof(doorbellRingCcc);

/* clang-format off */
static const attsAttr_t Doorbell_GATT_List[] = {
    { attTypePrimSvcUuid, (uint8_t*)doorbellSvcUuid, (uint16_t*)&doorbellSvcLen, sizeof(doorbellSvcUuid), ATTS_SET_UUID_128, ATTS_PERMIT_READ },
    { attTypeCharUuid,    (uint8_t*)doorbellRingCh,  (uint16_t*)&doorbellRingChLen, sizeof(doorbellRingCh), 0, ATTS_PERMIT_READ },
    { &doorbellRingCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET], doorbellRingValue, (uint16_t*)&doorbellRingValueLen, sizeof(doorbellRingValue), ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
    { attTypeCliChCfgUuid, doorbellRingCcc, (uint16_t*)&doorbellRingCccLen, sizeof(doorbellRingCcc), ATTS_SET_CCC, ATTS_PERMIT_READ | ATTS_PERMIT_WRITE },
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
static uint8_t        threadNetNameValue[THREAD_NET_NAME_MAX_LEN] = "DoorbellNet";
static uint16_t       threadNetNameValueLen = 11; /* strlen("DoorbellNet") */

/* Network Key characteristic (write-only: ATT_PROP_WRITE, no ATT_PROP_READ) */
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
    {NULL, (attsAttr_t*)Battery_GATT_List,   NULL, NULL, BATTERY_SVC_HDL,    BATTERY_LEVEL_HDL_MAX - 1  },
    {NULL, (attsAttr_t*)Doorbell_GATT_List,  NULL, NULL, DOORBELL_SVC_HDL,   DOORBELL_RING_HDL_MAX - 1  },
    {NULL, (attsAttr_t*)ThreadCfg_GATT_List, NULL, NULL, THREAD_CFG_SVC_HDL, THREAD_CFG_SVC_HDL_MAX - 1 },
};

/* CCC descriptor table: GATT SC, Battery Level, Doorbell Ring, Thread Status */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    {GATT_SC_CH_CCC_HDL,       ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {DOORBELL_RING_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
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
    'Q', 'P', 'G', ' ', 'T', 'h', 'r', 'e', 'a', 'd',
    ' ', 'D', 'o', 'o', 'r', 'b', 'e', 'l', 'l'
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
