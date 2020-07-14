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

/*	Copyright (c) 1988, 1989, 1997, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* at_aarp.c: 2.0, 1.17; 10/4/93; Apple Computer, Inc. */;

/* This file is at_aarp.c and it contains all the routines used by AARP. This
 * is part of the LAP layer.
 */

#include <sysglue.h>
#include <at/appletalk.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/at_lap.h>
#include <at_pat.h>
#include <at_elap.h>
#include <at_aarp.h>
#include <at_ddp.h>
#include <at_snmp.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_blue.h>
#include <sys/systm.h>

static int	probing;
/* Following two variables are used to keep track of how many dynamic addresses
 * we have tried out at startup.
 */
int	no_of_nodes_tried;	/* no of node addresses we've tried 
				 * so far, within a network number
				 */
int	no_of_nets_tried;	/* no. of network numbers tried
				 */

struct	etalk_addr	etalk_multicast_addr = {
	{0x09, 0x00, 0x07, 0xff, 0xff, 0xff}};
struct	etalk_addr	ttalk_multicast_addr = {
	{0xC0, 0x00, 0x40, 0x00, 0x00, 0x00}};

struct	etalk_addr	zone_multicast_addr;

struct	etalk_addr	et_zeroaddr = {
	{0, 0, 0, 0, 0, 0}};

aarp_amt_t		probe_cb;
aarp_amt_t	   	et_aarp_amt[IF_TYPE_ET_MAX][AMTSIZE];

int aarp_init();
int aarp_send_data();
StaticProc int aarp_rcv_pkt();
StaticProc int aarp_req_cmd_in();
StaticProc int aarp_resp_cmd_in();
StaticProc int aarp_probe_cmd_in();
StaticProc int aarp_chk_addr();
StaticProc int aarp_send_resp();
StaticProc int aarp_send_req();
StaticProc int aarp_send_probe();
StaticProc aarp_amt_t *aarp_lru_entry();
StaticProc int aarp_glean_info();
StaticProc int aarp_delete_amt_info();
StaticProc int aarp_sched_probe();
StaticProc int aarp_build_pkt();
StaticProc int aarp_sched_req();
StaticProc int aarp_get_rand_node();
StaticProc int aarp_get_next_node();
StaticProc int aarp_get_rand_net();
extern elap_specifics_t elap_specifics[];
extern void *atalk_timeout();
extern void atalk_untimeout();
static void *probe_tmo;
atlock_t arpinp_lock;

extern void AARPwakeup(aarp_amt_t *);
extern int pat_output(int, gbuf_t *, unsigned char *, int);

/****************************************************************************
 * aarp_init()
 *
 ****************************************************************************/
static int is_registered = 0;	/* For y-adapter */

