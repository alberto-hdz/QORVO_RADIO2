/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "qPinCfg.h"
 *
 * GPIO / peripheral pin definitions for the minimal MaxSonar_Test application.
 *
 * Board: QPG6200L Development Kit
 *
 * Wiring:
 *   Sensor TX  -> GPIO 11 (UART0 RX)  sensor serial out
 *   Sensor RX  -> GPIO 10 (UART0 TX)  held HIGH -> continuous ranging
 *   Sensor +5  -> 5 V supply (sensor requires 5 V)
 *   Sensor GND -> GND
 *
 * UART1 (GPIO 8/9) is reserved for the debug console (gpCom, 115200 baud).
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

#include "qPinCfg_Common.h"

#define SENSOR_UART_TX_GPIO     10  /* UART0 TX -> Sensor RX */
#define SENSOR_UART_RX_GPIO     11  /* UART0 RX <- Sensor TX */

/* No status LEDs in this minimal build. Keep the list empty so any
 * framework code that references QPINCFG_STATUS_LED still compiles. */
#define QPINCFG_STATUS_LED      QPINCFG_GPIO_LIST()

/* Pull unused pins low to minimise current. */
#define QPINCFG_UNUSED                                      \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN,                  \
                      PB2_BUTTON_GPIO_PIN,                  \
                      PB3_BUTTON_GPIO_PIN,                  \
                      PB4_BUTTON_GPIO_PIN,                  \
                      PB5_BUTTON_GPIO_PIN,                  \
                      SW_BUTTON_GPIO_PIN,                   \
                      RED_LED_GPIO_PIN,                     \
                      GREEN_LED_GPIO_PIN,                   \
                      BLUE_LED_GPIO_PIN,                    \
                      WHITE_WARM_LED_GPIO_PIN,              \
                      WHITE_COOL_LED_GPIO_PIN,              \
                      ANIO0_GPIO_PIN,                       \
                      EXT_32KXTAL_P, EXT_32KXTAL_N,         \
                      DEBUG_SWJDP_SWDIO_TMS_GPIO,           \
                      DEBUG_SWJDP_SWCLK_TCK_GPIO,           \
                      BOARD_UNUSED_GPIO_PINS)

#endif /* _QPINCFG_H_ */
