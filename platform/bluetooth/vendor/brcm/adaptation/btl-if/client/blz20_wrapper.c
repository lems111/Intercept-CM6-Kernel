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
#include <sys/select.h>
#include <poll.h>
#include <string.h>
#include <limits.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sco.h>

/* this tag needs to be defined before btl_ifc.h include */
#define LOG_TAG "BLZ20_WRAPPER"

#include "btl_ifc.h"
#include "btl_ifc_wrapper.h"

/*******************************************************************************
**
**  Typedefs and structs
**          
**
*******************************************************************************/

struct asocket {
    int fd;           /* primary socket fd */
    int abort_fd[2];  /* pipe used to abort */
};

/* blz main ctrl block */
typedef struct {
    BOOLEAN init_complete;
    list_head sk_list;        
    tCTRL_HANDLE btlif_ctrl;

    /* stats */
    int stats_btsk_cnt;
    
} t_btsk_ctrl;

/* bts channel ctrl block */
typedef struct {
    list_head node;
    t_wsock *p_ws;
    UINT32 flags;

    /* btlif async operations */
    pthread_mutex_t btlif_cond_mutex;;
    pthread_cond_t  btlif_cond;

    tBTL_PARAMS btlif_async_params;

    tBTLIF_BTS_CTRL_MSG_IDS rx_ev;
    int btlif_wait_flags;
  
    /* common */
    struct asocket *asock;
    UINT32 fcntl_flags;      
    int prot;
    int lm; /* auth/encrypt */
    int sndbuf; /* send buffer size */

    /* rfcomm */
    struct sockaddr_rc rc_addr;
    socklen_t rc_addrlen;
    UINT32 bta_hdl;
    UINT32 rfc_scn;

    /* l2cap */
    UINT32 l2c_hdl;

    /* sco */
    UINT32 sco_hdl;
    
    UINT32 magic; /* for sanity check */
} t_btsk;

/* search keys */
typedef enum {
    KEY_RC_CHAN,
    KEY_BTA_HDL,        
    KEY_LISTEN_FD,
    KEY_DATA_FD,
    KEY_ANY_SOCK,
    KEY_ALL
} t_search_key;

/*******************************************************************************
**
**  Macros and defintitions
**          
**
*******************************************************************************/

/* enables extra checking of connection lists etc */
#define ENABLE_EXTRA_DEBUG

/* common flags */
#define BTSK_FLAG_SET(btsk, flag)   (btsk->flags |= flag)
#define BTSK_FLAG_CLR(btsk, flag)   (btsk->flags &= ~flag)
#define BTSK_FLAG_ISSET(btsk, flag) (btsk->flags & flag)

#define BTSK_FLAG_ASYNC_MSG_PND       (1<<0)
#define BTSK_FLAG_BIND_COMPLETE       (1<<1)
#define BTSK_FLAG_REMOTE_DISC_PND     (1<<2)

/* wait event macros */
#define BTS_MAX_WAIT_EVENT_BITS (BTLIF_BTS_CMD_END-BTLIF_BTS_CMD_BASE-1)
#define ADD_WAIT_EVENT(btsk, event) (btsk->btlif_wait_flags |= bts_event_to_flag(event))
#define CLR_WAIT_EVENT(btsk, event) (btsk->btlif_wait_flags &= ~(bts_event_to_flag(event)))
#define CLR_ALL_WAIT_EVENTS(btsk) (btsk->btlif_wait_flags = 0)
#define BTS_WAITFLAG_ISSET(btsk, event) (btsk->btlif_wait_flags & (bts_event_to_flag(event)))
#define BTS_SET_WAITFLAGS(btsk, flags) (btsk->btlif_wait_flags = flags)
#define BTS_ADD_WAITFLAGS(btsk, flags) (btsk->btlif_wait_flags |= flags)

#define DUMP_BSOCK(msg, btsk) if (btsk && btsk->p_ws) debug("%s fd (%d:%d), bta %d, rc %d, wflags 0x%x", msg, btsk->p_ws->listen_fd, btsk->p_ws->wsock, btsk->bta_hdl, (int)btsk->rc_addr.rc_channel, btsk->btlif_wait_flags);

/* misc definitions */
#define INVALID_VAL (-1)
#define BLZ_RFC_SCN_INVALID (-1)
#define BLZ_HANDLE_INVALID (-1)
#define BTSK_MAGIC 0xb0555e20

/*******************************************************************************
**
**  Local functions & variables
**          
**
*******************************************************************************/

static t_btsk_ctrl btsk_ctrl;
pthread_mutex_t btlif_ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;