int
aarp_init(elapp, aflag)
register elap_specifics_t	*elapp;
int	aflag;
{
	struct BlueFilter BF, *bf = &BF;
	extern struct BlueFilter RhapFilter[];
#ifdef BF_if
	extern at_register_addr(struct BlueFilter *);
#else
	extern at_register_addr(struct BlueFilter *, struct ifnet *);
#endif
	extern int pat_control(int, int, void *);
	extern struct ifnet *ifunit(char *);

	if (aflag)
		goto aarp_sleep;

	if (is_registered)
	{	is_registered = 0;
		RhapFilter[BFS_ATALK].BF_flags = 0;
	}

	if (probing != PROBE_TENTATIVE)	/* How do I set the initial probe */
		probing = PROBE_IDLE;	/* state ???*/
	else {
		dPrintf(D_M_AARP,D_L_ERROR, 
			("aarp_init: error :probing == PROBE_TENTATIVE\n"));
		return(-1);
	}

	pat_control(elapp->pat_id, PAT_REG_AARP_UPSTREAM, aarp_rcv_pkt);
	pat_control(elapp->pat_id, PAT_REG_CHECKADDR,     aarp_chk_addr);

	/*pick a random addr or start with what we have from initial_node addr */
	if (!ATALK_VALUE(elapp->cfg.initial_addr)) {
		dPrintf(D_M_AARP, D_L_INFO,
			("aarp_init: pick up a new node number\n"));
		aarp_get_rand_node(elapp);
		aarp_get_rand_net(elapp);
	}
	probe_cb.elapp = elapp;
	probe_cb.no_of_retries = 0;
	probe_cb.error = 0;

	no_of_nodes_tried = 0; /* haven't tried any addresses yet */
	no_of_nets_tried = 0;

	if (aarp_send_probe() == -1) {
		probing = PROBE_IDLE;	/* not probing any more */
		dPrintf(D_M_AARP, D_L_ERROR, 
			("aarp_init: aarp_send_probe returns error\n"));
		return(-1);
	}
	return(ENOTREADY);
aarp_sleep:
	if (probe_cb.error != 0) {
		probing = PROBE_IDLE;	/* not probing any more */
		dPrintf(D_M_AARP, D_L_ERROR,
			("aarp_init: probe_cb.error creates error =%d\n", probe_cb.error));
		return(-1);
	}
	bzero ((caddr_t) et_aarp_amt, sizeof(et_aarp_amt));

	elapp->cfg.node = elapp->cfg.initial_addr;
	elapp->elap_if.ifThisNode = elapp->cfg.initial_addr;
	probing = PROBE_DONE;
	/* Register our node address with the y-adapter */
	bf->BF_flags = (BF_VALID|BF_ATALK);
	bf->BF_address = *(unsigned short *)&(elapp->cfg.node.atalk_net); /* Sigh */
	bf->BF_node = elapp->cfg.node.atalk_node;
#ifdef BF_ifp
	bf->BF_ifp = ifunit(ELAPP2IFNAME(elapp));
#endif

#ifdef BF_ifp
	at_register_addr(bf);
#else
	at_register_addr(bf, ifunit(ELAPP2IFNAME(elapp)));
#endif
	is_registered = 1;

	return(0);
}

/*
 * Register an appletalk address:
 *  plant the info in a filter for the Y-adapter, if enabled
 */
int
at_register_addr(register struct BlueFilter *bf,
#ifndef BF_ifp
		 register struct ifnet *ifp
#endif
		 )
{	register struct BlueFilter *bf1;
	extern struct ifnet_blue *blue_if;
#ifdef BF_ifp
	register struct ifnet *ifp = bf->BF_ifp;
#endif
	bf1=&blue_if->filter[BFS_ATALK];
	if (bf1->BF_flags & BF_VALID)
	{	kprintf("Dammit, Rodney, how can I work with all these interruptions?\n");
		return(1);
	}
	*bf1 = *bf;
	bf1->BF_flags |= BF_VALID;
	return(0);
}

/****************************************************************************
 * aarp_rcv_pkt()
 *
 * remarks :
 *	(1) The caller must take care of freeing the real storage (gbuf)
 *	(2) The incoming packet is of the form {802.3, 802.2, aarp}.
 *
 ****************************************************************************/
StaticProc   int   aarp_rcv_pkt(pkt, elapp)
register aarp_pkt_t 	*pkt;
elap_specifics_t	*elapp;
{
	switch (pkt->aarp_cmd) {
	case AARP_REQ_CMD:
		return (aarp_req_cmd_in (pkt, elapp));
	case AARP_RESP_CMD:
		return (aarp_resp_cmd_in (pkt, elapp));
	case AARP_PROBE_CMD:
		return (aarp_probe_cmd_in (pkt, elapp));
	default:
		return (-1);
	}/* end of switch*/
}



/****************************************************************************
 *  aarp_req_cmd_in()
 *
 ****************************************************************************/
StaticProc   int	aarp_req_cmd_in (pkt, elapp)
aarp_pkt_t		*pkt;
elap_specifics_t	*elapp;
{
/*
	kprintf("aarp_req_cmd_in: ifThisNode=%d:%d srcNode=%d:%d dstNode=%d:%d\n",
			NET_VALUE(elapp->elap_if.ifThisNode.atalk_net),
			elapp->elap_if.ifThisNode.atalk_node,
			NET_VALUE(pkt->src_at_addr.atalk_net),
			pkt->src_at_addr.atalk_node,
			NET_VALUE(pkt->dest_at_addr.atalk_net),
			pkt->dest_at_addr.atalk_node);
*/
	if ((probing == PROBE_DONE) && 
	    (ATALK_EQUAL(pkt->dest_at_addr, elapp->elap_if.ifThisNode))) {
		if (aarp_send_resp(elapp, pkt) == -1)
			return(-1);
	}
	/* now to glean some info */
	aarp_glean_info(pkt, elapp);
	return (0);
}



