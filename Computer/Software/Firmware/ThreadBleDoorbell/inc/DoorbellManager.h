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

/** @file "DoorbellManager.h"
 *
 * GPADC-based doorbell button manager for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Hardware:
 *   Pin  : ANIO0_GPI28 (GPIO 28, physical Pin 11 on IoT Carrier Board)
 *   J24  : Must be set to 1-2 (connects ANIO0 to GPIO 28 pad)
 *   Range: 0.0 V – 3.6 V, 11-bit resolution (~1.76 mV / step)
 *
 * Operation:
 *   The manager runs a FreeRTOS polling task that reads the ADC every
 *   DOORBELL_ADC_POLL_MS milliseconds and detects press/release transitions
 *   using hysteresis thresholds and a simple debounce counter.
 *
 *   On a confirmed press/release, it calls AppManager::NotifyAnalogEvent()
 *   which posts a kEventType_Analog event to the main AppTask queue.
 *
 * Thresholds (configurable via #defines below):
 *   DOORBELL_ADC_PRESS_MV   : ADC voltage above which button is "pressed"
 *   DOORBELL_ADC_RELEASE_MV : ADC voltage below which button is "released"
 *   DOORBELL_DEBOUNCE_COUNT : Consecutive samples needed to change state
 */

#ifndef _DOORBELL_MANAGER_H_
#define _DOORBELL_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus

/* --- Configurable thresholds -------------------------------------------- */

/** Voltage (mV) above which the analog button is considered pressed.
 *  Adjust based on your pull-down / voltage divider resistor values. */
#ifndef DOORBELL_ADC_PRESS_MV
#define DOORBELL_ADC_PRESS_MV    1500
#endif

/** Voltage (mV) below which the analog button is considered released
 *  (hysteresis prevents chatter near the press threshold). */
#ifndef DOORBELL_ADC_RELEASE_MV
#define DOORBELL_ADC_RELEASE_MV  500
#endif

/** Number of consecutive ADC samples that must agree before a state change
 *  is accepted.  At 100 ms poll rate, 3 samples = 300 ms debounce. */
#ifndef DOORBELL_DEBOUNCE_COUNT
#define DOORBELL_DEBOUNCE_COUNT  3
#endif

/** ADC polling interval in milliseconds. */
#ifndef DOORBELL_ADC_POLL_MS
#define DOORBELL_ADC_POLL_MS     100
#endif

/* --- Public class -------------------------------------------------------- */

class DoorbellManager
{
public:
    /**
     * Initialise the GPADC peripheral for GPIO 28 (ANIO0).
     * Must be called once from Application_Init() / AppTask::Init().
     * @return true on success, false on driver error.
     */
    static bool Init(void);

    /**
     * Start the ADC polling FreeRTOS task.
     * Call after Init() and after the FreeRTOS scheduler has started.
     */
    static void StartPolling(void);

    /** @return true if the doorbell button is currently pressed. */
    static bool IsPressed(void);

private:
    static void PollTask(void* pvParameters);

    static bool    sIsPressed;
    static uint8_t sDebounceCount;
};

#endif /* __cplusplus */

#endif /* _DOORBELL_MANAGER_H_ */
