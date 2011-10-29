/* Copyright 2009 Broadcom Corporation
**
** This program is the proprietary software of Broadcom Corporation and/or its
** licensors, and may only be used, duplicated, modified or distributed 
** pursuant to the terms and conditions of a separate, written license 
** agreement executed between you and Broadcom (an "Authorized License").  
** Except as set forth in an Authorized License, Broadcom grants no license 
** (express or implied), right to use, or waiver of any kind with respect to 
** the Software, and Broadcom expressly reserves all rights in and to the 
** Software and all intellectual property rights therein.  
** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS 
** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
** ALL USE OF THE SOFTWARE.  
**
** Except as expressly set forth in the Authorized License,
** 
** 1.     This program, including its structure, sequence and organization, 
**        constitutes the valuable trade secrets of Broadcom, and you shall 
**        use all reasonable efforts to protect the confidentiality thereof, 
**        and to use this information only in connection with your use of 
**        Broadcom integrated circuit products.
** 
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
**        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
**        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
**        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
**        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
**        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
**        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
**        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
**        OF USE OR PERFORMANCE OF THE SOFTWARE.
**
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
**        ITS LICENSORS BE LIABLE FOR 
**        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
**              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
**              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
**              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
**        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
**              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
**              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
**              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/
 


#ifndef BTL_IF_H
#define BTL_IF_H

#include "dtun.h"

#ifndef BTL_IF_CLIENT_TEST_ENABLE
#include "btld_prop.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define CTRL_CHAN_MAX 1
#define DTUN_CHAN_MAX 1
#define AG_CHAN_MAX 31
#define AV_CHAN_MAX 1
#define FM_CHAN_MAX 1
#define SCO_CHAN_MAX 1
#define PBS_CHAN_MAX 1
#define FTPS_CHAN_MAX 1
#define SAPS_CHAN_MAX 1
#define DUN_CHAN_MAX 3
#define HH_CHAN_MAX 1
#define SPP_CHAN_MAX 5
#define TEST_CHAN_MAX 1
#define BTS_CHAN_MAX (32*2)

#define BTS_CLIENT_BIND_CHAN_OFFSET 32

#ifdef DTUN_LOCAL_SERVER_ADDR
#define BTLIF_IP_ADDR DTUN_LOCAL_SERVER_ADDR
#else
#define BTLIF_IP_ADDR "127.0.0.1"
#define BTLIF_IP_ADDR_ALT "10.0.2.2" // emulator host
#endif

#define BTLIF_AV_SUBPORT_DEFAULT 0

#define BTLIF_PORT_BASE (9000)
#define BTLIF_PORT_BASE_CTRL      (BTLIF_PORT_BASE)

#define BTLIF_PORT_BASE_DTUN      (BTLIF_PORT_BASE_CTRL+CTRL_CHAN_MAX)

#define BTLIF_PORT_BASE_SPP       (BTLIF_PORT_BASE_DTUN+DTUN_CHAN_MAX)

#define BTLIF_PORT_BASE_AG        (BTLIF_PORT_BASE_SPP+SPP_CHAN_MAX)

#define BTLIF_PORT_BASE_AV        (BTLIF_PORT_BASE_AG+AG_CHAN_MAX)

#define BTLIF_PORT_BASE_FM        (BTLIF_PORT_BASE_AV+AV_CHAN_MAX)

#define BTLIF_PORT_BASE_SCO       (BTLIF_PORT_BASE_FM+FM_CHAN_MAX)

#define BTLIF_PORT_BASE_PBS       (BTLIF_PORT_BASE_SCO+SCO_CHAN_MAX)
 	
#define BTLIF_PORT_BASE_FTPS      (BTLIF_PORT_BASE_PBS+PBS_CHAN_MAX)

#define BTLIF_PORT_BASE_DUN       (BTLIF_PORT_BASE_FTPS+FTPS_CHAN_MAX)

#define BTLIF_PORT_BASE_HH        (BTLIF_PORT_BASE_DUN+DUN_CHAN_MAX)
 	
#define BTLIF_PORT_BASE_TEST      (BTLIF_PORT_BASE_HH + HH_CHAN_MAX)

#define BTLIF_PORT_BASE_SAPS      (BTLIF_PORT_BASE_TEST + TEST_CHAN_MAX)

#define BTLIF_PORT_BASE_BTS       (BTLIF_PORT_BASE_SAPS + SAPS_CHAN_MAX)

#define BTLIF_PORT_BASE_END       (BTLIF_PORT_BASE_BTS + BTS_CHAN_MAX)

#define BTLIF_PORT_INVALID  0xfefe

typedef struct sockaddr_in SA_TYPE;

#define CTRL_SOCKET_INVALID (-1)
#define DATA_SOCKET_INVALID (-1)

#define BACKLOG_DEFAULT 5

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#endif

#ifndef BD_ADDR_LEN
#define BD_ADDR_LEN     6
typedef char BD_ADDR[BD_ADDR_LEN];
#endif

/* Device Name Length Definition : 
 * This value should be accorded with btld's configuration. (refer btld.txt)
 * It means,
 * we must care both configurations when the value needs to be changed. 
 *
 * BTA_DM_REMOTE_DEVICE_NAME_LENGTH should be less than BD_NAME_LEN(248)
 * 2010.04.29 - BTA_DM_REMOTE_DEVICE_NAME_LENGTH was configured as 126 */
#ifndef BTA_DM_REMOTE_DEVICE_NAME_LENGTH
#define BTA_DM_REMOTE_DEVICE_NAME_LENGTH    126
#endif

#ifndef DATA_TYPES_H
typedef unsigned char BOOLEAN;
typedef unsigned char UINT8;
typedef char INT8;
typedef unsigned char BYTE;
typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short UINT16;
typedef signed short INT16;
#endif

typedef unsigned short tSUB;
typedef unsigned short tSTATUS; 
typedef unsigned short tRESULT; 
typedef unsigned short tSUBPORT;
typedef unsigned short tRFCHAN;
typedef unsigned short tCMD_TYPE;
typedef unsigned short tHDR_LEN;

typedef int tCTRL_HANDLE; 
typedef int tDATA_HANDLE;

/* available subsystems */
typedef enum {
   SUB_NONE,
   SUB_CTRL,
   SUB_DTUN,
   SUB_AG,
   SUB_AV,
   SUB_FM,
   SUB_SCO,
   SUB_PR,
   SUB_PBS,
   SUB_FTPS,
   SUB_DUN,
   SUB_HH,
   SUB_SPP,
   SUB_TEST,
   SUB_SAPS,
   SUB_BTS,
   BTL_IF_SUBSYSTEM_MAX,
} tBTL_IF_SUBSYSTEM;

