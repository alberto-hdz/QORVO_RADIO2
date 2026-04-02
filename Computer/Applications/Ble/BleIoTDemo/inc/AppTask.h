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
 * @file AppTask.h
 * @brief FreeRTOS main application task interface.
 *
 * AppTask owns the event queue and the top-level FreeRTOS task that drives
 * the application. All other modules post AppEvent structures to this task
 * via PostEvent(), which handles both ISR and task contexts transparently.
 */

#ifndef _APPTASK_H_
#define _APPTASK_H_

typedef enum { APP_NO_ERROR = 0, APP_NO_MEMORY, APP_ERROR_COUNT } AppError;

#ifdef __cplusplus

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"
#include "qDrvGPIO.h"
#include "FreeRTOS.h"

#define APP_TASK_NAME "AppTask"

/**
 * @brief Main application task class.
 *
 * Singleton class (accessed via GetAppTask()) that owns the FreeRTOS task
 * and event queue. Other tasks/ISRs call PostEvent() to inject events;
 * the main loop dequeues them and dispatches to AppManager.
 */
class AppTask {
   public:
    /** @brief Initialise the event queue, FreeRTOS task, buttons, and AppManager. */
    AppError Init();

    /** @brief Enable or disable low-power sleep mode. */
    void EnableSleep(bool enable);

    /**
     * @brief Post an event to the application event queue.
     *
     * Safe to call from both task and ISR contexts. Events of type
     * kEventType_Invalid are silently dropped.
     *
     * @param event Pointer to the event to enqueue (copied by value).
     */
    void PostEvent(AppEvent* event);

    /** @brief Trigger a factory-reset sequence (resets the system). */
    void FactoryReset(void);

    /** @brief Trigger a software reset via the watchdog. */
    static void ResetSystem(void);

   private:
    friend AppTask& GetAppTask(void);

    /** @brief FreeRTOS task entry point. */
    static void Main(void* pvParameter);

    /** @brief Route a dequeued event to AppManager. */
    void DispatchEvent(AppEvent* event);

    static AppTask sAppTask;
};

/** @brief Global accessor for the AppTask singleton. */
inline AppTask& GetAppTask(void)
{
    return AppTask::sAppTask;
}

#endif // __cplusplus

#endif // _APPTASK_H_
