/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * @file AppManager.h
 * @brief Application manager interface for the BLE Gateway Doorbell demo.
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
    void HeartbeatEventHandler(AppEvent* aEvent);
#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
    void ResetCountEventHandler(AppEvent* aEvent);
#endif
    static AppManager sAppMgr;
};

inline AppManager& GetAppMgr(void) { return AppManager::sAppMgr; }

#endif /* _APPMANAGER_H_ */