/* btl-if error codes */
typedef enum {
    BTL_IF_SUCCESS,
    BTL_IF_GENERIC_ERR,
    BTL_IF_CTRL_CH_FAILED,
    BTL_IF_CONNECTION_FAILED,
    BTL_IF_NOT_CONNECTED,
    BTL_IF_NOT_READY,
    BTL_IF_ALREADY_INITIALIZED,
    BTL_IF_SUBSYSTEM_NOT_REGISTERED,
    BTL_IF_SUBSYSTEM_INVALID,
    BTL_IF_SUBSYSTEM_BUSY,
    BTL_IF_PROTOCOL_ERROR
} tBTL_IF_Result;



/**********************************************************************/
/*  COMMAND DEFINITIONS
 *
 *
 */


/* control msg types */
typedef enum {
    BTLIF_CTRL_CMD_BASE = 0x1000,

    /* generic subsystem ctrl messages */
    BTLIF_REGISTER_SUBSYS_REQ,
    BTLIF_REGISTER_SUBSYS_RSP,  
    BTLIF_UNREGISTER_SUBSYS_REQ,
    BTLIF_UNREGISTER_SUBSYS_RSP,      
    BTLIF_CONNECT_REQ,
    BTLIF_CONNECT_RSP,
    BTLIF_CONNECT_IND,
    BTLIF_DISCONNECT_REQ,
    BTLIF_DISCONNECT_RSP,    
    BTLIF_DISCONNECT_IND,
    BTLIF_LISTEN_REQ,
    BTLIF_SET_CONFIG,
    BTLIF_GET_CONFIG,
    BTLIF_DATA_CHAN_IND,
    BTLIF_DATA_CHAN_DISC_IND,
    BTLIF_SUBSYSTEM_ATTACHED, 
    BTLIF_SUBSYSTEM_DETACHED,
    BTLIF_CONNECT_IND_ACK,
    BTLIF_DISC_RX_BUF_PENDING,

    /* custom subsystem ctrl messages */
    
    BTLIF_CTRL_CMD_END =0x1100
} tBTLIF_CTRL_MSG_IDS;

/* control message ids for DTUN subsystem. */
typedef enum {
    BTLIF_DTUN_CMD_BASE = BTLIF_CTRL_CMD_END+1,
    BTLIF_DTUN_METHOD_CALL,
    BTLIF_DTUN_SIGNAL_EVT,
    BTLIF_DTUN_CMD_END
} tBTLIF_DTUN_CTRL_MSG_IDS;

/* control message ids for AG subsystem. */
typedef enum {
    BTLIF_AG_CMD_BASE = BTLIF_DTUN_CMD_END+1,
    BTLIF_AG_CMD_END,
} tBTLIF_AG_CTRL_MSG_IDS;

/* control message ids for AV subsystem. */
typedef enum {
    BTLIF_AV_CMD_BASE = BTLIF_AG_CMD_END+1,
    BTLIF_AV_CMD_END,
} tBTLIF_AV_CTRL_MSG_IDS;

/* control message ids for FM subsystem. */
typedef enum {
    BTLIF_FM_CMD_BASE = BTLIF_AV_CMD_END+1,
    BTLIF_FM_ENABLE,
    BTLIF_FM_DISABLE,
    BTLIF_FM_TUNE,
    BTLIF_FM_MUTE,
    BTLIF_FM_SEARCH,
    BTLIF_FM_SEARCH_ABORT,
    BTLIF_FM_SET_RDS_MODE,
    BTLIF_FM_SET_RDS_RBDS,
    BTLIF_FM_RDS_UPDATE,
    BTLIF_FM_AUDIO_MODE,
    BTLIF_FM_AUDIO_PATH,
    BTLIF_FM_SCAN_STEP,
    BTLIF_FM_SET_REGION,
    BTLIF_FM_CONFIGURE_DEEMPHASIS,
    BTLIF_FM_ESTIMATE_NFL,
    BTLIF_FM_GET_AUDIO_QUALITY,
    BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION,
    BTLIF_FM_AF_JMP_EVT,
    BTLIF_FM_SET_VOLUME,
    BTLIF_FM_SET_VOLUME_EVT,
    BTLIF_FM_CMD_END
} tBTLIF_FM_CTRL_MSG_IDS;

/* control message ids for SCO subsystem. */
typedef enum {
    BTLIF_SCO_CMD_BASE = BTLIF_FM_CMD_END+1,
    BTLIF_SCO_CMD_END,
} tBTLIF_SCO_CTRL_MSG_IDS;

/* Control message ids for Bluetooth Printer subsystem. */
typedef enum {
    BTLIF_PR_CMD_BASE = BTLIF_SCO_CMD_END+1,
    BTLIF_PR_ENABLE,
    BTLIF_PR_DISABLE,
    BTLIF_PR_GET_CAPS,
    BTLIF_PR_AUTH_RSP,
    BTLIF_PR_PRINT,
    BTLIF_PR_PARTIAL_IMAGE_RSP,
    BTLIF_PR_REF_OBJ_RSP,
    BTLIF_PR_ABORT,
    BTLIF_PR_CANCEL_BP_STATUS,
    BTLIF_PR_CARD_TO_XHTML,
    BTLIF_PR_CAL_TO_XHTML,
    BTLIF_PR_NOTE_TO_XHTML,
    BTLIF_PR_CMD_END
} tBTLIF_PR_CTRL_MSG_IDS;

/* Control message ids for Phone Book Server subsystem */
typedef enum {
    BTLIF_PBS_CMD_BASE = BTLIF_PR_CMD_END+1,
    BTLIF_PBS_ENABLE,
    BTLIF_PBS_DISABLE,
    BTLIF_PBS_AUTH_RSP,
    BTLIF_PBS_ACCESS_RSP,
    BTLIF_PBS_CMD_END
} tBTLIF_PBS_CTRL_MSG_IDS;

/* Control message ids for FTPS Server subsystem */
typedef enum {
    BTLIF_FTPS_CMD_BASE = BTLIF_PBS_CMD_END+1,
    BTLIF_FTPS_ENABLE,
    BTLIF_FTPS_DISABLE,
    BTLIF_FTPS_CLOSE,
    BTLIF_FTPS_AUTH_RSP,
    BTLIF_FTPS_ACCESS_RSP,
    BTLIF_FTPS_CMD_END
} tBTLIF_FTPS_CTRL_MSG_IDS;

