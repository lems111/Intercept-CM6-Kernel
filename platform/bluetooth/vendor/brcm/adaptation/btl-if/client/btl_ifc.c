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
 

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "btl_ifc.h"
#include "btl_ifc_wrapper.h"

#if (defined LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "BTL_IFC"

#define CHECK_SUBSYSTEM(sub)     if (sub >= BTL_IF_SUBSYSTEM_MAX) { error("%s :: invalid subsystem [%d]", __FUNCTION__, sub); return FALSE;}

typedef enum {
    STATE_STOPPED,
    STATE_STARTING,        
    STATE_RUNNING,
    STATE_SHUTTING_DOWN
} t_state;

BOOLEAN sub_reg_done[BTL_IF_SUBSYSTEM_MAX] = {0};
tBTL_IF_CB btlif_cb[BTL_IF_SUBSYSTEM_MAX];

pthread_t client_thread_id;

fd_set read_set;
fd_set active_set;
int max_fd;

char *btlif_server_addr = BTLIF_IP_ADDR;

t_state state = STATE_STOPPED;

#define BTL_IFC_CTRL_CH_CONNECT_MAX_RETRY 3
static int signal_fds[2];
static inline int create_signal_fds(fd_set* set)
{
    if(signal_fds[0] == 0 && socketpair(AF_UNIX, SOCK_STREAM, 0, signal_fds) < 0)
    {
        error("create_signal_sockets:socketpair failed: %s", strerror(errno));
        return -1;
    }
    FD_SET(signal_fds[0], set);
    return signal_fds[0];
}
static inline int send_wakeup_signal()
{
    char sig_on = 1;
    return send(signal_fds[1], &sig_on, sizeof(sig_on), 0);     
}
static inline int reset_signal()
{
    char sig_recv = 0;
    recv(signal_fds[0], &sig_recv, sizeof(sig_recv), MSG_WAITALL);
    return (int)sig_recv;
}
static inline int is_signaled(fd_set* set)
{
    return FD_ISSET(signal_fds[0], set);
}


const char *sub2str[] =
{
    "NONE",         //SUB_NONE
    "CTRL",         //SUB_CTRL
    "DTUN",         //SUB_DTUN
    "AG",           //SUB_AG
    "AV",           //SUB_AV
    "FM",           //SUB_FM
    "SCO",          //SUB_SCO    
    "PR",           //SUB_PR
    "PBS",          //SUB_PBS
    "FTPS",         //SUB_FTPS
    "DUN",          //SUB_DUN
    "HH",           //SUB_HH
    "SPP",          //SUB_SPP
    "TEST",         //SUB_TEST    
    "SAPS",         //SUB_SAPS
    "BTS",          //SUB_BTS       
    "",             //SUB_MAX
};

// FIXME -- move into common client/server file