void btlif_signal_event(t_btsk *btsk, int event_id, tBTL_PARAMS *params);
void btlif_ctrl_callback(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
t_btsk* btsk_find_by_key(t_search_key key, int val);

/*******************************************************************************
**
**  errno
**          
**
*******************************************************************************/

/* parse errno to provide meaningful error code up in JNI */
/* fixme --  support standard errno + btlif */
/* check /asm-generic/errno.h for available error codes */
/* fixme -- define btl if specific errno values that can be parsed from jni code */

#define ERRNO_SETME (0)
#define ERRNO_SUCCESS (0)

#define BTS_SET_ERRNO(err) {errno = err; if (err) debug("set errno %d (%s) l.%d ", err, blz20_strerror(err), __LINE__);}

/******************************************************************************
**
**  Helper functions
**          
**
*******************************************************************************/

char *bd2str(UINT8* addr)
{
    static char msg[100];
    sprintf(msg, "%02x%02x%02x%02x%02x%02x",
                       addr[0],addr[1],addr[2],
                       addr[3],addr[4],addr[5]);
    return msg;
}

static char *protocol_name(int type)
{
    if (type == BTPROTO_RFCOMM)
        return "BTPROTO_RFCOMM";
    else if (type == BTPROTO_L2CAP)
        return "BTPROTO_L2CAP";
    else if (type == BTPROTO_SCO)
        return "BTPROTO_SCO";
    else 
        return "UNKNOWN";
}


char *pollev2str(UINT32 ev)
{
    static char buf[128];
    int pos=0;

    if (ev&POLLIN) pos+=sprintf(buf+pos, "POLLIN ");     /* Data other than high-priority data may be read without blocking */
    if (ev&POLLPRI) pos+=sprintf(buf+pos, "POLLPRI ");   /* High-priority data may be read without blocking */
    if (ev&POLLOUT) pos+=sprintf(buf+pos, "POLLOUT ");   /* Normal data may be written without blocking */   
    if (ev&POLLERR) pos+=sprintf(buf+pos, "POLLERR ");   /* An error has occurred on the device or stream */
    if (ev&POLLHUP) pos+=sprintf(buf+pos, "POLLHUP ");   /* The device has been disconnected */   
    if (ev&POLLNVAL) pos+=sprintf(buf+pos, "POLLNVAL "); /* The specified fd value is invalid */
    //if (ev&POLLRDBAND) pos+=sprintf(buf+pos, "POLLRDBAND|");
    //if (ev&POLLWRBAND) pos+=sprintf(buf+pos, "POLLWRBAND|");    
    //if (ev&POLLRDNORM) pos+=sprintf(buf+pos, "POLLRDNORM|");
    //if (ev&POLLWRNORM) pos+=sprintf(buf+pos, "POLLWRNORM|");
    //if (ev&POLLMSG) pos+=sprintf(buf+pos, "POLLMSG|");
    //if (ev&POLLREMOVE) pos+=sprintf(buf+pos, "POLLREMOVE|");
    //if (ev&POLLRDHUP) pos+=sprintf(buf+pos, "POLLRDHUP|");
    return buf;
}

/*******************************************************************************
**
**
**  INIT   
**
**
*******************************************************************************/

static t_btsk* btsk_alloc_add(void);
static UINT32 bts_ev2flag(tBTLIF_BTS_CTRL_MSG_IDS ev1, tBTLIF_BTS_CTRL_MSG_IDS ev2, tBTLIF_BTS_CTRL_MSG_IDS ev3);
char *bts_dump_waitevents(t_btsk *btsk);
void btsk_dump_list(char *msg, t_search_key key, int val);


/* called once per application process */
int blz20_init(void)
{    
    if (btsk_ctrl.init_complete) return 0;

    debug("initializing...");

    /* initialize async list */
    INIT_LIST_HEAD(&btsk_ctrl.sk_list);  

    /* make sure client is initialized */
    BTL_IFC_ClientInit();

    btsk_ctrl.init_complete = TRUE;

    debug("success");
    return 0;
}


/*******************************************************************************
**
**  blz20_strerror
**          
**
*******************************************************************************/

char *blz20_strerror(int num)
{
    static char buf[256];

    /* fixme -- add custom bts error strings */

    if (num == 0)
        return "success";

    (void)strerror_r(num, buf, sizeof(buf));
    return (buf);
}



/*******************************************************************************
**
** Function         btsk_alloc_add
**
** Description    
**
** Returns           
*******************************************************************************/

static t_btsk* btsk_alloc_add(void)
{
    t_btsk *p;

    p = malloc(sizeof(t_btsk));

    if (!p)
    {
        BTS_SET_ERRNO(ERRNO_SETME);
        error("no mem");
        return NULL;
    }

    /* initialize members */
    
    INIT_LIST_HEAD(&p->node);

    p->p_ws = NULL;
    p->flags = 0;

    pthread_cond_init(&p->btlif_cond, NULL);
    pthread_mutex_init(&p->btlif_cond_mutex, NULL);    

    memset(&p->btlif_async_params, 0, sizeof(tBTL_PARAMS));
    p->rx_ev = 0;
    p->btlif_wait_flags = 0;
    p->asock = NULL;
    p->fcntl_flags = 0;

    p->prot = INVALID_VAL;
    p->lm = INVALID_VAL;
    p->sndbuf = INVALID_VAL;
    p->btlif_wait_flags = 0;
    p->lm = 0;    

    memset(&p->rc_addr, 0, sizeof(struct sockaddr_rc));
    p->rfc_scn = BLZ_RFC_SCN_INVALID;
    p->bta_hdl = INVALID_VAL;

    p->l2c_hdl = BLZ_HANDLE_INVALID;

    p->sco_hdl = BLZ_HANDLE_INVALID;
    p->magic = BTSK_MAGIC;

    /* add to list */
    list_add(&p->node, &btsk_ctrl.sk_list);

    btsk_ctrl.stats_btsk_cnt++;

    debug("success");

#ifdef ENABLE_EXTRA_DEBUG
    btsk_dump_list("", KEY_ALL, 0);
#endif

    return p;
}


/*******************************************************************************
**
** Function         btsk_free
**
** Description    
**
** Returns           
*******************************************************************************/

static int btsk_free(t_btsk *p_skt)
{
    if (!p_skt)
    {
       BTS_SET_ERRNO(ERRNO_SETME);
       return -1;
    }

    ASSERTC(p_skt->magic == BTSK_MAGIC, "magic failed", p_skt->magic);
    
    /* remove from list */
    list_del(&p_skt->node);

    /* free memory */
    free(p_skt);

    p_skt = NULL;

    btsk_ctrl.stats_btsk_cnt--;

#ifdef ENABLE_EXTRA_DEBUG
    btsk_dump_list("", KEY_ALL, 0);
#endif

    debug("success");

    return 0;
}


/*******************************************************************************
**
**
**  Socket list helper functions
**
**
*******************************************************************************/

#ifdef ENABLE_EXTRA_DEBUG

void btsk_dump_list(char *msg, t_search_key key, int val)
{
    struct list_head *pos;
    t_btsk *p;
    int dump = 0;
    
    list_for_each(pos, &btsk_ctrl.sk_list) 
    {
        p = list_get_obj(pos, t_btsk, node);

        ASSERTC(p, "corrupt list", 0);

        switch(key)
        {
           case KEY_RC_CHAN:
               if ((int)p->rc_addr.rc_channel == val) dump = 1;
               break;               
               
           case KEY_LISTEN_FD:
               if (p->p_ws->listen_fd == val) dump = 1;
               break;
                   
           case KEY_DATA_FD:
               if (p->p_ws->wsock == val) dump = 1;
               break;

           case KEY_ANY_SOCK:
               if  ( (p->p_ws->wsock == val) || (p->p_ws->listen_fd == val) ) dump = 1;
               break;

           case KEY_BTA_HDL:
               if  (p->bta_hdl == val) dump = 1;
               break;

           case KEY_ALL:
               dump = 1;
               break;
               
           default:
               ASSERTC(0, "invalid key", key);
               break;

        }       

        if (dump == 1)
            DUMP_BSOCK(msg, p); 
    }
}

void btsk_check_duplicates(t_search_key key, int val)
{
    struct list_head *pos;
    t_btsk *p;
    int counter = 0;

    //info("key %d, val %d, list %x", key, val, &btsk_ctrl.sk_list);

    list_for_each(pos, &btsk_ctrl.sk_list) 
    {
       p = list_get_obj(pos, t_btsk, node);

       ASSERTC(p, "corrupt list", 0); 
      
       switch(key)
       {
           case KEY_RC_CHAN:
               if ((int)p->rc_addr.rc_channel == val) counter++;
               break;               
               
           case KEY_LISTEN_FD:
               if (p->p_ws->listen_fd == val) counter++;
               break;
                   
           case KEY_DATA_FD:
               if (p->p_ws->wsock == val) counter++;
               break;

           case KEY_ANY_SOCK:
               if  ( (p->p_ws->wsock == val) || (p->p_ws->listen_fd == val) ) counter++;
               break;

           case KEY_BTA_HDL:
               if  (p->bta_hdl == val) counter++;
               break;
               
           default:
               ASSERTC(0, "invalid key", key);
               break;

       }
    }

    if (counter > 1)
    {
            error("WARNING : duplicate entries found for key %d, val %d", key, val);

            // dump full list 
            
            list_for_each(pos, &btsk_ctrl.sk_list) 
            {
               p = list_get_obj(pos, t_btsk, node);
               DUMP_BSOCK("", p);
            }
            ASSERTC(0, "duplicates found", counter);
    }

    //debug("not found");
    return NULL;

}

#endif

/* assume rc_chan is unique for both listeners and outgoing connections */
t_btsk* btsk_find_by_key(t_search_key key, int val)
{
    struct list_head *pos;
    t_btsk *p;
    
#ifdef ENABLE_EXTRA_DEBUG
    btsk_check_duplicates(key, val);
#endif

    //info("key %d, val %d, list %x", key, val, &btsk_ctrl.sk_list);

    list_for_each(pos, &btsk_ctrl.sk_list) 
    {
        p = list_get_obj(pos, t_btsk, node);

        ASSERTC(p, "corrupt list", 0); 
       
        switch(key)
        {
            case KEY_RC_CHAN:
                if ((int)p->rc_addr.rc_channel == val) return p;
                break;   
                
            case KEY_LISTEN_FD:
                if (p->p_ws->listen_fd == val) return p;
                break;
                    
            case KEY_DATA_FD:
                if (p->p_ws->wsock == val) return p;
                break;

            case KEY_ANY_SOCK:
                if  ( (p->p_ws->wsock == val) || (p->p_ws->listen_fd == val) ) return p;
                break;

            case KEY_BTA_HDL:
                if  (p->bta_hdl == val) return p;
                break;
                
            default:
                ASSERTC(0, "invalid key", key);
                break;

        }        
    }

    //debug("not found");
    return NULL;
}


/*******************************************************************************
**
**  wait flag management
**          
**
*******************************************************************************/

/* convert an event to a bitmask */
UINT32 bts_event_to_flag(tBTLIF_BTS_CTRL_MSG_IDS event)
{   
    UINT32 flag = 0;
    UINT32 bitno = (event-BTLIF_BTS_CMD_BASE-1);
    
    /* if we reach max we will need to expand to UINT32 array */
    ASSERTC((bitno < 32), "out of bounds", bitno);

    flag |= (1<<bitno);

    verbose("flag %x, bitno %d", flag, bitno);
    
    return flag;
}

static UINT32 bts_ev2flag(tBTLIF_BTS_CTRL_MSG_IDS ev1, tBTLIF_BTS_CTRL_MSG_IDS ev2, tBTLIF_BTS_CTRL_MSG_IDS ev3)
{
    UINT32 flag = 0;

    if (ev1) flag |= bts_event_to_flag(ev1);
    if (ev2) flag |= bts_event_to_flag(ev2);
    if (ev3) flag |= bts_event_to_flag(ev3);    

    return flag;
}


char *bts_dump_waitevents(t_btsk *btsk)
{
    static char buf[128];
    int bitno = 0;
    int pos = 0;
    UINT32 flags = btsk->btlif_wait_flags;
    UINT32 eflag;
  
    pos += sprintf(buf+pos, "|");

    /* traverse through all set events */
    while (flags && (bitno < BTS_MAX_WAIT_EVENT_BITS))
    {
        eflag = (1<<bitno);

        if (eflag & flags)
        {
            pos += sprintf(buf+pos, "%s|", dump_msg_id(bitno+BTLIF_BTS_CMD_BASE+1));
            flags &= ~eflag; // clear this bit to make sure we end this loop if no more flags are set
        }
        bitno++;
    }

    ASSERTC(pos<128, "overflow", pos);

    return buf;
}


/*******************************************************************************
**
** Function         btlif_set_wait_event
**
** Description     
**
** Returns           
*******************************************************************************/

/* NOTE : there can only be one outstanding btlif request per socket */

void btlif_set_wait_event_flags(t_btsk *btsk, UINT32 wait_flags)
{
    /* store any events we are waiting for */
    BTS_SET_WAITFLAGS(btsk, wait_flags);
    
    /* clear return variable */
    memset(&btsk->btlif_async_params, 0, sizeof(tBTL_PARAMS));     
}

/* sets single wait event */
void btlif_set_wait_event(t_btsk *btsk, tBTLIF_BTS_CTRL_MSG_IDS event)
{
    /* store any events we are waiting for */
    ADD_WAIT_EVENT(btsk, event);
    
    /* clear return variable */
    memset(&btsk->btlif_async_params, 0, sizeof(tBTL_PARAMS));     
}

/* sets up to 3 wait events */
void btlif_set_wait_events(t_btsk *btsk, tBTLIF_BTS_CTRL_MSG_IDS ev1, tBTLIF_BTS_CTRL_MSG_IDS ev2, tBTLIF_BTS_CTRL_MSG_IDS ev3)
{
    UINT32 flags = bts_ev2flag(ev1, ev2, ev3);

    btlif_set_wait_event_flags(btsk, flags); 
}


/*******************************************************************************
**
** Function         btlif_wait_response
**
** Description     wait for configured wait events
**
** Returns           
*******************************************************************************/

tBTL_PARAMS* btlif_wait_response(t_btsk *btsk)
{    
    debug("id(s) %s", bts_dump_waitevents(btsk));

    pthread_mutex_lock(&btsk->btlif_cond_mutex);
    pthread_cond_wait(&btsk->btlif_cond, &btsk->btlif_cond_mutex);
    pthread_mutex_unlock(&btsk->btlif_cond_mutex);

    DUMP_BSOCK("unblocked", btsk);
 
    return &btsk->btlif_async_params;
}

/*******************************************************************************
**
** Function         bts_abort_all
**
** Description     release all bt sockets
**
** Returns           
*******************************************************************************/

void bts_abort_all(void)
{
    struct list_head *pos;
    t_btsk *p;

    list_for_each(pos, &btsk_ctrl.sk_list) 
    {
        p = list_get_obj(pos, t_btsk, node);
        ASSERTC(p, "corrupt list", 0);
        btlif_signal_event(p, BTLIF_BTS_EVT_ABORT, NULL);        
    }
}

/*******************************************************************************
**
** Function         btlif_signal_event
**
** Description    
**
** Returns           
*******************************************************************************/

void btlif_signal_event(t_btsk *btsk, int event_id, tBTL_PARAMS *params)
{    
    pthread_mutex_lock(&btsk->btlif_cond_mutex);

    DUMP_BSOCK("", btsk);

    /* if event matches the current wait event or is an abort, unblock current wait action */
    if (BTS_WAITFLAG_ISSET(btsk, event_id) || (event_id == BTLIF_BTS_EVT_ABORT))
    {
        debug("event %s matched", dump_msg_id(event_id));

        /* store response params if any */
        if (params)
            memcpy(&btsk->btlif_async_params, params, sizeof(tBTL_PARAMS));
        else
            memset(&btsk->btlif_async_params, 0, sizeof(tBTL_PARAMS));            

        /* store received event */
        btsk->rx_ev = event_id;

        /* clear this wait flag */
        CLR_WAIT_EVENT(btsk, event_id);
    
        /* release waiting thread */
        pthread_cond_signal(&btsk->btlif_cond);
    }
    else
    {
        debug("### event %s not matched ###", dump_msg_id(event_id));
        /* fixme -- deal with this */
    }
    
    pthread_mutex_unlock(&btsk->btlif_cond_mutex);
}  

/*******************************************************************************
**
** Function         btlif_send_msg_wait_response
**
** Description    This function will send a request and waif for any set events
**
** Returns           
*******************************************************************************/

tBTL_PARAMS* btlif_send_msg_wait_response(t_btsk *btsk, tBTLIF_CTRL_MSG_ID msg_id, tBTL_PARAMS *params, int param_len, UINT32 wait_flags)
{
    tBTL_IF_Result result;
 
    /* make sure we are not going to receive the btlif response before we */
    pthread_mutex_lock(&btlif_ctrl_mutex);

    /* store event we are waiting for */
    BTS_SET_WAITFLAGS(btsk, wait_flags);

    debug("send %s, wait for %s", dump_msg_id(msg_id), bts_dump_waitevents(btsk));

    /* clear return variable */
    memset(&btsk->btlif_async_params, 0, sizeof(tBTL_PARAMS));        

    /* send message */ 
    result = BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, msg_id, params, param_len);

    if (result != BTL_IF_SUCCESS)
    {        
        error("send msg failed due to %d", result);
        pthread_mutex_unlock(&btlif_ctrl_mutex);
        CLR_ALL_WAIT_EVENTS(btsk);    
        BTS_SET_ERRNO(ERRNO_SETME);
        return NULL;
    }

    /* lock wait mutex */
    pthread_mutex_lock(&btsk->btlif_cond_mutex);

    /* unlock btl if mutex as we are now inside wait mutex */
    pthread_mutex_unlock(&btlif_ctrl_mutex);

    /* wait for response */    
    pthread_cond_wait(&btsk->btlif_cond, &btsk->btlif_cond_mutex);

    pthread_mutex_unlock(&btsk->btlif_cond_mutex);

    DUMP_BSOCK("unblocked", btsk);

    return &btsk->btlif_async_params;
}


