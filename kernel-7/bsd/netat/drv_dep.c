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
 * Copyright 1994 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
 * The copyright notice above does not evidence any actual or
 * intended publication of such source code.
 *
 */

#include <localglue.h>
#include <sysglue.h>
#include <at/appletalk.h>
#include <at/elap.h>
#include <at/at_lap.h>
#include <atlog.h>
#include <lap.h>
#include <at_elap.h>
#include <at_aarp.h>
#include <at_pat.h>
#include <routing_tables.h>

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/etherdefs.h>
#include <net/tokendefs.h>

#define DSAP_SNAP 0xaa

extern at_state_t *at_statep;
extern elap_specifics_t elap_specifics[]; 

static llc_header_t	snap_hdr_at = SNAP_HDR_AT;
static llc_header_t	snap_hdr_aarp = SNAP_HDR_AARP;
static unsigned char snap_proto_ddp[5] = SNAP_PROTO_AT;
static unsigned char snap_proto_aarp[5] = SNAP_PROTO_AARP;
/* static struct etalk_addr etalk_mcast_base = 
   		{0x09,0x00,0x07,0x00,0x00,0x00}; *** not used *** 
*/
int pktsIn, pktsOut;
int xpatcnt = 0;

extern pat_unit_t pat_units[];

#ifndef _AIX

/* struct ifnet atalkif[1]; *** not used *** */
struct ifqueue atalkintrq; 	/* appletalk and aarp packet input queue */

/* xxdevcnt, xxtype, xxnddp tables are no longer needed since the ifp's 
   are being initialized as a result of actions taken by the drivers.
   There was a bug in which "xxnddp" was indexed by count in one place,
   and unit in another place.  However upon closer examination we 
   determined that "xxnddp" is no longer needed.
   Annette DeSchon (deschon@apple.com) 11/20/97
*/

short appletalk_inited = 0;

#define FETCH_AND_ADD(x,y) (*x += y)


extern int (*sys_ATsocket )(), (*sys_ATgetmsg)(), (*sys_ATputmsg)();
extern int (*sys_ATPsndreq)(), (*sys_ATPsndrsp)();
extern int (*sys_ATPgetreq)(), (*sys_ATPgetrsp)();

void atalk_load()
{
	extern int _ATsocket(), _ATgetmsg(), _ATputmsg();
	extern int _ATPsndreq(), _ATPsndrsp(), _ATPgetreq(), _ATPgetrsp();

	extern atlock_t ddpall_lock;
	extern atlock_t ddpinp_lock;
	extern atlock_t arpinp_lock;

	sys_ATsocket  = _ATsocket;
	sys_ATgetmsg  = _ATgetmsg;
	sys_ATputmsg  = _ATputmsg;
	sys_ATPsndreq = _ATPsndreq;
	sys_ATPsndrsp = _ATPsndrsp;
	sys_ATPgetreq = _ATPgetreq;
	sys_ATPgetrsp = _ATPgetrsp;

	xpatcnt = 0;
	ATLOCKINIT(ddpall_lock);
	ATLOCKINIT(ddpinp_lock);
	ATLOCKINIT(arpinp_lock);
	gref_init();
	atp_init();
/*	adsp_init(); 
		for 2225395
		this happens in adsp_open and is undone on ADSP_UNLINK 
*/
} /* atalk_load */

void atalk_unload()  /* not currently used */
{
	extern gbuf_t *scb_resource_m;
	extern gbuf_t *atp_resource_m;

	sys_ATsocket  = 0;
	sys_ATgetmsg  = 0;
	sys_ATputmsg  = 0;
	sys_ATPsndreq = 0;
	sys_ATPsndrsp = 0;
	sys_ATPgetreq = 0;
	sys_ATPgetrsp = 0;

	if (scb_resource_m) {
		gbuf_freem(scb_resource_m);
		scb_resource_m = 0;
	}
	if (atp_resource_m) {
		gbuf_freem(atp_resource_m);
		atp_resource_m = 0;
	}
/* 	CleanupGlobals()
	this happens on ADSP_UNLINK for 2225395 */
	appletalk_inited = 0;
} /* atalk_unload */