const char* dump_msg_id(tBTLIF_CTRL_MSG_ID id)
{
    switch(id)
    {
        /* COMMON */
        case BTLIF_REGISTER_SUBSYS_REQ: return "BTLIF_REGISTER_SUBSYS_REQ";
        case BTLIF_REGISTER_SUBSYS_RSP: return "BTLIF_REGISTER_SUBSYS_RSP";
        case BTLIF_UNREGISTER_SUBSYS_REQ: return "BTLIF_UNREGISTER_SUBSYS_REQ";
        case BTLIF_UNREGISTER_SUBSYS_RSP: return "BTLIF_UNREGISTER_SUBSYS_RSP";        
        case BTLIF_CONNECT_REQ: return "BTLIF_CONNECT_REQ";
        case BTLIF_CONNECT_RSP: return "BTLIF_CONNECT_RSP";
        case BTLIF_CONNECT_IND: return "BTLIF_CONNECT_IND";
        case BTLIF_DISCONNECT_REQ: return "BTLIF_DISCONNECT_REQ";
        case BTLIF_DISCONNECT_RSP: return "BTLIF_DISCONNECT_RSP";        
        case BTLIF_DISCONNECT_IND: return "BTLIF_DISCONNECT_IND";
        case BTLIF_LISTEN_REQ: return "BTLIF_LISTEN_REQ";
        case BTLIF_SET_CONFIG: return "BTLIF_SET_CONFIG";
        case BTLIF_GET_CONFIG: return "BTLIF_GET_CONFIG";        
        case BTLIF_DATA_CHAN_IND : return "BTLIF_DATA_CHAN_IND";
        case BTLIF_DATA_CHAN_DISC_IND : return "BTLIF_DATA_CH_DISC_IND";   
        case BTLIF_SUBSYSTEM_ATTACHED : return "BTLIF_SUBSYSTEM_ATTACHED";
        case BTLIF_SUBSYSTEM_DETACHED : return "BTLIF_SUBSYSTEM_DETACHED";
        case BTLIF_CONNECT_IND_ACK : return "BTLIF_CONNECT_IND_ACK";
        case BTLIF_DISC_RX_BUF_PENDING : return "BTLIF_DISC_RX_BUF_PENDING";

        /* FM */
        case BTLIF_FM_ENABLE: return "BTLIF_FM_ENABLE";
        case BTLIF_FM_DISABLE: return "BTLIF_FM_DISABLE";
        case BTLIF_FM_TUNE: return "BTLIF_FM_TUNE";
        case BTLIF_FM_MUTE: return "BTLIF_FM_MUTE";
        case BTLIF_FM_SEARCH: return "BTLIF_FM_SEARCH";
        case BTLIF_FM_SEARCH_ABORT: return "BTLIF_FM_SEARCH_ABORT";
        case BTLIF_FM_SET_RDS_MODE: return "BTLIF_FM_SET_RDS_MODE";
        case BTLIF_FM_SET_RDS_RBDS: return "BTLIF_FM_SET_RDS_RBDS";
        case BTLIF_FM_RDS_UPDATE: return "BTLIF_FM_RDS_UPDATE";
        case BTLIF_FM_AUDIO_MODE: return "BTLIF_FM_AUDIO_MODE";
        case BTLIF_FM_AUDIO_PATH: return "BTLIF_FM_AUDIO_PATH";
        case BTLIF_FM_SCAN_STEP: return "BTLIF_FM_SCAN_STEP";
        case BTLIF_FM_SET_REGION: return "BTLIF_FM_SET_REGION";  
        case BTLIF_FM_CONFIGURE_DEEMPHASIS: return "BTLIF_FM_CONFIGURE_DEEMPHASIS"; 
        case BTLIF_FM_ESTIMATE_NFL: return "BTLIF_FM_ESTIMATE_NFL"; 
        case BTLIF_FM_GET_AUDIO_QUALITY: return "BTLIF_FM_GET_AUDIO_QUALITY"; 
        case BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION: return "BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION"; 
        case BTLIF_FM_AF_JMP_EVT: return "BTLIF_FM_AF_JMP_EVT";
        case BTLIF_FM_SET_VOLUME: return "BTLIF_FM_SET_VOLUME";
        case BTLIF_FM_SET_VOLUME_EVT: return "BTLIF_FM_SET_VOLUME_EVT";

        /* DTUN */
        case BTLIF_DTUN_METHOD_CALL: return "BTLIF_DTUN_METHOD_CALL";
        case BTLIF_DTUN_SIGNAL_EVT: return "BTLIF_DTUN_SIGNAL_EVT";
    
        /* PBAP */
        case BTLIF_PBS_ENABLE:  return "BTLIF_PBS_ENABLE";
        case BTLIF_PBS_DISABLE: return "BTLIF_PBS_DISABLE";
        case BTLIF_PBS_AUTH_RSP: return "BTLIF_PBS_AUTH_RSP";
        case BTLIF_PBS_ACCESS_RSP: return "BTLIF_PBS_ACCESS_RSP";

        case BTLIF_PBS_ENABLE_EVT: return "BTLIF_PBS_ENABLE_EVT";
        case BTLIF_PBS_OPEN_EVT: return "BTLIF_PBS_OPEN_EVT";
        case BTLIF_PBS_CLOSE_EVT: return "BTLIF_PBS_CLOSE_EVT";
        case BTLIF_PBS_AUTH_EVT: return "BTLIF_PBS_AUTH_EVT";
        case BTLIF_PBS_ACCESS_EVT: return "BTLIF_PBS_ACCESS_EVT";
        case BTLIF_PBS_OPER_CMPL_EVT: return "BTLIF_PBS_OPER_CMPL_EVT";

        /* FTP Server */
        case BTLIF_FTPS_ENABLE: return "BTLIF_FTPS_ENABLE";
        case BTLIF_FTPS_DISABLE: return "BTLIF_FTPS_DISABLE";
        case BTLIF_FTPS_CLOSE: return "BTLIF_FTPS_CLOSE";        
        case BTLIF_FTPS_AUTH_RSP: return "BTLIF_FTPS_AUTH_RSP";
        case BTLIF_FTPS_ACCESS_RSP: return "BTLIF_FTPS_ACCESS_RSP";  
        case BTLIF_FTPS_ENABLE_EVT: return "BTLIF_FTPS_ENABLE_EVT";
        case BTLIF_FTPS_DISABLE_EVT: return "BTLIF_FTPS_DISABLE_EVT";
        case BTLIF_FTPS_OPEN_EVT: return "BTLIF_FTPS_OPEN_EVT";
        case BTLIF_FTPS_CLOSE_EVT: return "BTLIF_FTPS_CLOSE_EVT";
        case BTLIF_FTPS_AUTH_EVT: return "BTLIF_FTPS_AUTH_EVT";
        case BTLIF_FTPS_ACCESS_EVT: return "BTLIF_FTPS_ACCESS_EVT";    
        case BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT: return "BTLIF_FTPS_FILE_TRANSFER_IN_PRGS_CMPL_EVT";
        case BTLIF_FTPS_OPER_PUT_CMPL_EVT: return "BTLIF_FTPS_OPER_PUT_CMPL_EVT";
        case BTLIF_FTPS_OPER_GET_CMPL_EVT: return "BTLIF_FTPS_OPER_GET_CMPL_EVT";
        case BTLIF_FTPS_OPER_DEL_CMPL_EVT: return "BTLIF_FTPS_OPER_DEL_CMPL_EVT";

        /* DG Server */
        case BTLIF_DG_ENABLE: return "BTLIF_DG_ENABLE";
        case BTLIF_DG_DISABLE: return "BTLIF_DG_DISABLE";
        case BTLIF_DG_LISTEN: return "BTLIF_DG_LISTEN";
        case BTLIF_DG_OPEN: return "BTLIF_DG_OPEN";
        case BTLIF_DG_CLOSE: return "BTLIF_DG_CLOSE";
        case BTLIF_DG_SHUTDOWN: return "BTLIF_DG_SHUTDOWN";
        case BTLIF_DG_SPP_CREATE: return "BTLIF_DG_SPP_CREATE";
        case BTLIF_DG_SPP_DESTORY: return "BTLIF_DG_SPP_DESTORY";
        case BTLIF_DG_ENABLE_EVT: return "BTLIF_DG_ENABLE_EVT";
        case BTLIF_DG_DISABLE_EVT: return "BTLIF_DG_DISABLE_EVT";
        case BTLIF_DG_LISTEN_EVT: return "BTLIF_DG_LISTEN_EVT";
        case BTLIF_DG_OPENING_EVT: return "BTLIF_DG_OPENING_EVT";
        case BTLIF_DG_OPEN_EVT: return "BTLIF_DG_OPEN_EVT";
        case BTLIF_DG_CLOSE_EVT: return "BTLIF_DG_CLOSE_EVT";
        case BTLIF_DG_SHUTDOWN_EVT: return "BTLIF_DG_SHUTDOWN_EVT";    
        case BTLIF_DG_RIL_FLOW_CTRL_REQ: return "BTLIF_DG_RIL_FLOW_CTRL_REQ";
        case BTLIF_DG_RIL_FLOW_CTRL_EVT: return "BTLIF_DG_RIL_FLOW_CTRL_EVT";

        /* HH */
        case BTLIF_HH_OPEN: return "BTLIF_HH_OPEN";
        case BTLIF_HH_CLOSE: return "BTLIF_HH_CLOSE";
        case BTLIF_HH_OPEN_EVT: return "BTLIF_HH_OPEN_EVT";
        case BTLIF_HH_CLOSE_EVT: return "BTLIF_HH_CLOSE_EVT";

        /* testmode */
        case BTLIF_TST_SET_TESTMODE: return "BTLIF_TST_SET_TESTMODE";
        case BTLIF_TST_GET_TST_STATE: return "BTLIF_TST_GET_TST_STATE";
        case BTLIF_TST_TX_RX_TEST: return "BTLIF_TST_TX_RX_TEST";
        case BTLIF_TST_SEND_TST_CMD: return "BTLIF_TST_SEND_TST_CMD";

        /* BTS Server */
        case BTLIF_BTS_RFC_LISTEN_REQ: return "BTLIF_BTS_RFC_LISTEN_REQ";
        case BTLIF_BTS_RFC_LISTEN_RSP: return "BTLIF_BTS_RFC_LISTEN_RSP";
        case BTLIF_BTS_RFC_LISTEN_CANCEL: return "BTLIF_BTS_RFC_LISTEN_CANCEL";        
        case BTLIF_BTS_RFC_CON_IND: return "BTLIF_BTS_RFC_CON_IND";
        case BTLIF_BTS_RFC_CON_IND_ACK: return "BTLIF_BTS_RFC_CON_IND_ACK";          
        case BTLIF_BTS_RFC_CON_REQ: return "BTLIF_BTS_RFC_CON_REQ";
        case BTLIF_BTS_RFC_CON_RSP: return "BTLIF_BTS_RFC_CON_RSP";
        case BTLIF_BTS_RFC_CLOSE: return "BTLIF_BTS_RFC_CLOSE";
        case BTLIF_BTS_RFC_CLOSE_CFM: return "BTLIF_BTS_RFC_CLOSE_CFM";
        case BTLIF_BTS_RFC_DISC_IND: return "BTLIF_BTS_RFC_DISC_IND";
        case BTLIF_BTS_RFC_DISC_IND_ACK: return "BTLIF_BTS_RFC_DISC_IND_ACK";
        case BTLIF_BTS_RFC_LST_NOTIFY_DSOCK: return "BTLIF_BTS_RFC_LST_NOTIFY_DSOCK";
        case BTLIF_BTS_L2C_LISTEN_REQ: return "BTLIF_BTS_L2C_LISTEN_REQ";
        case BTLIF_BTS_L2C_LISTEN_RSP: return "BTLIF_BTS_L2C_LISTEN_RSP";
        case BTLIF_BTS_L2C_CONNECT_REQ: return "BTLIF_BTS_L2C_CONNECT_REQ";
        case BTLIF_BTS_L2C_CONNECT_RSP: return "BTLIF_BTS_L2C_CONNECT_RSP";
        case BTLIF_BTS_L2C_DISC_REQ: return "BTLIF_BTS_L2C_DISC_REQ";
        case BTLIF_BTS_L2C_DISC_RSP: return "BTLIF_BTS_L2C_DISC_RSP";
        case BTLIF_BTS_L2C_DISC_IND: return "BTLIF_BTS_L2C_DISC_IND";
        case BTLIF_BTS_SCO_CON_REQ: return "BTLIF_BTS_SCO_CON_REQ";
        case BTLIF_BTS_SCO_CON_RSP: return "BTLIF_BTS_SCO_CON_RSP";
        case BTLIF_BTS_SCO_DISC_REQ: return "BTLIF_BTS_SCO_DISC_REQ";
        case BTLIF_BTS_SCO_DISC_RSP: return "BTLIF_BTS_SCO_DISC_RSP";
        case BTLIF_BTS_SCO_DISC_IND: return "BTLIF_BTS_SCO_DISC_IND";
        case BTLIF_BTS_EVT_ABORT: return "BTLIF_BTS_EVT_ABORT";

        /* Printer */
        case BTLIF_PR_ENABLE:            return "BTLIF_PR_ENABLE";
        case BTLIF_PR_DISABLE:           return "BTLIF_PR_DISABLE";
        case BTLIF_PR_GET_CAPS:          return "BTLIF_PR_GET_CAPS";
        case BTLIF_PR_AUTH_RSP:          return "BTLIF_PR_AUTH_RSP";
        case BTLIF_PR_PRINT:             return "BTLIF_PR_PRINT";
        case BTLIF_PR_PARTIAL_IMAGE_RSP: return "BTLIF_PR_PARTIAL_IMAGE_RSP";
        case BTLIF_PR_REF_OBJ_RSP:       return "BTLIF_PR_REF_OBJ_RSP";
        case BTLIF_PR_ABORT:             return "BTLIF_PR_ABORT";
        case BTLIF_PR_CANCEL_BP_STATUS:  return "BTLIF_PR_CANCEL_BP_STATUS";
        case BTLIF_PR_CARD_TO_XHTML:     return "BTLIF_PR_CARD_TO_XHTML";
        case BTLIF_PR_CAL_TO_XHTML:      return "BTLIF_PR_CAL_TO_XHTML";
        case BTLIF_PR_ENABLE_EVT:        return "BTLIF_PR_ENABLE_EVT";
        case BTLIF_PR_OPEN_EVT:          return "BTLIF_PR_OPEN_EVT";
        case BTLIF_PR_PROGRESS_EVT:      return "BTLIF_PR_PROGRESS_EVT";
        case BTLIF_PR_CLOSE_EVT:         return "BTLIF_PR_CLOSE_EVT";
        case BTLIF_PR_GETCAPS_EVT:       return "BTLIF_PR_GETCAPS_EVT";
        case BTLIF_PR_AUTH_EVT:          return "BTLIF_PR_AUTH_EVT";
        case BTLIF_PR_THUMBNAIL_EVT:     return "BTLIF_PR_THUMBNAIL_EVT";
        case BTLIF_PR_PARTIAL_IMAGE_EVT: return "BTLIF_PR_PARTIAL_IMAGE_EVT";
        case BTLIF_PR_GET_OBJ_EVT:       return "BTLIF_PR_GET_OBJ_EVT";
        case BTLIF_PR_BP_DOC_CMPL_EVT:   return "BTLIF_PR_BP_DOC_CMPL_EVT";
        case BTLIF_PR_JOB_STATUS_EVT:    return "BTLIF_PR_JOB_STATUS_EVT";
        case BTLIF_PR_NOTE_TO_XHTML:     return "BTLIF_PR_NOTE_TO_XHTML";
        
        /* SAP Server */
        case BTLIF_SAPS_ENABLE:            return "BTLIF_SAPS_ENABLE";
        case BTLIF_SAPS_DISABLE:           return "BTLIF_SAPS_DISABLE";
        case BTLIF_SAPS_DISCONNECT:        return "BTLIF_SAPS_DISCONNECT";
        case BTLIF_SAPS_STATUS:            return "BTLIF_SAPS_STATUS";
        case BTLIF_SAPS_CARD_STATUS:       return "BTLIF_SAPS_CARD_STATUS";
        case BTLIF_SAPS_READER_STATUS:     return "BTLIF_SAPS_READER_STATUS";
        case BTLIF_SAPS_ENABLE_EVT:        return "BTLIF_SAPS_ENABLE_EVT";
        case BTLIF_SAPS_DISABLE_EVT:       return "BTLIF_SAPS_DISABLE_EVT";
        case BTLIF_SAPS_OPEN_EVT:          return "BTLIF_SAPS_OPEN_EVT";
        case BTLIF_SAPS_CLOSE_EVT:         return "BTLIF_SAPS_CLOSE_EVT";
        
        case BTLIF_SAPS_RIL_SIM_ON:        return "BTLIF_SAPS_RIL_SIM_ON";
        case BTLIF_SAPS_RIL_SIM_OFF:       return "BTLIF_SAPS_RIL_SIM_OFF";
        case BTLIF_SAPS_RIL_SIM_RESET:     return "BTLIF_SAPS_RIL_SIM_RESET";
        case BTLIF_SAPS_RIL_SIM_ATR:       return "BTLIF_SAPS_RIL_SIM_ATR";
        case BTLIF_SAPS_RIL_SIM_APDU:      return "BTLIF_SAPS_RIL_SIM_APDU";
        case BTLIF_SAPS_RIL_SIM_ON_EVT:    return "BTLIF_SAPS_RIL_SIM_ON_EVT";
        case BTLIF_SAPS_RIL_SIM_OFF_EVT:   return "BTLIF_SAPS_RIL_SIM_OFF_EVT";
        case BTLIF_SAPS_RIL_SIM_RESET_EVT: return "BTLIF_SAPS_RIL_SIM_RESET_EVT";
        case BTLIF_SAPS_RIL_SIM_ATR_EVT:   return "BTLIF_SAPS_RIL_SIM_ATR_EVT";
        case BTLIF_SAPS_RIL_SIM_APDU_EVT:  return "BTLIF_SAPS_RIL_SIM_APDU_EVT";
        
        default : return "UNKNOWN MSG ID";
    }
}

