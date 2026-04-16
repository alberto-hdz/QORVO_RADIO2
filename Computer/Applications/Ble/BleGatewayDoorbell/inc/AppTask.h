/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * @file AppTask.h
 * @brief FreeRTOS task and event queue interface for the BLE Gateway Doorbell demo.
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

inline AppTask& GetAppTask(void) { return AppTask::sAppTask; }
#endif /* __cplusplus */

#endif /* _APPTASK_H_ */