/****************************************************************************
 *  aarp_resp_cmd_in()
 *
 ****************************************************************************/
StaticProc   int	aarp_resp_cmd_in (pkt, elapp)
aarp_pkt_t		*pkt;
elap_specifics_t	*elapp;
{
	register aarp_amt_t	*amt_ptr;
	gbuf_t		        *m;

	switch (probing) {
	case PROBE_TENTATIVE :
		if (ATALK_EQUAL(pkt->src_at_addr, probe_cb.elapp->cfg.initial_addr)) {

			/* this is a response to AARP_PROBE_CMD.  There's
			 * someone out there with the address we desire
			 * for ourselves.
			 */
			atalk_untimeout(aarp_sched_probe, 0, probe_tmo);
			probe_cb.no_of_retries = 0;
			aarp_get_next_node(probe_cb.elapp);
			no_of_nodes_tried++;

			if (no_of_nodes_tried == AARP_MAX_NODES_TRIED) {
				aarp_get_rand_net(probe_cb.elapp);
				aarp_get_rand_node(probe_cb.elapp);
				no_of_nodes_tried = 0;
				no_of_nets_tried++;
			}
			if (no_of_nets_tried == AARP_MAX_NETS_TRIED) {
				/* We have tried enough nodes and nets, give up.
				 */
				probe_cb.error = EADDRNOTAVAIL;
				AARPwakeup(&probe_cb);
				return(0);
			}
			if (aarp_send_probe() == -1) {
				/* expecting aarp_send_probe to fill in 
				 * probe_cb.error
				 */
				AARPwakeup(&probe_cb);
				return(-1);
			}
		} else {
			/* hmmmm! got a response packet while still probing
			 * for AT address and the AT dest address doesn't
			 * match!!
			 * What should I do here??  kkkkkkkkk
			 */
			 return(-1);
		}
		break;

	case PROBE_DONE :
		AMT_LOOK(amt_ptr, pkt->src_at_addr, elapp);
		if (amt_ptr == NULL)
			return(-1);
		if (amt_ptr->tmo)
		atalk_untimeout(aarp_sched_req, amt_ptr, amt_ptr->tmo);
		amt_ptr->tmo = 0;

		if (amt_ptr->m == NULL) {
			/* this may be because of a belated response to 
			 * aarp reaquest.  Based on an earlier response, we
			 * might have already sent the packet out, so 
			 * there's nothing to send now.  This is okay, no 
			 * error.
			 */
			return(0);
		}
		amt_ptr->dest_addr = pkt->src_addr;
		if ((elapp->elap_if.ifType == IFTYPE_FDDITALK)
			|| (elapp->elap_if.ifType == IFTYPE_TOKENTALK))
			ddp_bit_reverse(&amt_ptr->dest_addr);
		m = amt_ptr->m;
		amt_ptr->m = NULL;
		pat_output(amt_ptr->elapp->pat_id, m,
			   (unsigned char *)&amt_ptr->dest_addr, 0);
		break;
	default :
		/* probing in a weird state?? */
		return(-1);
	}
	return(0);
}



/****************************************************************************
 *  aarp_probe_cmd_in()
 *
 ****************************************************************************/
