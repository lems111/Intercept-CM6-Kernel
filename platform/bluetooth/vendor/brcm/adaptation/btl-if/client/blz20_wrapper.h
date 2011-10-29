
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
 
#ifndef BLZ20_WRAPPER_H
#define BLZ20_WRAPPER_H

#undef socket
#define socket blz20_wrp_socket

#undef connect
#define connect blz20_wrp_connect

#undef bind 
#define bind blz20_wrp_bind

#undef listen 
#define listen blz20_wrp_listen

#undef accept 
#define accept blz20_wrp_accept

#undef close
#define close blz20_wrp_close

#undef shutdown
#define shutdown blz20_wrp_shutdown

#undef setsockopt
#define setsockopt blz20_wrp_setsockopt

#undef getsockopt
#define getsockopt blz20_wrp_getsockopt

#undef fcntl
#define fcntl blz20_wrp_fcntl

#undef poll
#define poll blz20_wrp_poll

#undef read
#define read blz20_wrp_read

#undef write
#define write blz20_wrp_write

#undef select
#define select blz20_wrp_select

#undef strerror
#define strerror blz20_strerror

/*******************************************************************************/
/* 
  * NOTE : before any of these wrappers are used blz wrapper needs 
  *            to be initialized with below function call 
  */


/* called once per application process */
extern int blz20_wrp_init(void);

/********************************************************************************/

extern int blz20_wrp_socket(int socket_family, int socket_type, int protocol); 
extern int blz20_wrp_bind(int s, const struct sockaddr *my_addr, socklen_t addrlen);
extern int blz20_wrp_listen(int s, int backlog);
extern int blz20_wrp_accept(int s, struct sockaddr *addr, socklen_t *addrlen);   
extern int blz20_wrp_connect(int s, const struct sockaddr *addr, socklen_t addrlen);
extern int blz20_wrp_close (int s);
extern int blz20_wrp_shutdown(int s, int how);
extern int blz20_wrp_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
extern int blz20_wrp_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
extern int blz20_wrp_fcntl(int s, int cmd, long arg);
extern int blz20_wrp_read(int fd, void *buf, size_t count);
extern int blz20_wrp_select(int nfds, fd_set *readfds, fd_set *writefds,
                               fd_set *exceptfds, struct timeval *timeout);
extern int blz20_wrp_poll(struct pollfd *fds, nfds_t nfds, int timeout);
extern char *blz20_strerror(int num);

#endif

