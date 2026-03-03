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

/** @file "DoorbellManager.cpp"
 *
 * GPADC-based doorbell button manager for the DK Analog application.
 *
 * Hardware connection:
 *   GPIO 29 = ANIO1 (DK expansion header, accessible without carrier board)
 *
 * Typical DK wiring:
 *   - Connect a 10 kΩ pull-down resistor from GPIO 29 to GND.
 *   - Connect a push-button between GPIO 29 and 3.3 V.
 *   - Button released : 0 V   (< DOORBELL_ADC_RELEASE_MV)
 *   - Button pressed  : ~VCC  (> DOORBELL_ADC_PRESS_MV)
 *
 * Resolution:  11-bit  (2048 steps over 0-3.6 V => 1.76 mV/step)
 * Poll rate :  DOORBELL_ADC_POLL_MS  (default 100 ms)
 * Debounce  :  DOORBELL_DEBOUNCE_COUNT consecutive matching samples
 */

#include "DoorbellManager.h"
#include "AppManager.h"
#include "gpLog.h"
#include "qDrvGPADC.h"

#include "FreeRTOS.h"
#include "task.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* -------------------------------------------------------------------------
 * Static members
 * ------------------------------------------------------------------------- */
bool    DoorbellManager::sIsPressed    = false;
uint8_t DoorbellManager::sDebounceCount = 0;

/* -------------------------------------------------------------------------
 * GPADC driver instance and channel config
 * GPIO 29, alt 1 = ANIO1
 * ------------------------------------------------------------------------- */
static qDrvGPADC_t sAdcDrv;

static const qDrvIOB_PinAlt_t sAdcPin = Q_DRV_GPADC_PIN(29, 1);

/* FreeRTOS task storage */
#define DOORBELL_TASK_STACK_SIZE  (2 * configMINIMAL_STACK_SIZE)
#define DOORBELL_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

static StaticTask_t  sDoorbellTask;
static StackType_t   sDoorbellStack[DOORBELL_TASK_STACK_SIZE];

/* -------------------------------------------------------------------------
 * Init
 * ------------------------------------------------------------------------- */
bool DoorbellManager::Init(void)
{
    qResult_t res;

    /* 1. Configure GPIO 29 pin as analog input (ANIO1, alt 1) */
    res = qDrvGPADC_PinConfigSet(&sAdcPin, 1);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] PinConfigSet failed: %d", 0, res);
        return false;
    }

    /* 2. Initialise GPADC driver (no DMA, no interrupt callbacks) */
    qDrvGPADC_Config_t adcConfig = {
        .dma = false,
    };
    res = qDrvGPADC_Init(&sAdcDrv, &adcConfig, NULL, NULL, 0);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] Init failed: %d", 0, res);
        return false;
    }

    /* 3. Configure Slot A -> ANIO1 (single-ended, high-voltage mode) */
    qDrvGPADC_SlotConfig_t slotConfig = {
        .pChannel   = qRegGPADC_ChannelAnIo1,
        .nChannel   = qRegGPADC_ChannelNone,
        .diffMode   = false,
        .waitTime   = 0,
        .voltageMode = qDrvGPADC_VoltageModeHigh,
        .higherSpeed = false,
        .filterCap  = 0,
        .postBuffer = qDrvGPADC_PostBufferA,
    };
    res = qDrvGPADC_SlotConfigSet(&sAdcDrv, qRegGPADC_SlotA, &slotConfig);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] SlotConfigSet failed: %d", 0, res);
        return false;
    }

    res = qDrvGPADC_SlotEnable(&sAdcDrv, qRegGPADC_SlotA);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] SlotEnable failed: %d", 0, res);
        return false;
    }

    /* 4. Configure Buffer A: 11-bit, normal update mode, no preset IRQ */
    qDrvGPADC_BufferConfig_t bufferConfig = {
        .resolution = qDrvGPADC_Resolution11Bit,
        .updateMode = qRegGPADC_BufferUpdateModeNormal,
        .irqEnable  = false,
        .preset     = {
            .min = Q_DRV_GPADC_PRESET_VALUE_UNUSED,
            .max = Q_DRV_GPADC_PRESET_VALUE_UNUSED,
        },
    };
    res = qDrvGPADC_BufferConfigSet(&sAdcDrv, qRegGPADC_BufferA, &bufferConfig);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] BufferConfigSet failed: %d", 0, res);
        return false;
    }

    /* 5. Start continuous conversion */
    res = qDrvGPADC_ContinuousStart(&sAdcDrv);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("[ADC] ContinuousStart failed: %d", 0, res);
        return false;
    }

    GP_LOG_SYSTEM_PRINTF("[ADC] GPADC ready on GPIO29 (ANIO1)", 0);
    GP_LOG_SYSTEM_PRINTF("[ADC] Press threshold : %u mV", 0, DOORBELL_ADC_PRESS_MV);
    GP_LOG_SYSTEM_PRINTF("[ADC] Release threshold: %u mV", 0, DOORBELL_ADC_RELEASE_MV);
    return true;
}

