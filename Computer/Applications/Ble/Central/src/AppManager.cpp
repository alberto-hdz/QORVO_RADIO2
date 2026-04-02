/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * AppManager.cpp - LED Receiver (LedRX) Application Logic
 * 
 * This board RECEIVES LED commands via BLE writes.
 * When a phone/central writes value 1, LED turns ON.
 * When a phone/central writes value 0, LED turns OFF.
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

/* Timing constants */
#define LED_BLINK_INTERVAL    250
#define BLE_START_TIMEOUT     2
#define FACTORY_RESET_TIMEOUT 10

/* Static instance */
AppManager AppManager::sAppMgr;

/* LED GPIO pins */
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/* BLE callback structure */
static BleIf_Callbacks_t appCallbacks;

/*
 * ===========================================================================
 * CALLBACK FUNCTION PROTOTYPES
 * ===========================================================================
 */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, 
                                            uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr);
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, 
                                             uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue, 
                                             BleIf_Attr_t* pAttr);
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

/*
 * ===========================================================================
 * INITIALIZATION
 * ===========================================================================
 */
void AppManager::Init()
{
    /* Register button */
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    /* Set up BLE callbacks */
    appCallbacks.stackCallback = BLE_Stack_Callback;
    appCallbacks.chrReadCallback = BLE_CharacteristicRead_Callback;
    appCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    appCallbacks.cccCallback = BLE_CCCD_Callback;

    /* Initialize BLE */
    BleIf_Init(&appCallbacks);

    /* Initialize LEDs - both OFF initially */
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(APP_STATE_LED, false);
    StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
    
    /* Print startup banner */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("        LED RECEIVER (LedRX)", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("This board receives LED commands via BLE.", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("Instructions:", 0);
    GP_LOG_SYSTEM_PRINTF("  1. Connect with Qorvo Connect app", 0);
    GP_LOG_SYSTEM_PRINTF("  2. Find 'LedRX' and connect", 0);
    GP_LOG_SYSTEM_PRINTF("  3. Find LED Control characteristic", 0);
    GP_LOG_SYSTEM_PRINTF("  4. Write 01 to turn LED ON", 0);
    GP_LOG_SYSTEM_PRINTF("  5. Write 00 to turn LED OFF", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    
    /* Auto-start advertising */
    GP_LOG_SYSTEM_PRINTF("Starting BLE advertising...", 0);
    
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("SUCCESS: Now advertising as 'LedRX'", 0);
        GP_LOG_SYSTEM_PRINTF("Blue LED blinking = advertising", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("Could not start advertising yet", 0);
        GP_LOG_SYSTEM_PRINTF("Hold button for 2+ seconds to start manually", 0);
    }
    GP_LOG_SYSTEM_PRINTF("", 0);
}

/*
 * ===========================================================================
 * EVENT HANDLER (Main dispatcher)
 * ===========================================================================
 */
void AppManager::EventHandler(AppEvent* aEvent)
{
    if(aEvent == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("ERROR: Null event received", 0);
        return;
    }

    switch(aEvent->Type)
    {
        case AppEvent::kEventType_ResetCount:
            break;
            
        case AppEvent::kEventType_Buttons:
            ButtonEventHandler(aEvent);
            break;
            
        case AppEvent::kEventType_BleConnection:
            BleEventHandler(aEvent);
            break;
            
        default:
            GP_LOG_SYSTEM_PRINTF("Unknown event type: %d", 0, static_cast<int>(aEvent->Type));
            break;
    }
}

/*
 * ===========================================================================
 * BLE EVENT HANDLER
 * ===========================================================================
 */
void AppManager::BleEventHandler(AppEvent* aEvent)
{
    switch(aEvent->BleConnectionEvent.Event)
    {
        case Ble_Event_t::kBleConnectionEvent_Connected:
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF(">>> CONNECTED <<<", 0);
            GP_LOG_SYSTEM_PRINTF("Ready to receive LED commands!", 0);
            GP_LOG_SYSTEM_PRINTF("Write 01 to LED Control to turn LED ON", 0);
            GP_LOG_SYSTEM_PRINTF("Write 00 to LED Control to turn LED OFF", 0);
            GP_LOG_SYSTEM_PRINTF("", 0);
            /* Solid blue LED = connected */
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, true);
            break;
            
        case Ble_Event_t::kBleConnectionEvent_Advertise_Start:
            GP_LOG_SYSTEM_PRINTF("Advertising started (blue LED blinking)", 0);
            /* Blinking blue LED = advertising */
            StatusLed_BlinkLed(APP_BLE_CONNECTION_LED, LED_BLINK_INTERVAL, LED_BLINK_INTERVAL);
            break;
            
        case Ble_Event_t::kBleConnectionEvent_Disconnected:
            GP_LOG_SYSTEM_PRINTF("", 0);
            GP_LOG_SYSTEM_PRINTF(">>> DISCONNECTED <<<", 0);
            GP_LOG_SYSTEM_PRINTF("LED state preserved. Restarting advertising...", 0);
            GP_LOG_SYSTEM_PRINTF("", 0);
            /* Turn off connection LED, keep state LED as-is */
            StatusLed_SetLed(APP_BLE_CONNECTION_LED, false);
            /* Auto-restart advertising */
            BleIf_StartAdvertising();
            break;
            
        case Ble_Event_t::kBleLedControlCharUpdate:
            /* ============================================ */
            /* THIS IS WHERE WE RECEIVE THE LED COMMAND!   */
            /* ============================================ */
            GP_LOG_SYSTEM_PRINTF("", 0);
            if(aEvent->BleConnectionEvent.Value == 0)
            {
                GP_LOG_SYSTEM_PRINTF(">>> RECEIVED: LED OFF (value=0) <<<", 0);
                StatusLed_SetLed(APP_STATE_LED, false);
                GP_LOG_SYSTEM_PRINTF("White LED is now OFF", 0);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF(">>> RECEIVED: LED ON (value=%d) <<<", 0, aEvent->BleConnectionEvent.Value);
                StatusLed_SetLed(APP_STATE_LED, true);
                GP_LOG_SYSTEM_PRINTF("White LED is now ON", 0);
            }
            GP_LOG_SYSTEM_PRINTF("", 0);
            break;
            
        default:
            GP_LOG_SYSTEM_PRINTF("Unhandled BLE event: %d", 0, 
                               static_cast<int>(aEvent->BleConnectionEvent.Event));
            break;
    }
}

/*
 * ===========================================================================
 * BUTTON EVENT HANDLER
 * For this receiver, button is only used to restart advertising
 * ===========================================================================
 */
void AppManager::ButtonEventHandler(AppEvent* aEvent)
{
    if(aEvent->ButtonEvent.Index != APP_MULTI_FUNC_BUTTON)
    {
        return;
    }
    
    switch(aEvent->ButtonEvent.State)
    {
        case ButtonEvent_t::kButtonState_Pressed:
            GP_LOG_SYSTEM_PRINTF("Button pressed", 0);
            break;
            
        case ButtonEvent_t::kButtonState_Released:
            GP_LOG_SYSTEM_PRINTF("Button released (held %d sec)", 0, aEvent->ButtonEvent.HeldSec);
            
            if(aEvent->ButtonEvent.HeldSec < BLE_START_TIMEOUT)
            {
                /* Short press - toggle LED manually for testing */
                static bool ledState = false;
                ledState = !ledState;
                StatusLed_SetLed(APP_STATE_LED, ledState);
                GP_LOG_SYSTEM_PRINTF("Manual toggle: LED %s", 0, ledState ? "ON" : "OFF");
            }
            else if(aEvent->ButtonEvent.HeldSec < FACTORY_RESET_TIMEOUT)
            {
                /* Long press - restart advertising */
                GP_LOG_SYSTEM_PRINTF("Restarting advertising...", 0);
                BleIf_StartAdvertising();
            }
            break;
            
        case ButtonEvent_t::kButtonState_Held:
            if(aEvent->ButtonEvent.HeldSec == BLE_START_TIMEOUT)
            {
                GP_LOG_SYSTEM_PRINTF("Hold detected: Will restart advertising on release", 0);
            }
            break;
            
        default:
            break;
    }
}

/*
 * ===========================================================================
 * BLE STACK CALLBACK
 * ===========================================================================
 */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg)
{
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;

    if(pMsg->event >= BLEIF_DM_CBACK_START && pMsg->event <= BLEIF_DM_CBACK_END)
    {
        GP_LOG_SYSTEM_PRINTF("BLE event %d (status=%d)", 0, pMsg->event, pMsg->status);
        
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
    else if(pMsg->event >= BLEIF_ATT_CBACK_START && pMsg->event <= BLEIF_ATT_CBACK_END)
    {
        GP_LOG_SYSTEM_PRINTF("ATT event %d (status=%d)", 0, pMsg->event, pMsg->status);
    }
}

/*
 * ===========================================================================
 * CHARACTERISTIC READ CALLBACK
 * ===========================================================================
 */
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, 
                                            uint8_t operation, uint16_t offset,
                                            BleIf_Attr_t* pAttr)
{
    GP_LOG_SYSTEM_PRINTF("Characteristic READ: handle=%d", 0, handle);
}

/*
 * ===========================================================================
 * CHARACTERISTIC WRITE CALLBACK
 * 
 * THIS IS THE KEY FUNCTION FOR THE RECEIVER!
 * When the phone writes to our LED Control characteristic, this is called.
 * ===========================================================================
 */
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, 
                                             uint8_t operation, uint16_t offset,
                                             uint16_t len, uint8_t* pValue, 
                                             BleIf_Attr_t* pAttr)
{
    GP_LOG_SYSTEM_PRINTF("Characteristic WRITE: handle=%d, len=%d, value=%d", 0, handle, len, pValue[0]);
    
    AppEvent event;
    event.Type = AppEvent::kEventType_BleConnection;
    
    switch(handle)
    {
        case LED_CONTROL_HDL:
            /* LED Control characteristic was written! */
            event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
            event.BleConnectionEvent.Value = pValue[0];
            GetAppTask().PostEvent(&event);
            break;
            
        default:
            GP_LOG_SYSTEM_PRINTF("Unknown handle written: %d", 0, handle);
            break;
    }
}

/*
 * ===========================================================================
 * CCCD CALLBACK
 * ===========================================================================
 */
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    GP_LOG_SYSTEM_PRINTF("CCCD changed: handle=%d, value=0x%04X", 0, event->handle, event->value);
}