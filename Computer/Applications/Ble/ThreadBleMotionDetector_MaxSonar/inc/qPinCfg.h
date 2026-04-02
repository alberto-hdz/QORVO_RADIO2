/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "qPinCfg.h"
 *
 * GPIO / Peripheral pin definitions for the QPG6200 Thread+BLE MaxSonar Motion Detector.
 *
 * Board: QPG6200L Development Kit
 *
 * Sensor wiring:
 *   Sensor TX  -> GPIO 8  (UART1 RX - board receives sensor serial output)
 *   Sensor RX  -> GPIO 9  (UART1 TX - held HIGH to enable continuous ranging)
 *   Sensor +5  -> 5V supply
 *   Sensor GND -> GND
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

#define SENSOR_UART_RX_GPIO     UART1_RX_GPIO_PIN   /* GPIO 8  - receives sensor TX */
#define SENSOR_UART_TX_GPIO     UART1_TX_GPIO_PIN   /* GPIO 9  - held HIGH (continuous ranging) */

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
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO, DEBUG_SWJDP_SWCLK_TCK_GPIO,               \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