StaticProc   int	aarp_probe_cmd_in (pkt, elapp)
register aarp_pkt_t	*pkt;
elap_specifics_t	*elapp;
{
	register aarp_amt_t	*amt_ptr;

	switch (probing) {
	case PROBE_TENTATIVE :
		if ((elapp == probe_cb.elapp) &&
			ATALK_EQUAL(pkt->src_at_addr, probe_cb.elapp->cfg.initial_addr)) {
			/* some bozo is probing for address I want... and I 
			 * can't tell him to shove off!
			 */
			atalk_untimeout(aarp_sched_probe, 0, probe_tmo);
			probe_cb.no_of_retries = 0;
			aarp_get_next_node(probe_cb.elapp);
			no_of_nodes_tried++;

			if (no_of_nodes_tried == AARP_MAX_NODES_TRIED) {
				aarp_get_rand_net(probe_cb.elapp);
				aarp_get_rand_node(probe_cb.elapp);
				no_of_nodes_tried = 0;
				no_of_nets_tried++;
			}
			if (no_of_nets_tried == AARP_MAX_NETS_TRIED) {
				/* We have tried enough nodes and nets, give up.
				 */
				probe_cb.error = EADDRNOTAVAIL;
				AARPwakeup(&probe_cb);
				return(0);
			}
			if (aarp_send_probe() == -1) {
				/* expecting aarp_send_probe to fill in 
				 * probe_cb.error
				 */
				AARPwakeup(&probe_cb);
				return(-1);
			}
		} else {
			/* somebody's probing... none of my business yet, so
			 * just ignore the packet
			 */
			return (0);
		}
		break;

	case PROBE_DONE :
		if (ATALK_EQUAL(pkt->src_at_addr, elapp->elap_if.ifThisNode)) {
			if (aarp_send_resp(elapp, pkt) == -1)
				return (-1);
			return (0);
		}
		AMT_LOOK(amt_ptr, pkt->src_at_addr, elapp);

		if (amt_ptr)
		        aarp_delete_amt_info(amt_ptr);
		break;
	default :
		/* probing in a weird state?? */
		return (-1);
	}
	return (0);
}



/****************************************************************************
 *  aarp_chk_addr()
 ****************************************************************************/
StaticProc   int	aarp_chk_addr(p, elapp)
char	                  *p;
register elap_specifics_t *elapp;
{
        register at_ddp_t  *ddp_hdrp;

	ddp_hdrp = (at_ddp_t *)p;

	if ((ddp_hdrp->dst_node == elapp->cfg.node.atalk_node) &&
	    (NET_VALUE(ddp_hdrp->dst_net) == 
		 NET_VALUE(elapp->elap_if.ifThisNode.atalk_net))) {
	        return(0);	    /* exact match in address */
		}

	if (AARP_BROADCAST(ddp_hdrp, elapp)) {
	        return(0);          /* some kind of broadcast address */
	}
	return (AARP_ERR_NOT_OURS); /* not for us */
}



/****************************************************************************
 *  aarp_send_data()
 *
 * remarks :
 *	1. The message coming in would be of the form {802.3, 802.2, ddp,...} 
 *
 *	2. The message coming in would be freed here if transmission goes 
 *	through okay. If an error is returned by aarp_send_data, the caller 
 *	can assume that	the message is not freed.  The exception to 
 *	this scenario is the prepended atalk_addr field.  This field
 * 	will ALWAYS be removed.  If the message is dropped,
 *	it's not an "error".
 *
 ****************************************************************************/

