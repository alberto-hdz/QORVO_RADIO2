/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "AppTask.cpp"
 *
 * Main FreeRTOS application task for the QPG6200 Thread+BLE MaxSonar Motion Detector.
 *
 * Initialisation order:
 *   1. FreeRTOS event queue
 *   2. AppTask FreeRTOS task (spawns main loop)
 *   3. ButtonHandler (PB1 commissioning button)
 *   4. AppManager::Init() -> BLE stack init + GATT + advertising + Thread init
 *   5. SensorManager::Init() -> UART peripheral configuration
 *   6. SensorManager::StartSensing() -> sensor polling FreeRTOS task
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
#include "SensorManager.h"

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
#include "ResetCount.h"
#endif

#include "hal_PowerMode.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define APP_EVENT_QUEUE_SIZE  20
#define APP_TASK_STACK_SIZE   (6 * 1024)
#define APP_TASK_PRIORITY     2

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

AppError AppTask::Init()
{
#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    ResetCount_Init();
#endif

    PRINT_APP_VERSION(GP_VERSIONINFO_GLOBAL_VERSION);

    sAppEventQueue = xQueueCreateStatic(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent),
                                        sAppEventQueueBuffer, &sAppEventQueueStruct);
    if(sAppEventQueue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("Failed to allocate app event queue", 0);
        return APP_NO_MEMORY;
    }

    sAppTaskHandle = xTaskCreateStatic(Main, APP_TASK_NAME,
                                       (sizeof(appStack) / sizeof(appStack[0])),
                                       nullptr, APP_TASK_PRIORITY,
                                       appStack, &appTaskStruct);
    if(sAppTaskHandle == nullptr)
    {
        return APP_NO_MEMORY;
    }

#if defined(GP_APP_DIVERSITY_BUTTONHANDLER)
    GetAppButtons().Init();
#endif

    GetAppMgr().Init();

    if(!SensorManager::Init())
    {
        GP_LOG_SYSTEM_PRINTF("WARNING: SensorManager init failed", 0);
    }
    else
    {
        SensorManager::StartSensing();
    }

    GP_LOG_SYSTEM_PRINTF("AppTask init done", 0);
    return APP_NO_ERROR;
}

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

void AppTask::DispatchEvent(AppEvent* aEvent)
{
    GetAppMgr().EventHandler(aEvent);
}

void AppTask::FactoryReset(void)
{
    ResetSystem();
}

void AppTask::ResetSystem(void)
{
    gpSched_ScheduleEvent(0, gpReset_ResetBySwPor);
}
