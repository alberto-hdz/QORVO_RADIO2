/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "SensorManager.cpp"
 *
 * HC-SR04 ultrasonic sensor manager implementation.
 *
 * Hardware:
 *   Trig (GPIO 28) - digital output, send 10 us HIGH pulse to trigger measurement
 *   Echo (GPIO 29) - digital input,  pulse width (us) proportional to distance
 *
 * Distance (cm) = echo pulse width (us) / 58
 * Maximum range  ~= 400 cm (echo pulse ~23 ms)
 * Minimum range  ~= 2 cm
 * Motion threshold: <= 200 cm
 */

#include "SensorManager.h"
#include "AppManager.h"
#include "gpLog.h"
#include "gpSched.h"
#include "qPinCfg.h"
#include "qDrvGPIO.h"
#include "qDrvIOB.h"

#include "FreeRTOS.h"
#include "task.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define TRIG_PULSE_US        10u      /* 10 us trigger pulse */
#define ECHO_TIMEOUT_US      25000u   /* timeout waiting for echo (~4 m max) */
#define US_PER_CM            58u      /* us per centimetre (sound round-trip) */

#define SENSOR_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

bool     SensorManager::sMotionDetected = false;
uint16_t SensorManager::sLastDistanceCm = 0xFFFFu;

static StaticTask_t sSensorTask;
static StackType_t  sSensorStack[SENSOR_TASK_STACK_SIZE];

bool SensorManager::Init(void)
{
    /* Trig: output, start LOW */
    qDrvIOB_ConfigOutputSet(SENSOR_TRIG_GPIO, qDrvIOB_Drive2mA, qDrvIOB_SlewRateSlow);
    qDrvGPIO_Write(SENSOR_TRIG_GPIO, 0);

    /* Echo: input, no pull resistor */
    qDrvIOB_ConfigInputSet(SENSOR_ECHO_GPIO, qDrvIOB_PullNone, false);

    GP_LOG_SYSTEM_PRINTF("[Sensor] HC-SR04 ready (Trig=GPIO%d, Echo=GPIO%d)", 0,
                         (int)SENSOR_TRIG_GPIO, (int)SENSOR_ECHO_GPIO);
    return true;
}

void SensorManager::StartSensing(void)
{
    TaskHandle_t handle =
        xTaskCreateStatic(SensorTask, "HCSR04", SENSOR_TASK_STACK_SIZE,
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

void SensorManager::TriggerPulse(void)
{
    qDrvGPIO_Write(SENSOR_TRIG_GPIO, 1);
    uint32_t start = gpSched_GetCurrentTime();
    while((gpSched_GetCurrentTime() - start) < TRIG_PULSE_US) {}
    qDrvGPIO_Write(SENSOR_TRIG_GPIO, 0);
}

uint16_t SensorManager::MeasureDistance(void)
{
    TriggerPulse();

    /* Wait for Echo to go HIGH */
    uint32_t deadline = gpSched_GetCurrentTime() + ECHO_TIMEOUT_US;
    while(!qDrvGPIO_Read(SENSOR_ECHO_GPIO))
    {
        if(gpSched_GetCurrentTime() >= deadline)
        {
            return 0xFFFFu;  /* no echo received */
        }
    }

    uint32_t echoStart = gpSched_GetCurrentTime();

    /* Wait for Echo to go LOW */
    deadline = echoStart + ECHO_TIMEOUT_US;
    while(qDrvGPIO_Read(SENSOR_ECHO_GPIO))
    {
        if(gpSched_GetCurrentTime() >= deadline)
        {
            return 0xFFFFu;  /* echo pulse too long (out of range) */
        }
    }

    uint32_t pulseUs = gpSched_GetCurrentTime() - echoStart;
    return (uint16_t)(pulseUs / US_PER_CM);
}

void SensorManager::ProcessDistance(uint16_t distanceCm)
{
    sLastDistanceCm = distanceCm;
    bool detected   = (distanceCm > 0 && distanceCm != 0xFFFFu &&
                       distanceCm <= MOTION_DISTANCE_THRESHOLD_CM);

    if(detected != sMotionDetected)
    {
        sMotionDetected = detected;
        AppManager::NotifySensorEvent(detected, distanceCm);
    }
}

void SensorManager::SensorTask(void* /*pvParameters*/)
{
    /* HC-SR04 requires at least 60 ms between measurements */
    GP_LOG_SYSTEM_PRINTF("[Sensor] HC-SR04 task started", 0);

    while(true)
    {
        uint16_t distance = MeasureDistance();

        if(distance == 0xFFFFu)
        {
            GP_LOG_SYSTEM_PRINTF("[Sensor] No echo / out of range", 0);
            ProcessDistance(0xFFFFu);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[Sensor] Distance: %u cm", 0, (unsigned)distance);
            ProcessDistance(distance);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }

    vTaskDelete(nullptr);
}