int	aarp_send_data(m, elapp, dest_at_addr, loop)
register gbuf_t	   	   *m;
register elap_specifics_t  *elapp;
struct	 atalk_addr	   *dest_at_addr;
int						loop;			/* if true, loopback broadcasts */
{
	register aarp_amt_t	*amt_ptr;
	register at_ddp_t	*ddp_hdrp;
	int			error;
	int s;
	extern void elap_input(gbuf_t *, elap_specifics_t *, char *);

	if (gbuf_len(m) <= 0)
		ddp_hdrp = (at_ddp_t *)gbuf_rptr(gbuf_cont(m));
	else
		ddp_hdrp = (at_ddp_t *)gbuf_rptr(m);

	if ((ddp_hdrp->dst_node == ddp_hdrp->src_node) &&
	    (NET_VALUE(ddp_hdrp->dst_net)  == NET_VALUE(ddp_hdrp->src_net))) {
	        /*
		 * we're sending to ourselves
		 * so loop it back upstream
		 */
		elap_input(m, elapp, NULL);
		return(0);
	}
	ATDISABLE(s, arpinp_lock);
	AMT_LOOK(amt_ptr, *dest_at_addr, elapp);


	if (amt_ptr) {
	        if (amt_ptr->m) {
		        /*
			 * there's already a packet awaiting transmission, so
			 * drop this one and let the upper layer retransmit
			 * later.
			 */
			ATENABLE(s, arpinp_lock);
		        gbuf_freel(m);
			return (0);
		}
		ATENABLE(s, arpinp_lock);
		return (pat_output(elapp->pat_id, m,
				   (unsigned char *)&amt_ptr->dest_addr, 0));
        }
	/*
	 * either this is a packet to be broadcasted, or the address
	 * resolution needs to be done
	 */
	if (AARP_BROADCAST(ddp_hdrp, elapp)) {
	        gbuf_t	             *newm = 0;
		struct	etalk_addr   *dest_addr;

		ATENABLE(s, arpinp_lock);
		dest_addr =  &elapp->cable_multicast_addr;
		if (loop)
			newm = (gbuf_t *)gbuf_dupm(m);

		if ( !(error = pat_output(elapp->pat_id, m,
					  (unsigned char *)dest_addr, 0))) { 
			/*
			 * The message transmitted successfully;
			 * Also loop a copy back up since this
			 * is a broadcast message.
			 */
			if (loop) {
				if (newm == NULL)
			        return (error);
				elap_input(newm, elapp, NULL);
			} /* endif loop */
		} else {
		        if (newm)
			        gbuf_freem(newm);
		}
		return (error);
	}
	NEW_AMT(amt_ptr, *dest_at_addr,elapp);

        if (amt_ptr->m) {
	        /*
		 * no non-busy slots available in the cache, so
		 * drop this one and let the upper layer retransmit
		 * later.
		 */
		ATENABLE(s, arpinp_lock);
	        gbuf_freel(m);
		return (0);
	}
	amt_ptr->dest_at_addr = *dest_at_addr;
	amt_ptr->dest_at_addr.atalk_unused = 0;
	amt_ptr->last_time = time.tv_sec;
	amt_ptr->m = m;
	amt_ptr->elapp = elapp;
	amt_ptr->no_of_retries = 0;
	ATENABLE(s, arpinp_lock);

	if ((error = aarp_send_req(amt_ptr))) {
		aarp_delete_amt_info(amt_ptr);
		return(error);
	}
	return(0);
}



/****************************************************************************
 * aarp_send_resp()
 *
 * remarks :
 *	The pkt being passed here is only to "look at".  It should neither
 *	be used for transmission, nor freed.  Its contents also must not be
 *	altered.
 *
 ****************************************************************************/
StaticProc   int	aarp_send_resp(elapp, pkt)
register elap_specifics_t   *elapp;
aarp_pkt_t		    *pkt;
{
	register aarp_pkt_t	*new_pkt;
	register gbuf_t		*m;

	if ((m = gbuf_alloc(AT_WR_OFFSET+sizeof(aarp_pkt_t), PRI_MED)) == NULL) {
		return (-1);
	}
	gbuf_rinc(m,AT_WR_OFFSET);
	gbuf_wset(m,0);

	new_pkt = (aarp_pkt_t *)gbuf_rptr(m);
	aarp_build_pkt(new_pkt, elapp);

	new_pkt->aarp_cmd = AARP_RESP_CMD;
	new_pkt->dest_addr =  pkt->src_addr;
	new_pkt->dest_at_addr = pkt->src_at_addr;
	new_pkt->dest_at_addr.atalk_unused = 0;
	new_pkt->src_at_addr = elapp->elap_if.ifThisNode;
	new_pkt->src_at_addr.atalk_unused = 0;
	gbuf_winc(m,sizeof(aarp_pkt_t));
	if ((elapp->elap_if.ifType == IFTYPE_FDDITALK)
		|| (elapp->elap_if.ifType == IFTYPE_TOKENTALK))
		ddp_bit_reverse(&new_pkt->dest_addr);

	if (pat_output(elapp->pat_id, m, (unsigned char *)&new_pkt->dest_addr,
		       AARP_AT_TYPE))
	        return(-1);
	return(0);
}



/****************************************************************************
 * aarp_send_req()
 *
 ****************************************************************************/

