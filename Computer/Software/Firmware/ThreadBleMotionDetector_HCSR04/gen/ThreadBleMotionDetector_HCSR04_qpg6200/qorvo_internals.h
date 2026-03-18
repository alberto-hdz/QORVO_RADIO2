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
 * Thread+BLE Doorbell component manifest.
 * Based on BleDoorbell internals with OpenThread component added.
 *
 * NOTE: The OpenThread-specific components (GP_COMP_OPENTHREAD etc.) must be
 *       verified against your SDK version. Regenerate using QPG6200DK.py
 *       with openthread enabled for a production-ready component list.
 */

#ifndef _QORVO_INTERNALS_H_
#define _QORVO_INTERNALS_H_

/*
 * Enabled components - BLE (inherited from BleDoorbell)
 */

#define GP_COMP_ASSERT
#define GP_COMP_BASECOMPS
#define GP_COMP_BATTERYMONITOR
#define GP_COMP_BLE
#define GP_COMP_BLEACTIVITYMANAGER
#define GP_COMP_BLEADDRESSRESOLVER
#define GP_COMP_BLEADVERTISER
#define GP_COMP_BLECHANNELMAPMANAGER
#define GP_COMP_BLECOMPS
#define GP_COMP_BLECONFIG
#define GP_COMP_BLECONNECTIONMANAGER
#define GP_COMP_BLEDATACHANNELRXQUEUE
#define GP_COMP_BLEDATACHANNELTXQUEUE
#define GP_COMP_BLEDATACOMMON
#define GP_COMP_BLEDATARX
#define GP_COMP_BLEDATATX
#define GP_COMP_BLELLCP
#define GP_COMP_BLELLCPFRAMEWORK
#define GP_COMP_BLELLCPPROCEDURES
#define GP_COMP_COM
#define GP_COMP_ECC
#define GP_COMP_ENCRYPTION
#define GP_COMP_FREERTOS
#define GP_COMP_GPHAL
#define GP_COMP_GPHAL_BLE
#define GP_COMP_GPHAL_MAC
#define GP_COMP_GPHAL_PBM
#define GP_COMP_GPHAL_RADIO
#define GP_COMP_GPHAL_SEC
#define GP_COMP_HALAPPUCSUBSYSTEM
#define GP_COMP_HALCORTEXM4
#define GP_COMP_HALPLATFORM
#define GP_COMP_HCI
#define GP_COMP_LOG
#define GP_COMP_MACCORE
#define GP_COMP_MACDISPATCHER
#define GP_COMP_NVM
#define GP_COMP_PAD
#define GP_COMP_PD
#define GP_COMP_PERIPHERAL
#define GP_COMP_POOLMEM
#define GP_COMP_QORVOBLEHOST
#define GP_COMP_RADIO
#define GP_COMP_RANDOM
#define GP_COMP_RAP
#define GP_COMP_REGMAP
#define GP_COMP_RESET
#define GP_COMP_RTIL
#define GP_COMP_RT_NRT_COMMON
#define GP_COMP_RXARBITER
#define GP_COMP_SCHED
#define GP_COMP_UPGRADE
#define GP_COMP_UTILS
#define GP_COMP_VERSION
#define GP_COMP_WMRK

/*
 * Enabled components - Thread (OpenThread)
 * NOTE: Verify exact component names against your SDK/OpenThread library version.
 *       These are the expected components based on the QPG6200 Thread+BLE stack.
 */

#define GP_COMP_OPENTHREAD


/*
 * Components numeric ids (inherited from BleDoorbell)
 */

