/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * This software is owned by Qorvo Inc and protected under applicable copyright
 * laws. It is delivered under the terms of the license and is intended and
 * supplied for use solely and exclusively with products manufactured by
 * Qorvo Inc.
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO
 * THIS SOFTWARE. QORVO INC. SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */

/**
 * @file AppTask.cpp
 * @brief FreeRTOS task and event queue for the BLE Gateway Doorbell demo.
 *
 * AppTask owns:
 *   - A statically-allocated FreeRTOS task (AppTask::Main)
 *   - A statically-allocated FreeRTOS message queue (sAppEventQueue)
 *
 * All application events (BLE stack callbacks, button ISRs, heartbeat timer)
 * are serialised through PostEvent() and processed sequentially in the
 * AppTask::Main() loop to avoid race conditions.
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

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
#include "ResetCount.h"
#endif

#include "hal_PowerMode.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------- */
/*  Task configuration                                                        */
/* -------------------------------------------------------------------------- */

/** @brief Maximum number of events that can be queued before dropping. */
#define APP_EVENT_QUEUE_SIZE 20

/** @brief Stack size for the application task (bytes). */
#define APP_TASK_STACK_SIZE  (4 * 1024)

/** @brief FreeRTOS task priority. */
#define APP_TASK_PRIORITY    2

/**
 * @brief Inactivity threshold before allowing sleep entry (microseconds).
 *
 * Prevents the SoC from entering deep sleep while there is pending work
 * that will complete within this window.
 */
#ifndef APP_GOTOSLEEP_THRESHOLD
#define APP_GOTOSLEEP_THRESHOLD 1000
#endif

/** @brief Maximum number of registered GPIO callbacks. */
#define MAX_GPIO_CALLBACKS 3

/* Macro that expands the global version tuple into a printf call. */
#define _PRINT_APP_VERSION(_major, _minor, _revision, _patch) \
    { GP_LOG_SYSTEM_PRINTF("Software Version: %d.%d.%d.%d", 0, \
                           _major, _minor, _revision, _patch); }
#define PRINT_APP_VERSION(_args) _PRINT_APP_VERSION(_args)

/* -------------------------------------------------------------------------- */
/*  Static storage (avoids dynamic heap allocation)                           */
/* -------------------------------------------------------------------------- */

namespace {
uint8_t        sAppEventQueueBuffer[APP_EVENT_QUEUE_SIZE * sizeof(AppEvent)];
StaticQueue_t  sAppEventQueueStruct;
QueueHandle_t  sAppEventQueue;

StackType_t    appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t   appTaskStruct;
TaskHandle_t   sAppTaskHandle;
} // namespace

AppTask AppTask::sAppTask;

/* -------------------------------------------------------------------------- */
/*  AppTask public methods                                                    */
/* -------------------------------------------------------------------------- */

AppError AppTask::Init()
{
#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    ResetCount_Init();
#endif

    PRINT_APP_VERSION(GP_VERSIONINFO_GLOBAL_VERSION);

    /* Create the statically-allocated event queue. */
    sAppEventQueue = xQueueCreateStatic(
        APP_EVENT_QUEUE_SIZE,
        sizeof(AppEvent),
        sAppEventQueueBuffer,
        &sAppEventQueueStruct
    );
    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: failed to create app event queue", 0);
        return APP_NO_MEMORY;
    }

    /* Create the statically-allocated application task. */
    sAppTaskHandle = xTaskCreateStatic(
        Main,
        APP_TASK_NAME,
        sizeof(appStack) / sizeof(appStack[0]),
        nullptr,
        APP_TASK_PRIORITY,
        appStack,
        &appTaskStruct
    );
    if(sAppTaskHandle == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: failed to create app task", 0);
        return APP_NO_MEMORY;
    }

#if defined(GP_APP_DIVERSITY_BUTTONHANDLER)
    /* Set up the button handler (registers GPIO interrupt callbacks). */
    GetAppButtons().Init();
#endif

    /* Initialise the application-level logic. */
    GetAppMgr().Init();

    GP_LOG_SYSTEM_PRINTF("AppTask init done", 0);
    return APP_NO_ERROR;
}

/* -------------------------------------------------------------------------- */

void AppTask::EnableSleep(bool enable)
{
    if(enable)
    {
#if defined(GP_DIVERSITY_GPHAL_XP4002)
        hal_PowerModeResult_t result = hal_SetPowerMode(
            hal_StandbyPowerModeELPS,
            hal_ActivePowerModeEHPS,
            hal_SleepModeRC
        );
        GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM,
                  hal_PowerModeResultSuccess == result);
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

/* -------------------------------------------------------------------------- */

/**
 * @brief Main FreeRTOS task loop.
 *
 * Blocks indefinitely on the event queue.  Each dequeued event is forwarded
 * to AppManager for dispatch.  All application logic runs in this context.
 */
void AppTask::Main(void* pvParameter)
{
    (void)pvParameter;

    AppEvent event;

    while(true)
    {
        if(xQueueReceive(sAppEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
        }
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Thread-safe event submission.
 *
 * If called from an ISR context (checked via xPSR.ISR) the ISR-safe queue
 * API is used; otherwise the normal task API is used.
 *
 * @param aEvent  Pointer to the event to enqueue.  Ignored if NULL or Invalid.
 */
void AppTask::PostEvent(AppEvent* aEvent)
{
    if((aEvent == NULL) || (aEvent->Type == AppEvent::kEventType_Invalid))
    {
        return;
    }

    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: event queue is null – event dropped", 0);
        return;
    }

    /* Determine whether we are inside an ISR by reading the xPSR register. */
    xPSR_Type psr;
    psr.w = __get_xPSR();

    if(psr.b.ISR != 0)
    {
        /* ISR context – use the ISR-safe queue API. */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if(!xQueueSendFromISR(sAppEventQueue, aEvent, &xHigherPriorityTaskWoken))
        {
            GP_LOG_SYSTEM_PRINTF("IRQ: failed to post event (queue full?)", 0);
        }
        else if(xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        /* Task context – use the standard queue API. */
        if(!xQueueSend(sAppEventQueue, aEvent, 1))
        {
            GP_LOG_SYSTEM_PRINTF("ERROR: failed to post event (queue full?)", 0);
        }
    }
}

/* -------------------------------------------------------------------------- */

void AppTask::DispatchEvent(AppEvent* aEvent)
{
    GetAppMgr().EventHandler(aEvent);
}

/* -------------------------------------------------------------------------- */

void AppTask::FactoryReset(void)
{
    ResetSystem();
}

/* -------------------------------------------------------------------------- */

void AppTask::ResetSystem(void)
{
    gpSched_ScheduleEvent(0, gpReset_ResetBySwPor);
}