StaticProc   int	aarp_send_req (amt_ptr)
register aarp_amt_t 	*amt_ptr;
{
	register aarp_pkt_t  *pkt;
	register gbuf_t	     *m;
	int	             error;

	if ((m = gbuf_alloc(AT_WR_OFFSET+sizeof(aarp_pkt_t), PRI_MED)) == NULL) {
		return (ENOBUFS);
	}
	gbuf_rinc(m,AT_WR_OFFSET);
	gbuf_wset(m,0);

	pkt = (aarp_pkt_t *)gbuf_rptr(m);
	aarp_build_pkt(pkt, amt_ptr->elapp);

	pkt->aarp_cmd = AARP_REQ_CMD;
	pkt->dest_addr = et_zeroaddr;
	pkt->dest_at_addr = amt_ptr->dest_at_addr;
	pkt->dest_at_addr.atalk_unused = 0;
	pkt->src_at_addr = amt_ptr->elapp->elap_if.ifThisNode;
	gbuf_winc(m,sizeof(aarp_pkt_t));
	
	amt_ptr->no_of_retries++;
	amt_ptr->tmo = atalk_timeout(aarp_sched_req, amt_ptr, AARP_REQ_TIMER_INT);
	error = pat_output(amt_ptr->elapp->pat_id, m,
			   (unsigned char *)&amt_ptr->elapp->cable_multicast_addr, AARP_AT_TYPE);
	if (error)
	{
		atalk_untimeout(aarp_sched_req, amt_ptr, amt_ptr->tmo);
		amt_ptr->tmo = 0;
		return(error);
	}

	return(0);
}



/****************************************************************************
 * aarp_send_probe()
 *
 ****************************************************************************/
StaticProc  int	aarp_send_probe()
{
	register aarp_pkt_t  *pkt;
	register gbuf_t	     *m;

	if ((m = gbuf_alloc(AT_WR_OFFSET+sizeof(aarp_pkt_t), PRI_MED)) == NULL) {
		probe_cb.error = ENOBUFS;
		return (-1);
	}
	gbuf_rinc(m,AT_WR_OFFSET);
	gbuf_wset(m,0);
	pkt = (aarp_pkt_t *)gbuf_rptr(m);
	aarp_build_pkt(pkt, probe_cb.elapp);

	pkt->aarp_cmd = AARP_PROBE_CMD;
	pkt->dest_addr = et_zeroaddr;
	pkt->src_at_addr.atalk_unused = 0;
	pkt->src_at_addr = probe_cb.elapp->cfg.initial_addr;
	pkt->dest_at_addr = probe_cb.elapp->cfg.initial_addr;
	gbuf_winc(m,sizeof(aarp_pkt_t));

	probe_cb.error = pat_output(probe_cb.elapp->pat_id, m,
		(unsigned char *)&probe_cb.elapp->cable_multicast_addr, AARP_AT_TYPE);
	if (probe_cb.error) {
		return(-1);
	}

	probing = PROBE_TENTATIVE;
	probe_cb.no_of_retries++;
	probe_tmo = atalk_timeout(aarp_sched_probe, 0, AARP_PROBE_TIMER_INT);

	return(0);
}



/****************************************************************************
 * aarp_lru_entry()
 *
 ****************************************************************************/

StaticProc   aarp_amt_t	*aarp_lru_entry(at)
register aarp_amt_t	*at;
{
	register aarp_amt_t  *at_ret;
	register int	     i;

	at_ret = at;

	for (i = 1, at++; i < AMT_BSIZ; i++, at++) {
		if (at->last_time < at_ret->last_time && (at->m == NULL))
			at_ret = at;
	}
        return(at_ret);
}



/****************************************************************************
 * aarp_glean_info()
 *
 ****************************************************************************/