/*******************************************************************************
**
** Function         btl if callback handlers
**
** Description    
**
** Returns           
*******************************************************************************/

static void btlif_process_listen_rsp(tBTLIF_BTS_RFC_LISTEN_RSP_PARAM *params)
{
    t_btsk *btsk;

    /* search for bt socket bound to this rc channel */
    btsk = btsk_find_by_key(KEY_ANY_SOCK, params->listen_fd);

    if (!btsk)
    {
        error("no bt sock found");
        return;
    }

    /* store bta hdl */
    btsk->bta_hdl = params->bta_handle;

    info("listen response for bta hdl %d", (int)btsk->bta_hdl);

    /* notify blocking thread */
    btlif_signal_event(btsk, BTLIF_BTS_RFC_LISTEN_RSP, params);
}

static void btlif_process_con_ind(tBTLIF_BTS_RFC_CON_IND_PARAM *params)
{
    t_btsk *btsk;
    int result;
    tBTLIF_BTS_RFC_CON_IND_ACK_PARAM ack;
    
    btsk = btsk_find_by_key(KEY_ANY_SOCK, params->listen_fd);
    
    if (!btsk)
    {
        error("no bt sock found, bta handle %d", params->handle);
        return;
    }
    
    /* store params */
    memcpy(btsk->rc_addr.rc_bdaddr.b, params->rem_bda, 6);
    
    ASSERTC(btsk->bta_hdl == params->handle, "invalid bta handle", params->handle);
    
    info("incoming rfcomm connection from %s, signal event", bd2str(btsk->rc_addr.rc_bdaddr.b));
    
    ack.handle = btsk->bta_hdl;

    /* wake up any blocking thread */
    btlif_signal_event(btsk, BTLIF_BTS_RFC_CON_IND, params);  
    
    /* notify server that we have processed the message and are ready for incoming connection */
    BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_IND_ACK, 
                        (tBTL_PARAMS *)&ack, sizeof(tBTLIF_BTS_RFC_CON_IND_ACK_PARAM));
    
}

static void btlif_process_con_rsp(tBTLIF_BTS_RFC_CON_RSP_PARAM *params)
{
    t_btsk *btsk;

    btsk = btsk_find_by_key(KEY_ANY_SOCK, params->sock);

    if (!btsk)
    {
        error("no bt sock found, scn %d", params->scn_remote);
        return;
    }

    /* store assigned bta handle */
    btsk->bta_hdl = params->handle;

    /* check if we are waiting for this event */
    if (BTSK_FLAG_ISSET(btsk, BTSK_FLAG_ASYNC_MSG_PND) && BTS_WAITFLAG_ISSET(btsk, BTLIF_BTS_RFC_CON_RSP) )
    {
        int s;
        
        info("async connection rsp received, status %d", params->status);
        
        BTSK_FLAG_CLR(btsk, BTSK_FLAG_ASYNC_MSG_PND);

        if (params->status != BTL_IF_SUCCESS)
        {
            info("connection failed due to %s", params->status);
            return;
        }

        /* establish datapath */
        s = wrp_sock_connect(btsk->p_ws, btsk->p_ws->wsock, btl_ifc_get_srvaddr(),  wrp_getport(SUB_BTS, btsk->rc_addr.rc_channel));

        ASSERTC(s>0, "failed", s);                
    }
    else
    {
        /* wake up any blocking thread */
        btlif_signal_event(btsk, BTLIF_BTS_RFC_CON_RSP, params);              
    }
}

static void btlif_process_disc_ind(tBTLIF_BTS_RFC_DISC_IND_PARAM *params)
{
    t_btsk *btsk;
    tBTLIF_BTS_RFC_DISC_IND_ACK_PARAM ack;
                  
    btsk = btsk_find_by_key(KEY_ANY_SOCK, params->sock);

    if (!btsk)
    {
        /* already closed */
        info("no bt sock found, scn %d (already closed)", params->scn_remote);

        ack.scn_remote = 0; /* don't care */
        ack.sock = params->sock;
        ack.status = BTL_IF_NOT_CONNECTED;
        
        BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_DISC_IND_ACK, (tBTL_PARAMS *)&ack, sizeof(tBTLIF_BTS_RFC_DISC_IND_ACK_PARAM));
        return;
    }

    info("disconnect ind status %d, rc chan %d",  params->status, params->scn_remote);

    /* set flag to have poll/read notify the disconnection */
    BTSK_FLAG_SET(btsk, BTSK_FLAG_REMOTE_DISC_PND);

    /* make sure we signal any blocking waits */
    btlif_signal_event(btsk, BTLIF_BTS_RFC_DISC_IND, params);    

    /* acknowledge disconnection */
    ack.scn_remote = btsk->rc_addr.rc_channel;
    ack.sock = params->sock;
    ack.status = BTL_IF_SUCCESS;

    BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_DISC_IND_ACK, (tBTL_PARAMS *)&ack, sizeof(tBTLIF_BTS_RFC_DISC_IND_ACK_PARAM));
}

/*******************************************************************************
**
** Function         btlif_ctrl_callback
**
** Description    
**
** Returns           
*******************************************************************************/

void btlif_ctrl_callback(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{       
    pthread_mutex_lock(&btlif_ctrl_mutex);

    info("btlif_ctrl_callback : msg id %s", dump_msg_id(id));
    
    switch(id)
    {
        case BTLIF_BTS_RFC_LISTEN_RSP:
            btlif_process_listen_rsp(&params->bts_rfc_listen_rsp_param);
            break;
        case BTLIF_BTS_RFC_CON_IND:   
            btlif_process_con_ind(&params->bts_rfc_con_ind_param);
            break;
        case BTLIF_BTS_RFC_CON_RSP:
            btlif_process_con_rsp(&params->bts_rfc_con_rsp_param);
            break;
        case BTLIF_BTS_RFC_DISC_IND:
            btlif_process_disc_ind(&params->bts_rfc_disc_ind_param);
            break;
        case BTLIF_BTS_RFC_CLOSE_CFM:
            /* needed ?*/
            break;
        case BTLIF_DATA_CHAN_IND :
            info("rfcomm connection up on fd %d (subport %d)", params->chan_ind.handle, params->chan_ind.subport);
            break;
        case BTLIF_SUBSYSTEM_DETACHED:
            info("BTS subsystem disconnected");
            bts_abort_all();
            btsk_ctrl.init_complete = 0;
            break;

        default:
            error("%s unhandled msg id %d(0x%x)", dump_msg_id(id), id, id);
            break;
    }
    
    pthread_mutex_unlock(&btlif_ctrl_mutex);   
}



/*******************************************************************************
**
**  RFCOMM
**          
**
*******************************************************************************/

static int __bind_prot_rfcomm(t_btsk *btsk, int s, struct sockaddr_rc *addr, socklen_t addrlen)
{
    int res;
    t_btsk *p_tmp;
    
    debug("rc_chan %d", addr->rc_channel);

    if (addr->rc_channel >= BTS_CHAN_MAX)
    {
        error("rc channel to large");
        BTS_SET_ERRNO(ERRNO_SETME);
        return -1;
    }

    /* make sure this channel is not yet bound on another socket */
    if(btsk_find_by_key(KEY_RC_CHAN, addr->rc_channel))
    {
        error("trying to bind channel already in use");
        BTS_SET_ERRNO(EADDRINUSE);
        return -1;   
    }

    /* store address & len */
    memcpy((char*)&btsk->rc_addr, (char*)addr, sizeof(struct sockaddr_rc));
    btsk->rc_addrlen = addrlen;
    
    /* bind address  */
    res = wrp_sock_bind(btsk->p_ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_BTS, btsk->rc_addr.rc_channel)+BTS_CLIENT_BIND_CHAN_OFFSET);            

    if (res < 0)
        return res;

    BTSK_FLAG_SET(btsk, BTSK_FLAG_BIND_COMPLETE);

    return res;    
}

static int __listen_prot_rfcomm(t_btsk *btsk, int s, int backlog)
{
    tBTL_PARAMS params;
    tBTLIF_BTS_RFC_LISTEN_RSP_PARAM *rsp;
    int result = 0;

    params.bts_rfc_listen_req_param.rc_chan = btsk->rc_addr.rc_channel;

    /* translate lm settings */
    if (btsk->lm)
    {
       params.bts_rfc_listen_req_param.auth = btsk->lm & RFCOMM_LM_AUTH;
       params.bts_rfc_listen_req_param.encrypt = btsk->lm & RFCOMM_LM_ENCRYPT;
       params.bts_rfc_listen_req_param.master = btsk->lm & RFCOMM_LM_MASTER;
    }

    /* send this down as a unique reference */
    params.bts_rfc_listen_req_param.listen_fd = s;

    rsp = (tBTLIF_BTS_RFC_LISTEN_RSP_PARAM*) btlif_send_msg_wait_response(btsk, BTLIF_BTS_RFC_LISTEN_REQ, &params, 
                    sizeof(tBTLIF_BTS_RFC_LISTEN_REQ_PARAM), bts_ev2flag(BTLIF_BTS_RFC_LISTEN_RSP, 0, 0));

    /* make sure we check for abort event */
    if (btsk->rx_ev == BTLIF_BTS_EVT_ABORT)
    {                
        /* listen aborted */
        BTS_SET_ERRNO(ECONNABORTED);
        return -1;
    }

    if (rsp->status== BTL_IF_SUCCESS)
    {
        /* success */
        info("success, scn %d", rsp->scn);
    
        /* make sure this was for the right rc_chan */
        if (rsp->rc_chan != btsk->rc_addr.rc_channel)
        {
            BTS_SET_ERRNO(ERRNO_SETME);        
            error("warning : invalid rc channel in response");
        }

        /* store allocated scn */
        btsk->rfc_scn = rsp->scn;
    }
    else
    {
        /* failure */        
        error("failed with reason %d", rsp->status);
        BTS_SET_ERRNO(EBADF); /* fixme -- use other errno ? */
        return -1;
    } 

    result = wrp_sock_listen_bl(btsk->p_ws, s, backlog);     

    if (result == 0)
        info("success");
    else
        error("error %s", strerror(errno));

    return result;
}
    

