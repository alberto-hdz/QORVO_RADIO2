/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-1
 */

/** @file "AppManager.cpp"
 *
 * Application logic for the QPG6200 Thread+BLE MaxSonar Motion Detector.
 *
 * Board: QPG6200L Development Kit
 *
 * -- Behaviour ---------------------------------------------------------------
 *  Boot:
 *    BLE advertising starts automatically ("QPG MaxSonar Motion")
 *    If Thread credentials are stored in NVM, Thread join is attempted.
 *
 *  BLE connection state (WHITE_COOL LED):
 *    Blinking  = advertising
 *    Solid ON  = phone connected
 *    OFF       = disconnected
 *
 *  Thread network state (GREEN LED):
 *    Blinking  = joining
 *    Solid ON  = attached
 *    OFF       = detached
 *
 *  Motion state (BLUE LED):
 *    Solid ON  = motion detected within 2 m
 *    OFF       = no motion / object beyond 2 m
 *
 * -- Thread Motion Payload (UDP, port 5683) ----------------------------------
 *  4 bytes: [0x01=type] [0x00/0x01=state] [dist_hi] [dist_lo]
 *
 * -- BLE Commissioning -------------------------------------------------------
 *  1. Connect to "QPG MaxSonar Motion"
 *  2. Write Thread Network Name (up to 16 bytes)
 *  3. Write Thread Network Key  (16-byte master key)
 *  4. Optionally write Channel and PAN ID
 *  5. Write 0x01 to Join
 *  6. Subscribe to Thread Status notifications
 */

#include "AppManager.h"
#include "AppButtons.h"
#include "AppTask.h"
#include "MotionDetector_Config.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "StatusLed.h"
#include "BleIf.h"

#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

extern "C" void otSysInit(int argc, char* argv[]);

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define ADV_BLINK_ON_MS          500
#define ADV_BLINK_OFF_MS         500
#define THREAD_JOIN_BLINK_ON_MS  200
#define THREAD_JOIN_BLINK_OFF_MS 200

#define BTN_RESTART_ADV_THRESHOLD   2
#define BTN_FACTORY_RESET_THRESHOLD 5

#define THREAD_MOTION_PORT  5683
#define THREAD_MOTION_MCAST "ff03::1"

#define THREAD_MSG_TYPE_MOTION  0x01

#define LED_BLE_STATE    0
#define LED_THREAD_STATE 1
#define LED_MOTION       2

AppManager AppManager::sAppMgr;

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;
static BleIf_Callbacks_t sAppCallbacks;

static bool         sThreadCredentialsAvailable = false;
static otInstance*  sThreadInstance             = nullptr;
static otUdpSocket  sThreadUdpSocket;
static bool         sThreadUdpSocketOpen        = false;

static void BLE_Stack_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_CharacteristicRead_Callback(uint16_t connId, uint16_t handle, uint8_t op,
                                            uint16_t offset, BleIf_Attr_t* pAttr);
static void BLE_CharacteristicWrite_Callback(uint16_t connId, uint16_t handle, uint8_t op,
                                             uint16_t offset, uint16_t len, uint8_t* pValue,
                                             BleIf_Attr_t* pAttr);
static void BLE_CCCD_Callback(BleIf_AttsCccEvt_t* event);

static void Thread_Init(void);
static void Thread_StartJoin(void);
static void Thread_SendMotionPacket(bool detected, uint16_t distanceCm);
static void Thread_UdpReceiveCallback(void* aContext, otMessage* aMessage,
                                      const otMessageInfo* aMessageInfo);
static void Thread_StateChangeCallback(uint32_t aFlags, void* aContext);

extern "C" uint8_t* ThreadCfg_GetNetworkName(uint16_t* pLen);
extern "C" uint8_t* ThreadCfg_GetNetworkKey(void);
extern "C" uint8_t  ThreadCfg_GetChannel(void);
extern "C" uint16_t ThreadCfg_GetPanId(void);
extern "C" void     ThreadCfg_SetStatus(uint8_t status);
extern "C" uint8_t  ThreadCfg_GetStatus(void);

