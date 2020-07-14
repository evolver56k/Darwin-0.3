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
 *	Copyright (c) 1987, 1988, 1989 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *
 *
 */


#define RESOLVE_DBG			/* define debug globals in debug.h */

#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <llap.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/ep.h>
#include <nbp.h>
#include <rtmp.h>
#include <at/zip.h>
#include <at/at_lap.h>
#include <at_elap.h>
#include <at_ddp.h>
#include <adsp_local.h>
#include <at_ddp_brt.h>
#include <at_zip.h>
#include <routing_tables.h>
#include <atlog.h>
#include <at_snmp.h>
#include <at_aurp.h>

#include "at_kdebug.h"

/* globals */
	/* Queue of LAP interfaces which have registered themselves with DDP */
static	at_if_t	at_ifQueueHd = { &at_ifQueueHd, &at_ifQueueHd };

	/* Pointer to LAP interface which DDP addresses are tied to */
/*at_if_t		*at_ifDefault = 0; */

	/* DDP statistics */
static	at_ddp_stats_t	at_ddp_stats;
snmpStats_t		snmpStats;				/* snmp ddp & echo stats */

	/* DDP input queue */
static	char	*InputQueue = NULL;

	/* DDP Socket Table */
ddp_socket_t	ddp_socket [DDP_SOCKET_LAST + 1];

at_if_t  *ifID_table[IF_TOTAL_MAX];
			/* the table of ifID structures, one per 
			 * interface (not just ethernet)
			 * NOTE: for MH, entry 0 in this table is 
			 *       now defined to be the default I/F
			 */

/* Appletalk state */
extern at_state_t	*at_statep;

	/* routing mode special */
void (*ddp_AURPsendx)();
at_if_t *aurp_ifID = 0;
extern routing_needed();
extern pktsIn,pktsOut;
/* extern char ddp_off_flag; */
int pktsDropped,pktsHome;
atlock_t ddpall_lock;
atlock_t ddpinp_lock;

int ot_protoCnt = 0;
char ot_protoT[256];
char ot_atp_socketM[256];
char ot_adsp_socketM[256];

extern int *atp_pidM;
extern int *adsp_pidM;
extern gref_t *atp_inputQ[];
extern CCB *adsp_inputQ[];

at_if_t *forUs(at_ddp_t *);

extern void
  routershutdown(),
  ddp_brt_shutdown();
void ddp_notify_nbp();

#define MAX_NOTIFY_NBP 16
static struct {
	unsigned char socket;
	unsigned char ddptype;
	unsigned short filler;
	int pid;
} notify_nbp_tbl[MAX_NOTIFY_NBP];

void
Xddp_notify_nbp(id)
int	id;
{
	unsigned char socket;

	if ((socket = notify_nbp_tbl[id].socket) != 0) {
		notify_nbp_tbl[id].socket = 0;
		ddp_notify_nbp(
			socket,
			notify_nbp_tbl[id].pid,
			notify_nbp_tbl[id].ddptype);
	}
}

int
ot_ddp_check_socket(socket, pid)
unsigned char socket;
int pid;
{
	extern ddp_dev_t ddp_devs[];
	int cnt;

	dPrintf(D_M_DDP, D_L_INFO, ("ot_ddp_check_socket: %d\n", socket));
	cnt = (DDP_SOCK_UPQ(socket) == NULL) ? 0 :
		( (ddp_devs[socket].pid == pid) ? 1 : 0 );
	if (ot_protoT[3]) { /* ATP */
		if ((atp_inputQ[socket] != NULL)
		    && (atp_inputQ[socket] != (gref_t *)1)
		    && (atp_pidM[socket] == pid))
			cnt++;
	}
	if (ot_protoT[DDP_ADSP]) {
		if ((adsp_inputQ[socket] != NULL) && (adsp_pidM[socket] == pid))
			cnt++;
	}

	return(cnt);
}

/****************************************************************/
/*								*/
/*								*/
/*			Support Routines			*/
/*								*/
/*								*/
/****************************************************************/

/*
 * Name:
 * 	ddp_checksum
 *
 * Description:
 *	This procedure determines the checksum of an extended DDP datagram.
 *      Add the unsigned bytes into an unsigned 16-bit accumulator.
 *      After each add, rotate the sign bit into the low order bit of
 *      the accumulator. When done, if the checksum is 0, changed into 0xFFFF.
 *
 * Calling sequence:
 *	checksum = ddp_checksum(mp, offset)
 *
 * Parameters:
 *	mp		pointer to the datagram gbuf_t
 *	offset		offset to start at in first gbuf_t block
 *
 * Return value:
 *	The DDP checksum.
 *
 */

static	u_short
ddp_checksum(mp, offset)
register gbuf_t	*mp;
register int	offset;
{
	register u_char	*data;
	register int   	 length;
	register u_short checksum;

	checksum = 0;

	do {
		if (offset >= gbuf_len(mp))
			offset -= gbuf_len(mp);
		else {
			data = ((unsigned char *) gbuf_rptr(mp)) + offset;
			length = gbuf_len(mp) - offset;
			offset = 0;
			/* Portable checksum from 3.0 */
		   	while (length--) {
				checksum += *data++;
				checksum = (checksum & 0x8000) ?
					((checksum << 1) | 1) : (checksum << 1);
			}
		}
	} while ( (mp = gbuf_cont(mp)) );

	if (checksum == 0)
		checksum = 0xffff;

	return(checksum);
}


void ddp_shutdown()
{
	routershutdown();
	ddp_brt_shutdown();
	ddps_shutdown((gref_t *)InputQueue);
	InputQueue = NULL;
	dPrintf(D_M_DDP, D_L_VERBOSE, ("DDP shutdown completed"));
	return;
}


/*
 * Name:
 * 	ddp_init
 *
 * Description:
 *	Initializes DDP.  The streams version allocates a new queue pair to
 *	be used as the input queue for incoming lap datagrams.  The write 
 *	queue of this pair is not used.
 *
 * Return value:
 *
 * Priority Level:
 *	This function is called by the stream head open routines, or
 *	by the lap layer's ONLINE function via ddp_add_if().
 */
static	int
ddp_init()
{
	char		*q;
	char		*ddps_init();
	void		ddp_brt_init();
	at_socket	socket;

	typedef void	(*proc_ptr)();
	proc_ptr	handler;
	extern	void	sip_init();

	dPrintf(D_M_DDP, D_L_STARTUP, ("ddp_init starting\n"));
	/* If using streams, we may get called more than once */
	if (InputQueue == NULL) {
		bzero((char *)notify_nbp_tbl, sizeof(notify_nbp_tbl));
		if ((q = ddps_init()) == NULL)
			return(ENOBUFS);
		else {
			InputQueue = q;
			/* initialize the ddp_socket table */
			bzero(ddp_socket,sizeof(ddp_socket));
			for (socket = DDP_SOCKET_1st_RESERVED; 
				socket <= DDP_SOCKET_LAST; socket++)
				ddp_socket[socket].number = socket;
			bzero(&at_ddp_stats, sizeof(at_ddp_stats_t));
			ddp_brt_init();

			/* Initialize all the protocols implemented in 
			 * kernel
			 */
			handler = (proc_ptr)ep_init();
			ddp_socket[EP_SOCKET].number = EP_SOCKET;
			ddp_socket[EP_SOCKET].flags |= DDP_CALL_HANDLER;
			DDP_SOCK_HANDLER(EP_SOCKET) = handler;

			handler = (proc_ptr)rtmp_init();
			ddp_socket[RTMP_SOCKET].number= RTMP_SOCKET;
			ddp_socket[RTMP_SOCKET].flags|=DDP_CALL_HANDLER;
			DDP_SOCK_HANDLER(RTMP_SOCKET) = handler;

			handler = (proc_ptr)zip_init();
			ddp_socket[ZIP_SOCKET].flags|=DDP_CALL_HANDLER;
			ddp_socket[ZIP_SOCKET].number = ZIP_SOCKET;
			DDP_SOCK_HANDLER(ZIP_SOCKET) = handler;

			handler = (proc_ptr)nbp_init();
			ddp_socket[NBP_SOCKET].number = NBP_SOCKET;
			ddp_socket[NBP_SOCKET].flags |= DDP_CALL_HANDLER;
			DDP_SOCK_HANDLER(NBP_SOCKET) = handler;

			bzero(ifID_table, sizeof(ifID_table));

			sip_init();
		}
	}
	dPrintf(D_M_DDP, D_L_STARTUP, ("ddp_init OK\n"));

	return (0);
}


