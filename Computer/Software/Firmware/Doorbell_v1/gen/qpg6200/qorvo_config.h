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

/** @file "qorvo_config.h"
 *
 */

#ifndef _QORVO_CONFIG_H_
#define _QORVO_CONFIG_H_

/*
 * Version info
 */

#define GP_CHANGELIST                                             0
#define GP_VERSIONINFO_APP                                        HelloWorld_qpg6200
#define GP_VERSIONINFO_BASE_COMPS                                 0,0,0,0
#define GP_VERSIONINFO_BLE_COMPS                                  0,0,0,0
#define GP_VERSIONINFO_DATE                                       2025-10-20
#define GP_VERSIONINFO_GLOBAL_VERSION                             0,0,0,0
#define GP_VERSIONINFO_HOST                                       UNKNOWN
#define GP_VERSIONINFO_PROJECT                                    P346_IoT_SDK
#define GP_VERSIONINFO_USER                                       UNKNOWN@UNKNOWN


/*
 * Component: gpBle
 */

/* Buffer size for HCI ACL RX packets */
#define GP_BLE_DIVERSITY_HCI_BUFFER_SIZE_RX                       255

/* Buffer size for HCI ACL TX packets */
#define GP_BLE_DIVERSITY_HCI_BUFFER_SIZE_TX                       255

/* WcBleHost will be calling gpBle_ExecuteCommand */
#define GP_DIVERSITY_BLE_EXECUTE_CMD_WCBLEHOST


/*
 * Component: gpBleComps
 */

/* The amount of dedicated connection complete buffers */
#define GP_BLE_NR_OF_CONNECTION_COMPLETE_EVENT_BUFFERS            1


/*
 * Component: gpBleConfig
 */

/* The amount of LLCP procedures that are supported */
#define GP_BLE_NR_OF_SUPPORTED_PROCEDURES                         5

/* Bluetooth spec version (LL and HCI) */
#define GP_DIVERSITY_BLECONFIG_VERSION_ID                         gpBleConfig_BleVersionId_5_4


/*
 * Component: gpBleLlcpFramework
 */

/* The amount of LLCP procedures callbacks that are supported */
#define GP_BLE_NR_OF_SUPPORTED_PROCEDURE_CALLBACKS                0


/*
 * Component: gpBsp
 */

/* Contains filename of BSP header file to include */
#define GP_BSP_FILENAME                                           "gpBsp_QPG6200_QFN32_DK_LED_PB_10dBm.h"

/* UART baudrate */
#define GP_BSP_UART_COM_BAUDRATE                                  115200


/*
 * Component: gpCom
 */

/* Enable SYN datastream encapsulation */
#define GP_COM_DIVERSITY_SERIAL

#define GP_COM_MAX_PACKET_PAYLOAD_SIZE                            250

/* Use UART for COM - defined as default in code */
#define GP_DIVERSITY_COM_UART


/*
 * Component: gphal
 */

/* Amount of 64-bit long IEEE addresses entries to keep data pending for */
#define GPHAL_DP_LONG_LIST_MAX                                    10

/* Amount of 16-bit short address entries to keep data pending for */
#define GPHAL_DP_SHORT_LIST_MAX                                   10

/* Number of entries in the whitelist (minimum=GP_HAL_BLE_MAX_NR_OF_WL_CONTROLLER_SPECIFIC_ENTRIES) */
#define GP_DIVERSITY_BLE_MAX_NR_OF_FILTER_ACCEPT_LIST_ENTRIES     3

/* Max BLE connections supported */
#define GP_DIVERSITY_BLE_MAX_NR_OF_SUPPORTED_CONNECTIONS          1

/* Max BLE slave connections supported */
#define GP_DIVERSITY_BLE_MAX_NR_OF_SUPPORTED_SLAVE_CONNECTIONS    1

/* Number of PBMs of first supported size */
#define GP_HAL_PBM_TYPE1_AMOUNT                                   14

/* Number of PBMs of second supported size */
#define GP_HAL_PBM_TYPE2_AMOUNT                                   14


/*
 * Component: gpLog
 */

/* overrule log string length from application code */
#define GP_LOG_MAX_LEN                                            256


/*
 * Component: gpMacCore
 */

