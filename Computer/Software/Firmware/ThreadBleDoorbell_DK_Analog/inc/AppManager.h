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

/** @file "AppManager.h"
 *
 * Application manager for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Responsibilities:
 *   - BLE stack initialisation and event handling
 *   - Thread network commissioning (credentials stored in NVM)
 *   - Analog (GPADC) doorbell ring event handling
 *   - Thread mesh event handling (forward remote rings locally)
 *   - LED state machine
 */

#ifndef _APPMANAGER_H_
#define _APPMANAGER_H_

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"
#include "AppButtons.h"

class AppManager
{
public:
    void Init();
    void EventHandler(AppEvent* aEvent);

    /* Called by DoorbellManager polling task when ADC button state changes */
    static void NotifyAnalogEvent(bool pressed, uint16_t adcRaw);

    /* Called by Thread task when a network event occurs */
    static void NotifyThreadEvent(ThreadEventType_t event, uint32_t value);

private:
    friend AppManager& GetAppMgr(void);

    void BleEventHandler(AppEvent* aEvent);
    void ButtonEventHandler(AppEvent* aEvent);
    void AnalogEventHandler(AppEvent* aEvent);
    void ThreadEventHandler(AppEvent* aEvent);

    /* Ring the doorbell locally (LED + BLE notification + Thread multicast) */
    void RingDoorbell(bool fromThread, bool fromPhone);

    static AppManager sAppMgr;
};

inline AppManager& GetAppMgr(void)
{
    return AppManager::sAppMgr;
}

#endif /* _APPMANAGER_H_ */
