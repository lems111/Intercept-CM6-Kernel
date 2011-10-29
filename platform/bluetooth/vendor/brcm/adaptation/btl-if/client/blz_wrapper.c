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

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sco.h>

/* this tag needs to be defined before btl_ifc.h include */
#define LOG_TAG "BLZ_WRAPPER"

#include "btl_ifc.h"
#include "btl_ifc_wrapper.h"

tCTRL_HANDLE ctrl_hdl;
static BOOLEAN blz_wrapper_init_done = FALSE;

#define CHECK_BLZ_INIT BLZ_AUTO_INIT
//#define CHECK_BLZ_INIT() if (!blz_wrapper_init_done){ error("%s : blz wrapper not initialized\n", __FUNCTION__); exit(1);}
#define BLZ_AUTO_INIT() if (!blz_wrapper_init_done) { info("autostarting blz wrapper\n");if (blz_wrapper_init()<0) return -1;}
#define DUMP_WSOCK(msg, ws) if (ws) debug("[%s] %s: (%d:%d), disc_pending %d, asnc %d, selct %d, flags %x\n", __FUNCTION__, msg, ws->wsock, ws->listen_fd, ws->disconnect_pending, ws->async_msg_pnd, ws->select_pending, ws->flags);

/*******************************************************************************
**
**  Misc
**          
**
*******************************************************************************/

void swap_array(char *dst, char *src, int n)
{
    char *d = dst;
    char *s = src;
    unsigned int i;

    for (i = 0; i < n; i++)
        d[i] = s[n - 1 - i];
}

/*******************************************************************************
**
**  Blocking syscall management
**          
**
*******************************************************************************/
    
typedef enum {
    WAIT_EVENT_NONE,
    WAIT_EVENT_CONNECT_CFM,    
} t_blz_event;

pthread_mutex_t ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

static t_blz_event wait_event_id;
static tBTL_PARAMS wait_params;

struct sockaddr_rc rc_addr_stored;
static list_head ws_async_pending;

tBTL_PARAMS* wait_response(t_blz_event event)
{
    debug("wait_response [%d]\n", event);
    pthread_mutex_lock(&cond_mutex);    
    pthread_mutex_unlock(&ctrl_mutex);
    wait_event_id = event;
    pthread_cond_wait(&cond, &cond_mutex);
    pthread_mutex_unlock(&cond_mutex);
    debug("wait_response [%d] unblocked\n", event);
  return &wait_params;
}

void signal_event(t_blz_event event, tBTL_PARAMS *params)
{
    debug("signal_event [%d]\n", event);
    pthread_mutex_lock(&cond_mutex);

    /* if event matches the current wait event, unblock action */
    if (event == wait_event_id)
    {
        //wait_params = *params;        
        memcpy(&wait_params, params, sizeof(tBTL_PARAMS));

        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&cond_mutex);
}

/*******************************************************************************
**
**  Async syscall management
**          
**
*******************************************************************************/

void ws_async_add(t_wsock *ws)
{
    DUMP_WSOCK("", ws);
    list_add(&ws->async_node, &ws_async_pending);  
}
void ws_async_del(t_wsock *ws)
{ 
    DUMP_WSOCK("", ws);
    list_del(&ws->async_node);  
}

t_wsock * ws_async_fetch(void)
{
    t_wsock *ws = NULL;

    if (!list_isempty(&ws_async_pending)) 
    {
        /* get first in queue */
        ws = list_get_obj(ws_async_pending.Next, t_wsock, async_node);
        DUMP_WSOCK("", ws);
    }
    return ws;
}

t_wsock *ws_async_find_fdset(fd_set *fds)
{
    t_wsock *ws;
    struct list_head *pos; 

    list_for_each(pos, &ws_async_pending) 
    {
        ws = list_get_obj(pos, t_wsock, async_node);

        if ((ws->wsock>0) && FD_ISSET(ws->wsock, fds))
        {  
            //DUMP_WSOCK("found wsock", ws);        
            return ws;
        }    
    }
    return NULL;
}

/*******************************************************************************
**
**  Bluez wrapper helper functions
**          
**
*******************************************************************************/

void blz_ctrl_msg(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    t_wsock *ws;

    debug("[blz ctrl] received message [%s]\n", dump_msg_id(id));

    pthread_mutex_lock(&ctrl_mutex);

    switch(id)
    {
        case BTLIF_CONNECT_REQ:
            /* managed by upper layers */
            break;
            
        case BTLIF_CONNECT_IND:
            {
                /* search for listener bound to this rfcomm channel */
                ws = wrp_find_wsock_by_rfhdl(params->ag_conreq.rf_chan, TRUE);

                /* While Bluetooth is turning off and a stereo headset is trying
                 * to connect, if we don't have headset's bd_address and rf_chan 
                 * information, we could not connect it again. Hence
                 * "rc_addr_stored.rc_bdaddr.b" and "rc_addr_stored.rc_channel"
                 * need to be stored for next connection.
                 */

                /* save incoming bd address for later notification through syscalls */
                swap_array((char*)rc_addr_stored.rc_bdaddr.b, (char *)&params->ag_conreq, 6);
                rc_addr_stored.rc_channel = params->ag_conreq.rf_chan;

                if (!ws)
                {
                    error("BTLIF_CONNECT_IND : wsock not found for rf chan %d\n", params->ag_conreq.rf_chan);
                    error("Disconnect RFCOMM Channel");
                    BTLIF_AG_Disconnect(ctrl_hdl, params->ag_conreq.rf_chan);
                    pthread_mutex_unlock(&ctrl_mutex);

                    return;
                }

                BTLIF_AG_ConnectIndAck(ctrl_hdl, (BD_ADDR *)&params->ag_conreq, params->ag_conreq.rf_chan);
            }
            break;

        case BTLIF_CONNECT_RSP:
            {                
                ws = ws_async_fetch(); 

                DUMP_WSOCK("async fetch", ws);

                if (ws && ws->async_msg_pnd)
                {
                    debug("blz_ctrl_msg : set selectpending 0\n");

                    memcpy(&ws->async_params, params, sizeof(tBTLIF_CONNECT_RSP_PARAM));
                    //ws->async_params = *params;

                    ws->select_pending = 0;
                    
                    // fixme -- break current select call instead of waiting out the full timeout                    
                }
                else
                {
                    /* blocking call */
                    signal_event(WAIT_EVENT_CONNECT_CFM, params);
                }
            }
            break;

        case BTLIF_DISCONNECT_RSP:
               /* managed by upper layers in blz wrapper */
            break;

        case BTLIF_DISCONNECT_IND:
           {
                t_wsock *ws;

                /* data channel disconnected by server */
                debug("Channel disconnected remotely, rf_ch %d\n", params->ag_disc.rf_chan);

                /* search for active data socket bound to this rfcomm channel */
                ws = wrp_find_wsock_by_rfhdl(params->ag_disc.rf_chan, FALSE);
                
                if (ws)
                {
                    /* make sure we notify user next time select or poll is called */
                    ws->disconnect_pending= 1;
                    DUMP_WSOCK("Set disconnect pending\n", ws);
                }  
           }
           break;
            
        default: 
            break;
    }

    pthread_mutex_unlock(&ctrl_mutex);
}


