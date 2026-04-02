/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "qPinCfg.h"
 *
 * GPIO / Peripheral pin definitions for the QPG6200 Thread+BLE HC-SR04 Motion Detector.
 *
 * Board: QPG6200L Development Kit
 *
 * Sensor wiring:
 *   Sensor VCC  -> 5V
 *   Sensor GND  -> GND
 *   Sensor Trig -> GPIO 28 (ANIO0, digital output - trigger pulse)
 *   Sensor Echo -> GPIO 29 (ANIO1, digital input  - echo pulse width)
 *
 * Button (commissioning / factory-reset):
 *   PB1 (GPIO 3) - Short press (<2 s): restart BLE advertising
 *                  Long press (>=5 s): factory-reset Thread credentials
 *
 * LED assignments:
 *   WHITE_COOL (GPIO 1)  - BLE status:    blinks=advertising, solid=connected
 *   GREEN      (GPIO 11) - Thread status: blinks=joining,     solid=joined
 *   BLUE       (GPIO 12) - Motion:        solid=motion detected, off=no motion
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

#include "qPinCfg_Common.h"

#define APP_MULTI_FUNC_BUTTON   PB1_BUTTON_GPIO_PIN

/* HC-SR04 sensor pins */
#define SENSOR_TRIG_GPIO        ANIO0_GPIO_PIN    /* GPIO 28 - trigger output */
#define SENSOR_ECHO_GPIO        ANIO1_GPIO_PIN    /* GPIO 29 - echo input     */

#define APP_BLE_STATE_LED       WHITE_COOL_LED_GPIO_PIN
#define APP_THREAD_STATE_LED    GREEN_LED_GPIO_PIN
#define APP_MOTION_LED          BLUE_LED_GPIO_PIN

#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)

#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_BLE_STATE_LED, APP_THREAD_STATE_LED, APP_MOTION_LED)

#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB2_BUTTON_GPIO_PIN,                                                   \
                      PB3_BUTTON_GPIO_PIN,                                                   \
                      PB4_BUTTON_GPIO_PIN, PB5_BUTTON_GPIO_PIN, SW_BUTTON_GPIO_PIN,          \
                      WHITE_WARM_LED_GPIO_PIN, RED_LED_GPIO_PIN,                             \
                      UART1_RX_GPIO_PIN, UART1_TX_GPIO_PIN,                                  \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
