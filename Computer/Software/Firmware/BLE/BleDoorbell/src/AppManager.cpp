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

/** @file "AppManager.cpp"
 *
 * Application logic for the QPG6200 BLE Doorbell demo.
 *
 * Board: QPG6200L Development Kit
 *
 * Behaviour:
 *   - On boot: print banner, start BLE advertising as "QPG Doorbell"
 *   - WHITE LED blinks  = advertising (waiting for connection)
 *   - WHITE LED solid   = connected to phone
 *   - BLUE LED blinks   = doorbell ring event (button press or remote write)
 *
 * Button (PB5):
 *   Short press  -> rings the doorbell (sends BLE notification 0x01 + LED blink)
 *   Long press (2s+) -> manually restarts advertising
 *
 * nRF Connect / Qorvo Connect:
 *   - Scan for "QPG Doorbell" and connect
 *   - Enable notifications on the Doorbell Ring characteristic
 *   - Press PB5: board sends notification value=0x01 to phone
 *   - Write 0x01 to the Doorbell Ring characteristic: board rings locally
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"
#include "BleDoorbell_Config.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* LED timing */
#define ADV_BLINK_ON_MS   500
#define ADV_BLINK_OFF_MS  500
#define RING_BLINK_ON_MS  100
#define RING_BLINK_OFF_MS 100

/* Button hold threshold for re-advertising (seconds) */
#define BLE_RESTART_ADV_THRESHOLD 2

AppManager AppManager::sAppMgr;

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

static BleIf_Callbacks_t appCallbacks;

/* Forward declarations */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t operation,
                                            uint16_t offset, BleIf_Attr_t* pAttr);
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t operation,
                                             uint16_t offset, uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr);
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

/* Ring the doorbell locally - called for both button press and remote write */
static void DoorbellRing(bool fromPhone)
{
    static uint32_t ringCount = 0;
    ringCount++;

    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("##############################################", 0);
    GP_LOG_SYSTEM_PRINTF("#                                            #", 0);
    GP_LOG_SYSTEM_PRINTF("#   ** DING DONG! **  Ring #%d              #", 0, ringCount);
    if(fromPhone)
    {
        GP_LOG_SYSTEM_PRINTF("#   Source: Remote (phone wrote 0x01)        #", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("#   Source: Local (PB5 button pressed)       #", 0);
    }
    GP_LOG_SYSTEM_PRINTF("#                                            #", 0);
    GP_LOG_SYSTEM_PRINTF("##############################################", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* BLUE LED: rapid blink to show ring event */
    StatusLed_BlinkLed(APP_BLE_CONNECTION_LED, RING_BLINK_ON_MS, RING_BLINK_OFF_MS);
}

void AppManager::Init()
{
    /* Set up BLE callbacks */
    appCallbacks.stackCallback    = BLE_Stack_Callback;
    appCallbacks.chrReadCallback  = BLE_CharacteristicRead_Callback;
    appCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    appCallbacks.cccCallback      = BLE_CCCD_Callback;

    /* Initialize BLE stack */
    BleIf_Init(&appCallbacks);

    /* Initialize LEDs - both OFF at start */
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(APP_STATE_LED, false);
    StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);

    /* Register doorbell button */
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    /* Print startup banner */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("     QPG6200 BLE DOORBELL DEMO", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("Board  : QPG6200L Development Kit", 0);
    GP_LOG_SYSTEM_PRINTF("Device : QPG Doorbell (BLE Peripheral)", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE blinks = advertising", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE solid  = connected to phone", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE blinks  = doorbell ring event", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Button Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  PB5 short press = ring doorbell", 0);
    GP_LOG_SYSTEM_PRINTF("  PB5 hold 2s+    = restart advertising", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- App Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  1. Open nRF Connect or Qorvo Connect", 0);
    GP_LOG_SYSTEM_PRINTF("  2. Scan and connect to 'QPG Doorbell'", 0);
    GP_LOG_SYSTEM_PRINTF("  3. Find Doorbell Ring characteristic", 0);
    GP_LOG_SYSTEM_PRINTF("  4. Enable notifications (subscribe)", 0);
    GP_LOG_SYSTEM_PRINTF("  5. Press PB5 to ring! (notification=0x01)", 0);
    GP_LOG_SYSTEM_PRINTF("  6. Write 0x01 to remotely ring the board", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Auto-start BLE advertising */
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started - scan for 'QPG Doorbell'", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising will start after stack reset...", 0);
    }
}

void AppManager::EventHandler(AppEvent* aEvent)
{
    if(aEvent == nullptr)
    {
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
        default:
            break;
    }
}

void AppManager::BleEventHandler(AppEvent* aEvent)
{
    switch(aEvent->BleConnectionEvent.Event)
    {
        case Ble_Event_t::kBleConnectionEvent_Advertise_Start:
            GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started - WHITE LED blinking", 0);
            StatusLed_BlinkLed(APP_STATE_LED, ADV_BLINK_ON_MS, ADV_BLINK_OFF_MS);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            break;

        case Ble_Event_t::kBleConnectionEvent_Connected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Phone connected! WHITE LED solid ON", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Press PB5 to send a ring notification", 0);
            StatusLed_SetLed(APP_STATE_LED, true);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            break;

        case Ble_Event_t::kBleConnectionEvent_Disconnected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Phone disconnected. WHITE LED OFF", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Hold PB5 for 2s to restart advertising", 0);
            StatusLed_SetLed(APP_STATE_LED, false);
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            break;

        case Ble_Event_t::kBleLedControlCharUpdate:
            /* Phone wrote to Doorbell Ring characteristic */
            if(aEvent->BleConnectionEvent.Value == DOORBELL_STATE_RINGING)
            {
                DoorbellRing(true /* fromPhone */);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell reset by phone (value=0x00)", 0);
                StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            }
            break;

        default:
            break;
    }
}

