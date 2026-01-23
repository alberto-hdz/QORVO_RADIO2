// File: uart_receiver/main.c
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
#include "StatusLed.h"
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP
#define RECEIVER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000)
#define RECEIVER_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
// UART configuration (same as sender but TX/RX conceptually swapped)
#define UART_INSTANCE 1
#define UART_TX_GPIO 8 // GPIO8 for TX (response if needed)
#define UART_RX_GPIO 9 // GPIO9 for RX (receive messages)
#define UART_BAUD_RATE 115200
static qDrvUart_Handle_t uartHandle;
static uint32_t messagesReceived = 0;
// Receive buffer
#define RX_BUFFER_SIZE 128
static uint8_t rxBuffer[RX_BUFFER_SIZE];
static uint16_t rxIndex = 0;
static void ConfigureUART(void)
{
 qDrvUart_Config_t config;
 
 qDrvUart_GetDefaultConfig(&config);
 config.baudRate = UART_BAUD_RATE;
 config.txPin = UART_TX_GPIO;
 config.rxPin = UART_RX_GPIO;
 config.dataBits = qDrvUart_DataBits8;
 config.parity = qDrvUart_ParityNone;
 config.stopBits = qDrvUart_StopBits1;
 
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
static void ProcessMessage(uint8_t* message, uint16_t length)
{
 // Null-terminate for printing
 message[length] = '\0';
 
 // Log received message
 GP_LOG_SYSTEM_PRINTF("RX[%lu]: %s", 0, messagesReceived, message);
 
 // Parse message (format: "MSG:counter:payload")
 if (strncmp((char*)message, "MSG:", 4) == 0)
 {
 messagesReceived++;
 
 // Blink LED to show successful reception
 HAL_LED_TGL_GRN();
 
 GP_LOG_SYSTEM_PRINTF("Valid message #%lu received!", 0, messagesReceived);
 }
}
static void receiver_Task(void* pvParameters)
{
 uint8_t byte;
 uint16_t bytesRead;
 
 GP_LOG_SYSTEM_PRINTF("Receiver task started - waiting for data...", 0);
 
 while(1)
 {
 // Try to read one byte
 bytesRead = qDrvUart_Read(uartHandle, &byte, 1);
 
 if (bytesRead > 0)
 {
 // Check for end of line
 if (byte == '\n' || byte == '\r')
 {
 if (rxIndex > 0)
 {
 // Process complete message
 ProcessMessage(rxBuffer, rxIndex);
 rxIndex = 0;
 }
 }
 else
 {
 // Add byte to buffer
 if (rxIndex < RX_BUFFER_SIZE - 1)
 {
 rxBuffer[rxIndex++] = byte;
 }
 else
 {
 // Buffer overflow - reset
 GP_LOG_SYSTEM_PRINTF("RX buffer overflow!", 0);
 rxIndex = 0;
 }
 }
 }
 else
 {
 // No data - yield to other tasks
 vTaskDelay(10); // 10ms polling interval
 }
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
 
 // Configure UART
 ConfigureUART();
 
 // Create receiver task
 static StaticTask_t xReceiverTaskPCB;
 static StackType_t xReceiverTaskStack[RECEIVER_TASK_STACK_SIZE];
 
 TaskHandle_t receiverTaskHandle = xTaskCreateStatic(
 receiver_Task,
 "receiver_Task",
 RECEIVER_TASK_STACK_SIZE,
 NULL,
 RECEIVER_TASK_PRIORITY,
 xReceiverTaskStack,
 &xReceiverTaskPCB
 );
 GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, receiverTaskHandle != NULL);
 
 GP_LOG_SYSTEM_PRINTF("=== BOARD 2: UART RECEIVER ===", 0);
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