/* Control message ids for DG subsystem */
typedef enum {
    BTLIF_DG_CMD_BASE = BTLIF_FTPS_CMD_END+1,
    BTLIF_DG_ENABLE,
    BTLIF_DG_DISABLE,
    BTLIF_DG_LISTEN,
    BTLIF_DG_OPEN,
    BTLIF_DG_CLOSE,
    BTLIF_DG_SHUTDOWN,
    BTLIF_DG_SPP_CREATE,
    BTLIF_DG_SPP_DESTORY,
    BTLIF_DG_CONNECT_DATA_PATH,

    BTLIF_DG_RIL_FLOW_CTRL_REQ,
    BTLIF_DG_CMD_END
} tBTLIF_DG_CTRL_MSG_IDS;

/* Control message ids for HH subsystem */
typedef enum {
    BTLIF_HH_CMD_BASE = BTLIF_DG_CMD_END+1,
    BTLIF_HH_OPEN,
    BTLIF_HH_CLOSE,
    BTLIF_HH_CMD_END
} tBTLIF_HH_CTRL_MSG_IDS;

/* Control messages for TEST subsystem */
typedef enum {
    BTLIF_TST_CMD_BASE = BTLIF_HH_CMD_END+1,
    BTLIF_TST_SET_TESTMODE,
    BTLIF_TST_GET_TST_STATE,
    BTLIF_TST_TX_RX_TEST,
    BTLIF_TST_SEND_TST_CMD,
    BTLIF_TST_CMD_END
} tBTLIF_TEST_CTRL_MSG_IDS;

/* Control messages for SAP subsystem */
typedef enum {
    BTLIF_SAPS_CMD_BASE = BTLIF_TST_CMD_END+1,
	BTLIF_SAPS_ENABLE,
	BTLIF_SAPS_DISABLE,
	BTLIF_SAPS_DISCONNECT,
	BTLIF_SAPS_STATUS,
	BTLIF_SAPS_CARD_STATUS,
	BTLIF_SAPS_READER_STATUS,

    /* Control messages for SAP/RIL subsystem */
    BTLIF_SAPS_RIL_SIM_ON,
    BTLIF_SAPS_RIL_SIM_OFF,
    BTLIF_SAPS_RIL_SIM_RESET,
    BTLIF_SAPS_RIL_SIM_ATR,
    BTLIF_SAPS_RIL_SIM_APDU,

    BTLIF_SAPS_CMD_END
} tBTLIF_SAPS_CTRL_MSG_IDS;

/* Control message ids for BT Socket subsystem */
typedef enum {
    BTLIF_BTS_CMD_BASE = BTLIF_SAPS_CMD_END+1,

    /* RFCOMM */
    BTLIF_BTS_RFC_LISTEN_REQ,
    BTLIF_BTS_RFC_LISTEN_RSP,
    BTLIF_BTS_RFC_LISTEN_CANCEL,
    BTLIF_BTS_RFC_CON_IND,
    BTLIF_BTS_RFC_CON_IND_ACK,    
    BTLIF_BTS_RFC_CON_REQ,
    BTLIF_BTS_RFC_CON_RSP,
    BTLIF_BTS_RFC_CLOSE,
    BTLIF_BTS_RFC_CLOSE_CFM,
    BTLIF_BTS_RFC_DISC_IND,
    BTLIF_BTS_RFC_DISC_IND_ACK,
    BTLIF_BTS_RFC_LST_NOTIFY_DSOCK,

    /* L2CAP */
    BTLIF_BTS_L2C_LISTEN_REQ,
    BTLIF_BTS_L2C_LISTEN_RSP,
    BTLIF_BTS_L2C_CONNECT_REQ,
    BTLIF_BTS_L2C_CONNECT_RSP,
    BTLIF_BTS_L2C_DISC_REQ,
    BTLIF_BTS_L2C_DISC_RSP,
    BTLIF_BTS_L2C_DISC_IND,

    /* SCO */
    BTLIF_BTS_SCO_CON_REQ,
    BTLIF_BTS_SCO_CON_RSP,
    BTLIF_BTS_SCO_CON_IND,
    BTLIF_BTS_SCO_DISC_REQ,
    BTLIF_BTS_SCO_DISC_RSP,
    BTLIF_BTS_SCO_DISC_IND,

    BTLIF_BTS_EVT_ABORT,
    BTLIF_BTS_CMD_END
} tBTLIF_BTS_CTRL_MSG_IDS;

enum {
    BTLIF_COMMANDS_END = BTLIF_BTS_CMD_END
};


/**********************************************************************/
/*  EVENT DEFINITIONS
 *
 *
 */

/* Event message ids for Bluetooth Printer subsystem. */
typedef enum {
    BTLIF_PR_EVT_BASE = BTLIF_COMMANDS_END+1,
    BTLIF_PR_ENABLE_EVT,
    BTLIF_PR_OPEN_EVT,
    BTLIF_PR_PROGRESS_EVT,
    BTLIF_PR_CLOSE_EVT,
    BTLIF_PR_GETCAPS_EVT,
    BTLIF_PR_AUTH_EVT,    
    BTLIF_PR_THUMBNAIL_EVT,
    BTLIF_PR_PARTIAL_IMAGE_EVT,
    BTLIF_PR_GET_OBJ_EVT,
    BTLIF_PR_BP_DOC_CMPL_EVT,
    BTLIF_PR_JOB_STATUS_EVT,
    BTLIF_PR_EVT_END
} tBTLIF_PR_EVT_MSG_IDS;

/* Event message ids for Phone Book Server subsystem */
typedef enum {
    BTLIF_PBS_EVT_BASE = BTLIF_PR_EVT_END+1,
    BTLIF_PBS_ENABLE_EVT,
    BTLIF_PBS_OPEN_EVT,
    BTLIF_PBS_CLOSE_EVT,
    BTLIF_PBS_AUTH_EVT,
    BTLIF_PBS_ACCESS_EVT,
    BTLIF_PBS_OPER_CMPL_EVT,
    BTLIF_PBS_EVT_END
} tBTLIF_PBS_EVT_MSG_IDS;

/* Event message ids for FTPS Server subsystem */
typedef enum {
    BTLIF_FTPS_EVT_BASE = BTLIF_PBS_EVT_END+1,
    BTLIF_FTPS_ENABLE_EVT,
    BTLIF_FTPS_DISABLE_EVT,
    BTLIF_FTPS_OPEN_EVT,
    BTLIF_FTPS_CLOSE_EVT,
    BTLIF_FTPS_AUTH_EVT,
    BTLIF_FTPS_ACCESS_EVT,
    BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT,
    BTLIF_FTPS_OPER_PUT_CMPL_EVT,
    BTLIF_FTPS_OPER_GET_CMPL_EVT,
    BTLIF_FTPS_OPER_DEL_CMPL_EVT,
    BTLIF_FTPS_EVT_END
} tBTLIF_FTPS_EVT_MSG_IDS;

