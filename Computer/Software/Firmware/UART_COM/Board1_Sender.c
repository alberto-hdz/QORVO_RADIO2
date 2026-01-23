// File: uart_sender/main.c
#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"
#include "qPinCfg.h"
#include "qDrvUart.h"
#include "FreeRTOS.h"
#include "task.h"
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
#define SENDER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000)
#define SENDER_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
// UART configuration
#define UART_INSTANCE 1 // Use UART1
#define UART_TX_GPIO 8 // GPIO8 for TX
#define UART_RX_GPIO 9 // GPIO9 for RX
#define UART_BAUD_RATE 115200
static qDrvUart_Handle_t uartHandle;
// Message counter
static uint32_t messageCount = 0;
static void ConfigureUART(void)
{
 qDrvUart_Config_t config;
 
 // Set default configuration
 qDrvUart_GetDefaultConfig(&config);
 
 // Customize configuration
 config.baudRate = UART_BAUD_RATE;
 config.txPin = UART_TX_GPIO;
 config.rxPin = UART_RX_GPIO;
 config.dataBits = qDrvUart_DataBits8;
 config.parity = qDrvUart_ParityNone;
 config.stopBits = qDrvUart_StopBits1;
 
 // Initialize UART
 qResult_t result = qDrvUart_Init(UART_INSTANCE, &config, &uartHandle);
 if (result != Q_OK)
 {
 GP_LOG_SYSTEM_PRINTF("UART init failed: %d", 0, result);
 }
 else
 {
 GP_LOG_SYSTEM_PRINTF("UART initialized successfully", 0);
 }
}
static void SendMessage(const char* message, uint16_t length)
{
 qDrvUart_Write(uartHandle, (uint8_t*)message, length);
}
static void sender_Task(void* pvParameters)
{
 char buffer[64];
 
 GP_LOG_SYSTEM_PRINTF("Sender task started", 0);
 
 while(1)
 {
 // Create message with counter
 int len = snprintf(buffer, sizeof(buffer), 
 "MSG:%lu:HELLO_FROM_BOARD1\r\n", messageCount);
 
 // Send via UART
 SendMessage(buffer, len);
 
 // Log to debug console
 GP_LOG_SYSTEM_PRINTF("Sent: MSG:%lu", 0, messageCount);
 
 messageCount++;
 
 // Toggle LED to show activity
 HAL_LED_TGL_GRN();
 
 // Wait 1 second
 vTaskDelay(1000);
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
 
 // Configure UART for board-to-board communication
 ConfigureUART();
 
 // Create sender task
 static StaticTask_t xSenderTaskPCB;
 static StackType_t xSenderTaskStack[SENDER_TASK_STACK_SIZE];
 
 TaskHandle_t senderTaskHandle = xTaskCreateStatic(
 sender_Task,
 "sender_Task",
 SENDER_TASK_STACK_SIZE,
 NULL,
 SENDER_TASK_PRIORITY,
 xSenderTaskStack,
 &xSenderTaskPCB
 );
 GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, senderTaskHandle != NULL);
 
 GP_LOG_SYSTEM_PRINTF("=== BOARD 1: UART SENDER ===", 0);
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