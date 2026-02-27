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
 * GPIO / Peripheral pin definitions for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Board: QPG6200L Development Kit (QPG6200L DK) / IoT Carrier Board
 *
 * Analog input (doorbell button):
 *   ANIO0 / GPIO 28  (Pin 11 on IoT Carrier Board) - Analog doorbell button
 *     ADC threshold press   : voltage > 1.5 V
 *     ADC threshold release : voltage < 0.5 V  (hysteresis)
 *     J24 jumper must be set to 1-2 for GPIO 28 analog input
 *
 * Button (commissioning / factory-reset):
 *   PB1  (GPIO 3) - Short press: restart BLE advertising
 *                   Long press (5 s): factory-reset Thread credentials
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

/* BLE status LED: blinks=advertising, solid=connected */
#define APP_BLE_STATE_LED       WHITE_COOL_LED_GPIO_PIN /* GPIO 1 */

/* Thread status LED: blinks=joining, solid=joined network */
#define APP_THREAD_STATE_LED    GREEN_LED_GPIO_PIN      /* GPIO 11 */

/* Doorbell ring LED: rapid blinks on each ring event */
#define APP_RING_LED            BLUE_LED_GPIO_PIN       /* GPIO 12 */

/* Analog doorbell button on ANIO0 (GPIO 28, Pin 11 on carrier board).
 * NOTE: GPIO 28 is NOT registered as a digital GPIO - it is configured by
 * DoorbellManager via qDrvGPADC_PinConfigSet() as an analog input.
 * Do NOT add it to QPINCFG_BUTTONS or QPINCFG_UNUSED. */
#define APP_DOORBELL_ANIO_PIN   ANIO0_GPIO_PIN          /* GPIO 28 */

/*****************************************************************************
 *                    Module Definitions
 *****************************************************************************/

/* Buttons registered with ButtonHandler (digital only) */
#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)

/* LEDs managed by StatusLed (BLE state, Thread state, ring indicator) */
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_BLE_STATE_LED, APP_THREAD_STATE_LED, APP_RING_LED)

/* Unused pins pulled low to minimise power.
 * GPIO 28 (ANIO0) is deliberately excluded - it is claimed by the ADC driver. */
#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB2_BUTTON_GPIO_PIN, PB3_BUTTON_GPIO_PIN,                             \
                      PB4_BUTTON_GPIO_PIN, PB5_BUTTON_GPIO_PIN, SW_BUTTON_GPIO_PIN,          \
                      WHITE_WARM_LED_GPIO_PIN, RED_LED_GPIO_PIN,                             \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
