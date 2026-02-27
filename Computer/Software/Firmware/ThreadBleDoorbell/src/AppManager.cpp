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
 * Application logic for the QPG6200 Thread+BLE Doorbell demo.
 *
 * Board: QPG6200L Development Kit
 *
 * ── Behaviour ──────────────────────────────────────────────────────────────
 *  Boot:
 *    BLE advertising starts automatically ("QPG Thread Doorbell")
 *    If Thread credentials are stored in NVM, Thread join is also attempted.
 *
 *  BLE connection state (WHITE_COOL LED):
 *    Blinking  = advertising (waiting for phone)
 *    Solid ON  = phone connected
 *    OFF       = disconnected (hold PB1 for 2 s to restart advertising)
 *
 *  Thread network state (GREEN LED):
 *    Blinking  = joining / attaching to Thread network
 *    Solid ON  = attached (child, router, or leader)
 *    OFF       = detached / disabled
 *
 *  Ring event (BLUE LED):
 *    Rapid blink = doorbell ring (from analog button, BLE write, or Thread mesh)
 *
 * ── Commissioning via BLE ──────────────────────────────────────────────────
 *  1. Connect to "QPG Thread Doorbell" with nRF Connect or Qorvo Connect.
 *  2. Write Thread Network Name  (e.g. "DoorbellNet").
 *  3. Write Thread Network Key   (16-byte master key - write-only).
 *  4. Optionally write Channel   (1 byte, 11-26, default 15).
 *  5. Optionally write PAN ID    (2 bytes LE, default 0xABCD).
 *  6. Write 0x01 to Join         → device attempts to join Thread network.
 *  7. Subscribe to Thread Status  → watch device role changes (0=disabled … 4=leader).
 *
 * ── Buttons ────────────────────────────────────────────────────────────────
 *  PB1 short press  (<2 s)  : restart BLE advertising
 *  PB1 long press   (≥5 s)  : factory-reset Thread credentials and reboot
 *
 * ── Analog doorbell (GPIO 28 / ANIO0, Pin 11) ──────────────────────────────
 *  Press  → ring locally (BLUE LED, BLE notification 0x01, Thread UDP multicast)
 *  Release→ no action
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "ThreadBleDoorbell_Config.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"

/* OpenThread headers */
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------
 * LED timing constants
 * ------------------------------------------------------------------------- */
#define ADV_BLINK_ON_MS         500
#define ADV_BLINK_OFF_MS        500
#define THREAD_JOIN_BLINK_ON_MS  200
#define THREAD_JOIN_BLINK_OFF_MS 200
#define RING_BLINK_ON_MS        100
#define RING_BLINK_OFF_MS       100

/* -------------------------------------------------------------------------
 * Button thresholds (seconds held)
 * ------------------------------------------------------------------------- */
#define BTN_RESTART_ADV_THRESHOLD  2   /**< seconds to hold PB1 to restart BLE adv */
#define BTN_FACTORY_RESET_THRESHOLD 5  /**< seconds to hold PB1 for Thread factory reset */

/* -------------------------------------------------------------------------
 * Thread ring multicast address (all Thread nodes in the local network)
 * ff03::1 = Thread realm-local all-nodes multicast
 * ------------------------------------------------------------------------- */
#define THREAD_RING_PORT   5683   /**< CoAP default port (reused for simplicity) */
#define THREAD_RING_MCAST  "ff03::1"

/* LED indices (must match QPINCFG_STATUS_LED order in qPinCfg.h):
 *   0 = WHITE_COOL (BLE state)
 *   1 = GREEN      (Thread state)
 *   2 = BLUE       (ring indicator) */
#define LED_BLE_STATE    0
#define LED_THREAD_STATE 1
#define LED_RING         2

/* -------------------------------------------------------------------------
 * Static members
 * ------------------------------------------------------------------------- */
AppManager AppManager::sAppMgr;

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

static BleIf_Callbacks_t sAppCallbacks;

/* Thread state */
static bool           sThreadCredentialsAvailable = false;
static otInstance*    sThreadInstance             = nullptr;
static otUdpSocket    sThreadUdpSocket;
static bool           sThreadUdpSocketOpen        = false;

/* Ring counter for logging */
static uint32_t sRingCount = 0;

/* -------------------------------------------------------------------------
 * Forward declarations
 * ------------------------------------------------------------------------- */