void appletalk_hack_start(nddp)
	struct ifnet *nddp;
{
	if (!appletalk_inited) {
		appletalk_inited =1 ;
		atalk_load();
		atalkintrq.ifq_maxlen = IFQ_MAXLEN; 
	}

	/* Call to pat_attach(nddp) and the "xx" tables are no longer
	   needed since the ifp's are being initialized as a result
	   of actions taken by the drivers.  AD 11/21/97 */
} /* appletalk_hack_start */

/* return the unit number of this interface
   return -1 if the interface is not found
*/
/* int pat_ifpresent(name)	*** not used ***
	char *name;
{
	struct ifnet *ifp;

	if (ifp = (ifunit(name)))
	  return(ifp->if_unit);
	else
	  return(-1);
} 
*/

int 
pat_output(pat_id, mlist, dst_addr, type)
	int	pat_id;
	gbuf_t *mlist;
	unsigned char *dst_addr;
	int 	type;
{
	pat_unit_t *patp;
	gbuf_t *m, *m_prev, *new_mlist;
	llc_header_t *llc_header;
	
	struct sockaddr dst;

	/* this is for ether_output */

	dst.sa_family = AF_APPLETALK;
	dst.sa_len = sizeof(struct etalk_addr);
	bcopy (dst_addr, &dst.sa_data[0], dst.sa_len); 
	patp = (pat_unit_t *)&pat_units[pat_id];
	if (patp->state != PAT_ONLINE) {
		gbuf_freel(mlist);
		return ENOTREADY;
	}

	if (patp->xtype == IFTYPE_NULLTALK) {
		gbuf_freel(mlist);
		return 0;
	}
	new_mlist = 0;

  for (m = mlist; m; m = mlist) {
	mlist = gbuf_next(m);
	gbuf_next(m) = 0;

	gbuf_prepend(m,sizeof(llc_header_t));
	if (m == 0) {
		if (mlist)
			gbuf_freel(mlist);
		if (new_mlist)
			gbuf_freel(new_mlist);
		return 0;
	}

	llc_header = (llc_header_t *)gbuf_rptr(m);
	*llc_header = (type == AARP_AT_TYPE) ? snap_hdr_aarp : snap_hdr_at;

	m->m_pkthdr.len = gbuf_msgsize(m);
	m->m_pkthdr.rcvif = 0;

	if (new_mlist)
		gbuf_next(m_prev) = m;
	else
		new_mlist = m;
	m_prev = m;
	pktsOut++;
  }

	while ((m = new_mlist) != 0) {
		new_mlist = gbuf_next(m);
		gbuf_next(m) = 0;
		((struct ifnet *)patp->nddp)->if_output((void *)patp->nddp, m, &dst, NULL);
	}

	return 0;
} /* pat_output */

int
pat_online (ifName, ifType)
	char	*ifName;
	char	*ifType;
{
	/*###int pat_input();*/
	int pat_id;
	pat_unit_t *patp;
	register struct ifaddr *ifa;
	register struct sockaddr_dl *sdl;
	struct ifnet *nddp;

	if ((pat_id = pat_ID(ifName)) == -1) 
		return (-1);
	patp = &pat_units[pat_id];

	if (patp->xtype == IFTYPE_ETHERTALK) {
		if ((nddp = ifunit(ifName)) == NULL)
		   return -1;
	} else if (patp->xtype == IFTYPE_NULLTALK) {
		patp->xaddrlen = 6;
		bzero(patp->xaddr, patp->xaddrlen);
		if (ifType)
			*ifType = patp->xtype;
		patp->nddp = (void *)0;
		patp->state  = PAT_ONLINE;
		at_statep->flags |= AT_ST_IF_CHANGED;
		return (pat_id);
	} else
		return -1;

/*###********* No need to declare ourself as an if for now
	if_attach(NULL, pat_input, 0, 0, 0,
		if_name((void *)nddp), if_unit((void *)nddp), "AppleTalk",
			622, IFF_BROADCAST, NETIFCLASS_VIRTUAL, 0);
*/

/*	if_control((void *)nddp, IFCONTROL_GETADDR, patp->xaddr); */

	for (ifa = nddp->if_addrlist; ifa; ifa = ifa->ifa_next)
                if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) &&
                    (sdl->sdl_family == AF_LINK)) {
			bcopy(LLADDR(sdl), patp->xaddr, 6);
#ifdef APPLETALK_DEBUG
			kprintf("pat_online: our local enet address is=%s\n", 
				ether_sprintf(patp->xaddr));
#endif
			break;
		}

	patp->xaddrlen = 6;

	if (ifType)
		*ifType = patp->xtype;

	patp->nddp = (void *)nddp;
	patp->state  = PAT_ONLINE;
	at_statep->flags |= AT_ST_IF_CHANGED;

	return (pat_id);
} /* pat_online */

