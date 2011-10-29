
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
 
#ifndef BLZ_WRAPPER_H
#define BLZ_WRAPPER_H

#undef socket
#define socket btl_if_socket

#undef connect
#define connect btl_if_connect

#undef bind 
#define bind btl_if_bind

#undef listen 
#define listen btl_if_listen

#undef accept 
#define accept btl_if_accept

#undef close
#define close btl_if_close

#undef setsockopt
#define setsockopt btl_if_setsockopt

#undef getsockopt
#define getsockopt btl_if_getsockopt

#undef fcntl
#define fcntl btl_if_fcntl

#undef poll
#define poll btl_if_poll

//#undef read
//#define read btl_if_read

#undef select
#define select btl_if_select


/*******************************************************************************/
/* 
  * NOTE : before any of these wrappers are used blz wrapper needs 
  *            to be initialized with below function call 
  */


/* called once per application process */
extern int blz_wrapper_init(void);

/********************************************************************************/


extern int btl_if_socket(int socket_family, int socket_type, int protocol); 
extern int btl_if_bind(int s, const struct sockaddr *my_addr, socklen_t addrlen);
extern int btl_if_listen(int s, int backlog);
extern int btl_if_accept(int s, struct sockaddr *addr, socklen_t *addrlen);   
extern int btl_if_connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen);
extern int btl_if_close(int s);
extern int btl_if_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
extern int btl_if_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
extern int btl_if_fcntl(int s, int cmd, long arg);
extern int btl_if_read(int fd, void *buf, size_t count);
extern int btl_if_select(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout);
extern int btl_if_poll(struct pollfd *fds, nfds_t nfds, int timeout);


#endif

