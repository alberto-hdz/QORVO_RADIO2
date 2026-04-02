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
 * @file BleIoTDemo_Config.h
 * @brief GATT attribute handle definitions for the BLE IoT Demo application.
 *
 * Defines the GATT database layout:
 *
 *   Service 1 – Battery Service (SIG standard, 0x180F)
 *     Characteristic: Battery Level (0x2A19)  – Read / Notify
 *
 *   Service 2 – IoT Demo Service (custom 128-bit UUID)
 *     Characteristic: Button State             – Read / Notify
 *       Value 0x00 = button released
 *       Value 0x01 = button pressed
 *     Characteristic: LED Control              – Write (no response)
 *       Value 0x00 = LED off
 *       Value 0x01 = LED on
 *     Characteristic: Heartbeat Counter        – Read / Notify
 *       1-byte counter, increments every 5 seconds; wraps at 255 → 0
 *
 * Service UUIDs (128-bit, little-endian in firmware, big-endian for readability):
 *   Demo Service  : A1B2C3D4-E5F6-7890-ABCD-EF0123456789
 *   Button State  : A1B2C3D4-E5F6-7890-ABCD-EF0123456790
 *   LED Control   : A1B2C3D4-E5F6-7890-ABCD-EF0123456791
 *   Heartbeat     : A1B2C3D4-E5F6-7890-ABCD-EF0123456792
 *
 * Handle address map:
 *   0x2000 – Battery Service
 *   0x3000 – IoT Demo Service
 */

#ifndef _BLE_IOT_DEMO_CONFIG_H_
#define _BLE_IOT_DEMO_CONFIG_H_

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
#define BLE_ADV_INTERVAL_MIN 0x0020

/** @brief Maximum advertising interval (60 ms in 0.625 ms units). */
#define BLE_ADV_INTERVAL_MAX 0x0060

/** @brief Advertising broadcast duration (~60 seconds). */
#define BLE_ADV_BROADCAST_DURATION 0xF000

/* -------------------------------------------------------------------------- */
/*  GATT service group count (required by BleIf.c)                            */
/* -------------------------------------------------------------------------- */

/** @brief Total number of GATT service groups registered with the BLE stack. */
#define BLE_CONFIG_SVC_GROUPS 2

/* -------------------------------------------------------------------------- */
/*  CCC index table (must match BleIf_CccSet[] in BleIoTDemo_Config.c)        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Indices into the BleIf_CccSet[] CCC descriptor table.
 *
 * Each notifiable characteristic needs one entry here.  The order must
 * exactly match the array initialiser in BleIoTDemo_Config.c.
 */
enum {
    GATT_SC_CCC_IDX,           /**< GATT Service Changed indication (mandatory) */
    BATTERY_LEVEL_CCC_IDX,     /**< Battery Level notification */
    BUTTON_STATE_CCC_IDX,      /**< Button State notification  */
    HEARTBEAT_CCC_IDX,         /**< Heartbeat notification     */
    NUM_CCC_IDX                /**< Total number of CCC entries */
};

/* -------------------------------------------------------------------------- */
/*  Battery Service GATT handle addresses (0x2000 range)                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Attribute handles for the standard Battery Service (0x180F).
 */
enum {
    BATTERY_SVC_HDL = 0x2000, /**< Battery Service declaration     */
    BATTERY_LEVEL_CH_HDL,     /**< Battery Level char declaration  */
    BATTERY_LEVEL_HDL,        /**< Battery Level value             */
    BATTERY_LEVEL_CCC_HDL,    /**< Battery Level CCC descriptor    */
    BATTERY_LEVEL_HDL_MAX     /**< Sentinel – do not use directly  */
};

/* -------------------------------------------------------------------------- */
/*  IoT Demo Service GATT handle addresses (0x3000 range)                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Attribute handles for the custom IoT Demo Service.
 */
enum {
    DEMO_SVC_HDL = 0x3000,   /**< Demo Service declaration            */

    /* Button State characteristic */
    BUTTON_STATE_CH_HDL,     /**< Button State char declaration       */
    BUTTON_STATE_HDL,        /**< Button State value (R / Notify)     */
    BUTTON_STATE_CCC_HDL,    /**< Button State CCC descriptor         */

    /* LED Control characteristic */
    LED_CONTROL_CH_HDL,      /**< LED Control char declaration        */
    LED_CONTROL_HDL,         /**< LED Control value (Write)           */

    /* Heartbeat characteristic */
    HEARTBEAT_CH_HDL,        /**< Heartbeat char declaration          */
    HEARTBEAT_HDL,           /**< Heartbeat value (R / Notify)        */
    HEARTBEAT_CCC_HDL,       /**< Heartbeat CCC descriptor            */

    DEMO_SVC_HDL_MAX         /**< Sentinel – do not use directly      */
};

/* -------------------------------------------------------------------------- */
/*  Mandatory BleIf.c interface functions                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Populate the BLE advertising data payload.
 * @param buffer  Caller-allocated buffer of at least BLEIF_ADV_DATASET_MAX_SIZE bytes.
 * @return        Number of bytes written.
 */
uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer);

/**
 * @brief Populate the BLE scan-response payload (device name).
 * @param buffer  Caller-allocated buffer of at least BLEIF_ADV_DATASET_MAX_SIZE bytes.
 * @return        Number of bytes written.
 */
uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer);

#endif /* _BLE_IOT_DEMO_CONFIG_H_ */