static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t op,
                                            uint16_t offset, BleIf_Attr_t* pAttr);
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t op,
                                             uint16_t offset, uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr);
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

static void Thread_Init(void);
static void Thread_StartJoin(void);
static void Thread_SendRingMulticast(void);
static void Thread_UdpReceiveCallback(void* aContext, otMessage* aMessage,
                                      const otMessageInfo* aMessageInfo);
static void Thread_StateChangeCallback(uint32_t aFlags, void* aContext);

/* -------------------------------------------------------------------------
 * Helper: accessor for config values defined in Config.c
 * ------------------------------------------------------------------------- */
extern "C" uint8_t* ThreadCfg_GetNetworkName(uint16_t* pLen);
extern "C" uint8_t* ThreadCfg_GetNetworkKey(void);
extern "C" uint8_t  ThreadCfg_GetChannel(void);
extern "C" uint16_t ThreadCfg_GetPanId(void);
extern "C" void     ThreadCfg_SetStatus(uint8_t status);
extern "C" uint8_t  ThreadCfg_GetStatus(void);

/* =========================================================================
 *  AppManager::Init
 * ========================================================================= */
void AppManager::Init()
{
    /* --- BLE ------------------------------------------------------------ */
    sAppCallbacks.stackCallback    = BLE_Stack_Callback;
    sAppCallbacks.chrReadCallback  = BLE_CharacteristicRead_Callback;
    sAppCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    sAppCallbacks.cccCallback      = BLE_CCCD_Callback;
    BleIf_Init(&sAppCallbacks);

    /* --- LEDs ----------------------------------------------------------- */
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(LED_BLE_STATE,    false);
    StatusLed_SetLed(LED_THREAD_STATE, false);
    StatusLed_SetLed(LED_RING,         false);

    /* --- Button --------------------------------------------------------- */
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    /* --- Thread --------------------------------------------------------- */
    Thread_Init();

    /* --- Banner --------------------------------------------------------- */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("  QPG6200 THREAD + BLE DOORBELL DEMO", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("Board  : QPG6200L Development Kit", 0);
    GP_LOG_SYSTEM_PRINTF("Button : ANIO0_GPI28 (GPIO 28, Pin 11)", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE blinks  = BLE advertising", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE solid   = BLE connected", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN blinks  = Thread joining", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN solid   = Thread attached", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE blinks   = Ring event", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Thread Commissioning (via BLE) ---", 0);
    GP_LOG_SYSTEM_PRINTF("  1. Connect to 'QPG Thread Doorbell'", 0);
    GP_LOG_SYSTEM_PRINTF("  2. Write Thread Network Name (16 bytes)", 0);
    GP_LOG_SYSTEM_PRINTF("  3. Write Thread Network Key  (16 bytes)", 0);
    GP_LOG_SYSTEM_PRINTF("  4. Write Channel  (1 byte, 11-26)", 0);
    GP_LOG_SYSTEM_PRINTF("  5. Write PAN ID   (2 bytes LE)", 0);
    GP_LOG_SYSTEM_PRINTF("  6. Write 0x01 to Join characteristic", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Doorbell ---", 0);
    GP_LOG_SYSTEM_PRINTF("  Analog button on GPIO28/ANIO0", 0);
    GP_LOG_SYSTEM_PRINTF("  Hold PB1 5s = factory reset Thread creds", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Start BLE advertising */
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started - scan for 'QPG Thread Doorbell'", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising will start after stack reset...", 0);
    }
}

/* =========================================================================
 *  AppManager::EventHandler  - dispatch to sub-handlers
 * ========================================================================= */
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
        case AppEvent::kEventType_Analog:
            AnalogEventHandler(aEvent);
            break;
        case AppEvent::kEventType_Thread:
            ThreadEventHandler(aEvent);
            break;
        default:
            break;
    }
}

/* =========================================================================
 *  BleEventHandler
 * ========================================================================= */
