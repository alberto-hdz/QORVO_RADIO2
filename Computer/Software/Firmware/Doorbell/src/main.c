/*
 * Doorbell Device
 * Copyright (c) 2024-2025, Qorvo Inc
 * 
 * Hardware: Pushbutton connected between GPIO4 and GND
 * Function: Detects button presses, blinks LED, logs to serial console
 */

/*****************************************************************************
 *                    Includes
 *****************************************************************************/
#include "hal.h"
#include "gpHal_DEFS.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "gpSched.h"
#include "qPinCfg.h"
#include "qDrvGPIO.h"
#include "qDrvIOB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "StatusLed.h"
#include <stdlib.h>

/*****************************************************************************
 *                    Definitions
 *****************************************************************************/
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

// Hardware configuration
#define BUTTON_GPIO 4              // Pushbutton connected to GPIO4

// Task priorities and stack sizes
#define LED_TASK_PRIORITY           (tskIDLE_PRIORITY + 1)
#define BUTTON_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)
#define LED_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 1000)
#define BUTTON_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE + 1000)

// Timing configuration
#define INTERVAL_MS 1000           // LED blink interval
#define DEBOUNCE_DELAY_MS 50       // Button debounce delay
#define BUTTON_CHECK_INTERVAL_MS 10
#define ALERT_BLINK_COUNT 5        // Number of blinks when button pressed
#define ALERT_BLINK_DURATION_MS 100

/*****************************************************************************
 *                    Global Variables
 *****************************************************************************/
// Status LED configuration from qPinCfg.h
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/*****************************************************************************
 *                    Function Declarations
 *****************************************************************************/
static void ledToggle_Task(void* pvParameters);
static void button_Task(void* pvParameters);

/*****************************************************************************
 *                    LED Task
 *****************************************************************************/
/*
 * Blinks LED at regular interval to show device is alive
 * Toggles every 500ms (creating 1 second on/off cycle)
 */
static void ledToggle_Task(void* pvParameters)
{
    Bool value = false;
    
    while(1)
    {
        StatusLed_SetLed(GREEN_LED_GPIO_PIN, value);
        value = !value;  // Flip LED state
        vTaskDelay(INTERVAL_MS / 2);
    }
    vTaskDelete(NULL);
}

/*****************************************************************************
 *                    Button Task
 *****************************************************************************/
/*
 * Monitors button and detects presses with debouncing
 * 
 * Button logic:
 * - GPIO4 has internal pull-up, so normally reads HIGH (1)
 * - When button pressed: GPIO4 connected to GND, reads LOW (0)
 * - Wait 50ms and check again to confirm (debouncing)
 * - Blink LED rapidly as feedback
 * - Wait for button release before detecting next press
 */
static void button_Task(void* pvParameters)
{
    while(1)
    {
        // Check if button pressed (GPIO reads LOW)
        if (qDrvGPIO_Read(BUTTON_GPIO) == 0)
        {
            // Wait for debounce
            vTaskDelay(DEBOUNCE_DELAY_MS);
            
            // Check again to confirm real press (not bounce)
            if (qDrvGPIO_Read(BUTTON_GPIO) == 0)
            {
                // Button press confirmed!
                GP_LOG_SYSTEM_PRINTF("=================================", 0);
                GP_LOG_SYSTEM_PRINTF("DOORBELL PRESSED!", 0);
                GP_LOG_SYSTEM_PRINTF("=================================", 0);
                
                // Visual feedback: blink LED rapidly
                for (int i = 0; i < ALERT_BLINK_COUNT; i++)
                {
                    StatusLed_SetLed(GREEN_LED_GPIO_PIN, true);
                    vTaskDelay(ALERT_BLINK_DURATION_MS);
                    StatusLed_SetLed(GREEN_LED_GPIO_PIN, false);
                    vTaskDelay(ALERT_BLINK_DURATION_MS);
                }
                
                // Wait for button release to avoid repeated triggers
                while (qDrvGPIO_Read(BUTTON_GPIO) == 0)
                {
                    vTaskDelay(BUTTON_CHECK_INTERVAL_MS);
                }
                
                GP_LOG_SYSTEM_PRINTF("Button released", 0);
                
                // TODO: Add mesh network message here
                // sendCoAPMessage("gateway", "/doorbell", "pressed");
            }
        }
        
        // Check button state every 10ms
        vTaskDelay(BUTTON_CHECK_INTERVAL_MS);
    }
    vTaskDelete(NULL);
}

/*****************************************************************************
 *                    Application Initialization
 *****************************************************************************/
/*
 * Initialize hardware and create tasks
 * Called once at startup before scheduler starts tasks
 */
void Application_Init(void)
{
    // Configure system to use internal 64kHz oscillator (no external crystal)
    gpHal_Set32kHzCrystalAvailable(false);
    
    // Initialize Qorvo base components (hardware abstraction layer)
    gpBaseComps_StackInit();
    
    // Initialize communication (UART) and logging
    gpCom_Init();
    gpLog_Init();
    
    // Initialize GPIO pin configuration
    qResult_t res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);  // Halt if initialization fails
    }
    
    // Initialize status LED driver
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);
    
    // Configure GPIO4 as input with pull-up resistor
    // Pull-up keeps pin HIGH when button not pressed
    // Button press connects to GND, pulling pin LOW
    qDrvIOB_ConfigInputSet(BUTTON_GPIO, qDrvIOB_PullUp, true);
    GP_LOG_SYSTEM_PRINTF("Doorbell ready on GPIO%d", 0, BUTTON_GPIO);
    
    // Create LED task (static allocation)
    static StaticTask_t xLedTaskPCB;
    static StackType_t xLedTaskStack[LED_TASK_STACK_SIZE];
    TaskHandle_t ledTaskHandle = xTaskCreateStatic(
        ledToggle_Task,           // Task function
        "ledToggle_Task",         // Task name (for debugging)
        LED_TASK_STACK_SIZE,      // Stack size
        NULL,                     // No parameters
        LED_TASK_PRIORITY,        // Priority
        xLedTaskStack,            // Stack memory
        &xLedTaskPCB              // Task control block
    );
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, ledTaskHandle != NULL);
    
    // Create button monitoring task
    static StaticTask_t xButtonTaskPCB;
    static StackType_t xButtonTaskStack[BUTTON_TASK_STACK_SIZE];
    TaskHandle_t buttonTaskHandle = xTaskCreateStatic(
        button_Task,
        "button_Task",
        BUTTON_TASK_STACK_SIZE,
        NULL,
        BUTTON_TASK_PRIORITY,
        xButtonTaskStack,
        &xButtonTaskPCB
    );
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, buttonTaskHandle != NULL);
}

/*****************************************************************************
 *                    Main Entry Point
 *****************************************************************************/
/*
 * First function called when chip powers on
 * Initializes hardware and starts FreeRTOS scheduler
 * Never returns - scheduler runs forever
 */
int main(void) 
{
    // Initialize interrupt controller
    HAL_INITIALIZE_GLOBAL_INT();
    
    // Initialize hardware (clocks, peripherals, etc.)
    HAL_INIT();
    
    // Enable interrupts globally
    HAL_ENABLE_GLOBAL_INT();
    
    // Initialize FreeRTOS scheduler
    gpSched_Init();
    
    // Schedule Application_Init to run when scheduler starts
    gpSched_ScheduleEvent(0, Application_Init);
    
    // Start scheduler - takes control never returns
    vTaskStartScheduler();
    
    
    return 0;
}