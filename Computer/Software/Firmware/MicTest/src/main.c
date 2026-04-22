/*
 * Copyright (c) 2025, Qorvo Inc
 *
 * Microphone test application.
 * Uses I2S_0 Master RX to clock and read the SPG08P4HM4H-1 PDM MEMS microphone.
 *
 * Wiring:
 *   GPIO3 (I2S SCK, ~1 MHz) -> mic CLK pin
 *   GPIO5 (I2S SDI)         <- mic DATA pin
 *   GPIO2 (I2S WS)          -> mic LR/SEL pin (selects left/right channel; tie to GND or VCC)
 *   3.3V                    -> mic VDD
 *   GND                     -> mic GND
 *
 * Each 16-bit I2S word contains 16 consecutive PDM bits.
 * The ratio of '1' bits in the bitstream approximates the sound pressure level.
 * A density near 50% = silence; higher/lower = sound present.
 * Level is reported over UART1 every ~50ms.
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

#include "qDrvI2S.h"

#include "app_common.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define I2S_INSTANCE_ID    0
#define I2S_SCK_GPIO       3   /* -> mic CLK */
#define I2S_SDI_GPIO       5   /* <- mic DATA */
#define I2S_WS_GPIO        2   /* -> mic LR select */

/* prescaler = (F_CLK / (2 * F_SCK)) - 1 = (64MHz / 2MHz) - 1 = 31 => F_SCK = 1 MHz */
#define I2S_PRESCALER      31

/* 128 bytes = 64 x 16-bit words = 1024 PDM bits per poll */
#define RX_BUF_SIZE        128

#define STACK_SIZE         1024
#define MIC_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)

static qDrvI2S_t i2sDrv = Q_DRV_I2S_INSTANCE_DEFINE(I2S_INSTANCE_ID);

static TaskHandle_t micTaskHandle;
static StaticTask_t micTaskData;
static StackType_t  micTaskStack[STACK_SIZE];

static UInt8 rxBuffer[RX_BUF_SIZE];

static UInt32 countOnes(const UInt8* buf, UInt16 len)
{
    UInt32 count = 0;
    for(UInt16 i = 0; i < len; i += 2)
    {
        UInt16 word = (UInt16)((UInt16)buf[i] | ((UInt16)buf[i + 1] << 8));
        count += (UInt32)__builtin_popcount(word);
    }
    return count;
}

static void micTask(void* pvParameters)
{
    (void)pvParameters;

    while(1)
    {
        qResult_t res = qDrvI2S_RxBufferSet(&i2sDrv, rxBuffer, RX_BUF_SIZE);
        if(res != Q_OK)
        {
            GP_LOG_SYSTEM_PRINTF("RxBufferSet failed: %d", 0, res);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        res = qDrvI2S_Start(&i2sDrv);
        if(res != Q_OK)
        {
            GP_LOG_SYSTEM_PRINTF("I2S Start failed: %d", 0, res);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        /* Count 1-bits: density near 50% = silence, deviation = sound */
        UInt32 ones = countOnes(rxBuffer, RX_BUF_SIZE);
        UInt32 totalBits = RX_BUF_SIZE * 8;
        UInt32 percent = (ones * 100) / totalBits;

        /* Distance from 50% baseline as a simple level indicator */
        UInt32 level = (percent >= 50) ? (percent - 50) : (50 - percent);

        GP_LOG_SYSTEM_PRINTF("PDM density:%u%% level:%u ones:%u/%u", 0,
                             (UInt32)percent, (UInt32)level, (UInt32)ones, (UInt32)totalBits);
    }
}

void Application_Init(void)
{
    qResult_t res;

#ifndef GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT
#error GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT must be defined
#endif
    gpBaseComps_StackInit();

    gpCom_Init();
    gpLog_Init();

    GP_LOG_SYSTEM_PRINTF("Microphone test: PDM via I2S_0 Master RX", 0);

    res = qPinCfg_Init(NULL);
    if(res != Q_OK)
    {
        GP_LOG_SYSTEM_PRINTF("qPinCfg_Init failed: %d", 0, res);
        Q_ASSERT(false);
    }

    /* Configure I2S pins: SDI=GPIO5, SCK=GPIO3, WS=GPIO2 */
    qDrvI2S_PinConfig_t pinCfg = Q_DRV_I2S_MASTER_RX_PIN_CONFIG(I2S_INSTANCE_ID,
                                                                  I2S_SDI_GPIO,
                                                                  I2S_SCK_GPIO,
                                                                  I2S_WS_GPIO);
    res = qDrvI2S_PinConfigSet(&pinCfg, qDrvI2S_ModeMasterRx);
    GP_ASSERT_SYSTEM(res == Q_OK);

    qDrvI2S_Config_t cfg = {
        .mode         = qDrvI2S_ModeMasterRx,
        .transferMode = qDrvI2S_TransferModePolling,
        .clkSrc       = qDrvI2S_ClkSrcMain,
        .prescaler    = I2S_PRESCALER,
        .wordLen      = 16,
        .wsOffset     = 1,
        .txBytes      = {.left = 0, .right = 0},
        .rxBytes      = {.left = 2, .right = 2},
    };

    res = qDrvI2S_Init(&i2sDrv, &cfg, NULL, NULL, Q_DRV_IRQ_PRIO_DEFAULT);
    GP_ASSERT_SYSTEM(res == Q_OK);

    micTaskHandle = xTaskCreateStatic(micTask,
                                      "micTask",
                                      STACK_SIZE,
                                      NULL,
                                      MIC_TASK_PRIORITY,
                                      micTaskStack,
                                      &micTaskData);
    GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, micTaskHandle != NULL);
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
