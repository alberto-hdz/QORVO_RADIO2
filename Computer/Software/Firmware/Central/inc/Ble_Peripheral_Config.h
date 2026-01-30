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

/** @file "Ble_Peripheral_Config.h"
 */

#ifndef _BLE_PERIPHERAL_CONFIG_H_
#define _BLE_PERIPHERAL_CONFIG_H_

#pragma once

#include "BleIf.h"

#define BLE_ADV_CHANNEL_37 0x01
#define BLE_ADV_CHANNEL_38 0x02
#define BLE_ADV_CHANNEL_39 0x04

#define BLE_ADV_INTERVAL_MIN 0x0020       // 20 ms
#define BLE_ADV_INTERVAL_MAX 0x0060       // 60 ms

#define BLE_ADV_BROADCAST_DURATION 0xF000 /* ~60 seconds */

#define BLE_CONFIG_SVC_GROUPS 2

/* Index of ccc register call back array */
enum {
    GATT_SC_CCC_IDX,       /* GATT service, service changed characteristic */
    BATTERY_LEVEL_CCC_IDX, /* Battery Level CCC */
    LED_CONTROL_CCC_IDX,   /* LED Control CCC */
    NUM_CCC_IDX
};

/* Battery Service handle values */
enum {
    BATTERY_SVC_HDL = 0x2000, /* Battery service declaration */
    BATTERY_LEVEL_CH_HDL,     /* Battery Level characteristic declaration */
    BATTERY_LEVEL_HDL,        /* Battery Level value */
    BATTERY_LEVEL_CCC_HDL,    /* Battery Level CCC descriptor */

    BATTERY_LEVEL_HDL_MAX,
};

/* Battery Service handle values */
enum {
    LED_CONTROL_SVC_HDL = 0x3000, /* LED Control service declaration */
    LED_CONTROL_CH_HDL,           /* LED Control characteristic declaration */
    LED_CONTROL_HDL,              /* LED Control value */
    LED_CONTROL_CCC_HDL,          /* LED Control CCC descriptor */

    LED_CONTROL_HDL_MAX,
};

uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer);

uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer);

#endif /* _BLE_PERIPHERAL_CONFIG_H_ */