/*
 * ddp_add_if()
 *
 * Description:
 *	This procedure is called by each LAP interface when it wants to place
 *	itself online.  The LAP interfaces passes in a pointer to its at_if
 *	struct, which is added to DDP's list of active interfaces (at_ifQueueHd).
 *	When DDP wants to transmit a packet, it searches this list for the 
 *	interface to use.
 *	
 *	If AT_IFF_DEFAULT is set, then this interface is to be brought online
 *	as the interface DDP socket addresses are tied to.  Of course there can
 *	be only one default interface; we return an error if it's already set. 
 *
 * Calling Sequence:
 *	ret_status = ddp_add_if(ifID)
 *
 * Formal Parameters:
 *	ifID		pointer to LAP interface's at_if struct.
 *
 * Completion Status:
 *	0		Procedure successfully completed.
 *	EALREADY	This interface is already online, or there is
 *			already a default interface.
 *	ENOBUFS		Cannot allocate input queue
 *
 * Side Effects:
 *	The global at_ifDefault may be set.
 */
int ddp_add_if(ifID)
register at_if_t	*ifID;
{
	int	status;
	int port = -1;

	dPrintf(D_M_DDP, D_L_STARTUP, ("ddp_add_if: called, ifID:0x%x\n", (u_int) ifID));
	/* Initialize some of the ddp data structures and protocol modules
	 * like NBP and RTMP. 
	 */
	if (!(at_statep->flags & AT_ST_DDP_INIT)) {
		
		if ((status = ddp_init()) != 0) {
			ifID->ddpInputQueue = NULL;
			return(status);
		}
		at_statep->flags |= AT_ST_DDP_INIT;
	}
	if (ifID->ifFlags & AT_IFF_DEFAULT) {
		if (ifID_table[IFID_HOME]) {
			return(EEXIST);    /* home port already set */ 
		}
		else {
			port = 0;
			ifID_table[IFID_HOME] = ifID;
		}
	}
	else
		for (port=IFID_HOME+1; port<IF_TOTAL_MAX; port++) {	/* add i/f to port list */
			if (!ifID_table[port]) {
				ifID_table[port] = ifID;
				ifID->ifPort = port;					/* set ddp port # in ifID */
				break;
			}
		}

	if (port == IF_TOTAL_MAX) {							/* no space left */
		ifID->ddpInputQueue = NULL;
		return(ENOMEM);
	}
	dPrintf(D_M_DDP, D_L_STARTUP, ("ddp:adding ifID_table[%d]\n", port));
	
	/* Add this interface to the list of online interfaces */
	ddp_insque((LIB_QELEM_T *)ifID, (LIB_QELEM_T *)at_ifQueueHd.FwdLink);
	
	ifID->ddpInputQueue = InputQueue;
	ATALK_ASSIGN(ifID->ifARouter, 0, 0, 0);
			at_statep->ifs_online++;

	return (0);
}


/*
 * ddp_rem_if()
 *
 * Description:
 *	This procedure is called by each LAP interface when it wants to take
 *	itself offline.  The LAP interfaces passes in a pointer to its at_if
 *	struct; DDP's list of active interfaces (at_ifQueueHd) is searched and
 *	this interface is removed from the list.  DDP can still transmit 
 *	packets as long as this interface is not the default interface; the
 *	sender will just get ENETUNREACH errors when it tries to send to an
 *	interface that went offline.  However, if the default interface is
 *	taken offline, we no longer have a node ID to use as a source address
 * 	and DDP must return ENETDOWN when a caller tries to send a packet.
 *	
 * Calling Sequence:
 *	ret_status = ddp_rem_if(ifID)
 *
 * Formal Parameters:
 *	ifID		pointer to LAP interface's at_if struct.
 *
 * Completion Status:
 *	0		Procedure successfully completed.
 *	EALREADY	This interface is already offline.
 *
 * Side Effects:
 *	The global at_ifDefault may be reset.
 */
int ddp_rem_if(ifID)
register at_if_t	*ifID;
{
	register at_if_t	*ifQueuep;
	int s;

	ATDISABLE(s, ddpall_lock);
	ifQueuep = at_ifQueueHd.FwdLink;
	while (ifQueuep != &at_ifQueueHd) {
		if (ATALK_EQUAL(ifQueuep->ifThisNode, ifID->ifThisNode)){
			trackrouter_rem_if(ifID);
			ifID->ifARouterTimer = 0;
			ddp_remque((LIB_QELEM_T *)ifQueuep);
			if (at_statep->ifs_online == 1) {	/* if last interface */
				ddp_shutdown();
			}
			ifID_table[ifID->ifPort] = NULL;
			at_statep->ifs_online--;
			ATENABLE(s, ddpall_lock);
			return (0);
		}
		ifQueuep = ifQueuep->FwdLink;
	}
	ATENABLE(s, ddpall_lock);
	return (EALREADY);
}

/*
 * The user may have registered an NVE with the NBP on a socket.  When the
 * socket is closed, the NVE should be deleted from NBP's name table.  The
 * user should delete the NVE before the socket is shut down, but there
 * may be circumstances when he can't.  So, whenever a DDP socket is closed,
 * this routine is used to notify NBP of the socket closure.  This would
 * help NBP get rid of all NVE's registered on the socket.
 */
void ddp_notify_nbp(socket, pid, ddptype)
     unsigned char socket;
     int pid;
     unsigned char ddptype;
{
	register int		size;
	register gbuf_t		*mp;
	register at_ddp_t	*ddp;
	register at_nbp_t	*nbp;
	void		ddp_input();
	int id;

	/* *** do this only when the default interface is filled in? *** */
	if (DDP_SOCK_UPQ(NBP_SOCKET) && ifID_table[IFID_HOME]) {
		size = DDP_X_HDR_SIZE + 2; /* for NBP control fields */
			
		if (!(mp = gbuf_alloc(size+8, PRI_HI))) {
			dPrintf(D_M_DDP, D_L_ERROR, ("DDP: TROUBLE out of gbuf"));
			/* *** find an empty entry? *** */
			for (id = 0; id < MAX_NOTIFY_NBP; id++) {
				if (notify_nbp_tbl[id].socket == 0) {
					notify_nbp_tbl[id].socket = socket;
					notify_nbp_tbl[id].ddptype = ddptype;
					notify_nbp_tbl[id].pid = pid;
					atalk_timeout(Xddp_notify_nbp, id, SYS_HZ/10);
					break;
				}
			}
			return;
		}

		gbuf_wset(mp, size);
		ddp = (at_ddp_t *)gbuf_rptr(mp);
		nbp = (at_nbp_t *)ddp->data;

		nbp->control = NBP_CLOSE_NOTE;
		nbp->tuple_count = nbp->at_nbp_id = 0;
		((int *)gbuf_wptr(mp))[0] = (int)pid;
		((int *)gbuf_wptr(mp))[1] = (int)ddptype;
	
		ddp->type = NBP_DDP_TYPE;
		ddp->unused = ddp->hopcount = 0;
		DDPLEN_ASSIGN(ddp, size);
		UAS_ASSIGN(ddp->checksum, 0);
		NET_NET(ddp->src_net, ifID_table[IFID_HOME]->ifThisNode.atalk_net);
		ddp->src_node = ifID_table[IFID_HOME]->ifThisNode.atalk_node;
		ddp->src_socket = (at_socket)socket;
		NET_NET(ddp->dst_net, ifID_table[IFID_HOME]->ifThisNode.atalk_net);
		ddp->dst_node = ifID_table[IFID_HOME]->ifThisNode.atalk_node;
		ddp->dst_socket = NBP_SOCKET;
		ddp_input(mp, 0);  /* ifID 0 implies loopback */
	}
} /* ddp_notify_nbp */


/* This routine shrinks the ddp header from long to short, 
 * It also prepends ALAP header and fills up some of the
 * fields as appropriate.
 */
