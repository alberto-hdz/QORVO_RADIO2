/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * Ble_Peripheral_Config.c (for LedRX - LED Receiver)
 * 
 * This file configures:
 * - BLE Services and Characteristics
 * - Device name: "LedRX"
 * - LED Control characteristic that accepts WRITES to control the LED
 */

#include "BleIf.h"
#include "bstream.h"
#include "qReg.h"
#include "Ble_Peripheral_Config.h"

#define BLE_CHARACTERISTIC_VALUE_UUID_OFFSET 3

/*
 * ===========================================================================
 * CUSTOM SERVICE UUIDs (128-bit)
 * MUST MATCH the transmitter (ButtonTX) UUIDs!
 * ===========================================================================
 */

/* LED Service UUID: 12345678-1234-5678-9abc-123456789abc (little-endian) */
#define LED_SERVICE_UUID_128 \
    0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0xbc, 0x9a, \
    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12

/* LED Control Characteristic UUID: 87654321-4321-8765-cba9-987654321cba (little-endian) */
#define LED_CONTROL_CHAR_UUID_128 \
    0xba, 0x1c, 0x32, 0x54, 0x76, 0x98, 0xa9, 0xcb, \
    0x65, 0x87, 0x21, 0x43, 0x21, 0x43, 0x65, 0x87

/*
 * ===========================================================================
 * STANDARD ATTRIBUTE TYPE UUIDs (16-bit)
 * ===========================================================================
 */
static const uint8_t attTypePrimSvcUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_PRIMARY_SERVICE)};
static const uint8_t attTypeCharUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CHARACTERISTIC)};
static const uint8_t attTypeChUserDescUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CHAR_USER_DESC)};
static const uint8_t attTypeCliChCfgUuid[ATT_16_UUID_LEN] = {UINT16_TO_BYTES(ATT_UUID_CLIENT_CHAR_CONFIG)};

/*
 * ===========================================================================
 * BATTERY SERVICE
 * ===========================================================================
 */
static const uint8_t batterySvcUuid[] = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t batterySvcLen = sizeof(batterySvcUuid);

static const uint8_t batteryCh[] = {
    ATT_PROP_READ | ATT_PROP_NOTIFY,
    UINT16_TO_BYTES(BATTERY_LEVEL_HDL),
    UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)
};
static const uint16_t batteryChLen = sizeof(batteryCh);

static uint8_t batteryChValue[1] = {100};
static const uint16_t batteryChValueLen = sizeof(batteryChValue);

static uint8_t batteryLevelChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t batteryLevelLenChCcc = sizeof(batteryLevelChCcc);