void
pat_offline(pat_id)
	int pat_id;
{
	pat_unit_t *patp = &pat_units[pat_id];
	
	if (patp->state == PAT_ONLINE) {
/*#####
	  if (patp->xtype != IFTYPE_NULLTALK) {

		if_detach((void *)patp->nddp);
	  }
*/
		at_statep->flags |= AT_ST_IF_CHANGED;
		bzero(patp, sizeof(pat_unit_t));
	}
} /* pat_offline */

int
pat_mcast(pat_id, control, data)
	int pat_id;
	int control;
	unsigned char *data;
{
	struct ifnet *nddp;
	struct ifreq request;

	/* this is for ether_output */

	request.ifr_addr.sa_family = AF_UNSPEC;
	request.ifr_addr.sa_len = 6; /* warning ### */
	bcopy (data, &request.ifr_addr.sa_data[0], 6);

	nddp = (struct ifnet *)pat_units[pat_id].nddp;

	/*### LD Direct access to the ifp->if_ioctl function */

	if (nddp == 0) {
#ifdef APPLETALK_DEBUG
		kprintf("pat_mcast: BAD ndpp\n");
#endif
		return(-1);
	}	
#ifdef APPLETALK_DEBUG
	else kprintf("pat_mcast: register multicast for ifname=%s\n", nddp->if_name);
#endif
	return (*nddp->if_ioctl)(nddp, (control == PAT_REG_MCAST) ?
		SIOCADDMULTI : SIOCDELMULTI, &request);
} /* pat_mcast */

void
atalkintr()
{
	struct mbuf *m;
	struct ifnet *ifp;
	int s, ret;

next:
	s = splimp();
	IF_DEQUEUE(&atalkintrq, m);
	splx(s);	

	if (m == 0) 
		return;	

        if ((m->m_flags & M_PKTHDR) == 0) {
#ifdef APPLETALK_DEBUG
                kprintf("atalkintr: no HDR on packet received");
#endif
		return;
	}

        ifp = m->m_pkthdr.rcvif;

	/* do stuff */
/*
       if (m->m_len < ENET_LLC_SIZE &&
           (m = m_pullup(m, ENET_LLC_SIZE)) == 0) {
		kprintf("atalkintr: packet too small\n");
           goto next;
       }
*/

	ret = pat_input(NULL, ifp, m);
/*
	if (ret)
		kprintf("atalkintr: pat_input ret=%d\n", ret);
*/
	goto next;
} /* atalkintr */


