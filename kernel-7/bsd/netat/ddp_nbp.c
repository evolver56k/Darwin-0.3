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
 *	Copyright (c) 1988, 1989, 1997, 1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */


#include <sysglue.h>

/* reaching for DDP and NBP headers in the datagram */
#define DATA_DDP(mp)	((at_ddp_t *)(gbuf_rptr(mp)))
#define	DATA_NBP(mp)	((at_nbp_t *)((DATA_DDP(mp))->data))

/* Get to the nve_entry_t part ofthe buffer */
#define	NVE_ENTRY(mp)	(nve_entry_t *)(gbuf_rptr(mp))

#include <string.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/nbp.h>
#include <at/zip.h>
#include <rtmp.h>
#include <lap.h>
#include <at/elap.h> /* router */
#include <at/at_lap.h>
#include <routing_tables.h>	/* router */
#include <at_elap.h> 		/* router */
#include <at_ddp.h>			/* for FROM_US */
#include <at_snmp.h>

#ifndef	MIN
#define	MIN(a,b)	((a)>(b)?(b):(a))
#endif

#include "nbp.h"

#define	errno	nbperrno
#define NBP_DEBUG  0

	/* externs */
extern at_state_t 	at_state, *at_statep;
extern at_if_t 		*ifID_table[];


	/* statics */
static	nve_entry_t	name_registry = {&name_registry, &name_registry};
static	at_nvestr_t	this_zone;
static	int		Extended_net;
static	int		errno;
static  gbuf_t  *lzones=0;	/* head of local zones list */
static int	lzonecnt=0;		/* # zones stored in lzones	*/
static u_int  hzonehash=0;	/* hash val of home zone */
atlock_t 	nve_lock;
static int	nve_lock_pri;

static int	nbp_lkup_reply(nbp_req_t *, nve_entry_t *);
static int	nbp_strcmp(at_nvestr_t *, at_nvestr_t *, u_char);
static int	nbp_setup_resp(nbp_req_t *, int);
static int	nbp_send_resp(nbp_req_t *);
static int	nbp_insert_entry(nve_entry_t *);
static int	nbp_enum_gen(nve_entry_t *);

	/* macros */
#define NBP2ENTITY(nbp)  &nbp->tuple[0].en_u.en_se.entity
#define NVE_LOCK nve_lock

	/* prototypes */
at_nvestr_t *getRTRLocalZone(if_zone_t *);
at_nvestr_t *getSPLocalZone(int);
at_nvestr_t *getLocalZone(int);
int nbp_add_multicast( at_nvestr_t	*, at_if_t *);
int isZoneLocal(at_nvestr_t *);
u_int	nbp_strhash (at_nvestr_t *);

int sethzonehash(elapp)
elap_specifics_t *elapp;
{
	if (elapp->cfg.zonename.len)  {
		hzonehash = nbp_strhash(&elapp->cfg.zonename);
	}
}


int	nbp_init ()
{
	void		nbp_input();
	at_ddp_cfg_t	ddp_cfg;

	/* Initialize Extended_net */
	ddp_get_cfg (&ddp_cfg, 0);
	if (ddp_cfg.flags & AT_IFF_LOCALTALK)
		Extended_net = 0;
	else
		Extended_net = 1;

	ATLOCKINIT(nve_lock);
	return ((int)nbp_input);
}

void	nbp_shutdown()
{
	/* delete all NVE's and release buffers */
	register nve_entry_t	*nve_entry, *next_nve;
	extern	 void	nbp_delete_entry();

	ATDISABLE(nve_lock_pri,NVE_LOCK);
	for (nve_entry = name_registry.fwd, next_nve = nve_entry->fwd;
		nve_entry != &name_registry;
		nve_entry = next_nve, next_nve = nve_entry->fwd) {
		nbp_delete_entry(nve_entry);
	}
	ATENABLE(nve_lock_pri,NVE_LOCK);

	if (lzones) {
		gbuf_freem(lzones);
		lzones = NULL; 
	}
} /* nbp_shutdown */

void	nbp_input (m, ifID)
register gbuf_t	*m;
register at_if_t *ifID;
{
	void		zip_handler(), nbp_handler(), rtmp_handler();

	switch (gbuf_type(m)) {

	/* *** fix this later: this code may no longer be needed *** */
	case MSG_ERROR :
		if (*gbuf_rptr(m) == ESHUTDOWN) {
			/* DDP shutting down, clean up the name registry */
			nbp_shutdown();
		}
		break;
	/* *** end code that may no longer be needed *** */

	case MSG_DATA :
		switch (((at_ddp_t *)(DATA_DDP(m)))->type) {
		case NBP_DDP_TYPE :
			nbp_handler (m, ifID);
			return;
		case ZIP_DDP_TYPE :
			dPrintf(D_M_NBP_LOW, D_L_WARNING,
				("nbp_input: calling zip_handler\n"));
			zip_handler (m);
			break;
		case RTMP_DDP_TYPE :  /* applicable only in case of LocalTalk */
			dPrintf(D_M_NBP_LOW,D_L_WARNING,
				("nbp_input: calling rtmp_handler\n"));
			rtmp_handler (m);
			break;
		}
		break;
	default :
		dPrintf(D_M_NBP, D_L_WARNING,
			("nbp_input: default_handler type=%d\n", gbuf_type(m)));
		break;
	}
	gbuf_freem(m);
}

static
u_char *nbp2zone(nbp, maxp)
	at_nbp_t *nbp;
	u_char *maxp;
{

	u_char *p;

	p = (u_char*)NBP2ENTITY(nbp);	/* p -> object */
	if (p >= maxp) return NULL;
	p += (*p +1);					/* p -> type   */
	if (p >= maxp) return NULL;
	p += (*p +1);					/* p -> zone   */
	if (p >= maxp) return NULL;
	if ((p + *p) >= maxp) return NULL;
	return(p);
}

static	void	nbp_handler (m, ifID)
register gbuf_t	*m;
register at_if_t *ifID;