static	at_ddp_short_t	*ddp_shrink_hdr (mp)
register gbuf_t	*mp;
{
	register at_ddp_t	*ddp;
	register at_ddp_short_t	*ddp_short;
	register at_llap_hdr_t	*llap;
	gbuf_t *newmp;

	if ((newmp = (gbuf_t *)gbuf_copym((gbuf_t *) mp)) == (gbuf_t *)NULL)
	    return ((at_ddp_short_t *)NULL);
	gbuf_freem(mp);
	mp = newmp;

	ddp = (at_ddp_t *)gbuf_rptr(mp);
	gbuf_rinc(mp,((DDP_X_HDR_SIZE - DDP_HDR_SIZE) - LLAP_HDR_SIZE));
	llap = (at_llap_hdr_t *)gbuf_rptr(mp);
	ddp_short = (at_ddp_short_t *)(gbuf_rptr(mp) + LLAP_HDR_SIZE);

	llap->destination = ddp->dst_node;
	llap->type = LLAP_TYPE_DDP;
	ddp_short->length = ddp->length - (DDP_X_HDR_SIZE - DDP_HDR_SIZE);
	ddp_short->unused = 0;
	return ((at_ddp_short_t *)mp);
}


/* mp points to message of the form {llap, short ddp, ...}.
 * Get rid of llap, extend ddp header to make it of the form
 * {extended ddp, ... }
 */
static	gbuf_t	*ddp_extend_hdr (mp)
register gbuf_t	*mp;
{
	register at_llap_hdr_t	*llap;
	register at_ddp_short_t	*ddp_short;
	register at_ddp_t	*ddp;
	char		buf[DDP_HDR_SIZE + LLAP_HDR_SIZE];
	gbuf_t		*m1, *m2;

	/* We need to remove the llap header from the packet and extend the
	 * short DDP header in to a long one.  5 bytes of additional space
	 * is required in effect, but we can not afford to put these 5 bytes
	 * in a separate buffer, since the ddp buffer would end up being
	 * fragmented into two pieces, which is a no-no.  So, we first get
	 * rid of the llap and ddp short headers and then add the extended
	 * header.
	 */
	
	/* Assuming that the llap and ddp short headers are placed next
	 * to each other in the same buffer
	 */
	bcopy(gbuf_rptr(mp), buf, LLAP_HDR_SIZE + DDP_HDR_SIZE);
	m1 = ddp_adjmsg(mp, LLAP_HDR_SIZE+DDP_HDR_SIZE) ? mp : 0;

	/* If the message did not contain any ddp data bytes, then m would
	 * be NULL at this point... and we can't just grow a NULL message, 
	 * we need to ALLOC a new one.
	 */
	if (m1) {
		if ((m2 = (gbuf_t *)ddp_growmsg(m1, -DDP_X_HDR_SIZE)) == NULL) {
			dPrintf(D_M_DDP, D_L_WARNING,
				("Dropping packet - no bufs to extend hdr"));
			at_ddp_stats.rcv_dropped_nobuf++;
			gbuf_freem(m1);
			return(NULL);
		}
	} else
		/* Original message mp has already been freed by ddp_adjmsg if we
		 * managed to arrive here... this case occurs only when the
		 * message mp did not contain any ddp data bytes, only lap and
		 * ddp headers
		 */
		if ((m2 = gbuf_alloc(AT_WR_OFFSET+DDP_X_HDR_SIZE, PRI_MED)) == NULL) {
			dPrintf(D_M_DDP,D_L_WARNING,
				("Packet (no data) dropped - no bufs to extend hdr"));
			at_ddp_stats.rcv_dropped_nobuf++;
			return(NULL);
		} else {
			gbuf_rinc(m2,AT_WR_OFFSET);
			gbuf_wset(m2,DDP_X_HDR_SIZE);
		}
	
	/* By the time we arrive here, m2 points to message of the form
	 * {Extended DDP, ... }
	 * mp and m1 are either non-existent or irrelevant.
	 */
	ddp = (at_ddp_t *)gbuf_rptr(m2);
	llap = (at_llap_hdr_t *)buf;
	ddp_short = (at_ddp_short_t *)(buf + LLAP_HDR_SIZE);

	ddp->unused = ddp->hopcount = 0;
	ddp->length = ddp_short->length + DDP_X_HDR_SIZE - DDP_HDR_SIZE;
	UAS_ASSIGN(ddp->checksum, 0);
	NET_NET(ddp->dst_net, ifID_table[IFID_HOME]->ifThisNode.atalk_net);
	NET_NET(ddp->src_net, ifID_table[IFID_HOME]->ifThisNode.atalk_net);
	ddp->src_node = llap->source;
	ddp->dst_node = llap->destination;
	ddp->dst_socket = ddp_short->dst_socket;
	ddp->src_socket = ddp_short->src_socket;
	ddp->type = ddp_short->type;
	return (m2);
}


int ddp_get_cfg(cfgp, src_socket)
register at_ddp_cfg_t	*cfgp;
at_socket src_socket;
{
	register at_if_t *ifID = ifID_table[IFID_HOME];

	dPrintf(D_M_DDP, D_L_VERBOSE, ("ddp_get_cfg() entry"));
	cfgp->network_up = (ifID != NULL);

	cfgp->flags = ifID? ifID->ifFlags : 0;

	if (ifID) {
		NET_NET(cfgp->node_addr.net, ifID->ifThisNode.atalk_net);
		cfgp->node_addr.node = ifID->ifThisNode.atalk_node;
		cfgp->node_addr.socket = src_socket;
		if (!ROUTING_MODE && (ifID->ifRouterState == NO_ROUTER)) {
			NET_ASSIGN(cfgp->router_addr.net, 0);
			cfgp->router_addr.node = 0;
		} else {
			NET_NET(cfgp->router_addr.net, ifID->ifARouter.atalk_net);
			cfgp->router_addr.node = ifID->ifARouter.atalk_node;
		}
		cfgp->netlo = ifID->ifThisCableStart;
		cfgp->nethi = ifID->ifThisCableEnd;
	}
	else {
		NET_ASSIGN(cfgp->node_addr.net, 64000);
		cfgp->node_addr.node = 128;
		NET_ASSIGN(cfgp->router_addr.net, 0);
		cfgp->router_addr.node = 0;
		cfgp->node_addr.socket = src_socket;
		cfgp->netlo = 1;
		cfgp->nethi = 64500;
	}
	
	return(0);
}


int ddp_get_stats(statsp)
at_ddp_stats_t	*statsp;
{
	dPrintf(D_M_DDP, D_L_VERBOSE, ("ddp_get_cfg() entry"));
	bcopy(&at_ddp_stats, statsp, sizeof(at_ddp_stats));
	return(0);
}


/****************************************************************/
/*								*/
/*								*/
/*			Module Code				*/
/*								*/
/*								*/
/****************************************************************/

/*
 * ddp_bind_socket()
 *
 * Description:
 *
 * Calling Sequence:
 *	ret_status = ddp_bind_socket(socketp)
 *
 * Formal Parameters:
 *	socketp		pointer to a socket table entry
 *
 * Completion Status:
 *	0		Procedure successfully completed.
 *	EINVAL		Invalid well-known socket supplied.
 *	EACCES		User is not super-user.
 *	ENETDOWN	There are no interfaces online.
 *
 * Side Effects:
 *	NONE
 */

int
ddp_bind_socket(socketp)
register ddp_socket_t	*socketp;
{
	register at_socket     socket;
	u_char proto;
	int s;

	ATDISABLE(s, ddpall_lock);
	if (socketp->dev == NULL)
		proto = 0;
	else
		proto = ((ddp_dev_t *)socketp->dev)->proto;

	/* Request for dynamic socket? */
	if (socketp->number == 0) {
		/* Search table for free one */
		for (socket = DDP_SOCKET_LAST-proto; 
			socket >= DDP_SOCKET_1st_DYNAMIC; socket--)
			if (DDP_SOCK_UPQ(socket) == 0) {
				if ((proto == DDP_ATP) && atp_inputQ[socket])
					continue;
				else if ((proto == DDP_ADSP) && adsp_inputQ[socket])
					continue;
				break;
			}
		if (socket < DDP_SOCKET_1st_DYNAMIC)
		{
			ATENABLE(s, ddpall_lock);
			return(EADDRNOTAVAIL);	/* Error if no free sockets */
		}
		else
			socketp->number = ddp_socket[socket].number;
	} else {
		/* Asking to open a socket by its number.  Check if its legal & 
		 * free. 
		 */
		socket = socketp->number;
		if (socket > DDP_SOCKET_LAST) 
		{
			ATENABLE(s, ddpall_lock);
			return(EINVAL);
		}
		if (DDP_SOCK_UPQ(socket) || DDP_SOCK_HANDLER(socket))
		{
			ATENABLE(s, ddpall_lock);
			return(EISCONN);
		}
	}
	if (proto == DDP_ATP) {
		if (atp_inputQ[socket])
		{
			ATENABLE(s, ddpall_lock);
			return(EISCONN);
		}
		ot_atp_socketM[socket] = 1;
	} else if (proto == DDP_ADSP) {
		if (adsp_inputQ[socket])
		{
			ATENABLE(s, ddpall_lock);
			return(EISCONN);
		}
		ot_adsp_socketM[socket] = 1;
	}
	ddp_socket[socket].flags = socketp->flags;
	DDP_SOCK_UPQ(socket) = socketp->sock_u.gref;
	ATENABLE(s, ddpall_lock);

	dPrintf(D_M_DDP, D_L_VERBOSE,
		("Socket 0x%x is opened", (u_char)socket));

	return(0);
}
      

