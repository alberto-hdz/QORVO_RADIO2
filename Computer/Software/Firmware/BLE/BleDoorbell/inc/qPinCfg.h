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
 * GPIO pin definitions for the QPG6200 BLE Doorbell demo application.
 *
 * Board: QPG6200 IoT Carrier Board (QPG6200L DK)
 *
 * Button assignments:
 *   PB5 (GPIO 0) - Doorbell button: press to ring the doorbell
 *
 * LED assignments:
 *   WHITE_COOL (GPIO 1) - BLE status: blinks=advertising, solid=connected, off=idle
 *   BLUE       (GPIO 12)- Doorbell indicator: blinks on each ring event
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

/* Doorbell button: press to send a ring notification */
#define APP_MULTI_FUNC_BUTTON  PB5_BUTTON_GPIO_PIN   /* GPIO 0 */

/* BLE status LED: blinks during advertising, solid when connected */
#define APP_STATE_LED          WHITE_COOL_LED_GPIO_PIN /* GPIO 1 */

/* Doorbell ring LED: blinks on each doorbell event */
#define APP_BLE_CONNECTION_LED BLUE_LED_GPIO_PIN       /* GPIO 12 */

/*****************************************************************************
 *                    Module Definitions
 *****************************************************************************/

/* Buttons registered with ButtonHandler */
#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)

/* LEDs managed by StatusLed (max 2) */
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_STATE_LED, APP_BLE_CONNECTION_LED)

/* Unused pins pulled low to minimize power */
#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN, PB2_BUTTON_GPIO_PIN, PB3_BUTTON_GPIO_PIN,        \
                      PB4_BUTTON_GPIO_PIN, SW_BUTTON_GPIO_PIN, WHITE_WARM_LED_GPIO_PIN,      \
                      RED_LED_GPIO_PIN, GREEN_LED_GPIO_PIN, ANIO0_GPIO_PIN,                  \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
