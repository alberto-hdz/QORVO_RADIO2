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
 * @file BleDoorbellGW_Config.c
 * @brief GATT attribute database and BLE advertising payloads for the
 *        BLE Gateway Doorbell application.
 *
 * Defines two GATT services:
 *
 *   1. Battery Service (SIG 0x180F)
 *        Battery Level (0x2A19): Read / Notify – fixed at 100 % for demo
 *
 *   2. Doorbell Gateway Service (custom 128-bit UUIDs)
 *        Doorbell Ring : Read / Notify  (0x00 = idle, 0x01 = ringing)
 *        LED Control   : Write          (0x00 = off,  0x01 = on)
 *        Heartbeat     : Read / Notify  (1-byte counter, wraps 255 → 0)
 *
 * UUIDs (big-endian for readability, stored little-endian in firmware):
 *   Doorbell Service  : C000C001-0000-1000-8000-00805F9B3400
 *   Doorbell Ring     : C000C002-0000-1000-8000-00805F9B3400
 *   LED Control       : C000C003-0000-1000-8000-00805F9B3400
 *   Heartbeat         : C000C004-0000-1000-8000-00805F9B3400
 *
 * The two mandatory global arrays required by BleIf.c:
 *   svcGroups[]    – GATT service group descriptors
 *   BleIf_CccSet[] – CCC table for notifiable characteristics
 *
 * Testing with nRF Connect:
 *   1. Scan for "QPG Doorbell GW" and connect.
 *   2. Find the custom service and enable notifications on Doorbell Ring.
 *   3. Press PB5 on the board to see ring events (0x01 on press, 0x00 on release).
 *   4. Write 0x01 to LED Control to turn on the BLUE LED from the app.
 *   5. Observe the Heartbeat characteristic incrementing every 5 seconds.
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "Ble_Peripheral_Config.h"

/* Offset of the characteristic value UUID within a characteristic declaration. */
#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/* -------------------------------------------------------------------------- */
/*  Standard GATT attribute type UUIDs                                        */
/* -------------------------------------------------------------------------- */

static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN]  = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN]     = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/* ==========================================================================
 *  Service 1 – Battery Service (SIG standard, UUID 0x180F)
 * ========================================================================== */

static const uint8_t  batterySvcUuid[]      = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen         = sizeof(batterySvcUuid);

/* Battery Level characteristic declaration: [properties | handle | UUID] */
static const uint8_t  batteryCh[]           = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
                                                UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t batteryChLen          = sizeof(batteryCh);

/* Fixed at 100 % – demo board is assumed always powered. */
static uint8_t        batteryChValue[1]     = {100};
static const uint16_t batteryChValueLen     = sizeof(batteryChValue);

/* CCC descriptor – notifications disabled by default. */
static uint8_t        batteryLevelChCcc[]   = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelLenChCcc  = sizeof(batteryLevelChCcc);