void AppManager::BleEventHandler(AppEvent* aEvent)
{
    switch(aEvent->BleConnectionEvent.Event)
    {
        case Ble_Event_t::kBleConnectionEvent_Advertise_Start:
            GP_LOG_SYSTEM_PRINTF("[BLE] Advertising started", 0);
            StatusLed_BlinkLed(LED_BLE_STATE, ADV_BLINK_ON_MS, ADV_BLINK_OFF_MS);
            break;

        case Ble_Event_t::kBleConnectionEvent_Connected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Phone connected", 0);
            StatusLed_SetLed(LED_BLE_STATE, true);
            break;

        case Ble_Event_t::kBleConnectionEvent_Disconnected:
            GP_LOG_SYSTEM_PRINTF("[BLE] Phone disconnected", 0);
            StatusLed_SetLed(LED_BLE_STATE, false);
            break;

        case Ble_Event_t::kBleLedControlCharUpdate:
            /* Phone wrote to Doorbell Ring characteristic */
            if(aEvent->BleConnectionEvent.Value == DOORBELL_STATE_RINGING)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Remote ring from phone", 0);
                RingDoorbell(false /* fromThread */, true /* fromPhone */);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell reset by phone", 0);
                StatusLed_SetLed(LED_RING, false);
            }
            break;

        default:
            break;
    }
}

/* =========================================================================
 *  ButtonEventHandler  - digital PB1
 * ========================================================================= */
void AppManager::ButtonEventHandler(AppEvent* aEvent)
{
    if(aEvent->ButtonEvent.Index != APP_MULTI_FUNC_BUTTON)
    {
        return;
    }

    if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Pressed)
    {
        /* Short press: nothing on press, action on release */
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
    {
        uint8_t held = aEvent->ButtonEvent.HeldSec;

        if(held >= BTN_FACTORY_RESET_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Factory reset - clearing Thread credentials", 0);
            /* Clear Thread credentials from NVM and reboot */
            if(sThreadInstance != nullptr)
            {
                otInstanceFactoryReset(sThreadInstance);
            }
            AppTask::ResetSystem();
        }
        else if(held >= BTN_RESTART_ADV_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Restarting BLE advertising", 0);
            BleIf_StartAdvertising();
        }
    }
    else if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Held)
    {
        if(aEvent->ButtonEvent.HeldSec == BTN_RESTART_ADV_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Release now to restart BLE advertising", 0);
        }
        else if(aEvent->ButtonEvent.HeldSec == BTN_FACTORY_RESET_THRESHOLD)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] Release now to factory-reset Thread credentials!", 0);
        }
    }
}

/* =========================================================================
 *  AnalogEventHandler  - GPADC doorbell on GPIO 28
 * ========================================================================= */
void AppManager::AnalogEventHandler(AppEvent* aEvent)
{
    if(aEvent->AnalogEvent.State == kAnalogEvent_Pressed)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] Doorbell button pressed (raw=%u)", 0,
                             aEvent->AnalogEvent.AdcRaw);
        RingDoorbell(false /* fromThread */, false /* fromPhone */);
    }
    else
    {
        /* Release - no action required for a doorbell */
    }
}

/* =========================================================================
 *  ThreadEventHandler
 * ========================================================================= */
void AppManager::ThreadEventHandler(AppEvent* aEvent)
{
    switch(aEvent->ThreadEvent.Event)
    {
        case kThreadEvent_Joined:
            GP_LOG_SYSTEM_PRINTF("[Thread] Attached to network (role=%d)", 0,
                                 aEvent->ThreadEvent.Value);
            StatusLed_SetLed(LED_THREAD_STATE, true);
            ThreadCfg_SetStatus((uint8_t)aEvent->ThreadEvent.Value);
            /* Notify any connected BLE peer */
            {
                uint8_t status = ThreadCfg_GetStatus();
                BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
            }
            break;

        case kThreadEvent_Detached:
            GP_LOG_SYSTEM_PRINTF("[Thread] Detached from network", 0);
            StatusLed_SetLed(LED_THREAD_STATE, false);
            ThreadCfg_SetStatus(THREAD_STATUS_DETACHED);
            {
                uint8_t status = THREAD_STATUS_DETACHED;
                BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
            }
            break;

        case kThreadEvent_RingReceived:
            GP_LOG_SYSTEM_PRINTF("[Thread] Ring event received from mesh", 0);
            RingDoorbell(true /* fromThread */, false /* fromPhone */);
            break;

        case kThreadEvent_Error:
            GP_LOG_SYSTEM_PRINTF("[Thread] Error: 0x%x", 0, aEvent->ThreadEvent.Value);
            break;

        default:
            break;
    }
}

/* =========================================================================
 *  RingDoorbell  - local ring effect + BLE notification + Thread multicast
 * ========================================================================= */
