/*
 * Movement Detector using LV-MaxSonar-EZ (MB1000)
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 * Sensor: MaxBotix LV-MaxSonar-EZ series (MB1000-000)
 * Interface: Pulse Width output (Pin 2 - PW)
 *
 * Wiring:
 * Sensor Pin 2 (PW)   -> GPIO11 (input - reads pulse width)
 * Sensor Pin 4 (RX)   -> GPIO10 (output - set HIGH for continuous ranging)
 * Sensor Pin 6 (Vcc)  -> 3.3V or preferably 5V
 * Sensor Pin 7 (GND)  -> GND
 * Sensor Pin 1 (BW)   -> Leave open or connect to GND
 */
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

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

// Timing configuration
#define INTERVAL_MS               100   // Time between readings (ms)
#define MOVEMENT_THRESHOLD_CM     5     // cm change needed to detect movement

// Task configuration
#define LED_TASK_PRIORITY         (tskIDLE_PRIORITY + 1)
#define SENSOR_TASK_PRIORITY      (tskIDLE_PRIORITY + 1)
#define LED_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE + 1000)
#define SENSOR_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE + 1000)

// GPIO assignments
#define RX_GPIO                   10    // Output – HIGH = continuous ranging
#define PW_GPIO                   11    // Input  – pulse width from sensor

// Pulse width scaling (LV-MaxSonar-EZ)
#define US_PER_INCH               147
#define US_PER_CM                 58    // ≈ 147 / 2.54

// If gpSched_GetCurrentTime() returns milliseconds instead of microseconds,
// uncomment the next line:
// #define USE_MS_TIMER

// Function declarations
static void ledToggle_Task(void* pvParameters);
static void sensor_Task(void* pvParameters);