/* called once per application process */
int blz_wrapper_init(void)
{
    if (blz_wrapper_init_done) return 0;

    info("btl_if_blz_wrapper_init\n");

    /* initialize async list */
    INIT_LIST_HEAD(&ws_async_pending);  

    /* initialize wrapper */
    wrp_init();

    /* make sure client is initialized */
    BTL_IFC_ClientInit();
    
    if (btl_ifc_main_running())
    {
       debug("blz wrapper init done !\n");
       blz_wrapper_init_done = TRUE;
       return 0;
    }

    debug("blz wrapper init failed\n");
    return -1;
}

/* called once per subsystem init */
int blz_init_subsystem(tBTL_IF_SUBSYSTEM sub)
{
    /* register with datapath server, no datacallback used as upper layers will manage raw sockets */   
    return BTL_IFC_RegisterSubSystem(&ctrl_hdl, sub, NULL, blz_ctrl_msg);
}

/*******************************************************************************
**
** Wrapper APIx
**          
**
*******************************************************************************/

int btl_if_socket(int socket_family, int socket_type, int protocol)
{
    CHECK_BLZ_INIT();
    
    if (socket_family != PF_BLUETOOTH)
        return -1;
    
    info("btl_if_socket :: fam %d, type %d, proto %d\n", socket_family, socket_type, protocol);
    
    switch (protocol)
    {
        case BTPROTO_RFCOMM :
            /* make sure this subsystem is initialized */
            blz_init_subsystem(SUB_AG);
            return wrp_sock_create(SUB_AG);
            break;

        case BTPROTO_SCO :            
            /* make sure this subsystem is initialized */            
            blz_init_subsystem(SUB_SCO);
            return wrp_sock_create(SUB_SCO);
            
        default:
            return -1;
    }
    return -1;
}


int btl_if_read(int fd, void *buf, size_t count)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    ws = wrp_find_wsock(fd);

    info("%s: fd %d, buf %x\n", __FUNCTION__, fd, count);

    /* while we have an async operation pending, return 0 */
    if (ws->async_msg_pnd)
        return 0;
    
    return read(fd, buf, count);
}

/* returns nbr of fd:s that has info on requested events */
int btl_if_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int i;
    t_wsock* ws;
    int disconnected = 0;
    int result;

    CHECK_BLZ_INIT();

    /* check if this socket is disconnected */
    for (i=0; i < (int)nfds; i++)
    {
        //info("%s: poll fd %d, ev %d, timeout %d\n", __FUNCTION__, fds[i].fd, fds[i].events, timeout);

        ws = wrp_find_wsock(fds[i].fd);        

        // remove for now to reduce logging
        //DUMP_WSOCK("", ws);
        
        if (ws==NULL)
        {
            /* notify poller that this socket is not up anymore */
            debug("wsock down, return POLLHUP poll fd %d, ev %x\n", fds[i].fd, fds[i].events);            
            errno = EIO; // signal io error
            fds[i].revents = POLLHUP;
            disconnected = 1;
        }

        /* return error if link disconnection was detected (this should trigger a close) */
        else if (ws->disconnect_pending)
        {
            /* only notify data sockets */
            if (ws->wsock == fds[i].fd)
            {                
                error("detected pending data socket disconnect, notify caller\n");
                errno = EIO; // signal io error
                fds[i].revents =  POLLHUP;      
                disconnected = 1;
            }
        }
    }

    /* return -1 will trigger a close of this socket */
    if (disconnected) {
        /* wait full poll timeout before returning */
        usleep(timeout*1000);
        return -1;
    }
    
    /* transparent poll */
    result = poll(fds, nfds, timeout);

    if (result == 0)
    {
       //info("transp poll : no events on fds\n");
    }
    else
    {
        for (i=0; i < result; i++)
            info("transp poll : (fd %d) returned r_ev %d\n", fds[i].fd, fds[i].revents);
    }

    return result;
}


int btl_if_select(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout)
{
    t_wsock* ws;
    int result;

    CHECK_BLZ_INIT();

    //info("%s, timeout %d,%d\n", __FUNCTION__, (int)timeout->tv_sec, (int)timeout->tv_usec);

    /* check if we need to do any special handling of this select call for pending btl if operations */
    ws = ws_async_find_fdset(readfds);

    //DUMP_WSOCK("", ws);

    if (ws)
    {

        DUMP_WSOCK("", ws);

        /* return error if link disconnection was detected (this should trigger a close) */
        if (ws->disconnect_pending || !ws->busy)
        {
             error("disconnect pending, return -1\n");
             errno = EIO; // signal io error
             return -1;
        }

        if (ws->select_pending)
        {
            debug("btl_if_select : async pending, sleep [%d s, %d us]\n", (int)timeout->tv_sec, (int)timeout->tv_usec);
            
            /* wait full timeout for now... */            
            sleep(timeout->tv_sec);
            usleep(timeout->tv_usec);
            return 0;
        }
        else 
        {
           int result;
           debug("btl_if_select : async connection completed, check result\n");

           /* server accepted our connect request, now setup actual datachannel.
                        we will only have 1 datachannel for this subsystem so lets use subport 0 */

           ws = wrp_find_wsock(ws->wsock);                           

           if(ws->async_msg_pnd == BTLIF_CONNECT_RSP)
           {    
              /* check result of connection attempt */
              if (ws->async_params.conrsp.result !=  BTL_IF_SUCCESS)
              {
                  info("async connection completed with result %d\n", ws->async_params.conrsp.result);

                  /* clear async list */
                  ws_async_del(ws);  
                  ws->async_msg_pnd = 0;            
                  errno = EBADF;
                  return -1;
              }
           }
           else
           {
               debug("async operation %d completed but was unhandled\n", ws->async_msg_pnd);
           }

           /* establish connection */
           result = wrp_sock_connect(ws, ws->wsock, btl_ifc_get_srvaddr(), ws->port);

           /* clear async list */
           ws_async_del(ws);                      
           ws->async_msg_pnd = 0;

           /* now set the flags on new socket fd */
           fcntl(ws->wsock, F_SETFL, ws->flags);

        }
    }

    /* ok, we are done with any special handling, let user call select transparantly */
    result = select(nfds, readfds, writefds, exceptfds, timeout);
   
    debug("btl_if_select : transparant mode, result %d\n", result);

    return result;
}


