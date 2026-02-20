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
 * GPIO pin definitions for the QPG6200 Standalone Doorbell demo.
 *
 * Board: QPG6200 IoT Carrier Board (QPG6200L DK)
 *
 * Button assignments:
 *   PB5 (GPIO 0)  - DOORBELL button: press to ring
 *   PB1 (GPIO 3)  - DISMISS  button: press to silence
 *
 * LED assignments:
 *   WHITE_COOL (GPIO 1)  - Status blink (slow=idle, fast=ringing)
 *   GREEN      (GPIO 11) - Ready indicator (ON=ready, OFF=ringing)
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

/* Buttons */
#define APP_DOORBELL_BUTTON  PB5_BUTTON_GPIO_PIN    /* GPIO 0 - ring the doorbell */
#define APP_DISMISS_BUTTON   PB1_BUTTON_GPIO_PIN    /* GPIO 3 - dismiss/silence   */

/* LEDs */
#define APP_STATUS_LED       WHITE_COOL_LED_GPIO_PIN /* GPIO 1  - status blink     */
#define APP_READY_LED        GREEN_LED_GPIO_PIN      /* GPIO 11 - ready indicator  */

/*****************************************************************************
 *                    Module Definitions
 *****************************************************************************/

/* Buttons registered with ButtonHandler */
#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_DOORBELL_BUTTON, APP_DISMISS_BUTTON)

/* LEDs managed by StatusLed (max 2 supported) */
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_STATUS_LED, APP_READY_LED)

/* Unused pins pulled low */
#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB2_BUTTON_GPIO_PIN, PB3_BUTTON_GPIO_PIN, PB4_BUTTON_GPIO_PIN,        \
                      SW_BUTTON_GPIO_PIN, WHITE_WARM_LED_GPIO_PIN,                           \
                      RED_LED_GPIO_PIN, BLUE_LED_GPIO_PIN, ANIO0_GPIO_PIN,                   \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
