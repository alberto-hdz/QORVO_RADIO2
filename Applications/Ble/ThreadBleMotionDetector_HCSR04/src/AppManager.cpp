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
 * Application logic for the QPG6200 Thread+BLE HC-SR04 Motion Detector.
 *
 * Board: QPG6200L Development Kit (standalone - no carrier board required)
 *
 * -- Behaviour ---------------------------------------------------------------
 *  Boot:
 *    BLE advertising starts automatically ("QPG HC-SR04 Motion")
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
 *  Motion event (BLUE LED):
 *    Solid ON  = motion detected (object within 2 m)
 *    OFF       = no motion
 *
 * -- Commissioning via BLE ---------------------------------------------------
 *  1. Connect to "QPG HC-SR04 Motion" with nRF Connect or Qorvo Connect.
 *  2. Write Thread Network Name  (e.g. "MotionNet").
 *  3. Write Thread Network Key   (16-byte master key - write-only).
 *  4. Optionally write Channel   (1 byte, 11-26, default 15).
 *  5. Optionally write PAN ID    (2 bytes LE, default 0xABCD).
 *  6. Write 0x01 to Join         -> device attempts to join Thread network.
 *  7. Subscribe to Thread Status  -> watch device role changes (0=disabled ... 4=leader).
 *
 * -- Buttons -----------------------------------------------------------------
 *  PB1 short press  (<2 s)  : restart BLE advertising
 *  PB1 long press   (>=5 s) : factory-reset Thread credentials and reboot
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "MotionDetector_Config.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"

/* OpenThread headers */
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

extern "C" void otSysInit(int argc, char* argv[]);

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------
 * LED timing constants
 * ------------------------------------------------------------------------- */
#define ADV_BLINK_ON_MS          500
#define ADV_BLINK_OFF_MS         500
#define THREAD_JOIN_BLINK_ON_MS  200
#define THREAD_JOIN_BLINK_OFF_MS 200

/* -------------------------------------------------------------------------
 * Button thresholds (seconds held)
 * ------------------------------------------------------------------------- */
#define BTN_RESTART_ADV_THRESHOLD   2   /**< seconds to hold PB1 to restart BLE adv */
#define BTN_FACTORY_RESET_THRESHOLD 5   /**< seconds to hold PB1 for Thread factory reset */

/* -------------------------------------------------------------------------
 * Thread motion multicast address (all Thread nodes in the local network)
 * ff03::1 = Thread realm-local all-nodes multicast
 * ------------------------------------------------------------------------- */
#define THREAD_MOTION_PORT   5683   /**< CoAP default port (reused for simplicity) */
#define THREAD_MOTION_MCAST  "ff03::1"

/* LED indices (must match QPINCFG_STATUS_LED order in qPinCfg.h):
 *   0 = WHITE_COOL (BLE state)
 *   1 = GREEN      (Thread state)
 *   2 = BLUE       (motion indicator) */
#define LED_BLE_STATE    0
#define LED_THREAD_STATE 1
#define LED_MOTION       2

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
static void Thread_SendMotionMulticast(bool detected, uint16_t distanceCm);
static void Thread_UdpReceiveCallback(void* aContext, otMessage* aMessage,
                                      const otMessageInfo* aMessageInfo);
static void Thread_StateChangeCallback(uint32_t aFlags, void* aContext);