int btl_if_connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_connect : no wsock found\n");
        return -1;   
    }
    
    //info("btl_if_connect : fd %d, async %d [%s]\n", s, ws->flags&O_NONBLOCK, ws->name);

    switch(ws->sub)
    {
        case SUB_AG:
            {
                struct sockaddr_rc *addr = (struct sockaddr_rc*)serv_addr;
                
                info("btl_if_connect : rf_chan %d, bd[%x:%x]\n", addr->rc_channel, (unsigned char)addr->rc_bdaddr.b[4], (unsigned char)addr->rc_bdaddr.b[5]);
                
                /* check if socket is setup in non blocking mode */
                if (ws->flags & O_NONBLOCK)
                {            
                    /* make select wait until we are done with this operation */
                    ws->select_pending = 1;   
                
                    if (!ws->async_msg_pnd)
                    {
                        debug("set async pending, return EINPROGRESS and send connect req...\n");
                        
                        ws->async_msg_pnd = BTLIF_CONNECT_RSP;
                        ws->port = wrp_getport(SUB_AG, addr->rc_channel); // save this for later
                        ws->rf_chan = addr->rc_channel;

                        /* store this wsock for later retrieval when server responds */
                        ws_async_add(ws);
                   
                        BTLIF_AG_ConnectReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);
                
                        errno = EINPROGRESS;
                        return -1;
                    }
                    else
                        error("error : async already pending !\n");
                }
                else
                {
                    tBTL_PARAMS *params;

                    /* make sure we block this call until a response is received */
                    pthread_mutex_lock(&ctrl_mutex);

                    BTLIF_AG_ConnectReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);
                    
                    info("wait for con rsp\n");

                    /* wait here until we have a response */
                    params = wait_response(WAIT_EVENT_CONNECT_CFM);

                    info("connect response %d\n", params->conrsp.result);

                    if (params->conrsp.result == BTL_IF_SUCCESS)
                    {   
                        /* server accepted our connect request, now setup actual datachannel.
                                            we will only have 1 datachannel for this subsystem so lets use subport 0 */

                        ws = wrp_find_wsock(s);                           
                        ws->rf_chan = addr->rc_channel;

                        /* establish connection */
                        wrp_sock_connect(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_AG, addr->rc_channel));

                        /* add handle to active list */
                        //wsactive_add(ws, s);
                    }
                    else
                    {   
                        /* connection failed, clear async list */
                        ws_async_del(ws);                      
                        ws->async_msg_pnd = 0;

                        /* connection failed, return error to JNI */
                        return -1;
                    }
                }                
            }
            break;
            
        case SUB_SCO:
         {
                //struct sockaddr_rc *addr = (struct sockaddr_rc*)serv_addr;

                info("btl_if_connect : [sco]\n");
               
                /* check if socket is setup in non blocking mode */
                if (ws->flags & O_NONBLOCK)
                {
                    /* make select wait until we are done with this operation */
                    ws->select_pending = 1;   
                    
                    if (!ws->async_msg_pnd)
                    {
                        debug("set async pending, return EINPROGRESS\n");                        
                        ws->async_msg_pnd = BTLIF_CONNECT_RSP;
                        ws->port = wrp_getport(SUB_SCO, 0); // save this for later                        

                        /* store this wsock for later retrieval when server responds */
                        ws_async_add(ws);                   

                        BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_CONNECT_REQ, NULL, 0);
                        errno = EINPROGRESS;
                        return -1;
                    }                     
                }
                else
                {
                    tBTL_PARAMS *params;

                    /* make sure we block this call until a response is received */
                    pthread_mutex_lock(&ctrl_mutex);

                    BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_CONNECT_REQ, NULL, 0);
                    
                    info("wait for con rsp\n");

                    /* wait here until we have a response */
                    params = wait_response(WAIT_EVENT_CONNECT_CFM);

                    info("connect status %d !\n", params->conrsp.result);

                    if (params->conrsp.result == BTL_IF_SUCCESS)
                    {   
                        ws = wrp_find_wsock(s);                           

                        /* establish connection ono subport 0 */
                        if (wrp_sock_connect(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_SCO, 0))<0)
                           return -1;

                        /* add handle to active list */
                        //wsactive_add(ws, s);
                    }
                    else
                    {   
                        /* connection failed, give other status than -1 ? */
                        return -1;
                    }
                }                
            }            
          
            break;            
        default :
            error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
            return -1;

    }


    return ws->wsock;//wrp_sock_connect(ws, s); 
}

int btl_if_bind(int s, const struct sockaddr *my_addr, socklen_t addrlen)
{
    t_wsock* ws;
    int result = -1;

    CHECK_BLZ_INIT();

    //info("btl_if_bind : fd %d\n", s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_bind : no wsock found\n");
        return -1;   
    }

    switch(ws->sub)
    {
        case SUB_AG:
            {
                struct sockaddr_rc *addr = (struct sockaddr_rc*)my_addr;    

                memcpy((char*)&rc_addr_stored, (char*)addr, sizeof(struct sockaddr_rc));

                if (addr->rc_channel >= AG_CHAN_MAX)
                {
                    error("RFC channel too big\n");
                    return -1;
                }
               
                info("btl_if_bind : [rfc] rc chan %d, bd[%x:%x]\n", addr->rc_channel, (unsigned char)addr->rc_bdaddr.b[4], (unsigned char)addr->rc_bdaddr.b[5]);
  
                result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_AG, addr->rc_channel));

                if (result<0)
                    return result;
                
                /* store rf chan for this wsock so we can disconnet it later */
                ws->rf_chan = addr->rc_channel;

                /* bind here as instead of in listen call as we we have the addr params available */
                BTLIF_AG_ListenReq(ctrl_hdl, (BD_ADDR *)&addr->rc_bdaddr, addr->rc_channel);
            }
            break;

        case SUB_SCO:
            {
                //struct sockaddr_sco *addr = (struct sockaddr_sco*)my_addr; 
                
                info("btl_if_bind : [sco]\n");

                /* assume max 1 SCO link active, use subport 0 */
                result = wrp_sock_bind(ws, s, btl_ifc_get_srvaddr(), wrp_getport(SUB_SCO, 0));

                if (result<0)
                    return result;       

                BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_LISTEN_REQ, NULL, 0);            
            }
            break;            
        default :
            error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
            result = -1;
            break;

    }
    return result;
}



int btl_if_listen(int s, int backlog)
{
    t_wsock* ws;
    int result;

    CHECK_BLZ_INIT();

    //debug("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_listen : no wsock found\n");
        return -1;   
    }
    
    debug("btl_if_listen : fd %d [%s]\n", s, ws->name);

    /* already sent listen request in bind call */

    /* no translation needed */
    result = wrp_sock_listen(ws, s); 

    //wsactive_add(ws, s);

    return result;
}



int btl_if_accept(int s, struct sockaddr *my_addr, socklen_t *addrlen)
{
    int sock_new;
    t_wsock* ws;

    CHECK_BLZ_INIT();

    //info("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_accept : no wsock found\n");
        return -1;   
    }

    switch(ws->sub)
    {
      case SUB_AG:
          {
              info("btl_if_accept : [rfc], copy params\n");
              memcpy((void*)my_addr, (void*)&rc_addr_stored, sizeof(struct sockaddr_rc)); 
          }
          break;

      case SUB_SCO:
          {
              info("btl_if_accept : [sco]\n");           
          }  
          break;            
      default :
          error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
          return -1;

    }

    sock_new = wrp_sock_accept(ws, s);  

    if (sock_new < 0)
        return sock_new;

    /* remember listen fd  (done in wrp functions....0 */

    //ws->listen_fd = ws->wsock;    
    //ws->wsock = sock_new;
    
#if 0
    /* clone new socket */
    ws_new = wrp_clone(ws);

    /* map this to the new socket fd */
    ws_new->wsock = sock_new;

    /* add to active list */
    wsactive_add(ws_new);
#endif

    return sock_new;
}



int btl_if_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    CHECK_BLZ_INIT();
    info("btl_if_getsockopt -- not implemented\n");
    return 0;
}

int btl_if_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    CHECK_BLZ_INIT();

    debug("btl_if_setsockopt level:%d, optname:%d\n", level, optname);

    if (((level != SOL_RFCOMM) && (level != SOL_SOCKET)) )
        return -1;

    if (optname == RFCOMM_LM)
    {
        info("configure rfcomm lm mode\n");

        // fixme -- send across params over ctrl channel

    }
    return 0;
}


