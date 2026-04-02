/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * This software is owned by Qorvo Inc and protected under applicable copyright
 * laws. It is delivered under the terms of the license and is intended and
 * supplied for use solely and exclusively with products manufactured by
 * Qorvo Inc.
 *
 * THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO
 * THIS SOFTWARE. QORVO INC. SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR
 * SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */

/**
 * @file AppEvent.h
 * @brief Application event type definitions for the BLE IoT Demo.
 *
 * Defines all event types and their associated data structures that flow
 * through the FreeRTOS event queue. Events originate from BLE callbacks,
 * button presses, and the periodic heartbeat timer, and are dispatched
 * to AppManager for processing.
 */

#ifndef _APPEVENT_H_
#define _APPEVENT_H_

#include "ResetCount.h"
#include "AppButtons.h"
#include "BleIf.h"

/** @brief Actor identifiers (reserved for future multi-actor routing). */
typedef enum { kActor_App = 0, kActor_Invalid = 255 } Actor_t;

struct AppEvent;
typedef void (*EventHandler)(AppEvent*);

/**
 * @brief Unified application event structure.
 *
 * All inter-task communication in the application uses this structure.
 * The Type field selects which union member is valid.
 */
struct AppEvent {
    /**
     * @brief All possible event categories.
     *
     * - kEventType_ResetCount   : Reset counter event (optional build feature)
     * - kEventType_Buttons      : Physical button press/release/hold
     * - kEventType_BleConnection: BLE stack events (connect, advertise, GATT write)
     * - kEventType_Heartbeat    : Periodic 5-second heartbeat tick
     * - kEventType_Invalid      : Sentinel / uninitialized value
     */
    enum AppEventTypes {
        kEventType_ResetCount   = 0,
        kEventType_Buttons      = 1,
        kEventType_BleConnection = 2,
        kEventType_Heartbeat    = 3,
        kEventType_Invalid      = 255
    };

    AppEventTypes Type;

    union {
        /** Populated for kEventType_ResetCount */
        ResetCountEvent_t ResetCountEvent;

        /** Populated for kEventType_Buttons */
        ButtonEvent_t     ButtonEvent;

        /**
         * Populated for kEventType_BleConnection.
         * The Ble_Event_t struct carries both an Event enum (connection state
         * or characteristic update type) and an optional Value byte.
         */
        Ble_Event_t       BleConnectionEvent;
    };

    /** Optional direct handler; NULL means route through AppManager. */
    EventHandler Handler;
};

#endif // _APPEVENT_H_
