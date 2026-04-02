/*
 * Copyright (c) 2024-2025, Qorvo Inc
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
/** @file "BlePeripheral.h"
 *
 * BLE stack integration file.
 * Handles BLE connection and incoming BLE commands
 */

#ifndef _BLEIF_H_
#define _BLEIF_H_

#pragma once

/*****************************************************************************
 *                    Includes Definitions
 *****************************************************************************/

#include "global.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "att_api.h"
#include "svc_core.h"

/*****************************************************************************
 *                    Macro Definitions
 *****************************************************************************/

/*! Maximum size of advertising frame */
#define BLEIF_ADV_DATASET_MAX_SIZE 31

/*! Invalid connection indicator */
#define BLEIF_DM_CONN_NONE 0

/*! Maximum number of connections */
#define BLEIF_DM_CONN_MAX 1

/*! Number of supported advertising sets: must be set to 1 for legacy advertising */
#define BLEIF_DM_NUM_ADV_SETS 1

#define BLEIF_HCI_SUCCESS               0x00 /*!< Success */
#define BLEIF_HCI_ERR_REMOTE_TERMINATED 0x13 /*!< Remote user terminated connection */
#define BLEIF_HCI_ERR_LOCAL_TERMINATED  0x16 /*!< Connection terminated by local host */

/*! \brief      BD address length */
#define BDA_ADDR_LEN 6

/*****************************************************************************
 *                    Enum Definitions
 *****************************************************************************/

typedef struct {
    enum {
        kBleConnectionEvent_Connected = 0x0,
        kBleConnectionEvent_Advertise_Start = 0x1,
        kBleConnectionEvent_Disconnected = 0x2,

        kBleLedControlCharUpdate = 0x10,
    } Event;
    uint8_t Value;
} Ble_Event_t;

typedef enum {
    BLEIF_STATUS_NO_ERROR = 0,
    BLEIF_STATUS_BUFFER_TOO_SMALL = 1,
    BLEIF_STATUS_INVALID_ARGUMENT = 2,
    BLEIF_STATUS_KEY_LEN_TOO_SMALL = 3,
    BLEIF_STATUS_INVALID_DATA = 4,
    BLEIF_STATUS_NOT_IMPLEMENTED = 5,
    BLEIF_STATUS_NVM_ERROR = 6,
    BLEIF_STATUS_WRONG_STATE = 7
} BleIf_Status_t;

/** @brief status return values */
typedef enum {
    STATUS_NO_ERROR = 0,
    STATUS_BUFFER_TOO_SMALL = 1,
    STATUS_INVALID_ARGUMENT = 2,
    STATUS_KEY_LEN_TOO_SMALL = 3,
    STATUS_INVALID_DATA = 4,
    STATUS_NOT_IMPLEMENTED = 5,
    STATUS_NVM_ERROR = 6,
    STATUS_WRONG_STATE = 7
} Status_t;

