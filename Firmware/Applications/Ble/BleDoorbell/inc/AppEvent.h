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
 * Application event types for the BLE Doorbell demo.
 */

#ifndef _APPEVENT_H_
#define _APPEVENT_H_

#include "AppButtons.h"
#include "BleIf.h"

typedef enum { kActor_App = 0, kActor_Invalid = 255 } Actor_t;

struct AppEvent;
typedef void (*EventHandler)(AppEvent*);

struct AppEvent {
    enum AppEventTypes {
        kEventType_Buttons      = 0,
        kEventType_BleConnection = 1,
        kEventType_Invalid      = 255
    };

    AppEventTypes Type;

    union {
        /* Event from Button Handling */
        ButtonEvent_t ButtonEvent;

        /* BLE events (connection, advertising, characteristic writes) */
        Ble_Event_t BleConnectionEvent;
    };

    EventHandler Handler;
};

#endif /* _APPEVENT_H_ */
