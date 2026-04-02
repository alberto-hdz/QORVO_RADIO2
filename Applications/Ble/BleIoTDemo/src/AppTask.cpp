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
 * @brief FreeRTOS main application task for the BLE IoT Demo.
 *
 * AppTask owns:
 *   - A statically-allocated FreeRTOS event queue (APP_EVENT_QUEUE_SIZE entries)
 *   - The main application FreeRTOS task (APP_TASK_STACK_SIZE bytes of stack)
 *
 * Initialisation sequence
 * -----------------------
 *   1. Optional: reset counter (if GP_APP_DIVERSITY_RESETCOUNTING is defined)
 *   2. Create event queue
 *   3. Create main FreeRTOS task
 *   4. Initialise ButtonHandler (if GP_APP_DIVERSITY_BUTTONHANDLER is defined)
 *   5. Call AppManager::Init() to set up BLE, LEDs, and the heartbeat timer
 *
 * Event flow
 * ----------
 *   Any context (ISR, timer callback, BLE stack task) calls PostEvent().
 *   PostEvent() uses the ISR-safe queue API when called from an interrupt,
 *   and the normal queue API when called from a task.
 *   The main task loop dequeues events and dispatches them to AppManager.
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
/*  Task and queue configuration                                              */
/* -------------------------------------------------------------------------- */

/** @brief Maximum number of events in the application queue. */
#define APP_EVENT_QUEUE_SIZE 20

/**
 * @brief AppTask stack size in bytes.
 *
 * 4 KB is sufficient for BLE-only operation with FreeRTOS software timers.
 */
#define APP_TASK_STACK_SIZE  (4 * 1024)

/** @brief FreeRTOS task priority (numerically higher = higher priority). */
#define APP_TASK_PRIORITY    2

/**
 * @brief Idle time (µs) before entering low-power sleep.
 *
 * 1000 µs gives a good balance between responsiveness and power saving.
 */
#ifndef APP_GOTOSLEEP_THRESHOLD
#define APP_GOTOSLEEP_THRESHOLD 1000
#endif

/* Version print helper. */
#define _PRINT_APP_VERSION(_major, _minor, _revision, _patch) \
    GP_LOG_SYSTEM_PRINTF("SW Version: %d.%d.%d.%d", 0, _major, _minor, _revision, _patch)
#define PRINT_APP_VERSION(_args) _PRINT_APP_VERSION(_args)

/* -------------------------------------------------------------------------- */
/*  Static allocations                                                        */
/* -------------------------------------------------------------------------- */

namespace {
    /** @brief Raw backing store for the event queue. */
    uint8_t       sAppEventQueueBuffer[APP_EVENT_QUEUE_SIZE * sizeof(AppEvent)];
    StaticQueue_t sAppEventQueueStruct;
    QueueHandle_t sAppEventQueue;

    /** @brief Static stack for the main application task. */
    StackType_t   appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
    StaticTask_t  appTaskStruct;
    TaskHandle_t  sAppTaskHandle;
} // namespace

AppTask AppTask::sAppTask;

/* ========================================================================== */
/*  AppTask public methods                                                    */
/* ========================================================================== */

AppError AppTask::Init()
{
#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    ResetCount_Init();
#endif

    PRINT_APP_VERSION(GP_VERSIONINFO_GLOBAL_VERSION);

    /* Create the event queue using statically allocated memory. */
    sAppEventQueue = xQueueCreateStatic(
        APP_EVENT_QUEUE_SIZE,
        sizeof(AppEvent),
        sAppEventQueueBuffer,
        &sAppEventQueueStruct
    );
    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: Failed to allocate application event queue", 0);
        return APP_NO_MEMORY;
    }

    /* Create the main application task. */
    sAppTaskHandle = xTaskCreateStatic(
        Main,
        APP_TASK_NAME,
        (sizeof(appStack) / sizeof(appStack[0])),
        nullptr,
        APP_TASK_PRIORITY,
        appStack,
        &appTaskStruct
    );
    if(sAppTaskHandle == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: Failed to create application task", 0);
        return APP_NO_MEMORY;
    }

#if defined(GP_APP_DIVERSITY_BUTTONHANDLER)
    /* Initialise the button handler before AppManager so buttons are ready. */
    GetAppButtons().Init();
#endif

    /* Initialise the application (BLE, LEDs, heartbeat timer). */
    GetAppMgr().Init();

    GP_LOG_SYSTEM_PRINTF("AppTask initialisation complete", 0);

    return APP_NO_ERROR;
}

/* -------------------------------------------------------------------------- */

void AppTask::EnableSleep(bool enable)
{
    if(enable)
    {
#if defined(GP_DIVERSITY_GPHAL_XP4002)
        hal_PowerModeResult_t result =
            hal_SetPowerMode(hal_StandbyPowerModeELPS,
                             hal_ActivePowerModeEHPS,
                             hal_SleepModeRC);
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
 * @brief Post an event to the application event queue.
 *
 * Detects whether the caller is in an ISR (by reading the IPSR register) and
 * uses the appropriate FreeRTOS API.  Events of type kEventType_Invalid are
 * dropped before touching the queue.
 */
void AppTask::PostEvent(AppEvent* aEvent)
{
    if((aEvent == nullptr) || (aEvent->Type == AppEvent::kEventType_Invalid))
    {
        return;
    }

    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: PostEvent called before queue created", 0);
        return;
    }

    /* Check IPSR to detect ISR context. */
    xPSR_Type psr;
    psr.w = __get_xPSR();

    if(psr.b.ISR != 0)
    {
        /* Called from an interrupt service routine. */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if(!xQueueSendFromISR(sAppEventQueue, aEvent, &xHigherPriorityTaskWoken))
        {
            GP_LOG_SYSTEM_PRINTF("IRQ: Failed to post event (queue full?)", 0);
        }
        else if(xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
    else
    {
        /* Called from a task context. */
        if(!xQueueSend(sAppEventQueue, aEvent, pdMS_TO_TICKS(1)))
        {
            GP_LOG_SYSTEM_PRINTF("Failed to post event (queue full?)", 0);
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

/* ========================================================================== */
/*  Main task loop (private)                                                 */
/* ========================================================================== */

/**
 * @brief FreeRTOS task body – dequeues and dispatches events forever.
 *
 * Blocks indefinitely on the event queue; when an event arrives it is
 * dispatched to AppManager synchronously.  The task never returns.
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