int btl_if_fcntl(int s, int cmd, long arg)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();

    //info("%s (%d) cmd 0x%x, arg 0x%x\n", __FUNCTION__, s, cmd, (int)arg);

    ws = wrp_find_wsock(s);

    //DUMP_WSOCK("", ws);

    if (!ws) {
        error("btl_if_fcntl : no wsock found\n");
        return -1;   
    }    

    /* make sure we catch a non blocking configuration to emulate the blz sockets */
    if (cmd == F_SETFL)
    {
        ws->flags = arg;
    }

    //return 0;
    /* also let this pass onto sysfunc */
    return fcntl(s, cmd, arg);
}


int btl_if_close (int s)
{
    t_wsock* ws;

    CHECK_BLZ_INIT();
    
    info("%s (%d)\n", __FUNCTION__, s);

    ws = wrp_find_wsock(s);

    DUMP_WSOCK("", ws);

    if (!ws)
    {
        error("btl_if_close : no wsock found\n");
        return -1;   
    }

    /* send disconnect req if socket matches active wsock data handle s*/
    if (ws->wsock == s)
    {
        switch(ws->sub)
        {
            case SUB_AG:
                /* notify server that this subsystem is closed */
                BTLIF_AG_Disconnect(ctrl_hdl, ws->rf_chan);   
                break;
                
            case SUB_SCO:
                /* notify server that this subsystem is closed */
                BTL_IFC_CtrlSend(ctrl_hdl, SUB_SCO, BTLIF_DISCONNECT_REQ, NULL, 0);  
                break;

            default:
                error("%s: invalid subsystem %d\n", __FUNCTION__, ws->sub);
                return -1;
        }         
    }

    /* clear any disconnect pending flag */
    if(ws->disconnect_pending)
        debug("clear disconnect pending\n");
    
    ws->disconnect_pending = 0;

    /* only close requested socket, wsock will terminate when both listener and data socket is closed */
    if (wrp_close_s_only(ws, s) == TRUE)
        ws_async_del( ws ); 

    //wrp_close(ws);
    
    /* wsocket re-initialized when fully closed */      

    return 0;
}























// defined when compiling together with target jni

#ifndef JNI_TARGET /////////////////////////////////////////////////////////////////////////////////////////


// TEST OF JNI INTERFACE FUNCTIONS

 
// bluez wrapper remaps sco and rfcomm sockets to btl-if
#include "blz_wrapper.h"


#define HAVE_BLUETOOTH

#define JNI_FALSE FALSE
#define JNI_TRUE TRUE

#define LOGV(format, ...)  fprintf (stdout, format, ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, "\t\t"format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, "\t\t"format"\n", ## __VA_ARGS__)

typedef char* jstring;
typedef int JNIEnv;
typedef int jobject;
typedef int jint;

#define jboolean BOOLEAN

#define true (1)
#define false (0)
#define bool BOOLEAN
#define BTADDR_SIZE 6


typedef char u8;


#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH	31
#define PF_BLUETOOTH	AF_BLUETOOTH
#endif

#define BTPROTO_L2CAP	0
#define BTPROTO_HCI	1
#define BTPROTO_SCO	2
#define BTPROTO_RFCOMM	3
#define BTPROTO_BNEP	4
#define BTPROTO_CMTP	5
#define BTPROTO_HIDP	6
#define BTPROTO_AVDTP	7

#define SOL_HCI		0
#define SOL_L2CAP	6
#define SOL_SCO		17
#define SOL_RFCOMM	18



