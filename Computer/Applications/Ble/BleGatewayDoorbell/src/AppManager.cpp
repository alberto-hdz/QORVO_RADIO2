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
 * @brief Central application logic for the BLE Gateway Doorbell demo.
 *
 * Application overview
 * ====================
 * The QPG6200 acts as a BLE peripheral (GATT server) that announces itself
 * as "QPG Doorbell GW".  A Python gateway bridge running on the Raspberry Pi
 * connects as the BLE central, subscribes to notifications, and bridges all
 * events to the MQTT broker so that the Node-RED dashboard can display them.
 *
 * RF protocol
 * -----------
 * Bluetooth Low Energy (BLE) 5.0 is used for all wireless communication.
 * The QPG6200 uses 2.4 GHz BLE advertising to announce itself and then
 * maintains a GATT connection with the gateway bridge on the Pi.
 *
 * Data flow
 * ---------
 *   Button press  → BLE Notify (Doorbell Ring char, 0x01)
 *                 → ble_mqtt_bridge.py
 *                 → MQTT qorvo/demo/button  {"state":"pressed"}
 *                 → Node-RED Doorbell widget animates + plays chime audio
 *
 *   Button release → BLE Notify (Doorbell Ring char, 0x00)
 *                  → MQTT qorvo/demo/button  {"state":"released"}
 *
 *   5-second timer → BLE Notify (Heartbeat char, incrementing counter)
 *                  → MQTT qorvo/demo/heartbeat  {"count": N}
 *                  → Node-RED Movement Detector widget shows device is alive
 *
 *   MQTT qorvo/demo/led {"led":"on"/"off"}
 *                  → ble_mqtt_bridge.py
 *                  → BLE Write (LED Control char, 0x01 or 0x00)
 *                  → AppManager toggles the BLUE LED
 *
 * LED behaviour
 * -------------
 *   WHITE_COOL (BLE status):
 *     500 ms blink – advertising, waiting for the gateway to connect
 *     Solid ON     – gateway bridge is connected
 *     OFF          – disconnected / idle
 *
 *   BLUE (application LED):
 *     Controlled remotely via the LED Control GATT characteristic or
 *     the Node-RED Lighting System switch via MQTT.
 *
 * Button behaviour (PB5)
 * ----------------------
 *   Short press  (< 5 s) – rings the doorbell:
 *                           sends BLE notification 0x01 on press,
 *                           0x00 on release
 *   Long press   (≥ 5 s) – restarts BLE advertising so the gateway can
 *                           reconnect after an accidental disconnect
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

/** @brief BLE status LED blink interval while advertising (ms). */
#define LED_BLINK_INTERVAL_MS   500

/** @brief Button hold threshold to restart BLE advertising (seconds). */
#define BLE_RESTART_TIMEOUT_SEC 5

/** @brief Heartbeat notification period (ms). */
#define HEARTBEAT_PERIOD_MS     5000

/* -------------------------------------------------------------------------- */
/*  Module-level state                                                        */
/* -------------------------------------------------------------------------- */

/** @brief Singleton instance of the application manager. */
AppManager AppManager::sAppMgr;

/** @brief GPIO array for StatusLed – populated from qPinCfg.h. */
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/** @brief BLE stack callback registration structure. */
static BleIf_Callbacks_t appCallbacks;

/**
 * @brief 1-byte heartbeat counter sent periodically over BLE.
 *
 * Incremented every HEARTBEAT_PERIOD_MS milliseconds.  Wraps naturally
 * at 255 → 0 due to uint8_t overflow.
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

/* ==========================================================================
 *  AppManager public methods
 * ========================================================================== */

void AppManager::Init()
{
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("==============================================", 0);
    GP_LOG_SYSTEM_PRINTF("   QPG6200 BLE GATEWAY DOORBELL DEMO", 0);
    GP_LOG_SYSTEM_PRINTF("==============================================", 0);
    GP_LOG_SYSTEM_PRINTF("RF Protocol : Bluetooth Low Energy (BLE) 5.0", 0);
    GP_LOG_SYSTEM_PRINTF("Device name : QPG Doorbell GW", 0);
    GP_LOG_SYSTEM_PRINTF("Gateway     : 192.168.10.102 (Raspberry Pi)", 0);
    GP_LOG_SYSTEM_PRINTF("MQTT broker : 192.168.10.102:1883", 0);
    GP_LOG_SYSTEM_PRINTF("Dashboard   : http://192.168.10.102:1880/dashboard", 0);
    GP_LOG_SYSTEM_PRINTF("==============================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE blinks (500ms) = advertising", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE solid          = gateway connected", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE                 = controlled by dashboard", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Button Guide (PB5) ---", 0);
    GP_LOG_SYSTEM_PRINTF("  Short press  = ring the doorbell (BLE notify)", 0);
    GP_LOG_SYSTEM_PRINTF("  Hold >= 5s   = restart BLE advertising", 0);
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
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started – scan for 'QPG Doorbell GW'", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] WARNING: advertising failed to start", 0);
    }

    /*
     * Create a repeating FreeRTOS software timer for the periodic heartbeat.
     * The callback posts a kEventType_Heartbeat event every HEARTBEAT_PERIOD_MS ms.
     */
    sHeartbeatTimer = xTimerCreate(
        "DoorbellHB",                      /* Debug name                */
        pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS),/* Period                    */
        pdTRUE,                            /* Auto-reload               */
        nullptr,                           /* Timer ID (unused)         */
        HeartbeatTimerCallback             /* Callback function         */
    );

    if(sHeartbeatTimer != NULL)
    {
        xTimerStart(sHeartbeatTimer, 0);
        GP_LOG_SYSTEM_PRINTF("[HB] Heartbeat timer started (%d ms)", 0, HEARTBEAT_PERIOD_MS);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[HB] WARNING: failed to create heartbeat timer", 0);
    }

    GP_LOG_SYSTEM_PRINTF("AppManager init complete", 0);
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