/*******************************************************************************
**
** Socket Rx/Tx
**          
**
*******************************************************************************/

int tx_data(int fd, char *p, int len)
{
    int result;

    verbose("tx_data %d bytes (%d)", len, fd);

    hex_dump("[TX]", p, len, 0);

    result = send(fd, p, len, 0);

    if (result != len)
    {
         error("write failed (%d) errno=%d", result,errno);
         return -1;
    }

    return result;
}


int rx_data(int fd, char *p, int len)
{
    int ret;

    verbose("rx_data wait %d (%d)", len, fd);

    ret = recv(fd, p, len, MSG_WAITALL);
    
    hex_dump("[RX]", p, len, 0);

    if (ret == 0)
    {
        verbose("read 0, disconnected.");
    }
    else if (ret < 0)
    {
       error("failed");
    }
    return ret;
}


char *btl_ifc_get_srvaddr(void)
{
   return btlif_server_addr;
}

/*******************************************************************************
**
** Control channel
**          
**
*******************************************************************************/

void ctrl_init(void)
{
    int i;
    for (i=0; i< BTL_IF_SUBSYSTEM_MAX; i++)
    {
        btlif_cb[i].ctrl_cb = NULL;
        btlif_cb[i].data_cb = NULL;
        btlif_cb[i].ctrl_fd = DATA_SOCKET_INVALID;
        btlif_cb[i].sub = SUB_NONE;
    }
}

void ctrl_close_all(void)
{
    int i;
    for (i=0; i< BTL_IF_SUBSYSTEM_MAX; i++)
    {
        if (btlif_cb[i].ctrl_fd == DATA_SOCKET_INVALID)
            continue;
        
        verbose("closing ctrl (%s)", sub2str[i]);
        close(btlif_cb[i].ctrl_fd);

        btlif_cb[i].ctrl_cb = NULL;
        btlif_cb[i].data_cb = NULL;
        btlif_cb[i].ctrl_fd = DATA_SOCKET_INVALID;
        btlif_cb[i].sub = SUB_NONE;

        /* make sure we close any remaining data sockets not closed by client app */
        wrp_close_sub_all(i);
    }
}

