
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
 
#ifndef BTL_IFC_H
#define BTL_IFC_H

#include "btl_if.h"

/* some local debug macros */
#ifndef JNI_TARGET
#define info(fmt, ...)  fprintf (stdout, "%s: " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define debug(fmt, ...) fprintf (stdout, "%s: " fmt "\n", __FUNCTION__, ## __VA_ARGS__)
#define verbose(fmt, ...) //fprintf (stderr, "%s: " fmt "\n", __FUNCTION__, ## __VA_ARGS__)   
#define error(fmt, ...) fprintf (stderr, "##### ERROR : %s:  " fmt " #####\n", __FUNCTION__, ## __VA_ARGS__)
#else
#include "utils/Log.h"
#define info(fmt, ...)  LOGI ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define debug(fmt, ...) LOGD ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define verbose(fmt, ...) //LOGV ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define error(fmt, ...) LOGE ("##### ERROR : %s: " fmt "#####",__FUNCTION__,  ## __VA_ARGS__)
#endif

// fixme -- halt system if fail
#define ASSERTC(cond, msg, val) if (!(cond)) { error("### ASSERT : %s line %d %s (%d) ###", __FILE__, __LINE__, msg, val);}

extern const char *sub2str[];
extern fd_set read_set;
extern fd_set active_set;
extern int max_fd;

extern tBTL_IF_CB btlif_cb[BTL_IF_SUBSYSTEM_MAX];

/*******************************************************************************
**
** Function          BTL_IFC_RegisterSubSystem
**
** Description      Register subsystem
**                  
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_RegisterSubSystem(tCTRL_HANDLE *handle, tBTL_IF_SUBSYSTEM sub, 
                                                 tBTL_IF_DATA_CALLBACK data_cb, tBTL_IF_CTRL_CALLBACK ctrl_cb);

/*******************************************************************************
**
** Function          BTL_IFC_UnregisterSubSystem
**
** Description      Unregister subsystem
**                  
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_UnregisterSubSystem(tCTRL_HANDLE *handle, 
                                                tBTL_IF_SUBSYSTEM sub);

/*******************************************************************************
**
** Function          BTL_IFC_ConnectDatapath
**
** Description      
**                  
**
** Returns          tBTL_SockFd
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ConnectDatapath(tDATA_HANDLE* handle, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function           BTL_IFC_SetupListener
**
** Description      Notify remote side that a listener is available for incoming data connections.
**                       Data connect ind will be notified through ctrl callback
**                       Non blocking wait. 
**
** Returns            
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupDatapathListener(tCTRL_HANDLE ctrl, tBTL_IF_SUBSYSTEM sub, int sub_port);

/*******************************************************************************
**
** Function          BTL_IFC_SetupRxBuf
**
** Description     Setup dedicate receive buffer  
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupRxBuf(tDATA_HANDLE handle, char *p_rx, int size);


/*******************************************************************************
**
** Function          BTL_IFC_RegisterDatapath
**
** Description      Sends data frame to BTL
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SendData(tDATA_HANDLE handle, char *p, int len);

/*******************************************************************************
**
** Function          BTL_IFC_UnregisterDatapath
**
** Description      Disconnect datapath
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_DisconnectDatapath(tDATA_HANDLE handle);

/*******************************************************************************
**
** Function          BTL_IFC_CtrlSend
**
** Description     Send control msg
**                  
**
** Returns          tBTL_IF_DataPath
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_CtrlSend(int ctrl_fd, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len);

/*******************************************************************************
**
** Function          BTL_IFC_SendMsgNoParams
**
** Description     Send control msg without any parameters
**                  
**
** Returns          tBTL_IF_DataPath
**
**
*******************************************************************************/

int BTL_IFC_SendMsgNoParams(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id);


/*******************************************************************************
**
** Function          BTL_IFC_ClientInit
**
** Description      Init client
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ClientInit(void);

/*******************************************************************************
**
** Function          BTL_IFC_ClientShutdown
**
** Description      Shutdown client
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ClientShutdown(void);


/*******************************************************************************
**
** Function          BTL_IFC_SetRemoteSrv
**
** Description      Setup alternate remote server
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetRemoteSrv(const char *ip_addr);

/*******************************************************************************
**
** Function          BTLIF_AG_ConnectIndAck
**
** Description      Setup alternate remote server
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

int BTLIF_AG_ConnectIndAck(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);


/*******************************************************************************
**     AG API
**/

int BTLIF_AG_ConnectReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status);
int BTLIF_AG_ListenReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan);
int BTLIF_AG_Disconnect(tCTRL_HANDLE handle, unsigned short rf_chan);

BOOLEAN btl_ifc_main_running(void);
void btl_ifc_rxdata(tBTL_IF_SUBSYSTEM sub, tDATA_HANDLE handle, char *p, int len);
char* btl_ifc_get_srvaddr(void);
void btl_ifc_notify_local_event(tSUB sub, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
void btl_ifc_notify_rx_buf_pending(tDATA_HANDLE handle);
const char* dump_msg_id(tBTLIF_CTRL_MSG_ID id);

#endif


