/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "SensorManager.h"
 *
 * HC-SR04 ultrasonic sensor manager for the QPG6200 Motion Detector.
 *
 * Hardware connection (QPG6200L DK):
 *   Sensor VCC  -> 5V
 *   Sensor GND  -> GND
 *   Sensor Trig -> GPIO 28 (ANIO0, used as digital output)
 *   Sensor Echo -> GPIO 29 (ANIO1, used as digital input)
 *
 * Operation:
 *   1. Send a 10 us HIGH pulse on the Trig pin.
 *   2. Sensor fires an 8-cycle 40 kHz burst and raises the Echo pin.
 *   3. Echo pin stays HIGH for a duration proportional to object distance.
 *   4. Distance (cm) = echo pulse width (us) / 58.
 *
 * Motion is declared when the measured distance falls at or below
 * MOTION_DISTANCE_THRESHOLD_CM (200 cm = 2 m).
 */

#ifndef _SENSORMANAGER_H_
#define _SENSORMANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus

#ifndef MOTION_DISTANCE_THRESHOLD_CM
#define MOTION_DISTANCE_THRESHOLD_CM  200u
#endif

#ifndef SENSOR_PERIOD_MS
#define SENSOR_PERIOD_MS              100u
#endif

class SensorManager
{
public:
    static bool Init(void);
    static void StartSensing(void);
    static bool IsMotionDetected(void);
    static uint16_t GetLastDistanceCm(void);

private:
    static void     SensorTask(void* pvParameters);
    static uint16_t MeasureDistance(void);
    static void     TriggerPulse(void);
    static void     ProcessDistance(uint16_t distanceCm);

    static bool     sMotionDetected;
    static uint16_t sLastDistanceCm;
};

#endif /* __cplusplus */

#endif /* _SENSORMANAGER_H_ */
