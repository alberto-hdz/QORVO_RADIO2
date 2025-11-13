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
 * Movement Detector application with HC-SR04.
 *
 */

/*****************************************************************************
 *                    Includes Definitions
 *****************************************************************************/
#include "hal.h"  // Hardware Abstraction Layer gives access to microcontroller features
#include "gpHal_DEFS.h" // Qorvo's low level GPIO and hardware macros/constants
#include "gpBaseComps.h" // Qorvo's base libraries for communication, logging, and scheduling
#include "gpCom.h" // Qorvo's base libraries for communication, logging, and scheduling
#include "gpLog.h" // Qorvo's base libraries for communication, logging, and scheduling
#include "gpSched.h" // Qorvo's base libraries for communication, logging, and scheduling

#include "qPinCfg.h" // Handles pin configuration 
#include "qDrvGPIO.h" // GPIO driver functions

#include "FreeRTOS.h" // Core FreeRTOS headers for multitasking and scheduling
#include "task.h" // Core FreeRTOS headers for multitasking and scheduling

#include "StatusLed.h" // Manages onboard LEDs for visual feedback 

/*****************************************************************************
 *                    Macro Definitions
 *****************************************************************************/
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define INTERVAL_MS 1000 // How often tasks delay 

#define LED_TASK_PRIORITY           (tskIDLE_PRIORITY + 1) // both tasks have equal priority just above idle 
#define SENSOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 1) // both tasks have equal priority just above idle
#define LED_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 1000) // each task gets memory for its execution stack
#define SENSOR_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000) // each task gets memory for its execution stack

#define TRIG_GPIO 10  // GPIO10 for Trig (output)
#define ECHO_GPIO 11  // GPIO11 for Echo (input)

/******************************************************************************
 *                    Static Function Declarations
 ******************************************************************************/
static void ledToggle_Task(void* pvParameters);
static void sensor_Task(void* pvParameters);

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/******************************************************************************
 *                    Static Function Definitions 
 ******************************************************************************/
static void ledToggle_Task(void* pvParameters) // LED Task
{
    Bool value = false; // starts with LED off

    while(1)
    {
        StatusLed_SetLed(GREEN_LED_GPIO_PIN, value); // turns LED on or off depending on value

        value = !value; // toggles the LED state each iteration
        vTaskDelay(INTERVAL_MS / 2); // waits 500 ms before repeating
    }
    /* shall never be reached */
    vTaskDelete(NULL);
}

static void sensor_Task(void* pvParameters) // Measuring Distance
{
    static int prev_distance = 0;

    // Configure GPIO pins
    qDrvGPIO_PinConfig_t trigConfig;
    trigConfig.mode = qDrvIOB_ModeGpio;
    trigConfig.direction = qDrvGPIO_DirectionOutput;
    trigConfig.pull = qDrvIOB_PullNone;
    qDrvGPIO_SetPinConfig(TRIG_GPIO, &trigConfig);
    qDrvGPIO_WritePin(TRIG_GPIO, 0);

    qDrvGPIO_PinConfig_t echoConfig;
    echoConfig.mode = qDrvIOB_ModeGpio;
    echoConfig.direction = qDrvGPIO_DirectionInput;
    echoConfig.pull = qDrvIOB_PullNone;
    qDrvGPIO_SetPinConfig(ECHO_GPIO, &echoConfig);

    while(1)
    {
        // Send 10us trigger pulse
        qDrvGPIO_WritePin(TRIG_GPIO, 1);
        hal_WaitUs(10);
        qDrvGPIO_WritePin(TRIG_GPIO, 0);

        // Wait for echo to go high (start) - with timeout
        uint32_t timeout = 10000;
        while (qDrvGPIO_ReadPin(ECHO_GPIO) == 0 && timeout-- > 0) {}

        if (timeout == 0) {
            GP_LOG_SYSTEM_PRINTF("Sensor timeout - no echo", 0);
            vTaskDelay(INTERVAL_MS);
            continue;
        }

        // Start timing
        uint32_t start_time = gpSched_GetCurrentTime();

        // Wait for echo to go low (end) - with timeout
        timeout = 30000;
        while (qDrvGPIO_ReadPin(ECHO_GPIO) == 1 && timeout-- > 0) {}

        if (timeout == 0) {
            GP_LOG_SYSTEM_PRINTF("Sensor timeout - echo stuck high", 0);
            vTaskDelay(INTERVAL_MS);
            continue;
        }

        // End timing
        uint32_t end_time = gpSched_GetCurrentTime();

        // Calculate time in us, distance in cm (speed of sound 0.0343 cm/us, /2 for round trip)
        uint32_t time_us = end_time - start_time;
        int distance_cm = (time_us * 343) / 20000; // More accurate calculation

        // Only log if distance is reasonable (2-400cm for HC-SR04)
        if (distance_cm >= 2 && distance_cm <= 400) {
            // Detect movement if distance changed significantly
            if (distance_cm != prev_distance && abs(distance_cm - prev_distance) > 2) {
                GP_LOG_SYSTEM_PRINTF("Movement detected! Distance: %d cm", 0, distance_cm);
                prev_distance = distance_cm;
            } else {
                GP_LOG_SYSTEM_PRINTF("No movement. Distance: %d cm", 0, distance_cm);
            }
        } else {
            GP_LOG_SYSTEM_PRINTF("Out of range reading: %d cm", 0, distance_cm);
        }

        vTaskDelay(INTERVAL_MS);
    }
    /* shall never be reached */
    vTaskDelete(NULL);
}

/******************************************************************************
 *                   External Function Definitions
 ******************************************************************************/

void Application_Init(void) // App initialization
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

    // Sensor task initialization
    static StaticTask_t xSensorTaskPCB;
    static StackType_t xSensorTaskStack[SENSOR_TASK_STACK_SIZE];
    TaskHandle_t sensorTaskHandle = xTaskCreateStatic(
        sensor_Task,   /* The function that implements the task. */
        "sensor_Task", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
        SENSOR_TASK_STACK_SIZE, /* The size of the stack to allocate to the task. */
        NULL,                        /* The parameter passed to the task */
        SENSOR_TASK_PRIORITY,   /* The priority assigned to the task. */
        xSensorTaskStack,        /* The task stack memory */
        &xSensorTaskPCB);        /* The task PCB memory */
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, sensorTaskHandle != NULL);
}

int main(void) 
{
    HAL_INITIALIZE_GLOBAL_INT(); // Enable interrupts 

    // Hardware initialization
    HAL_INIT();

    HAL_ENABLE_GLOBAL_INT(); // Enable interrupts

    // Scheduler initialization
    gpSched_Init();

    /* Make sure to run the stack-intensive initialisation code from the scheduler task with larger stack */
    gpSched_ScheduleEvent(0, Application_Init);

    vTaskStartScheduler();

    return 0;
}
