/*
 * Copyright (c) 2025, Qorvo Inc
 *
 * qPinCfg for SpeakerTest application.
 * GPIO10 = PWMXL_4 output -> TPA2034D1 IN+
 */

#ifndef _QPINCFG_H_
#define _QPINCFG_H_

#include "qPinCfg_Common.h"

/* GPIO10 is configured as PWMXL alternate function by qDrvPWMXL_PinConfigSet().
 * Pull all other free GPIOs low to reduce noise. */
#define QPINCFG_UNUSED                                                                       \
    QPINCFG_GPIO_LIST(PB1_BUTTON_GPIO_PIN, PB2_BUTTON_GPIO_PIN, PB3_BUTTON_GPIO_PIN,        \
                      PB4_BUTTON_GPIO_PIN, WHITE_WARM_LED_GPIO_PIN, ANIO0_GPIO_PIN,          \
                      ANIO1_GPIO_PIN, EXT_32KXTAL_P, EXT_32KXTAL_N, BOARD_UNUSED_GPIO_PINS)

#endif // _QPINCFG_H_
