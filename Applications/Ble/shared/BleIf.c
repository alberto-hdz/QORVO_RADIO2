/*
 * Copyright (c) 2024-2025, Qorvo Inc
 *
 *
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

/** @file "AppManager.cpp"
 */

#include "gpBleComps.h"
#include "gpLog.h"
#include "hal.h"
#include "gpSched_runOn.h"

#include "wsf_types.h"
#include "wsf_msg.h"
#include "dm_api.h"

#include "bstream.h"
#include "gpHci_types.h"
#include "cordioBleHost.h"
#include "dm_main.h"
#include "dm_conn.h"
#include "svc_ch.h"
#include "l2c_defs.h"
#include "BleIf.h"
#include "Ble_Peripheral_Config.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_BLE

/*!< \brief RunOn API success return value */
#define RUNONSTATUS_OK (0)

// Timeout for BLE stack to start advertising
#define BLEIF_DM_ADV_START_TIMEOUT (1000)

/*!< \brief Definition of internal state of BLE interface */
typedef enum {
    BLE_STATE_NOT_INITIALIZED,
    BLE_STATE_INIT_IN_PROGRESS,
    BLE_STATE_IDLE,
    BLE_STATE_CONNECTED,
    BLE_STATE_ADV_STARTED,
} BleIf_State_t;

/*!< \brief Params for internal API */
typedef struct {
    uint8_t u8param1;
    uint8_t u8param2;
    uint16_t u16param1;
    uint16_t u16param2;
    uint16_t u16param3;
    uint32_t ptrParam; // all pointer types on this uC are 4 bytes, so we can aggregate
    Status_t outParam;
} runOnParam_t;

static wsfHandlerId_t BleIf_MsgHandlerId;

static volatile BleIf_State_t BleIf_State = BLE_STATE_NOT_INITIALIZED;
static BleIf_Callbacks_t BleIf_IntCb;

// Settings for CCC descriptors
extern attsCccSet_t BleIf_CccSet[NUM_CCC_IDX];

// Service groups, incl. GATT table for all services and characteristics
extern attsGroup_t svcGroups[BLE_CONFIG_SVC_GROUPS];

static uint32_t BleIf_SetAdvData_Internal(void* arg);
static uint32_t BleIf_SetAdvInterval_Internal(void* arg);

uint8_t BleIf_ReadCallback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset, attsAttr_t* pAttr)
{
    if(BleIf_IntCb.chrReadCallback != NULL)
    {
        BleIf_IntCb.chrReadCallback(connId, handle, operation, offset, (BleIf_Attr_t*)pAttr);
    }

    return ATT_SUCCESS;
}

/*! Attribute group write callback */
static uint8_t BleIf_WriteCallback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset, uint16_t len,
                                   uint8_t* pValue, attsAttr_t* pAttr)
{
    if(BleIf_IntCb.chrWriteCallback != NULL)
    {
        BleIf_IntCb.chrWriteCallback(connId, handle, operation, offset, len, pValue, (BleIf_Attr_t*)pAttr);
    }

    return ATT_SUCCESS;
}

static void BleIf_ATTS_CccCallback(attsCccEvt_t* pEvt)
{
    if((pEvt->handle != 0) && (BleIf_IntCb.cccCallback != NULL))
    {
        BleIf_IntCb.cccCallback((BleIf_AttsCccEvt_t*)pEvt);
    }
}

