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
 *   Modified, March 17, 1997 by Tuyen Nguyen for Rhapsody.
 */

/****************************************************************/
/*								*/
/*								*/
/*				S I P				*/
/*			System Information Protocol		*/
/*								*/
/*								*/
/****************************************************************/

/* System Information Protocol -- implemented to handle Responder
 * Queries.  The queries are ATP requests, but the ATP responses are faked
 * here in a DDP level handler routine.  The responder socket is always
 * the 1st socket in the dynamic socket range (128) and it is assumed
 * that the node will be registered on that socket.
 * 
 * In A/UX implementation, this implies that /etc/appletalk program will
 * register the node name on socket DDP_SOCKET_1st_DYNAMIC (128).
 */

#include <sysglue.h>

#include <at/appletalk.h>
#include <lap.h>
#include <at/ddp.h>
#include <at/atp.h>

#include <at/at_lap.h>
#include <at_ddp.h>

#define	SIP_SYSINFO_CMD		1
#define	SIP_DATALINK_CMD	6

#define	SIP_GOOD_RESPONSE	0x1
#define	SIP_BAD_RESPONSE	0xff

#define	SIP_DRIVER_VERSION	0x0001
#define	SIP_RESPONDER_VERSION	0x0001

typedef	struct {
	u_char	response;
	u_char	unused;
	u_short	responder_version;
} sip_userbytes_t;

static	void
sip_input(mp, ifID)
gbuf_t	*mp;
at_if_t	 ifID; /* not used */
{
	/* Packets arriving here are actually ATP packets, but since
	 * A/UX only send dummy responses, we're implementing responder as
	 * a DDP handler
	 */
	register at_ddp_t	*ddp;
	register at_atp_t	*atp;
	register gbuf_t		*tmp;
	u_char		*resp;
	sip_userbytes_t	ubytes;

	ddp = (at_ddp_t *)gbuf_rptr(mp);

	/* Make sure the packet we got is an ATP packet */
	if (ddp->type != ATP_DDP_TYPE) {
		gbuf_freem(mp);
		return;
	}
	
	/* assuming that the whole packet is in one contiguous buffer */
	atp = (at_atp_t	*)ddp->data;
	
	switch(UAL_VALUE(atp->user_bytes)) {
	case SIP_SYSINFO_CMD :
		/* Sending a response with "AppleTalk driver version" (u_short)
		 * followed by 14 zeros will pacify the interpoll.
		 * What?  You don't understand what it means to send 14 zeroes?
		 * Tsk, tsk, look up SIP protocol specs for details!!
		 */
		if ((tmp = (gbuf_t *)ddp_growmsg(mp, 16)) == NULL) {
			/* dont have buffers */
			gbuf_freem(mp);
			return;
		}
		if (tmp == mp) 
			/* extra space allocated on the same buffer block */
			resp = atp->data;
		else
			resp = (u_char *)gbuf_rptr(tmp);
		bzero(resp, 16);
		*(u_short *)resp = SIP_DRIVER_VERSION;

		ubytes.response = SIP_GOOD_RESPONSE;
		ubytes.unused = 0;
		ubytes.responder_version = SIP_RESPONDER_VERSION;
		break;
	case SIP_DATALINK_CMD :
		/* In this case, the magic spell is to send 2 zeroes after
		 * the "AppleTalk driver version".
		 */
		if ((tmp = (gbuf_t *)ddp_growmsg(mp, 4)) == NULL) {
			/* dont have buffers */
			gbuf_freem(mp);
			return;
		}
		if (tmp == mp) 
			/* extra space allocated on the same buffer block */
			resp = atp->data;
		else
			resp = (u_char *)gbuf_rptr(tmp);
		bzero(resp, 16);
		*(u_short *)resp = SIP_DRIVER_VERSION;

		ubytes.response = SIP_GOOD_RESPONSE;
		ubytes.unused = 0;
		ubytes.responder_version = SIP_RESPONDER_VERSION;
		break;
	default :
		/* bad request, send a bad command response back */
		ubytes.response = SIP_BAD_RESPONSE;
		ubytes.unused = 0;
		ubytes.responder_version = SIP_RESPONDER_VERSION;
	}

	NET_NET(ddp->dst_net, ddp->src_net);
	ddp->dst_node = ddp->src_node;
	ddp->dst_socket = ddp->src_socket;
	bcopy((caddr_t) &ubytes, (caddr_t) atp->user_bytes, sizeof(ubytes));
	atp->cmd = ATP_CMD_TRESP;
	atp->eom = 1;
	atp->sts = 0;
	atp->bitmap = 0;

	if (ddp_output(&mp, DDP_SOCKET_1st_DYNAMIC, NULL) != 0)
		gbuf_freem(mp);

	return;
}


void	sip_init()
{
	/* Open a "well known" dynamic socket with sip_input as the handler
	 * routine.  
	 */
	ddp_socket_t	socket;


	socket.number = DDP_SOCKET_1st_DYNAMIC;
	socket.flags = DDP_CALL_HANDLER;
	socket.sock_u.handler = sip_input;
	socket.dev = NULL;

	ddp_bind_socket(&socket);
	return;
}	
