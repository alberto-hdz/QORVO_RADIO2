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

/** @file "DoorbellManager.cpp"
 *
 * Digital GPIO doorbell button manager for the DK.
 *
 * Hardware connection:
 *   GPIO 5 = PB2 on QPG6200L Development Kit
 *   Active low: button connects GPIO 5 to GND; internal pull-up keeps it high at rest.
 *   No carrier board or jumper required.
 *
 * The manager configures GPIO 5 as a digital input with pull-up enabled,
 * then polls it every DOORBELL_POLL_MS milliseconds and detects press/release
 * transitions using a simple debounce counter.
 *
 * Poll rate  : DOORBELL_POLL_MS  (default 50 ms)
 * Debounce   : DOORBELL_DEBOUNCE_COUNT consecutive matching samples
 */

#include "DoorbellManager.h"
#include "AppManager.h"
#include "gpLog.h"
#include "qDrvGPIO.h"
#include "qPinCfg.h"

#include "FreeRTOS.h"
#include "task.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------
 * Static members
 * ------------------------------------------------------------------------- */
bool    DoorbellManager::sIsPressed     = false;
uint8_t DoorbellManager::sDebounceCount = 0;

/* FreeRTOS task storage */
#define DOORBELL_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#define DOORBELL_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

static StaticTask_t  sDoorbellTask;
static StackType_t   sDoorbellStack[DOORBELL_TASK_STACK_SIZE];

/* -------------------------------------------------------------------------
 * Init  - configure PB2 (GPIO 5) as a digital input with pull-up
 * ------------------------------------------------------------------------- */
bool DoorbellManager::Init(void)
{
    qDrvGPIO_InputConfig_t inputCfg = {
        .pull           = qDrvIOB_PullUp,
        .schmittTrigger = false,
        .irqType        = qDrvGPIO_IrqTypeNone,
        .highPriority   = false,
        .wakeup         = qDrvGPIO_WakeupNone,
        .callback       = nullptr,
    };

    qResult_t res = qDrvGPIO_InputConfigSet(APP_DOORBELL_BUTTON, &inputCfg);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[BTN] DoorbellManager GPIO%d input config failed: %d", 0,
                             APP_DOORBELL_BUTTON, res);
        return false;
    }

    GP_LOG_SYSTEM_PRINTF("[BTN] Doorbell button ready on GPIO%d (PB2, active low)", 0,
                         APP_DOORBELL_BUTTON);
    GP_LOG_SYSTEM_PRINTF("[BTN] Poll interval : %u ms", 0, DOORBELL_POLL_MS);
    GP_LOG_SYSTEM_PRINTF("[BTN] Debounce count: %u samples", 0, DOORBELL_DEBOUNCE_COUNT);
    return true;
}

/* -------------------------------------------------------------------------
 * StartPolling  - creates the FreeRTOS polling task
 * ------------------------------------------------------------------------- */
void DoorbellManager::StartPolling(void)
{
    TaskHandle_t handle =
        xTaskCreateStatic(PollTask, "DoorbellBtn", DOORBELL_TASK_STACK_SIZE,
                          nullptr, DOORBELL_TASK_PRIORITY, sDoorbellStack, &sDoorbellTask);
    Q_ASSERT(handle != nullptr);
}

/* -------------------------------------------------------------------------
 * IsPressed
 * ------------------------------------------------------------------------- */
bool DoorbellManager::IsPressed(void)
{
    return sIsPressed;
}

/* -------------------------------------------------------------------------
 * PollTask  - runs forever at DOORBELL_POLL_MS rate
 *
 * GPIO 5 (PB2) is active low:
 *   qDrvGPIO_Read() == false  →  GPIO low  →  button pressed
 *   qDrvGPIO_Read() == true   →  GPIO high →  button released
 * ------------------------------------------------------------------------- */
void DoorbellManager::PollTask(void* /*pvParameters*/)
{
    while(true)
    {
        /* Read the digital GPIO; active low means pressed = !high */
        bool currentlyPressed = !qDrvGPIO_Read(APP_DOORBELL_BUTTON);

        /* Debounce state machine */
        if(!sIsPressed)
        {
            /* Looking for a PRESS event */
            if(currentlyPressed)
            {
                sDebounceCount++;
                if(sDebounceCount >= DOORBELL_DEBOUNCE_COUNT)
                {
                    sIsPressed     = true;
                    sDebounceCount = 0;
                    GP_LOG_SYSTEM_PRINTF("[BTN] Doorbell PRESSED  (GPIO%d)", 0,
                                         APP_DOORBELL_BUTTON);
                    AppManager::NotifyAnalogEvent(true, 0);
                }
            }
            else
            {
                sDebounceCount = 0;
            }
        }
        else
        {
            /* Looking for a RELEASE event */
            if(!currentlyPressed)
            {
                sDebounceCount++;
                if(sDebounceCount >= DOORBELL_DEBOUNCE_COUNT)
                {
                    sIsPressed     = false;
                    sDebounceCount = 0;
                    GP_LOG_SYSTEM_PRINTF("[BTN] Doorbell RELEASED (GPIO%d)", 0,
                                         APP_DOORBELL_BUTTON);
                    AppManager::NotifyAnalogEvent(false, 0);
                }
            }
            else
            {
                sDebounceCount = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DOORBELL_POLL_MS));
    }

    /* Never reached */
    vTaskDelete(nullptr);
}