{
	register at_nbp_t	*nbp = DATA_NBP(m);
	register nve_entry_t	*nve_entry, *next_nve;
	register RT_entry	*rt;
	register int ddpSent = FALSE; 	/* true if we re-sent this pkt (don't free) */
	struct etalk_addr mcastAddr;
	nbp_req_t	nbp_req;
	nve_entry_t	*nbp_search_nve();
	void		nbp_delete_entry();
	at_ddp_t	*ddp;
	u_char *p;
	
	/* Some initializations */
	nbp_req.response = NULL;
	nbp_req.request = m;
	nbp_req.space_unused = nbp_req.flags = 0;


	dPrintf(D_M_NBP_LOW, D_L_USR1,
		("nbp_handler control:%d tuplecount:%d id:%d\n",
		nbp->control, nbp->tuple_count, nbp->at_nbp_id));
	switch (nbp->control) {
	case NBP_LKUP :
	  {
		at_net_al dst_net;
		ddp = DATA_DDP(m);
		dst_net = NET_VALUE(ddp->dst_net);
		dPrintf(D_M_NBP_LOW, D_L_USR2, (" LKUP %s\n",
			ifID != ifID_table[0] ? "non-home" : "home"));
		if ( ROUTING_MODE && (NET_VALUE(ddp->dst_net) != 0)
			&& ((dst_net < ifID->ifThisCableStart)
				|| (dst_net > ifID->ifThisCableEnd)) ) {
			routing_needed(m, ifID, TRUE);
			ddpSent = TRUE;
			break;
		}
	  }

		if (nbp_validate_n_hash (&nbp_req, TRUE, FALSE) == 0) {
			nbp_req.func = nbp_lkup_reply;
			(void) nbp_search_nve(&nbp_req, ifID);
			if (nbp_req.response) {
				nbp_send_resp(&nbp_req);
			}
		}
#ifdef NBP_DEBUG
	{
		char zone[35],object[35],type[35];
		strncpy(zone,nbp_req.nve.zone.str, nbp_req.nve.zone.len);
		strncpy(object,nbp_req.nve.object.str, nbp_req.nve.object.len);
		strncpy(type,nbp_req.nve.type.str, nbp_req.nve.type.len);
		object[nbp_req.nve.object.len] = '\0';
		zone[nbp_req.nve.zone.len] = '\0';
		type[nbp_req.nve.type.len] = '\0';
		if (ifID != ifID_table[0]) 
			dPrintf(D_M_NBP_LOW,D_L_USR2,
				("nbp_LKUP for:%s:%s@%s", object, type, zone));
	}
#endif /* NBP_DEBUG */

		break;
	case NBP_REGISTER :
		/* Whatever the outcome of this operation, we need to return
		 * a response to the requester; so set up the response buffers 
		 */
#ifdef NBP_DEBUG
	{
		at_net_al dst_net,src_net;
		int src_node, dst_node, src_skt, dst_skt;
		ddp = DATA_DDP(m);
		dst_net = NET_VALUE(ddp->dst_net);
		src_net = NET_VALUE(ddp->src_net);
		src_node = ddp->src_node;	
		dst_node = ddp->dst_node;	
		src_skt = ddp->src_socket;	
		dst_skt = ddp->dst_socket;	
		dPrintf(D_M_NBP_LOW,D_L_USR2,
			("nbp reg ddp src:%d.%d.%d ",src_net,src_node,src_skt));
		dPrintf(D_M_NBP_LOW,D_L_USR2,
			("dst: %d.%d.%d\n",dst_net,dst_node,dst_skt));
	}
#endif /* NBP_DEBUG */
		(void) nbp_setup_resp(&nbp_req, 0);
		if (nbp_validate_n_hash (&nbp_req, FALSE, TRUE) != 0) {
			/* bad tuple... send an error message */
			goto bad;
		}



		nbp_req.func = NULL;
		if ((nve_entry =  nbp_search_nve(&nbp_req, NULL)) != NULL) {
			/* We have an entry for the same name in the registry.
			 * This is either a duplicate request (client 
			 * retransmission) or another request. If this is a 
			 * retransmitted request (ie it's for the same socket),
			 * then return ok; else return error.
			 */
			if (*(int *)&nve_entry->address == 
				*(int *)&nbp_req.nve.address) {
				/* Send ok response */
				DATA_NBP(nbp_req.response)->tuple_count = 0;
				nbp_send_resp(&nbp_req);
				break;
			} else {
				errno = EADDRNOTAVAIL;
				goto bad;
			}
		} else {
			/* Normal case; no tuple found for this name, so insert
			 * this tuple in the registry and return ok response.
			 */
#ifdef NBP_DEBUG
		{
			char str[35];
			char object[35];
			strncpy(str,nbp_req.nve.zone.str,nbp_req.nve.zone.len);
			str[nbp_req.nve.zone.len] = '\0';
			strncpy(object,nbp_req.nve.object.str,nbp_req.nve.object.len);
			object[nbp_req.nve.object.len] = '\0';
			dPrintf(D_M_NBP_LOW, D_L_USR4,
				("nbp_register for zone %s (len:%d) obj:%s (len:%d)\n",
				str, nbp_req.nve.zone.len, object,
				nbp_req.nve.object.len));
		}
#endif /* NBP_DEBUG */
  
			if ((nbp_enum_gen(&nbp_req.nve) != 0) || 
				nbp_insert_entry(&nbp_req.nve) != 0)
				goto bad;
			else {
				/* Send okay response */
				DATA_NBP(nbp_req.response)->tuple_count = 0;
				nbp_send_resp(&nbp_req);
				break;
			}
		}
bad:
		DATA_NBP(nbp_req.response)->tuple_count = errno;
		nbp_send_resp(&nbp_req);
		break;
	case NBP_DELETE :
		/* Whatever the outcome of this operation, we need to return
		 * a response to the requester; so set up the response buffers 
		 */
		(void) nbp_setup_resp(&nbp_req, 0);
		if (nbp_validate_n_hash (&nbp_req, FALSE, TRUE) != 0) {
			/* bad tuple... send an error message */
			DATA_NBP(nbp_req.response)->tuple_count = errno;
			nbp_send_resp(&nbp_req);
			break;
		}

		nbp_req.func = NULL;
		if (MULTIHOME_MODE &&
		    nbp_req.nve.zone.len == 1 && 
            nbp_req.nve.zone.str[0] == '*'
           ) {	/* if mhome & *, remove nve from all default zones */

			int			found = FALSE;	/* if any found & deleted */
			at_if_t		**ifID;

			for (ifID=ifID_table; *ifID; ifID++) {
				nbp_req.nve.zone = (*ifID)->ifZoneName;
				if ((nve_entry = nbp_search_nve(&nbp_req, NULL)) == NULL) 
					continue;
				ATDISABLE(nve_lock_pri,NVE_LOCK);
				nbp_delete_entry(nve_entry);
				ATENABLE(nve_lock_pri,NVE_LOCK);
				found = TRUE;
			}
		    nbp_req.nve.zone.len = 1;
            nbp_req.nve.zone.str[0] = '*';
			if (found) 
				DATA_NBP(nbp_req.response)->tuple_count = 0;
			else
				DATA_NBP(nbp_req.response)->tuple_count = EADDRNOTAVAIL;
			nbp_send_resp(&nbp_req);
			break;
		} else {
			if ((nve_entry = nbp_search_nve(&nbp_req, NULL)) == NULL) {
				/* Can't find the tuple we're looking for, send error*/
				DATA_NBP(nbp_req.response)->tuple_count = EADDRNOTAVAIL;
				nbp_send_resp(&nbp_req);
			} else {
				/* Normal case; tuple found for this name, so delete
				 * the entry from the registry and return ok response.
				 */
				ATDISABLE(nve_lock_pri,NVE_LOCK);
				nbp_delete_entry(nve_entry);
				ATENABLE(nve_lock_pri,NVE_LOCK);
				DATA_NBP(nbp_req.response)->tuple_count = 0;
				nbp_send_resp(&nbp_req);
			}
		}
		break;
	case NBP_CLOSE_NOTE :
		ATDISABLE(nve_lock_pri,NVE_LOCK);
		for (nve_entry = name_registry.fwd, next_nve = nve_entry->fwd;
			nve_entry != &name_registry;
			nve_entry = next_nve, next_nve = nve_entry->fwd) {
			if ( DATA_DDP(m)->src_socket == nve_entry->address.socket 	&&
				 (*(int *)gbuf_wptr(m) == nve_entry->pid) 	 			&&
				 ot_ddp_check_socket(nve_entry->address.socket, 
					nve_entry->pid) < 2) {
					nbp_delete_entry(nve_entry);
			}
		}
		ATENABLE(nve_lock_pri,NVE_LOCK);
		break;
	case NBP_FWDRQ: 
		{
 		register int	zhome=0;		/* true if home zone == destination zone */
 		register int	zno, i;
 		register  gbuf_t	*m2;
		register error_found =0;
		register at_if_t *ifIDorig;

		if (MULTIHOME_MODE || !ROUTING_MODE)				/* for routers only! */
			break;

		ddp = DATA_DDP(m);

		ifIDorig = ifID;
		ifID= NULL;
		for (i = 0 ; i < RT_MAXENTRY; i++) {
			rt = &RT_table[i];
			if ((rt->EntryState & RTE_STATE_PERMANENT) &&
				NET_VALUE(ddp->dst_net) >= rt->NetStart && 
				NET_VALUE(ddp->dst_net) <=	rt->NetStop
			   ) {
			   	/* sanity check */
			   	if (rt->NetPort >= IF_TOTAL_MAX) {
					dPrintf(D_M_NBP,D_L_ERROR,
						("nbp_handler:FWDREQ: bad port# from RT_table\n"));
					error_found = TRUE;
					break;
				}
			 	ifID = ifID_table[rt->NetPort];
				if (!ifID || !IFID_VALID(ifID)) {
					dPrintf(D_M_NBP,D_L_ERROR,
						("nbp_handler:FWDREQ: ifID %s\n", 
						!ifID ? "not found" : "invalid"));
					error_found = TRUE;
					break;
				}
				if (ifID->ifState == LAP_OFFLINE) {
					dPrintf(D_M_NBP,D_L_ERROR,
						("nbp_handler:FWDREQ: ifID offline (port %d)\n",
					  	rt->NetPort));
					error_found = TRUE;
					break;
				}
			   break;
			}
		}
		if (error_found) /* the port is not correct */
			break;

		if (!ifID) { /* this packet is not for us, let the routing engine handle it  */
			routing_needed(m, ifIDorig, TRUE);
			ddpSent= TRUE;
			break;
		}

		/* 
		 * At this point, we have a valid Forward request for one of our 
		 * direclty connected port. Convert it to a NBP Lookup
		 */

		nbp->control = NBP_LKUP;
	 	NET_ASSIGN(ddp->dst_net, 0);
	 	ddp->dst_node = 255;


 /*### LD 01/18/94 Check if the dest is also the home zone. */
 
 		p = nbp2zone(nbp, gbuf_wptr(m));
 		if ((p == NULL) || !(zno = zt_find_zname(p))) {
 			dPrintf(D_M_NBP,D_L_WARNING,
 				("nbp_handler: FWDRQ:zone not found\n"));
			break;
 		}
		if (isZoneLocal((at_nvestr_t*)p)) 
			zhome = TRUE;				/* one of our  ports is in destination zone */
		if (!zt_get_zmcast(ifID, p, &mcastAddr)) {
			dPrintf(D_M_NBP,D_L_ERROR,
				("nbp_handler: FDWREQ:zt_get_zmcast error\n"));
			break;
		}
			

 		if (zhome) { /*### LD 01/18/95  In case our home is here, call back nbp */
 
 			if (!(m2 = (gbuf_t *)gbuf_copym((gbuf_t *)m))) {
 				dPrintf(D_M_NBP,D_L_ERROR, 
 					("nbp_handler: FWDRQ:gbuf_copym failed\n"));
 				break;
 			}
 
 			ddp = DATA_DDP(m2);
 			nbp = DATA_NBP(m2);
 			nbp->control  = NBP_LKUP;
 	 		NET_ASSIGN(ddp->dst_net, 0);
 	 		ddp->dst_node = 255;
 			dPrintf(D_M_NBP,D_L_INFO, 
 				("nbp_handler: FWDRQ:loop back for us\n"));
 			nbp_handler(m2, ifID_table[IFID_HOME]);
 		}
 
		if ((ifID->ifType == IFTYPE_FDDITALK)
			|| (ifID->ifType == IFTYPE_TOKENTALK))
			ddp_bit_reverse(&mcastAddr);
		ddp_router_output(m, ifID, ET_ADDR,NULL,NULL, &mcastAddr);
		ddpSent = TRUE;
		}
		break;

	case NBP_BRRQ:
		{
		register int	zno;			/* zone table entry numb */
		register int 	ztind;			/* zone bitmap index into RT_entry */
		register int	ztbit;			/* zone bit to check within above index */
		register int	zhome=0;		/* true if home zone == destination zone */
		register int	i;
		register  gbuf_t	*m2, *m3;
		register int fromUs = FALSE;
		register at_socket ourSkt;			/* originating skt */

		ddp = DATA_DDP(m);				/* for router & MH local only */
		if ((MULTIHOME_MODE && !FROM_US(ddp))&& !ROUTING_MODE) {
			dPrintf(D_M_NBP,D_L_USR2,
				("nbp_handler: BRREQ:non router or MH local\n"));

			break;
		}
 		p = nbp2zone(nbp, gbuf_wptr(m));
		if ((p == NULL) || !(zno = zt_find_zname(p))) {
			break;
		}
		if (MULTIHOME_MODE && ifID->ifRouterState == NO_ROUTER) {
			((at_nvestr_t*)p)->len = 1;
			((at_nvestr_t*)p)->str[0] = '*';
		}
		if (isZoneLocal((at_nvestr_t*)p)) {
			zhome = TRUE;				/* one of our  ports is in destination zone */
		}
		if (FROM_US(ddp)){	/* save, before we munge it */
			fromUs = TRUE;
			ourSkt = ddp->src_socket;
			dPrintf(D_M_NBP,D_L_USR2,
				("nbp_handler:BRRQ from us net:%d\n",
				(int)NET_VALUE(ddp->src_net)));
		}
			/* from ZT_CLR_ZMAP */
		i = zno - 1;
		ztind = i >> 3;
		ztbit = 0x80 >> (i % 8);
		for (i=0,rt=RT_table; i<RT_MAXENTRY; i++,rt++) {
			if (!(rt->ZoneBitMap[ztind] & ztbit)) 		/* if zone not in route, skip*/
				continue;
/*		dPrintf(D_M_NBP, D_L_USR3,
			("nbp_handler: BRREQ: port:%d, entry %d\n",
				rt->NetPort, i));
*/

			ifID = ifID_table[rt->NetPort];
			if (!ifID || !IFID_VALID(ifID)) {
				dPrintf(D_M_NBP, D_L_ERROR, 
					("nbp_handler:BRRQ: ifID %s\n", 
					!ifID ? "not found" : "invalid"));
				break;
			}

			ddp = DATA_DDP(m);
			ddp->src_node = ifID->ifThisNode.atalk_node;
			NET_NET(ddp->src_net,  ifID->ifThisNode.atalk_net);
			ddp->src_socket = NBP_SOCKET;
			if (!(m2 = (gbuf_t *)gbuf_copym((gbuf_t *)m))) {
				dPrintf(D_M_NBP,D_L_ERROR, 
					("nbp_handler: BRREQ:gbuf_copym failed\n"));
				break;
			}

			ddp = DATA_DDP(m2);
			nbp = DATA_NBP(m2);
/*			nbp->tuple[0].enu_addr.socket = NBP_SOCKET; */
			if (MULTIHOME_MODE && fromUs ) {
				/* set the return address of the lookup to that of the
				   interface it's going out on so that replies come back
				   on that net */
				dPrintf(D_M_NBP,D_L_USR3, 
				   ("nbp_handler: BRREQ: src changed to %d.%d.%d\n",
					NET_VALUE(ifID->ifThisNode.atalk_net),
					ifID->ifThisNode.atalk_node, ourSkt));
				NET_NET(nbp->tuple[0].enu_addr.net,ifID->ifThisNode.atalk_net);
				nbp->tuple[0].enu_addr.node = ifID->ifThisNode.atalk_node;
				nbp->tuple[0].enu_addr.socket = ourSkt; 
				ddp->src_socket = NBP_SOCKET;
			}
			else
				dPrintf(D_M_NBP, D_L_USR3, 
				   ("nbp_handler: BRREQ: not from us\n"));

			dPrintf(D_M_NBP, D_L_USR3,
				("nbp_handler dist:%d\n", rt->NetDist));
			if (rt->NetDist == 0) {			/* if direct connect, *we* do the LKUP */
				nbp->control  = NBP_LKUP;
	 			NET_ASSIGN(ddp->dst_net, 0);
	 			ddp->dst_node = 255;
				if (!zt_get_zmcast(ifID, p, &mcastAddr)) {
					dPrintf(D_M_NBP,D_L_ERROR, 
						("nbp_handler: BRRQ:zt_get_zmcast error\n"));
					break;
				}
				if ((ifID->ifType == IFTYPE_FDDITALK)
					|| (ifID->ifType == IFTYPE_TOKENTALK))
					ddp_bit_reverse(&mcastAddr);
				ddp_router_output(m2, ifID, ET_ADDR, NULL, NULL, &mcastAddr); 
			}
			else {							/* else fwd to router */
				ddp->dst_node = 0;
				if (rt->NetStart == 0)		/* if Ltalk */
					NET_ASSIGN(ddp->dst_net, rt->NetStop);
				else	
					NET_ASSIGN(ddp->dst_net, rt->NetStart);
				nbp->control  = NBP_FWDRQ;
				ddp_router_output(m2, ifID, AT_ADDR, rt->NextIRNet, rt->NextIRNode, NULL); 
			}
		}
		if (!zhome)
			break;

		if (!(m3 = (gbuf_t *)gbuf_copym((gbuf_t *)m))) {
			dPrintf(D_M_NBP,D_L_ERROR, 
				("nbp_handler: BRREQ:gbuf_copym failed\n"));
			break;
		}

		ddp = DATA_DDP(m3);
		nbp = DATA_NBP(m3);
				
		nbp->control  = NBP_LKUP;
	 	NET_ASSIGN(ddp->dst_net, 0);
	 	ddp->dst_node = 255;
 		dPrintf(D_M_NBP,D_L_INFO, 
			("nbp_handler: BRRQ:loop back for us\n"));
		nbp_handler(m3, ifID_table[IFID_HOME]);
		break;
		}

		case NBP_LKUP_REPLY:
		
		if (MULTIHOME_MODE || !ROUTING_MODE)				/* for routers only! */
				break;

			ddp = DATA_DDP(m);
			
			dPrintf(D_M_NBP,D_L_WARNING, 
				("nbp_handler: routing needed for LKUP_REPLY: from %d.%d\n",
				NET_VALUE(ddp->src_net), ddp->src_node));
			routing_needed(m, ifID, TRUE);
			ddpSent = TRUE;
			break;
		
	default :

		dPrintf(D_M_NBP,D_L_ERROR, 
			("nbp_handler: unhandled pkt: type:%d\n", nbp->control));

		routing_needed(m, ifID, TRUE);
		ddpSent = TRUE;
		break;
	} /* switch control */
	if (!ddpSent)
		gbuf_freem(m);
	return;
}