/*
 * ddp_close_socket()
 *
 * Description:                  
 * 	This procedure closes a DDP socket.
 *	NBP is notified that the socket is being closed so that it can 
 *	deregister any attached names.
 *
 * Calling Sequence:
 *	ret_status = ddp_close_socket(socketp)
 *
 * Formal Parameters:
 *	socketp		pointer to a socket table entry
 *
 * Completion Status:
 *	0		Procedure successfully completed.
 *	ENOTCONN	Socket was not open.
 *
 * Side Effects:
 *	NONE
 */
int ddp_close_socket(socketp)
register ddp_socket_t	*socketp;
{
	register at_socket	socket = socketp->number;

	/* Make sure socket was open, if not return error */
	if (!DDP_SOCK_UPQ(socket))
		return (ENOTCONN);

	/* Notify NBP that we are closing this DDP socket */
	ddp_notify_nbp(socket,
		((ddp_dev_t *)socketp->dev)->pid, 0);

	/* mark socket as closed */
	if (socketp->flags & DDP_CALL_HANDLER)
		socketp->sock_u.handler = NULL;
	else
		socketp->sock_u.gref = NULL;
	socketp->flags = 0;
	
	if (((ddp_dev_t *)socketp->dev)->proto == DDP_ATP)
		ot_atp_socketM[socket] = 0;
	else if (((ddp_dev_t *)socketp->dev)->proto == DDP_ADSP)
		ot_adsp_socketM[socket] = 0;

	dPrintf(D_M_DDP, D_L_VERBOSE,
		("Socket 0x%x is closed", (u_char)socket));

	return (0);
}


/* There are various ways a packet may go out.... it may be sent out
 * directly to destination node, or sent to a random router or sent
 * to a router whose entry exists in Best Router Cache.  Following are 
 * constants used WITHIN this routine to keep track of choice of destination
 */
#define DIRECT_ADDR	1
#define	BRT_ENTRY	2
#define	BRIDGE_ADDR	3

/* 
 * ddp_output()
 *
 * Remarks : 
 *	called to queue a atp/ddp data packet on the network interface.
 *	It returns 0 normally, and an errno in case of error.
 *
 */