void ctrl_socket_disconnected(tSUB sub)
{
    t_wsock *ws;

    info("[%s] ctrl socket disconnected, close (%d)", sub2str[sub], btlif_cb[sub].ctrl_fd);
    FD_CLR(btlif_cb[sub].ctrl_fd, &active_set);    

    ws = wrp_find_wsock(btlif_cb[sub].ctrl_fd);

    if (!ws)
    {
        error("%s : no wsock found, already disconnected remotely ?", __FUNCTION__);
        /* proceed to unregister subsystem locally */
    }
    else   
       wrp_close_full(ws);

    /* make sure we close any remaining data sockets not closed by client app */
    wrp_close_sub_all(sub);

    sub_reg_done[sub] = FALSE;

    /* notify user that the ctrl channel for this subsystem is down */
    btl_ifc_notify_local_event(sub, BTLIF_SUBSYSTEM_DETACHED, NULL);    

    /* close control path socket fd */
    close(btlif_cb[sub].ctrl_fd);

    /* clear this subsystem */
    btlif_cb[sub].ctrl_fd = DATA_SOCKET_INVALID;
}

/*****************************************************************************************

coder : Henry 20000310
content : sub value compensation
error : sometimes, we got error of sub value from btl_ifs.c to btl_ifc.c
03-09 18:51:03.722: INFO/BTL-IFS(2054): send_ctrl_msg: [BTL_IFS CTRL] send BTLIF_PBS_ACCESS_EVT (PBS) 334 pbytes (hdl 13)
03-09 18:51:03.727: INFO/BTL_IFC(1906): btl_ifc_ctrl_rx: [BTL_IFC CTRL] recv BTLIF_PBS_ACCESS_EVT (SPP) 336 pbytes (hdl 110)                

"NONE",         //SUB_NONE   0
"CTRL",         //SUB_CTRL  1
"DTUN",         //SUB_DTUN  2
"AG",           //SUB_AG   3
"AV",           //SUB_AV   4
"FM",           //SUB_FM   5
"SCO",          //SUB_SCO   6   
"PR",           //SUB_PR   7
"PBS",          //SUB_PBS   8
"FTPS",         // SUB_FTPS   9
"DUN",          // SUB_DUN  10
"HH",            // SUB_HH  11
"SPP",          // SUB_SPP  12
"TEST",         // SUB_TEST  13
"SAPS"        // SUB_SAPS  14
"BTS",         // SUB_BTS   15
"",              // SUB_MAX 16

*****************************************************************************************/
/* SecFeature.SPRINT by jheeya.kim 
tSUB btl_ifc_sub2str_evt_compensation(tBTLIF_CTRL_MSG_ID id)
{
  tSUB sub;

  sub = 0;

  if(id >= BTLIF_PR_EVT_BASE && id <= BTLIF_PR_EVT_END)
    sub = 7;
  else if(id >= BTLIF_PBS_EVT_BASE && id <= BTLIF_PBS_EVT_END)
    sub = 8;
  else if(id >= BTLIF_FTPS_EVT_BASE && id <= BTLIF_FTPS_EVT_END)
    sub = 9;
  else if(id >= BTLIF_DG_EVT_BASE && id <= BTLIF_DG_EVT_END)
    sub = 10;  
  else if(id >= BTLIF_HH_EVT_BASE && id <= BTLIF_HH_EVT_END)
    sub = 11;
  else if(id >= BTLIF_TST_EVT_BASE && id <= BTLIF_TST_EVT_END)
    sub = 13;
  else if(id >= BTLIF_SAPS_EVT_BASE && id <= BTLIF_SAPS_EVT_END)
    sub = 14;
  else if(id >= BTLIF_BTS_EVT_BASE && id <= BTLIF_BTS_EVT_END)
    sub = 15;
  
  return sub;  


} */

void btl_ifc_ctrl_rx(int fd, tSUB sub, char *p)
{
    tBTLIF_CTRL_MSG_PKT *pkt;
    // SecFeature.SPRINT by jheeya.kim tSUB sub_recovery;  // Henry 20000310
    
    pkt = (tBTLIF_CTRL_MSG_PKT *)p;

    info("[BTL_IFC CTRL] recv %s (%s) %d pbytes (hdl %d)", dump_msg_id(pkt->hdr.id), sub2str[sub], pkt->hdr.len, fd);

    // Henry : sub value compensate
/* SecFeature.SPRINT by jheeya.kim 
    sub_recovery = btl_ifc_sub2str_evt_compensation(pkt->hdr.id);

    if(sub_recovery == 0)
    {
      info("sub_recovery is zero, keep going");
      sub_recovery = sub;
    }

    info("Henry : compensation %d,  %s", sub_recovery, sub2str[sub_recovery]);

    if (btlif_cb[sub_recovery].ctrl_cb)
        btlif_cb[sub_recovery].ctrl_cb(fd, pkt->hdr.id, &pkt->params);
    else
        error("no callback configured for subsystem [%s]", sub2str[sub_recovery]);
*/
// SecFeature.SPRINT by jheeya.kim
    if (btlif_cb[sub].ctrl_cb)
        btlif_cb[sub].ctrl_cb(fd, pkt->hdr.id, &pkt->params);
    else
        error("no callback configured for subsystem [%s]", sub2str[sub]);
}

int btl_ifc_ctrl_tx(tSUB sub, tBTLIF_CTRL_MSG_PKT *pkt, int len)
{    
    return tx_data(btlif_cb[sub].ctrl_fd, (char *)pkt, len);
}

static int send_ctrl_msg(int fd, tSUB sub, tBTLIF_CTRL_MSG_ID type, tBTL_PARAMS *params, int param_len)
{
    tBTLIF_CTRL_MSG_PKT pkt;

    pkt.hdr.len = sizeof(tSUB) + param_len;
    pkt.hdr.id = type;
    pkt.sub = sub;

    info("[BTL_IFC CTRL] send %s (%s) %d pbytes (hdl %d)", dump_msg_id(type), sub2str[sub], param_len, fd);

    if (param_len)
        memcpy(&pkt.params, params, param_len); 

    return btl_ifc_ctrl_tx(sub, &pkt, sizeof(tBTL_IF_HDR) + sizeof(tSUB) + param_len); 
}

static int send_register_subsystem(int fd, tBTL_IF_SUBSYSTEM sub)
{
   return send_ctrl_msg(fd, sub, BTLIF_REGISTER_SUBSYS_REQ, NULL, 0);
}   

/* fixme -- unify rx path handling with process_ctrl_data */
static tBTL_IF_Result wait_register_subsystem_rsp(int fd)
{
    char rx_buf[BTLIF_CTRL_RX_BUF_MAX];
    tBTL_IF_HDR *hdr = (tBTL_IF_HDR*)rx_buf;
    tBTLIF_CTRL_MSG_PKT *pkt;

    /* wait for header */
    if (rx_data(fd, (char *)hdr, sizeof(tBTL_IF_HDR)) <= 0)
    {
        return BTL_IF_GENERIC_ERR;
    }
    
    /* wait for payload */
    if (rx_data(fd, rx_buf+sizeof(tBTL_IF_HDR), hdr->len) <=0)
    {
       return BTL_IF_GENERIC_ERR;
    }

    pkt = (tBTLIF_CTRL_MSG_PKT *)rx_buf;

    return pkt->params.result.result;
}

