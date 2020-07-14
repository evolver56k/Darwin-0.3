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
 *	Copyright (c) 1988, 1989, 1993-1998 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 */

/* at_elap.c: 2.0, 1.29; 10/4/93; Apple Computer, Inc. */

/* This is the file which implements all the streams driver 
 * functionality required for EtherTalk.
 */



#define RESOLVE_DBG				/* for debug.h global resolution */
#include <sysglue.h>
#include <sys/malloc.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/elap.h>
#include <lap.h>  /* for at_statep */
#include <routing_tables.h>     /* rtmp+zip table structs  */

#include <at/at_lap.h>
#include <at_pat.h>
#include <at_elap.h>
#include <at_aarp.h>
#include <at_ddp.h>
#include <at_zip.h>
#include <nbp.h>
#include <at_snmp.h>
#include <atlog.h>

#ifndef NULL
#define NULL 0
#endif

	/* globals */

elap_specifics_t	elap_specifics[IF_TYPE_ET_MAX];
at_state_t 	at_state;		/* global state of AT network */
at_state_t *at_statep = &at_state;
char *if_types[] = { IF_TYPE_1, IF_TYPE_2, IF_TYPE_3, IF_TYPE_4, IF_TYPE_5};
gref_t *shutdown_gref = NULL;
gref_t *at_qioctl = NULL;
gbuf_t *at_mioctl = NULL;
gbuf_t *at_delay_m = NULL;
gref_t *at_delay_gref = NULL;
int (*at_delay_func)() = NULL;
int at_delay_errno = 0;
char at_delay_flag = 0;
char at_ddp_debug;
int RoutingMix= 2000; /* default for nbr of ppsec */
snmpFlags_t		snmpFlags;

void ddp_bit_reverse();

	/* snmp defines */
#define MAX_BUFSIZE		8192   
#define MAX_RTMP		(MAX_BUFSIZE/sizeof(RT_entry)-1)
#define MAX_NBP 		\
	((MAX_BUFSIZE - SNMP_NBP_HEADER_SIZE)/sizeof(snmpNbpEntry_t)-1)
#define MAX_NBP_BYTES	(MAX_NBP * sizeof(snmpNbpEntry_t))
#define MAX_ZIP			(MAX_BUFSIZE/sizeof(ZT_entry)-1)
#define MAX_RTMP_BYTES	(MAX_RTMP * sizeof(RT_entry))
#define MAX_ZIP_BYTES	(MAX_ZIP * sizeof(ZT_entry))

	/* externs */
extern at_if_t 		*ifID_table[];
extern at_if_t		at_ifQueueHead;
extern int xpatcnt;
extern snmpStats_t	snmpStats;
extern atlock_t ddpinp_lock;
extern atlock_t arpinp_lock;
extern short appletalk_inited;
pat_unit_t pat_units[IF_TOTAL_MAX];


	/* protos */
extern snmpAarpEnt_t * getAarp(int *);
extern RT_entry *rt_getNextRoute(int);
extern ZT_entryno *zt_getNextZone(int);
extern int setLocalZones(at_nbptuple_t * , int);
extern int getRTRLocalZone(if_zone_t *);
extern at_nvestr_t *getDefZone(int);
StaticProc void getIfNames(if_name_t *);
StaticProc void get_ifs_stat();
StaticProc void add_route();
StaticProc int set_zones();
StaticProc void pat_init();
StaticProc void dodefer();
StaticProc int domcast();
StaticProc int elap_offline();
StaticProc void routerShutdown();
StaticProc void elap_hangup();
static getSnmpCfg();
	/* temp globals, etc. */

#define	NDEFERS	20
/* *** static int ndefers = NDEFERS;   not currently used *** */
static int deferno = 0;
static int gotIfs = 0;

#define ZT_BAD ZT_MAXENTRY +1



/***********************************************************************
 * lap_close()
 *
 **********************************************************************/
int lap_close(gref)
	gref_t *gref;
{
#ifdef CHECK_DDPR_FLAG
	extern int ddprunning_flag;
#endif
	int s;

	ddp_close(gref); /* for 2225395 */
	ATDISABLE(s, ddpinp_lock);
	if (gref == shutdown_gref) {
#ifdef CHECK_DDPR_FLAG
		while (ddprunning_flag)
			;
#endif
		if (at_mioctl != NULL) {
			gbuf_freem(at_mioctl);
			at_mioctl = NULL;
		}
		shutdown_gref = NULL;
		xpatcnt = 0;
		at_statep->flags = 0;
	}
	ATENABLE(s, ddpinp_lock);

	return 0;
}

StaticProc
validate_msg_size(m, gref, elapp)
	register gbuf_t *m;
	gref_t		*gref;
	elap_specifics_t **elapp;

/* checks ioctl message type for minimum expected message size & 
   sends error back if size invalid
*/
{
	register ioc_t		*iocbp;
	register at_elap_cfg_t		*cfgp;
	int i, size = 1;
	int if_specific = FALSE;
	
	*elapp = NULL;		
	iocbp = (ioc_t *) gbuf_rptr(m);

	switch (iocbp->ioc_cmd) {
	    case LAP_IOC_ONLINE:
		case LAP_IOC_OFFLINE:
		case ELAP_IOC_SWITCHZONE:
		case ELAP_IOC_SET_CFG:
		case ELAP_IOC_SET_ZONE :
			size = sizeof(at_elap_cfg_t);
			if_specific = TRUE;
			break;
		case LAP_IOC_ADD_ROUTE:
			size = sizeof(RT_entry);
			break;
		case LAP_IOC_ADD_ZONE:
			size = sizeof(if_zone_info_t);
			break;
		case LAP_IOC_GET_ROUTE:
			size = sizeof(RT_entry);
			break;
		case LAP_IOC_GET_ZONE:
			size = sizeof(ZT_entryno);
			break;
		case LAP_IOC_GET_IFID:
			size = sizeof(at_if_name_t);
			break;
		case LAP_IOC_SET_DBG:
			size = sizeof(dbgBits_t);
			break;
		case LAP_IOC_SNMP_GET_CFG:
		case LAP_IOC_SNMP_GET_AARP:
		case LAP_IOC_SNMP_GET_ZIP:
		case LAP_IOC_SNMP_GET_RTMP:
		case LAP_IOC_GET_DEFAULT_ZONE:
		case LAP_IOC_SNMP_GET_NBP:
			size = sizeof(int);
			break;
		case LAP_IOC_SET_MIX:
			size = sizeof(short);
			break;
		case LAP_IOC_GET_LOCAL_ZONE:
			size = sizeof(if_zone_t);
			break;
		case LAP_IOC_IS_ZONE_LOCAL:
			size = sizeof(at_nvestr_t);
			break;
		case LAP_IOC_CHECK_STATE:
			size = 1;
			break;
		case LAP_IOC_SET_DEFAULT_ZONES:
			size = sizeof(int) * IF_TOTAL_MAX;
			break;

		case ELAP_IOC_GET_STATS:
		case ELAP_IOC_GET_CFG:
		case LAP_IOC_GET_DBG:
		case LAP_IOC_GET_IFS_STAT: 
		case LAP_IOC_ROUTER_START:
		case LAP_IOC_ROUTER_SHUTDOWN:
		case LAP_IOC_ROUTER_INIT:
		case LAP_IOC_DO_DEFER:
		case LAP_IOC_DO_DELAY:
		case LAP_IOC_SHUT_DOWN:
		case LAP_IOC_SNMP_GET_DDP:
		case LAP_IOC_GET_MODE:
		case LAP_IOC_GET_IF_NAMES:
			size = 0;
			break;
				   		/* these ioctls  send variable length data */
		case LAP_IOC_SET_LOCAL_ZONES:
			size = -1;
			break;
		default:
			dPrintf(D_M_ELAP, D_L_ERROR, ("elap_wput: unknown ioctl\n"));
			goto error;
	}

	if (size == 0) {				/* a non-data ioctl */
		return(0);
	}

	/* if I/F related, validate further */
	if (if_specific) {
	  	/* get elapp from msg */
		int lap_id;
		cfgp = (at_elap_cfg_t * ) gbuf_rptr(gbuf_cont(m));
		lap_id = pat_ID(cfgp->if_name);
		if (lap_id < 0 ||  IF_ANY_MAX <= lap_id)  {
	 		ioc_ack(EINVAL, m, gref); 
			dPrintf(D_M_ELAP, D_L_ERROR, ("validate_msg:bad cmd (%x) for %s\n",
				iocbp->ioc_cmd, cfgp->if_name));
			return(EINVAL);
		}
		*elapp = &elap_specifics[lap_id]; 
	}	

	if (gbuf_cont(m) != NULL) {
		i = gbuf_len(gbuf_cont(m));
		if (size == -1)
			if (i >1)
				return(0);
			else
				goto error;
	}
	if (iocbp->ioc_count < size || (gbuf_cont(m) == NULL) || i < size) {
			dPrintf(D_M_ELAP, D_L_ERROR,
				("ioctl msg error:s:%d c:%d bcont:%c delta:%d\n",
				   size, iocbp->ioc_count,
				   gbuf_cont(m)? 'Y' : 'N', i));
	   goto error;
	}
	else
		return(0);
error:
	ioc_ack(EMSGSIZE, m, gref);
	return (EMSGSIZE);
} /* validate_msg_size */

void
elap_input(mp, elapp, src)
register elap_specifics_t *elapp;
register gbuf_t		*mp;
register char *src;
{
	/* Let ddp glean link address if it wishes to */
	elapp->stats.rcv_packets++;
	elapp->stats.rcv_bytes += gbuf_msgsize(mp);

	if (!MULTIPORT_MODE)
		ddp_glean (mp, &elapp->elap_if, src);
	gbuf_next(mp) = 0;
	ddp_input(mp, &elapp->elap_if);
} /* elap_input */


/***********************************************************************
 * elap_wput()
 *
 **********************************************************************/
