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

/** @file "qPinCfg.h"
 *
 * GPIO / Peripheral pin definitions for the QPG6200 Thread+BLE Doorbell DK demo.
 *
 * Board: QPG6200L Development Kit (QPG6200L DK) - standalone, no carrier board required.
 *
 * Digital doorbell button:
 *   PB2 (GPIO 5) - Doorbell trigger button (active low, internal pull-up)
 *     Short press rings the doorbell (BLE notification + Thread multicast)
 *
 * Button (commissioning / factory-reset):
 *   PB1  (GPIO 3) - Short press (<2 s): restart BLE advertising
 *                   Long press (>=5 s): factory-reset Thread credentials
 *
 * LED assignments:
 *   WHITE_COOL (GPIO 1)  - BLE status:    blinks=advertising, solid=connected
 *   GREEN      (GPIO 11) - Thread status: blinks=joining,     solid=joined
 *   BLUE       (GPIO 12) - Doorbell ring: rapid blinks on each ring event
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

/*****************************************************************************
 *                    Common Inclusion
 *****************************************************************************/
#include "qPinCfg_Common.h"

/*****************************************************************************
 *                    Pin Definitions
 *****************************************************************************/

/* Multi-function commissioning button (short: restart adv, long: factory reset) */
#define APP_MULTI_FUNC_BUTTON   PB1_BUTTON_GPIO_PIN   /* GPIO 3 */

/* Digital doorbell button on DK (PB2, GPIO 5, active low, pull-up).
 * DoorbellManager configures and polls this GPIO directly. */
#define APP_DOORBELL_BUTTON     PB2_BUTTON_GPIO_PIN   /* GPIO 5 */

/* BLE status LED: blinks=advertising, solid=connected */
#define APP_BLE_STATE_LED       WHITE_COOL_LED_GPIO_PIN /* GPIO 1 */

/* Thread status LED: blinks=joining, solid=joined network */
#define APP_THREAD_STATE_LED    GREEN_LED_GPIO_PIN      /* GPIO 11 */

/* Doorbell ring LED: rapid blinks on each ring event */
#define APP_RING_LED            BLUE_LED_GPIO_PIN       /* GPIO 12 */

/*****************************************************************************
 *                    Module Definitions
 *****************************************************************************/

/* Buttons registered with ButtonHandler (digital only - commissioning button PB1) */
#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)

/* LEDs managed by StatusLed (BLE state, Thread state, ring indicator) */
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_BLE_STATE_LED, APP_THREAD_STATE_LED, APP_RING_LED)

/* Unused pins pulled low to minimise power.
 * PB2 (GPIO 5) is excluded - it is claimed by DoorbellManager as a digital input.
 * GPIO 28 (ANIO0) and GPIO 29 (ANIO1) are excluded - analog pins unused on DK standalone. */
#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB3_BUTTON_GPIO_PIN,                                                   \
                      PB4_BUTTON_GPIO_PIN, PB5_BUTTON_GPIO_PIN, SW_BUTTON_GPIO_PIN,          \
                      WHITE_WARM_LED_GPIO_PIN, RED_LED_GPIO_PIN,                             \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
