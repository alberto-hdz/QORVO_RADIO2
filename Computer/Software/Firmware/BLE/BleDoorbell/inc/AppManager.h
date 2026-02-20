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
 * Application manager for the BLE Doorbell demo.
 */

#ifndef _APPMANAGER_H_
#define _APPMANAGER_H_

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"
#include "AppButtons.h"

class AppManager {
    public:
    void Init();
    void EventHandler(AppEvent* aEvent);

    private:
    friend AppManager& GetAppMgr(void);
    void BleEventHandler(AppEvent* aEvent);
    void ButtonEventHandler(AppEvent* aEvent);

    static AppManager sAppMgr;
};

inline AppManager& GetAppMgr(void)
{
    return AppManager::sAppMgr;
}

#endif /* _APPMANAGER_H_ */