static int __accept_prot_rfcomm(int s, t_btsk *btsk, struct sockaddr_rc *addr, socklen_t *addrlen)
{
    tBTLIF_BTS_RFC_NOTIFY_LST_DSOCK_PARAM ind;
    tBTL_IF_Result result;
    int s_new;
    
    DUMP_BSOCK("", btsk);
    
    /* copy configured params to acceptor */
    memcpy((void*)addr, (void*)&btsk->rc_addr, sizeof(struct sockaddr_rc)); 
    *addrlen = btsk->rc_addrlen;

    info("rc_chan %d, bd %s", addr->rc_channel, bd2str((UINT8*)&addr->rc_bdaddr));

    s_new = wrp_sock_accept(btsk->p_ws, s);  

    if (s_new < 0)
        return s_new;

    ind.data_sock_fd = s_new;
    ind.listen_fd = btsk->p_ws->listen_fd;

    /* notify new data socket to btlif server to keep as reference */
    result = BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_LST_NOTIFY_DSOCK, 
                              (tBTL_PARAMS*)&ind, sizeof(tBTLIF_BTS_RFC_NOTIFY_LST_DSOCK_PARAM));   

    ASSERTC(result == BTL_IF_SUCCESS, "data sock ind failed", result);

    return s_new;    
}


tBTL_IF_Result __connect_prot_rfcomm(int s, t_btsk *btsk, const struct sockaddr_rc *addr, socklen_t addrlen)
{
    tBTLIF_BTS_RFC_CON_REQ_PARAM req;
    tBTL_IF_Result result;

    info("connecting to... %s, rc chan %d", bd2str((UINT8*)&addr->rc_bdaddr), addr->rc_channel);

    req.auth = btsk->lm & RFCOMM_LM_AUTH;
    req.encrypt= btsk->lm & RFCOMM_LM_ENCRYPT;
    req.master = btsk->lm & RFCOMM_LM_MASTER;
    req.sock = s;
    
    memcpy(req.bd, addr->rc_bdaddr.b, sizeof(BD_ADDR));
    req.scn_remote = addr->rc_channel;

    /* copy configured params to acceptor */
    memcpy((void*)&btsk->rc_addr, (void*)addr, sizeof(struct sockaddr_rc)); 
    btsk->rc_addrlen = addrlen;
    btsk->rfc_scn = addr->rc_channel;

    /*  check blocking mode */    
    if (!(btsk->fcntl_flags & O_NONBLOCK))
    {
        tBTLIF_BTS_RFC_CON_RSP_PARAM *rsp;
        int result;
        
        info("blocking mode");
        
        /* set up expected wait events for this request */
        rsp = (tBTLIF_BTS_RFC_CON_RSP_PARAM *) btlif_send_msg_wait_response(btsk, BTLIF_BTS_RFC_CON_REQ, 
               &req, sizeof(tBTLIF_BTS_RFC_CON_REQ_PARAM), bts_ev2flag(BTLIF_BTS_RFC_CON_RSP, BTLIF_BTS_RFC_DISC_IND, 0));
                
        /* connect datapath */
        result = wrp_sock_connect(btsk->p_ws, btsk->p_ws->wsock, btl_ifc_get_srvaddr(), wrp_getport(SUB_BTS, addr->rc_channel));

        if (result < 0)
            return BTL_IF_GENERIC_ERR;
        
    }
    else
    {   
        info("non blocking mode");

        /*  make sure we don't process any response until we have set the wait event */
        pthread_mutex_lock(&btlif_ctrl_mutex);        
        
        /* send btl if connect request */
        result = BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CON_REQ, (tBTL_PARAMS*)&req, sizeof(tBTLIF_BTS_RFC_CON_REQ_PARAM));
        
        if (result != BTL_IF_SUCCESS)
        {        
            BTS_SET_ERRNO(ERRNO_SETME);
            return result;
        }

        /* mark this as async event to make sure poll will block */
        BTSK_FLAG_SET(btsk, BTSK_FLAG_ASYNC_MSG_PND);

        pthread_mutex_unlock(&btlif_ctrl_mutex); 

        /* simulate a real socket connection */
        errno = EINPROGRESS;

        /* wrp connection will be done from poll function once async message is received */        
    }

    return BTL_IF_SUCCESS;
}


int __close_prot_rfcomm(int s, t_btsk *btsk)
{
    int result;
    tBTL_PARAMS req;

    info("fd %d", s);

    
    /* check type of socket */
    if (s == btsk->p_ws->listen_fd)
    {
         /* make sure this socket is bound */
        if (!BTSK_FLAG_ISSET(btsk, BTSK_FLAG_BIND_COMPLETE))
        {
            info("bind not completed on this socket");
            return 0;
        }
        /* close listener socket */    

        req.bts_rfc_listen_cancel_param.handle = btsk->bta_hdl;
        req.bts_rfc_listen_cancel_param.listen_fd = s;
        result = BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_LISTEN_CANCEL, &req, sizeof(tBTLIF_BTS_RFC_LISTEN_CANCEL_PARAM));
    }
    else if (s == btsk->p_ws->wsock)
    {   
        /* close data socket */    

        req.bts_rfc_close_param.scn_remote = btsk->rc_addr.rc_channel; //btsk->rfc_scn;
        req.bts_rfc_close_param.sock = s;
        result = BTL_IFC_CtrlSend(btsk_ctrl.btlif_ctrl, SUB_BTS, BTLIF_BTS_RFC_CLOSE, &req, sizeof(tBTLIF_BTS_RFC_CLOSE_PARAM)); 
    }   

    return result;    
}


/*******************************************************************************
**
**  L2CAP
**          
**
*******************************************************************************/



/*******************************************************************************
**
**  SCO
**          
**
*******************************************************************************/



/*******************************************************************************
**
** Function         blz20_wrp_socket
**
** Description    
**
** Returns           
*******************************************************************************/

int blz20_wrp_socket(int socket_family, int socket_type, int protocol)
{
    t_btsk *p_skt;
    t_wsock *ws;
    int result;
    
    info("fam %d, type %d, prot %s", socket_family, socket_type, protocol_name(protocol));

    /* first some sanity checks */
    if ((socket_family != PF_BLUETOOTH) && (socket_type != BTPROTO_RFCOMM) && (socket_type != BTPROTO_L2CAP) && (socket_type != BTPROTO_SCO) )
    {
        BTS_SET_ERRNO(ERRNO_SETME);
        return -1;
    } 

    /* make sure we have setup the BTS ctrl channel */
    if (blz20_init() < 0)        
        return -1;

    /* Make sure ctrl channel for bts is setup. No data callback, use transparant socket for datapath */
    result = BTL_IFC_RegisterSubSystem(&btsk_ctrl.btlif_ctrl, SUB_BTS, NULL, btlif_ctrl_callback);

    if (result != BTL_IF_SUCCESS)
    {
        BTS_SET_ERRNO(ERRNO_SETME);
        return -1;
    }
    
    /* allocate wrapper socket */
    ws = wrp_wsock_create(SUB_BTS);

    if (!ws)
    {
        BTS_SET_ERRNO(ERRNO_SETME);
        return -1;
    }

    /* allocate bt sock */
    p_skt = btsk_alloc_add();

    if (!p_skt)
    {
        BTS_SET_ERRNO(ERRNO_SETME);        
        return -1;
    }

    p_skt->prot = protocol;

    /* link with wrapper socket */
    ws->priv = (t_btsk *)p_skt;
    p_skt->p_ws = ws;

    info("return %d", ws->wsock);

    return ws->wsock;    
}    


/*******************************************************************************
**
** Function         blz20_wrp_bind
**
** Description    
**
** Returns           
*******************************************************************************/

int blz20_wrp_bind(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    t_btsk *btsk;

    info("s %d", s);

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);
    
    if (!btsk) 
    {
        error("btsk not found");
        return -1;
    }
    
    DUMP_BSOCK("bind socket", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    switch(btsk->prot)
    {
        case BTPROTO_RFCOMM:
            return __bind_prot_rfcomm(btsk, s, (struct sockaddr_rc*)addr, addrlen);
        break;

        case BTPROTO_L2CAP:
            error("protocol %s not yet supported", protocol_name(btsk->prot));          
            break;

        case BTPROTO_SCO:
            error("protocol %s not yet supported", protocol_name(btsk->prot));          
            break;
    }

    info("failed");

    return -1;
}    


/*******************************************************************************
**
** Function         blz20_wrp_listen
**
** Description    
**
** Returns           
*******************************************************************************/

int blz20_wrp_listen(int s, int backlog)
{
    t_btsk *btsk;
    int result = -1;
    
    info("s %d, backlog %d", s, backlog);

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);
    
    if (!btsk) 
    {
        error("btsk not found");
        BTS_SET_ERRNO(ERRNO_SETME);        
        return -1;
    }
    
    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    switch(btsk->prot)
    {
        case BTPROTO_RFCOMM:
            result = __listen_prot_rfcomm(btsk, s, backlog);
            break;

        case BTPROTO_L2CAP:
            error("protocol %s not yet supported", protocol_name(btsk->prot));          
            break;

        case BTPROTO_SCO:
            error("protocol %s not yet supported", protocol_name(btsk->prot));          
            break;
    }
 
    return result;
}    


/*******************************************************************************
**
** Function         blz20_wrp_accept
**
** Description    
**
** Returns           
*******************************************************************************/

int blz20_wrp_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    t_btsk *btsk;
    int s_new = -1;

    info("s %d", s);

    btsk = btsk_find_by_key(KEY_LISTEN_FD, s);
    
    if (!btsk) 
    {
        error("btsk not found");
        return -1;
    }

    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->listen_fd == s, "socket not matching", s);

    switch(btsk->prot)
    {
        case BTPROTO_RFCOMM:
            s_new = __accept_prot_rfcomm(s, btsk, (struct sockaddr_rc *)addr, addrlen);
            break;

        case BTPROTO_L2CAP:
            error("protocol %s not yet supported", protocol_name(btsk->prot));
            break;

        case BTPROTO_SCO:
            error("protocol %s not yet supported", protocol_name(btsk->prot));            
            break;
    }

    if (s_new > 0)
       debug("success");

    return s_new;
}    


/*******************************************************************************
**
** Function         blz20_wrp_connect
**
** Description     
**
** Returns           
*******************************************************************************/

int blz20_wrp_connect(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    tBTL_IF_Result result = -1;
    t_btsk *btsk;

    info("s %d", s);

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);

    if (!btsk) 
    {
        error("btsk not found");
        BTS_SET_ERRNO(EBADF);
        return -1;
    }
    
    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    switch(btsk->prot)
    {
        case BTPROTO_RFCOMM:
            result = __connect_prot_rfcomm(s, btsk, (struct sockaddr_rc *)addr, addrlen);
            break;

        case BTPROTO_L2CAP:
            error("protocol %s not yet supported", protocol_name(btsk->prot));
            break;

        case BTPROTO_SCO:       
            error("protocol %s not yet supported", protocol_name(btsk->prot));
            break;
    }

    if (result != BTL_IF_SUCCESS)
    {
        /* errno already set in above protocol handlers  */
        return -1;
    }    

    debug("success");

    return btsk->p_ws->wsock;
}    

/*******************************************************************************
**
** Function         blz20_wrp_close
**
** Description     close blz socket wrapper
**
** Returns           
*******************************************************************************/