static	void	zip_handler (m)
gbuf_t	*m;
{
	at_ddp_t	   *ddp = DATA_DDP(m);
	register at_zip_t  *zip = ZIP_ZIP(ddp);
	register char	old_zone_len;
	register char	new_zone_len;
	register char	mcast_len;
	char		*new_zone;

	switch (zip->command) {
	case ZIP_NOTIFY :
		/* ZipNotify packet forwarded by ZIP module...
		 * change the zone name this_zone.
		 */
		old_zone_len = zip->data[4];
		mcast_len = zip->data[4+old_zone_len+1];
		new_zone_len = zip->data[4+old_zone_len+1+mcast_len+1];
		new_zone = &zip->data[4+old_zone_len+1+mcast_len+1];
		bcopy(new_zone, &this_zone, new_zone_len + 1);
		break;
	default :
		/* ignore all the packets we don't know about */
		break;
	} /* switch command */
	return;
}


static	void	rtmp_handler (m)
gbuf_t	*m;
{
	at_ddp_t	*ddp = DATA_DDP(m);
	at_rtmp		*rtmp = (at_rtmp *)ddp->data;
	at_net		new_net;
	register nve_entry_t	*nve_entry;

	NET_NET(new_net, rtmp->at_rtmp_this_net);
	for (nve_entry = name_registry.fwd; nve_entry != &name_registry ;
		nve_entry = nve_entry->fwd)
		NET_NET(nve_entry->address.net, new_net);
}


