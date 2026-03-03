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

/** @file "AppTask.cpp"
 *
 * Main FreeRTOS application task for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Initialisation order:
 *   1. ResetCount (optional, if GP_APP_DIVERSITY_RESETCOUNTING)
 *   2. FreeRTOS event queue
 *   3. AppTask FreeRTOS task (spawns Main loop)
 *   4. ButtonHandler (PB1 digital commissioning button)
 *   5. AppManager::Init()  → BLE stack init + GATT + advertising
 *   6. DoorbellManager::Init()  → GPADC for GPIO 28 analog button
 *   7. DoorbellManager::StartPolling() → ADC polling FreeRTOS task
 *
 * The Thread stack is initialised inside AppManager::Init() via
 * Thread_Init(), which is called after the BLE stack is up.
 */

#include "hal.h"
#include "gpLog.h"
#include "gpHal.h"
#include "gpReset.h"
#include "gpSched.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"

#include "qPinCfg.h"
#include "AppButtons.h"
#include "AppManager.h"
#include "AppTask.h"
#include "DoorbellManager.h"

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
#include "ResetCount.h"
#endif

#include "hal_PowerMode.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define APP_EVENT_QUEUE_SIZE  20
#define APP_TASK_STACK_SIZE   (6 * 1024)   /* Larger stack for Thread+BLE */
#define APP_TASK_PRIORITY     2

/** Threshold of inactivity before the scheduler enters sleep (us) */
#ifndef APP_GOTOSLEEP_THRESHOLD
#define APP_GOTOSLEEP_THRESHOLD 1000
#endif

#define _PRINT_APP_VERSION(_major, _minor, _revision, _patch)                       \
    GP_LOG_SYSTEM_PRINTF("SW Version: %d.%d.%d.%d", 0, _major, _minor, _revision, _patch)
#define PRINT_APP_VERSION(_args) _PRINT_APP_VERSION(_args)

namespace {
uint8_t       sAppEventQueueBuffer[APP_EVENT_QUEUE_SIZE * sizeof(AppEvent)];
StaticQueue_t sAppEventQueueStruct;
QueueHandle_t sAppEventQueue;

StackType_t  appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t appTaskStruct;
TaskHandle_t sAppTaskHandle;
} // namespace

AppTask AppTask::sAppTask;

/* -------------------------------------------------------------------------
 * Init
 * ------------------------------------------------------------------------- */
AppError AppTask::Init()
{
#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    ResetCount_Init();
#endif

    PRINT_APP_VERSION(GP_VERSIONINFO_GLOBAL_VERSION);

    /* Create the main event queue */
    sAppEventQueue = xQueueCreateStatic(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent),
                                        sAppEventQueueBuffer, &sAppEventQueueStruct);
    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("Failed to allocate app event queue", 0);
        return APP_NO_MEMORY;
    }

    /* Create the main application task */
    sAppTaskHandle = xTaskCreateStatic(Main, APP_TASK_NAME,
                                       (sizeof(appStack) / sizeof(appStack[0])),
                                       nullptr, APP_TASK_PRIORITY,
                                       appStack, &appTaskStruct);
    if(sAppTaskHandle == nullptr)
    {
        return APP_NO_MEMORY;
    }

    /* Initialise digital button handler (PB1) */
#if defined(GP_APP_DIVERSITY_BUTTONHANDLER)
    GetAppButtons().Init();
#endif

    /* Initialise BLE + Thread application manager */
    GetAppMgr().Init();

    /* Initialise GPADC for GPIO 28 analog doorbell button */
    if(!DoorbellManager::Init())
    {
        GP_LOG_SYSTEM_PRINTF("WARNING: DoorbellManager GPADC init failed", 0);
        /* Non-fatal: continue without analog button */
    }
    else
    {
        /* Start the ADC polling task (runs independently of the main task) */
        DoorbellManager::StartPolling();
    }

    GP_LOG_SYSTEM_PRINTF("AppTask init done", 0);
    return APP_NO_ERROR;
}

/* -------------------------------------------------------------------------
 * EnableSleep
 * ------------------------------------------------------------------------- */
void AppTask::EnableSleep(bool enable)
{
    if(enable)
    {
#if defined(GP_DIVERSITY_GPHAL_XP4002)
        hal_PowerModeResult_t ret;
        ret = hal_SetPowerMode(hal_StandbyPowerModeELPS, hal_ActivePowerModeEHPS, hal_SleepModeRC);
        GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, hal_PowerModeResultSuccess == ret);
#else
        if(!GP_BSP_32KHZ_CRYSTAL_AVAILABLE())
        {
            gpHal_SetSleepMode(gpHal_SleepModeRC);
        }
#endif
        hal_SleepSetGotoSleepThreshold(APP_GOTOSLEEP_THRESHOLD);
    }
    hal_SleepSetGotoSleepEnable(enable);
}

/* -------------------------------------------------------------------------
 * Main  - FreeRTOS task body
 * ------------------------------------------------------------------------- */
void AppTask::Main(void* /*pvParameter*/)
{
    AppEvent event;

    while(true)
    {
        if(xQueueReceive(sAppEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
        }
    }
}

/* -------------------------------------------------------------------------
 * PostEvent  - safe to call from ISR or task context
 * ------------------------------------------------------------------------- */
void AppTask::PostEvent(AppEvent* aEvent)
{
    if((aEvent == nullptr) || (aEvent->Type == AppEvent::kEventType_Invalid))
    {
        return;
    }

    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("Event queue is null", 0);
        return;
    }

    xPSR_Type psr;
    psr.w = __get_xPSR();

    if(psr.b.ISR != 0)
    {
        /* Called from an interrupt */
        BaseType_t woken = pdFALSE;
        if(!xQueueSendFromISR(sAppEventQueue, aEvent, &woken))
        {
            GP_LOG_SYSTEM_PRINTF("IRQ: failed to post event", 0);
        }
        else if(woken)
        {
            portYIELD_FROM_ISR(woken);
        }
    }
    else
    {
        if(!xQueueSend(sAppEventQueue, aEvent, 1))
        {
            GP_LOG_SYSTEM_PRINTF("Failed to post event (queue full?)", 0);
        }
    }
}

/* -------------------------------------------------------------------------
 * DispatchEvent
 * ------------------------------------------------------------------------- */
void AppTask::DispatchEvent(AppEvent* aEvent)
{
    GetAppMgr().EventHandler(aEvent);
}

/* -------------------------------------------------------------------------
 * FactoryReset / ResetSystem
 * ------------------------------------------------------------------------- */
void AppTask::FactoryReset(void)
{
    ResetSystem();
}

void AppTask::ResetSystem(void)
{
    gpSched_ScheduleEvent(0, gpReset_ResetBySwPor);
}