int
pat_input(xddp, nddp, m)
	void *xddp;
	void *nddp;
	gbuf_t *m;
{
#ifdef CHECK_DDPR_FLAG
	extern int ddprunning_flag;
#endif
	llc_header_t *llc_header;
	int pat_id;
	pat_unit_t *patp;
	char src[6];
	enet_header_t *enet_header = (enet_header_t *)gbuf_rptr(m);
		
	for (pat_id=0, patp = &pat_units[pat_id];
			pat_id < xpatcnt; pat_id++, patp++) {
		if ((patp->state == PAT_ONLINE) && (patp->nddp == nddp)) 
			break;
	}
	/* if we didn't find a matching interface */
	if (pat_id == xpatcnt) {
		gbuf_freem(m);
#ifdef CHECK_DDPR_FLAG
		FETCH_AND_ADD((atomic_p)&ddprunning_flag, -1);
#endif
		return EAFNOSUPPORT;
	}

	/* Ignore multicast packets from local station */
  	if (patp->xtype == IFTYPE_ETHERTALK) {
		bcopy((char *)enet_header->src, src, sizeof(src));
/*
		kprintf("pat_input: Packet received src addr=%s len=%x\n", 
		        ether_sprintf(enet_header->src), enet_header->len);
*/
/* In order to receive packets from the Blue Box, we cannot reject packets
   whose source address matches our local address.  
		if ((enet_header->dst[0] & 1) && 
				(bcmp(src, patp->xaddr, sizeof(src)) == 0)) {
#ifdef APPLETALK_DEBUG
			kprintf("pat_input: Packet rejected: think it's a local mcast\n");
#endif
			gbuf_freem(m);
#ifdef CHECK_DDPR_FLAG
		      FETCH_AND_ADD((atomic_p)&ddprunning_flag, -1);
#endif
			return EAFNOSUPPORT;
		}
*/

		llc_header = (llc_header_t *)(enet_header+1);

		gbuf_rinc(m,(ENET_LLC_SIZE));
#ifdef CHECK_DDPR_FLAG
		FETCH_AND_ADD((atomic_p)&ddprunning_flag, 1);
#endif
		pktsIn++;
/*
		kprintf("pat_input: enet_header: dest=%s LLC_PROTO= %02x%02x\n",
			ether_sprintf(enet_header->dst),
			llc_header->protocol[3],
			llc_header->protocol[4]);
*/

		if (LLC_PROTO_EQUAL(llc_header->protocol,snap_proto_aarp)) {
/*
			kprintf("pat_input: Calling aarp_func dest=%s LLC_PROTO=%02x%02x\n",
				ether_sprintf(enet_header->dst),
				llc_header->protocol[3],
				llc_header->protocol[4]);
*/
	       		patp->aarp_func(gbuf_rptr(m), patp->context);
			gbuf_freem(m);
		      } 
		else if (LLC_PROTO_EQUAL(llc_header->protocol,snap_proto_ddp)) {
			/* if we're a router take all pkts */
		  if (!ROUTING_MODE) {
		    if (patp->addr_check(gbuf_rptr(m), patp->context)
			== AARP_ERR_NOT_OURS) {
/*
			kprintf("pat_input: Packet Rejected: not for us? dest=%s LLC_PROTO= %02x%02x\n",
				ether_sprintf(enet_header->dst),
				llc_header->protocol[3],
				llc_header->protocol[4]);
*/
		      gbuf_freem(m);
#ifdef CHECK_DDPR_FLAG
		      FETCH_AND_ADD((atomic_p)&ddprunning_flag, -1);
#endif
		      return EAFNOSUPPORT;
		    }
		  }
		  gbuf_set_type(m, MSG_DATA);
		  elap_input(m, patp->context, src); 
		} else {
		  gbuf_rdec(m,(ENET_LLC_SIZE));

#ifdef APPLETALK_DEBUG
		  kprintf("pat_input: Packet Rejected: wrong LLC_PROTO  dest=%s LLC_PROTO= %02x%02x\n",
					ether_sprintf(enet_header->dst),
					llc_header->protocol[3],
					llc_header->protocol[4]);
#endif
		  gbuf_freem(m);
#ifdef CHECK_DDPR_FLAG
		  FETCH_AND_ADD((atomic_p)&ddprunning_flag, -1);
#endif
		  return EAFNOSUPPORT;
		}
	      }
#ifdef CHECK_DDPR_FLAG
	FETCH_AND_ADD((atomic_p)&ddprunning_flag, -1);
#endif
	return 0;
} /* pat_input */

#else /* AIX section to end of file (not supported) */

/* from beginning of file ... */
#include <sys/cdli.h>
#include <sys/ndd.h>
static struct ns_8022 elap_link;     /* The SNAP header description */
static struct ns_user elap_user;     /* The interface to the demuxer */

int
pat_ifpresent(name) /* AIX */
	char *name;
{
	return (int)ifunit(name);
}

