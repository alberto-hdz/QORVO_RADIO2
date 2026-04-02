/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "SensorManager.cpp"
 *
 * LV-MaxSonar EZ sensor manager implementation.
 *
 * Hardware:
 *   Sensor TX -> GPIO 8 (UART1 RX) receives ASCII serial data
 *   Sensor RX -> GPIO 9 (UART1 TX) held HIGH to enable free-run continuous ranging
 *
 * Serial protocol: 9600 baud, 8N1
 * Frame format   : 'R' + 3 ASCII decimal digits + CR  (e.g. "R079\r" = 79 inches)
 *
 * Distance conversion: 1 inch = 2.54 cm  (integer: inches x 254 / 100)
 * Motion threshold   : distance <= 200 cm
 */

#include "SensorManager.h"
#include "AppManager.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "qDrvUART.h"
#include "qDrvIOB.h"

#include "FreeRTOS.h"
#include "task.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define MAXSONAR_UART_INSTANCE  1
#define MAXSONAR_UART_BAUD      9600u

#define SENSOR_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

static qDrvUART_t sUartInstance = Q_DRV_UART_INSTANCE_DEFINE(MAXSONAR_UART_INSTANCE);

bool     SensorManager::sMotionDetected = false;
uint16_t SensorManager::sLastDistanceCm = 0xFFFFu;
uint8_t  SensorManager::sParseState     = 0;
uint8_t  SensorManager::sFrameBuf[3]    = {0};
uint8_t  SensorManager::sFrameIdx       = 0;

static StaticTask_t sSensorTask;
static StackType_t  sSensorStack[SENSOR_TASK_STACK_SIZE];

bool SensorManager::Init(void)
{
    qDrvUart_PinConfig_t pinConfig =
        Q_DRV_UART_PIN_CONFIG(MAXSONAR_UART_INSTANCE, SENSOR_UART_TX_GPIO, SENSOR_UART_RX_GPIO);

    qResult_t res = qDrvUART_PinConfigSet(&pinConfig);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[Sensor] UART pin config failed: %d", 0, (int)res);
        return false;
    }

    qDrvUART_Config_t uartConfig = Q_DRV_UART_CONFIG_DEFAULT(MAXSONAR_UART_BAUD);
    uartConfig.txDma = false;
    uartConfig.rxDma = false;

    qDrvUART_Callbacks_t callbacks = {nullptr, nullptr, nullptr};
    res = qDrvUART_Init(&sUartInstance, &uartConfig, &callbacks, nullptr, 5);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[Sensor] UART init failed: %d", 0, (int)res);
        return false;
    }

    res = qDrvUART_RxEnable(&sUartInstance);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[Sensor] UART RX enable failed: %d", 0, (int)res);
        return false;
    }

    GP_LOG_SYSTEM_PRINTF("[Sensor] MaxSonar UART ready (GPIO%d RX, GPIO%d TX, %u baud)", 0,
                         (int)SENSOR_UART_RX_GPIO, (int)SENSOR_UART_TX_GPIO,
                         (unsigned)MAXSONAR_UART_BAUD);
    return true;
}

void SensorManager::StartSensing(void)
{
    TaskHandle_t handle =
        xTaskCreateStatic(SensorTask, "MaxSonar", SENSOR_TASK_STACK_SIZE,
                          nullptr, SENSOR_TASK_PRIORITY, sSensorStack, &sSensorTask);
    Q_ASSERT(handle != nullptr);
}

bool SensorManager::IsMotionDetected(void)
{
    return sMotionDetected;
}

uint16_t SensorManager::GetLastDistanceCm(void)
{
    return sLastDistanceCm;
}

void SensorManager::SensorTask(void* /*pvParameters*/)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    GP_LOG_SYSTEM_PRINTF("[Sensor] Polling UART for MaxSonar data...", 0);

    while(true)
    {
        UInt8 byte;
        while(qDrvUART_RxNewDataCheck(&sUartInstance))
        {
            if(qDrvUART_Rx(&sUartInstance, &byte) == Q_OK)
            {
                ParseByte(byte);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
    }

    vTaskDelete(nullptr);
}

void SensorManager::ParseByte(uint8_t byte)
{
    switch(sParseState)
    {
        case 0:
            if(byte == 'R')
            {
                sParseState = 1;
                sFrameIdx   = 0;
            }
            break;

        case 1:
            if(byte >= '0' && byte <= '9')
            {
                sFrameBuf[sFrameIdx++] = byte;
                if(sFrameIdx >= 3)
                {
                    sParseState = 2;
                }
            }
            else
            {
                sParseState = 0;
                sFrameIdx   = 0;
            }
            break;

        case 2:
            if(byte == '\r')
            {
                uint16_t inches = (uint16_t)((sFrameBuf[0] - '0') * 100u +
                                             (sFrameBuf[1] - '0') * 10u +
                                             (sFrameBuf[2] - '0'));
                uint16_t cm = (uint16_t)(inches * 254u / 100u);
                ProcessDistance(cm);
            }
            sParseState = 0;
            sFrameIdx   = 0;
            break;

        default:
            sParseState = 0;
            sFrameIdx   = 0;
            break;
    }
}

void SensorManager::ProcessDistance(uint16_t distanceCm)
{
    sLastDistanceCm = distanceCm;
    bool detected   = (distanceCm > 0 && distanceCm <= MOTION_DISTANCE_THRESHOLD_CM);

    if(detected != sMotionDetected)
    {
        sMotionDetected = detected;
        AppManager::NotifySensorEvent(detected, distanceCm);
    }
}