static	int	nbp_validate_n_hash (nbp_req, wild_ok, checkLocal)
register nbp_req_t	*nbp_req;
register int		wild_ok;
register int		checkLocal;	/* if true check if local zone */
{
        register at_nvestr_t	*object, *type, *zone;
	at_nbptuple_t	*tuple;
	register int	i, part_wild;
	u_int		nbp_strhash();

	tuple = DATA_NBP(nbp_req->request)->tuple;
	nbp_req->flags = 0;
#ifdef COMMENTED_OUT
	{
		int net,node,skt;
		net = NET_VALUE(tuple->enu_addr.net);
		node = tuple->enu_addr.node;
		skt = tuple->enu_addr.socket;
		dPrintf(D_M_NBP_LOW,D_L_USR4,
			("nbp_validate: tuple addr:%d:%d:%d\n",net,node,skt));
	}
#endif /* COMMENTED_OUT */

	object = (at_nvestr_t *)tuple->enu_name;
	type = (at_nvestr_t *)(&object->str[object->len]);
	zone = (at_nvestr_t *)(&type->str[type->len]);
	
	if (object->len > NBP_NVE_STR_SIZE || type->len > NBP_NVE_STR_SIZE || 
		zone->len > NBP_NVE_STR_SIZE) {
		errno = EINVAL;
		return (-1);
	}
	
#ifdef NBP_DEBUG
	{
		char xzone[35],xobject[35],xtype[35];
		strncpy(xzone,zone->str, zone->len);
		strncpy(xobject,object->str, object->len);
		strncpy(xtype,type->str, type->len);
		xobject[object->len] = '\0';
		xzone[zone->len] = '\0';
		xtype[type->len] = '\0';
		dPrintf(D_M_NBP_LOW, D_L_USR4,
			("nbp_validate: looking for %s:%s@%s\n",
			xobject, xtype, xzone));
	}
#endif /* NBP_DEBUG */
	/* Is this request for our zone ?? */
	nbp_req->nve.zone.len = zone->len;
	nbp_req->nve.zone_hash = 0;
	bcopy(zone->str,nbp_req->nve.zone.str, zone->len);
    if (checkLocal && !isZoneLocal(zone)) {
    	char str[35];
    	strncpy(str,zone->str,zone->len);
    	str[zone->len] = '\0';
    	dPrintf(D_M_NBP_LOW,D_L_WARNING,
         	("nbp_val_n_hash bad zone: %s\n", str));
     	errno = EINVAL;
     	return(-1);
     }
	if (Extended_net) {
		/* EtherTalk...
		 * The zone name in the request must match either "*" or
		 * our zone name (this_zone).  Zero length zone field is the
		 * same as "*".
		 */
		if (zone->len != 0) {
			if (!(zone->len == 1 && zone->str[0] == '*')) {
				nbp_req->nve.zone_hash = 
					nbp_strhash(& nbp_req->nve.zone);
		}
#ifdef COMMENTED_OUT 
				/* we allow multi-zone registration now, so
				   the actual zone name will be checked in
				   nbp_search_nve() */
				if (nbp_strcmp(zone, &this_zone, NULL) != 0) {
					errno = EINVAL;
					return (-1);
				}
#endif /* COMMENTED_OUT */
		}
	} else {
		/* LocalTalk... 
		 * "this_zone" variable will not have been initialized, since
		 * there's no interaction between NBP and ZIP for LocalTalk.
		 * We are assuming that the zone name field in a lookup 
		 * request packet will always contain a legal value.  
		 * One check that could be performed here (but is not) is : 
		 * make sure that if there's no router around on the
		 * net, the zone name in the lookup request must be "*".  If
		 * it's anything else, reject the packet.
		 */
	}

	nbp_req->nve.address = tuple->enu_addr;
	nbp_req->nve.object.len = object->len;
	nbp_req->nve.object_hash = 0;
	if (object->len == 1 && (object->str[0] == NBP_ORD_WILDCARD ||
		object->str[0] == NBP_SPL_WILDCARD)) {
		if (wild_ok)
			nbp_req->flags |= NBP_WILD_OBJECT;
		else {
			errno = EINVAL;
			return (-1);
		}
	} else{
		for (i = part_wild = 0; (unsigned) i<object->len; i++) {
			if (object->str[i] == NBP_SPL_WILDCARD)
				if (wild_ok)
					if (part_wild) {
						errno = EINVAL;
						return (-1);
					} else
						part_wild++;
				else {
					errno = EINVAL;
					return (-1);
				}
			nbp_req->nve.object.str[i] = object->str[i];
		}
		if (!part_wild)
			nbp_req->nve.object_hash = 
				nbp_strhash(&nbp_req->nve.object);
	}

	nbp_req->nve.type.len = type->len;
	nbp_req->nve.type_hash = 0;
	if (type->len == 1 && (type->str[0] == NBP_ORD_WILDCARD ||
		type->str[0] == NBP_SPL_WILDCARD)) {
		if (wild_ok)
			nbp_req->flags |= NBP_WILD_TYPE;
		else {
			errno = EINVAL;
			return (-1);
		}
	} else {
		for (i = part_wild = 0; (unsigned) i<type->len; i++) {
			if (type->str[i] == NBP_SPL_WILDCARD)
				if (wild_ok)
					if (part_wild) {
						errno = EINVAL;
						return (-1);
					} else
						part_wild++;
				else {
					errno = EINVAL;
					return (-1);
				}
			nbp_req->nve.type.str[i] = type->str[i];
		}
		if (!part_wild)
			nbp_req->nve.type_hash = 
				nbp_strhash(&nbp_req->nve.type);
	}
#ifdef NBP_DEBUG
	{
		char zone[35],object[35],type[35];
		strncpy(zone,nbp_req->nve.zone.str, nbp_req->nve.zone.len);
		strncpy(object,nbp_req->nve.object.str, nbp_req->nve.object.len);
		strncpy(type,nbp_req->nve.type.str, nbp_req->nve.type.len);
		object[nbp_req->nve.object.len] = '\0';
		zone[nbp_req->nve.zone.len] = '\0';
		type[nbp_req->nve.type.len] = '\0';
		dPrintf(D_M_NBP_LOW,D_L_USR4,
			("nbp_validate: after hash: %s:%s@%s\n",
			object, type, zone));
	}
#endif /* NBP_DEBUG */
	return(0);
}