int 
pat_output(pat_id, mlist, dst_addr, type) /* AIX */
	int	pat_id;
	gbuf_t *mlist;
	unsigned char *dst_addr;
	int 	type;
{
	int len;
	pat_unit_t *patp;
	gbuf_t *m, *m_prev, *new_mlist, *m_temp;
	struct ndd *nddp;
	short size;
	enet_header_t *enet_header;
	llc_header_t *llc_header;

	patp = (pat_unit_t *)&pat_units[pat_id];
	if (patp->state != PAT_ONLINE) {
		gbuf_freel(mlist);
		return ENOTREADY;
	}

	if (patp->xtype == IFTYPE_NULLTALK) {
		gbuf_freel(mlist);
		return 0;
	}

	nddp = (void *)patp->nddp;
	new_mlist = 0;

  for (m = mlist; m; m = mlist) {
	mlist = gbuf_next(m);
	gbuf_next(m) = 0;

	gbuf_prepend(m,ENET_LLC_SIZE);
	if (m == 0) {
		if (mlist)
			gbuf_freel(mlist);
		if (new_mlist)
			gbuf_freel(new_mlist);
		return 0;
	}

	enet_header = (enet_header_t *)gbuf_rptr(m);
	bcopy(dst_addr, enet_header->dst, sizeof(enet_header->dst));
	bcopy(patp->xaddr, enet_header->src, sizeof(enet_header->src));
	size = gbuf_msgsize(m);
	enet_header->len = size - sizeof(enet_header_t);
	llc_header = (llc_header_t *)(gbuf_rptr(m)+sizeof(enet_header_t));
	*llc_header = (type == AARP_AT_TYPE) ? snap_hdr_aarp : snap_hdr_at;

	m->m_pkthdr.len = size;
	m->m_pkthdr.rcvif = 0;

	if (new_mlist)
		gbuf_next(m_prev) = m;
	else
		new_mlist = m;
	m_prev = m;
	pktsOut++;
  }

	if (new_mlist)
		(*nddp->ndd_output)(nddp, new_mlist);

	return 0;
}

int
pat_online (ifName, ifType)  /* AIX */
	char	*ifName;
	char	*ifType;
{
	void pat_input();
	int pat_id;
	pat_unit_t *patp;
	struct ndd *nddp;
	char ns_name[8];

	if ((pat_id = pat_ID(ifName)) == -1)
		return (-1);
	patp = &pat_units[pat_id];

	if (patp->xtype == IFTYPE_ETHERTALK) {
		ns_name[0] = ifName[0];
		ns_name[1] = 'n';
		strcpy(&ns_name[2], &ifName[1]);
	} else if (patp->xtype == IFTYPE_NULLTALK) {
		patp->xaddrlen = 6;
		bzero(patp->xaddr, patp->xaddrlen);
		if (ifType)
			*ifType = patp->xtype;
		patp->nddp = (void *)0;
		patp->state  = PAT_ONLINE;
		at_statep->flags |= AT_ST_IF_CHANGED;
		return (pat_id);
	} else
		return -1;

	if (ns_alloc(ns_name, &nddp))
		return -1;

	bzero(&elap_user, sizeof(elap_user));
	elap_user.isr = pat_input;
	elap_user.pkt_format = NS_HANDLE_HEADERS|NS_INCLUDE_MAC;

	elap_link.filtertype = NS_8022_LLC_DSAP_SNAP;
	elap_link.orgcode[0] = 0;
	elap_link.orgcode[2] = 0;
	elap_link.dsap = DSAP_SNAP;
	elap_link.ethertype = 0x80f3;	/* AARP SNAP code */
	if (ns_add_filter(nddp, &elap_link, sizeof(elap_link), &elap_user))
		return -1;

	elap_link.orgcode[0] = 0x08;
	elap_link.orgcode[2] = 0x07;
	elap_link.ethertype = 0x809b;	/* DDP SNAP code */
	if (ns_add_filter(nddp, &elap_link, sizeof(elap_link), &elap_user)) {
		elap_link.orgcode[0] = 0;
		elap_link.orgcode[2] = 0;
		elap_link.ethertype = 0x80f3;	/* AARP SNAP code */
		(void)ns_del_filter(nddp, &elap_link, sizeof(elap_link));
		return -1;
	}

	patp->xaddrlen = nddp->ndd_addrlen;
	bcopy(nddp->ndd_physaddr, patp->xaddr, patp->xaddrlen);

	if (ifType)
		*ifType = patp->xtype;

	patp->nddp = (void *)nddp;
	patp->state  = PAT_ONLINE;
	at_statep->flags |= AT_ST_IF_CHANGED;

	return (pat_id);
}