/* Device manager callback */
static void BleIf_DmCallback(dmEvt_t* pDmEvt)
{
    dmEvt_t* pMsg;
    uint16_t len;

    GP_ASSERT_SYSTEM(pDmEvt);

    len = DmSizeOfEvt(pDmEvt);

    switch(pDmEvt->hdr.event)
    {
        case DM_RESET_CMPL_IND:
        {
#if(CORDIO_BLE_HOST_ATT_MAX_MTU > ATT_DEFAULT_MTU)
            uint16_t rxAclLen;

            rxAclLen = HciGetMaxRxAclLen();
            if(CORDIO_BLE_HOST_ATT_MAX_MTU > (rxAclLen - L2C_HDR_LEN))
            {
                HciSetMaxRxAclLen(CORDIO_BLE_HOST_ATT_MAX_MTU + L2C_HDR_LEN);
            }
#endif // CORDIO_BLE_HOST_ATT_MAX_MTU > ATT_DEFAULT_MTU

            // Populate hash correctly
            AttsCalculateDbHash();
            // BLE stack initialized
            BleIf_State = BLE_STATE_IDLE;
            break;
        }
        case DM_ADV_START_IND:
        {
            BleIf_State = BLE_STATE_ADV_STARTED;
            break;
        }
        // on connection we automatically stop advertising, but no DM_ADV_STOP_IND is sent
        case DM_CONN_OPEN_IND:
            BleIf_State = BLE_STATE_CONNECTED;
            break;
        case DM_ADV_STOP_IND:
        case BLEIF_DM_CONN_CLOSE_IND:
        {
            BleIf_State = BLE_STATE_IDLE;
            break;
        }
        default:
            break;
    }

    if((pMsg = WsfMsgAlloc(len)) != NULL)
    {
        MEMCPY(pMsg, pDmEvt, len);
        WsfMsgSend(BleIf_MsgHandlerId, pMsg);
    }
}

/* ATT callback */
static void BleIf_AttCallback(attEvt_t* pAttEvt)
{
    attEvt_t* pMsg;
    GP_ASSERT_SYSTEM(pAttEvt);

    if((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pAttEvt->valueLen)) != NULL)
    {
        MEMCPY(pMsg, pAttEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t*)(pMsg + 1);
        MEMCPY(pMsg->pValue, pAttEvt->pValue, pAttEvt->valueLen);
        WsfMsgSend(BleIf_MsgHandlerId, pMsg);
    }
}

void AppServerConnCallback(dmEvt_t* pDmEvt)
{
    // appDbHdl_t  dbHdl;
    dmConnId_t connId = (dmConnId_t)pDmEvt->hdr.param;
    GP_ASSERT_SYSTEM(pDmEvt);

    if(pDmEvt->hdr.event == DM_CONN_OPEN_IND)
    {
        /* set up CCC table with uninitialized (all zero) values. */
        AttsCccInitTable(connId, NULL);

        /* set CSF values to default */
        AttsCsfConnOpen(connId, TRUE, NULL);

        /* set peer's data signing info */
        // appServerSetSigningInfo(connId);
    }
    else if(pDmEvt->hdr.event == DM_SEC_PAIR_CMPL_IND)
    {
        /* set peer's data signing info */
        // appServerSetSigningInfo(connId);
    }
    else if(pDmEvt->hdr.event == DM_CONN_CLOSE_IND)
    {
        /* clear CCC table on connection close */
        AttsCccClearTable(connId);
    }
}

static void BleIf_StackCallback(void* pMsg, uint32_t ulParameter2)
{
    BleIf_IntCb.stackCallback((BleIf_MsgHdr_t*)pMsg);
}

