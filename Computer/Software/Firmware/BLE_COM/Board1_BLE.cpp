/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 *
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

/** @file "AppManager.cpp"
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"
#include "Ble_Peripheral_Config.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define LED_BLINK_INTERVAL 250 // ms
#define FACTORY_RESET_TIMEOUT 10  // s
#define BLE_START_TIMEOUT     2   // s

AppManager AppManager::sAppMgr;

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

static BleIf_Callbacks_t appCallbacks;

// BLE Stack callback (DM, ATT events)
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
// Characteristic Read callback
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr);
// Characteristic Write callback
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue, BleIf_Attr_t* pAttr);
// Client Characteristic Configuration Descriptor callback
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

void AppManager::Init()
{
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    appCallbacks.stackCallback = BLE_Stack_Callback;
    appCallbacks.chrReadCallback = BLE_CharacteristicRead_Callback;
    appCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    appCallbacks.cccCallback = BLE_CCCD_Callback;

    BleIf_Init(&appCallbacks);

    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(APP_STATE_LED, false);
    StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
}

void AppManager::EventHandler(AppEvent* aEvent)
{
    if(aEvent == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("Event Queue is nullptr should never happen", 0);
        return;
    }

    switch(aEvent->Type)
    {
        case AppEvent::kEventType_ResetCount:
            /* ResetCountEventHandler(aEvent); */
            break;
        case AppEvent::kEventType_Buttons:
            /* Currently used to start BLE advertising */
            ButtonEventHandler(aEvent);
            break;
        case AppEvent::kEventType_BleConnection:
            BleEventHandler(aEvent);
            break;
        default:
            GP_LOG_SYSTEM_PRINTF("Unhandled event type: %d", 0, static_cast<int>(aEvent->Type));
            break;
    }
}

#if defined(GP_APP_DIVERSITY_RESETCOUNTING)
void AppManager::ResetCountEventHandler(AppEvent* aEvent)
{
    // ResetCountEventHandler is able to record number of resets
    // e.g. user can implement functionality based on number of resets
}
#endif

void AppManager::BleEventHandler(AppEvent* aEvent)
{
    if(aEvent->BleConnectionEvent.Event == Ble_Event_t::kBleConnectionEvent_Connected)
    {
        StatusLed_SetLed(APP_BLE_CONNECTION_LED, true);
    }
    else if(aEvent->BleConnectionEvent.Event == Ble_Event_t::kBleConnectionEvent_Advertise_Start)
    {
        StatusLed_BlinkLed(APP_BLE_CONNECTION_LED, LED_BLINK_INTERVAL, LED_BLINK_INTERVAL);
    }
    else if(aEvent->BleConnectionEvent.Event == Ble_Event_t::kBleConnectionEvent_Disconnected)
    {
        StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
    }
    else if(aEvent->BleConnectionEvent.Event == Ble_Event_t::kBleLedControlCharUpdate)
    {
        GP_LOG_SYSTEM_PRINTF("Updating Status LED", 0);
        if(aEvent->BleConnectionEvent.Value == 0)
        {
            StatusLed_SetLed(APP_STATE_LED, false);
        }
        else
        {
            StatusLed_SetLed(APP_STATE_LED, true);
        }
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("Unhandled BLE event: %d", 0, static_cast<int>(aEvent->BleConnectionEvent.Event));
    }
}

void AppManager::ButtonEventHandler(AppEvent* aEvent)
{
    if(aEvent->ButtonEvent.Index == APP_MULTI_FUNC_BUTTON)
    {
        if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Pressed)
        {
            /* Not implemented */
        }
        else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
        {
            if(aEvent->ButtonEvent.HeldSec == BLE_START_TIMEOUT)
            {
                GP_LOG_SYSTEM_PRINTF("Button: BLE start selected", 0);
            }
            else if(aEvent->ButtonEvent.HeldSec == FACTORY_RESET_TIMEOUT)
            {
                /* Not implemented */
            }
        }
        else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
        {
            GP_LOG_SYSTEM_PRINTF("Button: Function button released. Held for %d", 0, aEvent->ButtonEvent.HeldSec);

            if(aEvent->ButtonEvent.HeldSec < BLE_START_TIMEOUT)
            {
                /* Short hold */
                /* Not implemented */
            }
            else if(aEvent->ButtonEvent.HeldSec < FACTORY_RESET_TIMEOUT)
            {
                GP_LOG_SYSTEM_PRINTF("Button event: Starting BLE advertising", 0);
                if(BleIf_StartAdvertising() != STATUS_NO_ERROR)
                {
                    GP_LOG_SYSTEM_PRINTF("Failed to start BLE advertising", 0);
                }
            }
        }
    }
}