/* -------------------------------------------------------------------------
 * StartPolling  - creates the FreeRTOS polling task
 * ------------------------------------------------------------------------- */
void DoorbellManager::StartPolling(void)
{
    TaskHandle_t handle =
        xTaskCreateStatic(PollTask, "DoorbellADC", DOORBELL_TASK_STACK_SIZE,
                          nullptr, DOORBELL_TASK_PRIORITY, sDoorbellStack, &sDoorbellTask);
    Q_ASSERT(handle != nullptr);
}

/* -------------------------------------------------------------------------
 * IsPressed
 * ------------------------------------------------------------------------- */
bool DoorbellManager::IsPressed(void)
{
    return sIsPressed;
}

/* -------------------------------------------------------------------------
 * PollTask  - runs forever at DOORBELL_ADC_POLL_MS rate
 * ------------------------------------------------------------------------- */
void DoorbellManager::PollTask(void* /*pvParameters*/)
{
    while(true)
    {
        /* Read raw 11-bit ADC value from Buffer A */
        UInt16 adcRaw = qDrvGPADC_BufferRawResultGet(&sAdcDrv, qRegGPADC_BufferA);

        /* Convert raw count to millivolts */
        qDrvGPADC_Voltage_t v =
            qDrvGPADC_RawToVoltageConvert(&sAdcDrv, adcRaw, qDrvGPADC_Resolution11Bit,
                                          qRegGPADC_SlotA);
        uint32_t voltMv = (uint32_t)v.integer * 1000u + v.fractional;

        /* Hysteresis state machine with debounce counter */
        if(!sIsPressed)
        {
            /* Looking for a PRESS event */
            if(voltMv > DOORBELL_ADC_PRESS_MV)
            {
                sDebounceCount++;
                if(sDebounceCount >= DOORBELL_DEBOUNCE_COUNT)
                {
                    sIsPressed     = true;
                    sDebounceCount = 0;
                    GP_LOG_SYSTEM_PRINTF("[ADC] Doorbell PRESSED  (%.3u mV, raw=%u)", 0,
                                         voltMv, adcRaw);
                    AppManager::NotifyAnalogEvent(true, adcRaw);
                }
            }
            else
            {
                sDebounceCount = 0;
            }
        }
        else
        {
            /* Looking for a RELEASE event */
            if(voltMv < DOORBELL_ADC_RELEASE_MV)
            {
                sDebounceCount++;
                if(sDebounceCount >= DOORBELL_DEBOUNCE_COUNT)
                {
                    sIsPressed     = false;
                    sDebounceCount = 0;
                    GP_LOG_SYSTEM_PRINTF("[ADC] Doorbell RELEASED (%.3u mV, raw=%u)", 0,
                                         voltMv, adcRaw);
                    AppManager::NotifyAnalogEvent(false, adcRaw);
                }
            }
            else
            {
                sDebounceCount = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DOORBELL_ADC_POLL_MS));
    }

    /* Never reached */
    vTaskDelete(nullptr);
}