typedef enum {
    /*******************************************************************************
     * Bluetooth Low Energy Advertisement Data Type Definitions
     ******************************************************************************/

    /* Flags */
    BLEIF_ADV_DATA_TYPE_FLAGS = 0x01, /* Device capabilities and discoverability */
    /* Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID16_INCOMPLETE = 0x02,  /* Incomplete List of 16-bit Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID16_COMPLETE = 0x03,    /* Complete List of 16-bit Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID32_INCOMPLETE = 0x04,  /* Incomplete List of 32-bit Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID32_COMPLETE = 0x05,    /* Complete List of 32-bit Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID128_INCOMPLETE = 0x06, /* Incomplete List of 128-bit Service UUIDs */
    BLEIF_ADV_DATA_TYPE_UUID128_COMPLETE = 0x07,   /* Complete List of 128-bit Service UUIDs */
    /* Local Name */
    BLEIF_ADV_DATA_TYPE_NAME_SHORTENED = 0x08, /* Shortened Local Name */
    BLEIF_ADV_DATA_TYPE_NAME_COMPLETE = 0x09,  /* Complete Local Name */
    /* TX Power Level */
    BLEIF_ADV_DATA_TYPE_TX_POWER_LEVEL = 0x0A, /* Transmit Power Level */
    /* Class of Device */
    BLEIF_ADV_DATA_TYPE_CLASS_OF_DEVICE = 0x0D, /* Bluetooth Class of Device */
    /* Service Data */
    BLEIF_ADV_DATA_TYPE_SERVICE_DATA_16 = 0x16, /* Service Data with 16-bit UUID */
    /* Target Addresses */
    BLEIF_ADV_DATA_TYPE_PUBLIC_TARGET_ADDR = 0x17, /* Public Target Address */
    BLEIF_ADV_DATA_TYPE_RANDOM_TARGET_ADDR = 0x18, /* Random Target Address */
    /* Appearance and Advertising Interval */
    BLEIF_ADV_DATA_TYPE_APPEARANCE = 0x19,           /* Device Appearance Category */
    BLEIF_ADV_DATA_TYPE_ADVERTISING_INTERVAL = 0x1A, /* Advertising Interval */
    /* LE Bluetooth Device Address */
    BLEIF_ADV_DATA_TYPE_LE_BT_DEVICE_ADDRESS = 0x1B, /* LE Bluetooth Device Address */
    /* LE Role */
    BLEIF_ADV_DATA_TYPE_LE_ROLE = 0x1C, /* Device Role (Peripheral, Central, etc.) */
    /* Service Data with 32-bit and 128-bit UUIDs */
    BLEIF_ADV_DATA_TYPE_SERVICE_DATA_32 = 0x20,  /* Service Data with 32-bit UUID */
    BLEIF_ADV_DATA_TYPE_SERVICE_DATA_128 = 0x21, /* Service Data with 128-bit UUID */
    /* LE Secure Connections */
    BLEIF_ADV_DATA_TYPE_LE_SC_CONFIRM_VALUE = 0x22, /* LE Secure Connections Confirmation Value */
    BLEIF_ADV_DATA_TYPE_LE_SC_RANDOM_VALUE = 0x23,  /* LE Secure Connections Random Value */
    /* Manufacturer Specific */
    BLEIF_ADV_DATA_TYPE_MANUFACTURER_DATA = 0xFF, /* Manufacturer Specific Data */
} BleIf_AdvDataType_t;

/** \name DM Callback Events
 * Events handled by the DM state machine.
 */
#define BLEIF_DM_CBACK_START 0x20 /*!< \brief DM callback event starting value */

