/*
 * Copyright (c) 2025, Qorvo Inc
 *
 * qPinCfg for MicTest application.
 * GPIO2 = I2S_0 WS, GPIO3 = I2S_0 SCK, GPIO5 = I2S_0 SDI
 * Configured as I2S alternate function by qDrvI2S_PinConfigSet().
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

#include "qPinCfg_Common.h"

#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN, PB2_BUTTON_GPIO_PIN, PB3_BUTTON_GPIO_PIN,        \
                      PB4_BUTTON_GPIO_PIN, WHITE_WARM_LED_GPIO_PIN, ANIO0_GPIO_PIN,          \
                      ANIO1_GPIO_PIN, EXT_32KXTAL_P, EXT_32KXTAL_N, BOARD_UNUSED_GPIO_PINS)

#endif // _QPINCFG_H_
