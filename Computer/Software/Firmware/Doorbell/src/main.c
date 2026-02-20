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

/** @file "main.c"
 *
 * QPG6200 Standalone Doorbell Demo
 *
 * A standalone (no BLE) demo that simulates a doorbell using
 * the QPG6200L Development Kit. Demonstrates buttons, LEDs, and
 * serial logging in a clear, observable way.
 *
 * HOW IT WORKS:
 *
 *   IDLE state:
 *     - WHITE LED: slow heartbeat blink (100ms ON / 1900ms OFF)
 *     - GREEN LED: ON (shows device is ready)
 *     - Serial: startup banner, then silent
 *
 *   RINGING state (triggered by pressing PB5):
 *     - WHITE LED: rapid blink (150ms ON / 150ms OFF)
 *     - GREEN LED: OFF
 *     - Serial: "** DING DONG! **" + ring counter, repeats every 2 sec
 *     - Auto-dismisses after 30 seconds if not manually dismissed
 *
 *   DISMISS (press PB5 again OR press PB1 at any time):
 *     - Returns to IDLE state
 *     - Serial: shows total ring count
 *
 * BUTTONS:
 *   PB5 (GPIO 0) - Doorbell button: press to ring; press again to dismiss
 *   PB1 (GPIO 3) - Dismiss button:  always silences the doorbell
 *
 * LEDs:
 *   WHITE_COOL (GPIO 1)  - Status blink (slow=idle, fast=ringing)
 *   GREEN      (GPIO 11) - Ready indicator (ON=ready, OFF=ringing)
 *
 * SERIAL OUTPUT:
 *   UART1 TX pin: GPIO 9
 *   Baud rate:    115200
 *   Connect any serial terminal (PuTTY, CoolTerm, screen, etc.)
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
#include "StatusLed.h"
#include "ButtonHandler.h"
#include "FreeRTOS.h"
#include "task.h"

/*****************************************************************************
 *                    Defines
 *****************************************************************************/
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* Doorbell states */
#define DOORBELL_STATE_IDLE    0
#define DOORBELL_STATE_RINGING 1

/* LED blink timing in milliseconds */
#define HEARTBEAT_ON_MS    100    /* White: short flash during idle */
#define HEARTBEAT_OFF_MS  1900    /* White: long pause during idle  */
#define RING_BLINK_ON_MS   150    /* White: fast blink while ringing */
#define RING_BLINK_OFF_MS  150    /* White: fast blink while ringing */

/* Auto-dismiss timeout in microseconds (30 seconds) */
#define AUTO_DISMISS_US   30000000UL

/* Periodic ring-reminder interval in microseconds (2 seconds) */
#define RING_TICK_US       2000000UL

/*****************************************************************************
 *                    Static Variables
 *****************************************************************************/
static uint8_t  s_State     = DOORBELL_STATE_IDLE;
static uint32_t s_RingCount = 0;

static const uint8_t StatusLedGpios[] = QPINCFG_STATUS_LED;

/*****************************************************************************
 *                    Forward Declarations
 *****************************************************************************/
static void Doorbell_StartRinging(void);
static void Doorbell_Dismiss(void);
static void Doorbell_RingTick(void);
static void Doorbell_AutoDismiss(void);
static void ButtonCallback(uint8_t btnIdx, bool btnPressed);

/*****************************************************************************
 *                    Doorbell Logic
 *****************************************************************************/

/**
 * @brief Transition to RINGING state.
 *        If already ringing, increments ring count and prints again.
 */