int blz20_wrp_close(int s)
{
    tBTL_IF_Result result = -1;
    t_btsk *btsk;

    info("s %d", s);
    
    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);

    if (!btsk) 
    {
        debug("std close (%d)", s);
        return close(s);
    }

    DUMP_BSOCK("", btsk);

    switch(btsk->prot)
    {
       case BTPROTO_RFCOMM:
           result = __close_prot_rfcomm(s, btsk);
           break;

       case BTPROTO_L2CAP:
           error("protocol %s not yet supported", protocol_name(btsk->prot));        
           break;

       case BTPROTO_SCO:
           error("protocol %s not yet supported", protocol_name(btsk->prot));
           break;
    }

    if (wrp_close_s_only(btsk->p_ws, s) == TRUE)
    {
        /* make sure free up this bt socket if both listener and datasocket are closed */
        btsk_free(btsk);
    }
    
    if (result != BTL_IF_SUCCESS)
    {
        /* errno already set in above protocol handlers  */
        return -1;
    }    

    return 0;
}


/*******************************************************************************
**
** Function         blz20_wrp_shutdown
**
** Description     shutdown blz socket wrapper
**
** Returns           
*******************************************************************************/

int blz20_wrp_shutdown(int s, int how)
{
    tBTL_IF_Result result = BTL_IF_GENERIC_ERR;
    t_btsk *btsk;

    info("s %d, how %d", s, how);
    
    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);

    if (!btsk) 
    {
        debug("std shutdown (%d)", s);
        return shutdown(s, how);
    }

    DUMP_BSOCK("", btsk);

    /* wake up any waiting threads */
    btlif_signal_event(btsk, BTLIF_BTS_EVT_ABORT, NULL);

    info("shutdown socket");
    
    shutdown(s, how);

    return 0;
}


/*******************************************************************************
**
** Function         blz20_wrp_setsockopt
**
** Description     Sets ctrl flags for blz socket calls and standard fd:s
**
** Returns           
*******************************************************************************/


int blz20_wrp_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{    
    t_btsk *btsk;

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);    
    
    if (!btsk) 
    {
        error("btsk not found");
        return -1;
    }

    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    if ((level == SOL_RFCOMM) && (optname == RFCOMM_LM))
    {       
        /* store security settings in btsk */    
        btsk->lm = *((int*)optval);        
        info("configure rfcomm lm mode 0x%x, (master:%d, auth %d, enc %d)", btsk->lm, (btsk->lm & RFCOMM_LM_MASTER)?1:0, 
             (btsk->lm & RFCOMM_LM_AUTH)?1:0, (btsk->lm & RFCOMM_LM_ENCRYPT)?1:0 );        
    }

    if ((level == SOL_SOCKET) && (optname == SO_SNDBUF))
    {
        /* store send buffer sz in btsk */  
        btsk->sndbuf = *((int*)optval);            
        info("configure rfcomm sndbuf len %d bytes", btsk->sndbuf); 
    }
    
    info("success");    
    
    return 0;
}    


/*******************************************************************************
**
** Function         blz20_wrp_getsockopt
**
** Description     Gets ctrl flags for blz socket calls and standard fd:s
**
** Returns           
*******************************************************************************/


int blz20_wrp_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    t_btsk *btsk;

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);
    
    if (!btsk) 
    {
        error("btsk not found");
        return -1;
    }

    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    if ((level == SOL_RFCOMM) && (optname == RFCOMM_LM))
    {       
        /* store in btsk */
        *((int*)optval) = btsk->lm;       
        info("lm %x", *((int*)optval));       
    }

    if ((level == SOL_SOCKET) && (optname == SO_SNDBUF))
    {
       *((int*)optval) = btsk->sndbuf;         
       info("sndbuf %d bytes", *((int*)optval)); 
    }

    if ((level == SOL_SOCKET) && (optname == SO_ERROR))
    {
       *((int*)optval) = errno;         
       info("fetch errno %d", *((int*)optval)); 
    }

    info("success");        
    return 0;
}


/*******************************************************************************
**
** Function         blz20_wrp_fcntl
**
** Description     
**
** Returns           
*******************************************************************************/


int blz20_wrp_fcntl(int s, int cmd, long arg)
{
    t_btsk *btsk;

    info("s %d, cmd %x", s, cmd);

    btsk = btsk_find_by_key(KEY_ANY_SOCK, s);
    
    if (!btsk) 
    {
        info("wsock not found, pass through transparantly");
        return fcntl(s, cmd, arg);
    }

    DUMP_BSOCK("", btsk);

    ASSERTC(btsk->p_ws->wsock == s, "socket not matching", s);

    /* make sure we catch a non blocking configuration to emulate the blz sockets */
    if (cmd == F_SETFL)
    {
       btsk->fcntl_flags |= arg;
    }
    else if (cmd == F_GETFL)
    {
       return btsk->fcntl_flags;
    }

    debug("transparant fcntl");

    /* also let this pass onto sysfunc */
    return fcntl(s, cmd, arg);
}


/*******************************************************************************
**
** Function         blz20_wrp_read
**
** Description    
**
** Returns           
*******************************************************************************/

int blz20_wrp_read(int fd, void *buf, size_t count)
{
    int n;
    
    n = read(fd, buf, count);

    debug("read %d bytes out of %d on fd %d", n, count, fd);

    /* check if socket disconnected and convert to blz socket behaviour */
    if (n == 0)
    {
        t_btsk *btsk;
        
        btsk = btsk_find_by_key(KEY_ANY_SOCK, fd);

        if (!btsk)
            return n;

        if (BTSK_FLAG_ISSET(btsk, BTSK_FLAG_REMOTE_DISC_PND))    
        {
            BTSK_FLAG_CLR(btsk, BTSK_FLAG_REMOTE_DISC_PND);
            info("socket remotely disconnected");
            BTS_SET_ERRNO(ECONNRESET)
            return -1;            
        }
        else if (btsk->rx_ev == BTLIF_BTS_EVT_ABORT)
        {
            btsk->rx_ev = 0;
            info("connection aborted");
            BTS_SET_ERRNO(ECONNABORTED)
            return -1;     
        }            
    }

    // hook up tput mon

    return n; 
}    


/*******************************************************************************
**
** Function         blz20_wrp_write
**
** Description     
**
** Returns           
*******************************************************************************/

int blz20_wrp_write(int fd, void *buf, size_t count)
{
    int n;
    
    /* monitoring purpose only */
    n = write(fd, buf, count);

    debug("wrote %d bytes out of %d on fd %d", n, count, fd);

    // hook up tput mon

    return n; 
}    



/*******************************************************************************
**
** Function         blz20_wrp_select
**
** Description     
**
** Returns           
*******************************************************************************/

int blz20_wrp_select(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout)
{
    /* not currently needed by BT socket api */
    error("unsupported");
    return -1;
}

/*******************************************************************************
**
** Function         blz20_wrp_poll
**
** Description     manages all custom bts poll actions that is normally done inside blz kernel driver
**
** Returns           
*******************************************************************************/

int blz20_wrp_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    tBTL_PARAMS *params;   
    t_btsk *btsk;
    int poll_ret; 
    int i;

    info("nfds %d, timeout %d ms", (int)nfds, timeout);

   /* 
    *  first check for custom bts actions, loop through poll fds 
    */
         
    for (i=0; i < (int)nfds; i++)
    {    
        verbose("check fd %d", fds[i].fd);

        /*
         *  check for listeners 
         */
               
        btsk = btsk_find_by_key(KEY_LISTEN_FD, fds[i].fd);

        if (btsk)
        {           
            DUMP_BSOCK("found listen fd in poll fds", btsk);
            
            /* check if this is a listener socket */
            if (btsk->p_ws->listen_fd == fds[i].fd)
            {
                if (timeout == -1)
                {
                    /* timeout -1 means blocking poll */
                    tBTLIF_BTS_RFC_CON_IND_PARAM *p;

                    info("blocking call for listen fd, wait for btlif event");

                    /* now wait for incoming connection */
                    btlif_set_wait_event(btsk, BTLIF_BTS_RFC_CON_IND);
                    params = btlif_wait_response(btsk);

                    /* make sure we check for abort event */
                    if (btsk->rx_ev == BTLIF_BTS_EVT_ABORT)
                    {                
                        /* connection aborted */
                        BTS_SET_ERRNO(ECONNABORTED);
                        return -1;
                    }

                    p = (tBTLIF_BTS_RFC_CON_IND_PARAM *)params;

                    ASSERTC(p->listen_fd == btsk->p_ws->listen_fd, "invalid sock", p->listen_fd);

                    info("status %d", p->status);           
                }
                else
                {
                    /* currently abort sockets does not use this */
                    error("poll timeout unsupported");
                    usleep(timeout*1000);

                    /* fixme -- add check whether we received a con ind on this btsk */
                }
            }
        }

        /*
         *  check for pending outgoing connections
         */

        btsk = btsk_find_by_key(KEY_ANY_SOCK, fds[i].fd);

        if (btsk && BTSK_FLAG_ISSET(btsk, BTSK_FLAG_ASYNC_MSG_PND))
        {
            DUMP_BSOCK("pending connect", btsk);
        
            if (timeout == -1)
            {
                int result;
                
                /* as timeout -1 was used this is same as blocking, therefor clear flag */
                BTSK_FLAG_CLR(btsk, BTSK_FLAG_ASYNC_MSG_PND);
            
                /* wait here until response comes back */
                btlif_set_wait_events(btsk, BTLIF_BTS_RFC_CON_RSP, BTLIF_BTS_RFC_DISC_IND, 0);
                params = btlif_wait_response(btsk);

                /* make sure we check for abort event */
                if (btsk->rx_ev == BTLIF_BTS_EVT_ABORT)
                {                
                    /* connection aborted */
                    BTS_SET_ERRNO(ECONNABORTED);                    
                    return -1;
                }

                /* check which event unblocked */
                if (btsk->rx_ev == BTLIF_BTS_RFC_CON_RSP)
                {
                    if (params->bts_rfc_con_rsp_param.status != BTL_IF_SUCCESS)
                    {
                        /* we failed to connect, notify timeout */
                        info("connection failed due to %d", params->bts_rfc_con_rsp_param.status);
                        return 1;
                    }

                    /* establish datapath */
                    result = wrp_sock_connect(btsk->p_ws, btsk->p_ws->wsock, btl_ifc_get_srvaddr(),  wrp_getport(SUB_BTS, btsk->rc_addr.rc_channel));

                    ASSERTC(result > 0, "failed", result);    

                    BTS_SET_ERRNO(ERRNO_SUCCESS);   
        
                    info("connection success");              
                }
                else if (btsk->rx_ev == BTLIF_BTS_RFC_DISC_IND)
                {
                    /* connection was rejected */
                    info("connection was rejected");                       
                    BTS_SET_ERRNO(ECONNREFUSED);
                    return -1;
                }                
            }
            else
            {
                /* currently abort sockets does not use this */
                error("poll timeout unsupported");
                usleep(timeout*1000);               

                /* fixme -- add check whether we received a con rsp on this btsk */                
            }
        }

        if (btsk && BTSK_FLAG_ISSET(btsk, BTSK_FLAG_REMOTE_DISC_PND))    
        {
            BTSK_FLAG_CLR(btsk, BTSK_FLAG_REMOTE_DISC_PND);
            info("socket remotely disconnected");
            BTS_SET_ERRNO(ECONNRESET)
            return -1;            
        }
    }

    do_poll:

    /* 
     *  now take care of transparent poll requests 
     */
        
    poll_ret = poll(fds, nfds, timeout);
 
    for (i=0; i < poll_ret; i++)
        debug("transp poll : (fd %d) returned r_ev [%s] (0x%x)", fds[i].fd, pollev2str(fds[i].revents), fds[i].revents);

    debug("return %d", poll_ret);
    return poll_ret;
}