/* Amount of indirect Tx packets kept by component */
#define GP_MACCORE_INDTX_ENTRIES                                  10

/* Number of known Neighbours for use with indirect transmission */
#define GP_MACCORE_MAX_NEIGHBOURS                                 10


/*
 * Component: gpNvm
 */

/* maximal length of a token used */
#define GP_NVM_MAX_TOKENLENGTH                                    32

/* Used to overwrite the amount of handles for LUT */
#define GP_NVM_NBR_OF_LOOKUPTABLE_HANDLES                         4

/* number of redundant copies (default 1 (indicating no copy) expect 2 for K8a builds) */
#define GP_NVM_NBR_OF_REDUNDANT_SECTORS                           1

/* Maximum number of unique tags in each pool. Used for memory allocation at Tag level API */
#define GP_NVM_NBR_OF_UNIQUE_TAGS                                 128

/* Maximum number of tokens tracked by token API */
#define GP_NVM_NBR_OF_UNIQUE_TOKENS                               230

#define GP_NVM_TYPE                                               6


/*
 * Component: gpPeripheral
 */

/* Enable gpPeripheral module. */
#define HAL_DIVERSITY_V2

/* High interrupt priority level value. */
#define Q_DRV_IRQ_PRIO_HIGH                                       configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY


/*
 * Component: gpSched
 */

#define GP_SCHED_EVENT_LIST_SIZE                                  92

/* Don't include the implementation for our mainloop MAIN_FUNCTION_NAME */
#define GP_SCHED_EXTERNAL_MAIN

/* Callback after every main loop iteration. */
#define GP_SCHED_NR_OF_IDLE_CALLBACKS                             2

/* Callback before every main loop iteration. */
#define GP_SCHED_NR_OF_PREPROCESS_IDLE_CALLBACKS                  1

/* Change the name of the main eventloop implementation */
#define MAIN_FUNCTION_NAME                                        main


/*
 * Component: halCortexM4
 */

/* Use application header */
#define GP_DIVERSITY_APPUC_FW_HEADER

/* Override application header in the application */
#define GP_DIVERSITY_CUSTOM_APPUC_FW_HEADER

/* Maximum size reserved for RT in flash. */
#define GP_DIVERSITY_RT_SYSTEM_MAX_FLASH_SIZE                     0x14000

/* set custom stack size */
#define GP_KX_STACK_SIZE                                          512

/* Select GPIO level interrupt code */
#define HAL_DIVERSITY_GPIO_INTERRUPT

/* Set if hal has real mutex capability. Used to skip even disabling/enabling global interrupts. */
#define HAL_MUTEX_SUPPORTED


/*
 * Component: qorvoBleHost
 */

/* The MTU size */
#define CORDIO_BLE_HOST_ATT_MAX_MTU                               GP_BLE_DIVERSITY_HCI_BUFFER_SIZE_RX

/* MTU is configured at runtime */
#define CORDIO_BLE_HOST_ATT_MTU_CONFIG_AT_RUNTIME

/* Number of chuncks WSF Poolmem Chunk 1 */
#define CORDIO_BLE_HOST_BUFPOOLS_1_AMOUNT                         3

/* Number of chuncks WSF Poolmem Chunk 2 */
#define CORDIO_BLE_HOST_BUFPOOLS_2_AMOUNT                         4

/* Number of chuncks WSF Poolmem Chunk 3 */
#define CORDIO_BLE_HOST_BUFPOOLS_3_AMOUNT                         2

/* Number of chuncks WSF Poolmem Chunk 4 */
#define CORDIO_BLE_HOST_BUFPOOLS_4_AMOUNT                         2

/* Number of chuncks WSF Poolmem Chunk 5 */
#define CORDIO_BLE_HOST_BUFPOOLS_5_AMOUNT                         2

/* Number of WSF Poolmem Chunks in use */
#define CORDIO_BLE_HOST_WSF_BUF_POOLS                             5

/* Cordio define - checked here */
#define DM_CONN_MAX                                               1

/* Memory freeing violation check */
#define WSF_BUF_FREE_CHECK                                        TRUE


#include "qorvo_internals.h"

#endif //_QORVO_CONFIG_H_