static void Doorbell_StartRinging(void)
{
    if(s_State == DOORBELL_STATE_RINGING)
    {
        /* Already ringing - count the extra press */
        s_RingCount++;
        GP_LOG_SYSTEM_PRINTF("", 0);
        GP_LOG_SYSTEM_PRINTF("** DING DONG! ** (Ring #%d - pressed again!)", 0, s_RingCount);
        return;
    }

    s_State = DOORBELL_STATE_RINGING;
    s_RingCount++;

    /* Update LEDs: fast blink + green off */
    StatusLed_BlinkLed(APP_STATUS_LED, RING_BLINK_ON_MS, RING_BLINK_OFF_MS);
    StatusLed_SetLed(APP_READY_LED, false);

    /* Print doorbell event */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("  ** DING DONG! **  Doorbell Ring #%d", 0, s_RingCount);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("  PB5 was pressed!", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE LED: fast blinking", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN LED: OFF", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("  Press PB5 or PB1 to dismiss.", 0);
    GP_LOG_SYSTEM_PRINTF("  Auto-dismiss in 30 seconds.", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);

    /* Schedule repeating ring reminder every 2 seconds */
    gpSched_ScheduleEvent(RING_TICK_US, Doorbell_RingTick);

    /* Schedule auto-dismiss */
    gpSched_ScheduleEvent(AUTO_DISMISS_US, Doorbell_AutoDismiss);
}

/**
 * @brief Transition back to IDLE state. Cancels all scheduled events.
 */
static void Doorbell_Dismiss(void)
{
    if(s_State == DOORBELL_STATE_IDLE)
    {
        return;
    }

    s_State = DOORBELL_STATE_IDLE;

    /* Cancel scheduled callbacks */
    gpSched_UnscheduleEvent(Doorbell_RingTick);
    gpSched_UnscheduleEvent(Doorbell_AutoDismiss);

    /* Update LEDs: slow heartbeat + green on */
    StatusLed_BlinkLed(APP_STATUS_LED, HEARTBEAT_ON_MS, HEARTBEAT_OFF_MS);
    StatusLed_SetLed(APP_READY_LED, true);

    /* Print dismiss event */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("----------------------------------------", 0);
    GP_LOG_SYSTEM_PRINTF("  Doorbell dismissed.", 0);
    GP_LOG_SYSTEM_PRINTF("  Total rings so far: %d", 0, s_RingCount);
    GP_LOG_SYSTEM_PRINTF("  WHITE LED: slow heartbeat", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN LED: ON - ready for next ring", 0);
    GP_LOG_SYSTEM_PRINTF("  Press PB5 to ring again.", 0);
    GP_LOG_SYSTEM_PRINTF("----------------------------------------", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
}

/**
 * @brief Scheduled by gpSched every 2 seconds while ringing.
 *        Prints a reminder and re-schedules itself.
 */
static void Doorbell_RingTick(void)
{
    if(s_State != DOORBELL_STATE_RINGING)
    {
        return;
    }

    GP_LOG_SYSTEM_PRINTF("** DING DONG! ** (Ring #%d) Press PB1 to dismiss.", 0, s_RingCount);

    /* Re-schedule for the next tick */
    gpSched_ScheduleEvent(RING_TICK_US, Doorbell_RingTick);
}

/**
 * @brief Triggered by gpSched after 30 seconds of ringing without dismiss.
 */
static void Doorbell_AutoDismiss(void)
{
    if(s_State == DOORBELL_STATE_RINGING)
    {
        GP_LOG_SYSTEM_PRINTF("", 0);
        GP_LOG_SYSTEM_PRINTF("Auto-dismiss: no response after 30 seconds.", 0);
        Doorbell_Dismiss();
    }
}

/*****************************************************************************
 *                    Button Handler Callback
 *****************************************************************************/

/**
 * @brief Callback from ButtonHandler (runs in gpSched context - debounced).
 *
 * @param gpioPin   GPIO number that changed
 * @param isPressed true = button pressed down, false = button released
 */
/* Button indices (order in buttons[] passed to ButtonHandler_Init) */
#define BTN_IDX_DOORBELL 0
#define BTN_IDX_DISMISS  1

static void ButtonCallback(uint8_t btnIdx, bool btnPressed)
{
    if(!btnPressed)
    {
        return;  /* Only process press events */
    }

    if(btnIdx == BTN_IDX_DOORBELL)
    {
        /* PB5: ring if idle, dismiss if ringing */
        if(s_State == DOORBELL_STATE_IDLE)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] PB5 pressed - ringing!", 0);
            Doorbell_StartRinging();
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] PB5 pressed - dismissing", 0);
            Doorbell_Dismiss();
        }
    }
    else if(btnIdx == BTN_IDX_DISMISS)
    {
        /* PB1: always dismisses */
        if(s_State == DOORBELL_STATE_RINGING)
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] PB1 pressed - dismissing", 0);
            Doorbell_Dismiss();
        }
        else
        {
            GP_LOG_SYSTEM_PRINTF("[BTN] PB1 pressed - already idle", 0);
        }
    }
}

/*****************************************************************************
 *                    Application Initialization
 *****************************************************************************/

/**
 * @brief Called from gpSched task after scheduler starts.
 *        Runs with a larger stack than main(), suitable for initialization.
 */
void Application_Init(void)
{
    gpHal_Set32kHzCrystalAvailable(false);

    /* Initialize stack components */
    gpBaseComps_StackInit();
    gpCom_Init();
    gpLog_Init();

    /* Configure GPIO pins */
    qResult_t res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    /* Initialize LED driver (active HIGH) */
    StatusLed_Init(StatusLedGpios, Q_ARRAY_SIZE(StatusLedGpios), true);

    /* Initialize ButtonHandler with debounced callback */
    static const ButtonConfig_t buttons[] = {
        {.gpio = APP_DOORBELL_BUTTON},
        {.gpio = APP_DISMISS_BUTTON},
    };
    ButtonHandler_Init(buttons, Q_ARRAY_SIZE(buttons), BUTTON_LOW, ButtonCallback);

    /* Start idle LED pattern */
    StatusLed_BlinkLed(APP_STATUS_LED, HEARTBEAT_ON_MS, HEARTBEAT_OFF_MS);
    StatusLed_SetLed(APP_READY_LED, true);

    /* Print startup banner to serial */
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("  QPG6200 STANDALONE DOORBELL DEMO", 0);
    GP_LOG_SYSTEM_PRINTF("========================================", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("Board  : QPG6200L Development Kit", 0);
    GP_LOG_SYSTEM_PRINTF("Serial : GPIO9 TX, 115200 baud, 8N1", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- LED Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE slow flash = idle / waiting", 0);
    GP_LOG_SYSTEM_PRINTF("  WHITE fast blink = RINGING!", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN ON         = ready for a ring", 0);
    GP_LOG_SYSTEM_PRINTF("  GREEN OFF        = currently ringing", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("--- Button Guide ---", 0);
    GP_LOG_SYSTEM_PRINTF("  PB5 = Ring the doorbell (or dismiss)", 0);
    GP_LOG_SYSTEM_PRINTF("  PB1 = Dismiss / silence", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
    GP_LOG_SYSTEM_PRINTF("Ready! Press PB5 to ring the doorbell.", 0);
    GP_LOG_SYSTEM_PRINTF("", 0);
}

/*****************************************************************************
 *                    Main Entry Point
 *****************************************************************************/

int main(void)
{
    HAL_INITIALIZE_GLOBAL_INT();
    HAL_INIT();
    HAL_ENABLE_GLOBAL_INT();

    gpSched_Init();

    /* Run Application_Init from the gpSched task (has a larger stack) */
    gpSched_ScheduleEvent(0, Application_Init);

    vTaskStartScheduler();

    return 0;
}
