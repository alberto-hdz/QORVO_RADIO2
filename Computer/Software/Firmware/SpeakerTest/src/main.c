/*
 * Copyright (c) 2025, Qorvo Inc
 *
 * Speaker test application.
 * Generates a 500 Hz square wave on GPIO10 via PWMXL_4.
 * Connect GPIO10 -> 1kOhm resistor -> TPA2034D1 IN+ pin.
 * TPA2034D1 IN- to GND. Speaker (CSS-66668N 8 Ohm) to amplifier output.
 */

#include "hal.h"
#include "gpSched.h"
#include "gpHal.h"
#include "gpBaseComps.h"
#include "gpCom.h"
#include "gpLog.h"
#include "qPinCfg.h"
#include "FreeRTOS.h"
#include "task.h"

#include "qDrvPWMXL.h"

#include "app_common.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

/* Single PWMXL instance (ID=0). GPIO10 = channel 4 (PWMXL_4). */
#define SPEAKER_PWMXL_ID      0
#define SPEAKER_PWMXL_CHANNEL 4
#define SPEAKER_GPIO_PIN      10

#define TONE_FREQ_HZ          500

static qDrvPWMXL_t pwmxlDrv = Q_DRV_PWMXL_INSTANCE_DEFINE(SPEAKER_PWMXL_ID);

void Application_Init(void)
{
    qResult_t res;
    UInt16 ticks;
    UInt8 prescaler;

#ifndef GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT
#error GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT must be defined
#endif
    gpBaseComps_StackInit();

    gpCom_Init();
    gpLog_Init();

    GP_LOG_SYSTEM_PRINTF("Speaker test: 500Hz tone on GPIO10 via PWMXL_4", 0);

    res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    /* Configure GPIO10 as PWMXL output */
    qDrvPWMXL_PinConfig_t pinCfg = Q_DRV_PWMXL_PIN_CONFIG(SPEAKER_PWMXL_CHANNEL, SPEAKER_GPIO_PIN);
    res = qDrvPWMXL_PinConfigSet(&pinCfg);
    GP_ASSERT_SYSTEM(res == Q_OK);

    /* Calculate period ticks for 500 Hz */
    Bool ok = qDrvPWMXL_FrequencyCalculate(TONE_FREQ_HZ, &ticks, &prescaler);
    if(!ok)
    {
        GP_LOG_SYSTEM_PRINTF("FrequencyCalculate failed for %u Hz", 0, TONE_FREQ_HZ);
        GP_ASSERT_SYSTEM(false);
    }

    qDrvPWMXL_Config_t cfg = {
        .countMode   = qDrvPWMXL_CountModePrescaled,
        .prescaler   = prescaler,
        .periodTicks = ticks,
    };

    res = qDrvPWMXL_Init(&pwmxlDrv, &cfg, NULL, NULL);
    GP_ASSERT_SYSTEM(res == Q_OK);

    res = qDrvPWMXL_ChannelInit(&pwmxlDrv, SPEAKER_PWMXL_CHANNEL);
    GP_ASSERT_SYSTEM(res == Q_OK);

    /* 50% duty cycle */
    res = qDrvPWMXL_ChannelDutySet(&pwmxlDrv, SPEAKER_PWMXL_CHANNEL, 5000);
    GP_ASSERT_SYSTEM(res == Q_OK);

    res = qDrvPWMXL_Enable(&pwmxlDrv, true);
    GP_ASSERT_SYSTEM(res == Q_OK);

    GP_LOG_SYSTEM_PRINTF("Tone running. Prescaler=%u Ticks=%u", 0, prescaler, ticks);
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
