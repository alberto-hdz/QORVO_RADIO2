/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * @file AppEvent.h
 * @brief Application event types for the BLE Gateway Doorbell demo.
 *
 * All inter-task communication uses AppEvent.  Events are produced by:
 *   - BLE stack callbacks (advertising, connect, disconnect, GATT writes)
 *   - Physical button handler (press, release, hold)
 *   - Periodic heartbeat software timer
 */
#ifndef _APPEVENT_H_
#define _APPEVENT_H_

#include "ResetCount.h"
#include "AppButtons.h"
#include "BleIf.h"

typedef enum { kActor_App = 0, kActor_Invalid = 255 } Actor_t;

struct AppEvent;
typedef void (*EventHandler)(AppEvent*);

struct AppEvent {
    enum AppEventTypes {
        kEventType_ResetCount    = 0,
        kEventType_Buttons       = 1,
        kEventType_BleConnection = 2,
        kEventType_Heartbeat     = 3,
        kEventType_Invalid       = 255
    };

    AppEventTypes Type;

    union {
        ResetCountEvent_t ResetCountEvent; /**< kEventType_ResetCount    */
        ButtonEvent_t     ButtonEvent;     /**< kEventType_Buttons       */
        Ble_Event_t       BleConnectionEvent; /**< kEventType_BleConnection */
    };

    EventHandler Handler;
};

#endif /* _APPEVENT_H_ */