void AppManager::RingDoorbell(bool fromThread, bool fromPhone)
{
    sRingCount++;

    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("##############################################", 0);
    GP_LOG_SYSTEM_PRINTF("#   ** DING DONG! ** Ring #%lu", 0, sRingCount);
    if(fromThread)
    {
        GP_LOG_SYSTEM_PRINTF("#   Source: Thread mesh (remote device)", 0);
    }
    else if(fromPhone)
    {
        GP_LOG_SYSTEM_PRINTF("#   Source: BLE (phone wrote 0x01)", 0);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("#   Source: Local (GPIO28/ANIO0 analog button)", 0);
    }
    GP_LOG_SYSTEM_PRINTF("##############################################", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Blink BLUE ring LED */
    StatusLed_BlinkLed(LED_RING, RING_BLINK_ON_MS, RING_BLINK_OFF_MS);

    /* Send BLE notification (value 0x01) if phone is connected */
    uint8_t ringValue = DOORBELL_STATE_RINGING;
    Status_t bleStatus = BleIf_SendNotification(DOORBELL_RING_HDL, 1, &ringValue);
    if(bleStatus == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Ring notification sent to phone", 0);
    }

    /* Forward ring over Thread mesh (only if we originated it locally) */
    if(!fromThread)
    {
        Thread_SendRingMulticast();
    }
}

/* =========================================================================
 *  NotifyAnalogEvent  - called from DoorbellManager polling task
 * ========================================================================= */
void AppManager::NotifyAnalogEvent(bool pressed, uint16_t adcRaw)
{
    AppEvent event;
    event.Type                  = AppEvent::kEventType_Analog;
    event.AnalogEvent.State     = pressed ? kAnalogEvent_Pressed : kAnalogEvent_Released;
    event.AnalogEvent.AdcRaw    = adcRaw;
    event.Handler               = nullptr;
    GetAppTask().PostEvent(&event);
}

/* =========================================================================
 *  NotifyThreadEvent  - called from Thread callbacks
 * ========================================================================= */
void AppManager::NotifyThreadEvent(ThreadEventType_t threadEvent, uint32_t value)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Thread;
    event.ThreadEvent.Event  = threadEvent;
    event.ThreadEvent.Value  = value;
    event.Handler            = nullptr;
    GetAppTask().PostEvent(&event);
}

/* =========================================================================
 *  Thread_Init
 *
 *  Initialises the OpenThread instance.  If Thread credentials have been
 *  stored in NVM (via a previous BLE commissioning session), Thread join
 *  is started automatically.
 * ========================================================================= */
static void Thread_Init(void)
{
    /* Obtain the OpenThread instance - OT must already be initialised by the
     * Qorvo platform layer (called from gpBaseComps_StackInit or equivalent).
     * The exact Qorvo API to retrieve the instance may differ by SDK version;
     * common options are:
     *   sThreadInstance = otInstanceInit(...) - allocate new instance
     *   sThreadInstance = otInstanceInitSingle() - single-instance build */
    sThreadInstance = otInstanceInitSingle();
    if(sThreadInstance == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] otInstanceInitSingle failed!", 0);
        return;
    }

    /* Register state-change callback to track network attach/detach */
    otSetStateChangedCallback(sThreadInstance, Thread_StateChangeCallback, nullptr);

    /* Check if Thread dataset is already stored in NVM */
    otOperationalDataset dataset;
    if(otDatasetGetActive(sThreadInstance, &dataset) == OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Credentials found in NVM - starting Thread", 0);
        sThreadCredentialsAvailable = true;
        Thread_StartJoin();
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] No credentials - waiting for BLE commissioning", 0);
    }
}

/* =========================================================================
 *  Thread_StartJoin
 *
 *  Applies the current Thread config parameters (from BLE GATT or NVM),
 *  enables the IPv6 interface, and starts the Thread stack.
 * ========================================================================= */