#define GP_COMPONENT_ID_APP                                       1
#define GP_COMPONENT_ID_ASSERT                                    29
#define GP_COMPONENT_ID_BASECOMPS                                 35
#define GP_COMPONENT_ID_BATTERYMONITOR                            34
#define GP_COMPONENT_ID_BLE                                       154
#define GP_COMPONENT_ID_BLEACTIVITYMANAGER                        228
#define GP_COMPONENT_ID_BLEADDRESSRESOLVER                        214
#define GP_COMPONENT_ID_BLEADVERTISER                             215
#define GP_COMPONENT_ID_BLECHANNELMAPMANAGER                      74
#define GP_COMPONENT_ID_BLECOMPS                                  216
#define GP_COMPONENT_ID_BLECONFIG                                 217
#define GP_COMPONENT_ID_BLECONNECTIONMANAGER                      75
#define GP_COMPONENT_ID_BLEDATACHANNELRXQUEUE                     218
#define GP_COMPONENT_ID_BLEDATACHANNELTXQUEUE                     219
#define GP_COMPONENT_ID_BLEDATACOMMON                             220
#define GP_COMPONENT_ID_BLEDATARX                                 221
#define GP_COMPONENT_ID_BLEDATATX                                 222
#define GP_COMPONENT_ID_BLEINITIATOR                              223
#define GP_COMPONENT_ID_BLELLCP                                   224
#define GP_COMPONENT_ID_BLELLCPFRAMEWORK                          225
#define GP_COMPONENT_ID_BLELLCPPROCEDURES                         226
#define GP_COMPONENT_ID_BLEPRESCHED                               234
#define GP_COMPONENT_ID_BSP                                       8
#define GP_COMPONENT_ID_COM                                       10
#define GP_COMPONENT_ID_ECC                                       192
#define GP_COMPONENT_ID_ENCRYPTION                                124
#define GP_COMPONENT_ID_FREERTOS                                  24
#define GP_COMPONENT_ID_GPHAL                                     7
#define GP_COMPONENT_ID_HALAPPUCSUBSYSTEM                         6
#define GP_COMPONENT_ID_HALCORTEXM4                               6
#define GP_COMPONENT_ID_HALPLATFORM                               6
#define GP_COMPONENT_ID_HCI                                       156
#define GP_COMPONENT_ID_LOG                                       11
#define GP_COMPONENT_ID_MACCORE                                   109
#define GP_COMPONENT_ID_MACDISPATCHER                             114
#define GP_COMPONENT_ID_NVM                                       32
#define GP_COMPONENT_ID_OPENTHREAD                                240
#define GP_COMPONENT_ID_PAD                                       126
#define GP_COMPONENT_ID_PD                                        104
#define GP_COMPONENT_ID_PERIPHERAL                                83
#define GP_COMPONENT_ID_POOLMEM                                   106
#define GP_COMPONENT_ID_QORVOBLEHOST                              185
#define GP_COMPONENT_ID_RADIO                                     204
#define GP_COMPONENT_ID_RANDOM                                    108
#define GP_COMPONENT_ID_RAP                                       80
#define GP_COMPONENT_ID_REGMAP                                    82
#define GP_COMPONENT_ID_RESET                                     33
#define GP_COMPONENT_ID_RTIL                                      66
#define GP_COMPONENT_ID_RT_NRT_COMMON                             -1
#define GP_COMPONENT_ID_RXARBITER                                 2
#define GP_COMPONENT_ID_SCHED                                     9
#define GP_COMPONENT_ID_STAT                                      22
#define GP_COMPONENT_ID_UPGRADE                                   115
#define GP_COMPONENT_ID_UTILS                                     4
#define GP_COMPONENT_ID_VERSION                                   129
#define GP_COMPONENT_ID_WMRK                                      51


/*
 * Component: gpBaseComps
 */

#define GP_BASECOMPS_DIVERSITY_NO_GPCOM_INIT
#define GP_BASECOMPS_DIVERSITY_NO_GPLOG_INIT
#define GP_BASECOMPS_DIVERSITY_NO_GPSCHED_INIT


/*
 * Component: gpBleComps
 */

#define GP_DIVERSITY_BLE_ACL_CONNECTIONS_SUPPORTED
#define GP_DIVERSITY_BLE_CONNECTIONS_SUPPORTED
#define GP_DIVERSITY_BLE_LEGACY_ADVERTISING
#define GP_DIVERSITY_BLE_LEGACY_ADVERTISING_FEATURE_PRESENT
#define GP_DIVERSITY_BLE_PERIPHERAL


/*
 * Component: gpBsp
 */

#define GP_DIVERSITY_QPG6200_QFN32_DK


/*
 * Component: gpCom
 */