int
elap_wput(gref, m)
gref_t 		*gref;
register	gbuf_t	*m;
{
	elap_specifics_t 	*elapp;
	register ioc_t		*iocbp;
	register at_elap_cfg_t	*cfgp;
	at_elap_stats_t		*statsp;
	static int		ifID_entry = 0;
	int			elap_online();
	int			error;
	int			i;
	int			(*func)();
	gbuf_t		*tmpm;
	pat_unit_t *patp;


	switch (gbuf_type(m)) {
	case MSG_DATA:
			gbuf_freem(m);
			dPrintf(D_M_ELAP,D_L_ERROR,
				("Output data to control channel is ignored\n"));
	break;

	case MSG_IOCTL:
		iocbp = (ioc_t *) gbuf_rptr(m);
		if (!xpatcnt) {
			gotIfs = 0;
			if (!(at_statep->flags & AT_ST_PAT_INIT)) {
				bzero(elap_specifics, sizeof(elap_specifics));
				pat_init();
				at_statep->flags |= AT_ST_PAT_INIT;
				at_ddp_debug = 0;
			}
		}

		switch (iocbp->ioc_cmd) {
		case LAP_IOC_ADD_IFNAME:
			/* Has this interface already been added? */
			if ((i = pat_ID(gbuf_rptr(gbuf_cont(m)))) != -1) {
			   ioc_ack(0, m, gref);
			   return 0;
			}

			shutdown_gref = gref;
			patp = &pat_units[xpatcnt];
			bcopy(gbuf_rptr(gbuf_cont(m)), patp->xname, sizeof(patp->xname));
			patp->xname[sizeof(patp->xname)-1] = '\0';
			patp->xunit = (char)IF_UNIT(patp->xname);
			if (strncmp(IF_TYPE_1, patp->xname, 2) == 0)
				patp->xtype = IFTYPE_ETHERTALK;
			else if (strncmp(IF_TYPE_3, patp->xname, 2) == 0)
				patp->xtype = IFTYPE_FDDITALK;
			else if (strncmp(IF_TYPE_4, patp->xname, 2) == 0)
				patp->xtype = IFTYPE_TOKENTALK;
			else if (strncmp(IF_TYPE_5, patp->xname, 2) == 0)
				patp->xtype = IFTYPE_NULLTALK;
			else
				patp->xtype = IFTYPE_LOCALTALK;
			ioc_ack(0, m, gref);
			xpatcnt++;
			return 0;

		/* *** fix this later: this code may no longer be needed *** */
		case LAP_IOC_DEL_IFNAME:
			if ((i = pat_ID(gbuf_rptr(gbuf_cont(m)))) != -1) {
				patp = &pat_units[i];
				patp->xname[0] = '\0';
			}
			ioc_ack(0, m, gref);
			return 0;
		}
		/* *** end of code that may no longer be needed *** */

		if (validate_msg_size(m, gref, &elapp))
			break;	

		if (elapp)
			cfgp = (at_elap_cfg_t*) gbuf_rptr(gbuf_cont(m));

		if (LAP_IOC_MYIOCTL(iocbp->ioc_cmd) || 
			ELAP_IOC_MYIOCTL(iocbp->ioc_cmd)) {

			switch (iocbp->ioc_cmd) {

			case LAP_IOC_CHECK_STATE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_CHECK_STATE\n");
#endif
				error = (shutdown_gref == NULL) ? ENOTREADY : 0;
				if (error == 0) {
					dPrintf(D_M_ELAP,D_L_INFO,
						("elap_wput: CHECK_STATE 1\n"));
//### LD 7/23 was 0x1000: too big for 2K cluster size
					if ((tmpm = gbuf_alloc(0x512, PRI_HI)) == NULL)
						ioc_ack(ENOBUFS, m, gref);
					else {
						*gbuf_rptr(tmpm) = *gbuf_rptr(gbuf_cont(m));
						gbuf_wset(tmpm,sizeof(int));
						gbuf_freeb(gbuf_cont(m));
						gbuf_cont(m) = tmpm;
						ddp_stop(m, gref);
					}
				} else {
					dPrintf(D_M_ELAP,D_L_INFO,
						("elap_wput: CHECK_STATE 2\n"));
					ioc_ack(error, m, gref);
				}
				break;

			/* *** fix this later: this code may no longer be needed *** */
			case LAP_IOC_SHUT_DOWN:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SHUT_DOWN\n");
#endif
				if (shutdown_gref != NULL) {
					if ((tmpm = gbuf_alloc(1, PRI_HI)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}
					gbuf_wset(tmpm,1);
					*gbuf_rptr(tmpm) = ESHUTDOWN;
					gbuf_set_type(tmpm, MSG_ERROR);
					atalk_putnext(shutdown_gref, tmpm);
				}
				ioc_ack(0, m, gref);
				break;
			/* *** end of code that may no longer be needed *** */

			case LAP_IOC_DO_DEFER:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_DO_DEFER\n");
#endif
			if (gbuf_cont(m) == NULL) {
				dodefer();
				ioc_ack(0, m, gref);
			} else {
				ioc_ack(EINVAL, m, gref);
			}
				break;
			case LAP_IOC_DO_DELAY:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_DO_DELAY\n");
#endif
				dodefer();
				if ((func = at_delay_func) != NULL) {
					at_delay_func = NULL;
					error = (*func)(at_delay_flag, m, gref);
					if (error != ENOTREADY)
						ioc_ack(error, m, gref);
				} else
					ioc_ack(0, m, gref);
				break;

		    case LAP_IOC_ONLINE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ONLINE\n");
#endif
				if ( elapp->elap_if.ifState == LAP_ONLINE ||
				     (elapp->elap_if.ifState == LAP_ONLINE_ZONELESS &&
					   !(cfgp->flags & ELAP_CFG_SET_RANGE)
					 )
				   ) {
					/* This don't make much sense...
					 * we're already ONLINE!
					 */
					ioc_ack(EALREADY, m, gref);
					break;
				}
				elapp->flags = cfgp->flags;
				if (elapp->flags & ELAP_CFG_HOME) {
					/* copy over the configured zone if any */
					if (cfgp->zonename.len)
						bcopy((caddr_t) &cfgp->zonename,
						      (caddr_t) &elapp->elap_if.ifZoneName, 
						      cfgp->zonename.len+1);
					if  (ifID_table[IFID_HOME])  {
						/* if home flag is set, and
						 * there is already a home
						 * ifID, it better be us!
						 */
						if (&elapp->elap_if !=
						    ifID_table[IFID_HOME] ) {
							/* only 1 home allowed! */
							ioc_ack(EEXIST, m, gref);
							break;
						}
						dPrintf(D_M_ELAP, D_L_STARTUP, ("elap_wput home I/F:%s\n",
							cfgp->if_name));
					}
					else {
						/* home port designation not
						 * allowed unless
						 * i/f is off line
						 */
						if ( elapp->elap_if.ifState != LAP_OFFLINE) {
			    			ioc_ack(EPERM, m, gref);			
							break;
						}
					}
				} /* if CFG_HOME */

				if (elapp->elap_if.ifState == LAP_OFFLINE)
					bzero ((caddr_t) &elapp->elap_if, sizeof(at_if_t));

				if (elapp->flags & ELAP_CFG_SET_RANGE ||
				    elapp->flags & ELAP_CFG_SEED) {

					if ( elapp->elap_if.ifState != LAP_ONLINE_ZONELESS &&
					     elapp->elap_if.ifState != LAP_OFFLINE
					   ) {				/* can't change range if ONLINE */
			    		ioc_ack(EFAULT, m, gref);			
						break;
					}
					dPrintf(D_M_ELAP, D_L_STARTUP_INFO,
						("elap_wput: found to be seed/set range\n"));


					ATALK_ASSIGN(elapp->cfg.initial_addr, 0, 0, 0);
					elapp->elap_if.ifThisCableStart = cfgp->netStart;
					elapp->elap_if.ifThisCableEnd   = cfgp->netEnd;
				}
				else {
					dPrintf(D_M_ELAP,D_L_ERROR, 
						("elap_wput: we believe we're not seed\n"));
				}

				at_delay_errno = 0;
				error = elap_online (gref, cfgp->if_name, NULL, m); 

				if (error != ENOTREADY)
					ioc_ack(error, m, gref);
				break;

			case LAP_IOC_OFFLINE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_OFFLINE\n");
#endif
				if (!xpatcnt) {
					ioc_ack(0, m, gref);
					break;
				}
				error = elap_offline(elapp);
				ioc_ack(error, m, gref);
				break;

			case LAP_IOC_GET_IFS_STAT:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_IFS_STAT\n");
#endif
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(if_cfg_t), 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				get_ifs_stat(gbuf_rptr(gbuf_cont(m)));
				gbuf_wset(gbuf_cont(m),sizeof(if_cfg_t));
				iocbp->ioc_count = sizeof(if_cfg_t);
				ioc_ack(0, m, gref);
				ifID_entry = 0;
				break;
				
			case ELAP_IOC_GET_CFG:
#ifdef APPLETALK_DEBUG
kprintf("ELAP_IOC_GET_CFG\n");
#endif
				/* an at_elap_cfg_t is passed down with this
				   ioctl, the elapp is obtained from it via
				   the if_name we then send back a copy of the
				   current at_elap_cfg_t by gbuf_alloc'ing a new
				   message 
				 */
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(at_elap_cfg_t), 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				cfgp = (at_elap_cfg_t *) gbuf_rptr(gbuf_cont(m));
                                if  (ifID_table[IFID_HOME])  {
			    	   elapp = (elap_specifics_t *)ifID_table[IFID_HOME]->ifLapp;
				   if (elapp) {
			    		*cfgp = elapp->cfg;
					gbuf_wset(gbuf_cont(m),sizeof(at_elap_cfg_t));
					iocbp->ioc_count = sizeof(at_elap_cfg_t);
					ioc_ack(0, m, gref);
				        }
				   else
					ioc_ack(EINVAL, m, gref);
				   }
                                else
				   ioc_ack(EINVAL, m, gref);
				break;

			case LAP_IOC_GET_IFID:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_IFID\n");
#endif
				/* return at_if_t struct for requested interface */
			{
				at_if_name_t *ifIN;
				at_if_t *ifID;
				int lap_id;

				ifIN = (at_if_name_t *)gbuf_rptr(gbuf_cont(m));
				lap_id = pat_ID(ifIN->if_name);
				{
					if (lap_id < 0 || IF_ANY_MAX <= lap_id)  {
						ioc_ack(EINVAL, m, gref);
						break;
					}
					else {
						ifID = &elap_specifics[lap_id].elap_if;
						gbuf_freem(gbuf_cont(m));
						gbuf_cont(m) = NULL;

						if ((gbuf_cont(m) = gbuf_alloc(sizeof(at_if_name_t), 
				    		PRI_MED)) == NULL) {
							ioc_ack(ENOBUFS, m, gref);
							break;
						}
						ifID = &elap_specifics[lap_id].elap_if; 
						ifIN = (at_if_name_t *)gbuf_rptr(gbuf_cont(m));
						ifIN->ifID = *ifID; 
						gbuf_wset(gbuf_cont(m),sizeof(at_if_name_t));
						iocbp->ioc_count = sizeof(at_if_name_t);
					}
				}
				ioc_ack(0, m, gref);
			}
			break;
				

			case ELAP_IOC_GET_STATS:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_STATS\n");
#endif
				if ( (gbuf_cont(m) == NULL)
						|| ((i = pat_ID(gbuf_rptr(gbuf_cont(m)))) == -1) ) {
					ioc_ack(EINVAL, m, gref);
					break;
				}
				gbuf_freem(gbuf_cont(m));
				if ((gbuf_cont(m) =gbuf_alloc(sizeof(at_elap_stats_t), 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				statsp = ((at_elap_stats_t *)gbuf_rptr(gbuf_cont(m)));
				elapp = &elap_specifics[i]; 
				if (elapp) {
					*statsp = elapp->stats;
					gbuf_wset(gbuf_cont(m),sizeof(at_elap_stats_t));
					iocbp->ioc_count = sizeof(at_elap_stats_t);
					ioc_ack(0, m, gref);
				}
				else 
					ioc_ack(EINVAL, m, gref);
				break;
			case ELAP_IOC_SET_CFG:
#ifdef APPLETALK_DEBUG
kprintf("ELAP_IOC_SET_CFG\n");
#endif
				if (elapp->elap_if.ifState == LAP_ONLINE) {
					/* Can not allow setting config after
					 * we're already ONLINE
					 */
					ioc_ack(EALREADY, m, gref);
					break;
				}
				ATALK_ASSIGN(elapp->cfg.initial_addr, 0, 0, 0);
		
				bcopy ((caddr_t) cfgp->if_name, (caddr_t) elapp->cfg.if_name, AT_IF_NAME_LEN);
				if (ATALK_VALUE(cfgp->initial_addr)) {
					at_net_al	initial_net;
					at_node	initial_node;

					initial_node = *(at_node *)&cfgp->
							initial_addr.atalk_node;
					initial_net = NET_VALUE(cfgp->
							initial_addr.atalk_net);
					/*
					 * The net effect of the code that was here after A/UX
					 * version 29 was to ignore the prior node number settings. 
					 * As a result * Barracuda 3.0.1 machines cannot "remember" 
					 * their AppleTalk node number from the last AppleTalk 
					 * session.
					 * The following code reverts this code back to pre version 
					 * 29 style.
					 * The problem that version 29 was fixing really happens because
					 * readxpram (and writexpram)  in lap_init was returning a -1 
					 * instead of 0 on failure.
					 * Madan Valluri. May 12, 1993.
					 */
					if ((initial_node<0xfe) && (initial_node>0) &&
					    !((initial_net == 0) ||
						((initial_net >= DDP_STARTUP_LOW)&&
					  	(initial_net <= DDP_STARTUP_HIGH))))
						elapp->cfg.initial_addr = 
						    cfgp->initial_addr;
				}
				ioc_ack(0, m, gref);
				break;

			case ELAP_IOC_SET_ZONE :
#ifdef APPLETALK_DEBUG
kprintf("ELAP_IOC_SET_ZONE\n");
#endif
				/* ioctl to set the desired zone name for the 
				 * interface and when no router is present, to set
				 * multicast addr for pram zone for when router returns
				 */
				cfgp = (at_elap_cfg_t * ) gbuf_rptr(gbuf_cont(m));

				if (cfgp->flags & ELAP_CFG_ZONE_MCAST) {
					nbp_add_multicast(&cfgp->zonename, &elapp->elap_if);
				}
				else  {
					if (elapp->elap_if.ifState == LAP_ONLINE) {
						/* Can not allow setting zonename
					 	* after we're already ONLINE
						 */
						ioc_ack(EALREADY, m, gref);
						break;
					}
				}

				if (cfgp->zonename.len)
					bcopy((caddr_t) &cfgp->zonename, 
						(caddr_t) &elapp->cfg.zonename, 
						cfgp->zonename.len+1);
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_ADD_ZONE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ADD_ZONE\n");
#endif
				i = set_zones((if_zone_info_t *)gbuf_rptr(gbuf_cont(m)));
				ioc_ack(i, m, gref);
				break;

			case LAP_IOC_ADD_ROUTE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ADD_ROUTE\n");
#endif
				add_route((RT_entry *)gbuf_rptr(gbuf_cont(m)));
				ioc_ack(0, m, gref);
				break;

			case LAP_IOC_ROUTER_INIT:	/* set routing tables sizes*/
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ROUTER_INIT\n");
#endif
			{
				router_init_t *p = (router_init_t *)gbuf_rptr(gbuf_cont(m));

				at_state.flags = p->flags;
				if (rt_table_init(p) == ENOBUFS) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				ioc_ack(0, m, gref);
			}
				break;
			case LAP_IOC_ROUTER_START:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ROUTER_START\n");
#endif
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				routerStart(0, m, gref);
				break;

			case LAP_IOC_ROUTER_SHUTDOWN:
			{ 
				int i, lap_id;
				gbuf_t *tmpm;

#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_ROUTER_SHUTDOWN\n");
#endif

				routerShutdown(); 

				/* go through ifID_table */
				for (i = 0; i < IF_TOTAL_MAX; i++)
				    if (ifID_table[i] && ifID_table[i]->ifName) {
					  lap_id = pat_ID(ifID_table[i]->ifName);

					  /* was LAP_IOC_OFFLINE processing */
					  elap_offline(&elap_specifics[lap_id]);

					  /* was LAP_IOC_DEL_IFNAME processing */
					  pat_units[lap_id].xname[0] = '\0';
				    }

				/* was LAP_IOC_SHUT_DOWN processing */
				if (shutdown_gref != NULL) {
					if ((tmpm = gbuf_alloc(1, PRI_HI)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}
					gbuf_wset(tmpm,1);
					*gbuf_rptr(tmpm) = ESHUTDOWN;
					gbuf_set_type(tmpm, MSG_ERROR);
					atalk_putnext(shutdown_gref, tmpm);
				}
				ioc_ack(0, m, gref);
			}
				break;


			case LAP_IOC_SET_DBG :
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SET_DBG\n");
#endif
				dbgBits = *(dbgBits_t *)gbuf_rptr(gbuf_cont(m));
				ioc_ack(0, m, gref);
				break;


			case LAP_IOC_GET_DBG:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_DBG\n");
#endif
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(dbgBits_t), 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				 *(dbgBits_t *)gbuf_rptr(gbuf_cont(m)) = dbgBits;
				gbuf_wset(gbuf_cont(m),sizeof(dbgBits_t));
				iocbp->ioc_count = sizeof(dbgBits_t);
				ioc_ack(0, m, gref);
				break;

			case LAP_IOC_GET_ZONE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_ZONE\n");
#endif
						/* return next ZT_entryno from ZT_table 
						   a pointer to the struct ZT_entryno is passed down from
						   user space and the first byte is cast to a int, if
						   this int is non-zero, then the first ZT_entry is
						   returned and subsequent calls with a zero value
						   will return the next entry in the table. The next
						   read after the last valid entry will return ENOMSG
						 */
			{
				ZT_entryno *pZTe;

				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;

				pZTe = zt_getNextZone(i);
				if (pZTe) {
					if ((gbuf_cont(m) = gbuf_alloc(sizeof(ZT_entryno), PRI_MED)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}	
					*(ZT_entryno *)gbuf_rptr(gbuf_cont(m)) = *pZTe;
					gbuf_wset(gbuf_cont(m),sizeof(ZT_entryno));
					iocbp->ioc_count = sizeof(ZT_entryno);
					ioc_ack(0, m, gref);
				}
				else
					ioc_ack(ENOMSG, m, gref);
			}
				break;
				
			case LAP_IOC_GET_DEFAULT_ZONE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_DEFAULT_ZONE\n");
#endif
				{
				at_nvestr_t	 *pnve;

				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				pnve = getDefZone(i);
				if (pnve) {
					if ((gbuf_cont(m) = gbuf_alloc(sizeof(*pnve), PRI_MED)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}	
					*(at_nvestr_t *)gbuf_rptr(gbuf_cont(m)) = *pnve;
					gbuf_wset(gbuf_cont(m),sizeof(*pnve));
					iocbp->ioc_count = sizeof(*pnve);
				}
				else 
					iocbp->ioc_count = 0;
				}
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_GET_LOCAL_ZONE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_LOCAL_ZONE\n");
#endif
				if (!(ROUTING_MODE || MULTIHOME_MODE)) {
					ioc_ack(EPERM, m, gref);
					break;
				}
			{
				if_zone_t 	 *ifz;
				int zone;

				zone =  ((if_zone_t *)gbuf_rptr(gbuf_cont(m)))->ifzn.zone;
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(if_zone_t), PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}	
				ifz = (if_zone_t *)gbuf_rptr(gbuf_cont(m));
				ifz->ifzn.zone = zone;
				getRTRLocalZone(ifz);
				gbuf_wset(gbuf_cont(m),sizeof(*ifz));
				iocbp->ioc_count = sizeof(*ifz);
			}
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_GET_ROUTE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_ROUTE\n");
#endif
				/* return next RT_entry from RT_table 
				 * a pointer to the struct RT_entry is
				 * passed down from user space and the first
				 * byte is cast to a int, if this int is
				 * non-zero, then the first RT_entry is
				 * returned and subsequent calls with a
				 * zero value will return the next entry in
				 * the table. The next read after the last
				 * valid entry will return ENOMSG
				 */
			{
				RT_entry *pRT;

				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;

				pRT = rt_getNextRoute(i);
				if (pRT) {
					if ((gbuf_cont(m) = gbuf_alloc(sizeof(RT_entry), PRI_MED)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}	
					*(RT_entry *)gbuf_rptr(gbuf_cont(m)) = *pRT;
					gbuf_wset(gbuf_cont(m),sizeof(RT_entry));
					iocbp->ioc_count = sizeof(RT_entry);
					ioc_ack(0, m, gref);
				}
				else
					ioc_ack(ENOMSG, m, gref);
			}
				break;
			
			case LAP_IOC_SNMP_GET_DDP:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_DDP\n");
#endif
				if (!shutdown_gref) {
					ioc_ack(ENOTREADY, m, gref);
					break;
				}
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(snmpStats_t), 
						PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				
				*(snmpStats_t *)gbuf_rptr(gbuf_cont(m)) = snmpStats;
				gbuf_wset(gbuf_cont(m),sizeof(snmpStats));
				iocbp->ioc_count = sizeof(snmpStats);
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_SNMP_GET_CFG:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_CFG\n");
#endif
			{
				int i,size;
				snmpCfg_t 	snmp;

				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if (!shutdown_gref) {		/* if stack down */
					iocbp->ioc_count = 0;
					ioc_ack(ENOTREADY, m, gref);
					dPrintf(D_M_ELAP_LOW, D_L_INFO,
						("elap_wput: cfg req, stack down\n"));
					break;
				}
				if (i == UPDATE_IF_CHANGED && 
					!(at_statep->flags & AT_ST_IF_CHANGED)) {
					iocbp->ioc_count = 0;
					ioc_ack(0, m, gref);
					dPrintf(D_M_ELAP_LOW, D_L_INFO,
						("elap_wput: cfg req, unchanged\n"));
					break;
				}
				dPrintf(D_M_ELAP_LOW, D_L_INFO,
					("elap_wput: cfg req, changed\n"));

				if (getSnmpCfg(&snmp)) {
					dPrintf(D_M_ELAP,D_L_ERROR,
						("elap_wput:SNMP_GET_CFG error\n"));
					ioc_ack(ENOMSG, m, gref);
					break;
				}
					/* send up only used part of table */
				size = sizeof(snmp) - 
					   sizeof(snmpIfCfg_t) * (MAX_IFS - snmp.cfg_ifCnt);

				if ((gbuf_cont(m) = gbuf_alloc(size, PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				bcopy(&snmp,gbuf_rptr(gbuf_cont(m)),size);
				gbuf_wset(gbuf_cont(m),size);
				iocbp->ioc_count = size;
				at_statep->flags &= ~AT_ST_IF_CHANGED;
				ioc_ack(0, m, gref);
			}
			break;

			case LAP_IOC_SNMP_GET_AARP:
			{
				snmpAarpEnt_t *snmpp;
				int bytes;
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_AARP\n");
#endif
				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				dPrintf(D_M_ELAP,D_L_INFO,
					("elap_wput:calling getarp,i=%d\n", i));
				snmpp = getAarp(&i); 
				bytes = i * sizeof(snmpAarpEnt_t);
				dPrintf(D_M_ELAP,D_L_INFO,
					("elap_wput:getarp returned, i=%d,bytes=%d\n", 
					i, bytes));
				if (snmpp) {
					if ((gbuf_cont(m) = gbuf_alloc(bytes, PRI_MED)) == NULL) {
						ioc_ack(ENOBUFS, m, gref);
						break;
					}	
					bcopy(snmpp, gbuf_rptr(gbuf_cont(m)), bytes);
					gbuf_wset(gbuf_cont(m),bytes);
					iocbp->ioc_count = bytes;
					ioc_ack(0, m, gref);
				}
				else
					ioc_ack(ENOMSG, m, gref);
			}
			break;

			case LAP_IOC_SNMP_GET_ZIP:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_ZIP\n");
#endif
			{ /* matching brace NOT in this case */
				register int i,j;
				register int size, total, tabsize;
				gbuf_t	*mn;		/* new gbuf */
				gbuf_t	*mo;		/* old gbuf */
				gbuf_t	*mt;		/* temp */
				snmpNbpTable_t		*nbp;

				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if (!shutdown_gref) {
					ioc_ack(ENOTREADY, m, gref);
					break;
				}
				if (i == UPDATE_IF_CHANGED && 
					!(at_statep->flags & AT_ST_ZT_CHANGED)) {
					iocbp->ioc_count = 0;
					ioc_ack(0, m, gref);
					break;
				}
				mo=(gbuf_t*)NULL;
				tabsize = getZipTableSize();

					/* retrieve table into multiple gbufs */
				for (i =0; i<tabsize;  i+=j) {
					j = tabsize - i > 
						MAX_ZIP ? MAX_ZIP : tabsize - i;
					size = j < MAX_ZIP ? sizeof(ZT_entry)*j : MAX_ZIP_BYTES;
					if ((mn = gbuf_alloc(size, PRI_MED)) == NULL) {
						if (gbuf_cont(m))
							gbuf_freem(gbuf_cont(m));
						ioc_ack(ENOBUFS, m, gref);
						break;
					}
					if (!mo)	{ 		/* if first new one */
						mt = mn;
						total = size;
					}
					else {
						gbuf_cont(mo) = mn;
						total += size;
					}
					mo = mn;
					getZipTable((ZT_entry*)gbuf_rptr(mn),i,j); 
					gbuf_wset(mn,size);
				}
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(int), PRI_MED)) == NULL) {
					if (mt)
						gbuf_freem(mt);
					iocbp->ioc_count = 0;
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				if (!tabsize) {
					dPrintf(D_M_ELAP,D_L_WARNING,
						("elap_wput:snmp: empty zip table\n"));
					total = 0;
				}
				*(int*)gbuf_rptr(gbuf_cont(m)) = total; 	/* return table size */
				gbuf_wset(gbuf_cont(m),sizeof(int));
				iocbp->ioc_count = sizeof(int);
				ioc_ack(0, m, gref);
				if (tabsize)
					atalk_putnext(gref,mt);		/* send up table */
				at_statep->flags &= ~AT_ST_ZT_CHANGED;
				break;

			case LAP_IOC_SNMP_GET_RTMP:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_RTMP\n");
#endif
				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if (!shutdown_gref) {
					ioc_ack(ENOTREADY, m, gref);
					break;
				}
				if (i == UPDATE_IF_CHANGED && 
					!(at_statep->flags & AT_ST_RT_CHANGED)) {
					iocbp->ioc_count = 0;
					ioc_ack(0, m, gref);
					break;
				}

				mo=(gbuf_t*)NULL;
				tabsize = getRtmpTableSize();

					/* retrieve table into multiple gbufs */
				for (i =0; i<tabsize;  i+=j) {
					j = tabsize - i > 
						MAX_RTMP ? MAX_RTMP : tabsize - i;
					size = j < MAX_RTMP ? sizeof(RT_entry)*j : MAX_RTMP_BYTES;
					if ((mn = gbuf_alloc(size, PRI_MED)) == NULL) {
						if (gbuf_cont(m))
							gbuf_freem(gbuf_cont(m));
						ioc_ack(ENOBUFS, m, gref);
						break;
					}
					if (!mo)	{ 		/* if first new one */
						mt = mn;
						total = size;
					}
					else {
						gbuf_cont(mo) = mn;
						total += size;
					}
					mo = mn;
					getRtmpTable((RT_entry*)gbuf_rptr(mn),i,j); 
					gbuf_wset(mn,size);
				}
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(int), PRI_MED)) == NULL) {
					if (mt)
						gbuf_freem(mt);
					iocbp->ioc_count = 0;
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				if (!tabsize)
					total = 0;
				*(int*)gbuf_rptr(gbuf_cont(m)) = total;	/* return table size */
				gbuf_wset(gbuf_cont(m),sizeof(int));
				iocbp->ioc_count = sizeof(int);
				ioc_ack(0, m, gref);
				if (tabsize)
					atalk_putnext(gref,mt);		/* send up table */
				at_statep->flags &= ~AT_ST_RT_CHANGED;
				break;

			case LAP_IOC_SNMP_GET_NBP:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SNMP_GET_NBP\n");
#endif
				i =  *(int *)gbuf_rptr(gbuf_cont(m));
				gbuf_freem(gbuf_cont(m));
				gbuf_cont(m) = NULL;
				if (!shutdown_gref) {
					ioc_ack(ENOTREADY, m, gref);
					break;
				}
				if (i == UPDATE_IF_CHANGED && 
					!(at_statep->flags & AT_ST_NBP_CHANGED)) {
					iocbp->ioc_count = 0;
					ioc_ack(0, m, gref);
					dPrintf(D_M_ELAP_LOW, D_L_INFO,
						("elap_wput: nbp req denied, no change\n"));
					break;
				}

				mo=(gbuf_t*)NULL;
				tabsize = getNbpTableSize();

					/* retrieve table into multiple gbufs */
				for (i =0; i<tabsize;  i+=j) {
					j = tabsize - i > 
						MAX_NBP ? MAX_NBP : tabsize - i;
					size = j < MAX_NBP ? sizeof(snmpNbpEntry_t)*j : MAX_NBP_BYTES;
					if (!i)
						size += SNMP_NBP_HEADER_SIZE;
					if ((mn = gbuf_alloc(size, PRI_MED)) == NULL) {
						if (gbuf_cont(m))
							gbuf_freem(gbuf_cont(m));
						ioc_ack(ENOBUFS, m, gref);
						break;
					}
					if (!mo)	{ 		/* if first new one */
						mt = mn;
						total = size;
						nbp = (snmpNbpTable_t*)gbuf_rptr(mn);
						nbp->nbpt_entries = tabsize;
						nbp->nbpt_zone = 
							ifID_table[IFID_HOME]->ifZoneName;
						getNbpTable(nbp->nbpt_table,i,j); 
					}
					else {
						gbuf_cont(mo) = mn;
						total += size;
						getNbpTable((snmpNbpEntry_t *)gbuf_rptr(mn),i,j); 
					}
					mo = mn;
					gbuf_wset(mn,size);
				}
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(int), PRI_MED)) == NULL) {
					if (mt)
						gbuf_freem(mt);
					iocbp->ioc_count = 0;
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				if (!tabsize)
					total = 0;
				*(int*)gbuf_rptr(gbuf_cont(m)) = total;	/* return table size */
				gbuf_wset(gbuf_cont(m),sizeof(int));
				iocbp->ioc_count = sizeof(int);
				ioc_ack(0, m, gref);
				if (tabsize)
					atalk_putnext(gref,mt);		/* send up table */
				at_statep->flags &= ~AT_ST_NBP_CHANGED;
				break;
			}
				
			case LAP_IOC_SET_MIX:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SET_MIX\n");
#endif
				RoutingMix = (*(short *)gbuf_rptr(gbuf_cont(m)) & 0xFFFF) * 2;
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_SET_LOCAL_ZONES:
				{	int i;
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SET_LOCAL_ZONES\n");
#endif
					i = setLocalZones((at_nbptuple_t*)gbuf_rptr(gbuf_cont(m)),
						gbuf_len(gbuf_cont(m)));
					ioc_ack(i,m,gref);
				}
				break;
				
			case LAP_IOC_IS_ZONE_LOCAL:
				{
					at_nvestr_t *zone;
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_IS_ZONE_LOCAL\n");
#endif
					zone = (at_nvestr_t *)gbuf_rptr(gbuf_cont(m));
					if (isZoneLocal(zone))
						ioc_ack(0,m,gref);
					else
						ioc_ack(ENOENT,m,gref);
				}
				break;
			case LAP_IOC_GET_MODE:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_MODE\n");
#endif
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(int), 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				if (MULTIHOME_MODE)
				 	*(int *)gbuf_rptr(gbuf_cont(m)) =  AT_MODE_MHOME;
				else if (ROUTING_MODE) 
				 	*(int *)gbuf_rptr(gbuf_cont(m)) =  AT_MODE_ROUTER; 
				else
				 	*(int*)gbuf_rptr(gbuf_cont(m)) =  AT_MODE_SPORT;
				gbuf_wset(gbuf_cont(m),sizeof(int));
				iocbp->ioc_count = sizeof(int);
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_GET_IF_NAMES:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_GET_IF_NAMES\n");
#endif
				if ((gbuf_cont(m) = gbuf_alloc(sizeof(if_name_t) * IF_ANY_MAX, 
			    		PRI_MED)) == NULL) {
					ioc_ack(ENOBUFS, m, gref);
					break;
				}
				getIfNames((if_name_t *)gbuf_rptr(gbuf_cont(m)));
				gbuf_wset(gbuf_cont(m),sizeof(if_name_t) * IF_ANY_MAX);
				iocbp->ioc_count   = sizeof(if_name_t) * IF_ANY_MAX;
				ioc_ack(0, m, gref);
				break;
			case LAP_IOC_SET_DEFAULT_ZONES:
#ifdef APPLETALK_DEBUG
kprintf("LAP_IOC_SET_DEFAULT_ZONES\n");
#endif
				setDefaultZones((int*)gbuf_rptr(gbuf_cont(m)));
				ioc_ack(0, m, gref);
				break;

			default:
#ifdef APPLETALK_DEBUG
kprintf("unknown ioctl %d\n", iocbp->ioc_cmd);
#endif
				ioc_ack(ENOTTY, m, gref);
				dPrintf(D_M_ELAP, D_L_WARNING,
					("elap_wput: unknown ioctl (%d)\n", iocbp->ioc_cmd));

				if (elapp)
					elapp->stats.unknown_mblks++;
				break;
			}
		}
		break;

	default:
		gbuf_freem(m);
		break;
	}

	return 0;
} /* elap_wput */


/* Called directly by ddp/zip.
 */
elap_dataput(m, elapp, addr_flag, addr)
register	gbuf_t	*m;
register elap_specifics_t *elapp;
u_char			addr_flag;
char *addr;
{
	register int		size;
	int			error;
	extern	int		zip_type_packet();
	struct	etalk_addr	dest_addr;
	struct	atalk_addr	dest_at_addr;
	gbuf_t			*tmp;
	extern	gbuf_t		*growmsg();
	int		loop = TRUE;		/* flag to aarp to loopback (default) */

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
	    gbuf_freel(m);		/* unknown address type, chuck it */
	    return(EINVAL);
        }

	while (gbuf_len(m) == 0) {
		tmp = m;
		m = gbuf_cont(m);
		gbuf_freeb(tmp); 
	}

	/* At this point, rptr points to ddp header for sure */

	if (elapp->elap_if.ifState == LAP_OFFLINE) {
	    gbuf_freel(m);
		return(ENETDOWN);
	}

	if (elapp->elap_if.ifState == LAP_ONLINE_FOR_ZIP) {
		/* see if this is a ZIP packet that we need
		 * to let through even though network is
		 * not yet alive!!
		 */
		if (zip_type_packet(m) == 0) {
	    	gbuf_freel(m);
			return(ENETDOWN);
		}
	}
	
	elapp->stats.xmit_packets++;
	size = gbuf_msgsize(m);
	elapp->stats.xmit_bytes += size;
	snmpStats.dd_outLong++;
	
	switch (addr_flag) {
	case AT_ADDR_NO_LOOP :
	case AT_ADDR :
	    /*
	     * we don't want elap to be looking into ddp header, so
	     * it doesn't know net#, consequently can't do 
	     * AMT_LOOKUP.  That task left to aarp now.
	     */
	    error = aarp_send_data(m,elapp,&dest_at_addr, loop);
	    break;
	case ET_ADDR :
	    error = pat_output(elapp->pat_id, m, &dest_addr, 0);
	    break;
        }
	return (error);
} /* elap_dataput */


/************************************************************************
 * elap_online()
 *
 ************************************************************************/

int	elap_online (gref, if_name, aerr, m)
gref_t	*gref;
char	*if_name;	/* name of h/w interface (e.g. en2) */
int	*aerr;
gbuf_t *m;

{
	register elap_specifics_t	*elapp;
	int			errno;
	int i,j;

	if (if_name == NULL) {
		elapp = (elap_specifics_t *)gref;
		gref = elapp->wait_q;
		if (aerr != NULL)
			goto AARP_sleep;
		if (elapp->elap_if.ifZipError != ZIP_RE_AARP) {
			errno = at_delay_errno ? at_delay_errno : elapp->elap_if.ifZipError;
			at_delay_errno = 0;
			ioc_ack(errno, m, gref);
			elapp->wait_p = 0; elapp->wait_q = 0;
			dPrintf(D_M_ELAP, D_L_STARTUP_INFO,
				("elap_online: ifZipError=%d\n",
				elapp->elap_if.ifZipError));
		}
		goto ZIP_sleep;
	}

	if ((i = pat_ID(if_name)) == -1)
		return(EINVAL);
	elapp = &elap_specifics[i];
	elapp->wait_p = elap_online; elapp->wait_q = gref; elapp->wait_m = m;
	if (elapp->elap_if.ifState == LAP_ONLINE_FOR_ZIP) {
		dPrintf(D_M_ELAP, D_L_STARTUP_INFO, 
			("elap_online: goto zip_online_only\n"));
		goto zip_online_only;
	}
	if (elapp->elap_if.ifState == LAP_ONLINE_ZONELESS) {
		ATALK_ASSIGN(elapp->cfg.initial_addr, 0, 0, 0);
		dPrintf(D_M_ELAP_LOW, D_L_STARTUP_INFO,
			("elap_online: goto re_aarp port=%d\n", elapp->elap_if.ifPort));
		goto re_aarp; 				/* just reset the net range */
	}

	if (!xpatcnt && !(at_statep->flags & AT_ST_PAT_INIT)) {
		pat_init();
	    at_statep->flags |= AT_ST_PAT_INIT;
	}

	dPrintf(D_M_ELAP, D_L_STARTUP_INFO, ("elap_online:%s elapp:0x%x\n",
		if_name[0] ? if_name : "NULL interface", (u_int) elapp));
	
	elapp->pat_id = pat_online(if_name, &elapp->elap_if.ifType);
	if (elapp->pat_id == -1) {
		/* pat_online returned in error, interface name must be bad */
		elapp->wait_m = 0;
		return(EINVAL);
	}
	if (elapp->elap_if.ifType == IFTYPE_TOKENTALK)
	{
		elapp->cable_multicast_addr = ttalk_multicast_addr;
		ddp_bit_reverse(&elapp->cable_multicast_addr);
	}
	else
	{
		elapp->cable_multicast_addr = etalk_multicast_addr;
		if (elapp->elap_if.ifType == IFTYPE_FDDITALK)
			ddp_bit_reverse(&elapp->cable_multicast_addr);
	}
	bcopy((caddr_t) if_name, (caddr_t) elapp->cfg.if_name, AT_IF_NAME_LEN);
	bcopy((caddr_t) if_name, (caddr_t) elapp->elap_if.ifName, AT_IF_NAME_LEN);
	elapp->elap_if.ifUnit = IF_UNIT(if_name);

	if (elapp->elap_if.ifState != LAP_OFFLINE) {
		/* the network must be up already or hanging 
		 * up! 
		 */
		elapp->wait_m = 0;
		return (elapp->elap_if.ifState == LAP_HANGING_UP?EAGAIN:EALREADY);
	}
	
	bzero ((caddr_t) &elapp->stats, sizeof(at_elap_stats_t));
	elapp->elap_if.ifFlags = AT_IFF_ETHERTALK;
	if (elapp->flags & ELAP_CFG_HOME)	/* tell ddp_add_if if this is home */
		elapp->elap_if.ifFlags |= AT_IFF_DEFAULT;
		
	elapp->elap_if.ifLapp = (void*)elapp;

	/* ELAP/DDP interface is not streams based anymore */

	/* Get DDP started */
	if (errno = ddp_add_if(&elapp->elap_if)) {
		elapp->wait_m = 0;
		return (errno);
	}

		/* set up multicast address for cable-wide broadcasts */
	i = pat_control(elapp->pat_id, PAT_REG_MCAST, &elapp->cable_multicast_addr);
	j = pat_control(elapp->pat_id, PAT_REG_CONTEXT, (caddr_t)elapp);


re_aarp :
	/* We now call aarp_init() to assign an apple
	 * talk node addr 
	 */
	ATALK_ASSIGN(elapp->cfg.node, 0, 0, 0);
AARP_sleep :
	if (aerr != NULL)
		errno = *aerr;
	else
		errno = aarp_init(elapp, 0);
	if (errno != 0) {
		if (errno == ENOTREADY)
			return (errno);
		dPrintf(D_M_ELAP, D_L_STATE_CHG, ("elap_online aarp_init for %s\n",
			elapp->elap_if.ifName));
		pat_control(elapp->pat_id, PAT_UNREG_MCAST, &elapp->cable_multicast_addr);
		pat_offline(elapp->pat_id);
		ddp_rem_if (&elapp->elap_if);
		elapp->elap_if.ifState = LAP_OFFLINE;
		if ((aerr != NULL) || (elapp->elap_if.ifZipError == ZIP_RE_AARP)) {
			ioc_ack(EADDRNOTAVAIL, m, gref);
			elapp->wait_p = 0; elapp->wait_q = 0; elapp->wait_m = 0;
			dPrintf(D_M_ELAP, D_L_STARTUP_INFO, ("elap_online: ack 2\n"));
		}
		return (EADDRNOTAVAIL);
	}
	else 
		dPrintf(D_M_ELAP,D_L_STARTUP_INFO,
			("elap_online: aarp_init returns zero\n"));

	if (ROUTING_MODE)
	{
		dPrintf(D_M_ELAP,D_L_STARTUP_INFO, 
			("elap_online: re_aarp, we know it's a router...\n"));

		if (elapp->flags & ELAP_CFG_SEED) {
					/* add route table entry (zones to be added later) */
			at_if_t *ifID;
			ifID = &elapp->elap_if;
			dPrintf(D_M_ELAP, D_L_STARTUP_INFO,
				("elap_online: rt_insert Cable %d-%d port =%d as SEED\n",
				ifID->ifThisCableStart, ifID->ifThisCableEnd, ifID->ifPort));
			rt_insert(	ifID->ifThisCableEnd,
						ifID->ifThisCableStart,
						0,0,0,
						ifID->ifPort,
						RTE_STATE_PERMANENT | RTE_STATE_ZKNOWN | RTE_STATE_GOOD
					 );
			/* LD 081694: set the RTR_SEED_PORT flag for seed ports */
			ifID->ifFlags |= RTR_SEED_PORT;
		}
		else 
			dPrintf(D_M_ELAP,D_L_STARTUP_INFO,
				("elap_online: it's a router, but non seed\n"));
	}
	if (elapp->flags & ELAP_CFG_ZONELESS) {
		elapp->elap_if.ifState = LAP_ONLINE_ZONELESS;
		if (aerr != NULL) {
			errno = at_delay_errno;
			at_delay_errno = 0;
			ioc_ack(errno, m, gref);
			elapp->wait_p = 0; elapp->wait_q = 0; elapp->wait_m = 0;
			dPrintf(D_M_ELAP, D_L_STARTUP_INFO, ("elap_online: ack 3\n"));
		}
		return(0);		/* if not home, it's zoneless */
	}

zip_online_only:
	if (ifID_table[IFID_HOME]) {
		{
			char str[35];
			at_nvestr_t *nve = &ifID_table[IFID_HOME]->ifZoneName;
			strncpy(str,nve->str,nve->len);
			str[nve->len] = '\0';
		}
	}
	sethzonehash(elapp);

	/* Get ZIP rolling to get zone multicast address, etc. */

	elapp->elap_if.ifState = LAP_ONLINE_FOR_ZIP;
	elapp->cfg.network_up = LAP_ONLINE_FOR_ZIP;
 	elapp->wait_m = m;
	if ((errno = zip_control(&elapp->elap_if, ZIP_ONLINE)) != 0) {
		if (errno == ENOTREADY) 
			return (ENOTREADY);
		
ZIP_sleep:
		switch (elapp->elap_if.ifZipError) {
		case 0 :
			break;
		case ENOTREADY :
			elapp->elap_if.ifZipError = 0;
			return (ENOTREADY);
		case ZIP_RE_AARP :
 			elapp->wait_m = m;
			goto re_aarp;
		case ENODEV :
			/* return ENODEV to /etc/appletalk and let the 
			 * network be ONLINE.... this is to tackle the 
			 * case where zone name resolution is not 
			 * complete.
			 */
			elapp->elap_if.ifState = LAP_ONLINE_FOR_ZIP;
			elapp->cfg.network_up = LAP_ONLINE_FOR_ZIP;
			return (ENODEV);
		default :
			return (elapp->elap_if.ifZipError);
		}
	}

	elapp->elap_if.ifState = LAP_ONLINE;
	elapp->cfg.network_up = LAP_ONLINE;

	return (0);
} /* elap_online */


/****************************************************************************
 * elap_offline()
 *
 ****************************************************************************/

StaticProc
int	elap_offline(elapp)
register elap_specifics_t *elapp;

{
	void	zip_sched_getnetinfo(); /* forward reference */
	int	errno;
	int s;

	if (elapp->elap_if.ifState == LAP_OFFLINE) {
#ifdef APPLETALK_DEBUG
kprintf("elap_offline: network is down already.\n");
#endif
		/* the network must be down already! */
		return (EALREADY);
	      }

	elapp->elap_if.ifState = LAP_HANGING_UP;
	elap_hangup(elapp);

	ATDISABLE(s, ddpinp_lock);
	if (MULTIPORT_MODE)
		RT_DELETE(elapp->elap_if.ifThisCableEnd,
			  elapp->elap_if.ifThisCableStart);
	ATENABLE(s, ddpinp_lock);

	if (errno = ddp_rem_if(&elapp->elap_if)) {
#ifdef APPLETALK_DEBUG
kprintf("elap_offline: ddp_rem_if ret %d\n", errno);
#endif
		return (errno);
	}

	dPrintf(D_M_ELAP, D_L_SHUTDN_INFO, ("elap_offline:%s\n", elapp->cfg.if_name));

	/* make sure no zip timeouts are left running */
	atalk_untimeout (zip_sched_getnetinfo, &elapp->elap_if,
		((at_if_t *)&elapp->elap_if)->tmo_3);

	/* clean up the elapp-> and elapp->cfg structures.
	 * initializing with 0 is okay,(state LAP_OFFLINE == 0).
	 */
	bzero ((caddr_t) elapp, sizeof(elap_specifics_t));
	/* Deal with deferred multicast requests */
	dodefer();

	/* *** all of the static variables (and more globals?) should
	   be reinitialized. *** */
	deferno = 0;
	gotIfs = 0;

	/* reset this as the last operation in the shutdown */
	/* *** appletalk_inited cannot be reinitialized until such time as
	   AppleTalk is no longer permanently linked into the kernel.
	   appletalk_inited = 0;
	   */

	return (0);
} /* elap_offline */

StaticProc
void elap_hangup(elapp)
register elap_specifics_t	*elapp;
{
	/* Since AppleTalk is going away, remove the cable
	 * multicast address  and turn the interface off so that all AppleTalk
	 * packets are dropped in the driver itself.
	 */
	/* Get rid of the zone multicast address prior to going Offline. */
	if (zone_multicast_addr.etalk_addr_octet[0] &&	
	    (elapp->flags & ELAP_CFG_HOME)) {
	    /* if zone is set && if this is the home port */
		pat_control(elapp->pat_id, PAT_UNREG_MCAST, 
			    &zone_multicast_addr);
		bzero((caddr_t) &zone_multicast_addr, AARP_ETHER_ADDR_LEN);
	}
	else if (*(int*)&elapp->ZoneMcastAddr)
		pat_control(elapp->pat_id, PAT_REG_MCAST, &elapp->ZoneMcastAddr);
	pat_control(elapp->pat_id, PAT_UNREG_MCAST, &elapp->cable_multicast_addr);
	pat_offline(elapp->pat_id);

	elapp->elap_if.ifState = LAP_OFFLINE;
	return;
}


int	elap_control (ifID, control, data)
register at_if_t  *ifID;
int	          control;
u_char	          *data;
{
	register elap_specifics_t	*elapp;

	/* extract elapp & verify */
	dPrintf(D_M_ELAP, D_L_INFO, ("elap_control:%d\n", control));
	if (!( elapp = (elap_specifics_t *) ifID->ifLapp)) {
		return(ENXIO);
	}

	switch (control) {
	case ELAP_CABLE_BROADCAST_FOR_ZONE :
		/* the cable-broadcast address is to be used as
		 * zone-multicast address.
		 */
		zone_multicast_addr = elapp->cable_multicast_addr;
		pat_control(elapp->pat_id, PAT_REG_MCAST, &zone_multicast_addr);
		break;
	case ELAP_REG_ZONE_MCAST :
		bcopy((caddr_t) data, (caddr_t) &zone_multicast_addr, AARP_ETHER_ADDR_LEN);
		pat_control(elapp->pat_id, PAT_REG_MCAST, &zone_multicast_addr);
		break;
	case ELAP_UNREG_ZONE_MCAST :
		if (*(int *)&zone_multicast_addr) {
			pat_control(elapp->pat_id, PAT_UNREG_MCAST, 
				&zone_multicast_addr);
			bzero((caddr_t) &zone_multicast_addr, AARP_ETHER_ADDR_LEN);
		}
		break;
	case ELAP_RESET_INITNODE :
		ATALK_ASSIGN(elapp->cfg.initial_addr, 0, 0, 0);
		break;
	case ELAP_DESIRED_ZONE :
		if (elapp->cfg.zonename.len)
			bcopy((caddr_t) &elapp->cfg.zonename, (caddr_t) data, 
				elapp->cfg.zonename.len+1);
		break;
	default :
		dPrintf(D_M_ELAP, D_L_ERROR, ("elap_control:invalid control:%d\n",
			control));
		break;
	}
	return (0);
}

StaticProc 
void get_ifs_stat(ifcfgp)
if_cfg_t	*ifcfgp;
{
	static if_cfg_t 	ifcfg;
	int 				ifType, ifMax, i;
	char 				xtype, ifTypeNameBuf[4], ifName[5], *ifTypeName;

	bzero(&ifcfg.state[0][0], sizeof(ifcfg.state));
	for (ifType=0; ifType<IF_TYPENO_CNT; ifType++) {
			
			/* first get reference info on this specific type then status */
		ifTypeName = NULL;
	 	if (!strcmp(if_types[ifType], IF_TYPE_1)) {
			ifMax = IF_TYPE_ET_MAX;
			xtype = IFTYPE_ETHERTALK;
		} else if (!strcmp(if_types[ifType], IF_TYPE_3)) {
			ifMax = IF_TYPE_TR_MAX;
			xtype = IFTYPE_TOKENTALK;
		} else if (!strcmp(if_types[ifType], IF_TYPE_4)) {
			ifMax = IF_TYPE_FD_MAX;
			xtype = IFTYPE_FDDITALK;
		} else if (!strcmp(if_types[ifType], IF_TYPE_5)) {
			ifMax = IF_TYPE_NT_MAX;
			xtype = IFTYPE_NULLTALK;
		} else
			continue;

		for (i=0; i<ifMax; i++) {
			if (pat_units[i].xtype == xtype) {
				ifcfg.state[ifType][pat_units[i].xunit] =
					elap_specifics[i].elap_if.ifState;
				if (ifTypeName == NULL) {
					strcpy(ifTypeNameBuf, pat_units[i].xname);
					ifTypeName = ifTypeNameBuf;
					for (; *ifTypeName != '\0'; ifTypeName++) {
						if ( (*ifTypeName >= '0') && (*ifTypeName <= '9') )
							break;
					}
					*ifTypeName = '\0';
					ifTypeName = ifTypeNameBuf;
				}
			}
		}
		
		/* here we query the OS for each possible I/F type in the system
		 * we only do this one time and keep the results in the static struct 
		 */
		if (!gotIfs && ifTypeName) {
			for (i=0; i<ifMax; i++) {
			  sprintf(ifName,"%s%d",ifTypeName,i);
			  if (!strcmp(ifName, "nt0"))
			    	/* set corresponding bit for I/F */
				ifcfg.avail[ifType] |= 1<<i;
			  else if (ifunit(ifName))
			           /* was (pat_ifpresent(ifName) != -1) */
			  	/* set corresponding bit for I/F */
			    	ifcfg.avail[ifType] |= 1<<i;	
			}
		} /* if !gotIfs */
	} /* for ifType */
	ifcfg.ver_minor = AT_VERSION_MINOR;
	ifcfg.ver_major = AT_VERSION_MAJOR;
	ifcfg.ver_date  = AT_VERSION_DATE;
	bcopy(&ifcfg, ifcfgp, sizeof(if_cfg_t));
	gotIfs = 1;
} /* get_ifs_stat */
	

ifName2Port(name)
	char *name;

/* returns DDP port number of given interface name. to get ifID,
   use port number as index in ifID_table[]

   returns -1 if interface has no ifID entry.
*/

{
	int i;

	for (i=0; i< IF_TOTAL_MAX; i++) {
		if (!ifID_table[i])				/* if we reached end of entries */
			return(-1);
		if (!strcmp(IFID2IFNAME(ifID_table[i]), name))
			return(i);					/* got a match */
	}	
	return(-1);							/* no luck, searched all entries */
}

StaticProc int set_zones(ifz)
	if_zone_info_t *ifz;

/* 1. adds zone to table
   2. looks up each route entry from zone I/F bitmap
   3. sets zone bit in each route entry

   returns  0 if successful
		   -1 if error occurred
*/
{
	int type, iftype,i,j;
	short zno;
	RT_entry *rte;

	zno = zt_add_zone(ifz->zone_name.str, ifz->zone_name.len);

	if (zno == ZT_MAXEDOUT) {
		dPrintf(D_M_ELAP, D_L_ERROR, ("set_zones: error: table full\n"));
		return(-1);
	}
	if (!IFID_VALID(ifID_table[IFID_HOME])) {
		dPrintf(D_M_ELAP, D_L_ERROR, ("set_zones: home ifID is invalid\n"));
		return(-1);
	}
	if (ifz->zone_home)
		ifID_table[IFID_HOME]->ifZoneName = ifz->zone_name;

	for (iftype=0; iftype< IF_TYPENO_CNT; iftype++) {
	 	if (!strcmp(if_types[iftype], IF_TYPE_1))
			type = IFTYPE_ETHERTALK;
	 	else if (!strcmp(if_types[iftype], IF_TYPE_3))
			type = IFTYPE_TOKENTALK;
	 	else if (!strcmp(if_types[iftype], IF_TYPE_4))
			type = IFTYPE_FDDITALK;
		else if (!strcmp(if_types[iftype], IF_TYPE_5))
			type = IFTYPE_NULLTALK;
		else
			continue;
		for (i=0; i<IF_ANY_MAX; i++)  {
			if (ifz->zone_ifs[iftype] & 1<<i) {
				for (j=0; j<IF_ANY_MAX; j++)  {
					if ( (elap_specifics[j].elap_if.ifType == type)
							&& (elap_specifics[j].elap_if.ifUnit == i) )
						break;
				}
				if (j < IF_ANY_MAX)  {
					rte = rt_blookup(ELAP_UNIT2NETSTOP(j));
					if (!rte) {
						dPrintf(D_M_ELAP, D_L_ERROR,
							("set_zones: error: can't find route\n"));
					}
					zt_set_zmap(zno, rte->ZoneBitMap); 

							/* if first zone for this I/F, make default */
					if (!elap_specifics[j].elap_if.ifDefZone)
 						 elap_specifics[j].elap_if.ifDefZone = zno;
				}
			}
		}
	}
	return(0);
}

StaticProc void add_route(rt)
RT_entry 	*rt;

/* support ioctl to manually add routes to table. 
   this is really only for testing
*/
{
	rt_insert( 	rt->NetStop, rt->NetStart, rt->NextIRNet, 
			 	rt->NextIRNode, rt->NetDist, rt->NetPort, 
			 	rt->EntryState);
	dPrintf(D_M_ELAP, D_L_STARTUP_INFO, ("adding route: %ud:%ud dist:%ud\n",
		rt->NetStart, rt->NetStop,rt->NetDist));
}

StaticProc void routerShutdown()
{
	dPrintf(D_M_ELAP, D_L_SHUTDN, ("routerShutdown called\n"));

        if (ROUTING_MODE) {

                rtmp_shutdown();

                /* free memory allocated for the rtmp/zip tables only at this point */
		FREE(ZT_table, M_RTABLE);
                FREE(RT_table, M_RTABLE);
        }             
        nbp_shutdown();         /* clear all known NVE */
        at_state.flags = 0;     /* make sure inits are done on restart */
}

routerStart(flag, m, gref)
int flag;
gbuf_t *m;
gref_t *gref;
{
	register short i;
	register at_if_t *ifID;
	extern rtmp_router_start();

	if (flag) {
		if ((m = at_delay_m) != NULL) {
			at_delay_m = NULL;
			at_delay_func = rtmp_router_start;
			at_delay_flag = 0;
	 		ioc_ack(ENOTREADY, m, at_delay_gref); 
		}
		return (0);
	}

	/*
	 * this will cause the ports to glean from the net the relevant
	 * information before forwarding
	 */
	for (i = 0 ; i < IF_TOTAL_MAX; i++) {
			ifID = ifID_table[i];
			if (ifID) {
				dPrintf(D_M_ELAP, D_L_STARTUP_INFO, 
					("routerStart Port %d (%s) set to activating\n",
					ifID->ifPort, ifID->ifName));
				ifID->ifRoutingState = PORT_ACTIVATING;
				ifID->ifFlags |= RTR_XNET_PORT;
			}
			else 
				break;
	}

	/*
	 * The next step is to check the information for each port before
	 * declaring the ports up and forwarding
	 */
	dPrintf(D_M_ELAP, D_L_STARTUP_INFO,
		("router_start: waiting 20 sec before starting up\n"));

	at_delay_m = m; at_delay_gref = gref;
	atalk_timeout(routerStart, 1, 20 * SYS_HZ); 
	return (0);
}

StaticProc
void ZIPContinue(elapp, m)
elap_specifics_t *elapp;
gbuf_t *m;
{
	if (elapp->wait_p == NULL)
		gbuf_freem(m);
	else {
		(*elapp->wait_p)((void *)elapp, NULL, NULL, m);
	}
}

void ZIPwakeup(ifID)
at_if_t *ifID;
{
	int s;
	gbuf_t *m;
	elap_specifics_t *elapp;

	ATDISABLE(s, ddpinp_lock);
	elapp = (elap_specifics_t *)ifID->ifLapp;
	if ( (elapp != NULL) && ((m = elapp->wait_m) != NULL) ) {
		elapp->wait_m = NULL;
		ATENABLE(s, ddpinp_lock);
		ZIPContinue(elapp, m);
	} else
		ATENABLE(s, ddpinp_lock);
}

StaticProc
void AARPContinue(elapp, m)
elap_specifics_t *elapp;
gbuf_t *m;
{
	int aerr;

	if (elapp->wait_p == NULL)
		gbuf_freem(m);
	else {
		aerr = aarp_init(elapp, 1);
		(*elapp->wait_p)((void *)elapp, NULL, &aerr, m);
	}
}

void AARPwakeup(probe_cb)
aarp_amt_t *probe_cb;
{
	int s;
	gbuf_t *m;
	elap_specifics_t *elapp;

	ATDISABLE(s, arpinp_lock);
	elapp = probe_cb->elapp;
	if ( (elapp != NULL) && ((m = elapp->wait_m) != NULL) ) {
		elapp->wait_m = NULL;
		ATENABLE(s, arpinp_lock);
		AARPContinue(elapp, m);
	} else
		ATENABLE(s, arpinp_lock);
}

IF_UNIT(if_name)
char *if_name;          
{       
        for (; *if_name != '\0'; if_name++)
                if ((*if_name >= '0') && (*if_name <= '9'))
                        return strtol(if_name, (char *)NULL, 10);
        return -1;
}               


StaticProc void getIfNames(names)
if_name_t *names;
{
	int i;
	at_if_t *ifID;
	bzero(names,sizeof(if_name_t) * IF_TOTAL_MAX);
	for (i=0, ifID=ifID_table[0]; ifID; ifID = ifID_table[++i], names++)
		if(ifID)
			strncpy((char *)names,ifID->ifName,sizeof(*names));

	return;
}

void ddp_bit_reverse(addr)
	unsigned char *addr;
{
static unsigned char reverse_data[] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
	};

	unsigned char k;

	for (k=0; k < 6; k++)
		addr[k] = reverse_data[addr[k]];
}

int elap_trackMcast(pat_id,func,addr)
	int pat_id;
	int func;
	caddr_t addr;
{
	int i, loc=-1;
	u_char c;
	switch(elap_specifics[pat_id].elap_if.ifType) {
	case IFTYPE_ETHERTALK: 
	case IFTYPE_FDDITALK: 
	case IFTYPE_NULLTALK: 
						/* set addr to point to unique part of addr */
		c = addr[5];
						/* first try to find match */
		for (i=0; i< MAX_MCASTS; i++) 
			if (c == pat_units[pat_id].mcast[i]) {
				loc = i;
				break;
			}
				
		switch (func) {
		case MCAST_TRACK_DELETE:
			if (loc >= 0) 
				pat_units[pat_id].mcast[loc] = 0;

			break;
		case MCAST_TRACK_ADD:
			dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:add loc:%d\n", i));
			if (loc >= 0) {
				dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:add, addr was there\n"));
				return(1);
				break;			/* already there */
			}		
			for (i=0; i< MAX_MCASTS; i++) 
				if ( pat_units[pat_id].mcast[i] == 0) {
					loc = i;
					break;
				}
			dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:add1 loc:%d\n", i));
			if (loc >= 0) {
				pat_units[pat_id].mcast[loc] = c;
				dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:add, adding(%x)\n",
					(*(int*)addr)&0xffffff));
			}
			else {
				/*errno = ENOMEM; */ /*LD 5/7/97 nobody is using that */
				return(-1);
			}
			break;	
		case MCAST_TRACK_CHECK:
			if (loc >= 0) {
				dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:check, addr was there\n"));
				return(0);
			}
			else {
				dPrintf(D_M_PAT_LOW, D_L_USR2, ("mctrack:add, addr was NOT there\n"));
				return(-1);
			}
			
		default:
			/*errno = EINVAL;*/ /*LD 5/7/97 nobody is using that */
			return(-1);
		}

	case IFTYPE_TOKENTALK:
		/* we would use the lowest byte of the addr argument as a value
		   to shift left a 1 to form the mcast mask for TR. We'll do this
		   when the time comes
		 */
	default:
		;
	}
	return(0);
}


