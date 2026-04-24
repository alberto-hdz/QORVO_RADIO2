/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "main.c"
 *
 * MaxSonar_Test - minimal LV-MaxSonar EZ sensor test for the QPG6200L DK.
 *
 * Reads the sensor's free-running serial output and prints each distance
 * reading to the debug console. No LEDs, no wireless stack.
 *
 * Wiring (sensor -> QPG6200L DK):
 *   Sensor +5  -> 5 V     (sensor requires 5 V supply)
 *   Sensor GND -> GND
 *   Sensor TX  -> GPIO 11 (UART0 RX)  sensor streams ASCII range frames
 *   Sensor RX  -> GPIO 10 (UART0 TX)  held HIGH -> continuous ranging
 *
 * Sensor protocol: 9600 baud, 8N1, frame = 'R' + 3 ASCII digits + '\r'
 *   e.g. "R079\r" = 79 inches. cm = inches * 254 / 100.
 *
 * Debug console: UART1 (GPIO 8/9) via J-Link USB, 115200 baud.
 */

#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"

#include "qPinCfg.h"
#include "qDrvUART.h"

#include "FreeRTOS.h"
#include "task.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define MAXSONAR_UART_INSTANCE  0
#define MAXSONAR_UART_BAUD      9600u
#define SENSOR_POLL_MS          50u

#define SENSOR_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 512)
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

static qDrvUART_t sUart = Q_DRV_UART_INSTANCE_DEFINE(MAXSONAR_UART_INSTANCE);

static StaticTask_t sSensorTaskPCB;
static StackType_t  sSensorStack[SENSOR_TASK_STACK_SIZE];

static void SensorTask(void* pvParameters)
{
    (void)pvParameters;

    /* Parser state: wait for 'R', then 3 digits, then CR. */
    uint8_t state = 0;
    uint8_t digits[3] = {0};
    uint8_t idx = 0;

    vTaskDelay(pdMS_TO_TICKS(500));
    GP_LOG_SYSTEM_PRINTF("[Sensor] polling started", 0);

    while(true)
    {
        UInt8 byte;
        while(qDrvUART_RxNewDataCheck(&sUart))
        {
            if(qDrvUART_Rx(&sUart, &byte) != Q_OK)
            {
                continue;
            }

            switch(state)
            {
                case 0:
                    if(byte == 'R') { state = 1; idx = 0; }
                    break;
                case 1:
                    if(byte >= '0' && byte <= '9')
                    {
                        digits[idx++] = byte;
                        if(idx >= 3) state = 2;
                    }
                    else { state = 0; idx = 0; }
                    break;
                case 2:
                    if(byte == '\r')
                    {
                        uint16_t inches = (digits[0] - '0') * 100u +
                                          (digits[1] - '0') * 10u +
                                          (digits[2] - '0');
                        uint16_t cm = (uint16_t)(inches * 254u / 100u);
                        GP_LOG_SYSTEM_PRINTF("[Sensor] %u in  (%u cm)", 0,
                                             (unsigned)inches, (unsigned)cm);
                    }
                    state = 0;
                    idx = 0;
                    break;
                default:
                    state = 0;
                    idx = 0;
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
    }
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

    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("=== QPG6200L - MaxSonar EZ Test ===", 0);
    GP_LOG_SYSTEM_PRINTF("Sensor UART: GPIO%d RX / GPIO%d TX @ %u baud", 0,
                         (int)SENSOR_UART_RX_GPIO,
                         (int)SENSOR_UART_TX_GPIO,
                         (unsigned)MAXSONAR_UART_BAUD);

    qDrvUart_PinConfig_t pinCfg =
        Q_DRV_UART_PIN_CONFIG(MAXSONAR_UART_INSTANCE,
                              SENSOR_UART_TX_GPIO, SENSOR_UART_RX_GPIO);
    res = qDrvUART_PinConfigSet(&pinCfg);
    GP_ASSERT_SYSTEM(res == Q_OK);

    qDrvUART_Config_t uartCfg = Q_DRV_UART_CONFIG_DEFAULT(MAXSONAR_UART_BAUD);
    uartCfg.txDma = false;
    uartCfg.rxDma = false;

    qDrvUART_Callbacks_t cbs = {NULL, NULL, NULL};
    res = qDrvUART_Init(&sUart, &uartCfg, &cbs, NULL, 5);
    GP_ASSERT_SYSTEM(res == Q_OK);

    res = qDrvUART_RxEnable(&sUart);
    GP_ASSERT_SYSTEM(res == Q_OK);

    TaskHandle_t handle = xTaskCreateStatic(
        SensorTask, "MaxSonar",
        SENSOR_TASK_STACK_SIZE, NULL,
        SENSOR_TASK_PRIORITY,
        sSensorStack, &sSensorTaskPCB);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, handle != NULL);
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