/*! \brief DM callback events */
enum {
    BLEIF_DM_RESET_CMPL_IND = BLEIF_DM_CBACK_START, /*!< \brief Reset complete */
    BLEIF_DM_ADV_START_IND,                         /*!< \brief Advertising started */
    BLEIF_DM_ADV_STOP_IND,                          /*!< \brief Advertising stopped */
    BLEIF_DM_ADV_NEW_ADDR_IND,                      /*!< \brief New resolvable address has been generated */
    BLEIF_DM_SCAN_START_IND,                        /*!< \brief Scanning started */
    BLEIF_DM_SCAN_STOP_IND,                         /*!< \brief Scanning stopped */
    BLEIF_DM_SCAN_REPORT_IND,                       /*!< \brief Scan data received from peer device */
    BLEIF_DM_CONN_OPEN_IND,                         /*!< \brief Connection opened */
    BLEIF_DM_CONN_CLOSE_IND,                        /*!< \brief Connection closed */
    BLEIF_DM_CONN_UPDATE_IND,                       /*!< \brief Connection update complete */
    BLEIF_DM_SEC_PAIR_CMPL_IND,                     /*!< \brief Pairing completed successfully */
    BLEIF_DM_SEC_PAIR_FAIL_IND,                     /*!< \brief Pairing failed or other security failure */
    BLEIF_DM_SEC_ENCRYPT_IND,                       /*!< \brief Connection encrypted */
    BLEIF_DM_SEC_ENCRYPT_FAIL_IND,                  /*!< \brief Encryption failed */
    BLEIF_DM_SEC_AUTH_REQ_IND,                      /*!< \brief PIN or OOB data requested for pairing */
    BLEIF_DM_SEC_KEY_IND,                           /*!< \brief Security key indication */
    BLEIF_DM_SEC_LTK_REQ_IND,                       /*!< \brief LTK requested for encyption */
    BLEIF_DM_SEC_PAIR_IND,                          /*!< \brief Incoming pairing request from master */
    BLEIF_DM_SEC_SLAVE_REQ_IND,                     /*!< \brief Incoming security request from slave */
    BLEIF_DM_SEC_CALC_OOB_IND,                      /*!< \brief Result of OOB Confirm Calculation Generation */
    BLEIF_DM_SEC_ECC_KEY_IND,                       /*!< \brief Result of ECC Key Generation */
    BLEIF_DM_SEC_COMPARE_IND,        /*!< \brief Result of Just Works/Numeric Comparison Compare Value Calculation */
    BLEIF_DM_SEC_KEYPRESS_IND,       /*!< \brief Keypress indication from peer in passkey security */
    BLEIF_DM_PRIV_RESOLVED_ADDR_IND, /*!< \brief Private address resolved */
    BLEIF_DM_PRIV_GENERATE_ADDR_IND, /*!< \brief Private resolvable address generated */
    BLEIF_DM_CONN_READ_RSSI_IND,     /*!< \brief Connection RSSI read */
    BLEIF_DM_PRIV_ADD_DEV_TO_RES_LIST_IND,   /*!< \brief Device added to resolving list */
    BLEIF_DM_PRIV_REM_DEV_FROM_RES_LIST_IND, /*!< \brief Device removed from resolving list */
    BLEIF_DM_PRIV_CLEAR_RES_LIST_IND,        /*!< \brief Resolving list cleared */
    BLEIF_DM_PRIV_READ_PEER_RES_ADDR_IND,    /*!< \brief Peer resolving address read */
    BLEIF_DM_PRIV_READ_LOCAL_RES_ADDR_IND,   /*!< \brief Local resolving address read */
    BLEIF_DM_PRIV_SET_ADDR_RES_ENABLE_IND,   /*!< \brief Address resolving enable set */
    BLEIF_DM_REM_CONN_PARAM_REQ_IND,         /*!< \brief Remote connection parameter requested */
    BLEIF_DM_CONN_DATA_LEN_CHANGE_IND,       /*!< \brief Data length changed */
    BLEIF_DM_CONN_WRITE_AUTH_TO_IND,         /*!< \brief Write authenticated payload complete */
    BLEIF_DM_CONN_AUTH_TO_EXPIRED_IND,       /*!< \brief Authenticated payload timeout expired */
    BLEIF_DM_PHY_READ_IND,                   /*!< \brief Read PHY */
    BLEIF_DM_PHY_SET_DEF_IND,                /*!< \brief Set default PHY */
    BLEIF_DM_PHY_UPDATE_IND,                 /*!< \brief PHY update */
    BLEIF_DM_ADV_SET_START_IND,              /*!< \brief Advertising set(s) started */
    BLEIF_DM_ADV_SET_STOP_IND,               /*!< \brief Advertising set(s) stopped */
    BLEIF_DM_SCAN_REQ_RCVD_IND,              /*!< \brief Scan request received */
    BLEIF_DM_EXT_SCAN_START_IND,             /*!< \brief Extended scanning started */
    BLEIF_DM_EXT_SCAN_STOP_IND,              /*!< \brief Extended scanning stopped */
    BLEIF_DM_EXT_SCAN_REPORT_IND,            /*!< \brief Extended scan data received from peer device */
    BLEIF_DM_PER_ADV_SET_START_IND,          /*!< \brief Periodic advertising set started */
    BLEIF_DM_PER_ADV_SET_STOP_IND,           /*!< \brief Periodic advertising set stopped */
    BLEIF_DM_PER_ADV_SYNC_EST_IND,           /*!< \brief Periodic advertising sync established */
    BLEIF_DM_PER_ADV_SYNC_EST_FAIL_IND,      /*!< \brief Periodic advertising sync establishment failed */
    BLEIF_DM_PER_ADV_SYNC_LOST_IND,          /*!< \brief Periodic advertising sync lost */
    BLEIF_DM_PER_ADV_SYNC_TRSF_EST_IND,      /*!< \brief Periodic advertising sync transfer established */
    BLEIF_DM_PER_ADV_SYNC_TRSF_EST_FAIL_IND, /*!< \brief Periodic advertising sync transfer establishment failed */
    BLEIF_DM_PER_ADV_SYNC_TRSF_IND,          /*!< \brief Periodic advertising sync info transferred */
    BLEIF_DM_PER_ADV_SET_INFO_TRSF_IND,      /*!< \brief Periodic advertising set sync info transferred */
    BLEIF_DM_PER_ADV_REPORT_IND,             /*!< \brief Periodic advertising data received from peer device */
    BLEIF_DM_REMOTE_FEATURES_IND,            /*!< \brief Remote features from peer device */
    BLEIF_DM_READ_REMOTE_VER_INFO_IND,       /*!< \brief Remote LL version information read */
    BLEIF_DM_CONN_IQ_REPORT_IND,             /*!< \brief IQ samples from CTE of received packet from peer device */
    BLEIF_DM_CTE_REQ_FAIL_IND,               /*!< \brief CTE request failed */
    BLEIF_DM_CONN_CTE_RX_SAMPLE_START_IND,   /*!< \brief Sampling received CTE started */
    BLEIF_DM_CONN_CTE_RX_SAMPLE_STOP_IND,    /*!< \brief Sampling received CTE stopped */
    BLEIF_DM_CONN_CTE_TX_CFG_IND,            /*!< \brief Connection CTE transmit parameters configured */
    BLEIF_DM_CONN_CTE_REQ_START_IND,         /*!< \brief Initiating connection CTE request started */
    BLEIF_DM_CONN_CTE_REQ_STOP_IND,          /*!< \brief Initiating connection CTE request stopped */
    BLEIF_DM_CONN_CTE_RSP_START_IND,         /*!< \brief Responding to connection CTE request started */
    BLEIF_DM_CONN_CTE_RSP_STOP_IND,          /*!< \brief Responding to connection CTE request stopped */
    BLEIF_DM_READ_ANTENNA_INFO_IND,          /*!< \brief Antenna information read */
    BLEIF_DM_CIS_CIG_CONFIG_IND,             /*!< \brief CIS CIG configure complete */
    BLEIF_DM_CIS_CIG_REMOVE_IND,             /*!< \brief CIS CIG remove complete */
    BLEIF_DM_CIS_REQ_IND,                    /*!< \brief CIS request */
    BLEIF_DM_CIS_OPEN_IND,                   /*!< \brief CIS connection opened */
    BLEIF_DM_CIS_CLOSE_IND,                  /*!< \brief CIS connection closed */
    BLEIF_DM_REQ_PEER_SCA_IND,               /*!< \brief Request peer SCA complete */
    BLEIF_DM_ISO_DATA_PATH_SETUP_IND,        /*!< \brief ISO data path setup complete */
    BLEIF_DM_ISO_DATA_PATH_REMOVE_IND,       /*!< \brief ISO data path remove complete */
    BLEIF_DM_DATA_PATH_CONFIG_IND,           /*!< \brief Data path configure complete */
    BLEIF_DM_READ_LOCAL_SUP_CODECS_IND,      /*!< \brief Local supported codecs read */
    BLEIF_DM_READ_LOCAL_SUP_CODEC_CAP_IND,   /*!< \brief Local supported codec capabilities read */
    BLEIF_DM_READ_LOCAL_SUP_CTR_DLY_IND,     /*!< \brief Local supported controller delay read */
    BLEIF_DM_BIG_START_IND,                  /*!< \brief BIG started */
    BLEIF_DM_BIG_STOP_IND,                   /*!< \brief BIG stopped */
    BLEIF_DM_BIG_SYNC_EST_IND,               /*!< \brief BIG sync established */
    BLEIF_DM_BIG_SYNC_EST_FAIL_IND,          /*!< \brief BIG sync establishment failed */
    BLEIF_DM_BIG_SYNC_LOST_IND,              /*!< \brief BIG sync lost */
    BLEIF_DM_BIG_SYNC_STOP_IND,              /*!< \brief BIG sync stopped */
    BLEIF_DM_BIG_INFO_ADV_REPORT_IND,        /*!< \brief BIG Info advertising data received from peer device */
    BLEIF_DM_L2C_CMD_REJ_IND,                /*!< \brief L2CAP Command Reject */
    BLEIF_DM_ERROR_IND,                      /*!< \brief General error */
    BLEIF_DM_HW_ERROR_IND,                   /*!< \brief Hardware error */
    BLEIF_DM_VENDOR_SPEC_IND                 /*!< \brief Vendor specific event */
};