int ddp_output(mp, src_socket, src_addr_included)
     register gbuf_t	**mp;
     at_socket	src_socket;
     int src_addr_included;
{
	register at_if_t	*ifID, *ifIDTmp;
	register at_ddp_t	*ddp;
	register ddp_brt_t	*brt;
	register at_net_al	dst_net;
	register int 		len;
	struct	 atalk_addr	at_dest;
	at_if_t		*ARouterIf = NULL;
	at_ddp_short_t	*ddp_short = NULL;
	int		loop = 0;
	int		error;
	int		addr_type;
	void		ddp_input();
	u_char	addr_flag, ddp_type;
	char	*addr = NULL;
	register gbuf_t	*m, *tmp_m;
	at_ddp_t	*tmp_ddp;

	snmpStats.dd_outReq++;
	if ((ifID = ifID_table[IFID_HOME]) == NULL)
		return (ENETDOWN);
	m = *mp;
	ddp = (at_ddp_t *)gbuf_rptr(m);
	if (MULTIHOME_MODE && (ifIDTmp = forUs(ddp))) {
		ifID = ifIDTmp;
		loop = TRUE;
	    dPrintf(D_M_DDP_LOW, D_L_USR1,
			("ddp_out: for us if:%s\n", ifID->ifName));
	}

    if ((ddp->dst_socket > (unsigned) (DDP_SOCKET_LAST + 1)) || 
		(ddp->dst_socket < DDP_SOCKET_1st_RESERVED)) {
	    dPrintf(D_M_DDP, D_L_ERROR,
			("Illegal destination socket on outgoing packet (0x%x)",
			ddp->dst_socket));
		at_ddp_stats.xmit_bad_addr++;
		return (ENOTSOCK);
	}
	if ((len = gbuf_msgsize(*mp)) > DDP_DATAGRAM_SIZE) {
	        /* the packet is too large */
	        dPrintf(D_M_DDP, D_L_ERROR,
			("Outgoing packet too long (len=%d bytes)", len));
		at_ddp_stats.xmit_bad_length++;
		return (EMSGSIZE);
	}
	at_ddp_stats.xmit_bytes += len;
	at_ddp_stats.xmit_packets++;

	DDPLEN_ASSIGN(ddp, len);
	ddp->hopcount = ddp->unused = 0;

	/* If this packet is for the same node, loop it back
	 * up...  Note that for LocalTalk, dst_net zero means "THIS_NET", so
	 * address 0.nn is eligible for loopback.  For Extended EtherTalk,
	 * dst_net 0 can be used only for cable-wide or zone-wide 
	 * broadcasts (0.ff) and as such, address of the form 0.nn is NOT
	 * eligible for loopback.
	 */
	dst_net = NET_VALUE(ddp->dst_net);

	/* If our packet is destined for the 'virtual' bridge
	 * address of NODE==0xFE, replace that address with a
	 * real bridge address.
	 */
	if ((ddp->dst_node == 0xfe) && 
	    ((dst_net == 0) ||
	     (dst_net >= ifID->ifThisCableStart &&
	      dst_net <= ifID->ifThisCableEnd))) {
		NET_NET(ddp->dst_net, ifID->ifARouter.atalk_net);
		dst_net = NET_VALUE(ifID->ifARouter.atalk_net);
		ddp->dst_node = ifID->ifARouter.atalk_node;
	}
	loop = ((ddp->dst_node == ifID->ifThisNode.atalk_node) &&
		(dst_net == NET_VALUE(ifID->ifThisNode.atalk_net))
	       );

	if (loop) {
		gbuf_t *mdata, *mdata_next;

		ddp_type = ddp->type;
		ddp->src_node = ifID->ifThisNode.atalk_node;
		ddp->src_socket = src_socket;
		NET_NET(ddp->src_net, ifID->ifThisNode.atalk_net);

		dPrintf(D_M_DDP_LOW, D_L_OUTPUT,
			("ddp_output: loop to %d:%d port=%d\n",
			  NET_VALUE(ddp->src_net),
			  ddp->src_node,
			  ifID->ifPort));

		if (UAS_VALUE(ddp->checksum)) {
			u_short tmp;
			tmp = ddp_checksum(*mp, 4);
			UAS_ASSIGN(ddp->checksum, tmp);
		}
		tmp_m = *mp;
		for (tmp_m=gbuf_next(tmp_m); tmp_m; tmp_m=gbuf_next(tmp_m)) {
			tmp_ddp = (at_ddp_t *)gbuf_rptr(tmp_m);
			DDPLEN_ASSIGN(tmp_ddp, gbuf_msgsize(tmp_m));
			tmp_ddp->hopcount = tmp_ddp->unused = 0;
			tmp_ddp->src_node = ddp->src_node;
			NET_NET(tmp_ddp->src_net, ddp->src_net);
			if (UAS_VALUE(tmp_ddp->checksum)) {
				u_short tmp;
				tmp = ddp_checksum(tmp_m, 4);
				UAS_ASSIGN(tmp_ddp->checksum, tmp);
			}
		}
		dPrintf(D_M_DDP, D_L_VERBOSE,
			("Looping back a packet from socket 0x%x to socket 0x%x",
			ddp->src_socket, ddp->dst_socket));

		for (mdata = *mp; mdata; mdata = mdata_next) {
			mdata_next = gbuf_next(mdata);
			gbuf_next(mdata) = 0;
			ddp_input(mdata, ifID);
		}
		return (0);
	}
        if ((ddp->dst_socket == ZIP_SOCKET) &&
	             (zip_type_packet(*mp) == ZIP_GETMYZONE)) {
	        ddp->src_socket = src_socket;
	        if ((error = zip_handle_getmyzone(ifID, *mp)) != 0)
		        return(error);
		gbuf_freem(*mp);
		return (0);
	}
	/*
	 * find out the interface on which the packet should go out
	 */
	for (ifID = at_ifQueueHd.FwdLink; ifID != &at_ifQueueHd; ifID = ifID->FwdLink) {

		if ((NET_VALUE(ifID->ifThisNode.atalk_net) == dst_net) || (dst_net == 0))
			/* the message is either going out (i) on the same 
			 * NETWORK in case of LocalTalk, or (ii) on the same
			 * CABLE in case of Extended AppleTalk (EtherTalk).
			 */
			break;

		if ((ifID->ifThisCableStart <= dst_net) &&
		    (ifID->ifThisCableEnd   >= dst_net)
		   )
			/* We're on EtherTalk and the message is going out to 
			 * some other network on the same cable.
			 */
			break;
		
		if (ARouterIf == NULL && ATALK_VALUE(ifID->ifARouter))
			ARouterIf = ifID;
	}
	dPrintf(D_M_DDP_LOW, D_L_USR1,
			("ddp_out: after search ifid:0x%x %s ifID0:0x%x\n",
			(u_int) ifID, ifID ? ifID->ifName : "",
			(u_int) ifID_table[0]));

	if (ifID != &at_ifQueueHd) {
		/* located the interface where the packet should
		 * go.... the "first-hop" destination address
		 * must be the same as real destination address.
		 */
		addr_type = DIRECT_ADDR;
	} else {
		/* no, the destination network number does
		 * not match known network numbers.  If we have
		 * heard from this network recently, BRT table
		 * may have address of a router we could use!
		 */
		if (!ROUTING_MODE && !MULTIHOME_MODE) {
		
			BRT_LOOK (brt, dst_net);
			if (brt) {
				/* Bingo... BRT has an entry for this network. 
				 * Use the link address as is.
				 */
				dPrintf(D_M_DDP, D_L_VERBOSE,
					("Found BRT entry to send to net 0x%x", dst_net));
				at_ddp_stats.xmit_BRT_used++;
				addr_type = BRT_ENTRY;
				ifID = brt->ifID;
			} else {
				/* No BRT entry available for dest network... do we 
				 * know of any router at all??
				 */
				if ((ifID = ARouterIf) != NULL)
					addr_type = BRIDGE_ADDR;
				else {
		 		dPrintf(D_M_DDP, D_L_WARNING,
						("Found no interface to send pkt"));
					at_ddp_stats.xmit_bad_addr++;
					return (ENETUNREACH);
				}
			}
		}
		else { /* We are in multiport mode,  so we can bypass all the rest 
			    * and directly ask for the routing of the packet
				*/ 

			ddp->src_socket = src_socket;
			at_ddp_stats.xmit_BRT_used++;
			if (!MULTIPORT_MODE || ifID == &at_ifQueueHd) 
				ifID = ifID_table[IFID_HOME];
		  if (!src_addr_included) {
			ddp->src_node = ifID->ifThisNode.atalk_node;
			NET_NET(ddp->src_net, ifID->ifThisNode.atalk_net); 
		  }
			routing_needed(*mp, ifID, TRUE);
			return (0);
		}
	}
	/* by the time we land here, we know the interface on 
	 * which this packet is going out....  ifID.  
	 */
	
		switch (addr_type) {
		case DIRECT_ADDR :
			at_dest.atalk_unused = 0;
			NET_ASSIGN(at_dest.atalk_net, dst_net);
			at_dest.atalk_node = ddp->dst_node;
			addr_flag = AT_ADDR;
			addr = (char *)&at_dest;
			break;
		case BRT_ENTRY :
			addr_flag = ET_ADDR;
			addr = (char *)&brt->et_addr;
			break;
		case BRIDGE_ADDR :
			at_dest = ifID->ifARouter;
			addr_flag = AT_ADDR;
			addr = (char *)&at_dest;
			break;

		}
		/* Irrespective of the interface on which 
		 * the packet is going out, we always put the 
		 * same source address on the packet (unless multihoming mode).
		 */
		if (MULTIHOME_MODE) {
		  if (!src_addr_included) {
			ddp->src_node = ifID->ifThisNode.atalk_node;
			NET_NET(ddp->src_net, ifID->ifThisNode.atalk_net); 
		  }
		}
		else {
			ddp->src_node = ifID_table[IFID_HOME]->ifThisNode.atalk_node;
			NET_NET(ddp->src_net, ifID_table[IFID_HOME]->ifThisNode.atalk_net);
		}

		ddp->src_socket = src_socket;
		if (UAS_VALUE(ddp->checksum)) {
			u_short tmp;
			tmp = ddp_checksum(*mp, 4);
			UAS_ASSIGN(ddp->checksum, tmp);
		}

	dPrintf(D_M_DDP, D_L_VERBOSE,
		("Packet going out to : net 0x%x, node 0x%x, socket 0x%x on %s",
		(ddp_short ? 0 : dst_net), 
		(ddp_short ? 0 : ddp->dst_node),
		(ddp_short ? ddp_short->dst_socket : ddp->dst_socket),ifID->ifName));
	 dPrintf(D_M_DDP_LOW, D_L_OUTPUT,
		("ddp_output: going out to %d:%d skt%d\n",
		dst_net, ddp->dst_node, ddp->dst_socket));

	if (ifID->ifState != LAP_OFFLINE) {
			tmp_m = *mp;
			for (tmp_m=gbuf_next(tmp_m); tmp_m; tmp_m=gbuf_next(tmp_m)) {
				tmp_ddp = (at_ddp_t *)gbuf_rptr(tmp_m);
				DDPLEN_ASSIGN(tmp_ddp, gbuf_msgsize(tmp_m));
				tmp_ddp->hopcount = tmp_ddp->unused = 0;
				tmp_ddp->src_node = ddp->src_node;
				NET_NET(tmp_ddp->src_net, ddp->src_net);
				tmp_ddp->dst_node = ddp->dst_node;
				NET_NET(tmp_ddp->dst_net, ddp->dst_net);
				if (UAS_VALUE(tmp_ddp->checksum)) {
					u_short tmp;
					tmp = ddp_checksum(tmp_m, 4);
					UAS_ASSIGN(tmp_ddp->checksum, tmp);
				}
			}
	{
	struct	etalk_addr	dest_addr;
	struct	atalk_addr	dest_at_addr;
	int		loop = TRUE;		/* flag to aarp to loopback (default) */
	elap_specifics_t *elapp = (elap_specifics_t *)ifID->ifLapp;

	m = *mp;

	/* the incoming frame is of the form {flag, address, ddp...}
	 * where "flag" indicates whether the address is an 802.3
	 * (link) address, or an appletalk address.  If it's an
	 * 802.3 address, the packet can just go out to the network
	 * through PAT, if it's an appletalk address, AT->802.3 address
	 * resolution needs to be done.
	 * If 802.3 address is known, strip off the flag and 802.3
	 * address, and prepend 802.2 and 802.3 headers.
	 */
	
	if (addr == NULL) {
		addr_flag = *(u_char *)gbuf_rptr(m);
		gbuf_rinc(m,1);
	}
	
	switch (addr_flag) {
	case AT_ADDR_NO_LOOP :
		loop = FALSE;
		/* pass thru */
	case AT_ADDR :
	if (addr == NULL) {
	    dest_at_addr = *(struct atalk_addr *)gbuf_rptr(m);
	    gbuf_rinc(m,sizeof(struct atalk_addr));
	} else
	    dest_at_addr = *(struct atalk_addr *)addr;
	    break;
	case ET_ADDR :
	if (addr == NULL) {
	    dest_addr = *(struct etalk_addr *)gbuf_rptr(m);
	    gbuf_rinc(m,sizeof(struct etalk_addr));
	} else
	    dest_addr = *(struct etalk_addr *)addr;
	    break;
	default :
		dPrintf(D_M_DDP_LOW,D_L_ERROR,
		    ("ddp_output: Unknown addr_flag = 0x%x\n", addr_flag));
	    gbuf_freel(m);		/* unknown address type, chuck it */
	    return 0;
        }

	while (gbuf_len(m) == 0) {
		tmp_m = m;
		m = gbuf_cont(m);
		gbuf_freeb(tmp_m); 
	}

	/* At this point, rptr points to ddp header for sure */

	if (elapp->elap_if.ifState == LAP_OFFLINE) {
	    gbuf_freel(m);
		return 0;
	}

	if (elapp->elap_if.ifState == LAP_ONLINE_FOR_ZIP) {
		/* see if this is a ZIP packet that we need
		 * to let through even though network is
		 * not yet alive!!
		 */
		if (zip_type_packet(m) == 0) {
	    	gbuf_freel(m);
			return 0;
		}
	}
	
	elapp->stats.xmit_packets++;
	elapp->stats.xmit_bytes += gbuf_msgsize(m);
	snmpStats.dd_outLong++;
	
	switch (addr_flag) {
	case AT_ADDR_NO_LOOP :
	case AT_ADDR :
	    /*
	     * we don't want elap to be looking into ddp header, so
	     * it doesn't know net#, consequently can't do 
	     * AMT_LOOKUP.  That task left to aarp now.
	     */
	    aarp_send_data(m,elapp,&dest_at_addr, loop);
	    break;
	case ET_ADDR :
	    pat_output(elapp->pat_id, m, &dest_addr, 0);
	    break;
        }
	}
			return(0);
	} 
	gbuf_freel(*mp);
	return (0);
} /* ddp_output */