void
pat_offline(pat_id)  /* AIX */
	int pat_id;
{
	pat_unit_t *patp = &pat_units[pat_id];

	if (patp->state == PAT_ONLINE) {
	  if (patp->xtype != IFTYPE_NULLTALK) {
		elap_link.filtertype = NS_8022_LLC_DSAP_SNAP;
		elap_link.orgcode[0] = 0;
		elap_link.orgcode[2] = 0;
		elap_link.dsap = DSAP_SNAP;
		elap_link.ethertype = 0x80f3;	/* AARP SNAP code */
		(void)ns_del_filter(patp->nddp, &elap_link, sizeof(elap_link));
		elap_link.orgcode[0] = 0x08;
		elap_link.orgcode[2] = 0x07;
		elap_link.ethertype = 0x809b;	/* DDP SNAP code */
		(void)ns_del_filter(patp->nddp, &elap_link, sizeof(elap_link));
		ns_free(patp->nddp);
	  }
		at_statep->flags |= AT_ST_IF_CHANGED;
		bzero(patp, sizeof(pat_unit_t));
	}
}

int
pat_mcast(pat_id, control, data) /* AIX */
	int pat_id;
	int control;
	unsigned char *data;
{
	struct ndd *nddp;

	nddp = (struct ndd *)pat_units[pat_id].nddp;
	return (*nddp->ndd_ctl)(nddp, (control == PAT_REG_MCAST) ?
		NDD_ENABLE_ADDRESS : NDD_DISABLE_ADDRESS,
			data, nddp->ndd_addrlen);
}

void
pat_input(nddp, m, unused)  /* AIX */
	struct ndd *nddp;
	gbuf_t *m;
	void *unused;
{
	extern int ddprunning_flag;
	llc_header_t *llc_header;
	int pat_id;
	pat_unit_t *patp;
	char src[6];
	enet_header_t *enet_header = (enet_header_t *)gbuf_rptr(m);

	for (pat_id=0, patp = &pat_units[pat_id];
			pat_id < xpatcnt; pat_id++, patp++) {
		if ((patp->state == PAT_ONLINE) && (patp->nddp == nddp))
			break;
	}
	if (pat_id == xpatcnt) {
		gbuf_freem(m);
		return;
	}

	/* Ignore multicast packets from local station */
  	if (patp->xtype == IFTYPE_ETHERTALK) {
		bcopy((char *)enet_header->src, src, sizeof(src));
		if ((enet_header->dst[0] & 1) && 
				(bcmp(src, patp->xaddr, sizeof(src)) == 0)) {
			gbuf_freem(m);
			return;
		}
		llc_header = (llc_header_t *)(enet_header+1);
  	}

	gbuf_rinc(m,(ENET_LLC_SIZE));
	(void)fetch_and_add((atomic_p)&ddprunning_flag, 1);
	pktsIn++;
	if (LLC_PROTO_EQUAL(llc_header->protocol,snap_proto_aarp)) {
		patp->aarp_func(gbuf_rptr(m), patp->context);
		gbuf_freem(m);
	} else if (LLC_PROTO_EQUAL(llc_header->protocol,snap_proto_ddp)) {
		/* if we're a router take all pkts */
		if (!ROUTING_MODE) {
			if (patp->addr_check(gbuf_rptr(m), patp->context)
					== AARP_ERR_NOT_OURS) {
				gbuf_freem(m);
				(void)fetch_and_add((atomic_p)&ddprunning_flag, -1);
				return;
			}
		}
		gbuf_set_type(m, MSG_DATA);
		elap_input(m, patp->context, src);
	} else
		gbuf_freem(m);
	(void)fetch_and_add((atomic_p)&ddprunning_flag, -1);
}
#endif  /* AIX */