/* Upshifts in place */
static	void	nbp_upshift (str, count)
register u_char	*str;
register int	count;
{
	register int	i, j;
	register u_char	ch;
	static	unsigned char	lower_case[] =
		{0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0xbe,
		 0xbf, 0xcf, 0x9b, 0x8b, 0x88, 0};
	static	unsigned char	upper_case[] = 
		{0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xae,
		 0xaf, 0xce, 0xcd, 0xcc, 0xcb, 0};

	for (j=0 ; j<count ; j++) {
		ch = str[j];
		if (ch >= 'a' && ch <= 'z')
			str[j] = ch + 'A' - 'a';
		else if (ch & 0x80)
			for (i=0; lower_case[i]; i++)
				if (ch == lower_case[i])
					str[j] = upper_case[i];
	}
}


static	u_int	nbp_strhash (nvestr)
register at_nvestr_t	*nvestr;
{
	/* upshift while hashing */
	register u_int	hash = 0;
	register int	i, len;
	union {
		u_char	h_4char[4];
		int	h_int;
	} un;

	for (i=0; (unsigned) i < nvestr->len; i+=sizeof(int)) {
		len = MIN((nvestr->len-i), sizeof(int));
		if (len == sizeof(int))
			bcopy(&(nvestr->str[i]), &un, sizeof(un));
		else {
			un.h_int = -1;
			for ( ; (unsigned) i<nvestr->len; i++)
				un.h_4char[i % sizeof(int)] = nvestr->str[i];
		}
		nbp_upshift (un.h_4char, len);
		hash ^= un.h_int;
	}
	
	return (hash);
}