static int send_unregister_subsystem(int fd, tBTL_IF_SUBSYSTEM sub)
{
   return send_ctrl_msg(fd, sub, BTLIF_UNREGISTER_SUBSYS_REQ, NULL, 0);
}   

/* fixme -- unify rx path handling with process_ctrl_data */
static tBTL_IF_Result wait_unregister_subsystem_rsp(int fd)
{
    char rx_buf[BTLIF_CTRL_RX_BUF_MAX];
    tBTL_IF_HDR *hdr = (tBTL_IF_HDR*)rx_buf;
    tBTLIF_CTRL_MSG_PKT *pkt;

    /* wait for header */
    if (rx_data(fd, (char *)hdr, sizeof(tBTL_IF_HDR)) <= 0)
    {
        return BTL_IF_GENERIC_ERR;
    }
    
    /* wait for payload */
    if (rx_data(fd, rx_buf+sizeof(tBTL_IF_HDR), hdr->len) <=0)
    {
       return BTL_IF_GENERIC_ERR;
    }

    pkt = (tBTLIF_CTRL_MSG_PKT *)rx_buf;

    return pkt->params.result.result;
}



/*******************************************************************************
**
** 
**          
**
*******************************************************************************/

BOOLEAN btl_ifc_main_running(void)
{
    return state == STATE_RUNNING;
}
    
void btl_ifc_rxdata(tBTL_IF_SUBSYSTEM sub, tDATA_HANDLE handle, char *p, int len)
{
    info("btl_if_rxdata : [%s], %d bytes", sub2str[sub], len);
      
    /* call handler if it is configured */
    if (btlif_cb[sub].data_cb)
        btlif_cb[sub].data_cb(handle, p, len);
}


int btl_ifc_ctrl_connect(tSUB sub)
{
    int sock;
    t_wsock *ws;
    int retry_cnt = BTL_IFC_CTRL_CH_CONNECT_MAX_RETRY;

    info("Connect control channel for subsystem [%s]", sub2str[sub]);

    /* create control socket for this sub system */
    sock = wrp_sock_create(SUB_CTRL);

    if (sock < 0)
        return -1;
    
    ws = wrp_find_wsock(sock);

    sock = wrp_sock_connect(ws, sock, btlif_server_addr, BTLIF_PORT_BASE_CTRL);
#ifndef DTUN_LOCAL_SERVER_ADDR
    while ((sock<0) && retry_cnt--)
    {
        /* if connection failed, try alternate server */

        error("Primary server failed (err %d)", errno);

        info("Try alternate server (emul host)");

        wrp_close_full(ws);
        sock = wrp_sock_create(SUB_CTRL);
        sock = wrp_sock_connect(ws, sock, BTLIF_IP_ADDR_ALT, BTLIF_PORT_BASE_CTRL);

        if (sock<0)
        {
            error("Secondary server failed %s", strerror(errno));
            usleep(500000);
            continue;
        }
        
        /* success, use alt server from now on */
        btlif_server_addr = BTLIF_IP_ADDR_ALT;        
    }
#endif
    if (sock < 0)
    {
        error("control channel failed %s", strerror(errno));
        wrp_close_full(ws);
        return -1;
    }
    
    return sock;
}


/* return 1 if fd was processed */
int process_ctrl_data(tSUB sub)
{
    /* use one dedicated static buffer for all connections */
    static char ctrl_rx_buf[BTLIF_CTRL_RX_BUF_MAX];
    tBTL_IF_HDR *hdr = (tBTL_IF_HDR*)ctrl_rx_buf;
    int fd = btlif_cb[sub].ctrl_fd;

    if ((fd != DATA_SOCKET_INVALID) && (FD_ISSET(fd, &read_set)))
    {
        int rx_discard = 0;    
        
        /* wait for header */
        if (rx_data(fd, (char *)hdr, sizeof(tBTL_IF_HDR)) <= 0)
        {
            ctrl_socket_disconnected(sub);
            return 1;
        }
        
        if (hdr->len > BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR))
        {
            rx_discard = hdr->len - BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR);
            error("[BTL_IFC] rx pkt size too big, truncate data (%d out of %d)", rx_discard, hdr->len);
            hdr->len=BTLIF_CTRL_RX_BUF_MAX-sizeof(tBTL_IF_HDR);                
        }   
        
        /* wait for payload */
        if (rx_data(fd, ctrl_rx_buf+sizeof(tBTL_IF_HDR), hdr->len) <=0)
        {
           ctrl_socket_disconnected(sub);
           return 1;
        }
    
        verbose("\t[SERVER-RX] %d bytes", hdr->len);
        
        btl_ifc_ctrl_rx(fd, sub, ctrl_rx_buf);

        /* now discard any excess */
        if (rx_discard)
            rx_data(fd, ctrl_rx_buf, rx_discard);        

        return 1;
    }
    return 0;
}


/* returns nbr of processed fds */
int monitor_ctrl_fds(void)
{
    int sub;
    int fds_processed = 0;
    
    /* manage incoming data */
    for (sub=0; sub < BTL_IF_SUBSYSTEM_MAX; sub++)
    {   
        fds_processed += process_ctrl_data(sub);
    }

    return fds_processed;
}

void btl_ifc_notify_local_event(tSUB sub, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    info("Notify local event %s", dump_msg_id(id));

    send_wakeup_signal();

    /* Notify user of new data connection. */  
    if (btlif_cb[sub].ctrl_cb)
        btlif_cb[sub].ctrl_cb(btlif_cb[sub].ctrl_fd, id, params);

    /* To make sure we don't get a data callback before user has processed 
       the data chan ind we add to active set after the notification */

    if ((id == BTLIF_DATA_CHAN_IND) && btlif_cb[sub].data_cb)
    {
        info("Datacallback registered, add to active list");
        wrp_add_to_active_set(params->chan_ind.handle);
    }
}

/* notify we are disconnecting and rx buffer is pending */
void btl_ifc_notify_rx_buf_pending(tDATA_HANDLE handle)
{
    tBTL_PARAMS params;
    t_wsock *ws;

    ws = wrp_find_wsock(handle);    

    if (ws && ws->rx_buf && ws->rx_buf_pending)
    {
        info("rx buffer pending, notify user (0x%x %d)", ws->rx_buf, ws->rx_buf_size);
        
        params.rx_buf_pnd.handle = ws->wsock;
        params.rx_buf_pnd.rx_buf = ws->rx_buf;
        params.rx_buf_pnd.rx_buf_size = ws->rx_buf_size;

        /* reset pending flag and notify */
        ws->rx_buf_pending = 0;
        btl_ifc_notify_local_event(ws->sub, BTLIF_DISC_RX_BUF_PENDING, &params);
    }
}

static void main_client_thread(void *p)
{
    struct timeval tv;
    int result;
    int fds_processed = 0;
    
    info("Client main thread starting");

    state = STATE_RUNNING;


    while (state == STATE_RUNNING)
    {
        
        read_set = active_set;
        
       /* wait for a connection or socket data, blocking call */
        result = select(max_fd+1, &read_set, NULL, NULL, NULL);

        if (result == 0) 
        {
            //debug("select timeout (max fd %d)", max_fd);
            continue;
        }
        else if (result < 0 && errno != EINTR) 
        {
            error("select err %s", strerror(errno));
            continue;
        }

        /* Exit if state had changed to no longer RUNNING while we were waiting for 'select' */
        if (state != STATE_RUNNING)
        {
            info("main_client_thread: bt shutdown while waiting for select");
            break;
        }
       if (FD_ISSET(0, &read_set))
        {
            FD_CLR(0, &active_set);
        }

        if(is_signaled(&read_set))
        {
            verbose("signal socket signaled");
            reset_signal();
            fds_processed++;
        }

        /* check for incoming data on active handles  if data handler is registered */
        fds_processed += wrp_check_active_handles();
        
        /* check for incoming ctrl msg */
        fds_processed += monitor_ctrl_fds();

        if (fds_processed != result)
        {
            error("%d fds remaining to process", result-fds_processed);
            sleep(1);
        }
        fds_processed = 0;
    }      

    info("Client main thread shutting down");

    /* close all ctrl handles */
    ctrl_close_all();
}