#define BLEIF_DM_CBACK_END BLEIF_DM_VENDOR_SPEC_IND /*!< \brief DM callback event ending value */

/** \name ATT Callback Events
 * Events related to ATT transactions.
 */
#define BLEIF_ATT_CBACK_START 0x02 /*!< \brief ATT callback event starting value */

/*! \brief ATT client callback events */
typedef enum BleIf_AttMsg_ {
    BLEIF_ATTC_FIND_INFO_RSP = BLEIF_ATT_CBACK_START, /*!< \brief Find information response */
    BLEIF_ATTC_FIND_BY_TYPE_VALUE_RSP,                /*!< \brief Find by type value response */
    BLEIF_ATTC_READ_BY_TYPE_RSP,                      /*!< \brief Read by type value response */
    BLEIF_ATTC_READ_RSP,                              /*!< \brief Read response */
    BLEIF_ATTC_READ_LONG_RSP,                         /*!< \brief Read long response */
    BLEIF_ATTC_READ_MULTIPLE_RSP,                     /*!< \brief Read multiple response */
    BLEIF_ATTC_READ_BY_GROUP_TYPE_RSP,                /*!< \brief Read group type response */
    BLEIF_ATTC_WRITE_RSP,                             /*!< \brief Write response */
    BLEIF_ATTC_WRITE_CMD_RSP,                         /*!< \brief Write command response */
    BLEIF_ATTC_PREPARE_WRITE_RSP,                     /*!< \brief Prepare write response */
    BLEIF_ATTC_EXECUTE_WRITE_RSP,                     /*!< \brief Execute write response */
    BLEIF_ATTC_HANDLE_VALUE_NTF,                      /*!< \brief Handle value notification */
    BLEIF_ATTC_HANDLE_VALUE_IND,                      /*!< \brief Handle value indication */
    BLEIF_ATTC_READ_MULT_VAR_RSP = 16,                /*!< \brief Read multiple variable length response */
    BLEIF_ATTC_MULT_VALUE_NTF,                        /*!< \brief Read multiple value notification */
    /* ATT server callback events */
    BLEIF_ATTS_HANDLE_VALUE_CNF,      /*!< \brief Handle value confirmation */
    BLEIF_ATTS_MULT_VALUE_CNF,        /*!< \brief Handle multiple value confirmation */
    BLEIF_ATTS_CCC_STATE_IND,         /*!< \brief Client chracteristic configuration state change */
    BLEIF_ATTS_DB_HASH_CALC_CMPL_IND, /*!< \brief Database hash calculation complete */
    /* ATT common callback events */
    BLEIF_ATT_MTU_UPDATE_IND,        /*!< \brief Negotiated MTU value */
    BLEIF_ATT_EATT_CONN_CMPL_IND,    /*!< \brief EATT Connect channels complete */
    BLEIF_ATT_EATT_RECONFIG_CMPL_IND /*!< \brief EATT Reconfigure complete */
} BleIf_AttMsg_t;