void get_bdaddr(const char *str, bdaddr_t *ba) {
    char *d = ((char *)ba) + 5, *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d-- = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(ba, 0, sizeof(bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}

BOOLEAN debug_no_encrypt(void)
{
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////

#define JNI_TEST_INITIATOR  

#ifdef JNI_TEST_INITIATOR

typedef struct {
    jstring address;
    const char *c_address;
    int rfcomm_channel;
    int last_read_err;
    int rfcomm_sock;
    int rfcomm_connected; // -1 in progress, 0 not connected, 1 connected
    int rfcomm_sock_flags;
} native_data_t;

native_data_t native = {0};

void set_native_addr(char *bd_string)
{
  printf("set bd [%s]\n", bd_string);
  native.c_address = bd_string;
}

void init_native_data(void)
{
    if (native.c_address == (jstring)"")
    {
        // default
        native.c_address = "c7:22:ab:f3:1f:00"; // 
        //native.c_address = "e0:55:b0:a1:47:20"; // ruby
        // native.c_address = "2a:9c:ca:38:16:00";
    }

    info("Using bd address [%s]\n", native.c_address);
    
    native.last_read_err = 0;
    native.rfcomm_channel = 3;
    native.rfcomm_connected = 0;
    native.rfcomm_sock = -1;
    native.rfcomm_sock_flags = 0;
}
native_data_t* get_native_data(JNIEnv *env, jobject obj)
{
    return &native;
}



static const char* get_line(int fd, char *buf, int len, int timeout_ms,
                            int *err) {
    char *bufit=buf;
    //int fd_flags = fcntl(fd, F_GETFL, 0);
    struct pollfd pfd;

again:
    *bufit = 0;
    pfd.fd = fd;
    pfd.events = POLLIN;
    *err = errno = 0;
    int ret = poll(&pfd, 1, timeout_ms);

    //LOGI("poll returned %d\n", ret);
    
    if (ret < 0) {
        LOGE("poll() error\n");
        *err = errno;
        return NULL;
    }
    if (ret == 0) {
        return NULL;
    }

    if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
        debug("RFCOMM poll() returned  success (%d), "
             "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
        errno = EIO;
        *err = errno;
        return NULL;
    }

    while ((int)(bufit - buf) < len)
    {
        errno = 0;
        int rc = read(fd, bufit, 1);

        if (rc == 0)
        {
            /* read 0 means link terminated, notify caller */
            errno = EIO;
            *err = errno;
            return NULL;

            break;
        }
        
        if (rc < 0) {
            if (errno == EBUSY) {
                LOGI("read() error %s (%d): repeating read()...",
                     strerror(errno), errno);
                goto again;
            }
            *err = errno;
            LOGE("read() error %s (%d)", strerror(errno), errno);
            return NULL;
        }


        if (*bufit=='\xd') {
            break;
        }

        if (*bufit=='\xa')
            bufit = buf;
        else
            bufit++;
    }

    *bufit = '\x0';

    info("Bluetooth AT recv [%s]\n", buf);

    return buf;
}

static jstring readNative(JNIEnv *env, jobject obj, jint timeout_ms) {
#ifdef HAVE_BLUETOOTH
    {
        native_data_t *nat = get_native_data(env, obj);
        if (nat->rfcomm_connected) {
            char buf[128];
            const char *ret = get_line(nat->rfcomm_sock,
                                       buf, sizeof(buf),
                                       timeout_ms,
                                       &nat->last_read_err);
            return (jstring)ret;
        }
        return NULL;
    }
#else
    return NULL;
#endif
}


static jboolean connectNative(JNIEnv *env, jobject obj)
{
    LOGV("\n\n%s\n", __FUNCTION__);
#ifdef HAVE_BLUETOOTH
    int lm;
    struct sockaddr_rc addr;
    native_data_t *nat = get_native_data(env, obj);

    nat->rfcomm_sock = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    printf("connectNative got socket %d\n\n\n\n", nat->rfcomm_sock);

    if (nat->rfcomm_sock < 0) {
        LOGE("%s: Could not create RFCOMM socket: %s\n", __FUNCTION__,
             strerror(errno));
        return JNI_FALSE;
    }

    if (debug_no_encrypt()) {
        lm = RFCOMM_LM_AUTH;
    } else {
        lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT;
    }

    if (lm && setsockopt(nat->rfcomm_sock, SOL_RFCOMM, RFCOMM_LM, &lm,
                sizeof(lm)) < 0) {
        LOGE("%s: Can't set RFCOMM link mode", __FUNCTION__);
        close(nat->rfcomm_sock);
        return JNI_FALSE;
    }

    memset(&addr, 0, sizeof(struct sockaddr_rc));
    get_bdaddr(nat->c_address, &addr.rc_bdaddr);
    addr.rc_channel = nat->rfcomm_channel;
    addr.rc_family = AF_BLUETOOTH;
    nat->rfcomm_connected = 0;
    while (nat->rfcomm_connected == 0) {
        if (connect(nat->rfcomm_sock, (struct sockaddr *)&addr,
                      sizeof(addr)) < 0) {
            if (errno == EINTR) continue;
            LOGE("%s: connect() failed: %s\n", __FUNCTION__, strerror(errno));
            close(nat->rfcomm_sock);
            nat->rfcomm_sock = -1;
            return JNI_FALSE;
        } else {
            nat->rfcomm_connected = 1;
        }
    }

    LOGE("%s: connect() success: %d\n", __FUNCTION__, nat->rfcomm_sock);   

    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

static jboolean connectAsyncNative(JNIEnv *env, jobject obj) {
    LOGV("\n\n%s\n", __FUNCTION__);

#ifdef HAVE_BLUETOOTH
    struct sockaddr_rc addr;
    native_data_t *nat = get_native_data(env, obj);

    if (nat->rfcomm_connected) {
        LOGV("RFCOMM socket is already connected or connection is in progress.\n");
        return JNI_TRUE;
    }

    if (nat->rfcomm_sock < 0) {
        int lm;

        nat->rfcomm_sock = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        if (nat->rfcomm_sock < 0) {
            LOGE("%s: Could not create RFCOMM socket: %s\n", __FUNCTION__,
                 strerror(errno));
            return JNI_FALSE;
        }

        if (debug_no_encrypt()) {
            lm = RFCOMM_LM_AUTH;
        } else {
            lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT;
        }

        if (lm && setsockopt(nat->rfcomm_sock, SOL_RFCOMM, RFCOMM_LM, &lm,
                    sizeof(lm)) < 0) {
            LOGE("%s: Can't set RFCOMM link mode", __FUNCTION__);
            close(nat->rfcomm_sock);
            return JNI_FALSE;
        }
        LOGI("Created RFCOMM socket fd %d.", nat->rfcomm_sock);
    }

    memset(&addr, 0, sizeof(struct sockaddr_rc));
    get_bdaddr(nat->c_address, &addr.rc_bdaddr);
    addr.rc_channel = nat->rfcomm_channel;
    addr.rc_family = AF_BLUETOOTH;

    
    if (nat->rfcomm_sock_flags >= 0) {
        nat->rfcomm_sock_flags = fcntl(nat->rfcomm_sock, F_GETFL, 0);
        if (fcntl(nat->rfcomm_sock,
                  F_SETFL, nat->rfcomm_sock_flags | O_NONBLOCK) >= 0) {
            int rc;
            nat->rfcomm_connected = 0;
            errno = 0;
            rc = connect(nat->rfcomm_sock,
                        (struct sockaddr *)&addr,
                         sizeof(addr));

            if (rc >= 0) {
                nat->rfcomm_connected = 1;
                LOGI("async connect successful");
                return JNI_TRUE;
            }
            else if (rc < 0) {
                if (errno == EINPROGRESS || errno == EAGAIN)
                {
                    LOGI("async connect is in progress (%s)",
                         strerror(errno));
                    nat->rfcomm_connected = -1;
                    return JNI_TRUE;
                }
                else
                {
                    LOGE("async connect error: %s (%d)", strerror(errno), errno);
                    close(nat->rfcomm_sock);
                    nat->rfcomm_sock = -1;
                    return JNI_FALSE;
                }
            }
        } // fcntl(nat->rfcomm_sock ...)
    } // if (nat->rfcomm_sock_flags >= 0)
#endif
    return JNI_FALSE;
}

static jint waitForAsyncConnectNative(JNIEnv *env, jobject obj,
                                           jint timeout_ms) {

#ifdef HAVE_BLUETOOTH
//    struct sockaddr_rc addr;
    native_data_t *nat = get_native_data(env, obj);

    LOGV("\n\n%s   rfcomm_connected %d\n", __FUNCTION__, nat->rfcomm_connected);


    //env->SetIntField(obj, field_mTimeoutRemainingMs, timeout_ms);

    if (nat->rfcomm_connected > 0) {
        LOGI("RFCOMM is already connected!");
        return 1;
    }

    if (nat->rfcomm_sock >= 0 && nat->rfcomm_connected == 0) {
        LOGI("Re-opening RFCOMM socket.");
        close(nat->rfcomm_sock);
        nat->rfcomm_sock = -1;
    }
    if (JNI_FALSE == connectAsyncNative(env, obj)) {
        LOGI("Failed to re-open RFCOMM socket!");
        return -1;
    }

    if (nat->rfcomm_sock >= 0) {
        /* Do an asynchronous select() */
        int n;
        fd_set rset, wset;
        struct timeval to;

        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(nat->rfcomm_sock, &rset);
        FD_SET(nat->rfcomm_sock, &wset);
        if (timeout_ms >= 0) {
            to.tv_sec = timeout_ms / 1000;
            to.tv_usec = 1000 * (timeout_ms % 1000);
        }
        n = select(nat->rfcomm_sock + 1,
                   &rset,
                   &wset,
                   NULL,
                   (timeout_ms < 0 ? NULL : &to));

        //if (timeout_ms > 0) {
          //  jint remaining = to.tv_sec*1000 + to.tv_usec/1000;
            //LOGV("Remaining time %ldms", (long)remaining);
            //env->SetIntField(obj, field_mTimeoutRemainingMs,
              //               remaining);
       // }

        LOGV("select() returned %d.\n", n);


        if (n <= 0) {
            if (n < 0)  {
                LOGE("select() on RFCOMM socket: %s (%d)\n",
                     strerror(errno),
                     errno);
                return -1;
            }
            return 0;
        }
        /* n must be equal to 1 and either rset or wset must have the
           file descriptor set. */

        if (FD_ISSET(nat->rfcomm_sock, &rset) ||
            FD_ISSET(nat->rfcomm_sock, &wset))
        {
            /* A trial async read() will tell us if everything is OK. */
            {
                char ch;
                errno = 0;

                printf("trying to read..\n");
                
                int nr = read(nat->rfcomm_sock, &ch, 1);

                printf("read %d\n", nr);
                /* It should be that nr != 1 because we just opened a socket
                   and we haven't sent anything over it for the other side to
                   respond... but one can't be paranoid enough.
                */
                if (nr >= 0 || errno != EAGAIN) {
                    LOGE("RFCOMM async connect() error: %s (%d), nr = %d\n",
                         strerror(errno),
                         errno,
                         nr);
                    /* Clear the rfcomm_connected flag to cause this function
                       to re-create the socket and re-attempt the connect()
                       the next time it is called.
                    */
                    nat->rfcomm_connected = 0;
                    /* Restore the blocking properties of the socket. */
                    fcntl(nat->rfcomm_sock, F_SETFL, nat->rfcomm_sock_flags);
                    close(nat->rfcomm_sock);
                    nat->rfcomm_sock = -1;
                    return -1;
                }
            }
            /* Restore the blocking properties of the socket. */
            fcntl(nat->rfcomm_sock, F_SETFL, nat->rfcomm_sock_flags);
            LOGI("Successful RFCOMM socket connect.");
            nat->rfcomm_connected = 1;
            return 1;
        }
    }
    else LOGE("RFCOMM socket file descriptor %d is bad!",
              nat->rfcomm_sock);
#endif
    return -1;
}

static void disconnectNative(JNIEnv *env, jobject obj) {
    LOGV("%s\n", __FUNCTION__);
#ifdef HAVE_BLUETOOTH
    native_data_t *nat = get_native_data(env, obj);
    if (nat->rfcomm_sock >= 0) {
        close(nat->rfcomm_sock);
        nat->rfcomm_sock = -1;
        nat->rfcomm_connected = 0;
    }
#endif
}

pthread_t jni_thread_id;


static void jni_read_write_thread(void *p)
{
    int fd = (int)p;
    int n;
    int x = 20;
    int slen;
    char jnibuf[1024];
    native_data_t *nat = get_native_data(NULL, (jobject)0);

    sleep(2);

    info("jni_read_write_thread starting... (fd %d)\n", fd);

#if 1
    while (x--)
    {
        /* send some data */
        //write(fd, "AT-TEST", strlen("AT-TEST"));
        printf("readNative\n");
        readNative(NULL, (jobject)0, 2000);

        if (nat->last_read_err)
        {
                printf("Link disconnected!!!\n");
                return;
        }

        
        sleep(1);
    }
    
    x=10;
#endif

    while (x--)
    {
#if 1
        slen = sprintf(jnibuf, "%d - android app sending some data", x);

        n = write(fd, jnibuf, slen);
        if (n<0)
            perror("write");
        printf("<--- %d bytes [%s]\n", n, jnibuf);
#endif

        n = read(fd, jnibuf, 10);
        jnibuf[n] = 0;

        if (n<0)
            perror("read");
        
        printf("---> %d bytes [%s]\n", n, jnibuf);

        if (n<=0)
        {
            return;
        }
        
        //sleep(1);
    }


    sleep(10);
}

void jni_test(void)
{
    JNIEnv *env = NULL; // dummy
    jobject obj = {0}; // dummy
    
    blz_wrapper_init();

    if (btl_ifc_main_running() == FALSE)
        return;

    blz_init_subsystem(SUB_AG);
    
    /* now test some JNI functions */

    init_native_data();

    if (connectNative(env, obj) == JNI_FALSE)
        return;

    /* create AT parser test thread */
    if (pthread_create(&jni_thread_id, NULL, 
                  (void*)jni_read_write_thread, (void*)native.rfcomm_sock)!=0)
       perror("pthread_create");

    //printf("wait thread start...\n");
    //sleep(10);

    /* wait for thread to terminate */
    pthread_join(jni_thread_id, NULL);

    sleep(5);
    printf("Disconnecting data channel...\n");

    disconnectNative(NULL, 0);

    /* wait for response to come back */
    sleep(3);

    
}

void jni_test_async(void)
{
   JNIEnv *env = NULL; // dummy
   jobject obj = {0}; // dummy

   blz_wrapper_init();
   
   if (blz_init_subsystem(SUB_AG)!=BTL_IF_SUCCESS)
       return;

   /* now test some JNI functions */

   init_native_data();

   connectAsyncNative(env, obj);

   printf("wait for async...\n\n\n");
   while (waitForAsyncConnectNative(env, obj, 6000) == JNI_FALSE)
   {
      sleep(1);
   }

   /* create AT parser test thread */
   if (pthread_create(&jni_thread_id, NULL, 
                  (void*)jni_read_write_thread, (void*)native.rfcomm_sock)!=0)
       perror("pthread_create");

   /* wait for thread to terminate */
   pthread_join(jni_thread_id, NULL);

   sleep(10);

   disconnectNative(env, obj);

   sleep(3);
}


// bluez wrapper remaps sco and rfcomm sockets to btl-if
#include "blz_wrapper.h"


typedef struct {
    int hcidev;
    int hf_ag_rfcomm_channel;
    int hs_ag_rfcomm_channel;
    int hf_ag_rfcomm_sock;
    int hs_ag_rfcomm_sock;
} ag_native_data_t;

typedef int jfieldID;


ag_native_data_t ag_native;

void init_ag_native_data(void)
{
    ag_native.hcidev = 0;
    ag_native.hf_ag_rfcomm_channel = 3;
    ag_native.hs_ag_rfcomm_channel = 7;
    ag_native.hf_ag_rfcomm_sock = 0;
    ag_native.hs_ag_rfcomm_sock = 0;
}

static jfieldID field_mConnectingHeadsetAddress;
static jfieldID field_mConnectingHeadsetRfcommChannel; /* -1 when not connected */
static jfieldID field_mConnectingHeadsetSocketFd;

static jfieldID field_mConnectingHandsfreeAddress;
static jfieldID field_mConnectingHandsfreeRfcommChannel; /* -1 when not connected */
static jfieldID field_mConnectingHandsfreeSocketFd;


static int setup_listening_socket(int dev, int channel);

void get_bdaddr_as_string(const bdaddr_t *ba, char *str) {
    const uint8_t *b = (const uint8_t *)ba;
    sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
            b[5], b[4], b[3], b[2], b[1], b[0]);
}


ag_native_data_t* get_ag_native_data(JNIEnv *env, jobject obj)
{
    return &ag_native;
}

#ifdef HAVE_BLUETOOTH

#if USE_ACCEPT_DIRECTLY==0
static int set_nb(int sk, bool nb) {
    int flags = fcntl(sk, F_GETFL, 0);
    if (flags < 0) {
        LOGE("Can't get socket flags with fcntl(): %s (%d)", 
             strerror(errno), errno);
        close(sk);
        return -1;
    }
    flags &= ~O_NONBLOCK;
    if (nb) flags |= O_NONBLOCK;
    int status = fcntl(sk, F_SETFL, flags);
    if (status < 0) {
        LOGE("Can't set socket to nonblocking mode with fcntl(): %s (%d)",
             strerror(errno), errno);
        close(sk);
        return -1;
    }
    return 0;
}
#endif /*USE_ACCEPT_DIRECTLY==0*/

static int do_accept(JNIEnv* env, jobject object, int ag_fd,
                     jfieldID out_fd,
                     jfieldID out_address,
                     jfieldID out_channel) 

{
    char addr[BTADDR_SIZE];

#if USE_ACCEPT_DIRECTLY==0
    if (set_nb(ag_fd, true) < 0)
        return -1;
#endif

    struct sockaddr_rc raddr;
    int alen = sizeof(raddr);
    int nsk = accept(ag_fd, (struct sockaddr *) &raddr, (socklen_t *)&alen);

    printf("nsk %d\n", nsk);
    
    if (nsk < 0) 
    {
        LOGE("Error on accept from socket fd %d: %s (%d)\n.",
             ag_fd,
             strerror(errno),
             errno);
#if USE_ACCEPT_DIRECTLY==0
        set_nb(ag_fd, false);
#endif
        return -1;
    }

    // test
    field_mConnectingHandsfreeSocketFd = nsk;

    //env->SetIntField(object, out_fd, nsk);
    //env->SetIntField(object, out_channel, raddr.rc_channel);

    get_bdaddr_as_string(&raddr.rc_bdaddr, addr);
    //env->SetObjectField(object, out_address, env->NewStringUTF(addr));

    printf("Successful accept() on AG socket %d: new socket %d, address %s, RFCOMM channel %d\n\n",
         ag_fd,
         nsk,
         addr,
         raddr.rc_channel);
#if USE_ACCEPT_DIRECTLY==0
    set_nb(ag_fd, false);
#endif
    return 0;
}

#if USE_SELECT
static inline int on_accept_set_fields(JNIEnv* env, jobject object,
                                       fd_set *rset, int ag_fd,
                                       jfieldID out_fd,
                                       jfieldID out_address,
                                       jfieldID out_channel) {

    //env->SetIntField(object, out_channel, -1);

    if (ag_fd >= 0 && FD_ISSET(ag_fd, &rset)) {
        return do_accept(env, object, ag_fd,
                         out_fd, out_address, out_channel);
    }
    else {
        LOGI("fd = %d, FD_ISSET() = %d",
             ag_fd,
             FD_ISSET(ag_fd, &rset));
        if (ag_fd >= 0 && !FD_ISSET(ag_fd, &rset)) {
            LOGE("WTF???");
            return -1;
        }
    }

    return 0;
}
#endif
#endif /* HAVE_BLUETOOTH */

static jboolean waitForHandsfreeConnectNative(JNIEnv* env, jobject object,
                                              jint timeout_ms) {
    LOGV("%s\n", __FUNCTION__);
#ifdef HAVE_BLUETOOTH

    //env->SetIntField(object, field_mTimeoutRemainingMs, timeout_ms);

    int n = 0;
    ag_native_data_t *nat = get_ag_native_data(env, object);
#if USE_ACCEPT_DIRECTLY
    if (nat->hf_ag_rfcomm_channel > 0) {
        LOGI("Setting HF AG server socket to RFCOMM port %d!", 
             nat->hf_ag_rfcomm_channel);
        struct timeval tv;
        int len = sizeof(tv);
        if (getsockopt(nat->hf_ag_rfcomm_channel, 
                       SOL_SOCKET, SO_RCVTIMEO, &tv, &len) < 0) {
            LOGE("getsockopt(%d, SOL_SOCKET, SO_RCVTIMEO): %s (%d)",
                 nat->hf_ag_rfcomm_channel,
                 strerror(errno),
                 errno);
            return JNI_FALSE;
        }
        LOGI("Current HF AG server socket RCVTIMEO is (%d(s), %d(us))!", 
             (int)tv.tv_sec, (int)tv.tv_usec);
        if (timeout_ms >= 0) {
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = 1000 * (timeout_ms % 1000);
            if (setsockopt(nat->hf_ag_rfcomm_channel, 
                           SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                LOGE("setsockopt(%d, SOL_SOCKET, SO_RCVTIMEO): %s (%d)",
                     nat->hf_ag_rfcomm_channel,
                     strerror(errno),
                     errno);
                return JNI_FALSE;
            }
            LOGI("Changed HF AG server socket RCVTIMEO to (%d(s), %d(us))!", 
                 (int)tv.tv_sec, (int)tv.tv_usec);
        }

        if (!do_accept(env, object, nat->hf_ag_rfcomm_sock, 
                       field_mConnectingHandsfreeSocketFd,
                       field_mConnectingHandsfreeAddress,
                       field_mConnectingHandsfreeRfcommChannel))
        {
            //env->SetIntField(object, field_mTimeoutRemainingMs, 0);
            return JNI_TRUE;
        }
        return JNI_FALSE;
    }
#else
#if USE_SELECT
    fd_set rset;
    FD_ZERO(&rset);
    int cnt = 0;
    if (nat->hf_ag_rfcomm_channel > 0) {
        LOGI("Setting HF AG server socket to RFCOMM port %d!", 
             nat->hf_ag_rfcomm_channel);
        cnt++;
        FD_SET(nat->hf_ag_rfcomm_sock, &rset);
    }
    if (nat->hs_ag_rfcomm_channel > 0) {
        LOGI("Setting HS AG server socket to RFCOMM port %d!", 
             nat->hs_ag_rfcomm_channel);
        cnt++;
        FD_SET(nat->hs_ag_rfcomm_sock, &rset);
    }
    if (cnt == 0) {
        LOGE("Neither HF nor HS listening sockets are open!");
        return JNI_FALSE;
    }

    struct timeval to;
    if (timeout_ms >= 0) {
        to.tv_sec = timeout_ms / 1000;
        to.tv_usec = 1000 * (timeout_ms % 1000);
    }
    n = select(MAX(nat->hf_ag_rfcomm_sock, 
                       nat->hs_ag_rfcomm_sock) + 1,
                   &rset,
                   NULL,
                   NULL,
                   (timeout_ms < 0 ? NULL : &to));
    if (timeout_ms > 0) {
        jint remaining = to.tv_sec*1000 + to.tv_usec/1000;
        LOGI("Remaining time %ldms", (long)remaining);
       // env->SetIntField(object, field_mTimeoutRemainingMs,
       //                  remaining);
    }

    LOGI("listening select() returned %d", n);

    if (n <= 0) {
        if (n < 0)  {
            LOGE("listening select() on RFCOMM sockets: %s (%d)",
                 strerror(errno),
                 errno);
        }
        return JNI_FALSE;
    }

    n = on_accept_set_fields(env, object, 
                             &rset, nat->hf_ag_rfcomm_sock,
                             field_mConnectingHandsfreeSocketFd,
                             field_mConnectingHandsfreeAddress,
                             field_mConnectingHandsfreeRfcommChannel);

    n += on_accept_set_fields(env, object,
                              &rset, nat->hs_ag_rfcomm_sock,
                              field_mConnectingHeadsetSocketFd,
                              field_mConnectingHeadsetAddress,
                              field_mConnectingHeadsetRfcommChannel);

    return !n ? JNI_TRUE : JNI_FALSE;
#else
    struct pollfd fds[2];
    int cnt = 0;
    if (nat->hf_ag_rfcomm_channel > 0) {
        LOGI("Setting HF AG server socket %d to RFCOMM port %d!", 
             nat->hf_ag_rfcomm_sock,
             nat->hf_ag_rfcomm_channel);
        fds[cnt].fd = nat->hf_ag_rfcomm_sock;
        fds[cnt].events = POLLIN | POLLPRI | POLLOUT | POLLERR;
        cnt++;
    }

    if (nat->hs_ag_rfcomm_channel > 0) {
        LOGI("Setting HS AG server socket %d to RFCOMM port %d!", 
             nat->hs_ag_rfcomm_sock, 
             nat->hs_ag_rfcomm_channel);
        fds[cnt].fd = nat->hs_ag_rfcomm_sock;
        fds[cnt].events = POLLIN | POLLPRI | POLLOUT | POLLERR;
        cnt++;
    }
    if (cnt == 0) {
        LOGE("Neither HF nor HS listening sockets are open!");
        return JNI_FALSE;
    }

    printf("polling file descriptors...\n");
    
    n = poll(fds, cnt, timeout_ms);
    if (n <= 0) {
        if (n < 0)  {
            LOGE("listening poll() on RFCOMM sockets: %s (%d)",
                 strerror(errno),
                 errno);
        }
        else {
           // env->SetIntField(object, field_mTimeoutRemainingMs, 0);
//            LOGI("listening poll() on RFCOMM socket timed out");
        }
        return JNI_FALSE;
    }

    LOGI("listening poll() on RFCOMM socket returned %d", n);
    int err = 0;
    for (cnt = 0; cnt < (int)(sizeof(fds)/sizeof(fds[0])); cnt++) {
        //LOGI("Poll on fd %d revent = %d.", fds[cnt].fd, fds[cnt].revents);
        if (fds[cnt].fd == nat->hf_ag_rfcomm_sock) {
            if (fds[cnt].revents & (POLLIN | POLLPRI | POLLOUT)) {
                LOGI("Accepting HF connection.\n\n\n");
                err += do_accept(env, object, fds[cnt].fd, 
                               field_mConnectingHandsfreeSocketFd,
                               field_mConnectingHandsfreeAddress,
                               field_mConnectingHandsfreeRfcommChannel);
                n--;
            }
        }
        else if (fds[cnt].fd == nat->hs_ag_rfcomm_sock) {
            if (fds[cnt].revents & (POLLIN | POLLPRI | POLLOUT)) {
                LOGI("Accepting HS connection.\n\n\n");
                err += do_accept(env, object, fds[cnt].fd, 
                               field_mConnectingHeadsetSocketFd,
                               field_mConnectingHeadsetAddress,
                               field_mConnectingHeadsetRfcommChannel);
                n--;
            }
        }
    } /* for */

    if (n != 0) {
        LOGI("Bogus poll(): %d fake pollfd entrie(s)!", n);
        return JNI_FALSE;
    }

    return !err ? JNI_TRUE : JNI_FALSE;
#endif /* USE_SELECT */
#endif /* USE_ACCEPT_DIRECTLY */
#else
    return JNI_FALSE;
#endif /* HAVE_BLUETOOTH */
}

static jboolean setUpListeningSocketsNative(JNIEnv* env, jobject object) {
    LOGV(__FUNCTION__);
#ifdef HAVE_BLUETOOTH
    ag_native_data_t *nat = get_ag_native_data(env, object);

    nat->hf_ag_rfcomm_sock =
        setup_listening_socket(nat->hcidev, nat->hf_ag_rfcomm_channel);
    if (nat->hf_ag_rfcomm_sock < 0)
        return JNI_FALSE;

    nat->hs_ag_rfcomm_sock =
        setup_listening_socket(nat->hcidev, nat->hs_ag_rfcomm_channel);
    if (nat->hs_ag_rfcomm_sock < 0) {
        close(nat->hf_ag_rfcomm_sock);
        nat->hf_ag_rfcomm_sock = -1;
        return JNI_FALSE;
    }

    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif /* HAVE_BLUETOOTH */
}

#ifdef HAVE_BLUETOOTH
static int setup_listening_socket(int dev, int channel) {
    struct sockaddr_rc laddr;
    int sk, lm;

    printf("\n\nSETUP LISTENING SOCKET \n\n\n");

    sk = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sk < 0) {
        LOGE("Can't create RFCOMM socket");
        return -1;
    }

    if (debug_no_encrypt()) {
        lm = RFCOMM_LM_AUTH;
    } else {
        lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT;
    }

	if (lm && setsockopt(sk, SOL_RFCOMM, RFCOMM_LM, &lm, sizeof(lm)) < 0) {
		LOGE("Can't set RFCOMM link mode");
		close(sk);
		return -1;
	}

    laddr.rc_family = AF_BLUETOOTH;
    bacpy(&laddr.rc_bdaddr, BDADDR_ANY);
    laddr.rc_channel = channel;
    
	if (bind(sk, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		LOGE("Can't bind RFCOMM socket");
		close(sk);
		return -1;
	}

    printf("bind done\n");

    listen(sk, 10);
    return sk;
}
#endif /* HAVE_BLUETOOTH */

/*
    private native void tearDownListeningSocketsNative();
*/
static void tearDownListeningSocketsNative(JNIEnv *env, jobject object) {
    LOGV(__FUNCTION__);
#ifdef HAVE_BLUETOOTH
    ag_native_data_t *nat = get_ag_native_data(env, object);

    if (nat->hf_ag_rfcomm_sock > 0) {
        if (close(nat->hf_ag_rfcomm_sock) < 0) {
            LOGE("Could not close HF server socket: %s (%d)\n",
                 strerror(errno), errno);
        }
        nat->hf_ag_rfcomm_sock = -1;
    }
    if (nat->hs_ag_rfcomm_sock > 0) {
        if (close(nat->hs_ag_rfcomm_sock) < 0) {
            LOGE("Could not close HS server socket: %s (%d)\n",
                 strerror(errno), errno);
        }
        nat->hs_ag_rfcomm_sock = -1;
    }
#endif /* HAVE_BLUETOOTH */
}

void jni_test_audiogateway(void)
{
    JNIEnv *env = NULL; // dummy
    jobject obj = {0}; // dummy
    pthread_t jni_thread_id_hfp;
//    pthread_t jni_thread_id_hs;    

    
    
    blz_wrapper_init();

    if (btl_ifc_main_running() == FALSE)
        return;

    blz_init_subsystem(SUB_AG);
    
    /* now test some JNI functions */

    init_ag_native_data();

    if (setUpListeningSocketsNative(env, obj) != JNI_TRUE)
        return;

    if (waitForHandsfreeConnectNative(env, obj, 10000) == JNI_TRUE)
    {
        /* create AT parser test thread */
        if (pthread_create(&jni_thread_id_hfp, NULL, 
                      (void*)jni_read_write_thread, (void*)field_mConnectingHandsfreeSocketFd)!=0)
           perror("pthread_create");

     
#if 0
        /* create AT parser test thread */
        if (pthread_create(&jni_thread_id_hs, NULL, 
                      (void*)jni_read_write_thread, (void*)field_mConnectingHeadsetSocketFd)!=0)
           perror("pthread_create");
#endif
        }

    sleep(15);
    
    tearDownListeningSocketsNative(env, obj);   

    sleep(3);    
}


// moved to blz20
#if 0
void jni_parsecmd(char *cmd)
{    
    if (!strcmp(cmd, "jni"))
    {
       set_native_addr(cmd+4);       
       jni_test();
    }
    else if (!strcmp(cmd, "jniasync"))
    {
       set_native_addr(cmd+10);          
       jni_test_async();
    }
    else if (!strcmp(cmd, "jniag"))
    {
       void jni_test_audiogateway(void);
       jni_test_audiogateway();
    }
}
#endif

#endif




    
#endif