StaticProc   int	aarp_glean_info(pkt, elapp)
register aarp_pkt_t	*pkt;
elap_specifics_t	*elapp;
{
    register aarp_amt_t   *amt_ptr;
	int s;

	ATDISABLE(s, arpinp_lock);
	AMT_LOOK(amt_ptr, pkt->src_at_addr, elapp);

	if (amt_ptr == NULL) {
	        /*
		 * amt entry for this address doesn't exist, add it to the cache
	         */
		NEW_AMT(amt_ptr, pkt->src_at_addr,elapp); 

		if (amt_ptr->m)
		{
		ATENABLE(s, arpinp_lock);
		        return(0);     /* no non-busy slots available in the cache */
		}
		amt_ptr->dest_at_addr = pkt->src_at_addr;
		amt_ptr->dest_at_addr.atalk_unused = 0;
		amt_ptr->last_time = (int)random();
	}
	/*
	 * update the ethernet address
	 * in either case
	 */
	amt_ptr->dest_addr = pkt->src_addr;
	if ((elapp->elap_if.ifType == IFTYPE_FDDITALK)
		|| (elapp->elap_if.ifType == IFTYPE_TOKENTALK))
		ddp_bit_reverse(&amt_ptr->dest_addr);
	ATENABLE(s, arpinp_lock);
	return(1);
}


/****************************************************************************
 * aarp_delete_amt_info()
 *
 ****************************************************************************/

StaticProc   int	aarp_delete_amt_info(amt_ptr)
register aarp_amt_t	*amt_ptr;
{
	register s;
	register gbuf_t		*m;
	ATDISABLE(s, arpinp_lock);
	amt_ptr->last_time = 0;
	ATALK_ASSIGN(amt_ptr->dest_at_addr, 0, 0, 0);
	amt_ptr->no_of_retries = 0;

	if (amt_ptr->m) {
	    m = amt_ptr->m;
	    amt_ptr->m = NULL;    
 	    ATENABLE(s, arpinp_lock);
	    gbuf_freel(m);
        }
	else
		ATENABLE(s, arpinp_lock);
	return(0);
}



/****************************************************************************
 * aarp_sched_probe()
 *
 ****************************************************************************/

StaticProc  int	aarp_sched_probe()
{
	if (probe_cb.no_of_retries != AARP_MAX_PROBE_RETRIES) {
		if (aarp_send_probe() == -1)
			AARPwakeup(&probe_cb);
	} else {
		probe_cb.error = 0;
		AARPwakeup(&probe_cb);
	}

	return(0);
}



/****************************************************************************
 * aarp_build_pkt()
 *
 ****************************************************************************/

StaticProc   int	aarp_build_pkt(pkt, elapp)
register aarp_pkt_t	*pkt;
elap_specifics_t	*elapp;
{	extern void elap_get_addr(int, unsigned char *);

	pkt->hardware_type = AARP_ETHER_HW_TYPE;
	pkt->stack_type = AARP_AT_PROTO;
	pkt->hw_addr_len = AARP_ETHER_ADDR_LEN;
	pkt->stack_addr_len = AARP_AT_ADDR_LEN;
	elap_get_addr(elapp->pat_id, pkt->src_addr.etalk_addr_octet);
	if ((elapp->elap_if.ifType == IFTYPE_FDDITALK)
		|| (elapp->elap_if.ifType == IFTYPE_TOKENTALK))
		ddp_bit_reverse(pkt->src_addr.etalk_addr_octet);
	return(0);
}



/****************************************************************************
 * aarp_sched_req()
 *
 ****************************************************************************/

StaticProc   int	aarp_sched_req(amt_ptr)
register aarp_amt_t	*amt_ptr;
{
	int s;

	ATDISABLE(s, arpinp_lock);
	if (amt_ptr->tmo == 0)
	{
		ATENABLE(s, arpinp_lock);
		return(0);
	}
	if (amt_ptr->no_of_retries < AARP_MAX_REQ_RETRIES) {
		ATENABLE(s, arpinp_lock);
		if (aarp_send_req(amt_ptr) == 0)
			return(0);
		ATDISABLE(s, arpinp_lock);
	}
	ATENABLE(s, arpinp_lock);
	aarp_delete_amt_info(amt_ptr);
	return(0);
}



/****************************************************************************
 * aarp_get_rand_node()
 *
 ****************************************************************************/
StaticProc   int	aarp_get_rand_node(elapp)
elap_specifics_t	*elapp;
{
	register u_char	node;

	/*
	 * generate a starting node number in the range 1 thru 0xfd.
	 * we use this as the starting probe point for a given net
	 * To generate a different node number each time we call
         * aarp_get_next_node
	 */
	node = ((u_char)(random() & 0xff)) % 0xfd + 2;
	
	elapp->cfg.initial_addr.atalk_node = node;
	return(0);
}



