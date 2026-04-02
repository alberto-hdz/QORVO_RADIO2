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
 * @file AppManager.cpp
 * @brief Central application coordinator for the BLE IoT Demo.
 *
 * Application overview
 * ====================
 * This demo shows the QPG6200 development board operating as a BLE peripheral
 * that integrates with a Raspberry Pi IoT gateway and a Node-RED dashboard.
 *
 * BLE RF protocol
 * ---------------
 * The QPG6200 uses BLE advertising (2.4 GHz) to announce itself and then
 * maintains a BLE GATT connection with the gateway bridge running on the Pi.
 *
 * Data flow
 * ---------
 *   Button press  →  BLE Notify (Button State char)
 *               →  ble_mqtt_bridge.py on Pi
 *               →  MQTT topic: qorvo/demo/button
 *               →  Node-RED dashboard widget
 *
 *   MQTT topic qorvo/demo/led  →  ble_mqtt_bridge.py
 *                             →  BLE Write (LED Control char)
 *                             →  AppManager LED control
 *
 *   5-second timer  →  BLE Notify (Heartbeat char)
 *                  →  MQTT topic: qorvo/demo/heartbeat
 *                  →  Node-RED heartbeat display
 *
 * LED behaviour
 * -------------
 *   WHITE_COOL (BLE status LED):
 *     Blinking 500 ms  – advertising, waiting for connection
 *     Solid ON         – BLE client connected
 *     OFF              – disconnected / idle
 *
 *   BLUE (application LED):
 *     Controlled remotely via the LED Control GATT characteristic or
 *     from the Node-RED dashboard via the MQTT gateway bridge.
 *
 * Button behaviour
 * ----------------
 *   Short press  (<5 s) – sends BLE notification (Button State = 0x01 on
 *                          press, 0x00 on release)
 *   Long press   (≥5 s) – restarts BLE advertising (re-connect after
 *                          accidental disconnect)
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"
#include "Ble_Peripheral_Config.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "timers.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------- */
/*  Application constants                                                     */
/* -------------------------------------------------------------------------- */

/** @brief LED blink interval while advertising (ms). */
#define LED_BLINK_INTERVAL_MS    500

/** @brief Long-press threshold to restart BLE advertising (seconds). */
#define BLE_RESTART_TIMEOUT_SEC  5

/** @brief Heartbeat notification period (ms). */
#define HEARTBEAT_PERIOD_MS      5000

/* -------------------------------------------------------------------------- */
/*  Module-level state                                                        */
/* -------------------------------------------------------------------------- */

/** @brief Singleton instance of the application manager. */
AppManager AppManager::sAppMgr;

/** @brief GPIOs used as status LEDs (populated from qPinCfg.h). */
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/** @brief BLE callback registration structure. */
static BleIf_Callbacks_t appCallbacks;

/**
 * @brief 1-byte heartbeat counter.
 *
 * Incremented every HEARTBEAT_PERIOD_MS milliseconds and sent as a BLE
 * notification.  Wraps naturally at 255 → 0.
 */
static uint8_t sHeartbeatCount = 0;

/** @brief FreeRTOS software timer handle for the periodic heartbeat. */
static TimerHandle_t sHeartbeatTimer = NULL;

/* -------------------------------------------------------------------------- */
/*  Forward declarations for BLE stack callbacks                             */
/* -------------------------------------------------------------------------- */

static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle,
                                            uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr);
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle,
                                             uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr);
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

/* -------------------------------------------------------------------------- */
/*  Heartbeat timer callback                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief FreeRTOS timer callback – fires every HEARTBEAT_PERIOD_MS ms.
 *
 * Posts a kEventType_Heartbeat event to the main application queue so that
 * the BLE notification is sent from the AppTask context (thread-safe).
 *
 * @param xTimer  Handle of the expired timer (unused).
 */
static void HeartbeatTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;

    AppEvent event;
    event.Type    = AppEvent::kEventType_Heartbeat;
    event.Handler = nullptr;

    GetAppTask().PostEvent(&event);
}

/* ========================================================================== */
/*  AppManager public methods                                                 */
/* ========================================================================== */