static 
getSnmpCfg(snmp)
	snmpCfg_t *snmp;
{
	int i;
	elap_specifics_t 	*elapp;
	snmpIfCfg_t			*ifc;
	at_if_t				*ifID;

	snmp->cfg_ifCnt = 0;
	
	bzero(snmp,sizeof(snmpCfg_t));
	for (i=0, elapp=elap_specifics,ifc=snmp->cfg_ifCfg; 
		 i<IF_TYPE_ET_MAX; i++, elapp++, ifc++) {
			ifID = &elap_specifics[i].elap_if;
		if (ifID->ifState != LAP_OFFLINE) {
			snmp->cfg_ifCnt++;
			strncpy(ifc->ifc_name,elapp->cfg.if_name,IF_NAME_SIZE);
			ifc->ifc_aarpSize = getAarpTableSize(i);
			ifc->ifc_addrSize = getPhysAddrSize(i);
			switch (ifID->ifType) {
				case IFTYPE_ETHERTALK:
					ifc->ifc_type = SNMP_TYPE_ETHER2;
					break;
				case IFTYPE_TOKENTALK:
					ifc->ifc_type = SNMP_TYPE_TOKEN;
					break;
				case IFTYPE_LOCALTALK:
					ifc->ifc_type = SNMP_TYPE_LOCAL;
					break;
				case IFTYPE_FDDITALK:
				case IFTYPE_NULLTALK:
				default:
					ifc->ifc_type = SNMP_TYPE_OTHER;
					break;
			}
			ifc->ifc_start 	= ifID->ifThisCableStart;
			ifc->ifc_end	= ifID->ifThisCableEnd;
			ifc->ifc_ddpAddr= ifID->ifThisNode;
			ifc->ifc_status	= ifID->ifState == LAP_ONLINE ? 1 : 2;
			ifc->ifc_zoneName.len = 0;
			if (ifID->ifZoneName.len != 0) {
				ifc->ifc_zoneName = ifID->ifZoneName;
			}
			else if (ifID->ifDefZone) {
				ifc->ifc_zoneName = ZT_table[ifID->ifDefZone-1].Zone;
			}
			else	/* temp, debug only */
				ifc->ifc_zoneName = ZT_table[0].Zone;
			if (ROUTING_MODE) {
				if (ifID->ifFlags & RTR_SEED_PORT) {
					ifc->ifc_netCfg  = SNMP_CFG_CONFIGURED;
					ifc->ifc_zoneCfg = SNMP_CFG_CONFIGURED;
				}
				else {
					ifc->ifc_netCfg  = SNMP_CFG_GARNERED;
					ifc->ifc_zoneCfg = SNMP_CFG_GARNERED;
				}
			}
			else  { 	/* single-port mode */
				if (ifID->ifRouterState == ROUTER_AROUND) {
					ifc->ifc_netCfg = SNMP_CFG_GARNERED;
				}
				else {
					ifc->ifc_netCfg = SNMP_CFG_GUESSED;
					ifc->ifc_zoneCfg = SNMP_CFG_UNCONFIG;
				}
			}
		}
	} 
	snmp->cfg_flags = at_statep->flags;

		
	return(0);
}	