int blz20_set_asocket(struct asocket *asock)
{
    t_btsk *btsk;

    /* find socket */
    btsk = btsk_find_by_key(KEY_ANY_SOCK, asock->fd);

    if (!btsk) 
    {
        error("btsk not found");
        return -1;
    }

    /* store abort socket */
    btsk->asock = asock;    

    info("success (%d,%d,%d)", asock->fd, asock->abort_fd[0], asock->abort_fd[1]);

    return 0;
}
































/*******************************************************************************
**
**                  JNI EMULATION TEST CODE NOT PART OF PRODUCTION CODE
**
** 
**
**     
*******************************************************************************/



#ifdef BTL_IF_CLIENT_TEST_ENABLE

#include "blz20_wrapper.h"


//#include "abort_socket.h"

#define HAVE_BLUETOOTH



#define JNIEnv int
#define jobject int*
#define jboolean BOOLEAN
#define jint int
#define jstring char*
#define true (1)
#define false (0)
#define bool BOOLEAN
#define BTADDR_SIZE 6

#define TYPE_AS_STR(t) \
    ((t) == TYPE_RFCOMM ? "RFCOMM" : ((t) == TYPE_SCO ? "SCO" : "L2CAP"))


#define LOGV debug
#define LOGI info

#define TYPE_RFCOMM 1
#define TYPE_SCO 2
#define TYPE_L2CAP 3

#define RFCOMM_SO_SNDBUF  (70 * 1024)


/////////////////////////////////////////////////////////////////////////////
// ASOCKET 



struct asocket *asocket_init(int fd) {
    int abort_fd[2];
    int flags;
    struct asocket *s;

    LOGV("fd %d", fd);

    /* set primary socket to non-blocking */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return NULL;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        return NULL;

    /* create pipe with non-blocking write, so that asocket_close() cannot
       block */
    if (pipe(abort_fd))
        return NULL;

    flags = fcntl(abort_fd[1], F_GETFL, 0);    

    if (flags == -1)
        return NULL;

    if (fcntl(abort_fd[1], F_SETFL, flags | O_NONBLOCK))
        return NULL;

    s = malloc(sizeof(struct asocket));
    if (!s)
        return NULL;

    s->fd = fd;
    s->abort_fd[0] = abort_fd[0];
    s->abort_fd[1] = abort_fd[1];

    blz20_set_asocket(s);

    return s;
}

int asocket_connect(struct asocket *s, const struct sockaddr *addr,
        socklen_t addrlen, int timeout) {

    int ret;
    LOGV("%s", __FUNCTION__);

    do {
        ret = connect(s->fd, addr, addrlen);
    } while (ret && errno == EINTR);

    if (ret && errno == EINPROGRESS) {
        /* ready to poll() */
        socklen_t retlen;
        struct pollfd pfd[2];

        pfd[0].fd = s->fd;
        pfd[0].events = POLLOUT;
        pfd[0].revents = 0;
        pfd[1].fd = s->abort_fd[0];
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;

        do {
            ret = poll(pfd, 2, timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0)
            return -1;
        else if (ret == 0) {
            info("ret == 0 ==> ETIMEDOUT");            
            /* timeout */
            errno = ETIMEDOUT;
            return -1;
        }

        if (pfd[1].revents) {
            info("pfd[1].revent => ECANCELED");
            /* abort due to asocket_abort() */
            errno = ECANCELED;
            return -1;
        }

        if (pfd[0].revents) {
            info("pfd[0].revents %x", pfd[0].revents);
            if (pfd[0].revents & POLLOUT) {
                info("POLLOUT");                
                /* connect call complete, read return code */
                retlen = sizeof(ret);
                if (getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &ret, &retlen))
                    return -1;
                /* got connect() return code */
                if (ret) {
                    info("got connect return code %d", ret);                     
                    errno = ret;
                }
            } else {
                info("ECONNABORTED"); 
                /* some error event on this fd */
                errno = ECONNABORTED;
                return -1;
            }
        }
    }

    return ret;
}

int asocket_accept(struct asocket *s, struct sockaddr *addr,
        socklen_t *addrlen, int timeout) {

    int ret;
    struct pollfd pfd[2];
    LOGV("%s", __FUNCTION__);


    pfd[0].fd = s->fd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    pfd[1].fd = s->abort_fd[0];
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    do {
        ret = poll(pfd, 2, timeout);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0)
        return -1;
    else if (ret == 0) {
        /* timeout */
        errno = ETIMEDOUT;
        return -1;
    }

    if (pfd[1].revents) {
        /* abort due to asocket_abort() */
        errno = ECANCELED;
        return -1;
    }

    if (pfd[0].revents) {
        if (pfd[0].revents & POLLIN) {
            /* ready to accept() without blocking */
            do {
                debug("accepting...");
                ret = accept(s->fd, addr, addrlen);
            } while (ret < 0 && errno == EINTR);
        } else {
            /* some error event on this fd */
            errno = ECONNABORTED;
            return -1;
        }
    }

    return ret;
}

int asocket_read(struct asocket *s, void *buf, size_t count, int timeout) {
    int ret;
    struct pollfd pfd[2];
    LOGV("%s", __FUNCTION__);

    pfd[0].fd = s->fd;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    pfd[1].fd = s->abort_fd[0];
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    info("s->fd %d (%d %d)", s->fd, s->abort_fd[0],  s->abort_fd[1]);

    do {
        ret = poll(pfd, 2, timeout);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0)
        return -1;
    else if (ret == 0) {
        /* timeout */
        errno = ETIMEDOUT;
        return -1;
    }

    if (pfd[1].revents) {
        /* abort due to asocket_abort() */
        errno = ECANCELED;
        return -1;
    }

    if (pfd[0].revents) {
        if (pfd[0].revents & POLLIN) {
            /* ready to read() without blocking */
            do {
                ret = read(s->fd, buf, count);
                info("read data... ret %d, fd %d, buf %x, count %d", ret, s->fd, buf, count);                
            } while (ret < 0 && errno == EINTR);
        } else {
            /* some error event on this fd */
            errno = ECONNABORTED;
            return -1;
        }
    }

    return ret;
}

int asocket_write(struct asocket *s, const void *buf, size_t count,
        int timeout) {
    int ret;
    struct pollfd pfd[2];
    LOGV("%s", __FUNCTION__);


    pfd[0].fd = s->fd;
    pfd[0].events = POLLOUT;
    pfd[0].revents = 0;
    pfd[1].fd = s->abort_fd[0];
    pfd[1].events = POLLIN;
    pfd[1].revents = 0;

    do {
        ret = poll(pfd, 2, timeout);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0)
        return -1;
    else if (ret == 0) {
        /* timeout */
        errno = ETIMEDOUT;
        return -1;
    }

    if (pfd[1].revents) {
        /* abort due to asocket_abort() */
        errno = ECANCELED;
        return -1;
    }

    if (pfd[0].revents) {
        if (pfd[0].revents & POLLOUT) {
            /* ready to write() without blocking */
            do {
                ret = write(s->fd, buf, count);
            } while (ret < 0 && errno == EINTR);
        } else {
            /* some error event on this fd */
            errno = ECONNABORTED;
            return -1;
        }
    }

    return ret;
}

void asocket_abort(struct asocket *s) {
    int ret;
    char buf = 0;
    LOGV("%s", __FUNCTION__);


    /* Prevent further use of fd, without yet releasing the fd */
    shutdown(s->fd, SHUT_RDWR);

    /* wake up calls blocked at poll() */
    do {
        ret = write(s->abort_fd[1], &buf, 1);
    } while (ret < 0 && errno == EINTR);
}

void asocket_destroy(struct asocket *s) {
    struct asocket s_copy = *s;

    LOGV("%s", __FUNCTION__);


    /* Clients should *not* be using these fd's after calling
       asocket_destroy(), but in case they do, set to -1 so they cannot use a
       stale fd */
    s->fd = -1;
    s->abort_fd[0] = -1;
    s->abort_fd[1] = -1;

    /* Call asocket_abort() in case there are still threads blocked on this
       socket. Clients should not rely on this behavior - it is racy because we
       are about to close() these sockets - clients should instead make sure
       all threads are done with the socket before calling asocket_destory().
     */
    asocket_abort(&s_copy);

    /* enough safety checks, close and release memory */
    close(s_copy.abort_fd[1]);
    close(s_copy.abort_fd[0]);
    close(s_copy.fd);

    free(s);
}

void jniThrowIOException(int *env, int err)
{
    error("jniThrowIOException err %s", strerror(err));
    //exit(1);
}

struct asocket *objarray[30];

static void abortNative(JNIEnv *env, jobject obj);
static void destroyNative(JNIEnv *env, jobject obj);



static void set_asock(int obj, struct asocket *asock) {

    objarray[obj] = asock;
}

static struct asocket *get_socketData(JNIEnv *env, int obj) {

    return objarray[obj];
}


static void initSocketFromFdNative(JNIEnv *env, jobject obj, jint fd) {
#ifdef HAVE_BLUETOOTH
    LOGV("");

    struct asocket *s = asocket_init(fd);

    if (!s) {
        LOGV("asocket_init() failed, throwing");
        jniThrowIOException(env, errno);
        return;
    }

    set_asock(obj, s);
    //env->SetIntField(obj, field_mSocketData, (jint)s);

    return;
#endif
    jniThrowIOException(env, ENOSYS);
}