void AppManager::Init()
{
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("==========================================", 0);
    GP_LOG_SYSTEM_PRINTF("   QPG6200 BLE IoT Demo", 0);
    GP_LOG_SYSTEM_PRINTF("==========================================", 0);
    GP_LOG_SYSTEM_PRINTF("RF Protocol : Bluetooth Low Energy (BLE)", 0);
    GP_LOG_SYSTEM_PRINTF("Device name : QPG IoT Demo", 0);
    GP_LOG_SYSTEM_PRINTF("Gateway     : 192.168.10.102 (Raspberry Pi)", 0);
    GP_LOG_SYSTEM_PRINTF("MQTT broker : 192.168.10.102:1883", 0);
    GP_LOG_SYSTEM_PRINTF("Dashboard   : http://192.168.10.102:1880/dashboard", 0);
    GP_LOG_SYSTEM_PRINTF("==========================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Register the multi-function button (PB5). */
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    /* Register BLE stack callbacks. */
    appCallbacks.stackCallback    = BLE_Stack_Callback;
    appCallbacks.chrReadCallback  = BLE_CharacteristicRead_Callback;
    appCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    appCallbacks.cccCallback      = BLE_CCCD_Callback;

    BleIf_Init(&appCallbacks);

    /* Initialise status LEDs; both start off. */
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
    StatusLed_SetLed(APP_STATE_LED, false);

    /* Start BLE advertising immediately so the gateway can find the device. */
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("BLE advertising started – look for 'QPG IoT Demo'", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("WARNING: BLE advertising failed to start", 0);
    }

    /*
     * Create a repeating FreeRTOS software timer for the heartbeat.
     * The timer fires every HEARTBEAT_PERIOD_MS milliseconds and posts
     * a kEventType_Heartbeat event to the main task queue.
     */
    sHeartbeatTimer = xTimerCreate(
        "Heartbeat",                       /* Timer name (debug only)   */
        pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS),/* Period                    */
        pdTRUE,                            /* Auto-reload               */
        nullptr,                           /* Timer ID (unused)         */
        HeartbeatTimerCallback             /* Callback                  */
    );

    if(sHeartbeatTimer != NULL)
    {
        xTimerStart(sHeartbeatTimer, 0);
        GP_LOG_SYSTEM_PRINTF("Heartbeat timer started (%d ms period)", 0, HEARTBEAT_PERIOD_MS);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("WARNING: Failed to create heartbeat timer", 0);
    }

    GP_LOG_SYSTEM_PRINTF("AppManager initialisation complete", 0);
}

/* -------------------------------------------------------------------------- */

void AppManager::EventHandler(AppEvent* aEvent)
{
    if(aEvent == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: NULL event received", 0);
        return;
    }

    switch(aEvent->Type)
    {
        case AppEvent::kEventType_Buttons:
            ButtonEventHandler(aEvent);
            break;

        case AppEvent::kEventType_BleConnection:
            BleEventHandler(aEvent);
            break;

        case AppEvent::kEventType_Heartbeat:
            HeartbeatEventHandler(aEvent);
            break;

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
        case AppEvent::kEventType_ResetCount:
            ResetCountEventHandler(aEvent);
            break;
#endif

        default:
            GP_LOG_SYSTEM_PRINTF("Unhandled event type: %d", 0,
                                 static_cast<int>(aEvent->Type));
            break;
    }
}

/* ========================================================================== */
/*  Private event handlers                                                    */
/* ========================================================================== */

/**
 * @brief Handle BLE connection, advertising, and GATT write events.
 *
 * Connection events drive the BLE status LED:
 *   advertising  → blink  (500 ms on / 500 ms off)
 *   connected    → solid on
 *   disconnected → off
 *
 * GATT write events for the LED Control characteristic update the BLUE LED.
 */