void	ddp_input(mp, ifID)
register gbuf_t   *mp;
register at_if_t *ifID;
{
	register at_ddp_t *ddp;
	register int       msgsize;
	register at_socket socket;
	register int	   len;
	register at_net_al dst_net;
	extern volatile RoutingMix;

	KERNEL_DEBUG(DBG_AT_DDP_INPUT | DBG_FUNC_START, 0,0,0,0,0);

	/* Makes sure we know the default interface before starting to
	 * accept incomming packets. If we don't we may end up with a
	 * null ifID_table[0] and have impredicable results (specially
	 * in router mode. This is a transitory state (because we can
	 * begin to receive packet while we're not completly set up yet.
	 */

	if (ifID_table[IFID_HOME] == (at_if_t *)NULL) {
		dPrintf(D_M_DDP, D_L_ERROR,
			("dropped incoming packet ifID_home not set yet\n"));
		gbuf_freem(mp);
		goto out; /* return */
	}

	/*
	 * if a DDP packet has been broadcast, we're going to get a copy of
	 * it here; if it originated at user level via a write on a DDP 
	 * socket; when it gets here, the first block in the chain will be
	 * empty since it only contained the lap level header which will be
	 * stripped in the lap level immediately below ddp
	 */

	if ((mp = (gbuf_t *)ddp_compress_msg(mp)) == NULL) {
		dPrintf(D_M_DDP, D_L_ERROR,
			("dropped short incoming ET packet (len %d)", 0));
		snmpStats.dd_inTotal++;
		at_ddp_stats.rcv_bad_length++;
		goto out; /* return; */
	}
	msgsize = gbuf_msgsize(mp);

	at_ddp_stats.rcv_bytes += msgsize;
	at_ddp_stats.rcv_packets++;

	/* if the interface pointer is 0, the packet has been 
	 * looped back by 'write' half of DDP.  It is of the
	 * form {extended ddp,...}.  The packet is meant to go
	 * up to some socket on the same node.
	 */
	if (ifID == (at_if_t *)NULL)		/* if loop back is specified */
		ifID = ifID_table[IFID_HOME];	/* that means the home port */

		/* the incoming datagram has extended DDP header and is of 
		 * the form {ddp,...}.
		 */
		if (msgsize < DDP_X_HDR_SIZE) {
			dPrintf(D_M_DDP, D_L_ERROR,
				("dropped short incoming ET packet (len %d)", 
				msgsize));
			at_ddp_stats.rcv_bad_length++;
			gbuf_freem(mp);
			goto out; /* return; */
		}
	/*
	 * At this point, the message is always of the form
	 * {extended ddp, ... }.
	 */
	ddp = (at_ddp_t *)gbuf_rptr(mp);
	len = DDPLEN_VALUE(ddp);

	if (msgsize != len) {
	        if ((unsigned) msgsize > len) {
		        if (len < DDP_X_HDR_SIZE) {
			        dPrintf(D_M_DDP, D_L_ERROR,
				       ("Length problems, ddp length %d, buffer length %d",
				       len, msgsize));
				snmpStats.dd_tooLong++;
				at_ddp_stats.rcv_bad_length++;
				gbuf_freem(mp);
				goto out; /* return; */
			}
		        /*
			 * shave off the extra bytes from the end of message
		         */
		        mp = ddp_adjmsg(mp, -(msgsize - len)) ? mp : 0;
		        if (mp == 0)
				goto out; /* return; */
		} else {
		        dPrintf(D_M_DDP, D_L_ERROR,
				("Length problems, ddp length %d, buffer length %d",
				len, msgsize));
				snmpStats.dd_tooShort++;
			at_ddp_stats.rcv_bad_length++;
			gbuf_freem(mp);
			goto out; /* return; */
		}
	}
	socket = ddp->dst_socket;

	/*
	 * We want everything in router mode, specially socket 254 for nbp so we need
	 * to bypass this test when we are a router.
	 */

	if (!ROUTING_MODE && (socket > DDP_SOCKET_LAST ||
			 socket < DDP_SOCKET_1st_RESERVED)) {
		dPrintf(D_M_DDP, D_L_WARNING,
			("Bad dst socket on incoming packet (0x%x)",
			ddp->dst_socket));
		at_ddp_stats.rcv_bad_socket++;
		gbuf_freem(mp);
		goto out; /* return; */
	}
	/*
	 * if the checksum is true, then upstream wants us to calc
	 */
	if (UAS_VALUE(ddp->checksum) && 
           (UAS_VALUE(ddp->checksum) != ddp_checksum(mp, 4))) {
		dPrintf(D_M_DDP, D_L_WARNING,
			("Checksum error on incoming pkt, calc 0x%x, exp 0x%x",
			ddp_checksum(mp, 4), UAS_VALUE(ddp->checksum)));
		snmpStats.dd_checkSum++;
		at_ddp_stats.rcv_bad_checksum++;
		gbuf_freem(mp);
		goto out; /* return; */
	}

/*############### routing input checking */

/* Router mode special: we send "up-stack" packets for this node or coming from any
 * other ports, but for the reserved atalk sockets (RTMP, ZIP, NBP [and EP])
 * BTW, the way we know it's for the router and not the home port is that the
 * MAC (ethernet) address is always the one of the interface we're on, but
 * the AppleTalk address must be the one of the home port. If it's a multicast
 * or another AppleTalk address, this is the router job's to figure out where it's
 * going to go.
 */
	dst_net = NET_VALUE(ddp->dst_net);
	if (((ddp->dst_node == ifID_table[IFID_HOME]->ifThisNode.atalk_node) &&
	 (dst_net == NET_VALUE(ifID_table[IFID_HOME]->ifThisNode.atalk_net))) ||
		((ddp->dst_node == 255) && 
		(((dst_net >= ifID_table[IFID_HOME]->ifThisCableStart) &&
		(dst_net <= ifID_table[IFID_HOME]->ifThisCableEnd)) || dst_net == 0)) ||
		(socket == RTMP_SOCKET) || (socket == NBP_SOCKET)  || (socket == EP_SOCKET) ||
		(socket == ZIP_SOCKET) || (ifID->ifRoutingState < PORT_ONLINE) ||
		( MULTIPORT_MODE && forUs(ddp))
     )
	 { 

	    gref_t   *gref;
		extern   ddp_dev_t ddp_devs[];
		pktsHome++;
		snmpStats.dd_inLocal++;
	if (ot_protoT[ddp->type]) {
		if (ddp->type == DDP_ATP) {
			if ((socket == ZIP_SOCKET) || (atp_inputQ[socket] == NULL)
			    || (atp_inputQ[socket] == (gref_t *)1))
				goto l_continue;
			atp_input(mp);
		} else if (ddp->type == DDP_ADSP) {
			if (adsp_inputQ[socket] == NULL)
				goto l_continue;
			adsp_input(mp);
		}
		goto out; /* return; */
	}
		/* 
		 * Assure that this isn't for a shut-down socket
		 */
l_continue:
		if ((ddp_devs[socket].flags&DDPF_SHUTDOWN) == 0 &&
		    (gref = DDP_SOCK_UPQ(socket)) != NULL) {
		        /* there is an upstream! either a queue or a handler routine */
		        if (ddp_socket[socket].flags & DDP_CALL_HANDLER) {
				dPrintf(D_M_DDP,D_L_INPUT,
					("ddp_input: skt hndlr skt %d hdnlr:0x%x\n",
					(u_int) socket,
					DDP_SOCK_HANDLER(socket)));
			        (*DDP_SOCK_HANDLER(socket))(mp, ifID);
			} else {
				dPrintf(D_M_DDP, D_L_INPUT, 
					("ddp_input: streamq, skt %d\n", socket));
			
				if (DDP_FLAGS(gref) & DDPF_SI) {
					if (si_ddp_input(gref, mp)) {
						at_ddp_stats.rcv_dropped_nobuf++;
					}
				} else
			        atalk_putnext(gref, mp);
			}
		} else {
		        dPrintf(D_M_DDP, D_L_VERBOSE,
				("Dropping a packet -- dest socket closed"));
			at_ddp_stats.rcv_bad_socket++;
			gbuf_freem(mp);
			snmpStats.dd_noHandler++;
			dPrintf(D_M_DDP, D_L_WARNING, 
				("ddp_input: dropped, hndlr socket %d unknown\n", socket));
		}
	}

	else { 
		dPrintf(D_M_DDP, D_L_ROUTING, ("ddp_input: routing_needed from  port=%d sock=%d\n",
					ifID->ifPort, ddp->dst_socket));

		snmpStats.dd_fwdReq++;
		if (((pktsIn-pktsHome+200) >= RoutingMix) && ((++pktsDropped % 5) == 0)) {
			at_ddp_stats.rcv_dropped_nobuf++;
			gbuf_freem(mp);
		}
		else {
			routing_needed(mp, ifID, FALSE);
		}
	}
out:
	KERNEL_DEBUG(DBG_AT_DDP_INPUT | DBG_FUNC_END, 0,0,0,0,0);
}


