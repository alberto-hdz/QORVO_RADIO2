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

/** @file "BleDoorbell_Config.c"
 *
 * BLE GATT attribute table for the QPG6200 BLE Doorbell demo.
 *
 * GATT Services:
 *   1. Battery Service  (UUID 0x180F) - standard; visible in nRF Connect
 *   2. Doorbell Service (custom 128-bit UUID) - ring notifications
 *
 * How to use with nRF Connect / Qorvo Connect:
 *   - Scan for "QPG Doorbell"
 *   - Connect and browse GATT services
 *   - Enable notifications on the "Doorbell Ring" characteristic
 *   - Press PB5 on the board to see a ring notification (value 0x01)
 *   - Write 0x01 to remotely trigger a ring on the board
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "BleDoorbell_Config.h"

#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/* -------------------------------------------------------------------------
 * Doorbell Service UUID (128-bit, custom)
 * UUID: D00RBELL-0000-1000-8000-00805F9B3400
 * Byte array (little-endian):
 * ------------------------------------------------------------------------- */
#define DOORBELL_SERVICE_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Doorbell Ring Characteristic UUID (128-bit, custom)
 * UUID: D00RBELL-0000-1000-8000-00805F9B3401 */
#define DOORBELL_RING_CHAR_UUID_128 \
    0x01, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x11, 0xBE, 0x00, 0xD0

/* Standard GATT attribute type UUIDs */
static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN]   = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN]      = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN]  = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/*****************************************************************************
 *                    Battery Service
 * Standard Bluetooth Battery Service - shows up automatically in nRF Connect
 *****************************************************************************/

static const uint8_t batterySvcUuid[]      = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen        = sizeof(batterySvcUuid);

static const uint8_t batteryCh[]           = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                               UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
                                               UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t batteryChLen         = sizeof(batteryCh);

static uint8_t batteryChValue[1]           = {100};   /* Fixed at 100% for demo */
static const uint16_t batteryChValueLen    = sizeof(batteryChValue);

static uint8_t batteryLevelChCcc[]         = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelLenChCcc = sizeof(batteryLevelChCcc);