static	nve_entry_t	*nbp_search_nve (nbp_req, ifID)
register nbp_req_t	*nbp_req;
register at_if_t 	*ifID;		/* NULL ok */
{
	register nve_entry_t	*nve_entry;

#ifdef NBP_DEBUG
	{
		char zone[35],object[35],type[35];
		strncpy(zone,nbp_req->nve.zone.str, nbp_req->nve.zone.len);
		strncpy(object,nbp_req->nve.object.str, nbp_req->nve.object.len);
		strncpy(type,nbp_req->nve.type.str, nbp_req->nve.type.len);
		object[nbp_req->nve.object.len] = '\0';
		zone[nbp_req->nve.zone.len] = '\0';
		type[nbp_req->nve.type.len] = '\0';
		dPrintf(D_M_NBP_LOW, D_L_USR4,
				("nbp_search: looking for %s:%s@%s resp:0x%x\n",object,type,zone,
				(u_int) nbp_req->response));
	}
#endif /* NBP_DEBUG */
	ATDISABLE(nve_lock_pri,NVE_LOCK);
	for (nve_entry = name_registry.fwd; nve_entry != &name_registry ; 
		nve_entry = nve_entry->fwd) {

		if ((nbp_req->nve.zone_hash) && 
			((nbp_req->nve.zone_hash != 
			  nve_entry->zone_hash) &&
			 (nbp_req->nve.zone_hash != hzonehash)
		    )
		   ) {
			dPrintf(D_M_NBP_LOW,D_L_USR4,
				("nbp_search: no match for zone, req hash:%x\n",
			nbp_req->nve.zone_hash));
			continue;
		}
		else { 	/* for this entry's zone OR no zone in request or entry */
				/* only in singleport mode (!MULTIPORT_MODE) with empty PRAM
                   can an entry have '*' for it's zone
                 */
			at_nvestr_t *ezone=&nve_entry->zone;
			at_nvestr_t *rzone=&nbp_req->nve.zone;
/*			if (!(rzone->len == 1 && rzone->str[0] == '*'))  { */
			if (!DEFAULT_ZONE(rzone) && !DEFAULT_ZONE(ezone))  {
				if (nbp_strcmp (rzone, ezone, 0) != 0)
					continue;
			}
			else {
				if (MULTIHOME_MODE && ifID && 
					NET_NOTEQ(nve_entry->address.net, ifID->ifThisNode.atalk_net)) {
				dPrintf(D_M_NBP, D_L_USR4, 
					("nbp search ifID (%d) & req net (%d) not eq\n",
					NET_VALUE(nve_entry->address.net),
					NET_VALUE(ifID->ifThisNode.atalk_net)));
					continue;
				}
				dPrintf(D_M_NBP, D_L_USR4, 
					("nbp search ifID (%d) & req net (%d)  equal\n",
					NET_VALUE(nve_entry->address.net),
					NET_VALUE(ifID->ifThisNode.atalk_net)));
			}
		
		}
		if (!(nbp_req->flags & NBP_WILD_OBJECT)) {
			if ((nbp_req->nve.object_hash) && 
				(nbp_req->nve.object_hash != 
				nve_entry->object_hash))
				continue;
			else {
				if (nbp_strcmp (&nbp_req->nve.object, 
					&nve_entry->object, 
					NBP_SPL_WILDCARD) != 0)
					continue;
			}
		}


		if (!(nbp_req->flags & NBP_WILD_TYPE)) {
			if ((nbp_req->nve.type_hash) && 
				(nbp_req->nve.type_hash !=nve_entry->type_hash))
				continue;
			else {
				if (nbp_strcmp (&nbp_req->nve.type, 
					&nve_entry->type, 
					NBP_SPL_WILDCARD) != 0)
					continue;
			}
		}

		/* Found a match! */
#ifdef NBP_DEBUG
	{
		char zone[35],object[35],type[35];
		int net;
		net = NET_VALUE(nve_entry->address.net);
		strncpy(zone,nbp_req->nve.zone.str, nbp_req->nve.zone.len);
		strncpy(object,nbp_req->nve.object.str, nbp_req->nve.object.len);
		strncpy(type,nbp_req->nve.type.str, nbp_req->nve.type.len);
		object[nbp_req->nve.object.len] = '\0';
		zone[nbp_req->nve.zone.len] = '\0';
		type[nbp_req->nve.type.len] = '\0';
		dPrintf(D_M_NBP_LOW, D_L_USR2,
			("nbp_search: found  %s:%s@%s  net:%d\n",
			object, type, zone, net));
	}
#endif /* NBP_DEBUG */
		if (nbp_req->func != NULL) {
			if ((*(nbp_req->func))(nbp_req, nve_entry) != 0) {
				/* errno expected to be set by func */
				ATENABLE(nve_lock_pri,NVE_LOCK);
				return (NULL);
			}
		} else {
			ATENABLE(nve_lock_pri,NVE_LOCK);
			return (nve_entry);
		}
	}
	ATENABLE(nve_lock_pri,NVE_LOCK);

	errno = 0;
	return (NULL);
}


static	int	nbp_lkup_reply (nbp_req, nve_entry)
register nbp_req_t	*nbp_req;
register nve_entry_t	*nve_entry;
{
	register at_nbptuple_t	*tuple;
	register int	tuple_size, buf_len;
	register int	obj_len, type_len;

	/* size of the current tuple we want to write... */
	tuple_size = nve_entry->object.len + 1 + 	/* object */
			nve_entry->type.len + 1 + 	/* type */
			2 + 				/* zone */
			sizeof (at_inet_t) + 1;		/* addr + enum */

	buf_len = ((nbp_req->flags & NBP_WILD_MASK) ? DDP_DATA_SIZE:tuple_size);
	if (nbp_req->response == NULL) {
		if (nbp_setup_resp (nbp_req, buf_len) != 0)
			/* errno expected to be set by nbp_setup_resp() */
			return (-1);
	}

	if ((nbp_req->space_unused < tuple_size) || 
		(DATA_NBP(nbp_req->response)->tuple_count == NBP_TUPLE_MAX)) {
		if (nbp_send_resp (nbp_req) != 0)
			return (-1);
		if (nbp_setup_resp (nbp_req, buf_len) != 0)
			return (-1);
	}

	/* At this point, we have a response buffer that can accommodate the
	 * tuple we want to write. Write it!
	 */
	tuple = (at_nbptuple_t *)gbuf_wptr(nbp_req->response);
	tuple->enu_addr = nve_entry->address;
	tuple->enu_enum = nve_entry->enumerator;
	obj_len = nve_entry->object.len + 1;
	bcopy(&nve_entry->object, &tuple->enu_name[0], obj_len);
	type_len = nve_entry->type.len + 1;
	bcopy(&nve_entry->type, &tuple->enu_name[obj_len], type_len);
	tuple->enu_name[obj_len + type_len] = 1;
	tuple->enu_name[obj_len + type_len + 1] = '*';

	nbp_req->space_unused -= tuple_size;
	gbuf_winc(nbp_req->response, tuple_size);

	/* increment the tuple count in header by 1 */
	DATA_NBP(nbp_req->response)->tuple_count++;

	return (0);
}


static	int	nbp_strcmp (str1, str2, embedded_wildcard)
register at_nvestr_t	*str1, *str2;
register u_char	embedded_wildcard;	/* If str1 may contain a character
					 * that's to be treated as an
					 * embedded wildcard, this character
					 * is it.  Making this special case
					 * since for zone names, squiggly
					 * equal is not to be treated as a 
					 * wildcard.
					 */
{
	u_char	        ch1,ch2;
	register int	i1, i2;
	register int	reverse = 0;
	register int	left_index;

	/* Embedded wildcard, if any, could only be in the first string (str1).
	 * returns 0 if two strings are equal (modulo case), -1 otherwise 
	 */
	
	if (str1->len == 0 || str2->len == 0) {
		return (-1);
	}	
	
	/* Wildcards are not allowed in str2.
	 *
	 * If str1 could potentially contain an embedded wildcard, since the
	 * embedded wildcard matches ZERO OR MORE characters, str1 can not be
	 * more than 1 character longer than str2.
	 *
	 * If str1 is not supposed to have embedded wildcards, the two strs 
	 * must be of equal length.
	 */
	if ((embedded_wildcard && (str2->len < (unsigned) (str1->len-1))) ||
		(!embedded_wildcard && (str2->len !=  str1->len))) {
		return (-1);
	}

	for (i1 = i2 = left_index = 0; (unsigned) i1 < str1->len ;) {
		ch1 = str1->str[i1];
		ch2 = str2->str[i2];

		if (embedded_wildcard && (ch1==embedded_wildcard)) {
			/* hit the embedded wild card... start comparing from 
			 * the other end of the string.
			 */
			reverse++;
			/* But, if embedded wildcard was the last character of 
			 * the string, the two strings match, so return okay.
			 */
			if (i1 == str1->len-1) {
				return (0);
			}
			
			i1 = str1->len - 1;
			i2 = str2->len - 1;
			
			continue;
		}
		
		nbp_upshift(&ch1, 1);
		nbp_upshift(&ch2, 1);

		if (ch1 != ch2) {
			return (-1);
		}
		
		if (reverse) {
			i1--; i2--;
			if (i1 == left_index) {
				return (0);
			}
		} else {
			i1++; i2++; left_index++;
		}
	}
	return (0);
}