static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    /* Process advertising/scanning and connection-related messages */
    if(pMsg->event >= BLEIF_DM_CBACK_START && pMsg->event <= BLEIF_DM_CBACK_END)
    {
        GP_LOG_SYSTEM_PRINTF("DM event %d: status %d", 0, pMsg->event, pMsg->status);
        switch(pMsg->event)
        {
            case BLEIF_DM_ADV_START_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Advertise_Start;
                GP_LOG_SYSTEM_PRINTF("BLE advertising started", 0);
                break;
            case BLEIF_DM_CONN_OPEN_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Connected;
                GP_LOG_SYSTEM_PRINTF("BLE connection opened", 0);
                break;
            case BLEIF_DM_ADV_STOP_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Disconnected;
                GP_LOG_SYSTEM_PRINTF("BLE advertising stopped", 0);
                break;
            case BLEIF_DM_CONN_CLOSE_IND:
                event.BleConnectionEvent.Event = Ble_Event_t::kBleConnectionEvent_Disconnected;
                GP_LOG_SYSTEM_PRINTF("BLE connection close", 0);
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
    /* Process attribute-related messages */
    else if(pMsg->event >= BLEIF_ATT_CBACK_START && pMsg->event <= BLEIF_ATT_CBACK_END)
    {
        GP_LOG_SYSTEM_PRINTF("ATT event %d: status %d", 0, pMsg->event, pMsg->status);
    }
}

static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr)
{
    /* Not implemented */
}

static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue, BleIf_Attr_t* pAttr)
{
    GP_LOG_SYSTEM_PRINTF("=== BLE Characteristic Write Received ===", 0);
    GP_LOG_SYSTEM_PRINTF("  Connection ID: %u", 0, connId);
    GP_LOG_SYSTEM_PRINTF("  Attribute Handle: 0x%04X (%u)", 0, handle, handle);
    GP_LOG_SYSTEM_PRINTF("  Operation: %u", 0, operation);
    GP_LOG_SYSTEM_PRINTF("  Offset: %u", 0, offset);
    GP_LOG_SYSTEM_PRINTF("  Length: %u bytes", 0, len);

    if (len == 0 || pValue == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("  WARNING: Empty or invalid write (len=0 or null pointer)", 0);
        return;
    }

    // Show first byte (most relevant for simple LED control)
    GP_LOG_SYSTEM_PRINTF("  Value[0]: 0x%02X (%d)  → LED will be %s", 0,
                         pValue[0], pValue[0],
                         (pValue[0] != 0) ? "ON" : "OFF");

    // Show a few more bytes if the write is longer (useful for debugging)
    if (len >= 2) GP_LOG_SYSTEM_PRINTF("  Value[1]: 0x%02X", 0, pValue[1]);
    if (len >= 3) GP_LOG_SYSTEM_PRINTF("  Value[2]: 0x%02X", 0, pValue[2]);
    if (len >= 4) GP_LOG_SYSTEM_PRINTF("  Value[3]: 0x%02X", 0, pValue[3]);

    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    switch(handle)
    {
        case LED_CONTROL_HDL:
            GP_LOG_SYSTEM_PRINTF("  → LED Control characteristic updated", 0);

            event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
            event.BleConnectionEvent.Value = pValue[0];
            GetAppTask().PostEvent(&event);
            break;

        default:
            GP_LOG_SYSTEM_PRINTF("  → WARNING: Write to unhandled handle 0x%04X", 0, handle);
            break;
    }
}

static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    /* Not implemented */
}