/* Event message ids for DG Server subsystem */
typedef enum {
    BTLIF_DG_EVT_BASE = BTLIF_FTPS_EVT_END+1,
    BTLIF_DG_ENABLE_EVT,
    BTLIF_DG_DISABLE_EVT,
    BTLIF_DG_LISTEN_EVT,
    BTLIF_DG_OPENING_EVT,
    BTLIF_DG_OPEN_EVT,
    BTLIF_DG_CLOSE_EVT,
    BTLIF_DG_SHUTDOWN_EVT,

    BTLIF_DG_RIL_FLOW_CTRL_EVT,
    BTLIF_DG_EVT_END
} tBTLIF_DG_EVT_MSG_IDS;

/* Event message ids for HH subsystem */
typedef enum {
    BTLIF_HH_EVT_BASE = BTLIF_DG_EVT_END+1,
    BTLIF_HH_OPEN_EVT,
    BTLIF_HH_CLOSE_EVT,
    BTLIF_HH_EVT_END
} tBTLIF_HH_EVT_MSG_IDS;

/* Event message ids for TEST Server subsystem */
typedef enum {
    BTLIF_TST_EVT_BASE = BTLIF_HH_EVT_END+1,
    BTLIF_TST_SET_TESTMODE_RSP,
    BTLIF_TST_GET_TST_STATE_RSP,
    BTLIF_TST_TX_RX_TEST_RSP,
    BTLIF_TST_SEND_TST_CMD_RSP,
    BTLIF_TST_EVT_END
} tBTLIF_TEST_EVT_MSG_IDS;

/* Event message Ids for SAP subsystem */
typedef enum {
    BTLIF_SAPS_EVT_BASE = BTLIF_TST_EVT_END+1,
	BTLIF_SAPS_ENABLE_EVT,
	BTLIF_SAPS_DISABLE_EVT,
	BTLIF_SAPS_OPEN_EVT,
	BTLIF_SAPS_CLOSE_EVT,

    BTLIF_SAPS_RIL_SIM_OPEN_EVT,
    BTLIF_SAPS_RIL_SIM_CLOSE_EVT,
    BTLIF_SAPS_RIL_SIM_ON_EVT,
    BTLIF_SAPS_RIL_SIM_OFF_EVT,
    BTLIF_SAPS_RIL_SIM_RESET_EVT,
    BTLIF_SAPS_RIL_SIM_ATR_EVT,
    BTLIF_SAPS_RIL_SIM_APDU_EVT,

    BTLIF_SAPS_EVT_END
} tBTLIF_SAPS_EVT_MSG_IDS;

/* Event message ids for BTS subsystem */
typedef enum {
    BTLIF_BTS_EVT_BASE = BTLIF_SAPS_EVT_END+1,        
    BTLIF_BTS_EVT_END
} tBTLIF_BTS_EVT_MSG_IDS;

typedef unsigned short tBTLIF_CTRL_MSG_ID;


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                                        //
// NOTE : PLEASE MAKE SURE THAT ALL BTL IF PARAMETER STRUCTS ARE PUT INSIDE THE PACKED PRAGMA AREA    //
//                                                                                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////


/****************************************************** 
  * Packet headers
  */

#pragma pack(1)

/* main packet header */
typedef struct
{
    tBTLIF_CTRL_MSG_ID id;
    tHDR_LEN           len;
} tBTL_IF_HDR;


/****************************************************** 
  * Local params
  */


/****************************************************** 
  * Generic params
  */

typedef struct
{
    tRESULT            result;
} tBTLIF_RESULT_PARAM;

typedef struct
{
    tRESULT            result; // result
    tSTATUS            status; // additional status code
} tBTLIF_CONNECT_RSP_PARAM;

typedef struct
{
    tSUBPORT            subport;
} tBTLIF_LISTEN_REQ_PARAM;

typedef struct
{
    tSUBPORT     subport;
    tDATA_HANDLE handle;
} tBTLIF_DATA_CHAN_IND;

typedef struct
{
    tCTRL_HANDLE handle;
} tBTLIF_SUBSYSTEM_DETACHED;

typedef struct
{
    tCTRL_HANDLE handle;
} tBTLIF_SUBSYSTEM_ATTACHED;

typedef struct
{
    tDATA_HANDLE handle;
    char *rx_buf;
    int rx_buf_size;
} tBTLIF_DISC_RX_BUF_PENDING;


/****************************************************** 
  * AG  params
  */

/* BTLIF_CONNECT_REQ */
typedef struct
{
    BD_ADDR            bd;
    tRFCHAN             rf_chan;
} tBTLIF_AG_CONNECT_REQ_PARAM;

/* BTLIF_LISTEN_REQ */
typedef struct
{
    BD_ADDR            bd;
    tRFCHAN             rf_chan;
} tBTLIF_AG_LISTEN_REQ_PARAM;


/* BTLIF_DISCONNECT_REQ & BTLIF_DISCONNECT_IND */
typedef struct
{
    tRFCHAN             rf_chan;
} tBTLIF_AG_DISCONNECT_PARAM;

/****************************************************** 
  * AV params
  */


/****************************************************** 
  * FM params
  */

typedef struct
{
    int i1;
} tBTLIF_FM_REQ_I_PARAM;

typedef struct
{
    int i1;
    int i2;
} tBTLIF_FM_REQ_II_PARAM;

typedef struct
{
    int i1;
    int i2;
    int i3;
} tBTLIF_FM_REQ_III_PARAM;

typedef struct
{
    int i1;
    int i2;
    int i3;
    int i4;
} tBTLIF_FM_REQ_IIII_PARAM;

typedef struct
{
    BOOLEAN z1;
} tBTLIF_FM_REQ_Z_PARAM;

typedef struct
{
    BOOLEAN z1;
    BOOLEAN z2;
} tBTLIF_FM_REQ_ZZ_PARAM;

typedef struct
{
    int i1;
    BOOLEAN z1;
} tBTLIF_FM_REQ_IZ_PARAM;

typedef struct
{
    int i1;
    BOOLEAN z1;
    BOOLEAN z2;
} tBTLIF_FM_REQ_IZZ_PARAM;

typedef struct
{
    int i1;
    int i2;
    int i3;
    char s1[65];    
} tBTLIF_FM_REQ_IIIS_PARAM;

