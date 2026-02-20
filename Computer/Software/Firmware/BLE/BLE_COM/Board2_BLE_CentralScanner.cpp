// File: ble_central/main.c
// This is a simplified BLE central that scans and connects
#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"
#include "qPinCfg.h"
#include "BleIf.h"
#include "FreeRTOS.h"
#include "task.h"
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
// Target device name to look for
#define TARGET_DEVICE_NAME "qBLE peripheral"
// State machine
typedef enum {
 BLE_STATE_IDLE,
 BLE_STATE_SCANNING,
 BLE_STATE_CONNECTING,
 BLE_STATE_CONNECTED,
 BLE_STATE_DISCOVERING,
 BLE_STATE_READY
} BleState_t;
static BleState_t bleState = BLE_STATE_IDLE;
static uint16_t connectionHandle = 0;
// Callbacks
static void BLE_Central_Callback(BleIf_MsgHdr_t* pMsg);
static void BLE_ScanResult_Callback(BleIf_ScanResult_t* pResult);
static BleIf_Callbacks_t centralCallbacks = {
 .stackCallback = BLE_Central_Callback,
 .scanResultCallback = BLE_ScanResult_Callback,
 .chrReadCallback = NULL,
 .chrWriteCallback = NULL,
 .cccCallback = NULL
};
static void BLE_Central_Callback(BleIf_MsgHdr_t* pMsg)
{
 switch(pMsg->event)
 {
 case BLEIF_DM_SCAN_START_IND:
 GP_LOG_SYSTEM_PRINTF("Scanning started...", 0);
 bleState = BLE_STATE_SCANNING;
 break;
 
 case BLEIF_DM_SCAN_STOP_IND:
 GP_LOG_SYSTEM_PRINTF("Scanning stopped", 0);
 break;
 
 case BLEIF_DM_CONN_OPEN_IND:
 GP_LOG_SYSTEM_PRINTF("Connected to peripheral!", 0);
 connectionHandle = pMsg->param.connOpen.handle;
 bleState = BLE_STATE_CONNECTED;
 
 // Start service discovery
 GP_LOG_SYSTEM_PRINTF("Starting service discovery...", 0);
 BleIf_DiscoverServices(connectionHandle);
 bleState = BLE_STATE_DISCOVERING;
 break;
 
 case BLEIF_DM_CONN_CLOSE_IND:
 GP_LOG_SYSTEM_PRINTF("Disconnected from peripheral", 0);
 bleState = BLE_STATE_IDLE;
 connectionHandle = 0;
 
 // Restart scanning
 vTaskDelay(1000);
 BleIf_StartScanning();
 break;
 
 case BLEIF_ATT_DISC_SVC_CMPL:
 GP_LOG_SYSTEM_PRINTF("Service discovery complete!", 0);
 bleState = BLE_STATE_READY;
 break;
 
 default:
 GP_LOG_SYSTEM_PRINTF("BLE event: %d", 0, pMsg->event);
 break;
 }
}
static void BLE_ScanResult_Callback(BleIf_ScanResult_t* pResult)
{
 // Parse advertising data for device name
 char deviceName[32] = {0};
 
 // Look for complete local name in advertising data
 uint8_t* advData = pResult->pData;
 uint8_t advLen = pResult->len;
 uint8_t i = 0;
 
 while (i < advLen)
 {
 uint8_t fieldLen = advData[i];
 uint8_t fieldType = advData[i + 1];
 
 if (fieldType == 0x09) // Complete Local Name
 {
 uint8_t nameLen = fieldLen - 1;
 if (nameLen > sizeof(deviceName) - 1)
 nameLen = sizeof(deviceName) - 1;
 memcpy(deviceName, &advData[i + 2], nameLen);
 deviceName[nameLen] = '\0';
 break;
 }
 
 i += fieldLen + 1;
 }
 
 GP_LOG_SYSTEM_PRINTF("Found device: %s (RSSI: %d)", 0, 
 deviceName[0] ? deviceName : "(unnamed)",
 pResult->rssi);
 
 // Check if this is our target device
 if (strstr(deviceName, TARGET_DEVICE_NAME) != NULL)
 {
 GP_LOG_SYSTEM_PRINTF("*** TARGET DEVICE FOUND! ***", 0);
 
 // Stop scanning
 BleIf_StopScanning();
 
 // Connect to the device
 GP_LOG_SYSTEM_PRINTF("Connecting to %02X:%02X:%02X:%02X:%02X:%02X", 0,
 pResult->addr[5], pResult->addr[4], pResult->addr[3],
 pResult->addr[2], pResult->addr[1], pResult->addr[0]);
 
 bleState = BLE_STATE_CONNECTING;
 BleIf_Connect(pResult->addr, pResult->addrType);
 }
}
// Write to LED characteristic on peripheral
static void WriteLedState(uint8_t state)
{
 if (bleState != BLE_STATE_READY)
 {
 GP_LOG_SYSTEM_PRINTF("Not ready to write (state=%d)", 0, bleState);
 return;
 }
 
 // LED Control handle (must match peripheral's GATT table)
 uint16_t ledControlHandle = 0x0015; // Adjust based on peripheral's handles
 
 GP_LOG_SYSTEM_PRINTF("Writing LED state: %d", 0, state);
 BleIf_WriteCharacteristic(connectionHandle, ledControlHandle, &state, 1);
}
static void central_Task(void* pvParameters)
{
 uint8_t ledState = 0;
 uint32_t toggleCount = 0;
 
 // Wait for BLE stack to initialize
 vTaskDelay(2000);
 
 GP_LOG_SYSTEM_PRINTF("Starting BLE scan...", 0);
 BleIf_StartScanning();
 
 while(1)
 {
 if (bleState == BLE_STATE_READY)
 {
 // Toggle LED on peripheral every 2 seconds
 if (toggleCount % 20 == 0) // Every 2 seconds (20 * 100ms)
 {
 ledState = !ledState;
 WriteLedState(ledState);
 GP_LOG_SYSTEM_PRINTF("Toggled remote LED to: %d", 0, ledState);
 
 // Also toggle local LED to show sync
 HAL_LED_TGL_GRN();
 }
 toggleCount++;
 }
 
 vTaskDelay(100); // 100ms loop
 }
}
void Application_Init(void)
{
 gpHal_Set32kHzCrystalAvailable(false);
 gpBaseComps_StackInit();
 gpCom_Init();
 gpLog_Init();
 
 qResult_t res = qPinCfg_Init(NULL);
 if(res != Q_OK)
 {
 GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
 Q_ASSERT(false);
 }
 
 // Initialize BLE as Central
 BleIf_InitCentral(&centralCallbacks);
 
 // Create central task
 static StaticTask_t xCentralTaskPCB;
 static StackType_t xCentralTaskStack[4096];
 
 xTaskCreateStatic(
 central_Task,
 "central_Task",
 4096,
 NULL,
 tskIDLE_PRIORITY + 1,
 xCentralTaskStack,
 &xCentralTaskPCB
 );
 
 GP_LOG_SYSTEM_PRINTF("=== BOARD 2: BLE CENTRAL ===", 0);
}
int main(void)
{
 HAL_INITIALIZE_GLOBAL_INT();
 HAL_INIT();
 HAL_ENABLE_GLOBAL_INT();
 gpSched_Init();
 gpSched_ScheduleEvent(0, Application_Init);
 vTaskStartScheduler();
 return 0;
}
