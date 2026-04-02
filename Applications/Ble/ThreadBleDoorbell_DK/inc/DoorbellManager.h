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
 * Digital GPIO doorbell button manager for the QPG6200 Thread+BLE Doorbell DK demo.
 *
 * Hardware:
 *   Pin  : PB2 (GPIO 5) on QPG6200L Development Kit
 *   Logic: Active low (button connects GPIO to GND; internal pull-up enabled)
 *   No carrier board or jumper required - uses the DK's built-in push button.
 *
 * Operation:
 *   The manager runs a FreeRTOS polling task that reads GPIO 5 every
 *   DOORBELL_POLL_MS milliseconds and detects press/release transitions
 *   using a simple debounce counter.
 *
 *   On a confirmed press/release, it calls AppManager::NotifyAnalogEvent()
 *   which posts a kEventType_Analog event to the main AppTask queue.
 *
 * Timing (configurable via #defines below):
 *   DOORBELL_POLL_MS        : GPIO polling interval in milliseconds
 *   DOORBELL_DEBOUNCE_COUNT : Consecutive matching samples needed to change state
 */

#ifndef _DOORBELL_MANAGER_H_
#define _DOORBELL_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus

/* --- Configurable timing ------------------------------------------------- */

/** GPIO polling interval in milliseconds. */
#ifndef DOORBELL_POLL_MS
#define DOORBELL_POLL_MS        50
#endif

/** Number of consecutive GPIO samples that must agree before a state change
 *  is accepted.  At 50 ms poll rate, 3 samples = 150 ms debounce. */
#ifndef DOORBELL_DEBOUNCE_COUNT
#define DOORBELL_DEBOUNCE_COUNT 3
#endif

/* --- Public class -------------------------------------------------------- */

class DoorbellManager
{
public:
    /**
     * Initialise GPIO 5 (PB2) as a digital input with pull-up.
     * Must be called once from AppTask::Init().
     * @return true on success, false on driver error.
     */
    static bool Init(void);

    /**
     * Start the GPIO polling FreeRTOS task.
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
