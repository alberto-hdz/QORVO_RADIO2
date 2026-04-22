/*
 * Copyright (c) 2025, Qorvo Inc
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#ifndef _QORVO_CONFIG_H_
#define _QORVO_CONFIG_H_

#define GP_CHANGELIST                                      0
#define GP_VERSIONINFO_APP                                 mictest_QPG6200DK
#define GP_VERSIONINFO_BASE_COMPS                          0,0,0,0
#define GP_VERSIONINFO_BLE_COMPS                           0,0,0,0
#define GP_VERSIONINFO_DATE                                2025-10-20
#define GP_VERSIONINFO_GLOBAL_VERSION                      0,0,0,0
#define GP_VERSIONINFO_HOST                                UNKNOWN
#define GP_VERSIONINFO_PROJECT                             R005_PeripheralLib
#define GP_VERSIONINFO_USER                                UNKNOWN@UNKNOWN

/*
 * Component: gpBsp
 */
#define GP_BSP_FILENAME                                    "gpBsp_QPG6200_QFN32_DK_IOT_debug.h"
#define GP_BSP_UART_COM_BAUDRATE                           115200

/*
 * Component: gpCom
 */
#define GP_COM_DIVERSITY_SERIAL
#define GP_COM_MAX_NUMBER_OF_MODULE_IDS                    2
#define GP_COM_MAX_PACKET_PAYLOAD_SIZE                     250
#define GP_DIVERSITY_COM_UART

/*
 * Component: gpPeripheral
 */
#define HAL_DIVERSITY_V2
#define Q_DRV_IRQ_PRIO_HIGH                                configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

/*
 * Component: gpSched
 */
#define GP_SCHED_EXTERNAL_MAIN
#define GP_SCHED_NR_OF_IDLE_CALLBACKS                      0

/*
 * Component: halCortexM4
 */
#define GP_DIVERSITY_APPUC_FW_HEADER
#define GP_DIVERSITY_CUSTOM_APPUC_FW_HEADER
#define GP_DIVERSITY_RT_SYSTEM_MAX_FLASH_SIZE              0x14000
#define HAL_MUTEX_SUPPORTED

#include "qorvo_internals.h"

#endif //_QORVO_CONFIG_H_
