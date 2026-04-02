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
 * @file qPinCfg.h
 * @brief GPIO pin assignments for the BLE IoT Demo on the QPG6200 DK.
 *
 * Pin mapping (QPG6200 Development Kit):
 *
 *   PB5  (GPIO)  – Multi-function button (short press = BLE notify,
 *                  long press ≥ 5 s = restart advertising)
 *   WHITE_COOL   – BLE status LED
 *                    Blinking : advertising
 *                    Solid ON : connected
 *                    OFF      : disconnected / idle
 *   BLUE         – Application state LED (controlled remotely via
 *                  the LED Control GATT characteristic)
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

/* ------------------------------------------------------------------------- */
/*  User customisation (optional)                                             */
/* ------------------------------------------------------------------------- */
// Uncomment to override the default board file from qorvo_internal.h:
// #define QPINCFG_CUSTOM_BOARD_FILENAME "my_board.h"

/* ------------------------------------------------------------------------- */
/*  Common inclusion                                                          */
/* ------------------------------------------------------------------------- */
#include "qPinCfg_Common.h"

/* ------------------------------------------------------------------------- */
/*  Functional pin aliases                                                    */
/* ------------------------------------------------------------------------- */

/** @brief Button used for BLE notifications and advertising control. */
#define APP_MULTI_FUNC_BUTTON PB5_BUTTON_GPIO_PIN

/** @brief LED that reflects the BLE connection state. */
#define APP_BLE_CONNECTION_LED WHITE_COOL_LED_GPIO_PIN

/** @brief LED that is remotely controlled via the LED Control characteristic. */
#define APP_STATE_LED BLUE_LED_GPIO_PIN

/* ------------------------------------------------------------------------- */
/*  Module configuration lists                                                */
/* ------------------------------------------------------------------------- */

/** @brief GPIOs registered with ButtonHandler as buttons. */
#define QPINCFG_BUTTONS QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)

/** @brief GPIOs managed by StatusLed. */
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_BLE_CONNECTION_LED, APP_STATE_LED)

/** @brief Unused GPIOs – pulled low to minimise current draw. */
#define QPINCFG_UNUSED                                                       \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN, PB2_BUTTON_GPIO_PIN,             \
                      PB3_BUTTON_GPIO_PIN, PB4_BUTTON_GPIO_PIN,             \
                      WHITE_WARM_LED_GPIO_PIN, RED_LED_GPIO_PIN,            \
                      GREEN_LED_GPIO_PIN, ANIO0_GPIO_PIN,                   \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                        \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO,                           \
                      DEBUG_SWJDP_SWCLK_TCK_GPIO,                          \
                      BOARD_UNUSED_GPIO_PINS)

#endif // _QPINCFG_H_
