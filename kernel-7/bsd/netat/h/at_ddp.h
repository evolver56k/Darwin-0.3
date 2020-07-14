/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


/*
 *	Copyright (c) 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* "@(#)at_ddp.h: 2.0, 1.14; 7/14/92; Copyright 1988-89, Apple Computer, Inc." */
#include <at/at_lap.h>
#include <at/elap.h>

/* DDP short header used internal to the ddp module. */
typedef struct {
	unsigned	unused:6,
			length:10;	/* Datagram length */
	at_socket	dst_socket;	/* Destination socket number.	*/
	at_socket	src_socket;	/* Source socket number.	*/
	u_char		type;		/* Protocol type.		*/
	u_char		data[594];	/* The DataGram buffer.		*/
} at_ddp_short_t;

typedef struct {
	at_socket	number;		/* the number of this socket	*/
	u_char		flags;
	u_char		otChecksum;
	u_char		otType;
	u_long		otPeerAddr;
	union {
		gref_t	*gref;	/* Upstream pointer	*/
		void	(*handler)();	/* Upstream handler routine	*/
	} sock_u;
	void *dev;
} ddp_socket_t;
/* consts for flags above */
#define	DDP_CALL_HANDLER	0x01	/* upstream is pointer to a routine */
#define	DDP_SOCK_UPQ(socket)		ddp_socket[socket].sock_u.gref
#define	DDP_SOCK_HANDLER(socket)	ddp_socket[socket].sock_u.handler

/* Table of minor numbers used */
typedef	struct {
	u_char		flags;		/* See below			   */
	u_char		proto;
	ddp_socket_t	*sock_entry;	/* pointer to the socket table entry */
	int	pid;
} ddp_dev_t;

/* Flags for the device table */
#define DDPF_ALLOCATED	0x1		/* This device is active	   */
#define DDPF_SHUTDOWN	0x2		/* Shutdown occurred; don't use	   */
#define DDPF_OT			0x04	/* OT interface channel */
#define DDPF_LINKSTR	0x08	/* proto interface channel */
#define DDPF_SI			0x10	/* socket interface channel */

#define DDP_FLAGS(Q)		(((ddp_dev_t *)((Q)->info))->flags)
#define	DDP_SOCKENTRY(Q)	(((ddp_dev_t *)((Q)->info))->sock_entry)
#define	DDP_SOCKNUM(Q)		(DDP_SOCKENTRY(Q)->number)
#define DDP_SOCKEVENT(Q)		(DDP_SOCKENTRY(Q)->eventq)

extern	ddp_socket_t ddp_socket[];
extern	at_if_t	*at_ifDefault;

#define	DDP_STARTUP_LOW		0xff00
#define	DDP_STARTUP_HIGH	0xfffe

typedef	struct {
	void **inputQ;
	int  *pidM;
	char  **socketM;
	char  *dbgBits;
} proto_reg_t;

#define FROM_US(ddp)	(NET_EQUAL(ddp->src_net,\
	ifID_table[IFID_HOME]->ifThisNode.atalk_net) && \
	ifID_table[IFID_HOME]->ifThisNode.atalk_node == ddp->src_node)

/* Queue format expected by queue instructions */
typedef struct LIB_QELEM {
  struct LIB_QELEM        *q_forw;
  struct LIB_QELEM        *q_back;
} LIB_QELEM_T;

/* from sys_glue.c */
int ddp_adjmsg(gbuf_t *m, int len);
gbuf_t *ddp_growmsg(gbuf_t  *mp, int len);
void ddp_insque(LIB_QELEM_T *elem, LIB_QELEM_T *prev);
void ddp_remque(LIB_QELEM_T *elem);
	     
/* from ddp.c */
int ddp_add_if(at_if_t *ifID);
int ddp_rem_if(at_if_t *ifID);
int ddp_get_cfg(at_ddp_cfg_t *cfgp, at_socket src_socket);
int ddp_get_stats(at_ddp_stats_t *statsp);
int ddp_bind_socket(ddp_socket_t *socketp);
int ddp_close_socket(ddp_socket_t *socketp);
int ddp_output(gbuf_t **mp, at_socket src_socket, int src_addr_included);
void ddp_input(gbuf_t   *mp, at_if_t *ifID);
int ddp_router_output(
     gbuf_t  *mp,
     at_if_t *ifID,
     int addr_type,
     at_net_al router_net,
     at_node router_node,
     etalk_addr_t *enet_addr);

/* from ddp_proto.c */
int ddp_open(gref_t *gref);
int ddp_close(gref_t *gref);
int ddp_putmsg(gref_t *gref, gbuf_t *mp);
char *ddps_init();
int ddps_shutdown(gref_t *grefInput);
gbuf_t *ddp_compress_msg(gbuf_t *mp);
void ddp_stop(gbuf_t *mioc, gref_t *gref);
	     
/* in ddp_lap.c */
void ddp_bit_reverse();