/* clang-format off */
static const attsAttr_t Battery_GATT_List[] = {
    /* Battery Service declaration */
    {
        .pUuid       = attTypePrimSvcUuid,
        .pValue      = (uint8_t*)batterySvcUuid,
        .pLen        = (uint16_t*)&batterySvcLen,
        .maxLen      = sizeof(batterySvcUuid),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level characteristic declaration */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)batteryCh,
        .pLen        = (uint16_t*)&batteryChLen,
        .maxLen      = sizeof(batteryCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level characteristic value */
    {
        .pUuid       = &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = (uint8_t*)batteryChValue,
        .pLen        = (uint16_t*)&batteryChValueLen,
        .maxLen      = sizeof(batteryChValue),
        .settings    = ATTS_SET_READ_CBACK,
        .permissions = ATTS_PERMIT_READ
    },
    /* Battery Level CCC descriptor */
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

/* ==========================================================================
 *  Service 2 – Doorbell Gateway Service (custom 128-bit UUIDs)
 *
 *  UUIDs (big-endian notation):
 *    Doorbell Service  : C000C001-0000-1000-8000-00805F9B3400
 *    Doorbell Ring     : C000C002-0000-1000-8000-00805F9B3400
 *    LED Control       : C000C003-0000-1000-8000-00805F9B3400
 *    Heartbeat         : C000C004-0000-1000-8000-00805F9B3400
 *
 *  Stored in little-endian byte order as required by the Bluetooth spec.
 * ========================================================================== */

/* Doorbell Gateway Service UUID – 128-bit, little-endian */
#define DOORBELL_GW_SERVICE_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x01, 0xC0, 0x00, 0xC0

/* Doorbell Ring characteristic UUID (byte 13 = 0xC0 → 0x02) */
#define DOORBELL_RING_CHAR_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x02, 0xC0, 0x00, 0xC0

/* LED Control characteristic UUID (byte 13 = 0xC0 → 0x03) */
#define LED_CONTROL_CHAR_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x03, 0xC0, 0x00, 0xC0

/* Heartbeat characteristic UUID (byte 13 = 0xC0 → 0x04) */
#define HEARTBEAT_CHAR_UUID_128 \
    0x00, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
    0x00, 0x10, 0x00, 0x00, 0x04, 0xC0, 0x00, 0xC0

/* ---- Doorbell Gateway Service declaration ---- */
static const uint8_t  doorbellSvcUuid[]     = {DOORBELL_GW_SERVICE_UUID_128};
static const uint16_t doorbellSvcLen        = sizeof(doorbellSvcUuid);

/* ---- Doorbell Ring characteristic ---- */
/* Declaration: [properties | handle (2 B) | UUID (16 B)] */
static const uint8_t  doorbellRingCh[]      = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(DOORBELL_RING_HDL),
                                                DOORBELL_RING_CHAR_UUID_128};
static const uint16_t doorbellRingChLen     = sizeof(doorbellRingCh);

/* Starts idle; updated to 0x01 on button press, 0x00 on release. */
static uint8_t        doorbellRingValue[1]  = {DOORBELL_STATE_IDLE};
static const uint16_t doorbellRingValueLen  = sizeof(doorbellRingValue);

/* CCC – gateway writes 0x0001 to enable doorbell ring notifications. */
static uint8_t        doorbellRingChCcc[]   = {UINT16_TO_BYTES(0x0000)};
static const uint16_t doorbellRingLenChCcc  = sizeof(doorbellRingChCcc);

/* ---- LED Control characteristic ---- */
static const uint8_t  ledControlCh[]        = {ATT_PROP_WRITE,
                                                UINT16_TO_BYTES(LED_CONTROL_HDL),
                                                LED_CONTROL_CHAR_UUID_128};
static const uint16_t ledControlChLen       = sizeof(ledControlCh);

/* Gateway writes 0x01 (on) or 0x00 (off) to control the BLUE LED. */
static uint8_t        ledControlValue[1]    = {0x00};
static const uint16_t ledControlValueLen    = sizeof(ledControlValue);

/* ---- Heartbeat characteristic ---- */
static const uint8_t  heartbeatCh[]         = {ATT_PROP_READ | ATT_PROP_NOTIFY,
                                                UINT16_TO_BYTES(HEARTBEAT_HDL),
                                                HEARTBEAT_CHAR_UUID_128};
static const uint16_t heartbeatChLen        = sizeof(heartbeatCh);

/* 1-byte counter starting at 0; incremented every 5 seconds. */
static uint8_t        heartbeatValue[1]     = {0x00};
static const uint16_t heartbeatValueLen     = sizeof(heartbeatValue);

/* CCC – gateway writes 0x0001 to subscribe to heartbeat notifications. */
static uint8_t        heartbeatChCcc[]      = {UINT16_TO_BYTES(0x0000)};
static const uint16_t heartbeatLenChCcc     = sizeof(heartbeatChCcc);

/* clang-format off */
static const attsAttr_t DoorbellGW_GATT_List[] = {
    /* ---- Doorbell Gateway Service declaration ---- */
    {
        .pUuid       = attTypePrimSvcUuid,
        .pValue      = (uint8_t*)doorbellSvcUuid,
        .pLen        = (uint16_t*)&doorbellSvcLen,
        .maxLen      = sizeof(doorbellSvcUuid),
        .settings    = ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },

    /* ---- Doorbell Ring characteristic declaration ---- */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)doorbellRingCh,
        .pLen        = (uint16_t*)&doorbellRingChLen,
        .maxLen      = sizeof(doorbellRingCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Doorbell Ring value – notified to gateway on button events */
    {
        .pUuid       = &doorbellRingCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = doorbellRingValue,
        .pLen        = (uint16_t*)&doorbellRingValueLen,
        .maxLen      = sizeof(doorbellRingValue),
        .settings    = ATTS_SET_READ_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },
    /* Doorbell Ring CCC – gateway enables notifications here */
    {
        .pUuid       = attTypeCliChCfgUuid,
        .pValue      = doorbellRingChCcc,
        .pLen        = (uint16_t*)&doorbellRingLenChCcc,
        .maxLen      = sizeof(doorbellRingChCcc),
        .settings    = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },

    /* ---- LED Control characteristic declaration ---- */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)ledControlCh,
        .pLen        = (uint16_t*)&ledControlChLen,
        .maxLen      = sizeof(ledControlCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* LED Control value – gateway writes to change the BLUE LED state */
    {
        .pUuid       = &ledControlCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = ledControlValue,
        .pLen        = (uint16_t*)&ledControlValueLen,
        .maxLen      = sizeof(ledControlValue),
        .settings    = ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_WRITE
    },

    /* ---- Heartbeat characteristic declaration ---- */
    {
        .pUuid       = attTypeCharUuid,
        .pValue      = (uint8_t*)heartbeatCh,
        .pLen        = (uint16_t*)&heartbeatChLen,
        .maxLen      = sizeof(heartbeatCh),
        .settings    = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Heartbeat value – notified every 5 seconds */
    {
        .pUuid       = &heartbeatCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue      = heartbeatValue,
        .pLen        = (uint16_t*)&heartbeatValueLen,
        .maxLen      = sizeof(heartbeatValue),
        .settings    = ATTS_SET_READ_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ
    },
    /* Heartbeat CCC – gateway enables notifications here */
    {
        .pUuid       = attTypeCliChCfgUuid,
        .pValue      = heartbeatChCcc,
        .pLen        = (uint16_t*)&heartbeatLenChCcc,
        .maxLen      = sizeof(heartbeatChCcc),
        .settings    = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};
/* clang-format on */

/* ==========================================================================
 *  Mandatory globals required by BleIf.c
 * ========================================================================== */

/**
 * @brief GATT service group descriptors.
 *
 * BleIf.c iterates this array to register all services with the ATT server.
 * The array size must equal BLE_CONFIG_SVC_GROUPS.
 */
attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    /* Battery Service */
    {NULL, (attsAttr_t*)Battery_GATT_List,    NULL, NULL,
     BATTERY_SVC_HDL,  BATTERY_LEVEL_HDL_MAX - 1},

    /* Doorbell Gateway Service */
    {NULL, (attsAttr_t*)DoorbellGW_GATT_List, NULL, NULL,
     DOORBELL_SVC_HDL, DOORBELL_SVC_HDL_MAX - 1},
};

/**
 * @brief CCC descriptor configuration table.
 *
 * One entry per notifiable characteristic.  The order must match
 * the *_CCC_IDX enumeration in BleDoorbellGW_Config.h.
 */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    /* handle                   value range                security         */
    {GATT_SC_CH_CCC_HDL,        ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL,     ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},
    {DOORBELL_RING_CCC_HDL,     ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},
    {HEARTBEAT_CCC_HDL,         ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},
};

/* ==========================================================================
 *  BLE advertising and scan-response payloads
 * ==========================================================================
 *
 * Advertising data frame (max 31 bytes for legacy advertising):
 *   [02][01][06]         – Flags: General Discoverable, BLE only
 *   [03][02][0F][18]     – Complete list of 16-bit UUIDs: Battery Service
 *
 * Scan-response frame carries the device name "QPG Doorbell GW" (15 chars).
 *   Length byte = 1 (type) + 15 (name) = 16 = 0x10
 */

static const uint8_t DefaultAdvDataFrame[] = {
    /* AD element 1 – Flags */
    0x02,                         /* length                                  */
    BLEIF_ADV_DATA_TYPE_FLAGS,    /* type                                    */
    0x06,                         /* General Discoverable | LE-only          */

    /* AD element 2 – Complete list of 16-bit service UUIDs (Battery Service) */
    0x03,                                  /* length                         */
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE,   /* type                           */
    batterySvcUuid[0], batterySvcUuid[1],  /* 0x180F Battery Service UUID    */
};

/* "QPG Doorbell GW" = 15 characters → length byte = 16 = 0x10 */
static const uint8_t ScanRespFrame[] = {
    0x10,                               /* length                            */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE,  /* type: Complete Local Name         */
    'Q','P','G',' ','D','o','o','r','b','e','l','l',' ','G','W'
};

/* ==========================================================================
 *  Public functions called by BleIf.c
 * ========================================================================== */

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