static void initSocketNative(JNIEnv *env, jobject obj) {
#ifdef HAVE_BLUETOOTH
    LOGV("start jni test");

    int fd;
    int lm = 0;
    int sndbuf;
    jboolean auth;
    jboolean encrypt;
    jint type;

    type = TYPE_RFCOMM; //env->GetIntField(obj, field_mType);

    switch (type) {
    case TYPE_RFCOMM:
        fd = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        break;
    case TYPE_SCO:
        fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO);
        break;
    case TYPE_L2CAP:
        fd = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
        break;
    default:
        jniThrowIOException(env, ENOSYS);
        return;
    }

    if (fd < 0) {
        LOGV("socket() failed, throwing");
        jniThrowIOException(env, errno);
        return;
    }

    auth = 1;//env->GetBooleanField(obj, field_mAuth);
    encrypt = 1;//env->GetBooleanField(obj, field_mEncrypt);

    /* kernel does not yet support LM for SCO */
    switch (type) {
    case TYPE_RFCOMM:
        lm |= auth ? RFCOMM_LM_AUTH : 0;
        lm |= encrypt ? RFCOMM_LM_ENCRYPT : 0;
        lm |= (auth && encrypt) ? RFCOMM_LM_SECURE : 0;
        break;
    case TYPE_L2CAP:
        lm |= auth ? L2CAP_LM_AUTH : 0;
        lm |= encrypt ? L2CAP_LM_ENCRYPT : 0;
        lm |= (auth && encrypt) ? L2CAP_LM_SECURE : 0;
        break;
    }

    if (lm) {
        if (setsockopt(fd, SOL_RFCOMM, RFCOMM_LM, &lm, sizeof(lm))) {
            LOGV("setsockopt(RFCOMM_LM) failed, throwing");
            jniThrowIOException(env, errno);
            return;
        }
    }

    if (type == TYPE_RFCOMM) {
        sndbuf = RFCOMM_SO_SNDBUF;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf))) {
            LOGV("setsockopt(SO_SNDBUF) failed, throwing");
            jniThrowIOException(env, errno);
            return;
        }
    }

    LOGV("...fd %d created (%s, lm = %x)", fd, TYPE_AS_STR(type), lm);

    initSocketFromFdNative(env, obj, fd);
    return;
#endif
    jniThrowIOException(env, ENOSYS);
}

#define RC_CHAN 6
#define L2CAP_PSM 1234


/* Returns errno instead of throwing, so java can check errno */
static int bindListenNative(JNIEnv *env, jobject obj, int type) {
#ifdef HAVE_BLUETOOTH
    debug("");

    //jint type;
    socklen_t addr_sz;
    struct sockaddr *addr;
    bdaddr_t bdaddr = *BDADDR_ANY;
    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return EINVAL;

    //type = env->GetIntField(obj, field_mType);

    switch (type) {
    case TYPE_RFCOMM:
        {
        struct sockaddr_rc addr_rc;
        addr = (struct sockaddr *)&addr_rc;
        addr_sz = sizeof(addr_rc);

        memset(addr, 0, addr_sz);
        addr_rc.rc_family = AF_BLUETOOTH;
        addr_rc.rc_channel = obj;//env->GetIntField(obj, field_mPort);
        memcpy(&addr_rc.rc_bdaddr, &bdaddr, sizeof(bdaddr_t));
        }break;
    case TYPE_SCO:
        {
        struct sockaddr_sco addr_sco;
        addr = (struct sockaddr *)&addr_sco;
        addr_sz = sizeof(addr_sco);

        memset(addr, 0, addr_sz);
        addr_sco.sco_family = AF_BLUETOOTH;
        memcpy(&addr_sco.sco_bdaddr, &bdaddr, sizeof(bdaddr_t));
        }
        break;
    case TYPE_L2CAP:
        {
        struct sockaddr_l2 addr_l2;
        addr = (struct sockaddr *)&addr_l2;
        addr_sz = sizeof(addr_l2);

        memset(addr, 0, addr_sz);
        addr_l2.l2_family = AF_BLUETOOTH;
        addr_l2.l2_psm = L2CAP_PSM; //env->GetIntField(obj, field_mPort);
        memcpy(&addr_l2.l2_bdaddr, &bdaddr, sizeof(bdaddr_t));
        }
        break;
    default:
        return ENOSYS;
    }

    if (bind(s->fd, addr, addr_sz)) {
        LOGV("...bind(%d) gave errno %d", s->fd, errno);
        return errno;
    }

    if (listen(s->fd, 1)) {
        LOGV("...listen(%d) gave errno %d", s->fd, errno);
        return errno;
    }

    LOGV("...bindListenNative(%d) success", s->fd);

    return s->fd;

#endif
    return ENOSYS;
}



static jobject acceptNative(JNIEnv *env, jobject obj, int type, int timeout) {
#ifdef HAVE_BLUETOOTH
    debug("");

    int fd;
    //jint type;
    struct sockaddr *addr;
    socklen_t addr_sz;
    //jstring addr_jstr;
    //char addr_cstr[BTADDR_SIZE];
    bdaddr_t *bdaddr;
    jboolean auth;
    jboolean encrypt;

    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return NULL;

    //type = env->GetIntField(obj, field_mType);

    switch (type) {
    case TYPE_RFCOMM:
        {
        struct sockaddr_rc addr_rc;
        addr = (struct sockaddr *)&addr_rc;
        addr_sz = sizeof(addr_rc);
        bdaddr = &addr_rc.rc_bdaddr;
        memset(addr, 0, addr_sz);
        } break;
    case TYPE_SCO:
        { struct sockaddr_sco addr_sco;
        addr = (struct sockaddr *)&addr_sco;
        addr_sz = sizeof(addr_sco);
        bdaddr = &addr_sco.sco_bdaddr;
        memset(addr, 0, addr_sz);
        } break;
    case TYPE_L2CAP:
        { struct sockaddr_l2 addr_l2;
        addr = (struct sockaddr *)&addr_l2;
        addr_sz = sizeof(addr_l2);
        bdaddr = &addr_l2.l2_bdaddr;
        memset(addr, 0, addr_sz);
        } break;
    default:
        jniThrowIOException(NULL, ENOSYS);
        return NULL;
    }

    fd = asocket_accept(s, addr, &addr_sz, timeout);

    LOGV("...accept(%d, %s) = %d (errno %d %s)",
            s->fd, TYPE_AS_STR(type), fd, errno, strerror(errno));

    if (fd < 0) {
        jniThrowIOException(env, errno);
        return NULL;
    }

    /* Connected - return new BluetoothSocket */
    auth = 1;//env->GetBooleanField(obj, field_mAuth);
    encrypt = 1;//env->GetBooleanField(obj, field_mEncrypt);

    // tmp -- close listener
    abortNative(NULL, obj);
    destroyNative(NULL, obj);

    /* now set new asocket for accepted connection */
    initSocketFromFdNative(env, obj, fd);

    //get_bdaddr_as_string(bdaddr, addr_cstr);

    //addr_jstr = env->NewStringUTF(addr_cstr);
    //return env->NewObject(class_BluetoothSocket, method_BluetoothSocket_ctor,
    //        type, fd, auth, encrypt, addr_jstr, -1);

    
    return fd;
#endif
    jniThrowIOException(env, ENOSYS);
    return NULL;
}


static void connectNative(JNIEnv *env, jobject obj, int type, bdaddr_t bd_address, int rc_chan, unsigned short l2_psm) {
#ifdef HAVE_BLUETOOTH
    info("");

    int ret;
    //jint type;
    //const char *c_address;
    //jstring address;
    bdaddr_t bdaddress;
    socklen_t addr_sz;
    struct sockaddr *addr;
    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return;

    //type = env->GetIntField(obj, field_mType);

    /* parse address into bdaddress */
    //address = (jstring) env->GetObjectField(obj, field_mAddress);
    //c_address = env->GetStringUTFChars(address, NULL);

    //if (get_bdaddr(c_address, &bdaddress)) {
    //    env->ReleaseStringUTFChars(address, c_address);
    //    jniThrowIOException(env, EINVAL);
    //    return;
    //}
    //env->ReleaseStringUTFChars(address, c_address);

    switch (type) {
    case TYPE_RFCOMM:{
        struct sockaddr_rc addr_rc;
        addr = (struct sockaddr *)&addr_rc;
        addr_sz = sizeof(addr_rc);

        memset(addr, 0, addr_sz);
        addr_rc.rc_family = AF_BLUETOOTH;
        addr_rc.rc_channel = rc_chan; //env->GetIntField(obj, field_mPort);
        memcpy(&addr_rc.rc_bdaddr, &bd_address, sizeof(bdaddr_t));

        } break;
    case TYPE_SCO:
        { struct sockaddr_sco addr_sco;
        addr = (struct sockaddr *)&addr_sco;
        addr_sz = sizeof(addr_sco);

        memset(addr, 0, addr_sz);
        addr_sco.sco_family = AF_BLUETOOTH;
        memcpy(&addr_sco.sco_bdaddr, &bdaddress, sizeof(bdaddr_t));

        } break;
    case TYPE_L2CAP:
        { struct sockaddr_l2 addr_l2;
        addr = (struct sockaddr *)&addr_l2;
        addr_sz = sizeof(addr_l2);

        memset(addr, 0, addr_sz);
        addr_l2.l2_family = AF_BLUETOOTH;
        addr_l2.l2_psm = l2_psm; //env->GetIntField(obj, field_mPort);
        memcpy(&addr_l2.l2_bdaddr, &bdaddress, sizeof(bdaddr_t));

        } break;
    default:
        jniThrowIOException(env, ENOSYS);
        return;
    }

    ret = asocket_connect(s, addr, addr_sz, -1);
    LOGV("...connect(%d, %s) = %d (errno %d)",
            s->fd, TYPE_AS_STR(type), ret, errno);

    if (ret)
        jniThrowIOException(env, errno);

    return;
#endif
    jniThrowIOException(env, ENOSYS);
}

#define jbyteArray char*
#define jbyte char

static jint readNative(JNIEnv *env, jobject obj, char *p_buf,
        jint length) {
#ifdef HAVE_BLUETOOTH
    info("");

    int ret;
    //jbyte *b;
    //int sz;
    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return -1;
#if 0    
    if (jb == NULL) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }
    sz = env->GetArrayLength(jb);
    if (offset < 0 || length < 0 || offset + length > sz) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }

    b = env->GetByteArrayElements(jb, NULL);
    if (b == NULL) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }
#endif
    ret = asocket_read(s, p_buf, length, -1);
    if (ret < 0) {
        jniThrowIOException(env, errno);
//        env->ReleaseByteArrayElements(jb, b, JNI_ABORT);
        return -1;
    }

  //  env->ReleaseByteArrayElements(jb, b, 0);
    return (jint)ret;

#endif
    jniThrowIOException(env, ENOSYS);
    return -1;
}

static jint writeNative(JNIEnv *env, jobject obj, jbyteArray jb, jint offset,
        jint length) {
#ifdef HAVE_BLUETOOTH
    info("");

    int ret;
    jbyte *b;
    //int sz;
    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return -1;
#if 0
    if (jb == NULL) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }
    sz = env->GetArrayLength(jb);
    if (offset < 0 || length < 0 || offset + length > sz) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }

    b = env->GetByteArrayElements(jb, NULL);
    if (b == NULL) {
        jniThrowIOException(env, EINVAL);
        return -1;
    }
#endif
    ret = asocket_write(s, jb, length, -1);

    if (ret < 0) {
        jniThrowIOException(env, errno);
//        env->ReleaseByteArrayElements(jb, b, JNI_ABORT);
        return -1;
    }

  //  env->ReleaseByteArrayElements(jb, b, JNI_ABORT);  // no need to commit
    return (jint)ret;

#endif
    jniThrowIOException(env, ENOSYS);
    return -1;
}


