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
 *	Copyright (c) 1997-1998 Apple Computer, Inc.
 *	All Rights Reserved.
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
 *	The copyright notice above does not evidence any actual or
 *	intended publication of such source code.
 */

#include <sysglue.h> 
#include <at/appletalk.h>
#include <lap.h>
#include <at/ep.h>
#include <at/ddp.h>
#include <at_ddp.h>
#include <routing_tables.h>
#include <at_snmp.h>

extern snmpStats_t	snmpStats;

/****************************************************************/
/*								*/
/*								*/
/*			Echo Protocol				*/
/*								*/
/*								*/
/****************************************************************/

static	void	ep_input (mp, ifID)
gbuf_t	*mp;
register at_if_t        *ifID;
{
	register at_ddp_t	*ddp;

	snmpStats.ec_echoReq++;
	ddp = (at_ddp_t *)gbuf_rptr(mp);

		/* ep packets that have a source broadcast can cause
         * possible broadcast storms, prevent that here
         */
	if ( NET_VALUE(ddp->src_net) == 0 || ddp->src_node == 255) {
		gbuf_freem(mp);
		return;
	}
	
	/*
	 * Check if this AEP message is for us or need to be forwarded
	 */
	if (!ROUTING_MODE || 
	(NET_VALUE(ifID->ifThisNode.atalk_net) == NET_VALUE(ddp->dst_net))
	  && (ifID->ifThisNode.atalk_node == ddp->dst_node)) {

		dPrintf(D_M_AEP, D_L_INFO, ("aep_input: received for this port from %d:%d\n",
			NET_VALUE(ddp->src_net), ddp->src_node));

		if (ddp->type == EP_DDP_TYPE && 
		    ddp->data[0] == EP_REQUEST) {
			ddp->data[0] = EP_REPLY;
			NET_NET(ddp->dst_net, ddp->src_net);
			ddp->dst_node = ddp->src_node;
			ddp->dst_socket = ddp->src_socket;
			/* send the packet out.... */
			snmpStats.ec_echoReply++;
			if (ddp_output(&mp, (at_socket)EP_SOCKET, NULL) != 0)
				gbuf_freem(mp);
		} else
			gbuf_freem(mp);
	}
	else {
		dPrintf(D_M_AEP, D_L_INFO,
			 ("aep_input: calling routing needed  from %d:%d to %d:%d\n",
			NET_VALUE(ddp->src_net), ddp->src_node, NET_VALUE(ddp->dst_net),
			ddp->dst_node));
		routing_needed(mp, ifID, TRUE);
	}

	return;
}

int	ep_init()
{
	return ((int)ep_input);
}
