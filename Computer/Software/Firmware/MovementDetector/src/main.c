/*
 * Copyright (c) 2024-2025, Qorvo Inc
 */

#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"
#include "qPinCfg.h"
#include "qDrvGPIO.h"
#include "qDrvIOB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "StatusLed.h"
#include <stdlib.h>

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
#define INTERVAL_MS 1000
#define LED_TASK_PRIORITY           (tskIDLE_PRIORITY + 1)
#define SENSOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)
#define LED_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 1000)
#define SENSOR_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000)
#define TRIG_GPIO 10
#define ECHO_GPIO 11

static void ledToggle_Task(void* pvParameters);
static void sensor_Task(void* pvParameters);
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

static void ledToggle_Task(void* pvParameters)
{
    Bool value = false;
    while(1)
    {
        StatusLed_SetLed(GREEN_LED_GPIO_PIN, value);
        value = !value;
        vTaskDelay(INTERVAL_MS / 2);
    }
    vTaskDelete(NULL);
}

static void sensor_Task(void* pvParameters)
{
    static int prev_distance = 0;

    qDrvIOB_ConfigOutputSet(TRIG_GPIO, qDrvIOB_Drive2mA, qDrvIOB_SlewRateSlow);
    qDrvGPIO_Write(TRIG_GPIO, 0);

    qDrvIOB_ConfigInputSet(ECHO_GPIO, qDrvIOB_PullNone, false);

    while(1)
    {
        qDrvGPIO_Write(TRIG_GPIO, 1);
        hal_Waitus(10);
        qDrvGPIO_Write(TRIG_GPIO, 0);

        uint32_t timeout = 10000;
        while (qDrvGPIO_Read(ECHO_GPIO) == 0 && timeout-- > 0) {}

        if (timeout == 0) {
            GP_LOG_SYSTEM_PRINTF("Sensor timeout - no echo", 0);
            vTaskDelay(INTERVAL_MS);
            continue;
        }

        uint32_t start_time = gpSched_GetCurrentTime();

        timeout = 30000;
        while (qDrvGPIO_Read(ECHO_GPIO) == 1 && timeout-- > 0) {}

        if (timeout == 0) {
            GP_LOG_SYSTEM_PRINTF("Sensor timeout - echo stuck high", 0);
            vTaskDelay(INTERVAL_MS);
            continue;
        }

        uint32_t end_time = gpSched_GetCurrentTime();
        uint32_t time_us = end_time - start_time;
        int distance_cm = (time_us * 343) / 20000;

        if (distance_cm >= 2 && distance_cm <= 400) {
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
    vTaskDelete(NULL);
}

void Application_Init(void)
{
    gpHal_Set32kHzCrystalAvailable(false);
    gpBaseComps_StackInit();
    gpCom_Init();
    gpLog_Init();

    qResult_t res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);

    static StaticTask_t xLedTaskPCB;
    static StackType_t xLedTaskStack[LED_TASK_STACK_SIZE];
    TaskHandle_t ledTaskHandle = xTaskCreateStatic(
        ledToggle_Task, "ledToggle_Task", LED_TASK_STACK_SIZE,
        NULL, LED_TASK_PRIORITY, xLedTaskStack, &xLedTaskPCB);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, ledTaskHandle != NULL);

    static StaticTask_t xSensorTaskPCB;
    static StackType_t xSensorTaskStack[SENSOR_TASK_STACK_SIZE];
    TaskHandle_t sensorTaskHandle = xTaskCreateStatic(
        sensor_Task, "sensor_Task", SENSOR_TASK_STACK_SIZE,
        NULL, SENSOR_TASK_PRIORITY, xSensorTaskStack, &xSensorTaskPCB);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, sensorTaskHandle != NULL);
}

int main(void) 
{
    HAL_INITIALIZE_GLOBAL_INT();
    HAL_INIT();
    HAL_ENABLE_GLOBAL_INT();
    gpSched_Init();
    gpSched_ScheduleEvent(0, Application_Init);
    vTaskStartScheduler();
    return 0;
}
