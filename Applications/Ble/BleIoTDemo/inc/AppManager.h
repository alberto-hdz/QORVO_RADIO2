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
 * @file AppManager.h
 * @brief Application manager singleton interface for the BLE IoT Demo.
 *
 * AppManager is the central coordinator of the application.  It initialises
 * the BLE stack, LEDs, buttons, and heartbeat timer during Init(), then
 * handles all events dispatched from AppTask via EventHandler().
 */

#ifndef _APPMANAGER_H_
#define _APPMANAGER_H_

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"
#include "AppButtons.h"

/**
 * @brief Central application manager.
 *
 * All application logic lives in this class.  It is a singleton accessed
 * through GetAppMgr().
 */
class AppManager {
   public:
    /** @brief Initialise BLE, LEDs, buttons, and the heartbeat timer. */
    void Init();

    /**
     * @brief Dispatch a dequeued event to the appropriate handler.
     * @param aEvent  Non-null pointer to the event to handle.
     */
    void EventHandler(AppEvent* aEvent);

   private:
    friend AppManager& GetAppMgr(void);

    /**
     * @brief Handle BLE stack events (connection, advertising, GATT writes).
     * @param aEvent  Event with Type == kEventType_BleConnection.
     */
    static void BleEventHandler(AppEvent* aEvent);

    /**
     * @brief Handle physical button press/release/hold events.
     *
     * Short press  : sends a BLE notification (button state).
     * Long press   : restarts BLE advertising.
     *
     * @param aEvent  Event with Type == kEventType_Buttons.
     */
    void ButtonEventHandler(AppEvent* aEvent);

    /**
     * @brief Handle the periodic heartbeat tick.
     *
     * Increments the heartbeat counter and sends a BLE notification if a
     * client is subscribed.
     *
     * @param aEvent  Event with Type == kEventType_Heartbeat.
     */
    static void HeartbeatEventHandler(AppEvent* aEvent);

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    void ResetCountEventHandler(AppEvent* aEvent);
#endif

    static AppManager sAppMgr;
};

/** @brief Global accessor for the AppManager singleton. */
inline AppManager& GetAppMgr(void)
{
    return AppManager::sAppMgr;
}

#endif // _APPMANAGER_H_