static	void	nbp_setup_hdr (nbp_req)
register nbp_req_t	*nbp_req;
{
	register at_ddp_t	*ddp;
	register at_nbp_t	*nbp;

	ddp = DATA_DDP(nbp_req->response);
	nbp = DATA_NBP(nbp_req->response);
	
	ddp->type = NBP_DDP_TYPE;
	UAS_ASSIGN(ddp->checksum, 0);
	ddp->unused = ddp->hopcount = 0;

	switch(DATA_NBP(nbp_req->request)->control) {
	case NBP_LKUP :
		ddp->dst_socket = nbp_req->nve.address.socket;
		ddp->dst_node = nbp_req->nve.address.node;
		NET_NET(ddp->dst_net, nbp_req->nve.address.net);
		nbp->control = NBP_LKUP_REPLY;
		break;
	case NBP_REGISTER :
	case NBP_DELETE :
		ddp->dst_socket = DATA_DDP(nbp_req->request)->src_socket;
		ddp->dst_node = DATA_DDP(nbp_req->request)->src_node;
		NET_NET(ddp->dst_net, DATA_DDP(nbp_req->request)->src_net);
		nbp->control = NBP_STATUS_REPLY;
		break;
	}
	nbp->at_nbp_id = DATA_NBP(nbp_req->request)->at_nbp_id;
	return;
}


static	int	nbp_setup_resp (nbp_req, tuples_size)
register nbp_req_t	*nbp_req;
register int		tuples_size;
{
	int	buf_size = tuples_size + DDP_X_HDR_SIZE + NBP_HDR_SIZE;
	nbp_req->response = gbuf_alloc(AT_WR_OFFSET+buf_size, PRI_MED);
	if (nbp_req->response == NULL) {
		errno = ENOBUFS;
		return(-1);
	}
	gbuf_rinc(nbp_req->response, AT_WR_OFFSET);
	gbuf_wset(nbp_req->response, DDP_X_HDR_SIZE + NBP_HDR_SIZE);
	nbp_setup_hdr(nbp_req);

	DATA_NBP(nbp_req->response)->tuple_count = 0;
	nbp_req->space_unused = tuples_size;

	return (0);
} /* nbp_setup_resp */


static	int	nbp_send_resp (nbp_req)
register nbp_req_t	*nbp_req;
{
	int		status;

	status = ddp_output(&nbp_req->response, (at_socket)NBP_SOCKET, NULL);

	if (status)
		gbuf_freem(nbp_req->response);
	nbp_req->response = NULL;
	errno = status;
	return(errno?-1:0);
}

static	int	nbp_insert_entry (nve_entry)
nve_entry_t	*nve_entry;
{
	gbuf_t		*tag;
	nve_entry_t	*new_entry;
	int i;
	at_if_t	*ifID;
	at_nvestr_t *zone;
	int		multiZone = 0;		/* true if we should do multiple registrations */
	int		defaultZone = 0;	/* true if zone is '*' */

	/* Got an nve entry on hand.... allocate a buffer, copy the entry
	 * on to it and stick it in the registry.
	 */
	if (nve_entry->zone.str[0] == '*')
		defaultZone=TRUE;
	if (defaultZone  && MULTIHOME_MODE) {
			multiZone = TRUE;
	}
	for (ifID = ifID_table[0],i=0; ifID; ifID=ifID_table[++i]) {
		if (!defaultZone && MULTIPORT_MODE ) {
				/* if registering for a specific zone and we have multiple
				   I/F's running, find the first I/F containing the 
				   requested zone 
				 */
			int zno;
			char ifs_in_zone[IF_TOTAL_MAX];
			if (!(zno = zt_find_zname(&nve_entry->zone))) {
				errno = ENOENT;
				return(-1);
			}
			getIfUsage(zno-1, ifs_in_zone);
			for(; ifID; ifID=ifID_table[++i])
				if (ifs_in_zone[i])
					break;
			if (!ifID) {
				errno = ENOENT;
				return(-1);
			}
		}

		if ((tag = gbuf_alloc(sizeof(nve_entry_t), PRI_HI)) == NULL){
			errno = ENOBUFS;
			return (-1);
		}
		gbuf_wset(tag, sizeof(nve_entry_t));
		new_entry = (nve_entry_t *)gbuf_rptr(tag);
		bcopy(nve_entry, new_entry, sizeof(nve_entry_t));
		NET_NET(new_entry->address.net, ifID->ifThisNode.atalk_net);
		new_entry->address.node = ifID->ifThisNode.atalk_node;
		if (multiZone ){
			new_entry->zone = ZT_table[ifID->ifDefZone-1].Zone;
			new_entry->zone_hash = nbp_strhash(&new_entry->zone);
		}
		
		if (defaultZone && !multiZone ) {
				/* put actual zone name in entry instead of "*" */
				/* if single port mode and no zone name, then a router
                   is down, so use pram zone name hint from elap cfg */
			if (!MULTIPORT_MODE && ifID->ifZoneName.str[0] == '*') {
				elap_specifics_t *elapp;
				elapp = (elap_specifics_t*)(ifID_table[IFID_HOME]->ifLapp);
				zone = &elapp->cfg.zonename;
			}
			else {
				zone = &ifID_table[IFID_HOME]->ifZoneName; 
			}
			new_entry->zone = *zone;
			if ( new_entry->zone.len == 0 ) {
				new_entry->zone.str[0] = '*';
				new_entry->zone.len = 1;
			}
			new_entry->zone_hash = nbp_strhash(&new_entry->zone);
		}
		new_entry->tag = tag;
#ifdef NEXT
/* Fixes PR 2243737: didn't deregister nbp entity properly */
		new_entry->pid =  current_proc()->p_pid;
#else
		new_entry->pid = getpid();
#endif

		ATDISABLE(nve_lock_pri,NVE_LOCK);
		ddp_insque(new_entry, name_registry.fwd);
		ATENABLE(nve_lock_pri,NVE_LOCK);
		at_statep->flags |= AT_ST_NBP_CHANGED;
#ifdef NBP_DEBUG
	{
		char zone[35],object[35],type[35];
		strncpy(zone,new_entry->zone.str, new_entry->zone.len);
		strncpy(object,new_entry->object.str, new_entry->object.len);
		strncpy(type,new_entry->type.str, new_entry->type.len);
		object[new_entry->object.len] = '\0';
		zone[new_entry->zone.len] = '\0';
		type[new_entry->type.len] = '\0';
		dPrintf(D_M_NBP_LOW, D_L_USR4,
			("nbp_insert:adding %s:%s@%s i/f:%s ",
			object, type, zone, ifID->ifName));
		dPrintf(D_M_NBP_LOW, D_L_USR4,
			("addr:%d.%d\n",
			NET_VALUE(new_entry->address.net),
			new_entry->address.node));
	}
#endif /* NBP_DEBUG */
		nbp_add_multicast(&new_entry->zone, ifID);
		if (!multiZone)
			break;
	}
	return (0);
}

nbp_add_multicast(zone, ifID)
at_nvestr_t *zone;
at_if_t *ifID;
{
	char data[ETHERNET_ADDR_LEN];
	int i;

	if (zone->str[0] == '*')
		return;
		{
			char str[35];
			strncpy(str,zone->str,zone->len);
			str[zone->len] = '\0';
			dPrintf(D_M_NBP_LOW, D_L_USR3,
				("nbp_add_multi getting mc for %s\n", str));
		}
  		zt_get_zmcast(ifID, zone, data); 
		if ((ifID->ifType == IFTYPE_FDDITALK)
			|| (ifID->ifType == IFTYPE_TOKENTALK))
			ddp_bit_reverse(data);
		dPrintf(D_M_NBP_LOW,D_L_USR3,
			("nbp_add_multi adding  0x%x%x port:%d ifID:0x%x if:%s\n",
			*(unsigned*)data, (*(unsigned *)(data+2))&0x0000ffff,
			i, (u_int) ifID, ifID->ifName));

		elap_control(ifID, ELAP_REG_ZONE_MCAST, data);
}