/* ==========================================================================
 *  Private event handlers
 * ========================================================================== */

/**
 * @brief Handle BLE advertising, connection, and GATT write events.
 *
 * Advertising → WHITE LED blinks at 500 ms
 * Connected   → WHITE LED solid on
 * Disconnected→ WHITE LED off
 * LED Control write → update BLUE LED state
 */
void AppManager::BleEventHandler(AppEvent* aEvent)
{
    switch(aEvent->BleConnectionEvent.Event)
    {
        case Ble_Event_t::kBleConnectionEvent_Advertise_Start:
            GP_LOG_SYSTEM_PRINTF("[BLE] Advertising – waiting for gateway bridge", 0);
            StatusLed_BlinkLed(APP_BLE_CONNECTION_LED,
                               LED_BLINK_INTERVAL_MS, LED_BLINK_INTERVAL_MS);
            break;

        case Ble_Event_t::kBleConnectionEvent_Connected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Gateway bridge connected – WHITE LED solid", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Press PB5 to send a doorbell ring", 0);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, true);
            break;

        case Ble_Event_t::kBleConnectionEvent_Disconnected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Gateway disconnected – WHITE LED off", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Hold PB5 for 5s to restart advertising", 0);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            break;

        case Ble_Event_t::kBleLedControlCharUpdate:
            /*
             * The gateway bridge wrote to the LED Control characteristic
             * in response to a Node-RED dashboard command over MQTT.
             */
            if(aEvent->BleConnectionEvent.Value == 0x00)
            {
                GP_LOG_SYSTEM_PRINTF("[LED] BLUE LED OFF (dashboard command)", 0);
                StatusLed_SetLed(APP_STATE_LED, false);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[LED] BLUE LED ON (dashboard command)", 0);
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
 * On press   : send BLE notification Doorbell Ring = 0x01 (ringing)
 * On release : send BLE notification Doorbell Ring = 0x00 (idle)
 * Long hold  : restart BLE advertising (reconnect after accidental disconnect)
 *
 * The gateway bridge translates these notifications into:
 *   Press   → MQTT qorvo/demo/button  {"state":"pressed"}
 *   Release → MQTT qorvo/demo/button  {"state":"released"}
 *
 * Node-RED's "Button → Doorbell" function only acts on "pressed" to trigger
 * the doorbell animation and chime audio on the dashboard.
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
        GP_LOG_SYSTEM_PRINTF("** DING DONG! Doorbell button pressed **", 0);

        /* Notify the gateway that the doorbell is ringing (0x01). */
        uint8_t ringValue = DOORBELL_STATE_RINGING;
        Status_t status = BleIf_SendNotification(DOORBELL_RING_HDL,
                                                 sizeof(ringValue), &ringValue);
        if(status == STATUS_NO_ERROR)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell Ring notification sent (0x01 – ringing)", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Notification failed – gateway not connected (status=%d)", 0, status);
            GP_LOG_SYSTEM_PRINTF("[BLE] Start ble_mqtt_bridge.py on the Raspberry Pi", 0);
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
    {
        GP_LOG_SYSTEM_PRINTF("** Button released (held %d s) **", 0,
                             aEvent->ButtonEvent.HeldSec);

        if(aEvent->ButtonEvent.HeldSec < BLE_RESTART_TIMEOUT_SEC)
        {
            /* Short release – notify doorbell is idle again (0x00). */
            uint8_t idleValue = DOORBELL_STATE_IDLE;
            Status_t status = BleIf_SendNotification(DOORBELL_RING_HDL,
                                                     sizeof(idleValue), &idleValue);
            if(status == STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell Ring notification sent (0x00 – idle)", 0);
            }
        }
        else
        {
            /* Long release – restart advertising so the gateway can reconnect. */
            GP_LOG_SYSTEM_PRINTF("[BLE] Long press detected – restarting advertising", 0);
            if(BleIf_StartAdvertising() != STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] WARNING: failed to restart advertising", 0);
            }
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
    {
        /* Inform user when the hold threshold is reached. */
        if(aEvent->ButtonEvent.HeldSec == BLE_RESTART_TIMEOUT_SEC)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Hold threshold reached – release to restart advertising", 0);
        }
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Send a periodic heartbeat BLE notification.
 *
 * Increments the 1-byte counter (wraps 255 → 0) and notifies the gateway.
 * The gateway publishes {"count": N} to MQTT topic qorvo/demo/heartbeat,
 * which the Node-RED Movement Detector widget uses to confirm the board is alive.
 *
 * If no client is subscribed, the BLE stack discards the notification
 * silently – no error recovery is needed.
 */
void AppManager::HeartbeatEventHandler(AppEvent* aEvent)
{
    (void)aEvent;

    sHeartbeatCount++;

    GP_LOG_SYSTEM_PRINTF("[HB] Heartbeat #%d", 0, sHeartbeatCount);

    Status_t status = BleIf_SendNotification(HEARTBEAT_HDL,
                                             sizeof(sHeartbeatCount),
                                             &sHeartbeatCount);
    if(status != STATUS_NO_ERROR)
    {
        /* Expected until the gateway connects and enables notifications. */
        GP_LOG_SYSTEM_PRINTF("[HB]  (no subscriber – heartbeat not forwarded)", 0);
    }
}

/* ==========================================================================
 *  BLE stack callbacks (static, called from BLE task context)
 * ========================================================================== */

/**
 * @brief Translate raw BLE stack events into AppEvent structures.
 *
 * Called by the BLE stack – not the application task.  Posts events to the
 * FreeRTOS queue so they are processed in the AppTask context.
 */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    /* Handle Device Manager events (advertising/connection lifecycle). */
    if(pMsg->event >= BLEIF_DM_CBACK_START && pMsg->event <= BLEIF_DM_CBACK_END)
    {
        switch(pMsg->event)
        {
            case BLEIF_DM_ADV_START_IND:
                event.BleConnectionEvent.Event =
                    Ble_Event_t::kBleConnectionEvent_Advertise_Start;
                break;

            case BLEIF_DM_CONN_OPEN_IND:
                event.BleConnectionEvent.Event =
                    Ble_Event_t::kBleConnectionEvent_Connected;
                break;

            case BLEIF_DM_ADV_STOP_IND:
            case BLEIF_DM_CONN_CLOSE_IND:
                event.BleConnectionEvent.Event =
                    Ble_Event_t::kBleConnectionEvent_Disconnected;
                break;

            default:
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
        /* ATT confirmations – not required for this application. */
        GP_LOG_SYSTEM_PRINTF("[BLE] ATT event %d (status %d)", 0,
                             pMsg->event, pMsg->status);
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle GATT characteristic read requests.
 *
 * The ATTS layer serves all readable values automatically from the GATT
 * database.  This callback exists for completeness and does nothing.
 */
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle,
                                            uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr)
{
    (void)connId; (void)handle; (void)operation; (void)offset; (void)pAttr;
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle GATT characteristic write requests.
 *
 * Only the LED Control characteristic (LED_CONTROL_HDL) is writable.
 * The write value is forwarded to AppManager via the event queue so it
 * is processed in the AppTask context.
 */
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle,
                                             uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr)
{
    (void)connId; (void)operation; (void)offset; (void)len; (void)pAttr;

    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    switch(handle)
    {
        case LED_CONTROL_HDL:
            /* Gateway bridge is forwarding an MQTT LED command. */
            event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
            event.BleConnectionEvent.Value = pValue[0];
            GetAppTask().PostEvent(&event);
            break;

        default:
            GP_LOG_SYSTEM_PRINTF("[BLE] Write to unknown handle 0x%04X ignored", 0, handle);
            break;
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief Handle Client Characteristic Configuration Descriptor (CCC) changes.
 *
 * Called when the gateway bridge enables or disables notifications on a
 * characteristic.  Logged for diagnostics; no application action required.
 */
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    const char* charName;

    switch(event->handle)
    {
        case BATTERY_LEVEL_CCC_HDL: charName = "Battery Level";  break;
        case DOORBELL_RING_CCC_HDL: charName = "Doorbell Ring";  break;
        case HEARTBEAT_CCC_HDL:     charName = "Heartbeat";      break;
        default:                    charName = "Unknown";        break;
    }

    if(event->value & 0x0001)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Notifications ENABLED  for '%s'", 0, charName);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Notifications DISABLED for '%s'", 0, charName);
    }
}

/* -------------------------------------------------------------------------- */

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
void AppManager::ResetCountEventHandler(AppEvent* aEvent)
{
    (void)aEvent;
    /* Reserved for reset-count based behaviour (e.g. factory reset). */
}
#endif