void AppManager::ButtonEventHandler(AppEvent* aEvent)
{
    if(aEvent->ButtonEvent.Index != APP_MULTI_FUNC_BUTTON)
    {
        return;
    }

    if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Pressed)
    {
        /* Button pressed - ring the doorbell immediately */
        DoorbellRing(false /* fromPhone */);

        /* Send BLE notification (value 0x01 = ringing) */
        uint8_t ringValue = DOORBELL_STATE_RINGING;
        Status_t status = BleIf_SendNotification(DOORBELL_RING_HDL, 1, &ringValue);

        if(status == STATUS_NO_ERROR)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Notification sent to phone (value=0x01)", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] No phone connected - notification not sent", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Connect via nRF Connect to receive notifications", 0);
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
    {
        /* Long hold: restart advertising */
        if(aEvent->ButtonEvent.HeldSec >= BLE_RESTART_ADV_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Restarting advertising...", 0);
            if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started - scan for 'QPG Doorbell'", 0);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Already advertising or connected", 0);
            }
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
    {
        if(aEvent->ButtonEvent.HeldSec == BLE_RESTART_ADV_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Hold detected - release to restart advertising", 0);
        }
    }
}

/*****************************************************************************
 *                    BLE Stack Callbacks (called from BleIf)
 *****************************************************************************/

static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

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
                event.Type = AppEvent::kEventType_Invalid;
                break;
        }

        if(event.Type != AppEvent::kEventType_Invalid)
        {
            GetAppTask().PostEvent(&event);
        }
    }
}

static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t operation,
                                            uint16_t offset, BleIf_Attr_t* pAttr)
{
    /* Read handled automatically by BleIf using the static attribute value */
    (void)connId; (void)handle; (void)operation; (void)offset; (void)pAttr;
}

static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t operation,
                                             uint16_t offset, uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr)
{
    (void)connId; (void)operation; (void)offset; (void)len; (void)pAttr;

    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    if(handle == DOORBELL_RING_HDL)
    {
        event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
        event.BleConnectionEvent.Value = pValue[0];
        GetAppTask().PostEvent(&event);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Write to unknown handle 0x%04X", 0, handle);
    }
}

static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    if(event->handle == DOORBELL_RING_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] *** NOTIFICATIONS ENABLED ***", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Phone subscribed to doorbell notifications!", 0);
            GP_LOG_SYSTEM_PRINTF("[BLE] Press PB5 to send a ring to the phone.", 0);
            GP_LOG_SYSTEM_PRINTF("", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Notifications disabled by phone", 0);
        }
    }
}