/****************************************************** 
  * Bluetooth Printer (PR) params
  */
#define BTLIF_PR_MAX_BULK_LENGTH 2048

#define MAX_SERVICE_NAME_LENGTH     32
#define MAX_AUTH_REALM_LENGTH       16
#define MAX_ROOT_PATH_LENGTH        16
#define MAX_PASSWORD_LENGTH         16
#define MAX_USER_NAME_LENGTH        8
#define BT_ADDRESS_STRING_LENGTH    20
#define MAX_FILE_NAME_LENGTH        64

/****************************************************** 
  * PBAP params
  */

typedef struct {
  int security_mask;
  char service_name[MAX_SERVICE_NAME_LENGTH];  // 32 is coming from btui_int.h, BTUI_NAME_LENGTH
  BOOLEAN enable_authen;
  unsigned char realm_len;
  unsigned char realm[MAX_AUTH_REALM_LENGTH];  // 16 coming from BTUI_MAX_AUTH_REALM_LENGTH
  char root_path[MAX_ROOT_PATH_LENGTH];   // Maximum path length supported by MMI  If we add an length paramerter, we can reduce the bytes need to be sent to btld
} tBTLIF_PBS_ENABLE_PARAM;

typedef struct {
   char authen_password[MAX_PASSWORD_LENGTH];   // BTUI_MAX_AUTH_KEY_LENGTH
   char authen_user_name[MAX_USER_NAME_LENGTH]; // We always response with "Guest
} tBTLIF_PBS_AUTH_RSP_PARAM ;

typedef struct {
   UINT8 oper_code;
   BOOLEAN      access;
   char remote_bd_addr[BT_ADDRESS_STRING_LENGTH];
   char remote_bd_name[BTA_DM_REMOTE_DEVICE_NAME_LENGTH +1];
   char filename[MAX_FILE_NAME_LENGTH];     // 550 is from bta_fs_cfg.c BAT_FS_PATH_LEN 256 BTA_FS_FILE_LEN 294 + 1 for '\0'
   
} tBTLIF_PBS_ACCESS_RSP_PARAM;

typedef struct {
    INT32      obj_size;   /* Total size of object */
    INT32      bytes;      /* Number of bytes read or written since last progress event */
} tBTLIF_PR_PROGRESS_EVT_PARAM;

/****************************************************** 
  *FTPS params
 */

typedef struct {
  UINT32 security_mask;
  INT8 service_name[32];  // 32 is coming from btui_int.h, BTUI_NAME_LENGTH
  BOOLEAN enable_authen;
  UINT8 realm_len;
  UINT8 realm[16];  // 16 coming from BTUI_MAX_AUTH_REALM_LENGTH
  INT8 root_path[260];   // Maximum path length supported by MMI  If we add an length paramerter, we can reduce the bytes need to be sent to btld
  UINT8 app_id;
} tBTLIF_FTPS_ENABLE_PARAM;


typedef struct {
    BD_ADDR  bd_addr;
} tBTLIF_FTPS_OPEN_PARAM;

typedef struct {
   INT8 authen_password[16];   // BTUI_MAX_AUTH_KEY_LENGTH
   INT8 authen_user_name[8]; // We always response with "Guest
} tBTLIF_FTPS_AUTH_RSP_PARAM ;

typedef struct {  
   UINT8 oper_code;
   BOOLEAN      access;
   UINT32  size_of_file; // milo : change this variable UINT8 to UINT32
    BD_ADDR bd_addr;
   INT8    bd_name[BTA_DM_REMOTE_DEVICE_NAME_LENGTH+1];
   INT8 filename[550];     // 550 is from bta_fs_cfg.c BAT_FS_PATH_LEN 256 BTA_FS_FILE_LEN 294 
} tBTLIF_FTPS_ACCESS_RSP_PARAM;

typedef struct {
    UINT32 file_size;
    UINT32 bytes_transferred;
} tBTLIF_FTPS_PROGRESS_PARAM;

typedef struct {
    INT8 file_name[550];  // 550 is from bta_fs_cfg.c BAT_FS_PATH_LEN 256 BTA_FS_FILE_LEN 294
    UINT8 oper_code; //put, get or del
} tBTLIF_FTPS_OPER_CMPL_PARAM;

/****************************************************** 
  * DG params
  */
  
/* DG RIL Flow Ctrl values */
#define DG_RIL_FLOW_CTRL_DTRDSR_OFF   0x00    /* DTR/DSR siganl off */
#define DG_RIL_FLOW_CTRL_DTRDSR_ON    0x01    /* DTR/DSR signal on */
#define DG_RIL_FLOW_CTRL_RTSCTS_ON    0x02    /* RTS/CTS signal on */
#define DG_RIL_FLOW_CTRL_RTSCTS_OFF   0x00    /* RTS/CTS signal off */
#define DG_RIL_FLOW_CTRL_RI_ON        0x04    /* Ring indicator signal on */
#define DG_RIL_FLOW_CTRL_RI_OFF       0x00    /* Ring indicator signal off */
#define DG_RIL_FLOW_CTRL_CD_ON        0x08    /* Carrier detect signal on */
#define DG_RIL_FLOW_CTRL_CD_OFF       0x00    /* Carrier detect signal off */

#define DG_RIL_FLOW_CTRL_SIGNAL_DTRDSR  0x01  /* DTR/DSR signal. */
#define DG_RIL_FLOW_CTRL_SIGNAL_RTSCTS  0x02  /* RTS/CTS signal. */
#define DG_RIL_FLOW_CTRL_SIGNAL_RI      0x04  /* Ring indicator signal. */
#define DG_RIL_FLOW_CTRL_SIGNAL_CD      0x08  /* Carrier detect signal. */


typedef struct {
    /* We don't really need service id since dun and spp are in separate sub system */
    UINT8 sec_mask;   /* tBTA_SEC is UINT8 */
    UINT8 app_id;
    INT8 service_name[32];
}tBTLIF_DG_LISTEN_PARAM;

typedef struct {
    BD_ADDR bd_addr;
    /* We don't really need service id since dun and spp are in separate sub system */
    UINT8 sec_mask;   /* tBTA_SEC is UINT8 */
    UINT8 app_id;
    INT8 service_name[32];
}tBTLIF_DG_OPEN_PARAM;

typedef struct {
    UINT8 app_id;  /* When reach btld, btld will look for port_handle based on app_id */
}tBTLIF_DG_CLOSE_PARAM;

typedef struct {
    UINT8 app_id; /* When reach btld, btld will look for port_handle based on app_id */
}tBTLIF_DG_SHUTDOWN_PARAM;