static void Thread_StartJoin(void)
{
    if(sThreadInstance == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Cannot join - not initialised", 0);
        return;
    }

    otError err;

    /* Apply BLE-written network parameters if not already stored in NVM */
    if(!sThreadCredentialsAvailable)
    {
        uint16_t nameLen;
        uint8_t* pName = ThreadCfg_GetNetworkName(&nameLen);
        uint8_t* pKey  = ThreadCfg_GetNetworkKey();
        uint8_t  ch    = ThreadCfg_GetChannel();
        uint16_t panId = ThreadCfg_GetPanId();

        /* Build an operational dataset from the BLE-written values */
        otOperationalDataset dataset;
        memset(&dataset, 0, sizeof(dataset));

        /* Network Name */
        if(nameLen > OT_NETWORK_NAME_MAX_SIZE)
        {
            nameLen = OT_NETWORK_NAME_MAX_SIZE;
        }
        memcpy(dataset.mNetworkName.m8, pName, nameLen);
        dataset.mComponents.mIsNetworkNamePresent = true;

        /* Network Key */
        memcpy(dataset.mNetworkKey.m8, pKey, OT_NETWORK_KEY_SIZE);
        dataset.mComponents.mIsNetworkKeyPresent = true;

        /* Channel */
        dataset.mChannel = ch;
        dataset.mComponents.mIsChannelPresent = true;

        /* PAN ID */
        dataset.mPanId = panId;
        dataset.mComponents.mIsPanIdPresent = true;

        /* Active Timestamp */
        dataset.mActiveTimestamp.mSeconds = 1;
        dataset.mComponents.mIsActiveTimestampPresent = true;

        err = otDatasetSetActive(sThreadInstance, &dataset);
        if(err != OT_ERROR_NONE)
        {
            GP_LOG_SYSTEM_PRINTF("[Thread] SetActiveDataset failed: %d", 0, (int)err);
            return;
        }
    }

    /* Enable IPv6 */
    err = otIp6SetEnabled(sThreadInstance, true);
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] IPv6 enable failed: %d", 0, (int)err);
        return;
    }

    /* Start Thread */
    err = otThreadSetEnabled(sThreadInstance, true);
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] SetEnabled failed: %d", 0, (int)err);
        return;
    }

    /* Blink GREEN LED while joining */
    StatusLed_BlinkLed(LED_THREAD_STATE, THREAD_JOIN_BLINK_ON_MS, THREAD_JOIN_BLINK_OFF_MS);
    GP_LOG_SYSTEM_PRINTF("[Thread] Joining network...", 0);

    /* Open a UDP socket for ring event multicast */
    if(!sThreadUdpSocketOpen)
    {
        otSockAddr sockAddr;
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.mPort = THREAD_RING_PORT;

        err = otUdpOpen(sThreadInstance, &sThreadUdpSocket,
                        Thread_UdpReceiveCallback, nullptr);
        if(err == OT_ERROR_NONE)
        {
            err = otUdpBind(sThreadInstance, &sThreadUdpSocket, &sockAddr, OT_NETIF_THREAD_INTERNAL);
            if(err == OT_ERROR_NONE)
            {
                sThreadUdpSocketOpen = true;
                GP_LOG_SYSTEM_PRINTF("[Thread] UDP socket open on port %d", 0, THREAD_RING_PORT);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[Thread] UDP bind failed: %d", 0, (int)err);
            }
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[Thread] UDP open failed: %d", 0, (int)err);
        }
    }
}

/* =========================================================================
 *  Thread_SendRingMulticast
 *
 *  Sends a 1-byte UDP message (value 0x01 = ring) to the Thread realm-local
 *  all-nodes multicast address ff03::1, port THREAD_RING_PORT.
 *  All other doorbell devices on the same Thread network will receive it.
 * ========================================================================= */
static void Thread_SendRingMulticast(void)
{
    if(sThreadInstance == nullptr || !sThreadUdpSocketOpen)
    {
        return;
    }

    otDeviceRole role = otThreadGetDeviceRole(sThreadInstance);
    if(role == OT_DEVICE_ROLE_DISABLED || role == OT_DEVICE_ROLE_DETACHED)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Not attached - ring not sent to mesh", 0);
        return;
    }

    otMessage* msg = otUdpNewMessage(sThreadInstance, nullptr);
    if(msg == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] UDP alloc failed", 0);
        return;
    }

    /* Payload: 1 byte ring command */
    uint8_t payload = DOORBELL_STATE_RINGING;
    otError err = otMessageAppend(msg, &payload, sizeof(payload));
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Message append failed: %d", 0, (int)err);
        otMessageFree(msg);
        return;
    }

    /* Destination: ff03::1 (realm-local all-nodes multicast) */
    otMessageInfo msgInfo;
    memset(&msgInfo, 0, sizeof(msgInfo));
    msgInfo.mPeerPort = THREAD_RING_PORT;
    otIp6AddressFromString(THREAD_RING_MCAST, &msgInfo.mPeerAddr);

    err = otUdpSend(sThreadInstance, &sThreadUdpSocket, msg, &msgInfo);
    if(err == OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Ring multicast sent to %s", 0, THREAD_RING_MCAST);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Ring multicast send failed: %d", 0, (int)err);
        otMessageFree(msg);
    }
}