/*! \brief ATT callback events */
#define BLEIF_ATT_CBACK_END BLEIF_ATT_EATT_RECONFIG_CMPL_IND /*!< \brief ATT callback event ending value */

/*****************************************************************************
 *                    Type Definitions
 *****************************************************************************/

typedef enum BleIf_AdvLocation_ { BLEIF_ADV_DATA_LOC_ADV = 0, BLEIF_ADV_DATA_LOC_SCAN = 1 } BleIf_AdvLocation_t;

/*! \brief      BD address data type */
typedef uint8_t BleIf_bdAddr_t[BDA_ADDR_LEN];

/*! Attribute structure */
typedef struct {
    uint8_t const* pUuid; /*! Pointer to the attribute's UUID */
    uint8_t* pValue;      /*! Pointer to the attribute's value */
    uint16_t* pLen;       /*! Pointer to the length of the attribute's value */
    uint16_t maxLen;      /*! Maximum length of attribute's value */
    uint8_t settings;     /*! Attribute settings */
    uint8_t permissions;  /*! Attribute permissions */
} BleIf_Attr_t;

/*! Common message structure passed to event handler */
typedef struct {
    uint16_t param; /*! General purpose parameter passed to event handler */
    uint8_t event;  /*! General purpose event value passed to event handler */
    uint8_t status; /*! General purpose status value passed to event handler */
} BleIf_MsgHdr_t;