typedef struct {
    UINT8 app_id; /* When reach btld, btld will look for port_handle based on app_id */
}tBTLIF_DG_OPENING_PARAM;

typedef struct {
    UINT8 app_id; 
}tBTLIF_DG_LISTENING_PARAM;

typedef struct {
    int signals;
    int mask;
}tBTLIF_DG_RIL_FLOW_CTRL_PARAM;


/**
 *  HH Params
 */
typedef struct {
    BD_ADDR   bd_addr;
    int       status;
} tBTLIF_HH_PARAM;


/**
 *  SAPS Params
 */

/* Disconnection types */
#define BTLIF_SAPS_DISCONNECT_TYPE_GRACEFUL   0x00
#define BTLIF_SAPS_DISCONNECT_TYPE_IMMEDIATE  0x01

/* SAP STATUS */
#define BTLIF_SAPS_STATUS_UNKNOWN      0x00
#define BTLIF_SAPS_STATUS_NO_SIM       0x01
#define BTLIF_SAPS_STATUS_NOT_READY    0x02
#define BTLIF_SAPS_STATUS_READY        0x03
#define BTLIF_SAPS_STATUS_CONNECTED    0x04

/* SIM card status */
#define BTLIF_SAPS_CARD_STATUS_UNKNOWN         0x00
#define BTLIF_SAPS_CARD_STATUS_RESET           0x01
#define BTLIF_SAPS_CARD_STATUS_NOT_ACCESSIBLE  0x02
#define BTLIF_SAPS_CARD_STATUS_REMOVED         0x03
#define BTLIF_SAPS_CARD_STATUS_INSERTED        0x04
#define BTLIF_SAPS_CARD_STATUS_RECOVERED       0x05

/*RESULT CODE */
#define BTLIF_SAPS_RESULT_OK                     0x00
#define BTLIF_SAPS_RESULT_NO_REASON              0x01
#define BTLIF_SAPS_RESULT_CARD_NOT_ACCESSIBLE    0x02
#define BTLIF_SAPS_RESULT_CARD_ALREADY_POWER_OFF 0x03
#define BTLIF_SAPS_RESULT_REMOVED                0x04
#define BTLIF_SAPS_RESULT_ALREADY_POWER_ON       0x05
#define BTLIF_SAPS_RESULT_DATA_NOT_AVAILABLE     0x06
#define BTLIF_SAPS_RESULT_NOT_SUPPORT            0x07

/* BTLIF_SAPS_OPEN_EVT */
typedef struct
{
    BD_ADDR  bd_addr;
} tBTLIF_SAPS_OPEN_EVT_PARAM;

/* BTLIF_SAPS_RIL_SIM_APDU_EVT */
typedef struct
{
    UINT16  req_len;
    UINT16  rsp_maxlen;
    UINT8   apdu_req[4];
} tBTLIF_SAPS_RIL_SIM_APDU_EVT_PARAM;

/* BTLIF_SAPS_RIL_SIM_APDU */
typedef struct
{
    tRESULT   result;
    UINT16    apdulen;
    UINT8     apdu[4];
} tBTLIF_SAPS_RIL_SIM_APDU_PARAM;

/* BTLIF_SAPS_RIL_SIM_ATR */
typedef struct
{
    tRESULT   result;
    UINT16    atrlen;
    UINT8     atr[4];
} tBTLIF_SAPS_RIL_SIM_ATR_PARAM;


/*
 * TESTMODE and trace support
 */
#define TST_MAX_CMD_LEN     (1152-6)        /* big enough for a  3-DH5 acl frame - header (6 bytes) */
#define TST_CMD_MASK        0xefffffff      /* MSB must be 0 for all commands */
#define TST_CMD_RSP_MASK    0x80000000      /* MSB in respones must be set to 1 */
#define BT_ADDR_LEN         6
#define TST_TX_POWER_DBM    0x08
/* testmode states. DO NOT CHANGE ASSIGNED VALUES. this will break the internal mapping! */
typedef enum {
    TST_DISABLE_TESTMODE = 0,
    TST_ENABLE_TESTMODE = 0x00000001,
    TST_SET_DUT = 0x00000011,
    TST_SET_TX_CARRIER_FREQ = 0x00000021,
    TST_SET_TX_TEST = 0x00000031,
    TST_SET_RX_TEST = 0x00000041,
    TST_SET_TRACE_LEVEL = 0xffff0000,   /* MSB: layer_id, LSB: type/level (0-5, ff)  */
    TST_GET_TESTMODE_STATE = 0xffffffff     /* keep this last */
} tBTLIF_TST_TESTMODE_STATE;

/* enable/disable test mode data type */
typedef struct {
    uint32_t            mode;
} tBTLIF_TST_SET_TESTMODE_PARAM;

typedef struct {
    uint32_t            mode;
} tBTLIF_TST_SET_TESTMODE_RSP_PARAM;

/* tx/rx carrier frequency test */
typedef struct tBTLIF_TST_RX_TX_TEST_PARAM_tag {
    uint32_t    testmode;
    uint8_t     carrier_enable;
    uint8_t     remote_bd[BT_ADDR_LEN];
    uint16_t    report_period;
    uint8_t     frequency;
    uint8_t     mode;
    uint8_t     hopping_mode;
    uint8_t     modulation_type;
    uint8_t     logical_channel;
    uint8_t     tx_power_level;
    union {
        int8_t  dbm;
        uint8_t tbl_idx;
    } tx_power_option;
    uint8_t     bb_packet_type;
    uint16_t    bb_packet_len;
} tBTLIF_TST_RX_TX_TEST_PARAM;

typedef enum {
    BTLIF_TST_STS_FAIL = -1,    /* failure due to incorrect command or no resources */
    BTLIF_TST_STS_OK = 0,   /* generic ok. this also returned by the HCI commands in case off success */
} BTLIF_TST_STATUS;


typedef struct tBTLIF_TST_RX_TX_TEST_RSP_PARAM_tag {
    uint32_t    testmode;
    int         status;     /* success 0, failure: negative; resource or not supported failures,
                             * positive >0: HCI command error status code */
} tBTLIF_TST_RX_TX_TEST_RSP_PARAM;

typedef struct tBTLIF_TST_CMD_PARAM_tag {
    uint32_t    cmd;
    uint16_t    len;
    uint8_t     payload[TST_MAX_CMD_LEN];
} tBTLIF_TST_CMD_PARAM;