void
elap_get_addr(pat_id, addr)
	int pat_id;
	unsigned char *addr; /* pointer to buffer where the address is returned */
{
	bcopy(pat_units[pat_id].xaddr, addr, pat_units[pat_id].xaddrlen);
}

int
pat_ID(if_name)
	char	*if_name;
{
	int pat_id;

	for (pat_id=0; pat_id < xpatcnt; pat_id++) {
		if (!strcmp(pat_units[pat_id].xname, if_name))
			return(pat_id);
	}

	return(-1);
}

int
pat_control(pat_id, control, data)
	int pat_id;
	int control;
	void *data;
{
	typedef int (*procptr)();

	switch(control) {
	case PAT_REG_CONTEXT :
		pat_units[pat_id].context = data;
		break;
	case PAT_REG_AARP_UPSTREAM :
		pat_units[pat_id].aarp_func = (procptr)data;
		break;
	case PAT_REG_CHECKADDR :
		pat_units[pat_id].addr_check = (procptr)data;
		break;
	case PAT_REG_MCAST :
		if (elap_trackMcast(pat_id, MCAST_TRACK_ADD,data) == 1)
			return(0);
		return(domcast(pat_id, control, data) == 0? 0 : -1);
	case PAT_UNREG_MCAST :
		elap_trackMcast(pat_id, MCAST_TRACK_DELETE, data);
		return(domcast(pat_id, control, data) == 0? 0 : -1);
	default :
		return(-1);
	}

	return(0);
}

