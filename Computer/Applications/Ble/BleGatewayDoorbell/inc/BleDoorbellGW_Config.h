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
 * @file BleDoorbellGW_Config.h
 * @brief GATT attribute handle definitions for the BLE Gateway Doorbell application.
 *
 * Defines the GATT database layout for a smart doorbell peripheral that
 * communicates with an IoT gateway (Raspberry Pi) running the BLE-MQTT bridge.
 *
 * Services
 * --------
 *   Service 1 – Battery Service (SIG standard, 0x180F)
 *     Characteristic: Battery Level (0x2A19)  – Read / Notify
 *
 *   Service 2 – Doorbell Gateway Service (custom 128-bit UUID)
 *     Characteristic: Doorbell Ring    – Read / Notify
 *       0x00 = idle (not ringing)
 *       0x01 = ringing (button pressed)
 *     Characteristic: LED Control      – Write
 *       0x00 = LED off
 *       0x01 = LED on
 *     Characteristic: Heartbeat        – Read / Notify
 *       1-byte counter, increments every 5 seconds; wraps at 255 -> 0
 *
 * UUIDs (128-bit, big-endian notation for readability):
 *   Doorbell Service  : C000C001-0000-1000-8000-00805F9B3400
 *   Doorbell Ring     : C000C002-0000-1000-8000-00805F9B3400
 *   LED Control       : C000C003-0000-1000-8000-00805F9B3400
 *   Heartbeat         : C000C004-0000-1000-8000-00805F9B3400
 *
 * Handle address map:
 *   0x2000 – Battery Service
 *   0x3000 – Doorbell Gateway Service
 *
 * MQTT data flow (via gateway bridge on Raspberry Pi):
 *   Doorbell Ring notify -> qorvo/demo/button    -> Node-RED dashboard
 *   Heartbeat notify     -> qorvo/demo/heartbeat -> Node-RED dashboard
 *   qorvo/demo/led       -> LED Control write    -> BLUE LED on board
 */

#ifndef _BLE_DOORBELL_GW_CONFIG_H_
#define _BLE_DOORBELL_GW_CONFIG_H_

#pragma once

#include "BleIf.h"

/* -------------------------------------------------------------------------- */
/*  BLE advertising parameters                                                */
/* -------------------------------------------------------------------------- */

/** @brief Advertising channel map: channels 37, 38, and 39. */
#define BLE_ADV_CHANNEL_37 0x01
#define BLE_ADV_CHANNEL_38 0x02
#define BLE_ADV_CHANNEL_39 0x04

/** @brief Minimum advertising interval (20 ms in 0.625 ms units). */
#define BLE_ADV_INTERVAL_MIN       0x0020

/** @brief Maximum advertising interval (60 ms in 0.625 ms units). */
#define BLE_ADV_INTERVAL_MAX       0x0060

/** @brief Advertising broadcast duration (~60 seconds). */
#define BLE_ADV_BROADCAST_DURATION 0xF000

/* -------------------------------------------------------------------------- */
/*  GATT service group count (required by BleIf.c)                            */
/* -------------------------------------------------------------------------- */

/** @brief Total number of GATT service groups registered with the BLE stack. */
#define BLE_CONFIG_SVC_GROUPS 2

/* -------------------------------------------------------------------------- */
/*  CCC index table (must match BleIf_CccSet[] in BleDoorbellGW_Config.c)     */
/* -------------------------------------------------------------------------- */

enum {
    GATT_SC_CCC_IDX,            /**< GATT Service Changed indication (mandatory) */
    BATTERY_LEVEL_CCC_IDX,      /**< Battery Level notification */
    DOORBELL_RING_CCC_IDX,      /**< Doorbell Ring notification */
    HEARTBEAT_CCC_IDX,          /**< Heartbeat notification */
    NUM_CCC_IDX                 /**< Total CCC entry count */
};

/* -------------------------------------------------------------------------- */
/*  Battery Service GATT handle addresses (0x2000 range)                       */
/* -------------------------------------------------------------------------- */

enum {
    BATTERY_SVC_HDL = 0x2000, /**< Battery Service declaration    */
    BATTERY_LEVEL_CH_HDL,     /**< Battery Level char declaration */
    BATTERY_LEVEL_HDL,        /**< Battery Level value            */
    BATTERY_LEVEL_CCC_HDL,    /**< Battery Level CCC descriptor   */
    BATTERY_LEVEL_HDL_MAX     /**< Sentinel – do not use directly */
};

/* -------------------------------------------------------------------------- */
/*  Doorbell Gateway Service GATT handle addresses (0x3000 range)             */
/* -------------------------------------------------------------------------- */

enum {
    DOORBELL_SVC_HDL = 0x3000, /**< Doorbell Service declaration         */

    /* Doorbell Ring characteristic */
    DOORBELL_RING_CH_HDL,      /**< Doorbell Ring char declaration       */
    DOORBELL_RING_HDL,         /**< Doorbell Ring value (R / Notify)     */
    DOORBELL_RING_CCC_HDL,     /**< Doorbell Ring CCC descriptor         */

    /* LED Control characteristic */
    LED_CONTROL_CH_HDL,        /**< LED Control char declaration         */
    LED_CONTROL_HDL,           /**< LED Control value (Write)            */

    /* Heartbeat characteristic */
    HEARTBEAT_CH_HDL,          /**< Heartbeat char declaration           */
    HEARTBEAT_HDL,             /**< Heartbeat value (R / Notify)         */
    HEARTBEAT_CCC_HDL,         /**< Heartbeat CCC descriptor             */

    DOORBELL_SVC_HDL_MAX       /**< Sentinel – do not use directly       */
};

/* -------------------------------------------------------------------------- */
/*  Doorbell ring state values                                                */
/* -------------------------------------------------------------------------- */

/** @brief Ring characteristic value: doorbell is idle (not ringing). */
#define DOORBELL_STATE_IDLE    0x00

/** @brief Ring characteristic value: doorbell is ringing (button pressed). */
#define DOORBELL_STATE_RINGING 0x01

/* -------------------------------------------------------------------------- */
/*  Mandatory BleIf.c interface functions                                     */
/* -------------------------------------------------------------------------- */

uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer);
uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer);

#endif /* _BLE_DOORBELL_GW_CONFIG_H_ */
