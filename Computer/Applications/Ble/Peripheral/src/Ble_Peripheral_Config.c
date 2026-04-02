/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
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

/** @file "Ble_Peripheral_Config.c"
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "Ble_Peripheral_Config.h"

#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

// Full list of avaialable standard attributes in att_uuid.h of Host stack

/* Custom LED Service and Characteristic UUIDs (128-bit) */
/* LED Service UUID: 12345678-1234-5678-9abc-123456789abc */
#define LED_SERVICE_UUID_128                                                                                           \
    0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12

/* LED Control Characteristic UUID: 87654321-4321-8765-cba9-987654321cba */
#define LED_CONTROL_CHAR_UUID_128                                                                                      \
    0xba, 0x1c, 0x32, 0x54, 0x76, 0x98, 0xa9, 0xcb, 0x65, 0x87, 0x21, 0x43, 0x21, 0x43, 0x65, 0x87

/* Attribute types */
static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeChUserDescUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CHAR_USER_DESC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/*****************************************************************************
 *                    Battery Service Definitions
 *****************************************************************************/

/* Battery service declaration */
static const uint8_t batterySvcUuid[] = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen = sizeof(batterySvcUuid);

/* Battery Level characteristic */
static const uint8_t batteryCh[] = {ATT_PROP_READ | ATT_PROP_NOTIFY, UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
                                    UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t batteryChLen = sizeof(batteryCh);

static uint8_t batteryChValue[1] = {100}; /* Initial battery level 100% */
static const uint16_t batteryChValueLen = sizeof(batteryChValue);

static uint8_t batteryLevelChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelLenChCcc = sizeof(batteryLevelChCcc);

/* clang-format off */
/* ---------------- */

static const attsAttr_t Battery_GATT_List[] = {
    /* Battery Service */
    {
        /* pUuid = "Primary Service" TYPE */
        .pUuid = attTypePrimSvcUuid,
        /* pValue = Battery Service UUID DATA */
        .pValue = (uint8_t*)batterySvcUuid,
        /* pLen = length of Battery Service UUID value */
        .pLen = (uint16_t*)&batterySvcLen,
        /* maxLen bytes in DATA (value) */
        .maxLen = sizeof(batterySvcUuid),
        /* Attribute settings */
        .settings = 0,
        /* R/W permissions */
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level Characteristic Declaration */
    {
        /* UUID = "Characteristic" TYPE */
        .pUuid = attTypeCharUuid,
        /* VALUE = [properties + handle + char UUID] */
        .pValue = (uint8_t*)batteryCh,
        /* pLen = length of value */
        .pLen = (uint16_t*)&batteryChLen,
        /* maxLen bytes in DATA (value) */
        .maxLen = sizeof(batteryCh),
        /* Attribute settings */
        .settings = 0,
        /* R/W permissions */
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level Characteristic Value */
    {
        .pUuid = &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue = (uint8_t*)batteryChValue,
        .pLen = (uint16_t*)&batteryChValueLen,
        .maxLen = sizeof(batteryChValue),
        .settings = ATTS_SET_READ_CBACK,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level CCC Descriptor */
    {
        .pUuid = attTypeCliChCfgUuid,
        .pValue = batteryLevelChCcc,
        .pLen = (uint16_t*)&batteryLevelLenChCcc,
        .maxLen = sizeof(batteryLevelChCcc),
        .settings = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};

/* clang-format on */
/* ---------------- */

/*****************************************************************************
 *                    LED Service Definitions
 *****************************************************************************/

/* LED Service declarations */
static const uint8_t ledSvcUuid[] = {LED_SERVICE_UUID_128};
static const uint16_t ledSvcLen = sizeof(ledSvcUuid);

/* LED Control Characteristic declarations */
// static uint8_t ledControlCharUuid[] = {LED_CONTROL_CHAR_UUID_128};
static const uint8_t ledControlCh[] = {ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_NOTIFY,
                                       UINT16_TO_BYTES(LED_CONTROL_HDL), LED_CONTROL_CHAR_UUID_128};
static const uint16_t ledControlChLen = sizeof(ledControlCh);

static uint8_t ledControlValue[1] = {0}; /* Initial LED state: OFF */
static const uint16_t ledControlValueLen = sizeof(ledControlValue);

static uint8_t ledControlChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t ledControlLenChCcc = sizeof(ledControlChCcc);

/* clang-format off */
/* ---------------- */

static const attsAttr_t LedControl_GATT_List[] = {
    /* LED Service */
    {
        /* pUuid = "Primary Service" TYPE */
        .pUuid = attTypePrimSvcUuid,
        /* pValue = LED Service UUID DATA */
        .pValue = (uint8_t*)ledSvcUuid,
        /* pLen = length of LED Service UUID value */
        .pLen = (uint16_t*)&ledSvcLen,
        /* maxLen bytes in DATA (value) */
        .maxLen = sizeof(ledSvcUuid),
        /* Attribute settings */
        .settings = 0,
        /* R/W permissions */
        .permissions = ATTS_PERMIT_READ
    },
    /* LED Control Characteristic Declaration */
    {
        /* UUID = "Characteristic" TYPE */
        .pUuid = attTypeCharUuid,
        /* VALUE = [properties + handle + char UUID] */
        .pValue = (uint8_t*)ledControlCh,
        /* pLen = length of value */
        .pLen = (uint16_t*)&ledControlChLen,
        /* maxLen bytes in DATA (value) */
        .maxLen = sizeof(ledControlCh),
        /* Attribute settings */
        .settings = 0,
        /* R/W permissions */
        .permissions = ATTS_PERMIT_READ
    },
    /* LED Control Characteristic Value */
    {
        .pUuid = &ledControlCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue = ledControlValue,
        .pLen = (uint16_t*)&ledControlValueLen,
        .maxLen = sizeof(ledControlValue),
        .settings = ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
    /* LED Control CCC Descriptor */
    {
        .pUuid = attTypeCliChCfgUuid,
        .pValue = ledControlChCcc,
        .pLen = (uint16_t*)&ledControlLenChCcc,
        .maxLen = sizeof(ledControlChCcc),
        .settings = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};

/*****************************************************************************
 *                    Global definitions
 *****************************************************************************/

/* ███████████████████████   Mandatory global variable for BleIf.c    █████████████████████████████████ */
attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    {NULL, (attsAttr_t*)Battery_GATT_List, NULL, NULL, BATTERY_SVC_HDL, BATTERY_LEVEL_HDL_MAX - 1},
    {NULL, (attsAttr_t*)LedControl_GATT_List, NULL, NULL, LED_CONTROL_SVC_HDL, LED_CONTROL_HDL_MAX - 1}
};

/* ███████████████████████   Mandatory global variable for BleIf.c    █████████████████████████████████ */
/*
 * Table defining Client Characteristic Configuration (CCCD) settings for all GATT attributes
 * that support notifications or indications. Used by the BLE stack to manage CCCD state
 * and security requirements for each characteristic.
 */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    /* cccd handle          value range               security level */
    {GATT_SC_CH_CCC_HDL, ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE},
    {LED_CONTROL_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE}
};

/* clang-format on */
/* ---------------- */

/*****************************************************************************
 *                    Advertising Definitions
 *****************************************************************************/

/* clang-format off */
/* ---------------- */

/*
 * BLE Advertisement Data Structure (31 bytes max for legacy advertising)
 * Applies to Scan Response and Advertising frames
 * ┌────────────────────────────────────────────────────────────────────┐
 * │ AD Elements:                                                       │
 * │                                                                    │
 * │ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐        │
 * │ │[Len][Type][Data]│ │[Len][Type][Data]│ │[Len][Type][Data]│  ...   │
 * │ └─────────────────┘ └─────────────────┘ └─────────────────┘        │
 * │    AD Element 1       AD Element 2       AD Element 3              │
 * │                                                                    │
 * │ Each AD Element:                                                   │
 * │ • Length  = Number of bytes that follow (Type + Data)              │
 * │ • AD Type = Advertisement Data Type (see Bluetooth spec)           │
 * │ • AD Data = Type-specific payload (little-endian for multi-byte)   │
 * └────────────────────────────────────────────────────────────────────┘
 */

const uint8_t DefaultAdvDataFrame[] = {
    0x02,                      /* length */
    BLEIF_ADV_DATA_TYPE_FLAGS, /* type */
    0x06,                      /* value (General Discoverable Mode; BLE only) */
    /* ------------------ */
    0x05,                                  /* length */
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE, /* type (complete list of services) */
    batterySvcUuid[0],                     /* value (Battery Svc UUID) */
    batterySvcUuid[1],                     /* value (Battery Svc UUID) */
    ledSvcUuid[0],                         /* value (LED Svc UUID) */
    ledSvcUuid[1],                         /* value (LED Svc UUID) */
    /* ------------------ */
};

const uint8_t ScanRespFrame[] = {
    0x10,                               /* length */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE,  /* type */
    'q', 'B', 'L', 'E', ' ', 'p', 'e', 'r', 'i', 'p', 'h', 'e', 'r', 'a', 'l'
    /* ------------------ */
};

/* clang-format on */
/* ---------------- */

/*****************************************************************************
 *                    Public Function Definitions
 *****************************************************************************/
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
