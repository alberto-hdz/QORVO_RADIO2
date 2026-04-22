/*
 * Copyright (c) 2020, Qorvo Inc
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
 */

/** @file "qorvo_internals.h"
 *
 */

#ifndef _QORVO_INTERNALS_H_
#define _QORVO_INTERNALS_H_

/*
 * Enabled components
 */

#define GP_COMP_ASSERT
#define GP_COMP_BASECOMPS
#define GP_COMP_BSP
#define GP_COMP_COM
#define GP_COMP_FREERTOS
#define GP_COMP_GPHAL
#define GP_COMP_GPHAL_PBM
#define GP_COMP_HALAPPUCSUBSYSTEM
#define GP_COMP_HALCORTEXM4
#define GP_COMP_HALPLATFORM
#define GP_COMP_LOG
#define GP_COMP_PERIPHERAL
#define GP_COMP_RANDOM
#define GP_COMP_RAP
#define GP_COMP_REGMAP
#define GP_COMP_RESET
#define GP_COMP_RT_NRT_COMMON
#define GP_COMP_SCHED
#define GP_COMP_UPGRADE
#define GP_COMP_UTILS
#define GP_COMP_VERSION

/*
 * Components numeric ids
 */

#define GP_COMPONENT_ID_APP                                1
#define GP_COMPONENT_ID_ASSERT                             29
#define GP_COMPONENT_ID_BASECOMPS                          35
#define GP_COMPONENT_ID_BSP                                8
#define GP_COMPONENT_ID_COM                                10
#define GP_COMPONENT_ID_FREERTOS                           24
#define GP_COMPONENT_ID_GPHAL                              7
#define GP_COMPONENT_ID_HALAPPUCSUBSYSTEM                  6
#define GP_COMPONENT_ID_HALCORTEXM4                        6
#define GP_COMPONENT_ID_HALPLATFORM                        6
#define GP_COMPONENT_ID_LOG                                11
#define GP_COMPONENT_ID_PAD                                126
#define GP_COMPONENT_ID_PD                                 104
#define GP_COMPONENT_ID_PERIPHERAL                         83
#define GP_COMPONENT_ID_RANDOM                             108
#define GP_COMPONENT_ID_RAP                                80
#define GP_COMPONENT_ID_REGMAP                             82
#define GP_COMPONENT_ID_RESET                              33
#define GP_COMPONENT_ID_RT_NRT_COMMON                      -1
#define GP_COMPONENT_ID_SCHED                              9
#define GP_COMPONENT_ID_STAT                               22
#define GP_COMPONENT_ID_UPGRADE                            115
#define GP_COMPONENT_ID_UTILS                              4
#define GP_COMPONENT_ID_VERSION                            129

/*
 * Component: gpBaseComps
 */

#define GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT

/*
 * Component: gpBsp
 */

#define GP_DIVERSITY_QPG6200_QFN32_DK_MINIMAL

/*
 * Component: gpCom
 */

#define GP_COM_DIVERSITY_SERIAL_NO_SYN_NO_CRC
#define GP_COM_DIVERSITY_SERIAL_NO_SYN_SENDTO_ID           1
#define GP_COM_DIVERSITY_SERIAL_SYN_DISABLED
#define GP_COM_ZERO_COPY_BLOCK_TRANSFERS

/*
 * Component: gpFreeRTOS
 */

#define GP_DIVERSITY_FREERTOS
#define GP_FREERTOS_DIVERSITY_STATIC_ALLOC

/*
 * Component: gphal
 */

#define GP_COMP_GPHAL_ES
#define GP_COMP_GPHAL_ES_ABS_EVENT
#define GP_COMP_GPHAL_ES_EXT_EVENT
#define GP_COMP_GPHAL_ES_REL_EVENT
#define GP_DIGITAL_COEX
#define GP_DIVERSITY_GPHAL_INTERN
#define GP_DIVERSITY_GPHAL_OSCILLATOR_BENCHMARK
#define GP_DIVERSITY_RT_SYSTEM_ADDR_FROM_LINKERSCRIPT
#define GP_DIVERSITY_RT_SYSTEM_BLEMGR_IN_FLASH
#define GP_DIVERSITY_RT_SYSTEM_GENERATE
#define GP_DIVERSITY_RT_SYSTEM_IN_FLASH
#define GP_DIVERSITY_RT_SYSTEM_PARTS_IN_FLASH
#define GP_HAL_DIVERSITY_INCLUDE_IPC
#define GP_HAL_INCLUDE_INFO_PAGE_HELPERS

/*
 * Component: gpLog
 */

#define GP_LOG_DIVERSITY_DEVELOPMENT_FORCE_FLUSH
#define GP_LOG_DIVERSITY_NO_TIME_NO_COMPONENT_ID
#define GP_LOG_DIVERSITY_VSNPRINTF

/*
 * Component: gpPd
 */

#define GP_DIVERSITY_PD_USE_PBM_VARIANT

/*
 * Component: gpUpgrade
 */

#define GP_APP_DIVERSITY_SIGNED_BOOTLOADER
#define GP_DIVERSITY_FLASH_APP_START_OFFSET                0xC400
#define GP_DIVERSITY_UPGRADE_IMAGE_USER_LICENSE_OFFSET     0xC000
#define GP_UPGRADE_DIVERSITY_COMPRESSION

/*
 * Component: gpUtils
 */

#define GP_UTILS_DIVERSITY_LINKED_LIST

/*
 * Component: halCortexM4
 */

#define GP_BSP_CONTROL_WDT_TIMER
#define GP_DIVERSITY_FLASH_BL_SIZE                         0x8000
#define GP_HAL_NVM_APPUC_OFFSET                            77824
#define GP_KX_FLASH_SIZE                                   2048
#define GP_KX_SYSRAM_SIZE                                  32
#define GP_KX_UCRAM_SIZE                                   192
#define HAL_DIVERSITY_SEMAILBOX
#define HAL_DIVERSITY_UART
#define QPG6200

/*
 * Other flags
 */

#define GP_BLE_NR_OF_CONNECTION_COMPLETE_EVENT_BUFFERS     0
#define GP_BLE_NR_OF_SUPPORTED_PROCEDURES                  0
#define GP_BLE_NR_OF_SUPPORTED_PROCEDURE_CALLBACKS         0
#define GP_DIVERSITY_APP_WITH_SECURE_BOOT_HEADER
#define GP_DIVERSITY_BOOTLOADER
#define GP_DIVERSITY_CORTEXM4
#define GP_DIVERSITY_GPHAL_XP4002
#define GP_DIVERSITY_LOG
#define GP_DIVERSITY_REGMAPS_IN_HAL_PLATFORM
#define GP_DIVERSITY_RT_SYSTEM_IN_FLASH_VERSION            0
#define GP_GIT_DESCRIBE                                    "04ead2973ae6"
#define GP_GIT_SHA                                         04ead2973ae65c6b4e16a40f57c165f1fa6955ba
#define GP_GIT_SHA_SHORT                                   04ead29
#define GP_HAL_ES_ABS_EVENT_NMBR_OF_EVENTS                 2
#define QORVO_QPINCFG_ENABLE
#define QPINCFG_BOARD_FILENAME                             "qPinCfg_IoTCarrierBoard_qpg6200.h"

#endif //_QORVO_INTERNALS_H_