StaticProc   int	aarp_get_next_node(elapp)
elap_specifics_t	*elapp;
{
	register u_char	node = elapp->cfg.initial_addr.atalk_node;

	/*
	 * return the next node number in the range 1 thru 0xfd.
	 */
	node = (node == 0xfd) ? (1) : (node+1);

	elapp->cfg.initial_addr.atalk_node = node;
	return(0);
}





/****************************************************************************
 * aarp_get_rand_net()
 *
 ****************************************************************************/
StaticProc   int	aarp_get_rand_net(elapp)
register elap_specifics_t	*elapp;
{
	register at_net_al	 last_net, new_net;

	if (elapp->elap_if.ifThisCableStart) {
		last_net = NET_VALUE(elapp->cfg.initial_addr.atalk_net);
		/*
		 * the range of network numbers valid for this
		 * cable is known.  Try to choose a number from
		 * this range only.  
		 */
		new_net= ((at_net_al)random() & 0xffff);
		/* two-byte random number generated... now fit it in 
		 * the prescribed range 
		 */
		new_net = new_net % (unsigned) (elapp->elap_if.ifThisCableEnd - 
				     elapp->elap_if.ifThisCableStart + 1)
			  + elapp->elap_if.ifThisCableStart;

		if (new_net == last_net) {
		        if (new_net == elapp->elap_if.ifThisCableEnd)
			        new_net = elapp->elap_if.ifThisCableStart;
			else
			        new_net++;
		}
		NET_ASSIGN(elapp->cfg.initial_addr.atalk_net, new_net);
	} else {
		/* The range of valid network numbers for this cable
		 * is not known... choose a network number from
		 * startup range.
		 */
		last_net = (NET_VALUE(elapp->cfg.initial_addr.atalk_net) & 0x00ff);
		new_net = (at_net_al)random() & 0x00ff;

		if (new_net == last_net)
		        new_net++;
		if (new_net == 0xff)
		        new_net = 0;
		NET_ASSIGN(elapp->cfg.initial_addr.atalk_net, (DDP_STARTUP_LOW | new_net));
	}
	return(0);
}


int
getAarpTableSize(elapId)
int		elapId;			/* elap_specifics array index (should be
						 * changed when we add a non-ethernet type
						 * of I/F to the mix. Unused for now.
						 */
{
	return(AMTSIZE);
}

int
getPhysAddrSize(elapId)
int		elapId;			/* elap_specifics array index (should be
						 * changed when we add a non-ethernet type
						 * of I/F to the mix. Unused for now.
						 */
{
	return(AARP_ETHER_ADDR_LEN);
}

#define ENTRY_SIZE 	sizeof(struct atalk_addr) + sizeof(struct etalk_addr)

snmpAarpEnt_t *
getAarp(elapId)
int		*elapId;		/* I/F table to retrieve & table
						   size entries on return */

/* gets aarp table for specified interface and builds
   a table in SNMP expected format. Returns pointer to said
   table and sets elapId to byte size of used portion of table
*/
{
	int i, cnt=0;
	aarp_amt_t *amtp;
	static snmpAarpEnt_t  snmp[AMTSIZE];
	snmpAarpEnt_t  *snmpp;


	if (*elapId <0 || *elapId >= IF_TYPE_ET_MAX
/*	   || elap_specifics[*elapId].elap_if.ifState == LAP_OFFLINE */
	   )
	   return NULL;
	
	
	for (i=0, amtp = et_aarp_amt[*elapId],snmpp = snmp;
		 i < AMTSIZE; i++,amtp++)	{

					/* last_time will be 0 if entry was never used */
		if (amtp->last_time) {
				/* copy just network & mac address.
				 * For speed, we assume that the atalk_addr
				 * & etalk_addr positions in the aarp_amt_t struct
				 * has not changed and copy both at once
				 */
			bcopy(&amtp->dest_at_addr, &snmpp->ap_ddpAddr, ENTRY_SIZE);
			snmpp++;
			cnt++;
			
		}
	}
	*elapId = cnt;
	return(snmp);
}
/*#endif *//*  COMMENTED_OUT */

