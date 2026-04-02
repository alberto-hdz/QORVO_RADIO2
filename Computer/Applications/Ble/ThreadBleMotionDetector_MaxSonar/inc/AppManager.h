/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "AppManager.h"
 *
 * Application manager for the QPG6200 Thread+BLE MaxSonar Motion Detector.
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

    static void NotifySensorEvent(bool motionDetected, uint16_t distanceCm);
    static void NotifyThreadEvent(ThreadEventType_t event, uint32_t value);

private:
    friend AppManager& GetAppMgr(void);

    void BleEventHandler(AppEvent* aEvent);
    void ButtonEventHandler(AppEvent* aEvent);
    void SensorEventHandler(AppEvent* aEvent);
    void ThreadEventHandler(AppEvent* aEvent);

    void ReportMotion(bool detected, uint16_t distanceCm, bool fromThread);

    static AppManager sAppMgr;
};

inline AppManager& GetAppMgr(void)
{
    return AppManager::sAppMgr;
}

#endif /* _APPMANAGER_H_ */