static	void	nbp_delete_entry (nve_entry)
nve_entry_t	*nve_entry;
{
	ddp_remque(nve_entry);
	gbuf_freem(nve_entry->tag);
	at_statep->flags |= AT_ST_NBP_CHANGED;
}


static	int	nbp_enum_gen (nve_entry)
register nve_entry_t	*nve_entry;
{
	register int		new_enum = 0;
	register nve_entry_t	*ne;

	ATDISABLE(nve_lock_pri,NVE_LOCK);
re_do:
	for (ne = name_registry.fwd; ne != &name_registry; ne = ne->fwd) {
		if ((*(int *)&ne->address == *(int *)&nve_entry->address) &&
			(ne->enumerator == new_enum)) {
			if (new_enum == 255) {
				errno = EADDRNOTAVAIL;
				ATENABLE(nve_lock_pri,NVE_LOCK);
				return (-1);
			} else {
				new_enum++;
				goto re_do;
			}
		}
	}

	ATENABLE(nve_lock_pri,NVE_LOCK);
	nve_entry->enumerator = new_enum;
	return (0);
}


getNbpTableSize()

/* for SNMP, returns size in # of entries */
{
	register nve_entry_t *nve;
	register int i=0;

	ATDISABLE(nve_lock_pri,NVE_LOCK);
	for (nve = name_registry.fwd; nve != &name_registry; nve = nve->fwd, i++) 
			;
	ATENABLE(nve_lock_pri,NVE_LOCK);
	return(i);
}

getNbpTable(p,s,c)
snmpNbpEntry_t	*p;
int 		s;		/* starting entry */
int			c;		/* # entries to copy */

/* for SNMP, returns section of nbp table */

{
	register nve_entry_t *nve;
	register int i=0;
	static   int nextNo=0;		/* entry that *next points to */
	static	 nve_entry_t  *next = (nve_entry_t*)NULL;
	
	if (s && next && nextNo == s) {
		nve = next;
		i = nextNo;
	}
	else
		nve = name_registry.fwd;

	ATDISABLE(nve_lock_pri,NVE_LOCK);
	for ( ; nve != &name_registry && c ; nve = nve->fwd, p++,i++) {
		if (i>= s) {
			p->nbpe_object = nve->object;
			p->nbpe_type   = nve->type;
			c--;
		}
	}
	ATENABLE(nve_lock_pri,NVE_LOCK);
	if (nve != &name_registry) {
		next = nve;
		nextNo = i;
	}else {
		next = (nve_entry_t*)NULL;
		nextNo = 0;
	}
}


#define ZONES_PER_BLK		31	/* 31 fits within a 1k blk) */
#define ZONE_BLK_SIZE		ZONES_PER_BLK * sizeof(at_nvestr_t)

setLocalZones(newzones, size)
at_nvestr_t *newzones;
int size;
/* updates list of zones which are local to all active ports
   missing zones are not deleted, only missing zones are added.
*/
{
	int	bytesread=0;		/* #bytes read from tuple */
	int i=0, dupe;
	gbuf_t	*m;
	at_nvestr_t		*pnve, *pnew = newzones;

	if (!lzones) {
		if(!(lzones = gbuf_alloc(ZONE_BLK_SIZE, PRI_MED)))
			return(ENOBUFS);
		gbuf_wset(lzones,0);
	}
	while (bytesread < size) {		/* for each new zone */
		{
			char str[35];
			strncpy(str,pnew->str,pnew->len);
			str[pnew->len] = '\0';
		}
		m = lzones;				
		pnve = (at_nvestr_t*)gbuf_rptr(m);
		dupe = 0;
		for (i=0; i<lzonecnt && !dupe; i++,pnve++)  {
			if (i && !(i%ZONES_PER_BLK))
				if (gbuf_cont(m)) {
					m = gbuf_cont(m);
					pnve = (at_nvestr_t*)gbuf_rptr(m);
				}
				else
					break;
			if (pnew->len != pnve->len)
				continue;
			if ((pnew->len > 33) || (pnew->len <0) ) {
				return(0);
			}
			if (!strncmp(pnew->str, pnve->str, pnew->len)) {
				dupe=1;
				continue;
			}
		}
		if (!dupe) {
			/* add new zone */
			if (lzonecnt && !(lzonecnt%ZONES_PER_BLK)) {
				if(!(gbuf_cont(m) = gbuf_alloc(ZONE_BLK_SIZE, PRI_MED)))
					return(ENOBUFS);
				gbuf_wset(gbuf_cont(m),0);
				pnve = (at_nvestr_t*)gbuf_rptr(gbuf_cont(m));
			}
			strncpy(pnve->str,pnew->str,pnew->len);
			pnve->len = pnew->len;
			lzonecnt++;
		}
		bytesread += (pnew->len+1);
		pnew = (at_nvestr_t*) (((char *)pnew) + pnew->len + 1);
	}
	/* showLocalZones1(); */
	return(0);
}

/**********
showLocalZones1()
{
	int i;
	at_nvestr_t *pnve;
	gbuf_t	*m;
	char str[35];
	
	for (i=0;  ; i++) {
		if (!(pnve = getLocalZone(i))) {
			break;
		}
		strncpy(str,pnve->str,pnve->len);
		str[pnve->len] = '\0';
	}
}

*********/

isZoneLocal(zone)
at_nvestr_t *zone;
{
	at_nvestr_t *pnve;
	int i;
	if ((zone->len == 1 && zone->str[0] == '*') || zone->len == 0)
		return(1);
	for (i=0;  ; i++) {
		if (!(pnve = getLocalZone(i))) 
			break;
		if (!nbp_strcmp(pnve,zone,0))
			return(1);
	}
	return(0);
}
	

#define NULL_PNVESTR (at_nvestr_t *) 0

at_nvestr_t *getLocalZone(zno)
	int zno;			/* zone number in virtual list to
						   return, 0 for first zone */
/* returns pointer to a new local zone number zno,
   returns null when no zones left.
*/
{
	if_zone_t ifz;
	ifz.ifzn.zone = zno;
	if (ROUTING_MODE || MULTIHOME_MODE) 
		return(getRTRLocalZone(&ifz));
	else
		return(getSPLocalZone(zno));
}


at_nvestr_t *getSPLocalZone(zno)
	int zno;			/* zone number in virtual list to
						   return, 0 for first zone */
/* single port mode version */
{
	int curz=0;		/* current zone */
	gbuf_t *m;
	at_nvestr_t *pnve;

	if (lzones) {
		m = lzones;
		pnve = (at_nvestr_t*)gbuf_rptr(m);
	}
	else
		return(NULL_PNVESTR);
	if ( zno>=lzonecnt )
		return(NULL_PNVESTR);
	for (curz=0; curz<zno; curz++,pnve++ ) {
		if ( curz<lzonecnt ) {
			if (curz && !(curz%ZONES_PER_BLK) ) {
				if (gbuf_cont(m)) {
					m = gbuf_cont(m);
					pnve = (at_nvestr_t*)gbuf_rptr(m);
				}
				else {
					return(NULL_PNVESTR);
				}
			}
			if ((pnve->len > NBP_NVE_STR_SIZE) || (pnve->len <0) ) {
				return(NULL_PNVESTR);
			}
		}
		else
			return(NULL_PNVESTR);
	}
	return(pnve);
}