/* 
 * ddp_router_output()
 *
 * Remarks : 
 *	This is a modified version of ddp_output for router use.
 *	The main difference is that the interface on which the packet needs
 *	to be sent is specified and a *destination* AppleTalk address is passed
 *	as an argument, this address may or may not be the same as the destination
 *	address found in the ddp packet... This is the trick about routing, the
 *	AppleTalk destination of the packet may not be the same as the Enet address
 *	we send the packet too (ie, we may pass the baby to another router).	
 *
 */
int ddp_router_output(mp, ifID, addr_type, router_net, router_node, enet_addr)
gbuf_t	*mp;
at_if_t *ifID;
int addr_type;
at_net_al router_net;
at_node router_node;
etalk_addr_t *enet_addr;
{
	register at_ddp_t	*ddp;
	struct	 atalk_addr	at_dest;
	void		ddp_input();
	int		addr_flag;
	char	*addr = NULL;
	register gbuf_t	*m, *tmp_m;
	at_ddp_t	*tmp_ddp;

    if (!ifID || ! IFID_VALID(ifID)) {

		dPrintf(D_M_DDP, D_L_WARNING, ("BAD BAD ifID\n"));
		gbuf_freel(mp);
		return(EPROTOTYPE);
	}
	ddp = (at_ddp_t *)gbuf_rptr(mp);

	if (ifID->ifFlags & AT_IFF_AURP) { /* AURP link? */
		if (ddp_AURPsendx) {
			for (tmp_m=gbuf_next(mp); tmp_m; tmp_m=gbuf_next(tmp_m)) {
				tmp_ddp = (at_ddp_t *)gbuf_rptr(tmp_m);
				DDPLEN_ASSIGN(tmp_ddp, gbuf_msgsize(tmp_m));
				tmp_ddp->hopcount = tmp_ddp->unused = 0;
				tmp_ddp->src_node = ddp->src_node;
				NET_NET(tmp_ddp->src_net, ddp->src_net);
				tmp_ddp->dst_node = ddp->dst_node;
				NET_NET(tmp_ddp->dst_net, ddp->dst_net);
				if (UAS_VALUE(tmp_ddp->checksum)) {
					u_short tmp;
					tmp = ddp_checksum(tmp_m, 4);
					UAS_ASSIGN(tmp_ddp->checksum, tmp);
				}
			}
			if (router_node == 255)
				router_node = 0;
			ddp_AURPsendx(AURPCODE_DATAPKT, mp, router_node);
			return 0;
		} else {
			gbuf_freel(mp);
			return EPROTOTYPE;
		}
	}

	/* keep some of the tests for now ####### */

	if (gbuf_msgsize(mp) > DDP_DATAGRAM_SIZE) {
	        /* the packet is too large */
		dPrintf(D_M_DDP, D_L_WARNING,
			("ddp_router_output: Packet too large size=%d\n",
			 gbuf_msgsize(mp)));
		gbuf_freel(mp);
		return (EMSGSIZE);
	}

	switch (addr_type) {

		case AT_ADDR :

			/*
			 * Check for packet destined to the home stack
			 */

			if	((ddp->dst_node == ifID->ifThisNode.atalk_node) &&
		        (NET_VALUE(ddp->dst_net) == NET_VALUE(ifID->ifThisNode.atalk_net))) {
					dPrintf(D_M_DDP_LOW, D_L_ROUTING, 
						("ddp_r_output: sending back home from port=%d socket=%d\n",
						ifID->ifPort, ddp->dst_socket));

					UAS_ASSIGN(ddp->checksum, 0);
					ddp_input(mp, ifID);	
					return(0);
			}

			NET_ASSIGN(at_dest.atalk_net, router_net);
			at_dest.atalk_node = router_node;

			addr_flag = AT_ADDR_NO_LOOP;
			addr = (char *)&at_dest;
			dPrintf(D_M_DDP_LOW, D_L_ROUTING_AT,
				("ddp_r_output: AT_ADDR out port=%d net %d:%d via rte %d:%d",
				ifID->ifPort, NET_VALUE(ddp->dst_net), ddp->dst_node, router_net,
				router_node));
			break;

		case ET_ADDR :
			addr_flag = ET_ADDR;
			addr = (char *)enet_addr;
			dPrintf(D_M_DDP_LOW, D_L_ROUTING,
				("ddp_r_output: ET_ADDR out port=%d net %d:%d\n",
				ifID->ifPort, NET_VALUE(ddp->dst_net), ddp->dst_node));
			break;
		}

		if (UAS_VALUE(ddp->checksum)) {
			u_short tmp;
			tmp = ddp_checksum(mp, 4);
			UAS_ASSIGN(ddp->checksum, tmp);
		}


	if (ifID->ifState != LAP_OFFLINE) {
			for (tmp_m=gbuf_next(mp); tmp_m; tmp_m=gbuf_next(tmp_m)) {
				tmp_ddp = (at_ddp_t *)gbuf_rptr(tmp_m);
				DDPLEN_ASSIGN(tmp_ddp, gbuf_msgsize(tmp_m));
				tmp_ddp->hopcount = tmp_ddp->unused = 0;
				tmp_ddp->src_node = ddp->src_node;
				NET_NET(tmp_ddp->src_net, ddp->src_net);
				tmp_ddp->dst_node = ddp->dst_node;
				NET_NET(tmp_ddp->dst_net, ddp->dst_net);
				if (UAS_VALUE(tmp_ddp->checksum)) {
					u_short tmp;
					tmp = ddp_checksum(tmp_m, 4);
					UAS_ASSIGN(tmp_ddp->checksum, tmp);
				}
			}
	{
	struct	etalk_addr	dest_addr;
	struct	atalk_addr	dest_at_addr;
	int		loop = TRUE;		/* flag to aarp to loopback (default) */
	elap_specifics_t *elapp = (elap_specifics_t *)ifID->ifLapp;

	m = mp;

	/* the incoming frame is of the form {flag, address, ddp...}
	 * where "flag" indicates whether the address is an 802.3
	 * (link) address, or an appletalk address.  If it's an
	 * 802.3 address, the packet can just go out to the network
	 * through PAT, if it's an appletalk address, AT->802.3 address
	 * resolution needs to be done.
	 * If 802.3 address is known, strip off the flag and 802.3
	 * address, and prepend 802.2 and 802.3 headers.
	 */
	
	if (addr == NULL) {
		addr_flag = *(u_char *)gbuf_rptr(m);
		gbuf_rinc(m,1);
	}
	
	switch (addr_flag) {
	case AT_ADDR_NO_LOOP :
		loop = FALSE;
		/* pass thru */
	case AT_ADDR :
	if (addr == NULL) {
	    dest_at_addr = *(struct atalk_addr *)gbuf_rptr(m);
	    gbuf_rinc(m,sizeof(struct atalk_addr));
	} else
	    dest_at_addr = *(struct atalk_addr *)addr;
	    break;
	case ET_ADDR :
	if (addr == NULL) {
	    dest_addr = *(struct etalk_addr *)gbuf_rptr(m);
	    gbuf_rinc(m,sizeof(struct etalk_addr));
	} else
	    dest_addr = *(struct etalk_addr *)addr;
	    break;
	default :
		dPrintf(D_M_DDP_LOW,D_L_ERROR,
		    ("ddp_output: Unknown addr_flag = 0x%x\n", addr_flag));

	    gbuf_freel(m);		/* unknown address type, chuck it */
	    return 0;
        }

	while (gbuf_len(m) == 0) {
		tmp_m = m;
		m = gbuf_cont(m);
		gbuf_freeb(tmp_m); 
	}

	/* At this point, rptr points to ddp header for sure */

	if (elapp->elap_if.ifState == LAP_OFFLINE) {
	    gbuf_freel(m);
		return 0;
	}

	if (elapp->elap_if.ifState == LAP_ONLINE_FOR_ZIP) {
		/* see if this is a ZIP packet that we need
		 * to let through even though network is
		 * not yet alive!!
		 */
		if (zip_type_packet(m) == 0) {
	    	gbuf_freel(m);
			return 0;
		}
	}
	
	elapp->stats.xmit_packets++;
	elapp->stats.xmit_bytes += gbuf_msgsize(m);
	snmpStats.dd_outLong++;
	
	switch (addr_flag) {
	case AT_ADDR_NO_LOOP :
	case AT_ADDR :
	    /*
	     * we don't want elap to be looking into ddp header, so
	     * it doesn't know net#, consequently can't do 
	     * AMT_LOOKUP.  That task left to aarp now.
	     */
	    aarp_send_data(m,elapp,&dest_at_addr, loop);
	    break;
	case ET_ADDR :
	    pat_output(elapp->pat_id, m, &dest_addr, 0);
	    break;
        }
	}
			return(0);
	} 
	gbuf_freel(mp);
	return (0);
}