static const attsAttr_t Battery_GATT_List[] = {
    {
        .pUuid = attTypePrimSvcUuid,
        .pValue = (uint8_t*)batterySvcUuid,
        .pLen = (uint16_t*)&batterySvcLen,
        .maxLen = sizeof(batterySvcUuid),
        .settings = 0,
        .permissions = ATTS_PERMIT_READ
    },
    {
        .pUuid = attTypeCharUuid,
        .pValue = (uint8_t*)batteryCh,
        .pLen = (uint16_t*)&batteryChLen,
        .maxLen = sizeof(batteryCh),
        .settings = 0,
        .permissions = ATTS_PERMIT_READ
    },
    {
        .pUuid = &batteryCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue = (uint8_t*)batteryChValue,
        .pLen = (uint16_t*)&batteryChValueLen,
        .maxLen = sizeof(batteryChValue),
        .settings = ATTS_SET_READ_CBACK,
        .permissions = ATTS_PERMIT_READ
    },
    {
        .pUuid = attTypeCliChCfgUuid,
        .pValue = batteryLevelChCcc,
        .pLen = (uint16_t*)&batteryLevelLenChCcc,
        .maxLen = sizeof(batteryLevelChCcc),
        .settings = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};

/*
 * ===========================================================================
 * LED CONTROL SERVICE (Custom)
 * 
 * This is the main service for receiving LED commands.
 * When a phone/central writes to this characteristic, we control the LED.
 * ===========================================================================
 */
static const uint8_t ledSvcUuid[] = {LED_SERVICE_UUID_128};
static const uint16_t ledSvcLen = sizeof(ledSvcUuid);

/* LED Control characteristic - READ and WRITE enabled */
static const uint8_t ledControlCh[] = {
    ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_WRITE_NO_RSP,
    UINT16_TO_BYTES(LED_CONTROL_HDL),
    LED_CONTROL_CHAR_UUID_128
};
static const uint16_t ledControlChLen = sizeof(ledControlCh);

/* LED Control value (default: 0 = OFF) */
static uint8_t ledControlValue[1] = {0};
static const uint16_t ledControlValueLen = sizeof(ledControlValue);

/* LED Control CCCD (not really needed for receiver, but kept for compatibility) */
static uint8_t ledControlChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t ledControlLenChCcc = sizeof(ledControlChCcc);

static const attsAttr_t LedControl_GATT_List[] = {
    /* Service Declaration */
    {
        .pUuid = attTypePrimSvcUuid,
        .pValue = (uint8_t*)ledSvcUuid,
        .pLen = (uint16_t*)&ledSvcLen,
        .maxLen = sizeof(ledSvcUuid),
        .settings = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Characteristic Declaration */
    {
        .pUuid = attTypeCharUuid,
        .pValue = (uint8_t*)ledControlCh,
        .pLen = (uint16_t*)&ledControlChLen,
        .maxLen = sizeof(ledControlCh),
        .settings = 0,
        .permissions = ATTS_PERMIT_READ
    },
    /* Characteristic Value - WRITABLE for receiving LED commands */
    {
        .pUuid = &ledControlCh[BLE_CHARACTERISTIC_VALUE_UUID_OFFSET],
        .pValue = ledControlValue,
        .pLen = (uint16_t*)&ledControlValueLen,
        .maxLen = sizeof(ledControlValue),
        .settings = ATTS_SET_WRITE_CBACK | ATTS_SET_UUID_128,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
    /* CCCD */
    {
        .pUuid = attTypeCliChCfgUuid,
        .pValue = ledControlChCcc,
        .pLen = (uint16_t*)&ledControlLenChCcc,
        .maxLen = sizeof(ledControlChCcc),
        .settings = ATTS_SET_CCC,
        .permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
    },
};

/*
 * ===========================================================================
 * GLOBAL SERVICE GROUPS
 * ===========================================================================
 */
attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS] = {
    {NULL, (attsAttr_t*)Battery_GATT_List, NULL, NULL, BATTERY_SVC_HDL, BATTERY_LEVEL_HDL_MAX - 1},
    {NULL, (attsAttr_t*)LedControl_GATT_List, NULL, NULL, LED_CONTROL_SVC_HDL, LED_CONTROL_HDL_MAX - 1}
};

/*
 * ===========================================================================
 * CCCD CONFIGURATION TABLE
 * ===========================================================================
 */
attsCccSet_t BleIf_CccSet[NUM_CCC_IDX] = {
    {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {BATTERY_LEVEL_CCC_HDL, ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE},
    {LED_CONTROL_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,   DM_SEC_LEVEL_NONE}
};

/*
 * ===========================================================================
 * ADVERTISING DATA
 * ===========================================================================
 */
const uint8_t DefaultAdvDataFrame[] = {
    /* Flags */
    0x02,
    BLEIF_ADV_DATA_TYPE_FLAGS,
    0x06,
    
    /* Service UUIDs */
    0x05,
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE,
    batterySvcUuid[0], batterySvcUuid[1],
    ledSvcUuid[0], ledSvcUuid[1],
};

/*
 * ===========================================================================
 * SCAN RESPONSE DATA
 * 
 * DEVICE NAME: "LedRX" (LED Receiver)
 * ===========================================================================
 */
const uint8_t ScanRespFrame[] = {
    0x06,                               /* Length: 5 chars + 1 type byte = 6 */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE,  /* Type: Complete Local Name */
    'L', 'e', 'd', 'R', 'X'
};

/*
 * ===========================================================================
 * PUBLIC FUNCTIONS
 * ===========================================================================
 */

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