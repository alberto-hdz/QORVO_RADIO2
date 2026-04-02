/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "SensorManager.h"
 *
 * LV-MaxSonar EZ series sensor manager for the QPG6200 Motion Detector.
 *
 * Hardware connection (QPG6200L DK):
 *   Sensor GND -> Board GND
 *   Sensor +5  -> Board 5V
 *   Sensor TX  -> GPIO 8  (UART1 RX - receives serial data from sensor)
 *   Sensor RX  -> GPIO 9  (UART1 TX - hold HIGH to enable continuous ranging)
 *
 * The sensor outputs ASCII serial data at 9600 baud, 8N1.
 * Frame format: 'R' + 3 ASCII decimal digits + CR  (e.g. "R079\r" = 79 inches)
 * Range is reported in inches; this driver converts to centimetres.
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

#ifndef SENSOR_POLL_MS
#define SENSOR_POLL_MS                50u
#endif

class SensorManager
{
public:
    static bool Init(void);
    static void StartSensing(void);
    static bool IsMotionDetected(void);
    static uint16_t GetLastDistanceCm(void);

private:
    static void SensorTask(void* pvParameters);
    static void ParseByte(uint8_t byte);
    static void ProcessDistance(uint16_t distanceCm);

    static bool     sMotionDetected;
    static uint16_t sLastDistanceCm;

    /* UART frame parser state */
    static uint8_t  sParseState;
    static uint8_t  sFrameBuf[3];
    static uint8_t  sFrameIdx;
};

#endif /* __cplusplus */

#endif /* _SENSORMANAGER_H_ */