#define GP_COM_DIVERSITY_ACTIVATE_TX_CALLBACK
#define GP_COM_DIVERSITY_SERIAL_NO_SYN_NO_CRC
#define GP_COM_DIVERSITY_SERIAL_NO_SYN_SENDTO_ID                  1
#define GP_COM_DIVERSITY_SERIAL_SYN_DISABLED
#define GP_COM_ZERO_COPY_BLOCK_TRANSFERS


/*
 * Component: gpFreeRTOS
 */

#define GP_DIVERSITY_FREERTOS
#define GP_FREERTOS_DIVERSITY_HEAP
#define GP_FREERTOS_DIVERSITY_SLEEP
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
#define GP_HAL_DIVERSITY_DUTY_CYCLE
#define GP_HAL_DIVERSITY_INCLUDE_IPC
#define GP_HAL_DIVERSITY_LEGACY_ADVERTISING
#define GP_HAL_DIVERSITY_RAW_ENHANCED_ACK_RX
#define GP_HAL_DIVERSITY_RAW_FRAME_ENCRYPTION
#define GP_HAL_INCLUDE_INFO_PAGE_HELPERS


/*
 * Component: gpHci
 */

#define GP_HCI_DIVERSITY_HOST_SERVER


/*
 * Component: gpLog
 */

#define GP_LOG_DIVERSITY_DEVELOPMENT_FORCE_FLUSH
#define GP_LOG_DIVERSITY_NO_TIME_NO_COMPONENT_ID
#define GP_LOG_DIVERSITY_VSNPRINTF


/*
 * Component: gpMacCore
 */

#define GP_MACCORE_DIVERSITY_ASSOCIATION_ORIGINATOR
#define GP_MACCORE_DIVERSITY_ASSOCIATION_RECIPIENT
#define GP_MACCORE_DIVERSITY_FFD
#define GP_MACCORE_DIVERSITY_INDIRECT_TRANSMISSION
#define GP_MACCORE_DIVERSITY_POLL_ORIGINATOR
#define GP_MACCORE_DIVERSITY_POLL_RECIPIENT
#define GP_MACCORE_DIVERSITY_RAW_FRAMES
#define GP_MACCORE_DIVERSITY_REGIONALDOMAINSETTINGS
#define GP_MACCORE_DIVERSITY_RX_WINDOWS
#define GP_MACCORE_DIVERSITY_SCAN_ACTIVE_ORIGINATOR
#define GP_MACCORE_DIVERSITY_SCAN_ACTIVE_RECIPIENT
#define GP_MACCORE_DIVERSITY_SCAN_ED_ORIGINATOR
#define GP_MACCORE_DIVERSITY_SCAN_ORIGINATOR
#define GP_MACCORE_DIVERSITY_SCAN_ORPHAN_ORIGINATOR
#define GP_MACCORE_DIVERSITY_SCAN_ORPHAN_RECIPIENT
#define GP_MACCORE_DIVERSITY_SCAN_RECIPIENT
#define GP_MACCORE_DIVERSITY_SECURITY_ENABLED
#define GP_MACCORE_DIVERSITY_THREAD_1_2
#define GP_MACCORE_DIVERSITY_TIMEDTX


/*
 * Component: gpMacDispatcher
 */

#define GP_MACDISPATCHER_DIVERSITY_SINGLE_STACK_FUNCTIONS


/*
 * Component: gpNvm
 */

#define GP_DATA_SECTION_NAME_NVM                                  gpNvm
#define GP_DATA_SECTION_START_NVM                                 -0x6000
#define GP_DIVERSITY_NVM
#define GP_NVM_DIVERSITY_ELEMENT_IF
#define GP_NVM_DIVERSITY_ELEMIF_KEYMAP
#define GP_NVM_DIVERSITY_SUBPAGEDFLASH_V2
#define GP_NVM_DIVERSITY_TAG_IF
#define GP_NVM_DIVERSITY_USE_POOLMEM
#define GP_NVM_DIVERSITY_VARIABLE_SETTINGS
#define GP_NVM_DIVERSITY_VARIABLE_SIZE
#define GP_NVM_USE_ASSERT_SAFETY_NET


/*
 * Component: gpPd
 */

#define GP_DIVERSITY_PD_USE_PBM_VARIANT


/*
 * Component: gpRandom
 */

#define GP_RANDOM_DIVERSITY_CTR_DRBG


/*
 * Component: gpSched
 */

