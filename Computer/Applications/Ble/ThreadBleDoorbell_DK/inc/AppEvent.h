/*
 * Copyright (c) 2024-2025, Qorvo Inc
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

/** @file "AppEvent.h"
 *
 * Application event types for the QPG6200 Thread+BLE Doorbell DK demo.
 *
 * Three event sources:
 *   - Buttons     : digital PB1 press/hold/release (commissioning)
 *   - BleConn     : BLE stack events (advertising, connect, characteristic writes)
 *   - Analog      : digital PB2 doorbell button (GPIO 5, active low, polled by DoorbellManager)
 *   - Thread      : OpenThread network events (joined, ring received, etc.)
 */

#ifndef _APPEVENT_H_
#define _APPEVENT_H_

#include "AppButtons.h"
#include "BleIf.h"

/* -------------------------------------------------------------------------
 * Doorbell button event (digital PB2 on DK)
 * ------------------------------------------------------------------------- */
typedef enum
{
    kAnalogEvent_Pressed  = 0,   /**< Button pressed (GPIO 5 read low) */
    kAnalogEvent_Released = 1,   /**< Button released (GPIO 5 read high) */
} AnalogEventType_t;

typedef struct
{
    AnalogEventType_t State;     /**< Press or release */
    uint16_t          AdcRaw;    /**< Unused in DK version (always 0) */
} AnalogEvent_t;

/* -------------------------------------------------------------------------
 * Thread network event
 * ------------------------------------------------------------------------- */
typedef enum
{
    kThreadEvent_Joined       = 0,  /**< Successfully attached to a Thread network */
    kThreadEvent_Detached     = 1,  /**< Left / lost the Thread network */
    kThreadEvent_RingReceived = 2,  /**< Remote doorbell ring arrived over Thread mesh */
    kThreadEvent_Error        = 3,  /**< Generic Thread stack error */
} ThreadEventType_t;

typedef struct
{
    ThreadEventType_t Event;
    uint32_t          Value;    /**< Event-specific data (e.g. error code) */
} ThreadEvent_t;

/* -------------------------------------------------------------------------
 * Unified AppEvent
 * ------------------------------------------------------------------------- */
typedef enum { kActor_App = 0, kActor_Invalid = 255 } Actor_t;

struct AppEvent;
typedef void (*EventHandler)(AppEvent*);

struct AppEvent
{
    enum AppEventTypes
    {
        kEventType_Buttons       = 0,
        kEventType_BleConnection = 1,
        kEventType_Analog        = 2,   /**< Digital doorbell button (PB2) */
        kEventType_Thread        = 3,   /**< OpenThread mesh event */
        kEventType_Invalid       = 255
    };

    AppEventTypes Type;

    union
    {
        /* Digital button event (PB1 commissioning button) */
        ButtonEvent_t ButtonEvent;

        /* BLE connection/advertising/characteristic event */
        Ble_Event_t BleConnectionEvent;

        /* Doorbell button event (PB2 digital button on DK) */
        AnalogEvent_t AnalogEvent;

        /* Thread network event */
        ThreadEvent_t ThreadEvent;
    };

    EventHandler Handler;
};

#endif /* _APPEVENT_H_ */
