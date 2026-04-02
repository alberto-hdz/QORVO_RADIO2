/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "AppEvent.h"
 *
 * Application event types for the QPG6200 Thread+BLE MaxSonar Motion Detector.
 *
 * Event sources:
 *   - Buttons   : PB1 commissioning button press/hold/release
 *   - BleConn   : BLE stack events (advertising, connect, characteristic writes)
 *   - Sensor    : Sensor readings (motion detected/cleared + distance)
 *   - Thread    : OpenThread network events
 */

#ifndef _APPEVENT_H_
#define _APPEVENT_H_

#include "AppButtons.h"
#include "BleIf.h"

typedef enum
{
    kSensorEvent_MotionDetected = 0,
    kSensorEvent_MotionCleared  = 1,
} SensorEventType_t;

typedef struct
{
    SensorEventType_t State;
    uint16_t          DistanceCm;
} SensorEvent_t;

typedef enum
{
    kThreadEvent_Joined         = 0,
    kThreadEvent_Detached       = 1,
    kThreadEvent_MotionReceived = 2,
    kThreadEvent_Error          = 3,
} ThreadEventType_t;

typedef struct
{
    ThreadEventType_t Event;
    uint32_t          Value;
} ThreadEvent_t;

typedef enum { kActor_App = 0, kActor_Invalid = 255 } Actor_t;

struct AppEvent;
typedef void (*EventHandler)(AppEvent*);

struct AppEvent
{
    enum AppEventTypes
    {
        kEventType_Buttons       = 0,
        kEventType_BleConnection = 1,
        kEventType_Sensor        = 2,
        kEventType_Thread        = 3,
        kEventType_Invalid       = 255
    };

    AppEventTypes Type;

    union
    {
        ButtonEvent_t    ButtonEvent;
        Ble_Event_t      BleConnectionEvent;
        SensorEvent_t    SensorEvent;
        ThreadEvent_t    ThreadEvent;
    };

    EventHandler Handler;
};

#endif /* _APPEVENT_H_ */