/* -------------------------------------------------------------------------
 * Helper: accessor for config values defined in MotionDetector_Config.c
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
    StatusLed_SetLed(LED_MOTION,       false);

    /* --- Button --------------------------------------------------------- */
    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    /* --- Wait for BLE stack reset to complete ----------------------------
     * BleIf_Init() calls DmDevReset() which asynchronously resets the BLE
     * radio controller.  Thread_Init() calls otSysInit() which also
     * accesses the radio hardware.  Starting both at the same time causes
     * a hard fault / watchdog reset on the shared radio, so we must wait
     * for the BLE reset to finish (DM_RESET_CMPL_IND) before initialising
     * the OpenThread radio. */
    {
        uint32_t waitMs = 0;
        while(!BleIf_IsReady() && waitMs < 3000)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            waitMs += 10;
        }
        if(waitMs >= 3000)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] WARNING: BLE stack reset did not complete in time", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Stack ready (%lu ms)", 0, waitMs);
        }
    }

    /* --- Thread --------------------------------------------------------- */
    Thread_Init();

    /* --- Banner --------------------------------------------------------- */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("  QPG6200 THREAD+BLE HC-SR04 MOTION DET.", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("Board  : QPG6200L Development Kit", 0);
    GP_LOG_SYSTEM_PRINTF("Sensor : HC-SR04 (GPIO Trig/Echo)", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE blinks  = BLE advertising", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE solid   = BLE connected", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN blinks  = Thread joining", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN solid   = Thread attached", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE  solid   = Motion detected", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Thread Commissioning (via BLE) ---", 0);
    GP_LOG_SYSTEM_PRINTF("  1. Connect to 'QPG HC-SR04 Motion'", 0);
    GP_LOG_SYSTEM_PRINTF("  2. Write Thread Network Name (16 bytes)", 0);
    GP_LOG_SYSTEM_PRINTF("  3. Write Thread Network Key  (16 bytes)", 0);
    GP_LOG_SYSTEM_PRINTF("  4. Write Channel  (1 byte, 11-26)", 0);
    GP_LOG_SYSTEM_PRINTF("  5. Write PAN ID   (2 bytes LE)", 0);
    GP_LOG_SYSTEM_PRINTF("  6. Write 0x01 to Join characteristic", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Motion Detection ---", 0);
    GP_LOG_SYSTEM_PRINTF("  HC-SR04 Trig=GPIO28, Echo=GPIO29", 0);
    GP_LOG_SYSTEM_PRINTF("  Threshold: 200 cm (2 m)", 0);
    GP_LOG_SYSTEM_PRINTF("  Hold PB1 5s = factory reset Thread creds", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Start BLE advertising */
    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising - scan for 'QPG HC-SR04 Motion'", 0);
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
        case AppEvent::kEventType_Sensor:
            SensorEventHandler(aEvent);
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
            /* Phone wrote to Motion Status characteristic */
            if(aEvent->BleConnectionEvent.Value == MOTION_STATE_DETECTED)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Remote motion set by phone", 0);
                ReportMotion(true, 0, false);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Motion cleared by phone", 0);
                StatusLed_SetLed(LED_MOTION, false);
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
 *  SensorEventHandler  - HC-SR04 distance result
 * ========================================================================= */
void AppManager::SensorEventHandler(AppEvent* aEvent)
{
    bool     detected   = (aEvent->SensorEvent.State == kSensorEvent_MotionDetected);
    uint16_t distanceCm = aEvent->SensorEvent.DistanceCm;

    ReportMotion(detected, distanceCm, false /* fromThread */);
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

        case kThreadEvent_MotionReceived:
        {
            bool     detected   = (aEvent->ThreadEvent.Value >> 16) & 0x01;
            uint16_t distCm     = (uint16_t)(aEvent->ThreadEvent.Value & 0xFFFF);
            GP_LOG_SYSTEM_PRINTF("[Thread] Motion event received from mesh (det=%d dist=%u)", 0,
                                 (int)detected, (unsigned)distCm);
            ReportMotion(detected, distCm, true /* fromThread */);
            break;
        }

        case kThreadEvent_Error:
            GP_LOG_SYSTEM_PRINTF("[Thread] Error: 0x%x", 0, aEvent->ThreadEvent.Value);
            break;

        default:
            break;
    }
}

/* =========================================================================
 *  ReportMotion  - LED + BLE notification + Thread multicast
 * ========================================================================= */
void AppManager::ReportMotion(bool detected, uint16_t distanceCm, bool fromThread)
{
    /* Update BLUE motion LED */
    StatusLed_SetLed(LED_MOTION, detected);

    if(detected)
    {
        GP_LOG_SYSTEM_PRINTF("[Motion] DETECTED  dist=%u cm", 0, (unsigned)distanceCm);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[Motion] CLEARED   dist=%u cm", 0, (unsigned)distanceCm);
    }

    /* BLE notification: Motion Status (1 byte) */
    uint8_t motionValue = detected ? MOTION_STATE_DETECTED : MOTION_STATE_CLEAR;
    BleIf_SendNotification(MOTION_STATUS_HDL, 1, &motionValue);

    /* BLE notification: Distance (2 bytes big-endian) */
    uint8_t distBytes[2] = { (uint8_t)(distanceCm >> 8), (uint8_t)(distanceCm & 0xFF) };
    BleIf_SendNotification(MOTION_DIST_HDL, 2, distBytes);

    /* Forward over Thread mesh (only if we originated it locally) */
    if(!fromThread)
    {
        Thread_SendMotionMulticast(detected, distanceCm);
    }
}

/* =========================================================================
 *  NotifySensorEvent  - called from SensorManager task
 * ========================================================================= */
void AppManager::NotifySensorEvent(bool motionDetected, uint16_t distanceCm)
{
    AppEvent event;
    event.Type                     = AppEvent::kEventType_Sensor;
    event.SensorEvent.State        = motionDetected ? kSensorEvent_MotionDetected
                                                    : kSensorEvent_MotionCleared;
    event.SensorEvent.DistanceCm   = distanceCm;
    event.Handler                  = nullptr;
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
 * ========================================================================= */
static void Thread_Init(void)
{
    otSysInit(0, nullptr);

    sThreadInstance = otInstanceInitSingle();
    if(sThreadInstance == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] otInstanceInitSingle failed!", 0);
        return;
    }

    otSetStateChangedCallback(sThreadInstance, Thread_StateChangeCallback, nullptr);

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
 * ========================================================================= */
static void Thread_StartJoin(void)
{
    if(sThreadInstance == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Cannot join - not initialised", 0);
        return;
    }

    otError err;

    if(!sThreadCredentialsAvailable)
    {
        uint16_t nameLen;
        uint8_t* pName = ThreadCfg_GetNetworkName(&nameLen);
        uint8_t* pKey  = ThreadCfg_GetNetworkKey();
        uint8_t  ch    = ThreadCfg_GetChannel();
        uint16_t panId = ThreadCfg_GetPanId();

        otOperationalDataset dataset;
        memset(&dataset, 0, sizeof(dataset));

        if(nameLen > OT_NETWORK_NAME_MAX_SIZE)
        {
            nameLen = OT_NETWORK_NAME_MAX_SIZE;
        }
        memcpy(dataset.mNetworkName.m8, pName, nameLen);
        dataset.mComponents.mIsNetworkNamePresent = true;

        memcpy(dataset.mNetworkKey.m8, pKey, OT_NETWORK_KEY_SIZE);
        dataset.mComponents.mIsNetworkKeyPresent = true;

        dataset.mChannel = ch;
        dataset.mComponents.mIsChannelPresent = true;

        dataset.mPanId = panId;
        dataset.mComponents.mIsPanIdPresent = true;

        dataset.mActiveTimestamp.mSeconds = 1;
        dataset.mComponents.mIsActiveTimestampPresent = true;

        err = otDatasetSetActive(sThreadInstance, &dataset);
        if(err != OT_ERROR_NONE)
        {
            GP_LOG_SYSTEM_PRINTF("[Thread] SetActiveDataset failed: %d", 0, (int)err);
            return;
        }
    }

    err = otIp6SetEnabled(sThreadInstance, true);
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] IPv6 enable failed: %d", 0, (int)err);
        return;
    }

    err = otThreadSetEnabled(sThreadInstance, true);
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] SetEnabled failed: %d", 0, (int)err);
        return;
    }

    StatusLed_BlinkLed(LED_THREAD_STATE, THREAD_JOIN_BLINK_ON_MS, THREAD_JOIN_BLINK_OFF_MS);
    GP_LOG_SYSTEM_PRINTF("[Thread] Joining network...", 0);

    if(!sThreadUdpSocketOpen)
    {
        otSockAddr sockAddr;
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.mPort = THREAD_MOTION_PORT;

        err = otUdpOpen(sThreadInstance, &sThreadUdpSocket,
                        Thread_UdpReceiveCallback, nullptr);
        if(err == OT_ERROR_NONE)
        {
            err = otUdpBind(sThreadInstance, &sThreadUdpSocket, &sockAddr, OT_NETIF_THREAD_INTERNAL);
            if(err == OT_ERROR_NONE)
            {
                sThreadUdpSocketOpen = true;
                GP_LOG_SYSTEM_PRINTF("[Thread] UDP socket open on port %d", 0, THREAD_MOTION_PORT);
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
 *  Thread_SendMotionMulticast
 *
 *  Payload: 4 bytes
 *    [0] = 0x01 (type: motion event)
 *    [1] = 0x00 (clear) or 0x01 (detected)
 *    [2] = distance high byte
 *    [3] = distance low byte
 * ========================================================================= */
static void Thread_SendMotionMulticast(bool detected, uint16_t distanceCm)
{
    if(sThreadInstance == nullptr || !sThreadUdpSocketOpen)
    {
        return;
    }

    otDeviceRole role = otThreadGetDeviceRole(sThreadInstance);
    if(role == OT_DEVICE_ROLE_DISABLED || role == OT_DEVICE_ROLE_DETACHED)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Not attached - motion event not sent to mesh", 0);
        return;
    }

    otMessage* msg = otUdpNewMessage(sThreadInstance, nullptr);
    if(msg == nullptr)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] UDP alloc failed", 0);
        return;
    }

    uint8_t payload[4] = {
        0x01,                               /* type: motion event */
        detected ? 0x01u : 0x00u,           /* state */
        (uint8_t)(distanceCm >> 8),         /* distance high byte */
        (uint8_t)(distanceCm & 0xFF)        /* distance low byte */
    };

    otError err = otMessageAppend(msg, payload, sizeof(payload));
    if(err != OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Message append failed: %d", 0, (int)err);
        otMessageFree(msg);
        return;
    }

    otMessageInfo msgInfo;
    memset(&msgInfo, 0, sizeof(msgInfo));
    msgInfo.mPeerPort = THREAD_MOTION_PORT;
    otIp6AddressFromString(THREAD_MOTION_MCAST, &msgInfo.mPeerAddr);

    err = otUdpSend(sThreadInstance, &sThreadUdpSocket, msg, &msgInfo);
    if(err == OT_ERROR_NONE)
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Motion multicast sent (det=%d dist=%u)", 0,
                             (int)detected, (unsigned)distanceCm);
    }
    else
    {
        GP_LOG_SYSTEM_PRINTF("[Thread] Motion multicast send failed: %d", 0, (int)err);
        otMessageFree(msg);
    }
}