StaticProc void
pat_init()
{
#ifdef CHECK_DDPR_FLAG
	extern int ddprunning_flag;
#endif
	/* zeroing the struct is okay since PAT_OFFLINE is zero */
	bzero(pat_units, sizeof(pat_units));
#ifdef CHECK_DDPR_FLAG
	ddprunning_flag = 0;
#endif
}

/*
 * Hopefully temporary hackery to deal with the need to
 *  register/deregister multicast addresses at interrupt level
 *  (as a result of incoming ZIP info).  ioctl routines can't
 *  be called at interrupt level due to locking constraints (MP
 *  and all that)
 * Unfortunately, I can't find a better way.
 */

static struct multidefer {
	int pat_id;
	int control;
	unsigned char addr[6];
} defer[NDEFERS];

StaticProc int
domcast(pat_id, control, data)
	int pat_id;
	int control;
	unsigned char *data;
{
	if (pat_units[pat_id].xtype == IFTYPE_NULLTALK)
		return 0;
#ifdef _AIX
	if  (getpid() == -1) {
		if (deferno < NDEFERS)  {
			defer[deferno].pat_id = pat_id;
			defer[deferno].control = control;
			bcopy(data, defer[deferno].addr, sizeof(defer[deferno].addr));
			deferno++;
			return 0;
			
		}
		else	
			return EINVAL;
	}
#endif

	dPrintf(D_M_PAT, D_L_STARTUP, ("domcast:%s multicast %08x%04x pat_id:%d\n",
			(control == PAT_REG_MCAST) ? "adding" : "deleting",
			*(unsigned*)data, (*(unsigned *)(data+2))&0x0000ffff, pat_id));

	return pat_mcast(pat_id, control, data);
}

/*
 * Handle any backed-up requests to register/unregister multicasts.
 * Should not be called at interrupt level!
 */
StaticProc void
dodefer()
{
	unsigned char *data;
	while (deferno > 0)
	{	--deferno;
		data = (unsigned char *)defer[deferno].addr;

		dPrintf(D_M_PAT, D_L_STARTUP,
			("dodefer:%s multicast %08x%04x pat_id:%d defno:%d\n",
			(defer[deferno].control == PAT_REG_MCAST) ? "adding" : "deleting",
			*(unsigned*)data,
			(*(unsigned *)(data+2))&0x0000ffff,
			defer[deferno].pat_id,
			deferno));
		
		domcast(defer[deferno].pat_id, defer[deferno].control,
			defer[deferno].addr);
	}
}
