/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * @file qPinCfg.h
 * @brief GPIO pin assignments for the BLE Gateway Doorbell on the QPG6200 DK.
 *
 *   PB5           - Doorbell button (short press = ring, hold >=5s = re-advertise)
 *   WHITE_COOL    - BLE status LED (blinks=advertising, solid=connected, off=idle)
 *   BLUE          - App state LED (remote-controlled via LED Control GATT char)
 */
#ifndef _QPINCFG_H_
#define _QPINCFG_H_

#include "qPinCfg_Common.h"

#define APP_MULTI_FUNC_BUTTON   PB5_BUTTON_GPIO_PIN
#define APP_BLE_CONNECTION_LED  WHITE_COOL_LED_GPIO_PIN
#define APP_STATE_LED           BLUE_LED_GPIO_PIN

#define QPINCFG_BUTTONS    QPINCFG_GPIO_LIST(APP_MULTI_FUNC_BUTTON)
#define QPINCFG_STATUS_LED QPINCFG_GPIO_LIST(APP_BLE_CONNECTION_LED, APP_STATE_LED)

#define QPINCFG_UNUSED                                                        \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN, PB2_BUTTON_GPIO_PIN,              \
                      PB3_BUTTON_GPIO_PIN, PB4_BUTTON_GPIO_PIN,              \
                      WHITE_WARM_LED_GPIO_PIN, RED_LED_GPIO_PIN,             \
                      GREEN_LED_GPIO_PIN, ANIO0_GPIO_PIN,                    \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,                         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO,                            \
                      DEBUG_SWJDP_SWCLK_TCK_GPIO,                           \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