int gbuf_freel(m)
	gbuf_t *m;
{
	gbuf_t *tmp_m;

	while ((tmp_m = m) != 0) {
		m = gbuf_next(m);
		gbuf_next(tmp_m) = 0;
		gbuf_freem(tmp_m);
	}
	return (0);
}

/*****************************************/

void rt_delete(NetStop, NetStart)
	unsigned short NetStop;
	unsigned short NetStart;
{
	RT_entry *found;
	int s;

	ATDISABLE(s, ddpinp_lock);
	if ((found = rt_bdelete(NetStop, NetStart)) != 0) {
		bzero(found, sizeof(RT_entry));
		found->right = RT_table_freelist;
		RT_table_freelist = found;
	}
	ATENABLE(s, ddpinp_lock);
}

int ddp_AURPfuncx(code, param, node)
	int code;
	void *param;
	unsigned char node;
{
	extern void zt_add_zonename();
	extern void zt_set_zmap();
	extern int  zt_ent_zindex();
	extern void zt_remove_zones();
	extern void rtmp_timeout();
	extern void rtmp_send_port();
	extern elap_specifics_t elap_specifics[];
	aurp_rtinfo_t rtinfo;
	elap_specifics_t *elapp;
	int k;

	switch (code) {
	case AURPCODE_DATAPKT: /* data packet */
		if (aurp_ifID) {
			dPrintf(D_M_DDP, D_L_TRACE, ("ddp_AURPfuncx: data, 0x%x, %d\n",
				(u_int) aurp_ifID, node));

			ddp_input((gbuf_t *)param, aurp_ifID);
		} else
			gbuf_freem((gbuf_t *)param);
		break;

	case AURPCODE_REG: /* register/deregister */
		if (!ROUTING_MODE)
			return -1;
		ddp_AURPsendx = (void(*)())param;

		if (param) {
			/* register AURP callback function */
			if (aurp_ifID)
				return 0;
			elapp = (elap_specifics_t *)&elap_specifics[1];
			for (k=1; k < IF_TOTAL_MAX; k++, elapp++) {
				if (elapp->pat_id == 0) {
					/* assign a port */
					elapp->pat_id = k;
					aurp_ifID = (at_if_t *)&elapp->elap_if;
					aurp_ifID->ifFlags = (AT_IFF_ETHERTALK|RTR_XNET_PORT);
					ddp_add_if(aurp_ifID);
					aurp_ifID->ifLapp = (void *)elapp;
					aurp_ifID->ifState = LAP_ONLINE;
					aurp_ifID->ifRoutingState = PORT_ONLINE;
					dPrintf(D_M_DDP, D_L_TRACE,
						("ddp_AURPfuncx: on, 0x%x\n",
						(u_int) aurp_ifID));

					ddp_AURPsendx(AURPCODE_DEBUGINFO,
							&dbgBits, aurp_ifID->ifPort);
					return 0;
				}
			}
			return -1;

		} else {
			/* deregister AURP callback function */
			if (aurp_ifID) {
				rtmp_purge(aurp_ifID);
				elapp = (elap_specifics_t *)aurp_ifID->ifLapp;
				ddp_rem_if(aurp_ifID);
				elapp->pat_id = 0;
				aurp_ifID->ifLapp = (void *)0;
				aurp_ifID->ifState = LAP_OFFLINE;
				aurp_ifID->ifRoutingState = PORT_OFFLINE;
				dPrintf(D_M_DDP, D_L_TRACE,
					("ddp_AURPfuncx: off, 0x%x\n", (u_int) aurp_ifID));
				aurp_ifID = 0;
			}
		}
		break;

	case AURPCODE_AURPPROTO: /* proto type - AURP */
		if (aurp_ifID) {
			aurp_ifID->ifFlags |= AT_IFF_AURP;
			rtinfo.RT_table = (void *)RT_table;
			rtinfo.ZT_table = (void *)ZT_table;
			rtinfo.RT_maxentry = RT_MAXENTRY;
			rtinfo.ZT_maxentry = ZT_MAXENTRY;
			rtinfo.rt_lock = (void *)&ddpinp_lock;
			rtinfo.rt_insert = (void *)rt_insert;
			rtinfo.rt_delete = (void *)rt_delete;
			rtinfo.rt_lookup = (void *)rt_blookup;
			rtinfo.zt_add_zname = (void *)zt_add_zonename;
			rtinfo.zt_set_zmap = (void *)zt_set_zmap;
			rtinfo.zt_get_zindex = (void *)zt_ent_zindex;
			rtinfo.zt_remove_zones = (void *)zt_remove_zones;
			ddp_AURPsendx(AURPCODE_RTINFO, &rtinfo, 0);
		}
		break;
	}

	return 0;
}


at_if_t *forUs(ddp)
register at_ddp_t *ddp;

/* checks to see if address of packet is for one of our interfaces
   returns *ifID if it's for us, NULL if not
*/
{
	register at_if_t **ifID = &ifID_table[0];
	int port;

	for (port=0; *ifID && port<IF_TOTAL_MAX; ifID++,port++) {	
		if ((ddp->dst_node == (*ifID)->ifThisNode.atalk_node) &&
	 		(NET_EQUAL(ddp->dst_net, (*ifID)->ifThisNode.atalk_net))
		   ) {
			dPrintf(D_M_DDP_LOW, D_L_ROUTING,
				("pkt was for port %d\n", port));

			return(*ifID);
		}
	}
	return((at_if_t *)NULL);
}
