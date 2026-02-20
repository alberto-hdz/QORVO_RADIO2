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

/** @file "BleDoorbell_Config.h"
 *
 * BLE GATT configuration for the QPG6200 BLE Doorbell demo.
 *
 * Services:
 *   - Standard Battery Service (0x180F) - visible in nRF Connect / Qorvo Connect
 *   - Custom Doorbell Service (128-bit UUID) - ring notifications and remote control
 */

#ifndef _BLE_DOORBELL_CONFIG_H_
#define _BLE_DOORBELL_CONFIG_H_

#pragma once

#include "BleIf.h"

#define BLE_ADV_CHANNEL_37 0x01
#define BLE_ADV_CHANNEL_38 0x02
#define BLE_ADV_CHANNEL_39 0x04

#define BLE_ADV_INTERVAL_MIN       0x0020       /* 20 ms */
#define BLE_ADV_INTERVAL_MAX       0x0060       /* 60 ms */
#define BLE_ADV_BROADCAST_DURATION 0xF000       /* ~60 seconds */

/* Number of GATT service groups registered with BleIf */
#define BLE_CONFIG_SVC_GROUPS 2

/* Index of CCC register callback array */
enum {
    GATT_SC_CCC_IDX,           /* GATT service, service changed characteristic */
    BATTERY_LEVEL_CCC_IDX,     /* Battery Level CCC */
    DOORBELL_RING_CCC_IDX,     /* Doorbell Ring CCC */
    NUM_CCC_IDX
};

/* Battery Service handle values (standard 16-bit service) */
enum {
    BATTERY_SVC_HDL = 0x2000,  /* Battery service declaration */
    BATTERY_LEVEL_CH_HDL,      /* Battery Level characteristic declaration */
    BATTERY_LEVEL_HDL,         /* Battery Level value */
    BATTERY_LEVEL_CCC_HDL,     /* Battery Level CCC descriptor */
    BATTERY_LEVEL_HDL_MAX,
};

/* Doorbell Service handle values (custom 128-bit service) */
enum {
    DOORBELL_SVC_HDL = 0x3000, /* Doorbell service declaration */
    DOORBELL_RING_CH_HDL,      /* Doorbell Ring characteristic declaration */
    DOORBELL_RING_HDL,         /* Doorbell Ring value */
    DOORBELL_RING_CCC_HDL,     /* Doorbell Ring CCC descriptor */
    DOORBELL_RING_HDL_MAX,
};

/* Doorbell ring state values */
#define DOORBELL_STATE_IDLE    0x00
#define DOORBELL_STATE_RINGING 0x01

/* These function names are required by BleIf.c */
uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer);
uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer);

#endif /* _BLE_DOORBELL_CONFIG_H_ */