typedef struct tBTLIF_TST_CMD_RSP_PARAM_tag {
    uint32_t    cmd;
    uint16_t    len;
    uint8_t     payload[TST_MAX_CMD_LEN];
} tBTLIF_TST_CMD_RSP_PARAM;

#define BTLIF_TST_CMD_HDR_LEN       (sizeof(uint32_t)+sizeof(uint16_t))
#define BTLIF_TST_CMD_STATUS_LEN    1       /* one byte for BTLIF_STACK_HCI_CMD status response */
#define BTLIF_TST_CMD_ST_SUCCESS    0
#define BTLIF_TST_CMD_ST_FAILURE    1
#define BTLIF_CMD_CHN_MASK    (0xff<<24)    /* cmd channel selector */
#define BTLIF_STACK_HCI_CMD   (0x01<<24)      /* command (hci typically) accessing btld btm/bta api */
#define BTLIF_RAW_HCI_CMD     (0x02<<24)      /* raw HCI command following the HCI spec */
#define BTLIF_CMD_RES_MASK    (0xff<<16)      /* command result behaviour/return channel */
#define BTLIF_NO_CMD_RES      (0x00<<16)      /* this command require no command result feedback */
#define BTLIF_CMD_RES_TST     (0x01<<16)      /* result via SUB_TEST interface */
#define BTLIF_CMD_RES_DM      (0x02<<16)      /* return result via DTUN DM interface */
#define BTLIF_CMD_OPCODE      (0xffff)        /* opcode is stored lower 16 bits for BTL_STACK_HCI_CMD */

/****************************************************** 
  * BTS params
  */

typedef struct {
    int             rc_chan; /* unique id for this channel */
    int             auth;
    int             encrypt;
    int             master;
    int             listen_fd;
} tBTLIF_BTS_RFC_LISTEN_REQ_PARAM;

typedef struct {
    int             rc_chan;    /* jni unique id  */    
    UINT8           scn;        /* allocated scn, unique rfcomm id */
    UINT32          bta_handle; /* rfcomm handle */
    UINT32          status;   
    int             listen_fd;    
} tBTLIF_BTS_RFC_LISTEN_RSP_PARAM;

typedef struct {
    UINT32          handle;
    int             listen_fd;
} tBTLIF_BTS_RFC_LISTEN_CANCEL_PARAM;

typedef struct {
    UINT8           status;     /* Whether the operation succeeded or failed. */
    UINT32          handle;     /* The connection handle */
    BD_ADDR         rem_bda;    /* The peer address */
    int             listen_fd;
} tBTLIF_BTS_RFC_CON_IND_PARAM;

typedef struct {
    UINT32          handle;     /* The connection handle */
} tBTLIF_BTS_RFC_CON_IND_ACK_PARAM;


typedef struct {
    unsigned char   scn_remote;
    BD_ADDR         bd;
    int             auth;
    int             encrypt;
    int             master;
    int             sock;    
} tBTLIF_BTS_RFC_CON_REQ_PARAM;

typedef struct {
    UINT8           status; 
    UINT32          handle;
    BD_ADDR         rem_bda;
    unsigned char   scn_remote;
    int             sock;
} tBTLIF_BTS_RFC_CON_RSP_PARAM;

typedef struct {
    UINT32          scn_remote;
    int             sock;
} tBTLIF_BTS_RFC_CLOSE_PARAM;

typedef struct {
    UINT8           status; 
    UINT32          handle;
} tBTLIF_BTS_RFC_CLOSE_CFM_PARAM;

typedef struct {
    UINT8           status; 
    UINT32          scn_remote;    
    int             sock;    
} tBTLIF_BTS_RFC_DISC_IND_PARAM;

typedef struct {
    UINT8           status; 
    UINT32          scn_remote;
    int             sock;
} tBTLIF_BTS_RFC_DISC_IND_ACK_PARAM;

typedef struct {
    int             listen_fd;
    int             data_sock_fd;
} tBTLIF_BTS_RFC_NOTIFY_LST_DSOCK_PARAM;


/****************************************************** 
  *  Parameter union 
  */
  