#define GP_SCHED_DIVERSITY_RUN_ON_SCHED
#define GP_SCHED_DIVERSITY_SCHEDULE_INSECONDSAPI
#define GP_SCHED_DIVERSITY_SLEEP
#define GP_SCHED_DIVERSITY_USE_ARGS


/*
 * Component: gpUpgrade
 */

#define GP_APP_DIVERSITY_SIGNED_BOOTLOADER
#define GP_DIVERSITY_FLASH_APP_START_OFFSET                       0xC400
#define GP_DIVERSITY_UPGRADE_IMAGE_USER_LICENSE_OFFSET            0xC000
#define GP_UPGRADE_DIVERSITY_COMPRESSION


/*
 * Component: gpUtils
 */

#define GP_DIVERSITY_UTILS_MATH
#define GP_UTILS_DIVERSITY_CRC_TABLE
#define GP_UTILS_DIVERSITY_LINKED_LIST


/*
 * Component: halCortexM4
 */

#define GP_BSP_CONTROL_WDT_TIMER
#define GP_DIVERSITY_FLASH_BL_SIZE                                0x8000
#define GP_HAL_NVM_APPUC_OFFSET                                   77824
#define GP_KX_FLASH_SIZE                                          2048
#define GP_KX_HEAP_SIZE                                           (8 * 1024)
#define GP_KX_SYSRAM_SIZE                                         32
#define GP_KX_UCRAM_SIZE                                          192
#define HAL_DIVERSITY_PWMXL
#define HAL_DIVERSITY_SEMAILBOX
#define HAL_DIVERSITY_UART
#define HAL_DIVERSITY_WATCHDOG_TIMEOUT_INTERRUPT_ENABLED
#define QPG6200


/*
 * Component: qorvoBleHost
 */

#define CORDIO_BLEHOST_DIVERSITY_HCI_INTERNAL
#define CORDIO_BLE_HOST_ATT_SERVER
#define CORDIO_BLE_HOST_EXCLUDE_CORDIOAPPFW
#define CORDIO_BLE_HOST_EXCLUDE_SMPR
#define CORDIO_BLE_HOST_PROFILES_ORIG_SERVPROF
#define WSF_ASSERT_ENABLED                                        TRUE


/*
 * Other flags
 */

#define GP_APP_DIVERSITY_BLE
#define GP_APP_DIVERSITY_BUTTONHANDLER
#define GP_APP_DIVERSITY_THREAD
#define GP_COM_DIVERSITY_NO_RX
#define GP_DIVERSITY_APP_WITH_SECURE_BOOT_HEADER
#define GP_DIVERSITY_BOOTLOADER
#define GP_DIVERSITY_CORTEXM4
#define GP_DIVERSITY_DEVELOPMENT
#define GP_DIVERSITY_GPHAL_INDIRECT_TRANSMISSION
#define GP_DIVERSITY_GPHAL_XP4002
#define GP_DIVERSITY_LOG
#define GP_DIVERSITY_NR_OF_STACKS                                 3
#define GP_DIVERSITY_OPENTHREAD
#define GP_DIVERSITY_REGMAPS_IN_HAL_PLATFORM
#define GP_DIVERSITY_RT_SYSTEM_IN_FLASH_VERSION                   0
#define GP_GIT_DESCRIBE                                           "04ead2973ae6"
#define GP_GIT_SHA                                                04ead2973ae65c6b4e16a40f57c165f1fa6955ba
#define GP_GIT_SHA_SHORT                                          04ead29
#define GP_HAL_ES_ABS_EVENT_NMBR_OF_EVENTS                        16
#define GP_MACCORE_DIVERSITY_DIAGCNTRS
#define GP_POOLMEM_DIVERSITY_MALLOC
#define GP_RX_ARBITER_DUTY_CYCLE
#define HAL_DEFAULT_GOTOSLEEP_THRES                               10000
#define HAL_DIVERSITY_SLEEP
#define HAL_TWI_CLK_SPEED                                         100000
#define HAL_UART_NO_RX
#define QORVO_QPINCFG_ENABLE
#define QPINCFG_BOARD_FILENAME                                    "qPinCfg_IoTCarrierBoard_qpg6200.h"

#endif //_QORVO_INTERNALS_H_