/*! \brief LE connection complete event */
typedef struct {
    BleIf_MsgHdr_t hdr;      /*!< \brief Event header */
    uint8_t status;          /*!< \brief Status. */
    uint16_t handle;         /*!< \brief Connection handle. */
    uint8_t role;            /*!< \brief Local connection role. */
    uint8_t addrType;        /*!< \brief Peer address type. */
    BleIf_bdAddr_t peerAddr; /*!< \brief Peer address. */
    uint16_t connInterval;   /*!< \brief Connection interval */
    uint16_t connLatency;    /*!< \brief Connection latency. */
    uint16_t supTimeout;     /*!< \brief Supervision timeout. */
    uint8_t clockAccuracy;   /*!< \brief Clock accuracy. */

    /* \brief enhanced fields */
    BleIf_bdAddr_t localRpa; /*!< \brief Local RPA. */
    BleIf_bdAddr_t peerRpa;  /*!< \brief Peer RPA. */
} BleIf_HciLeConnCmplEvt_t;

/*! \brief Disconnect complete event */
typedef struct {
    BleIf_MsgHdr_t hdr; /*!< \brief Event header. */
    uint8_t status;     /*!< \brief Disconnect complete status. */
    uint16_t handle;    /*!< \brief Connect handle. */
    uint8_t reason;     /*!< \brief Reason. */
} BleIf_HciDisconnectCmplEvt_t;

/*! \brief LE connection update complete event */
typedef struct {
    BleIf_MsgHdr_t hdr;    /*!< \brief Event header. */
    uint8_t status;        /*!< \brief Status. */
    uint16_t handle;       /*!< \brief Connection handle. */
    uint16_t connInterval; /*!< \brief Connection interval. */
    uint16_t connLatency;  /*!< \brief Connection latency. */
    uint16_t supTimeout;   /*!< \brief Supervision timeout. */
} BleIf_HciLeConnUpdateCmplEvt_t;

/* \brief Data structure for BLEIF_DM_ADV_SET_START_IND */
typedef struct {
    BleIf_MsgHdr_t hdr;                       /*! Header */
    uint8_t numSets;                          /*! Number of advertising sets */
    uint8_t advHandle[BLEIF_DM_NUM_ADV_SETS]; /*! Advertising handle array */
} BleIf_DmAdvSetStartEvt_t;

/*! \brief LE advertising set terminated */
typedef struct {
    BleIf_MsgHdr_t hdr;   /*!< \brief Event header. */
    uint8_t status;       /*!< \brief Status. */
    uint8_t advHandle;    /*!< \brief Advertising handle. */
    uint16_t handle;      /*!< \brief Connection handle. */
    uint8_t numComplEvts; /*!< \brief Number of completed extended advertising events. */
} BleIf_HciLeAdvSetTermEvt_t;

/*! \brief Union of DM callback event data types */
typedef union {
    BleIf_MsgHdr_t hdr;                        /*! Common header */
    BleIf_HciLeConnCmplEvt_t connOpen;         /*! BLEIF_DM_CONN_OPEN_IND */
    BleIf_HciDisconnectCmplEvt_t connClose;    /*! BLEIF_DM_CONN_CLOSE_IND */
    BleIf_HciLeConnUpdateCmplEvt_t connUpdate; /*! BLEIF_DM_CONN_UPDATE_IND */
    BleIf_DmAdvSetStartEvt_t advSetStart;      /*! BLEIF_DM_ADV_SET_START_IND */
    BleIf_HciLeAdvSetTermEvt_t advSetStop;     /*! BLEIF_DM_ADV_SET_STOP_IND */
} BleIf_DmEvt_t;

/*!
 * \brief ATT callback event
 *
 * \param hdr.event     Callback event
 * \param hdr.param     DM connection ID
 * \param hdr.status    Event status:  ATT_SUCCESS or error status
 * \param pValue        Pointer to value data, valid if valueLen > 0
 * \param valueLen      Length of value data
 * \param handle        Attribute handle
 * \param continuing    TRUE if more response packets expected
 * \param mtu           Negotiated MTU value
 */