/* =========================================================================
 *  Thread_UdpReceiveCallback
 *
 *  Parses the 4-byte motion payload and posts a Thread event.
 * ========================================================================= */
static void Thread_UdpReceiveCallback(void* /*aContext*/, otMessage* aMessage,
                                      const otMessageInfo* /*aMessageInfo*/)
{
    uint8_t  payload[4] = {0};
    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    if(len >= 4)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), payload, 4);
    }
    else if(len >= 1)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), payload, (uint16_t)len);
    }

    if(payload[0] == 0x01)
    {
        bool     detected   = (payload[1] != 0);
        uint16_t distanceCm = (uint16_t)((payload[2] << 8) | payload[3]);
        /* Pack detected flag and distance into the 32-bit value field */
        uint32_t value = ((uint32_t)(detected ? 1u : 0u) << 16) | distanceCm;
        AppManager::NotifyThreadEvent(kThreadEvent_MotionReceived, value);
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

    if(handle == MOTION_STATUS_HDL)
    {
        event.Type = AppEvent::kEventType_BleConnection;
        event.BleConnectionEvent.Event = Ble_Event_t::kBleLedControlCharUpdate;
        event.BleConnectionEvent.Value = (len > 0) ? pValue[0] : 0;
        GetAppTask().PostEvent(&event);
    }
    else if(handle == THREAD_JOIN_HDL)
    {
        if(len > 0 && pValue[0] == 0x01)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Thread join command received", 0);
            sThreadCredentialsAvailable = false;
            Thread_StartJoin();
        }
    }
    else if(handle == THREAD_NET_NAME_HDL ||
            handle == THREAD_NET_KEY_HDL  ||
            handle == THREAD_CHANNEL_HDL  ||
            handle == THREAD_PANID_HDL)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Thread config parameter updated (handle 0x%04X)", 0, handle);
    }
}

static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event)
{
    if(event->handle == MOTION_STATUS_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Motion Status notifications ENABLED", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Motion Status notifications disabled", 0);
        }
    }
    else if(event->handle == MOTION_DIST_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Distance notifications ENABLED", 0);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Distance notifications disabled", 0);
        }
    }
    else if(event->handle == THREAD_STATUS_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Thread Status notifications ENABLED", 0);
            uint8_t status = ThreadCfg_GetStatus();
            BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
        }
    }
}