/*******************************************************************************
**                                   BTL IF  CLIENT API
**
*******************************************************************************/

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

tBTL_IF_Result BTL_IFC_RegisterSubSystem(tCTRL_HANDLE *handle, 
                                                tBTL_IF_SUBSYSTEM sub, 
                                                tBTL_IF_DATA_CALLBACK data_cb, 
                                                tBTL_IF_CTRL_CALLBACK ctrl_cb)
{
    tBTL_IF_Result result;
    
    CHECK_SUBSYSTEM(sub);

    if (sub_reg_done[sub])
    {
        //debug("Subsystem %s already registered", sub2str[sub]);
        *handle = btlif_cb[sub].ctrl_fd;
        return BTL_IF_SUCCESS;
    }

    info("Register subsystem [%s]", sub2str[sub]);    
 
    btlif_cb[sub].ctrl_fd = btl_ifc_ctrl_connect(sub);

    if (btlif_cb[sub].ctrl_fd < 0)
        return BTL_IF_CTRL_CH_FAILED;

    btlif_cb[sub].data_cb = data_cb;
    btlif_cb[sub].ctrl_cb = ctrl_cb;    

    *handle = btlif_cb[sub].ctrl_fd;

    /* register subsystem in server */
    send_register_subsystem(btlif_cb[sub].ctrl_fd, sub);

    /* wait for response */
    result = wait_register_subsystem_rsp(btlif_cb[sub].ctrl_fd);

    debug("add new ctrl fd to active set");

    /* now route all ctrl path data through main select */
    FD_SET(btlif_cb[sub].ctrl_fd, &active_set);
    max_fd = MAX(btlif_cb[sub].ctrl_fd, max_fd);  

    sub_reg_done[sub] = TRUE;
    
    send_wakeup_signal();

    return result;
}



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
                                                tBTL_IF_SUBSYSTEM sub)
{
    tBTL_IF_Result result;
    
    CHECK_SUBSYSTEM(sub);

    if (sub_reg_done[sub] == FALSE)
    {
        debug("Subsystem %s not registered", sub2str[sub]);
        return BTL_IF_GENERIC_ERR;
    }

    info("Unregister subsystem [%s]", sub2str[sub]);

    /* unregister subsystem in server */
    send_unregister_subsystem(btlif_cb[sub].ctrl_fd, sub);

    /* wait for response */
    result = wait_unregister_subsystem_rsp(btlif_cb[sub].ctrl_fd);
    
    /* drop ctrl socket */
    ctrl_socket_disconnected(sub);

    return result;
}

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

tBTL_IF_Result BTL_IFC_ConnectDatapath(tDATA_HANDLE* handle, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    int s;
    t_wsock *ws;

    info("Connect client datapath [%s] on sub port %d", sub2str[sub], sub_port);
  
    CHECK_SUBSYSTEM(sub);

    s = wrp_sock_create(sub);
    ws = wrp_find_wsock(s);

    /* establish connection */
    if (wrp_sock_connect(ws, s, btlif_server_addr, wrp_getport(sub, sub_port)) == -1) 
    {
        /* free wsock */
        wrp_wsock_init(ws);
        return BTL_IF_GENERIC_ERR;
    }

    /* add handle to active list if data cb is registered */
    if (btlif_cb[sub].data_cb)
       wrp_add_to_active_set(s);
       
    send_wakeup_signal();

    *handle = s;
    
    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function           BTL_IFC_WaitForDataChan
**
** Description      Blocking wait
**                  
**
** Returns            
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_WaitForDataChan(tDATA_HANDLE* handle, 
                                              tBTL_IF_SUBSYSTEM sub, 
                                              int sub_port)
{
    t_wsock *ws;
    tDATA_HANDLE hdl;
    int s;
    int result;

    info("Waiting for datachannel [%s]", sub2str[sub]);    

    CHECK_SUBSYSTEM(sub); 

    s = wrp_sock_create(sub);       
    ws = wrp_find_wsock(s);

    result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(sub, sub_port));    

    if (result < 0)
        return BTL_IF_GENERIC_ERR;

    wrp_sock_listen(ws, s); 

    hdl = wrp_sock_accept(ws, s);

    if (hdl<0)
        return BTL_IF_GENERIC_ERR;

    *handle = hdl;

    return BTL_IF_SUCCESS;    
}

/*******************************************************************************
**
** Function           BTL_IFC_SetupDatapathListener
**
** Description      Notify remote side that a listener is available for incoming data connections.
**                       Data connect ind will be notified through ctrl callback
**                       Non blocking wait. 
**
** Returns            
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupDatapathListener(tCTRL_HANDLE ctrl, tBTL_IF_SUBSYSTEM sub, int sub_port)
{
    int result;
    tBTL_PARAMS param;
    t_wsock *ws;
    int s;
    
    info("Setup listener for datachannel [%s] sub port %d", sub2str[sub], sub_port);    

    CHECK_SUBSYSTEM(sub); 
    
    s = wrp_sock_create(sub);
    
    ws = wrp_find_wsock(s);
    
    result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(sub, sub_port));

    if (result < 0)
        return BTL_IF_GENERIC_ERR;

    wrp_sock_listen(ws, s); 

    /* add to active set in order to detect an incoming connection in wrp_check_fdset */
    wrp_add_to_active_set(s);

    send_wakeup_signal();

    param.listenreq.subport = sub_port;

    /* Notify server that we have a listener available */
    BTL_IFC_CtrlSend(ctrl, sub, BTLIF_LISTEN_REQ, &param, sizeof(tBTLIF_LISTEN_REQ_PARAM));   

    if (result<0)
        return BTL_IF_GENERIC_ERR;
    
    return BTL_IF_SUCCESS;
}
 
 
/*******************************************************************************
**
** Function          BTL_IFC_SetupRxBuf
**
** Description      
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupRxBuf(tDATA_HANDLE handle, char *p_rx, int size)
{ 
    int s;
    t_wsock *ws = wrp_find_wsock(handle);

    if (!ws)
    {
        error("BTL_IFC_SetupRxBuf, could not find wsock");
        return BTL_IF_GENERIC_ERR;
    }
  
    verbose("[BTL_IFC] Set receive buffer 0x%x, %d bytes", (int)p_rx, size);

    wrp_setup_rxbuf(ws, p_rx, size);
 
    return BTL_IF_SUCCESS;    
}

/*******************************************************************************
**
** Function          BTL_IFC_SetupRxFlow
**
** Description      
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_SetupRxFlow(tDATA_HANDLE handle, BOOLEAN flow_on)
{ 
    int s;
    t_wsock *ws = wrp_find_wsock(handle);

    if (!ws)
    {
        error("BTL_IFC_SetupRxFlow, could not find wsock");
        return BTL_IF_GENERIC_ERR;
    }
  
    verbose("BTL_IFC_SetupRxFlow : handle %d, flow_on %d", handle, flow_on);

    wrp_setup_rxflow(ws, flow_on);
 
    return BTL_IF_SUCCESS;    
}


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

tBTL_IF_Result BTL_IFC_SendData(tDATA_HANDLE handle, char *p, int len)
{
    debug("[BTL_IFC] send %d bytes on hdl %d", len, handle);
    
    if (tx_data(handle, p, len) == FALSE)
    { 
        error("[BTL_IFC] send failed");
        return BTL_IF_GENERIC_ERR;
    }
    return BTL_IF_SUCCESS;   
}

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

tBTL_IF_Result BTL_IFC_DisconnectDatapath(tDATA_HANDLE handle)
{
    t_wsock *ws = wrp_find_wsock(handle);
    
    info("[BTL_IFC] disconnect datapath hdl %d", handle);

    /* make sure flow is off before disconnecting link */
    //wrp_setup_rxflow(ws, FALSE);
    /* notify user in case there is an unprocessed buffer */
    //btl_ifc_notify_rx_buf_pending(handle);
 
    if (ws) 
    {
       /* Make sure flow off before disconnecting the link */
       wrp_setup_rxflow(ws, FALSE);
       send_wakeup_signal();
       wrp_close_full(ws);
    }
    else
    {
       error("ERROR : BTL_IFC_DisconnectDatapath : no wsock found");
    }

    return BTL_IF_SUCCESS;
}


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