typedef union
{
    /* Generic */
    tBTLIF_SUBSYSTEM_ATTACHED              attached;    
    tBTLIF_SUBSYSTEM_DETACHED              detached;
    tBTLIF_RESULT_PARAM                    result;
    tBTLIF_CONNECT_RSP_PARAM               conrsp;
    tBTLIF_LISTEN_REQ_PARAM                listenreq;
    tBTLIF_DATA_CHAN_IND                   chan_ind; 
    tBTLIF_DISC_RX_BUF_PENDING             rx_buf_pnd;
    
    /* AG */
    tBTLIF_AG_CONNECT_REQ_PARAM            ag_conreq;
    tBTLIF_AG_LISTEN_REQ_PARAM             ag_listen;
    tBTLIF_AG_DISCONNECT_PARAM             ag_disc;  

    /* AV */

    /* FM */
    tBTLIF_FM_REQ_I_PARAM                  fm_I_param;
    tBTLIF_FM_REQ_II_PARAM                 fm_II_param;
    tBTLIF_FM_REQ_III_PARAM                fm_III_param;
    tBTLIF_FM_REQ_IIII_PARAM               fm_IIII_param; 
    tBTLIF_FM_REQ_Z_PARAM                  fm_Z_param;
    tBTLIF_FM_REQ_ZZ_PARAM                 fm_ZZ_param;
    tBTLIF_FM_REQ_IZ_PARAM                 fm_IZ_param;
    tBTLIF_FM_REQ_IZZ_PARAM                fm_IZZ_param;
    tBTLIF_FM_REQ_IIIS_PARAM               fm_IIIS_param;

    /* DTUN */
    tDTUN_DEVICE_METHOD                    dtun_method;
    tDTUN_DEVICE_SIGNAL                    dtun_sig;

    /* PR */
    char                                   pr_bulk_data[BTLIF_PR_MAX_BULK_LENGTH];
    
    /* PBS */
    tBTLIF_PBS_ENABLE_PARAM                pbs_enable_param;
    tBTLIF_PBS_AUTH_RSP_PARAM              pbs_auth_rsp_param;
    tBTLIF_PBS_ACCESS_RSP_PARAM            pbs_access_req_param;

    /* FTPS */
    tBTLIF_FTPS_ENABLE_PARAM               ftps_enable_param;
    tBTLIF_FTPS_OPEN_PARAM                 ftps_open_param;
    tBTLIF_FTPS_AUTH_RSP_PARAM             ftps_auth_rsp_param;
    tBTLIF_FTPS_ACCESS_RSP_PARAM           ftps_access_req_param;
    tBTLIF_FTPS_PROGRESS_PARAM             ftps_progress_param; 
    tBTLIF_FTPS_OPER_CMPL_PARAM            ftps_oper_cmpl_param; 

    /* DG */
    tBTLIF_DG_LISTEN_PARAM                 dg_listen_param;                
    tBTLIF_DG_OPEN_PARAM                   dg_open_param;
    tBTLIF_DG_CLOSE_PARAM                  dg_close_param;
    tBTLIF_DG_SHUTDOWN_PARAM               dg_shutdown_param;
    tBTLIF_DG_OPENING_PARAM                dg_opening_param;
    tBTLIF_DG_LISTENING_PARAM              dg_listening_param;
    tBTLIF_DG_RIL_FLOW_CTRL_PARAM          dg_ril_flow_ctrl_param;

    /* HH */
    tBTLIF_HH_PARAM                        hh_param;

    /* TST */
    tBTLIF_TST_SET_TESTMODE_PARAM          tst_set_testmode_param;
    tBTLIF_TST_SET_TESTMODE_RSP_PARAM      tst_set_testmode_rsp_param;
    tBTLIF_TST_RX_TX_TEST_PARAM            tst_rx_tx_test_param;
    tBTLIF_TST_RX_TX_TEST_RSP_PARAM        tst_rx_tx_test_rsp_param;
    tBTLIF_TST_CMD_PARAM                   tst_cmd_param;
    tBTLIF_TST_CMD_RSP_PARAM               tst_cmd_rsp_param;

    /* SAPS */
    tBTLIF_SAPS_OPEN_EVT_PARAM             saps_open_evt_param;
    tBTLIF_SAPS_RIL_SIM_APDU_EVT_PARAM     saps_ril_apdu_evt_param;
    tBTLIF_SAPS_RIL_SIM_APDU_PARAM         saps_ril_apdu_param;
    tBTLIF_SAPS_RIL_SIM_ATR_PARAM          saps_ril_atr_param;

    /* BTS */
    tBTLIF_BTS_RFC_LISTEN_REQ_PARAM        bts_rfc_listen_req_param;
    tBTLIF_BTS_RFC_LISTEN_RSP_PARAM        bts_rfc_listen_rsp_param;
    tBTLIF_BTS_RFC_LISTEN_CANCEL_PARAM     bts_rfc_listen_cancel_param;
    tBTLIF_BTS_RFC_CON_IND_PARAM           bts_rfc_con_ind_param;
    tBTLIF_BTS_RFC_CON_IND_ACK_PARAM       bts_rfc_con_ind_ack_param;
    tBTLIF_BTS_RFC_CON_REQ_PARAM           bts_rfc_con_req_param;
    tBTLIF_BTS_RFC_CON_RSP_PARAM           bts_rfc_con_rsp_param;
    tBTLIF_BTS_RFC_CLOSE_PARAM             bts_rfc_close_param;    
    tBTLIF_BTS_RFC_CLOSE_CFM_PARAM         bts_rfc_close_cfm_param;
    tBTLIF_BTS_RFC_DISC_IND_PARAM          bts_rfc_disc_ind_param;
    tBTLIF_BTS_RFC_DISC_IND_ACK_PARAM      bts_rfc_disc_ind_ack_param;
    tBTLIF_BTS_RFC_NOTIFY_LST_DSOCK_PARAM  bts_rfc_notify_lst_dsock_param;

} tBTL_PARAMS;

#define BTLIF_PARAMS_MAX_SIZE sizeof(tBTL_PARAMS) 


/****************************************************** 
  *  CTRL messages
  */

/* tBTLIF_CTRL_MSG_ID */
typedef struct 
{
    tBTL_IF_HDR         hdr;
    tSUB                sub;
    tBTL_PARAMS         params;
} tBTLIF_CTRL_MSG_PKT;


#define BTLIF_CTRL_RX_BUF_MAX sizeof(tBTLIF_CTRL_MSG_PKT)

#pragma pack(0)


////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                                        //
// NOTE : PLEASE MAKE SURE THAT ALL BTL IF PARAMETER STRUCTS ARE PUT INSIDE THE PACKED PRAGMA AREA    //
//                                                                                                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////



/****************************************************** 
  * API Callbacks 
  */

typedef void (*tBTL_IF_DATA_CALLBACK)(tDATA_HANDLE handle, char *p, int len);
typedef void (*tBTL_IF_CTRL_CALLBACK)(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);

#define SUBSYS_MAX_CLIENTS 3 /* max 3 users per subsystem (if enabled) */

// extend to multiclients but default disabled (only for bts enabled)
typedef struct {
    int ctrl_fd;                        /* ctrl socket fd (to be obsoleted once mclient support verified) */
    int mctrl_fd[SUBSYS_MAX_CLIENTS];   /* multiclient fds */
    int mclnt_enabled:1;                /* allow multiclient */
    tBTL_IF_SUBSYSTEM sub;              /* datapath subsystem */
    tBTL_IF_DATA_CALLBACK data_cb;      /* data receive callback */
    tBTL_IF_CTRL_CALLBACK ctrl_cb;      /* called upon any remote configuration of datapath */
} tBTL_IF_CB;


/*******************************************************************/
// List management


typedef struct list_head {
  struct list_head *Next, *Prev;
} list_head;

#define INIT_LIST_HEAD(pHead) do { \
  (pHead)->Next = (pHead); (pHead)->Prev = (pHead); \
} while (0)


/* 
 * Extracts the object that holds the list head
 * pHead : The list head that resides in Obj
 * tPbj  : The type which list head resides in Obj
 * Obj   : The name of the list head in Obj
 */

#define list_get_obj(pHead, tObj, Obj) \
  ((tObj *)((char *)(pHead)-(unsigned int)(&((tObj *)0)->Obj)))

/*
 * Browse through a list
 * Pos : A pointer to a list_head used as temp object
 * list_head : The actual list to browse
 */

#define list_for_each(Pos, ListHead) \
  for (Pos = (ListHead)->Next; Pos != (ListHead); Pos = Pos->Next)

static __inline void __list_add(list_head * New, list_head * Prev, 
                               list_head * Next)
{
	Next->Prev = New;
	New->Next = Next;
	New->Prev = Prev;
	Prev->Next = New;
}

static __inline void list_add(list_head *New, list_head *List) 
{
  __list_add(New, List->Prev, List);
}

static __inline void list_del(list_head *ListHead)
{
  ListHead->Next->Prev = ListHead->Prev;
  ListHead->Prev->Next = ListHead->Next;
}

static __inline int list_isempty(list_head *Head)
{
  return Head->Next == Head;
}



#endif