/* Process All Messages */
static void BleIf_MsgHandler(wsfEventMask_t event, wsfMsgHdr_t* pMsg)
{
    if(pMsg == NULL)
    {
        return;
    }

    if(BleIf_IntCb.stackCallback != NULL)
    {
        if((pMsg->event == DM_ADV_START_IND) || (pMsg->event == DM_ADV_STOP_IND))
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            /* process the callback on the timer thread instead of BLE thread */
            xTimerPendFunctionCallFromISR(BleIf_StackCallback, (void*)pMsg, 0, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        else
        {
            BleIf_IntCb.stackCallback((BleIf_MsgHdr_t*)pMsg);
        }
    }
}

Status_t BleIf_Init(BleIf_Callbacks_t* callbacks)
{
    if(NULL == callbacks)
    {
        return BLEIF_STATUS_INVALID_ARGUMENT;
    }
    if(BleIf_State != BLE_STATE_NOT_INITIALIZED)
    {
        return BLEIF_STATUS_WRONG_STATE;
    }

    /* Store callbacks from app */
    memcpy(&BleIf_IntCb, callbacks, sizeof(BleIf_Callbacks_t));

    /* Initialize BLE Core-Controller Layer */
    gpBleComps_StackInit();

    /* Initialize BLE Core-Host Layer */
    cordioBleHost_Init();

    /* Register BLE Core-Host Layer stack callbacks */
    /* Register to BLE Core-Host device manager (device ready, advertising state) */
    DmRegister(BleIf_DmCallback);
    /* Register to BLE Core-Host device manager (connection state) */
    DmConnRegister(DM_CLIENT_ID_APP, BleIf_DmCallback);
    /* Register to BLE Core-Host ATT layer */
    AttRegister(BleIf_AttCallback);
    /* Register to BLE Core-Host ATT layer (connection state)*/
    AttConnRegister(AppServerConnCallback);

    /* GATT/GAP service */
    SvcCoreAddGroup();
    for(uint8_t i = 0; i < BLE_CONFIG_SVC_GROUPS; i++)
    {
        svcGroups[i].readCback = BleIf_ReadCallback;
        svcGroups[i].writeCback = BleIf_WriteCallback;

        AttsAddGroup(&svcGroups[i]);
    }

    AttsCccRegister(NUM_CCC_IDX, (attsCccSet_t*)BleIf_CccSet, BleIf_ATTS_CccCallback);

    /* handle events */
    BleIf_MsgHandlerId = WsfOsSetNextHandler(BleIf_MsgHandler);

    /* Start the BLE Core-Host Layer - should be called after initialization and configuration */
    DmDevReset();

    return BLEIF_STATUS_NO_ERROR;
}

static uint32_t BleIf_SetAdvInterval_Internal(void* arg)
{
    GP_ASSERT_DEV_EXT(arg != NULL);

    uint16_t intervalMin = ((runOnParam_t*)arg)->u16param1;
    uint16_t intervalMax = ((runOnParam_t*)arg)->u16param2;

    GP_ASSERT_DEV_EXT(sizeof(((runOnParam_t*)arg)->u16param1) == sizeof(intervalMin));
    GP_ASSERT_DEV_EXT(sizeof(((runOnParam_t*)arg)->u16param2) == sizeof(intervalMax));

    if(intervalMin > intervalMax)
    {
        ((runOnParam_t*)arg)->outParam = STATUS_INVALID_ARGUMENT;
        return RUNONSTATUS_OK;
    }

    uint8_t advHandle = DM_ADV_HANDLE_DEFAULT;

    DmAdvSetInterval(advHandle, intervalMin, intervalMax);

    ((runOnParam_t*)arg)->outParam = STATUS_NO_ERROR;

    return RUNONSTATUS_OK;
}

Status_t BleIf_SetAdvInterval(uint16_t intervalMin, uint16_t intervalMax)
{
    runOnParam_t param = {
        .u16param1 = intervalMin,
        .u16param2 = intervalMax,
        .outParam = STATUS_WRONG_STATE,
    };

    uint32_t ret = gpSched_RunOn(BleIf_SetAdvInterval_Internal, (void*)&param);
    BleIf_SetAdvInterval_Internal((void*)&param);

    GP_ASSERT_DEV_EXT(ret == RUNONSTATUS_OK);

    return param.outParam;
}

static uint32_t BleIf_SetAdvData_Internal(void* arg)
{
    GP_ASSERT_DEV_EXT(arg != NULL);

    BleIf_AdvLocation_t location = (BleIf_AdvLocation_t)((runOnParam_t*)arg)->u8param1;
    uint8_t len = ((runOnParam_t*)arg)->u8param2;
    uint8_t* pData = (uint8_t*)((runOnParam_t*)arg)->ptrParam;

    GP_ASSERT_DEV_EXT(sizeof(((runOnParam_t*)arg)->u8param1) == sizeof(location));
    GP_ASSERT_DEV_EXT(sizeof(((runOnParam_t*)arg)->u8param2) == sizeof(len));
    GP_ASSERT_DEV_EXT(sizeof(((runOnParam_t*)arg)->ptrParam) == sizeof(pData));

    if(NULL == pData)
    {
        ((runOnParam_t*)arg)->outParam = STATUS_INVALID_ARGUMENT;
        return RUNONSTATUS_OK;
    }

    uint8_t advHandle = DM_ADV_HANDLE_DEFAULT;

    /* Set the advertising data */
    DmAdvSetData(advHandle, HCI_ADV_DATA_OP_COMP_FRAG, location, len, pData);

    ((runOnParam_t*)arg)->outParam = STATUS_NO_ERROR;

    return RUNONSTATUS_OK;
}

Status_t BleIf_SetAdvData(BleIf_AdvLocation_t location, uint8_t len, uint8_t* pData)
{
    runOnParam_t param = {
        .u8param1 = (uint8_t)location,
        .u8param2 = len,
        .ptrParam = (uint32_t)pData,
        .outParam = STATUS_WRONG_STATE,
    };

    uint32_t ret = gpSched_RunOn(&BleIf_SetAdvData_Internal, (void*)&param);

    GP_ASSERT_DEV_EXT(ret == RUNONSTATUS_OK);

    return param.outParam;
}

static uint32_t BleIf_StartAdvertising_Internal(void* arg)
{
    GP_ASSERT_DEV_EXT(arg != NULL);

    uint8_t advHandle = DM_ADV_HANDLE_DEFAULT;
    uint8_t maxEaEvents = 0;
    uint16_t advDuration = BLE_ADV_BROADCAST_DURATION;

    uint8_t randAddr[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
    /* All zero peer address, i.e. no peer */
    const uint8_t peerAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    /* Set the local Advertising Address type to Random */
    DmAdvSetAddrType(DM_ADDR_RANDOM);
    /* Force the random static format on the randomly generated address */
    DM_RAND_ADDR_SET((uint8_t*)randAddr, DM_RAND_ADDR_STATIC);
    /* Set the Advertiser address in the host stack */
    DmDevSetRandAddr((uint8_t*)randAddr);

    /* Configure the advertising parameters */
    DmAdvConfig(advHandle, DM_ADV_CONN_UNDIRECT, DM_ADDR_PUBLIC, (uint8_t*)peerAddr);

    DmAdvSetChannelMap(advHandle, (BLE_ADV_CHANNEL_37 | BLE_ADV_CHANNEL_38 | BLE_ADV_CHANNEL_39));

    /* Scan advertising */
    DmAdvStart(DM_NUM_ADV_SETS, &advHandle, &advDuration, &maxEaEvents);

    ((runOnParam_t*)arg)->outParam = STATUS_NO_ERROR;

    return RUNONSTATUS_OK;
}

Status_t BleIf_StartAdvertising(void)
{
    uint8_t advDataBuf[BLEIF_ADV_DATASET_MAX_SIZE] = {0};
    uint8_t advDataSize;

    if(BleIf_State != BLE_STATE_IDLE)
    {
        return STATUS_WRONG_STATE;
    }

    advDataSize = Ble_Peripheral_Config_Load_Advertise_Frame(advDataBuf);
    /* BLE set advertising data */
    BleIf_SetAdvData(BLEIF_ADV_DATA_LOC_ADV, advDataSize, advDataBuf);

    uint8_t scanRespBuf[BLEIF_ADV_DATASET_MAX_SIZE] = {0};
    uint8_t scanRespSize;
    scanRespSize = Ble_Peripheral_Config_Load_Scan_Response_Frame(scanRespBuf);
    /* BLE set scan response */
    BleIf_SetAdvData(BLEIF_ADV_DATA_LOC_SCAN, scanRespSize, scanRespBuf);

    BleIf_SetAdvInterval(BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX);

    runOnParam_t param = {
        .outParam = STATUS_WRONG_STATE,
    };

    BleIf_StopAdvertising();

    uint32_t ret = gpSched_RunOn(BleIf_StartAdvertising_Internal, (void*)&param);
    uint32_t timeout = BLEIF_DM_ADV_START_TIMEOUT;

    while((BleIf_State != BLE_STATE_ADV_STARTED) && timeout--)
    {
        // DM callback set BleIf_State to BLE_STATE_ADV_STARTED
        vTaskDelay(10);
    }

    if(timeout == 0)
    {
        return STATUS_WRONG_STATE;
    }

    GP_ASSERT_DEV_EXT(ret == RUNONSTATUS_OK);

    return param.outParam;
}

Status_t BleIf_StopAdvertising(void)
{
    if(BleIf_State != BLE_STATE_ADV_STARTED)
    {
        return STATUS_WRONG_STATE;
    }

    uint8_t advHandle = DM_ADV_HANDLE_DEFAULT;

    DmAdvStop(DM_NUM_ADV_SETS, &advHandle);

    while(BleIf_State == BLE_STATE_ADV_STARTED)
    {
        vTaskDelay(10);
    }

    return STATUS_NO_ERROR;
}