typedef struct {
    BleIf_MsgHdr_t hdr; /*!< \brief Header structure */
    uint8_t* pValue;    /*!< \brief Value */
    uint16_t valueLen;  /*!< \brief Value length */
    uint16_t handle;    /*!< \brief Attribute handle */
    uint8_t continuing; /*!< \brief TRUE if more response packets expected */
    uint16_t mtu;       /*!< \brief Negotiated MTU value */
} BleIf_AttEvt_t;

/*! \brief ATTS client characteristic configuration callback structure */
typedef struct {
    BleIf_MsgHdr_t hdr; /*! Header structure */
    uint16_t handle;    /*! CCCD handle */
    uint16_t value;     /*! CCCD value */
    uint8_t idx;        /*! CCCD settings index */
} BleIf_AttsCccEvt_t;

/*! \brief Message processing callback */
typedef void (*BleIf_StackCback_t)(BleIf_MsgHdr_t* pDmEvt);

/*! \brief Attribute group read callback */
typedef void (*BleIf_ReadCback_t)(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                                  BleIf_Attr_t* pAttr);

/*! \brief Attribute group write callback */
typedef void (*BleIf_WriteCback_t)(uint16_t connId, uint16_t handle, uint8_t operation, uint16_t offset, uint16_t len,
                                   uint8_t* pValue, BleIf_Attr_t* pAttr);

/*! \brief CCC subscription change callback */
typedef void (*BleIf_ATTS_CccCback_t)(BleIf_AttsCccEvt_t* pEvt);

/*! \brief External callbacks structure */
typedef struct {
    BleIf_StackCback_t stackCallback;
    BleIf_ReadCback_t chrReadCallback;
    BleIf_WriteCback_t chrWriteCallback;
    BleIf_ATTS_CccCback_t cccCallback;
} BleIf_Callbacks_t;

/*****************************************************************************
 *                    Public Function Prototypes
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 *                    BLE API
 *****************************************************************************/

/** @brief Initialization function for BLE functionality
 *
 *  @param callbacks       Structure containing various callbacks to call in the application
 *  @return                Returns NO_ERROR if the operation completed successfully.
 */
Status_t BleIf_Init(BleIf_Callbacks_t* callbacks);

/** @brief Send notification over BLE
 *
 * @param handle          Attribute handle to send notification for
 * @param length          Length of the data to be sent
 * @param data            Pointer to the data to be sent
 * @return                Returns NO_ERROR if the operation completed successfully
 */

Status_t BleIf_SendNotification(uint16_t handle, uint16_t length, uint8_t* data);

/** @brief  Send indication over BLE
 *
 *  @param handle         Attribute handle to send indication for
 *  @param length         Length of the data to be sent
 *  @param data           Pointer to the data to be sent
 *  @return               Returns NO_ERROR if the operation completed successfully
 */

Status_t BleIf_SendIndication(uint16_t handle, uint16_t length, uint8_t* data);

/** @brief Sets minimum and maximum intervals for advertising packets
 *
 *  @param intervalMin     Minimum interval between advertisement packets.
 *  @param intervalMax     Maximum interval between advertisement packets.
 *  @return                Returns NO_ERROR if the operation completed successfully
 */
Status_t BleIf_SetAdvInterval(uint16_t intervalMin, uint16_t intervalMax);

/** @brief Set advertising data to be used
 *
 *  @param location        Type of advertising data that is being set (advertising or scan response).
 *  @param len             Length of the data.
 *  @param pData           Pointer to the advertising data.
 *  @return                Returns NO_ERROR if the operation completed successfully
 */
Status_t BleIf_SetAdvData(BleIf_AdvLocation_t location, uint8_t len, uint8_t* pData);

/** @brief Start BLE advertising using the default parameters
 *
 *  @return                Returns NO_ERROR if the operation completed successfully
 */
Status_t BleIf_StartAdvertising(void);

/** @brief Stops BLE advertising (execution will be blocked untill advertising is stopped)
 *
 *  @return                Returns NO_ERROR if the operation completed successfully
 */
Status_t BleIf_StopAdvertising(void);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _BLEIF_H_
