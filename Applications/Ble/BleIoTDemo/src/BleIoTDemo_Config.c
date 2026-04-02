/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * This software is owned by Qorvo Inc and protected under applicable copyright
 * laws. It is delivered under the terms of the license and is intended and
 * supplied for use solely and exclusively with products manufactured by
 * Qorvo Inc.
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO
 * THIS SOFTWARE. QORVO INC. SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */

/**
 * @file BleIoTDemo_Config.c
 * @brief GATT attribute database and BLE advertising payloads.
 *
 * Defines two GATT services:
 *
 *   1. Battery Service (SIG 0x180F)
 *        - Battery Level (0x2A19): Read / Notify, fixed at 100 %
 *
 *   2. IoT Demo Service (custom 128-bit UUID)
 *        - Button State : Read / Notify (0x00 released, 0x01 pressed)
 *        - LED Control  : Write        (0x00 off,      0x01 on)
 *        - Heartbeat    : Read / Notify (1-byte counter, wraps 255 → 0)
 *
 * The two mandatory global arrays required by BleIf.c are:
 *   - svcGroups[]    – GATT service group descriptors
 *   - BleIf_CccSet[] – CCC table for notifiable characteristics
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "Ble_Peripheral_Config.h"

/* Offset of the 128-bit UUID within a characteristic declaration value. */
#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/* -------------------------------------------------------------------------- */
/*  Attribute type UUIDs (SIG standard)                                       */
/* -------------------------------------------------------------------------- */

static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN]  = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN]     = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/* ========================================================================== */
/*  Service 1 – Battery Service (SIG standard, UUID 0x180F)                  */
/* ========================================================================== */

static const uint8_t  batterySvcUuid[]      = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen         = sizeof(batterySvcUuid);

/* Battery Level characteristic declaration: [properties | handle | UUID] */
static const uint8_t  batteryCh[]           = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
                                                UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t batteryChLen          = sizeof(batteryCh);

/* Fixed battery level – demo board is assumed always powered. */
static uint8_t        batteryChValue[1]     = {100};
static const uint16_t batteryChValueLen     = sizeof(batteryChValue);

/* CCC descriptor (notifications disabled by default). */
static uint8_t        batteryLevelChCcc[]   = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelLenChCcc  = sizeof(batteryLevelChCcc);