tBTL_IF_Result BTL_IFC_CtrlSend(int ctrl_fd, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len)
{
    int result;

    debug("%s", __FUNCTION__);

    CHECK_SUBSYSTEM(sub);

    result = send_ctrl_msg(ctrl_fd, sub, msg_id, params, param_len);

    if (result<0)
    {
        error("[BTL_IFC CTRL] send failed");
        return BTL_IF_GENERIC_ERR;
    }
    return BTL_IF_SUCCESS;    
}

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

int BTL_IFC_SendMsgNoParams(tCTRL_HANDLE handle, tSUB sub, tBTLIF_CTRL_MSG_ID msg_id)
{
    int result;

    CHECK_SUBSYSTEM(sub);

    result = BTL_IFC_CtrlSend(handle, sub, msg_id, NULL, 0);    

    if (result<0)
        return BTL_IF_GENERIC_ERR;

    return BTL_IF_SUCCESS;        
}
/*******************************************************************************
**
** Function          BTL_IF_ClientInit
**
** Description      Init client
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/
       
tBTL_IF_Result BTL_IFC_ClientInit(void)
{
    verbose("%s", __FUNCTION__);

    if (state == STATE_RUNNING)
    {
        //error("Client already initiated");
        return BTL_IF_ALREADY_INITIALIZED;
    }

    wrp_init();    

    ctrl_init();
    
    FD_ZERO(&read_set);
    FD_ZERO(&active_set);

    max_fd = create_signal_fds(&active_set);  

    state = STATE_STARTING;      

    /* create main server thread */
    if (pthread_create(&client_thread_id, NULL, 
                   (void*)main_client_thread, NULL)!=0)
        error("pthread_create : %s", strerror(errno));

    /* make sure thread starts up before exiting */
    while (state != STATE_RUNNING)
        sleep(1);

    return BTL_IF_SUCCESS;
}

/*******************************************************************************
**
** Function          BTL_IF_ClientShutdown
**
** Description      Shutdown client
**                  
**
** Returns          tBTL_IF_Result
**
*******************************************************************************/

tBTL_IF_Result BTL_IFC_ClientShutdown(void)
{
    debug("%s", __FUNCTION__);

    state = STATE_SHUTTING_DOWN;
    send_wakeup_signal();

    return BTL_IF_SUCCESS;
}


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

tBTL_IF_Result BTL_IFC_SetRemoteSrv(const char *ip_addr)
{
    debug("%s : [%s]", __FUNCTION__, ip_addr);

    btlif_server_addr = (char *)ip_addr;

    return BTL_IF_SUCCESS;
}


/*******************************************************************************
**
**
**
**
*******************************************************************************/

int BTLIF_AG_ConnectReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    memcpy(params.ag_conreq.bd, p_bd, 6); 
    params.ag_conreq.rf_chan = rf_chan;

    hex_dump("BTLIF_AG_ConnectReq", &params, 8, 0);

    return BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_REQ, &params, sizeof(tBTLIF_AG_CONNECT_REQ_PARAM));
}

int BTLIF_AG_ConnectIndAck(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    memcpy(params.ag_conreq.bd, p_bd, 6); 
    params.ag_conreq.rf_chan = rf_chan;

    hex_dump("BTLIF_AG_ConnectIndAck", &params, 8, 0);

    return BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_IND_ACK, &params, sizeof(tBTLIF_AG_CONNECT_REQ_PARAM));
}

int BTLIF_AG_ConnectRsp(tCTRL_HANDLE handle, tBTL_IF_Result result, tSTATUS status)
{
    tBTL_PARAMS params;

    params.conrsp.result = result;
    params.conrsp.status = status;
   
    return BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_RSP, &params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
}


int BTLIF_AG_ListenReq(tCTRL_HANDLE handle, BD_ADDR *p_bd, unsigned short rf_chan)
{
    tBTL_PARAMS params;

    memcpy(params.ag_listen.bd, p_bd, 6); 
    params.ag_listen.rf_chan = rf_chan;

    return BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_LISTEN_REQ, &params, sizeof(tBTLIF_AG_LISTEN_REQ_PARAM));
}


int BTLIF_AG_Disconnect(tCTRL_HANDLE handle, unsigned short rf_chan)
{
    tBTL_PARAMS params;
    params.ag_disc.rf_chan = rf_chan;

    return BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_DISCONNECT_REQ, &params, sizeof(tBTLIF_AG_DISCONNECT_PARAM));
}


#if defined(BTL_IF_CLIENT_TEST_ENABLE) && !defined(JNI_TARGET)


/*******************************************************************************
**
**
**       AG Test code
**
**
*******************************************************************************/

tBTL_IF_Result result;

#define RF_CHAN 3

char testdata[1024];
tCTRL_HANDLE ag_chandle;
tDATA_HANDLE ag_dhandle;

void AG_ReceiveData(int fd, char *p, int len)
{
    info("[DATA] Received %d bytes", len); 
}

void AG_ReceiveCtrlMsg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    debug("[CTRL] Received message [%s]", dump_msg_id(id));

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
            {                
                /* accept incoming connection */
                BTLIF_AG_ConnectRsp(handle, BTL_IF_SUCCESS, 0);

                /* now setup data connection */
                BTL_IFC_ConnectDatapath(&ag_dhandle, SUB_AG, 0);
            }
            break;
            
        case BTLIF_CONNECT_RSP:
            {
                int i;

                /* Incoming connection test scenario */
                if (params->conrsp.result != BTL_IF_SUCCESS)
                {
                    error("Connection failed :: result %d, status %d", params->conrsp.result, params->conrsp.status );
                    return;
                }

                /* server accepted our connect request, now setup actual datachannel.
                                    we will only have 1 datachannel for this subsystem so lets use subport 0 */
                BTL_IFC_ConnectDatapath(&ag_dhandle, SUB_AG, RF_CHAN);

                memset(testdata, 'a', 1024);

                /* send some testdata */    
                i=5;
                while (i--)
                {
                   BTL_IFC_SendData(ag_dhandle, testdata, 1024);
                   //sleep(1);
                }

                sleep(1);
                info("Now disconnecting link");

                BTLIF_AG_Disconnect(ag_chandle, RF_CHAN);

                sleep(1);
            
                /* We are done, unregister datapath */
                BTL_IFC_DisconnectDatapath(ag_dhandle);                    
            }
            break;
        
        case BTLIF_CONNECT_IND:
            {
                
            }
            break;
            
        case BTLIF_DISCONNECT_RSP:
            {
              
            }
            break;
        
        case BTLIF_DISCONNECT_IND:
           {
              /* data channel disconnected by server */
           }
           break;
            
        default: 
            break;
    }
}