/* clang-format off */
static const attsAttr_t Battery_GATT_List[] = {
    /* Battery Service Declaration */
    {
        .pUuid       = attTypePrimSvcUuid,
        .pValue      = (uint8_t*)batterySvcUuid,
        .pLen        = (uint16_t*)&batterySvcLen,
        .maxLen      = sizeof(batterySvcUuid),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level Characteristic Declaration */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)batteryCh,
        .pLen        = (uint16_t*)&batteryChLen,
        .maxLen      = sizeof(batteryCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level Characteristic Value */
    {
        .pUuid       = &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = (uint8_t*)batteryChValue,
        .pLen        = (uint16_t*)&batteryChValueLen,
        .maxLen      = sizeof(batteryChValue),
        .settings    = ATTS_SET_READ_CBACK,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level CCC Descriptor */
    {
        .pUuid       = attTypeCliChCfgUuid,
        .pValue      = batteryLevelChCcc,
        .pLen        = (uint16_t*)&batteryLevelLenChCcc,
        .maxLen      = sizeof(batteryLevelChCcc),
        .settings    = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};
/* clang-format on */

/*****************************************************************************
 *                    Doorbell Service
 * Custom service that sends ring notifications and accepts remote ring commands.
 *
 * "Doorbell Ring" Characteristic:
 *   - Read:   returns current state (0x00=idle, 0x01=ringing)
 *   - Notify: board sends 0x01 when PB5 is pressed
 *   - Write:  phone sends 0x01 to trigger a ring on the board
 *****************************************************************************/

static const uint8_t doorbellSvcUuid[]      = {DOORBELL_SERVICE_UUID_128};
static const uint16_t doorbellSvcLen        = sizeof(doorbellSvcUuid);

static const uint8_t doorbellRingCh[]       = {ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(DOORBELL_RING_HDL),
                                                DOORBELL_RING_CHAR_UUID_128};
static const uint16_t doorbellRingChLen     = sizeof(doorbellRingCh);

static uint8_t doorbellRingValue[1]         = {DOORBELL_STATE_IDLE};
static const uint16_t doorbellRingValueLen  = sizeof(doorbellRingValue);

static uint8_t doorbellRingChCcc[]          = {UINT16_TO_BYTES(0x0000)};
static const uint16_t doorbellRingLenChCcc  = sizeof(doorbellRingChCcc);

/* clang-format off */
static const attsAttr_t Doorbell_GATT_List[] = {
    /* Doorbell Service Declaration */
    {
        .pUuid       = attTypePrimSvcUuid,
        .pValue      = (uint8_t*)doorbellSvcUuid,
        .pLen        = (uint16_t*)&doorbellSvcLen,
        .maxLen      = sizeof(doorbellSvcUuid),
        .settings    = ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },
    /* Doorbell Ring Characteristic Declaration */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)doorbellRingCh,
        .pLen        = (uint16_t*)&doorbellRingChLen,
        .maxLen      = sizeof(doorbellRingCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Doorbell Ring Characteristic Value */
    {
        .pUuid       = &doorbellRingCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = doorbellRingValue,
        .pLen        = (uint16_t*)&doorbellRingValueLen,
        .maxLen      = sizeof(doorbellRingValue),
        .settings    = ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
    /* Doorbell Ring CCC Descriptor */
    {
        .pUuid       = attTypeCliChCfgUuid,
        .pValue      = doorbellRingChCcc,
        .pLen        = (uint16_t*)&doorbellRingLenChCcc,
        .maxLen      = sizeof(doorbellRingChCcc),
        .settings    = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};
/* clang-format on */

/*****************************************************************************
 *                    Mandatory Global Variables (read by BleIf.c)
 *****************************************************************************/

/* GATT service group table - BleIf registers all services here */
attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    {NULL, (attsAttr_t*)Battery_GATT_List,  NULL, NULL, BATTERY_SVC_HDL,  BATTERY_LEVEL_HDL_MAX - 1},
    {NULL, (attsAttr_t*)Doorbell_GATT_List, NULL, NULL, DOORBELL_SVC_HDL, DOORBELL_RING_HDL_MAX - 1},
};

/* CCC descriptor table - tracks which clients subscribed to notifications */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    /* cccd handle             value range               security level */
    {GATT_SC_CH_CCC_HDL,     ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {DOORBELL_RING_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
};

/*****************************************************************************
 *                    Advertising Data
 *
 * Advertising packet (max 31 bytes):
 *   [Flags] + [Battery Service UUID 16-bit]
 *
 * Scan response (max 31 bytes):
 *   [Complete Local Name: "QPG Doorbell"]
 *****************************************************************************/

static const uint8_t DefaultAdvDataFrame[] = {
    /* AD Element: Flags */
    0x02,                        /* length */
    BLEIF_ADV_DATA_TYPE_FLAGS,   /* type */
    0x06,                        /* General Discoverable + BLE only */
    /* AD Element: 16-bit Service UUIDs (Battery Service) */
    0x03,                                    /* length: 2 bytes UUID + type */
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE,     /* type */
    batterySvcUuid[0],                       /* Battery Service UUID LSB */
    batterySvcUuid[1],                       /* Battery Service UUID MSB */
};

/* Scan response: device name visible in nRF Connect / Qorvo Connect */
static const uint8_t ScanRespFrame[] = {
    0x0D,                               /* length: 12 chars + type = 13 */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE,  /* type: Complete Local Name */
    'Q', 'P', 'G', ' ', 'D', 'o', 'o', 'r', 'b', 'e', 'l', 'l'
};

/*****************************************************************************
 *                    Public Functions
 *****************************************************************************/

/* BleIf.c calls these functions by name - do not rename */
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

/* Aliases for calling from AppManager */
uint8_t BleDoorbell_Config_Load_Advertise_Frame(uint8_t* buffer)
{
    return Ble_Peripheral_Config_Load_Advertise_Frame(buffer);
}

uint8_t BleDoorbell_Config_Load_Scan_Response_Frame(uint8_t* buffer)
{
    return Ble_Peripheral_Config_Load_Scan_Response_Frame(buffer);
}
