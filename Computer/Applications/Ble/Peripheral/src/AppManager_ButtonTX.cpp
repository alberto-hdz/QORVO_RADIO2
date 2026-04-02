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

    // === ADDED SECTION ===
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("    Button Transmitter (ButtonTX)", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("Starting BLE advertising automatically...", 0);

    // Auto-start advertising so device is immediately discoverable
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("Advertising started! Look for 'ButtonTX' in your BLE app", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("Could not start advertising - hold button 2+ sec to start manually", 0);
    }
    GP_LOG_SYSTEM_PRINTF("", 0);
    // === END OF ADDED SECTION ===
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
            // ===========================================
            // BUTTON PRESSED - Turn ON the remote LED
            // ===========================================
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF("**** BUTTON PRESSED ****", 0);

            // Turn on LOCAL LED to show button is pressed
            StatusLed_SetLed(APP_STATE_LED, true);

            // Send BLE notification with value 1 (LED ON)
            uint8_t ledValue = 1;
            Status_t status = BleIf_SendNotification(LED_CONTROL_HDL, 1, &ledValue);

            if(status == STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("Notification sent: LED ON (value=1)", 0);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("Notification failed (error %d) - is device connected?", 0, status);
            }
        }
        else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
        {
            // ===========================================
            // BUTTON RELEASED - Turn OFF the remote LED
            // ===========================================
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF("**** BUTTON RELEASED ****", 0);
            GP_LOG_SYSTEM_PRINTF("Held for %d seconds", 0, aEvent->ButtonEvent.HeldSec);

            // Turn off LOCAL LED
            StatusLed_SetLed(APP_STATE_LED, false);

            // Check if short press or long hold
            if(aEvent->ButtonEvent.HeldSec < BLE_START_TIMEOUT)
            {
                // Short press - send notification to turn OFF remote LED
                uint8_t ledValue = 0;
                Status_t status = BleIf_SendNotification(LED_CONTROL_HDL, 1, &ledValue);

                if(status == STATUS_NO_ERROR)
                {
                    GP_LOG_SYSTEM_PRINTF("Notification sent: LED OFF (value=0)", 0);
                }
                else
                {
                    GP_LOG_SYSTEM_PRINTF("Notification failed (error %d) - is device connected?", 0, status);
                }
            }
            else if(aEvent->ButtonEvent.HeldSec < FACTORY_RESET_TIMEOUT)
            {
                // Long press (2+ seconds) - start BLE advertising
                GP_LOG_SYSTEM_PRINTF("Starting BLE advertising...", 0);
                if(BleIf_StartAdvertising() != STATUS_NO_ERROR)
                {
                    GP_LOG_SYSTEM_PRINTF("Failed to start BLE advertising", 0);
                }
            }
        }
        else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
        {
            if(aEvent->ButtonEvent.HeldSec == BLE_START_TIMEOUT)
            {
                GP_LOG_SYSTEM_PRINTF("Hold detected - will start advertising on release", 0);
            }
            else if(aEvent->ButtonEvent.HeldSec == FACTORY_RESET_TIMEOUT)
            {
                GP_LOG_SYSTEM_PRINTF("Factory reset not implemented", 0);
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
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;
    switch(handle)
    {
        case LED_CONTROL_HDL:
            /* update LED value */
            event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
            event.BleConnectionEvent.Value = pValue[0];
            break;
        default:
            GP_LOG_SYSTEM_PRINTF("RXCharWrite detected. Service handle not processed", 0);
            break;
    }
    /* Handle in AppManager */
    GetAppTask().PostEvent(&event);
}

static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    GP_LOG_SYSTEM_PRINTF("CCCD changed: handle=%d, value=0x%04X", 0, event->handle, event->value);

    if(event->handle == LED_CONTROL_CCC_HDL)
    {
        if(event->value & 0x0001)
        {
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF("*** NOTIFICATIONS ENABLED ***", 0);
            GP_LOG_SYSTEM_PRINTF("Button presses will now be sent to the connected device!", 0);
            GP_LOG_SYSTEM_PRINTF("", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("Notifications disabled", 0);
        }
    }
}