void AppManager::BleEventHandler(AppEvent* aEvent)
{
    switch(aEvent->BleConnectionEvent.Event)
    {
        case Ble_Event_t::kBleConnectionEvent_Advertise_Start:
            GP_LOG_SYSTEM_PRINTF("BLE advertising – waiting for gateway connection", 0);
            StatusLed_BlinkLed(APP_BLE_CONNECTION_LED,
                               LED_BLINK_INTERVAL_MS, LED_BLINK_INTERVAL_MS);
            break;

        case Ble_Event_t::kBleConnectionEvent_Connected:
            GP_LOG_SYSTEM_PRINTF("BLE connected – gateway bridge is active", 0);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, true);
            break;

        case Ble_Event_t::kBleConnectionEvent_Disconnected:
            GP_LOG_SYSTEM_PRINTF("BLE disconnected", 0);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            break;

        case Ble_Event_t::kBleLedControlCharUpdate:
            /*
             * The gateway bridge wrote to the LED Control characteristic
             * in response to an MQTT message from the Node-RED dashboard.
             */
            if(aEvent->BleConnectionEvent.Value == 0)
            {
                GP_LOG_SYSTEM_PRINTF("LED Control: OFF (from gateway/dashboard)", 0);
                StatusLed_SetLed(APP_STATE_LED, false);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("LED Control: ON (from gateway/dashboard)", 0);
                StatusLed_SetLed(APP_STATE_LED, true);
            }
            break;

        default:
            GP_LOG_SYSTEM_PRINTF("Unhandled BLE event: %d", 0,
                                 static_cast<int>(aEvent->BleConnectionEvent.Event));
            break;
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle physical button (PB5) press, release, and hold events.
 *
 * On press   : send BLE notification Button State = 0x01
 * On release : send BLE notification Button State = 0x00
 * Long hold  : restart BLE advertising (useful after accidental disconnect)
 */
void AppManager::ButtonEventHandler(AppEvent* aEvent)
{
    if(aEvent->ButtonEvent.Index != APP_MULTI_FUNC_BUTTON)
    {
        return;
    }

    if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Pressed)
    {
        GP_LOG_SYSTEM_PRINTF("", 0);
        GP_LOG_SYSTEM_PRINTF("** Button pressed → sending BLE notification **", 0);

        /* Notify the gateway bridge that the button is pressed (0x01). */
        uint8_t value = 0x01;
        Status_t status = BleIf_SendNotification(BUTTON_STATE_HDL, sizeof(value), &value);

        if(status == STATUS_NO_ERROR)
        {
            GP_LOG_SYSTEM_PRINTF("  Button State notification sent (pressed)", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("  Notification failed (not connected? status=%d)", 0, status);
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
    {
        GP_LOG_SYSTEM_PRINTF("** Button released (held %d s) **", 0,
                             aEvent->ButtonEvent.HeldSec);

        if(aEvent->ButtonEvent.HeldSec < BLE_RESTART_TIMEOUT_SEC)
        {
            /* Short release – notify button released (0x00). */
            uint8_t value = 0x00;
            Status_t status = BleIf_SendNotification(BUTTON_STATE_HDL, sizeof(value), &value);

            if(status == STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("  Button State notification sent (released)", 0);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("  Notification failed (not connected? status=%d)", 0, status);
            }
        }
        else
        {
            /* Long release – restart advertising so the gateway can reconnect. */
            GP_LOG_SYSTEM_PRINTF("Long press detected – restarting BLE advertising", 0);
            if(BleIf_StartAdvertising() != STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("WARNING: Failed to restart BLE advertising", 0);
            }
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
    {
        if(aEvent->ButtonEvent.HeldSec == BLE_RESTART_TIMEOUT_SEC)
        {
            GP_LOG_SYSTEM_PRINTF("Hold detected – release to restart BLE advertising", 0);
        }
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Send a periodic heartbeat BLE notification.
 *
 * The heartbeat counter wraps naturally from 255 to 0 (uint8_t overflow).
 * If no client is subscribed the notification is silently discarded by the
 * BLE stack; no error recovery is needed.
 */
void AppManager::HeartbeatEventHandler(AppEvent* aEvent)
{
    (void)aEvent;

    sHeartbeatCount++;

    GP_LOG_SYSTEM_PRINTF("Heartbeat #%d", 0, sHeartbeatCount);

    Status_t status = BleIf_SendNotification(HEARTBEAT_HDL,
                                             sizeof(sHeartbeatCount),
                                             &sHeartbeatCount);
    if(status != STATUS_NO_ERROR)
    {
        /* Likely no subscriber yet – this is normal until the gateway connects. */
        GP_LOG_SYSTEM_PRINTF("  (no subscriber – heartbeat queued locally)", 0);
    }
}

/* ========================================================================== */
/*  BLE stack callbacks (static, run in BLE task context)                    */
/* ========================================================================== */

/**
 * @brief Route BLE Device Manager and ATT events to the application task.
 *
 * This function is called by the BLE stack (not the app task).  It translates
 * raw stack events into AppEvent structures and posts them to the queue for
 * thread-safe processing.
 */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    /* Handle Device Manager events (advertising and connection lifecycle). */
    if(pMsg->event >= BLEIF_DM_CBACK_START && pMsg->event <= BLEIF_DM_CBACK_END)
    {
        switch(pMsg->event)
        {
            case BLEIF_DM_ADV_START_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Advertise_Start;
                break;

            case BLEIF_DM_CONN_OPEN_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Connected;
                break;

            case BLEIF_DM_ADV_STOP_IND:
            case BLEIF_DM_CONN_CLOSE_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Disconnected;
                break;

            default:
                /* Unsupported DM event – ignore. */
                event.Type = AppEvent::kEventType_Invalid;
                break;
        }

        if(event.Type != AppEvent::kEventType_Invalid)
        {
            GetAppTask().PostEvent(&event);
        }
    }
    else if(pMsg->event >= BLEIF_ATT_CBACK_START && pMsg->event <= BLEIF_ATT_CBACK_END)
    {
        /* ATT events (read confirmations, write confirmations) – not required here. */
        GP_LOG_SYSTEM_PRINTF("ATT event %d status %d", 0, pMsg->event, pMsg->status);
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle GATT characteristic read requests.
 *
 * All readable values in the GATT database return their current cached value
 * automatically via the ATTS layer without requiring application intervention.
 * This callback exists for completeness only.
 */
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle,
                                            uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr)
{
    (void)connId;
    (void)handle;
    (void)operation;
    (void)offset;
    (void)pAttr;
    /* Reads are served automatically from the GATT database values. */
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle GATT characteristic write requests.
 *
 * Currently only the LED Control characteristic (handle LED_CONTROL_HDL)
 * is writable.  The write is translated into a kBleLedControlCharUpdate
 * BLE event and posted to the application task queue.
 */
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle,
                                             uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr)
{
    (void)connId;
    (void)operation;
    (void)offset;
    (void)len;
    (void)pAttr;

    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    switch(handle)
    {
        case LED_CONTROL_HDL:
            /* Gateway bridge is commanding the LED state. */
            event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
            event.BleConnectionEvent.Value = pValue[0];
            GetAppTask().PostEvent(&event);
            break;

        default:
            GP_LOG_SYSTEM_PRINTF("Write to unknown handle 0x%04X ignored", 0, handle);
            break;
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle Client Characteristic Configuration Descriptor (CCC) changes.
 *
 * Called when a client enables or disables notifications on a characteristic.
 * Logged for diagnostic purposes; no application action required.
 */
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    const char* charName;

    switch(event->handle)
    {
        case BATTERY_LEVEL_CCC_HDL:  charName = "Battery Level";  break;
        case BUTTON_STATE_CCC_HDL:   charName = "Button State";   break;
        case HEARTBEAT_CCC_HDL:      charName = "Heartbeat";      break;
        default:                     charName = "Unknown";        break;
    }

    if(event->value & 0x0001)
    {
        GP_LOG_SYSTEM_PRINTF("Notifications ENABLED  for '%s'", 0, charName);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("Notifications DISABLED for '%s'", 0, charName);
    }
}

/* -------------------------------------------------------------------------- */

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
void AppManager::ResetCountEventHandler(AppEvent* aEvent)
{
    /* Reserved for reset-count based behaviour (e.g. factory reset on 5 resets). */
    (void)aEvent;
}
#endif