void AG_TestInitConnection(void)
{
    BD_ADDR bd = {0,1,2,3,4,5};

    info("AG_TestInitConnection");

    /* Initialize datpath client  */
    BTL_IFC_ClientInit();

    /* register with datapath server */   
    result = BTL_IFC_RegisterSubSystem(&ag_chandle, SUB_AG, AG_ReceiveData, AG_ReceiveCtrlMsg);

    if (result!=BTL_IF_SUCCESS)
    {
       info("Subsystem registration failed (%d)", result);
       exit(0);
    }

    BTLIF_AG_ConnectReq(ag_chandle, &bd, RF_CHAN);

    sleep(5);
    debug("Test done");    
    
}

void AG_TestWaitConnection(void)
{
    info("AG_TestWaitConnection");

    /* Initialize datpath client  */
    BTL_IFC_ClientInit();

    /* register with datapath server */   
    result = BTL_IFC_RegisterSubSystem(&ag_chandle, SUB_AG, AG_ReceiveData, AG_ReceiveCtrlMsg);

    if (result!=BTL_IF_SUCCESS)
    {
       info("Subsystem registration failed (%d)", result);
       exit(0);
    }

    BTL_IFC_SetupDatapathListener(ag_chandle, SUB_AG, RF_CHAN);

    sleep(5);
    debug("Test done");

}




//////////////////////////////////////////////////////////////////////////////////////////////////////////

tCTRL_HANDLE sco_chandle;
tDATA_HANDLE sco_dhandle;

void SCO_ReceiveData(int fd, char *p, int len)
{
    info("[SCO DATA] Received %d bytes", len);    
}


void SCO_ReceiveCtrlMsg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    debug("[SCO CTRL] Received message [%s]", dump_msg_id(id));

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
            {                
                /* accept incoming connection */
                BTL_IFC_CtrlSend(handle, SUB_AG, BTLIF_CONNECT_RSP, NULL, 0);

                /* now setup data connection */
                BTL_IFC_ConnectDatapath(&sco_dhandle, SUB_SCO, 0);
            }
            break;
            
        case BTLIF_CONNECT_RSP:
            {
                /* Incoming connection test scenario */
                if (params->conrsp.result == BTL_IF_SUCCESS)
                {
                    int i;

                    /* server accepted our connect request, now setup actual datachannel.
                                        we will only have 1 datachannel for this subsystem so lets use subport 0 */
                    BTL_IFC_ConnectDatapath(&sco_dhandle, SUB_SCO, 0);
   
                    memset(testdata, 's', 1024);

                    /* send some testdata */    
                    i=5;
                    while (i--)
                       BTL_IFC_SendData(sco_dhandle, testdata, 1024);

                    info("Now disconnecting link");

                    sleep(1);

                    BTL_IFC_SendMsgNoParams(sco_chandle, SUB_SCO, BTLIF_DISCONNECT_REQ);

                    /* We are done, unregister datapath */
                    BTL_IFC_DisconnectDatapath(sco_dhandle);                    
                }
                else
                {
                    info("SCO connection failed");
                    
                }
            }
            break;
        
        case BTLIF_CONNECT_IND:
            {
                /* there is an incoming datachannel, setup a listener on subport 0 */
                BTL_IFC_WaitForDataChan(&sco_dhandle, SUB_SCO, 0);

                info("Got data channel %d", sco_dhandle);                
            }
            break;
            
        case BTLIF_DISCONNECT_RSP:
            {
              
            }
            break;
        
        case BTLIF_DISCONNECT_IND:
           {
              /* data channel disconnected by server */
           }
           break;
            
        default: 
            break;
    }
}


void SCO_TestInitConnection(void)
{
    info("SCO_TestInitConnection");

    /* Initialize datpath client  */
    BTL_IFC_ClientInit();

    result = BTL_IFC_RegisterSubSystem(&sco_chandle, SUB_SCO, SCO_ReceiveData, SCO_ReceiveCtrlMsg);

    if (result!=BTL_IF_SUCCESS)
    {
       info("Subsystem registration failed (%d)", result);
       exit(0);
    }

    /* connect sub port 0 */
    BTL_IFC_CtrlSend(sco_chandle, SUB_SCO, BTLIF_CONNECT_REQ, NULL, 0);

    sleep(3);    
}

///////////////////////////////////////////////////////////////////////////////////


void FM_ReceiveCtrlMsg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params) {
    info("FM_ReceiveCtrlMsg: Received id: %d", id);
}


void FM_TestInitConnection(void)
{
    int s;    
    tCTRL_HANDLE ctrl;

    info("FM_TestInitConnection");

    /* Initialize datpath client  */
    BTL_IFC_ClientInit();
    

    /* register with datapath server */   
    s = BTL_IFC_RegisterSubSystem(&ctrl, SUB_FM, NULL, FM_ReceiveCtrlMsg);

    if (s < 0)
        return;

    tBTLIF_FM_REQ_I_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (int) 99;

    res = BTL_IFC_CtrlSend(ctrl, SUB_FM, BTLIF_FM_ENABLE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

    info("FM_TestInitConnection: Sent BTLIF_FM_ENABLE to fd: %i", s);
    

    sleep(1);    

    while(1)
        sleep(1);
#if 0
#endif

}


void AV_TestInitConnection(void)
{
    tCTRL_HANDLE av_ctrl;
    int fd;
    
    info("AV_TestInitConnection");

    /* Initialize datpath client  */
    BTL_IFC_ClientInit();

    /* register with datapath server */   
    //result = BTL_IFC_RegisterSubSystem(&av_ctrl, SUB_AV, NULL, NULL);

    //if (result!=BTL_IF_SUCCESS)
    //{
    //   info("Subsystem registration failed (%d)", result);
    //   exit(0);
    //}

    //while (1)
    {
        BTL_IFC_ConnectDatapath(&fd, SUB_AV, 0);

        write(fd, "hello1", 7);
        write(fd, "hello2", 7);
        write(fd, "hello3", 7);    
        write(fd, "hello4", 7);
        write(fd, "hello5", 7);

        sleep(3);
        BTL_IFC_DisconnectDatapath(fd);
        sleep(3);
    }
    
    debug("Test done");    
    
}



int main(int argc, char** argv)
{
    char *cmd, *arg;
 
    if (argc<2)
    {
        info("No option provided <aginit | scoinit | wait | jni>");
        exit(0);
    }
    
    cmd = argv[1];
    arg = argv[2];

    if (!strcmp(cmd, "aginit"))
    {
        AG_TestInitConnection();
        while(1) sleep(1);
    }
    else if (!strcmp(cmd, "scoinit"))
    {
        SCO_TestInitConnection();
        while(1) sleep(1);       
    }
    else if (!strcmp(cmd, "allinit"))
    {
        AG_TestInitConnection();
        SCO_TestInitConnection();
        while(1) sleep(1);        
    }
    else if (!strcmp(cmd, "wait"))
    {
        AG_TestWaitConnection();
        while(1) sleep(1);         
    }
    else if (!strcmp(cmd, "xinit"))
    {
        AG_TestInitConnection();
        SCO_TestInitConnection();   
        AG_TestInitConnection();
        SCO_TestInitConnection();   
        AG_TestInitConnection();
        SCO_TestInitConnection();   
        AG_TestInitConnection();
        SCO_TestInitConnection();           
        sleep(2);
    }
    else if (!strcmp(cmd, "datainit"))
    {
        AV_TestInitConnection();
    }
    else if (!strcmp(cmd, "jni"))
    {    
        void jni_parsecmd(char *cmd);
        jni_parsecmd(cmd);    
    }
    return 0;
}


#endif