// Status LED pins (from your original)
static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/* LED blinking task */
static void ledToggle_Task(void* pvParameters)
{
    Bool value = false;

    while (1)
    {
        StatusLed_SetLed(GREEN_LED_GPIO_PIN, value);
        value = !value;
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/* Main sensor reading task – free-run mode with debug logging */
static void sensor_Task(void* pvParameters)
{
    static int prev_distance_cm = 0;
    static Bool first_reading = true;

    // Configure pins
    qDrvIOB_ConfigOutputSet(RX_GPIO, qDrvIOB_Drive2mA, qDrvIOB_SlewRateSlow);
    qDrvIOB_ConfigInputSet(PW_GPIO, qDrvIOB_PullNone, false);

    // Enable continuous ranging
    qDrvGPIO_Write(RX_GPIO, 1);

    GP_LOG_SYSTEM_PRINTF("LV-MaxSonar-EZ initializing... (waiting 300 ms)", 0);
    vTaskDelay(300 / portTICK_PERIOD_MS);

    GP_LOG_SYSTEM_PRINTF("Sensor running – RX held HIGH (free-run mode)", 0);
    GP_LOG_SYSTEM_PRINTF("Initial PW pin state: %d (should toggle high/low ~every 50 ms)", 0,
                         qDrvGPIO_Read(PW_GPIO));

    while (1)
    {
        // Log PW state at start of cycle
        int pw_state = qDrvGPIO_Read(PW_GPIO);
        GP_LOG_SYSTEM_PRINTF("New cycle – PW pin state: %d", 0, pw_state);

        // Wait for rising edge (PW → 1)
        uint32_t timeout_rise = 100000;  // ~100 ms tolerance
        while (qDrvGPIO_Read(PW_GPIO) == 0 && timeout_rise-- > 0) {}

        if (timeout_rise == 0)
        {
            GP_LOG_SYSTEM_PRINTF("TIMEOUT: No rising edge detected on PW – check wiring, power, RX high?", 0);
            vTaskDelay(INTERVAL_MS / portTICK_PERIOD_MS);
            continue;
        }

        uint32_t start_time = gpSched_GetCurrentTime();

        // Wait for falling edge (PW → 0)
        uint32_t timeout_fall = 50000;   // max ~37–40 ms pulse
        while (qDrvGPIO_Read(PW_GPIO) == 1 && timeout_fall-- > 0) {}

        uint32_t end_time = gpSched_GetCurrentTime();

        if (timeout_fall == 0)
        {
            GP_LOG_SYSTEM_PRINTF("TIMEOUT: PW stuck HIGH – possible sensor issue or wiring fault", 0);
            vTaskDelay(INTERVAL_MS / portTICK_PERIOD_MS);
            continue;
        }

        uint32_t delta = end_time - start_time;

#ifdef USE_MS_TIMER
        uint32_t pulse_us = delta * 1000UL;
        GP_LOG_SYSTEM_PRINTF("Raw pulse: %lu us  (timer delta: %lu ms)", 0, pulse_us, delta);
#else
        uint32_t pulse_us = delta;
        GP_LOG_SYSTEM_PRINTF("Raw pulse: %lu us  (timer delta: %lu)", 0, pulse_us, delta);
#endif

        int distance_cm = (int)(pulse_us / US_PER_CM);

        // Report result
        if (distance_cm >= 15 && distance_cm <= 645)
        {
            if (first_reading)
            {
                prev_distance_cm = distance_cm;
                first_reading = false;
                GP_LOG_SYSTEM_PRINTF("First valid reading: %d cm (pulse %lu us)", 0, distance_cm, pulse_us);
            }
            else
            {
                int change = abs(distance_cm - prev_distance_cm);
                if (change > MOVEMENT_THRESHOLD_CM)
                {
                    GP_LOG_SYSTEM_PRINTF("MOVEMENT! Distance: %d cm  change: %d cm  pulse: %lu us",
                                         0, distance_cm, change, pulse_us);
                    prev_distance_cm = distance_cm;
                }
                else
                {
                    GP_LOG_SYSTEM_PRINTF("Stable: %d cm (pulse %lu us)", 0, distance_cm, pulse_us);
                }
            }
        }
        else if (distance_cm < 15)
        {
            GP_LOG_SYSTEM_PRINTF("Very close / min range reported: %d cm (pulse %lu us)", 0, distance_cm, pulse_us);
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("Invalid / out of range: %d cm (pulse %lu us)", 0, distance_cm, pulse_us);
        }

        vTaskDelay(INTERVAL_MS / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/* Application entry point / initialization */
void Application_Init(void)
{
    gpHal_Set32kHzCrystalAvailable(false);

    gpBaseComps_StackInit();
    gpCom_Init();
    gpLog_Init();

    qResult_t res = qPinCfg_Init(NULL);
    if (res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);

    GP_LOG_SYSTEM_PRINTF("Movement Detector – LV-MaxSonar-EZ started", 0);
    GP_LOG_SYSTEM_PRINTF("RX GPIO%d = HIGH, PW GPIO%d", 0, RX_GPIO, PW_GPIO);

    // LED task
    static StaticTask_t xLedTaskPCB;
    static StackType_t xLedTaskStack[LED_TASK_STACK_SIZE];
    TaskHandle_t ledTask = xTaskCreateStatic(
        ledToggle_Task, "LED", LED_TASK_STACK_SIZE,
        NULL, LED_TASK_PRIORITY, xLedTaskStack, &xLedTaskPCB);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, ledTask != NULL);

    // Sensor task
    static StaticTask_t xSensorTaskPCB;
    static StackType_t xSensorTaskStack[SENSOR_TASK_STACK_SIZE];
    TaskHandle_t sensorTask = xTaskCreateStatic(
        sensor_Task, "Sensor", SENSOR_TASK_STACK_SIZE,
        NULL, SENSOR_TASK_PRIORITY, xSensorTaskStack, &xSensorTaskPCB);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, sensorTask != NULL);
}

/* Main program entry */
int main(void)
{
    HAL_INITIALIZE_GLOBAL_INT();
    HAL_INIT();
    HAL_ENABLE_GLOBAL_INT();

    gpSched_Init();
    gpSched_ScheduleEvent(0, Application_Init);

    vTaskStartScheduler();

    // Should never reach here
    return 0;
}