/* clang-format off */
static const attsAttr_t Battery_GATT_List[] = {
    /* Battery Service declaration */
    {
        .pUuid      = attTypePrimSvcUuid,
        .pValue     = (uint8_t*)batterySvcUuid,
        .pLen       = (uint16_t*)&batterySvcLen,
        .maxLen     = sizeof(batterySvcUuid),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level characteristic declaration */
    {
        .pUuid      = attTypeCharUuid,
        .pValue     = (uint8_t*)batteryCh,
        .pLen       = (uint16_t*)&batteryChLen,
        .maxLen     = sizeof(batteryCh),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level characteristic value */
    {
        .pUuid      = &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue     = (uint8_t*)batteryChValue,
        .pLen       = (uint16_t*)&batteryChValueLen,
        .maxLen     = sizeof(batteryChValue),
        .settings   = ATTS_SET_READ_CBACK,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level CCC descriptor */
    {
        .pUuid      = attTypeCliChCfgUuid,
        .pValue     = batteryLevelChCcc,
        .pLen       = (uint16_t*)&batteryLevelLenChCcc,
        .maxLen     = sizeof(batteryLevelChCcc),
        .settings   = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};
/* clang-format on */

/* ========================================================================== */
/*  Service 2 – IoT Demo Service (custom 128-bit UUIDs)                      */
/*                                                                            */
/*  UUIDs (big-endian notation):                                              */
/*    Demo Service  : A1B2C3D4-E5F6-7890-ABCD-EF0123456789                  */
/*    Button State  : A1B2C3D4-E5F6-7890-ABCD-EF0123456790                  */
/*    LED Control   : A1B2C3D4-E5F6-7890-ABCD-EF0123456791                  */
/*    Heartbeat     : A1B2C3D4-E5F6-7890-ABCD-EF0123456792                  */
/*                                                                            */
/*  Stored in little-endian byte order as required by the Bluetooth spec.    */
/* ========================================================================== */

/* Demo Service UUID – 128-bit, little-endian */
#define DEMO_SERVICE_UUID_128 \
    0x89, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB, \
    0x90, 0x78, 0xF6, 0xE5, 0xD4, 0xC3, 0xB2, 0xA1

/* Button State characteristic UUID – differs only in byte 0 */
#define BUTTON_STATE_CHAR_UUID_128 \
    0x90, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB, \
    0x90, 0x78, 0xF6, 0xE5, 0xD4, 0xC3, 0xB2, 0xA1

/* LED Control characteristic UUID – differs only in byte 0 */
#define LED_CONTROL_CHAR_UUID_128 \
    0x91, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB, \
    0x90, 0x78, 0xF6, 0xE5, 0xD4, 0xC3, 0xB2, 0xA1

/* Heartbeat characteristic UUID – differs only in byte 0 */
#define HEARTBEAT_CHAR_UUID_128 \
    0x92, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB, \
    0x90, 0x78, 0xF6, 0xE5, 0xD4, 0xC3, 0xB2, 0xA1

/* ---- Demo Service declaration ---- */
static const uint8_t  demoSvcUuid[]  = {DEMO_SERVICE_UUID_128};
static const uint16_t demoSvcLen     = sizeof(demoSvcUuid);

/* ---- Button State characteristic ---- */
/* Declaration: [properties | handle (2 B) | UUID (16 B)] */
static const uint8_t  buttonStateCh[]    = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                             UINT16_TO_BYTES(BUTTON_STATE_HDL),
                                             BUTTON_STATE_CHAR_UUID_128};
static const uint16_t buttonStateChLen   = sizeof(buttonStateCh);

static uint8_t        buttonStateValue[1]  = {0x00}; /* 0x00 = released */
static const uint16_t buttonStateValueLen  = sizeof(buttonStateValue);

static uint8_t        buttonStateChCcc[]   = {UINT16_TO_BYTES(0x0000)};
static const uint16_t buttonStateLenChCcc  = sizeof(buttonStateChCcc);

/* ---- LED Control characteristic ---- */
static const uint8_t  ledControlCh[]    = {ATT_PROP_WRITE,
                                            UINT16_TO_BYTES(LED_CONTROL_HDL),
                                            LED_CONTROL_CHAR_UUID_128};
static const uint16_t ledControlChLen   = sizeof(ledControlCh);

static uint8_t        ledControlValue[1]  = {0x00}; /* 0x00 = off */
static const uint16_t ledControlValueLen  = sizeof(ledControlValue);

/* ---- Heartbeat characteristic ---- */
static const uint8_t  heartbeatCh[]    = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                           UINT16_TO_BYTES(HEARTBEAT_HDL),
                                           HEARTBEAT_CHAR_UUID_128};
static const uint16_t heartbeatChLen   = sizeof(heartbeatCh);

static uint8_t        heartbeatValue[1]  = {0x00}; /* starts at 0 */
static const uint16_t heartbeatValueLen  = sizeof(heartbeatValue);

static uint8_t        heartbeatChCcc[]   = {UINT16_TO_BYTES(0x0000)};
static const uint16_t heartbeatLenChCcc  = sizeof(heartbeatChCcc);

/* clang-format off */
static const attsAttr_t IotDemo_GATT_List[] = {
    /* ---- Demo Service declaration ---- */
    {
        .pUuid      = attTypePrimSvcUuid,
        .pValue     = (uint8_t*)demoSvcUuid,
        .pLen       = (uint16_t*)&demoSvcLen,
        .maxLen     = sizeof(demoSvcUuid),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },

    /* ---- Button State characteristic declaration ---- */
    {
        .pUuid      = attTypeCharUuid,
        .pValue     = (uint8_t*)buttonStateCh,
        .pLen       = (uint16_t*)&buttonStateChLen,
        .maxLen     = sizeof(buttonStateCh),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Button State value – client can read; server sends notifications */
    {
        .pUuid      = &buttonStateCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue     = buttonStateValue,
        .pLen       = (uint16_t*)&buttonStateValueLen,
        .maxLen     = sizeof(buttonStateValue),
        .settings   = ATTS_SET_READ_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },
    /* Button State CCC – client writes 0x0001 to enable notifications */
    {
        .pUuid      = attTypeCliChCfgUuid,
        .pValue     = buttonStateChCcc,
        .pLen       = (uint16_t*)&buttonStateLenChCcc,
        .maxLen     = sizeof(buttonStateChCcc),
        .settings   = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },

    /* ---- LED Control characteristic declaration ---- */
    {
        .pUuid      = attTypeCharUuid,
        .pValue     = (uint8_t*)ledControlCh,
        .pLen       = (uint16_t*)&ledControlChLen,
        .maxLen     = sizeof(ledControlCh),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* LED Control value – client writes 0x01 (on) or 0x00 (off) */
    {
        .pUuid      = &ledControlCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue     = ledControlValue,
        .pLen       = (uint16_t*)&ledControlValueLen,
        .maxLen     = sizeof(ledControlValue),
        .settings   = ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_WRITE
    },

    /* ---- Heartbeat characteristic declaration ---- */
    {
        .pUuid      = attTypeCharUuid,
        .pValue     = (uint8_t*)heartbeatCh,
        .pLen       = (uint16_t*)&heartbeatChLen,
        .maxLen     = sizeof(heartbeatCh),
        .settings   = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Heartbeat value – client reads or subscribes for notifications */
    {
        .pUuid      = &heartbeatCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue     = heartbeatValue,
        .pLen       = (uint16_t*)&heartbeatValueLen,
        .maxLen     = sizeof(heartbeatValue),
        .settings   = ATTS_SET_READ_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },
    /* Heartbeat CCC – client writes 0x0001 to enable notifications */
    {
        .pUuid      = attTypeCliChCfgUuid,
        .pValue     = heartbeatChCcc,
        .pLen       = (uint16_t*)&heartbeatLenChCcc,
        .maxLen     = sizeof(heartbeatChCcc),
        .settings   = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};
/* clang-format on */

/* ========================================================================== */
/*  Mandatory globals required by BleIf.c                                     */
/* ========================================================================== */

/**
 * @brief GATT service group descriptors.
 *
 * BleIf.c iterates this array to register all services with the ATT server.
 * The array size must equal BLE_CONFIG_SVC_GROUPS.
 */
attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    /* Battery Service */
    {NULL, (attsAttr_t*)Battery_GATT_List, NULL, NULL,
     BATTERY_SVC_HDL, BATTERY_LEVEL_HDL_MAX - 1},

    /* IoT Demo Service */
    {NULL, (attsAttr_t*)IotDemo_GATT_List, NULL, NULL,
     DEMO_SVC_HDL, DEMO_SVC_HDL_MAX - 1},
};

/**
 * @brief CCC descriptor configuration table.
 *
 * One entry per notifiable characteristic.  The order must match
 * the *_CCC_IDX enumeration in BleIoTDemo_Config.h.
 */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    /* handle                    value range                 security */
    {GATT_SC_CH_CCC_HDL,        ATT_CLIENT_CFG_INDICATE,    DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL,     ATT_CLIENT_CFG_NOTIFY,      DM_SEC_LEVEL_NONE},
    {BUTTON_STATE_CCC_HDL,      ATT_CLIENT_CFG_NOTIFY,      DM_SEC_LEVEL_NONE},
    {HEARTBEAT_CCC_HDL,         ATT_CLIENT_CFG_NOTIFY,      DM_SEC_LEVEL_NONE},
};

/* ========================================================================== */
/*  BLE advertising and scan-response payloads                               */
/* ========================================================================== */

/*
 * Advertising data frame (max 31 bytes for legacy advertising)
 *
 *   [02] [01] [06]        – Flags: General Discoverable, BLE only
 *   [03] [02] [0F][18]    – Complete list of 16-bit UUIDs: Battery Service
 */
static const uint8_t DefaultAdvDataFrame[] = {
    /* AD element 1 – Flags */
    0x02,                         /* length  */
    BLEIF_ADV_DATA_TYPE_FLAGS,    /* type    */
    0x06,                         /* value   (General Discoverable | LE only) */

    /* AD element 2 – Complete list of 16-bit service UUIDs */
    0x03,                                          /* length  */
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE,           /* type    */
    batterySvcUuid[0], batterySvcUuid[1],          /* Battery Service 0x180F */
};

/*
 * Scan-response frame – carries the device name so it does not consume
 * advertising data space.
 *
 * "QPG IoT Demo" = 12 characters → length byte = 1 (type) + 12 = 13 = 0x0D
 */
static const uint8_t ScanRespFrame[] = {
    0x0D,                              /* length                              */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE, /* type                                */
    'Q', 'P', 'G', ' ', 'I', 'o', 'T', ' ', 'D', 'e', 'm', 'o'
};

/* ========================================================================== */
/*  Public functions called by BleIf.c                                        */
/* ========================================================================== */

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