/* =========================================================================
 *  Thread_UdpReceiveCallback
 *
 *  Called by OpenThread when a UDP message arrives on the doorbell port.
 *  Byte 0 == 0x01 → ring event from another device.
 * ========================================================================= */
static void Thread_UdpReceiveCallback(void* /*aContext*/, otMessage* aMessage,
                                      const otMessageInfo* /*aMessageInfo*/)
{
    uint8_t payload = 0;
    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    if(len >= 1)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), &payload, 1);
    }

    if(payload == DOORBELL_STATE_RINGING)
    {
        AppManager::NotifyThreadEvent(kThreadEvent_RingReceived, 0);
    }
}

/* =========================================================================
 *  Thread_StateChangeCallback
 * ========================================================================= */
static void Thread_StateChangeCallback(uint32_t aFlags, void* /*aContext*/)
{
    if(aFlags & OT_CHANGED_THREAD_ROLE)
    {
        otDeviceRole role = otThreadGetDeviceRole(sThreadInstance);
        GP_LOG_SYSTEM_PRINTF("[Thread] Role changed: %d", 0, (int)role);

        if(role == OT_DEVICE_ROLE_CHILD  ||
           role == OT_DEVICE_ROLE_ROUTER ||
           role == OT_DEVICE_ROLE_LEADER)
        {
            AppManager::NotifyThreadEvent(kThreadEvent_Joined, (uint32_t)role);
        }
        else if(role == OT_DEVICE_ROLE_DETACHED)
        {
            AppManager::NotifyThreadEvent(kThreadEvent_Detached, 0);
        }
    }
}

/* =========================================================================
 *  BLE Callbacks
 * ========================================================================= */

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

static void BLE_CharacteristicRead_Callback(uint16_t /*connId*/, uint16_t /*handle*/,
                                             uint8_t /*op*/, uint16_t /*offset*/,
                                             BleIf_Attr_t* /*pAttr*/)
{
    /* Static attribute values are returned automatically by BleIf */
}

static void BLE_CharacteristicWrite_Callback(uint16_t /*connId*/, uint16_t handle,
                                              uint8_t /*op*/, uint16_t /*offset*/,
                                              uint16_t len, uint8_t* pValue,
                                              BleIf_Attr_t* /*pAttr*/)
{
    AppEvent event;

    if(handle == DOORBELL_RING_HDL)
    {
        /* Remote ring from phone */
        event.Type = AppEvent::kEventType_BleConnection;
        event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
        event.BleConnectionEvent.Value = (len > 0) ? pValue[0] : 0;
        GetAppTask().PostEvent(&event);
    }
    else if(handle == THREAD_JOIN_HDL)
    {
        /* Phone triggered Thread join */
        if(len > 0 && pValue[0] == 0x01)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Thread join command received", 0);
            sThreadCredentialsAvailable = false; /* use BLE-written values */
            Thread_StartJoin();
        }
    }
    else if(handle == THREAD_NET_NAME_HDL   ||
            handle == THREAD_NET_KEY_HDL    ||
            handle == THREAD_CHANNEL_HDL    ||
            handle == THREAD_PANID_HDL)
    {
        /* BleIf has already written the new value into the GATT attribute buffer.
         * No explicit action needed here; Thread_StartJoin() reads the values
         * from the GATT buffers via the ThreadCfg_Get*() accessors. */
        GP_LOG_SYSTEM_PRINTF("[BLE] Thread config parameter updated (handle 0x%04X)", 0, handle);
    }
}

static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    if(event->handle == DOORBELL_RING_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell Ring notifications ENABLED", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Doorbell Ring notifications disabled", 0);
        }
    }
    else if(event->handle == THREAD_STATUS_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Thread Status notifications ENABLED", 0);
            /* Send current status immediately */
            uint8_t status = ThreadCfg_GetStatus();
            BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
        }
    }
}
