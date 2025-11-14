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

/** @file "main.c"
 *
 * Hello World application.
 *
 */

/*****************************************************************************
 *                    Includes Definitions
 *****************************************************************************/
#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"

#include "qPinCfg.h"

#include "FreeRTOS.h"
#include "task.h"

#include "StatusLed.h"

/*****************************************************************************
 *                    Macro Definitions
 *****************************************************************************/
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define INTERVAL_MS 1000

#define LED_TASK_PRIORITY           (tskIDLE_PRIORITY + 1)
#define HELLO_WORLD_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)
#define LED_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 1000)
#define HELLO_WORLD_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000)

/******************************************************************************
 *                    Static Function Declarations
 ******************************************************************************/
static void ledToggle_Task(void* pvParameters);
static void helloWorld_Task(void* pvParameters);

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/******************************************************************************
 *                    Static Function Definitions
 ******************************************************************************/
static void ledToggle_Task(void* pvParameters)
{
    Bool value = false;

    while(1)
    {
        StatusLed_SetLed(GREEN_LED_GPIO_PIN, value);

        value = !value;
        vTaskDelay(INTERVAL_MS / 2);
    }
    /* shall never be reached */
    vTaskDelete(NULL);
}

static void helloWorld_Task(void* pvParameters)
{
    while(1)
    {
        GP_LOG_SYSTEM_PRINTF("Hello world", 0);
        vTaskDelay(INTERVAL_MS);
    }
    /* shall never be reached */
    vTaskDelete(NULL);
}

/******************************************************************************
 *                   External Function Definitions
 ******************************************************************************/

void Application_Init(void)
{
    gpHal_Set32kHzCrystalAvailable(false);

    // Qorvo components initialization
    gpBaseComps_StackInit();
    // gpCom and gpLog are not initialized in gpBaseComps_StackInit()
    gpCom_Init();
    gpLog_Init();

    qResult_t res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);

    // Led task initialization
    static StaticTask_t xLedTaskPCB;
    static StackType_t xLedTaskStack[LED_TASK_STACK_SIZE];
    TaskHandle_t ledTaskHandle = xTaskCreateStatic(
        ledToggle_Task,      /* The function that implements the task. */
        "ledToggle_Task",    /* The text name assigned to the task - for debug only as it is not used by the kernel. */
        LED_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
        NULL,                /* The parameter passed to the task */
        LED_TASK_PRIORITY,   /* The priority assigned to the task. */
        xLedTaskStack,       /* The task stack memory */
        &xLedTaskPCB);       /* The task PCB memory */
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, ledTaskHandle != NULL);

    // Hello world task initialization
    static StaticTask_t xHelloWorldTaskPCB;
    static StackType_t xHelloWorldTaskStack[HELLO_WORLD_TASK_STACK_SIZE];
    TaskHandle_t helloWorldTaskHandle = xTaskCreateStatic(
        helloWorld_Task,   /* The function that implements the task. */
        "helloWorld_Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
        HELLO_WORLD_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
        NULL,                        /* The parameter passed to the task */
        HELLO_WORLD_TASK_PRIORITY,   /* The priority assigned to the task. */
        xHelloWorldTaskStack,        /* The task stack memory */
        &xHelloWorldTaskPCB);        /* The task PCB memory */
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, helloWorldTaskHandle != NULL);
}

int main(void)
{
    HAL_INITIALIZE_GLOBAL_INT();

    // Hardware initialization
    HAL_INIT();

    HAL_ENABLE_GLOBAL_INT();

    // Scheduler initialization
    gpSched_Init();

    /* Make sure to run the stack-intensive initialisation code from the scheduler task with larger stack */
    gpSched_ScheduleEvent(0, Application_Init);

    vTaskStartScheduler();

    return 0;
}
