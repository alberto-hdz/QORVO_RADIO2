/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "AppTask.h"
 *
 * Main FreeRTOS application task for the QPG6200 Thread+BLE MaxSonar Motion Detector.
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

class AppTask
{
public:
    AppError Init();
    void     EnableSleep(bool enable);
    void     PostEvent(AppEvent* event);
    void     FactoryReset(void);
    static void ResetSystem(void);

private:
    friend AppTask& GetAppTask(void);
    static void Main(void* pvParameter);
    void        DispatchEvent(AppEvent* event);

    static AppTask sAppTask;
};

inline AppTask& GetAppTask(void)
{
    return AppTask::sAppTask;
}

#endif //__cplusplus

#endif // _APPTASK_H_