static void abortNative(JNIEnv *env, jobject obj) {
#ifdef HAVE_BLUETOOTH
    info("");
    struct asocket *s = get_socketData(env, obj);

    if (!s)
        return;

    asocket_abort(s);

    LOGV("...asocket_abort(%d) complete", s->fd);
    return;
#endif
    jniThrowIOException(env, ENOSYS);
}


static void destroyNative(JNIEnv *env, jobject obj) {
#ifdef HAVE_BLUETOOTH
    info("");
    struct asocket *s = get_socketData(env, obj);
    int fd = s->fd;

    if (!s)
        return;

    asocket_destroy(s);

    LOGV("...asocket_destroy(%d) complete", fd);
    return;
#endif
    jniThrowIOException(env, ENOSYS);
}


/*******************************************************************************
**
**                          TEST BENCH FOR BT SOCKET API
**
**        
*******************************************************************************/

pthread_t jni_thread_id;

void connect_rfchan(bdaddr_t bd, int scn)
{
    info("\n\n\n################ CONNECT SCN %d #############################", scn);
    initSocketNative(0,scn);   
    connectNative(NULL, scn, TYPE_RFCOMM, bd, scn, 0);    
}

void start_server_chan(int scn)
{
    int fd;
    
    info("\n\n\n################ START SERVER SCN %d #############################", scn);

    initSocketNative(0,scn); 

    bindListenNative(0,scn, TYPE_RFCOMM);

    /* no timeout */
    acceptNative(NULL, scn, TYPE_RFCOMM, -1);   
}


void close_rfchan(int scn)
{
    info("\n\n\n################ CLOSE SCN %d #############################", scn);

    abortNative(NULL, scn);
    destroyNative(NULL, scn);
}

/* assumes scn is already connected */
void write_sz(int scn, int sz)
{
    int i = 0;
    char buf[1024];

    memset(buf, 'm', 1024);

    info("\n\n\n################ WRITE %d BYTES ON SCN %d #############################", sz, scn);

    /* write incrementally larger buffers */
    writeNative(0, scn, buf, 0, sz);
}



/* assumes scn is already connected */
void write_incremental(int scn)
{
    int i = 0;
    char buf[1024];

    /* write incrementally larger buffers */
    while (i<1024)
    {    
        writeNative(0,scn, buf, 0, i);
    }
}

/*******************************************************************************
**
** Function         test01_multiconn
**
** Description     open up multiple socket connections
**
** Returns           
*******************************************************************************/

void ctest01_singleconn(int scn, bdaddr_t bd)
{
    connect_rfchan(bd, scn);
    sleep(3);
    write_sz(scn, 26);
    sleep(10);
    close_rfchan(scn);        
    sleep(1);
}


void ctest02_multiconn(bdaddr_t bd)
{
    int scn;

    for (scn=30; scn<31; scn++)
    {
        connect_rfchan(bd, scn);
        close_rfchan(scn);        
        sleep(1);
    }
}


void stest_01_multlisteners(void)
{
    int scn;

    for (scn=1; scn<16; scn++)
    {
        start_server_chan(scn);
        sleep(1);
    }
    
}


/*******************************************************************************
**
** Function        threads
**
** Description     
**
** Returns           
*******************************************************************************/

static void jni_ag_server_thread(void *p)
{
   stest_01_multlisteners();

   while(1)
    sleep(1);
}


static void jni_ag_client_thread(void *p)
{
    int fd;
    int ret = 0;    
    bdaddr_t bd;

    //get_bdaddr("00:0a:3a:85:fb:3e", &bd);
    get_bdaddr("00:04:3e:00:00:01", &bd);



    /* TEST SUITE FOR CLIENT API */      

    /* NORMAL */


    ctest01_singleconn(30, bd);

    /* PERFORMANCE */
    /* write */
    /* check max write speed */
    
    /* ABNORMAL */

    /* fixme -- read & write to the wrong scn, check status */
    /* fixme -- close the wrong socket */
    /* fixme -- close same socket twice */

    while(1)
        sleep(1);    

}



void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}


UINT32 get_int(char **p, int DefaultValue)
{
  UINT32 Value = 0;
  BOOLEAN   UseDefault;

  UseDefault = TRUE;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

int get_signed_int(char **p, int DefaultValue)
{
  int    Value = 0;
  BOOLEAN   UseDefault;
  BOOLEAN   NegativeNum = FALSE;

  UseDefault = TRUE;
  skip_blanks(p);

  if ( (**p) == '-')
    {
      NegativeNum = TRUE;
      (*p)++;
    }
  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == FALSE)? Value : -Value);
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

UINT32 get_hex(char **p, int DefaultValue)
{
  UINT32 Value = 0;
  BOOLEAN   UseDefault;

  UseDefault = TRUE;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') ||
          ((**p)<= 'f' && (**p)>= 'a') ||
          ((**p)<= 'F' && (**p)>= 'A') )
    {
      if (**p >= 'a')
        Value = Value * 16 + (**p) - 'a' + 10;
      else if (**p >= 'A')
        Value = Value * 16 + (**p) - 'A' + 10;
      else
        Value = Value * 16 + (**p) - '0';
      UseDefault = FALSE;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}


void server_test(void)
{
        /* create test thread */
         if (pthread_create(&jni_thread_id, NULL, 
                       (void*)jni_ag_server_thread, (void*)0)!=0)
            perror("pthread_create");

         while (1)
            sleep(1);
}

void client_test(void)
{
        /* create test thread */
         if (pthread_create(&jni_thread_id, NULL, 
                       (void*)jni_ag_client_thread, (void*)0)!=0)
            perror("pthread_create");

         while (1)
            sleep(1);
}


/*******************************************************************************
**
**  Console cmd table
**
*******************************************************************************/

#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)
#define if_cmd(str)  if (is_cmd(str))

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    BOOLEAN is_job;
} t_cmd;


const t_cmd console_cmd_list[];


static void job_handler(void *param)
{
    char *job_cmd = (char*)param;

    info("job starting (%s)", job_cmd);

    process_cmd(job_cmd, TRUE);

    info("job terminating");

    free(job_cmd);
}

static int create_job(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void*)job_handler, (void*)job_cmd)!=0)
      perror("pthread_create");

    return 0;
}


void do_help(char *p)
{
    int i = 0;

    while (console_cmd_list[i].name != NULL)
    {
        printf("\"%s\" - %s\n", (UINT32)console_cmd_list[i].name, (UINT32)console_cmd_list[i].help);
        i++;
    }
}

void do_rfcon(char *p)
{
    bdaddr_t bd;
    int scn; 
    char bd_str[20];

    scn = get_int(&p, -1);
    get_str(&p, bd_str);
    get_bdaddr(bd_str, &bd);

    printf("scn %d, bd %s\n", scn, bd_str);
    connect_rfchan(bd, scn);
}

void do_rfsend(char *p)
{
    int scn, sz; 

    scn = get_int(&p, -1);
    sz = get_int(&p, 0);
    
    write_sz(scn, sz);
}

void do_rfsendmulti(char *p)
{
    int scn, sz, count, i;

    scn = get_int(&p, -1);
    sz = get_int(&p, 0);
    count = get_int(&p, 0);

    /* blocking write */
    for (i=0; i<count;i++)
        write_sz(scn, sz);
}

void do_rfrecv(char *p)
{
    char buf[1024];    
    int scn;
    int recv = 0;

    scn = get_int(&p, -1);

    start_server_chan(scn);  

    while (recv !=-1)
    {
        recv = readNative(0, scn, buf, 1024);   
        info("### READ %d BYTES ###", recv);
    }
}

void do_rfdisc(char *p)
{
    int scn;
    scn = get_int(&p, -1);
    close_rfchan(scn);   
}

void do_wrpconn(char *p)
{
    int port, s;
    t_wsock *ws;

    ws = wrp_wsock_create(SUB_BTS);
    port = get_int(&p, -1);

    wrp_sock_connect(ws, ws->wsock,  btl_ifc_get_srvaddr(),  port);
}

void do_wrpdisc(char *p)
{
    int port, s;
    t_wsock *ws;
    
    ws = wrp_wsock_create(SUB_BTS);

    port = get_int(&p, -1);

    wrp_close_s_only(ws, ws->wsock);
}

void do_rflisten(char *p)
{
    int scn;
    scn = get_int(&p, -1);
    start_server_chan(scn);   
}

/* 
  *  OLD BLZ WRAPPER TEST API 
  */

extern void jni_test(void);
extern void jni_test_async(void);
extern void jni_test_audiogateway(void);


/* old blz wrapper test api */
void do_blz_wrapper_test(char *p)
{
    p++; // skip space 

    if (!strncmp(p, "jniconn", 7))
    {
    printf("asdsad\n");
       set_native_addr(p+4);       
       jni_test();
    }
    else if (!strncmp(p, "jniasync", 8))
    {
       set_native_addr(p+10);          
       jni_test_async();
    }
    else if (!strncmp(p, "jniag", 5))
    {
       void jni_test_audiogateway(void);
       jni_test_audiogateway();
    }
}

const t_cmd console_cmd_list[] =  
{
    { "help", do_help, "lists all available console commands", FALSE },
    { "rcon", do_rfcon, "<scn> <bd xx:yy:... >", FALSE },
    { "rsnd", do_rfsend, "<scn> <nbr bytes>", FALSE },
    { "rsndmulti", do_rfsendmulti, "<scn> <nbr bytes> <nbr buffers>", FALSE },
    { "rrecv", do_rfrecv, "<scn>", TRUE },
    { "rdisc", do_rfdisc, "<scn>", FALSE },
    { "rlist", do_rflisten, "<scn>", TRUE },
    { "wrpcon", do_wrpconn, "<port>", FALSE },
    { "wrpdisc", do_wrpdisc, "<port>", FALSE },

    /* test old blz wrapper, cram all of them into this one */
    { "blz", do_blz_wrapper_test, "<jniconn <bd> | jniasync | jniag>", TRUE },    
    NULL, NULL, "",
};



void process_cmd(char *p, BOOLEAN is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {   
            if (!is_job && console_cmd_list[i].is_job)
                create_job(p_saved);
            else
                console_cmd_list[i].handler(p);
            return;
        }
        i++;
    }
}

static char *local_getline(char *zPrompt, FILE *in){
  char *zLine;
  int nLine;
  int n;
  int eol;

  if( zPrompt && *zPrompt ){
    printf("%s",zPrompt);
    fflush(stdout);
  }
  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, in)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}


#if defined(HAVE_READLINE) && HAVE_READLINE==1
#include <readline/readline.h>
#include <readline/history.h>
#else
#define readline(p) local_getline(p,stdin)
#define add_history(X)
#define read_history(X)
#define write_history(X)
#endif

#define HISTORY_FILE ".history"

void jni_parsecmd(char *cmd)
{
    read_history(HISTORY_FILE); 
    
    printf("\ntype 'help' for list of commands\n\n");  

    while (1)  
    {    
        char *line = (char *)readline("> ");
        
        add_history(line);

        /* write for each new command */
        write_history(HISTORY_FILE);

        if (line[0]==0)
           continue;
        
        process_cmd(line, FALSE);
  }
}

#endif

