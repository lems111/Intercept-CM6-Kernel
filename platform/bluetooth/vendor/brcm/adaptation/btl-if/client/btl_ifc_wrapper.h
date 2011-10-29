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

#ifndef BTL_IFC_WRAPPER_H
#define BTL_IFC_WRAPPER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "btl_if.h"

// fixme - allocate dynamically

/* max simultaneous users of one socket type */
#define SOCKET_USER_MAX 10


typedef struct {
    list_head node; // datapath active handle list
    list_head async_node; // async syscall list (wrapper)
    tBTL_IF_SUBSYSTEM sub; // backref
    int listen_fd;
    int wsock;
    int busy;
    int flags;
    int port;
    char *name;   // backref  tcp addr or local socket name    
    SA_TYPE address;
    char *rx_buf; // dedicated receive buffer
    int rx_buf_size;
    int rx_buf_pending; // if set, buffer has not been received yet
    int rx_flow;

    void *priv;

    /* ag */
    unsigned short rf_chan;

    /* fixme -- use bit flags instead */
    int select_pending;
    int disconnect_pending;
    
    int async_msg_pnd;
    tBTL_PARAMS async_params;
} t_wsock;

void hex_dump(char *msg, void *data, int size, int trunc);
int rx_data(int fd, char *p, int len);

void wrp_init(void);
void wrp_wsock_init(t_wsock *ws);
t_wsock* wrp_find_wsock(int s);
t_wsock* wrp_find_fdset(fd_set *fds);
t_wsock* wrp_find_wsock_by_rfhdl(unsigned short rf_chan, BOOLEAN is_listener);

void wsactive_init(void);
void wsactive_add(t_wsock *ws, int s);
void wsactive_del(t_wsock *ws, int s);
BOOLEAN ws_in_active_list(t_wsock *ws);
int wrp_remove_active_set(int s);
int wrp_add_to_active_set(int s);

void wrp_setup_rxbuf(t_wsock *ws, char *p, int size);
void wrp_setup_rxflow(t_wsock *ws, BOOLEAN flow_on);

t_wsock* wrp_wsock_create(int sub);
int wrp_sock_create(int sub);
t_wsock* wrp_clone(t_wsock *ws);
int wrp_sock_bind(t_wsock* ws, int s, char *name, int port);
int wrp_sock_accept(t_wsock* ws, int s);
int wrp_sock_listen(t_wsock* ws, int s);
int wrp_sock_listen_bl(t_wsock* ws, int s, int backlog);

int wrp_sock_connect(t_wsock* ws, int s, char *name, int port);
int wrp_check_active_handles(void);
void wrp_close_sub_all(int sub);
void wrp_close_full(t_wsock *ws);
BOOLEAN wrp_close_s_only(t_wsock *ws, int s);

int wrp_getport(tSUB sub, int sub_port);



#endif

