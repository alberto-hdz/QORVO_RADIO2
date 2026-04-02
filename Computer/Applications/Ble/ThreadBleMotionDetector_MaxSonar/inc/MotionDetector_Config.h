/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "MotionDetector_Config.h"
 *
 * BLE GATT attribute handle definitions for the QPG6200 Thread+BLE Motion Detector.
 *
 * GATT Service layout:
 *
 *  [Battery Service - 0x180F]
 *    0x2000 : Service Declaration
 *    0x2001 : Battery Level Char Declaration
 *    0x2002 : Battery Level Value             (Read)
 *    0x2003 : Battery Level CCC               (Read/Write)
 *
 *  [Motion Detection Service - custom 128-bit UUID]
 *    0x3000 : Service Declaration
 *    0x3001 : Motion Status Char Declaration
 *    0x3002 : Motion Status Value             (Read / Write / Notify) 0x00=none, 0x01=detected
 *    0x3003 : Motion Status CCC               (Read/Write)
 *    0x3004 : Distance Char Declaration
 *    0x3005 : Distance Value                  (Read / Notify) 2 bytes big-endian, cm
 *    0x3006 : Distance CCC                    (Read/Write)
 *
 *  [Thread Config Service - custom 128-bit UUID]
 *    0x4000 : Service Declaration
 *    0x4001 : Network Name Char Declaration
 *    0x4002 : Network Name Value              (Read / Write, 16 bytes max)
 *    0x4003 : Network Key Char Declaration
 *    0x4004 : Network Key Value               (Write-only, 16 bytes)
 *    0x4005 : Channel Char Declaration
 *    0x4006 : Channel Value                   (Read / Write, 1 byte, 11-26)
 *    0x4007 : PAN ID Char Declaration
 *    0x4008 : PAN ID Value                    (Read / Write, 2 bytes LE)
 *    0x4009 : Join Char Declaration
 *    0x400A : Join Value                      (Write 0x01 = join Thread network)
 *    0x400B : Thread Status Char Declaration
 *    0x400C : Thread Status Value             (Read / Notify, 1 byte)
 *    0x400D : Thread Status CCC               (Read / Write)
 */

#ifndef _MOTIONDETECTOR_CONFIG_H_
#define _MOTIONDETECTOR_CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_ADV_CHANNEL_37         0x01
#define BLE_ADV_CHANNEL_38         0x02
#define BLE_ADV_CHANNEL_39         0x04

#define BLE_ADV_INTERVAL_MIN       0x0020
#define BLE_ADV_INTERVAL_MAX       0x0060
#define BLE_ADV_BROADCAST_DURATION 0xF000

#define NUM_CCC_IDX                5
#define BLE_CONFIG_SVC_GROUPS      3

/* Battery Service */
#define BATTERY_SVC_HDL            0x2000
#define BATTERY_LEVEL_CH_HDL       0x2001
#define BATTERY_LEVEL_HDL          0x2002
#define BATTERY_LEVEL_CCC_HDL      0x2003
#define BATTERY_LEVEL_HDL_MAX      (BATTERY_LEVEL_CCC_HDL + 1)

/* Motion Detection Service */
#define MOTION_SVC_HDL             0x3000
#define MOTION_STATUS_CH_HDL       0x3001
#define MOTION_STATUS_HDL          0x3002
#define MOTION_STATUS_CCC_HDL      0x3003
#define MOTION_DIST_CH_HDL         0x3004
#define MOTION_DIST_HDL            0x3005
#define MOTION_DIST_CCC_HDL        0x3006
#define MOTION_SVC_HDL_MAX         (MOTION_DIST_CCC_HDL + 1)

#define MOTION_STATE_NONE          0x00
#define MOTION_STATE_DETECTED      0x01

/* Thread Config Service */
#define THREAD_CFG_SVC_HDL         0x4000

#define THREAD_NET_NAME_CH_HDL     0x4001
#define THREAD_NET_NAME_HDL        0x4002

#define THREAD_NET_KEY_CH_HDL      0x4003
#define THREAD_NET_KEY_HDL         0x4004

#define THREAD_CHANNEL_CH_HDL      0x4005
#define THREAD_CHANNEL_HDL         0x4006

#define THREAD_PANID_CH_HDL        0x4007
#define THREAD_PANID_HDL           0x4008

#define THREAD_JOIN_CH_HDL         0x4009
#define THREAD_JOIN_HDL            0x400A

#define THREAD_STATUS_CH_HDL       0x400B
#define THREAD_STATUS_HDL          0x400C
#define THREAD_STATUS_CCC_HDL      0x400D
#define THREAD_CFG_SVC_HDL_MAX     (THREAD_STATUS_CCC_HDL + 1)

#define THREAD_STATUS_DISABLED     0x00
#define THREAD_STATUS_DETACHED     0x01
#define THREAD_STATUS_CHILD        0x02
#define THREAD_STATUS_ROUTER       0x03
#define THREAD_STATUS_LEADER       0x04

#define GATT_SC_CH_CCC_HDL         0x0013

#define THREAD_NET_NAME_MAX_LEN    16
#define THREAD_NET_KEY_LEN         16
#define THREAD_PANID_LEN           2
#define THREAD_CHANNEL_LEN         1
#define THREAD_JOIN_LEN            1
#define THREAD_STATUS_LEN          1
#define MOTION_STATUS_LEN          1
#define MOTION_DIST_LEN            2

uint8_t Ble_Peripheral_Config_Load_Advertise_Frame(uint8_t* buffer);
uint8_t Ble_Peripheral_Config_Load_Scan_Response_Frame(uint8_t* buffer);

#ifdef __cplusplus
}
#endif

#endif /* _MOTIONDETECTOR_CONFIG_H_ */