void AppManager::Init()
{
    sAppCallbacks.stackCallback    = BLE_Stack_Callback;
    sAppCallbacks.chrReadCallback  = BLE_CharacteristicRead_Callback;
    sAppCallbacks.chrWriteCallback = BLE_CharacteristicWrite_Callback;
    sAppCallbacks.cccCallback      = BLE_CCCD_Callback;
    BleIf_Init(&sAppCallbacks);

    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    StatusLed_SetLed(LED_BLE_STATE,    false);
    StatusLed_SetLed(LED_THREAD_STATE, false);
    StatusLed_SetLed(LED_MOTION,       false);

    GetAppButtons().RegisterMultiFunc(APP_MULTI_FUNC_BUTTON);

    Thread_Init();

    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("  QPG6200 THREAD+BLE MAXSONAR MOTION DET.", 0);
    GP_LOG_SYSTEM_PRINTF("============================================", 0);
    GP_LOG_SYSTEM_PRINTF("Sensor : LV-MaxSonar EZ (UART 9600 baud)", 0);
    GP_LOG_SYSTEM_PRINTF("Range  : detect within %u cm", 0, (unsigned)200);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE blinks  = BLE advertising", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE solid   = BLE connected", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN blinks  = Thread joining", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN solid   = Thread attached", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE  solid   = Motion detected", 0);
    GP_LOG_SYSTEM_PRINTF("  BLUE  off     = No motion", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    if(BleIf_StartAdvertising() == STATUS_NO_ERROR)
    {
        GP_LOG_SYSTEM_PRINTF("[BLE] Advertising - scan for 'QPG MaxSonar Motion'", 0);
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
            if(aEvent->BleConnectionEvent.Value == MOTION_STATE_DETECTED)
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Motion override: DETECTED (from phone)", 0);
                ReportMotion(true, 0, false);
            }
            else
            {
                GP_LOG_SYSTEM_PRINTF("[BLE] Motion override: CLEARED (from phone)", 0);
                StatusLed_SetLed(LED_MOTION, false);
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

    if(aEvent->ButtonEvent.State == ButtonEvent_t::kButtonState_Released)
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

void AppManager::SensorEventHandler(AppEvent* aEvent)
{
    bool detected   = (aEvent->SensorEvent.State == kSensorEvent_MotionDetected);
    uint16_t distCm = aEvent->SensorEvent.DistanceCm;

    GP_LOG_SYSTEM_PRINTF("[Sensor] %s @ %u cm", 0,
                         detected ? "MOTION DETECTED" : "motion cleared",
                         (unsigned)distCm);

    ReportMotion(detected, distCm, false);
}

void AppManager::ThreadEventHandler(AppEvent* aEvent)
{
    switch(aEvent->ThreadEvent.Event)
    {
        case kThreadEvent_Joined:
            GP_LOG_SYSTEM_PRINTF("[Thread] Attached (role=%d)", 0, aEvent->ThreadEvent.Value);
            StatusLed_SetLed(LED_THREAD_STATE, true);
            ThreadCfg_SetStatus((uint8_t)aEvent->ThreadEvent.Value);
            {
                uint8_t status = ThreadCfg_GetStatus();
                BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
            }
            break;

        case kThreadEvent_Detached:
            GP_LOG_SYSTEM_PRINTF("[Thread] Detached", 0);
            StatusLed_SetLed(LED_THREAD_STATE, false);
            ThreadCfg_SetStatus(THREAD_STATUS_DETACHED);
            {
                uint8_t status = THREAD_STATUS_DETACHED;
                BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
            }
            break;

        case kThreadEvent_MotionReceived:
            {
                bool detected = (aEvent->ThreadEvent.Value & 0xFF) != 0;
                uint16_t dist = (uint16_t)(aEvent->ThreadEvent.Value >> 16);
                GP_LOG_SYSTEM_PRINTF("[Thread] Motion event from mesh: %s @ %u cm", 0,
                                     detected ? "detected" : "cleared", (unsigned)dist);
                ReportMotion(detected, dist, true);
            }
            break;

        case kThreadEvent_Error:
            GP_LOG_SYSTEM_PRINTF("[Thread] Error: 0x%x", 0, aEvent->ThreadEvent.Value);
            break;

        default:
            break;
    }
}

void AppManager::ReportMotion(bool detected, uint16_t distanceCm, bool fromThread)
{
    StatusLed_SetLed(LED_MOTION, detected);

    uint8_t motionState = detected ? MOTION_STATE_DETECTED : MOTION_STATE_NONE;
    BleIf_SendNotification(MOTION_STATUS_HDL, 1, &motionState);

    uint8_t distBuf[2] = {(uint8_t)(distanceCm >> 8), (uint8_t)(distanceCm & 0xFF)};
    BleIf_SendNotification(MOTION_DIST_HDL, 2, distBuf);

    if(!fromThread)
    {
        Thread_SendMotionPacket(detected, distanceCm);
    }
}

void AppManager::NotifySensorEvent(bool motionDetected, uint16_t distanceCm)
{
    AppEvent event;
    event.Type                   = AppEvent::kEventType_Sensor;
    event.SensorEvent.State      = motionDetected ? kSensorEvent_MotionDetected
                                                  : kSensorEvent_MotionCleared;
    event.SensorEvent.DistanceCm = distanceCm;
    event.Handler                = nullptr;
    GetAppTask().PostEvent(&event);
}

void AppManager::NotifyThreadEvent(ThreadEventType_t threadEvent, uint32_t value)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Thread;
    event.ThreadEvent.Event  = threadEvent;
    event.ThreadEvent.Value  = value;
    event.Handler            = nullptr;
    GetAppTask().PostEvent(&event);
}

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

static void Thread_StartJoin(void)
{
    if(sThreadInstance == nullptr)
    {
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
            err = otUdpBind(sThreadInstance, &sThreadUdpSocket, &sockAddr,
                            OT_NETIF_THREAD_INTERNAL);
            if(err == OT_ERROR_NONE)
            {
                sThreadUdpSocketOpen = true;
                GP_LOG_SYSTEM_PRINTF("[Thread] UDP socket open on port %d", 0,
                                     THREAD_MOTION_PORT);
            }
        }
    }
}

static void Thread_SendMotionPacket(bool detected, uint16_t distanceCm)
{
    if(sThreadInstance == nullptr || !sThreadUdpSocketOpen)
    {
        return;
    }

    otDeviceRole role = otThreadGetDeviceRole(sThreadInstance);
    if(role == OT_DEVICE_ROLE_DISABLED || role == OT_DEVICE_ROLE_DETACHED)
    {
        return;
    }

    otMessage* msg = otUdpNewMessage(sThreadInstance, nullptr);
    if(msg == nullptr)
    {
        return;
    }

    uint8_t payload[4] = {
        THREAD_MSG_TYPE_MOTION,
        detected ? (uint8_t)0x01 : (uint8_t)0x00,
        (uint8_t)(distanceCm >> 8),
        (uint8_t)(distanceCm & 0xFF),
    };

    otError err = otMessageAppend(msg, payload, sizeof(payload));
    if(err != OT_ERROR_NONE)
    {
        otMessageFree(msg);
        return;
    }

    otMessageInfo msgInfo;
    memset(&msgInfo, 0, sizeof(msgInfo));
    msgInfo.mPeerPort = THREAD_MOTION_PORT;
    otIp6AddressFromString(THREAD_MOTION_MCAST, &msgInfo.mPeerAddr);

    err = otUdpSend(sThreadInstance, &sThreadUdpSocket, msg, &msgInfo);
    if(err != OT_ERROR_NONE)
    {
        otMessageFree(msg);
    }
}

static void Thread_UdpReceiveCallback(void* /*aContext*/, otMessage* aMessage,
                                      const otMessageInfo* /*aMessageInfo*/)
{
    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    if(len < 4)
    {
        return;
    }

    uint8_t payload[4] = {0};
    otMessageRead(aMessage, otMessageGetOffset(aMessage), payload, sizeof(payload));

    if(payload[0] != THREAD_MSG_TYPE_MOTION)
    {
        return;
    }

    bool     detected = (payload[1] != 0);
    uint16_t distCm   = ((uint16_t)payload[2] << 8) | payload[3];
    uint32_t value    = ((uint32_t)distCm << 16) | (detected ? 1u : 0u);
    AppManager::NotifyThreadEvent(kThreadEvent_MotionReceived, value);
}

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
        GP_LOG_SYSTEM_PRINTF("[BLE] Thread config updated (handle 0x%04X)", 0, handle);
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
    }
    else if(event->handle == MOTION_DIST_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            GP_LOG_SYSTEM_PRINTF("[BLE] Distance notifications ENABLED", 0);
        }
    }
    else if(event->handle == THREAD_STATUS_CCC_HDL)
    {
        if(event->value & ATT_CLIENT_CFG_NOTIFY)
        {
            uint8_t status = ThreadCfg_GetStatus();
            BleIf_SendNotification(THREAD_STATUS_HDL, 1, &status);
        }
    }
}
