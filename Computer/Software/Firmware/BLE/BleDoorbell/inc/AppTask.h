/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 *
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

/** @file "AppTask.h"
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

class AppTask {
    public:
    AppError Init();
    void EnableSleep(bool enable);
    void PostEvent(AppEvent* event);
    void FactoryReset(void);
    static void ResetSystem(void);

    private:
    friend AppTask& GetAppTask(void);
    static void Main(void* pvParameter);
    void DispatchEvent(AppEvent* event);

    static AppTask sAppTask;
};

inline AppTask& GetAppTask(void)
{
    return AppTask::sAppTask;
}

#endif //__cplusplus

#endif // APP_TASK_H
