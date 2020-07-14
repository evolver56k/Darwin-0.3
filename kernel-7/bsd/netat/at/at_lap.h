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
 *
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

/* The at_if structure identifies a LAP interface to DDP.  These structs
 * are stored in a doubly-linked list.  When DDP needs to send a packet,
 * it traverses this list for the interface to send the packet on.  
 * For an end-node this will usually be the first and only interface.  
 * If we're a router and the destination network is non-zero, DDP 
 * calls the internet router routine instead of traversing its list.
 *
 * Feb-June 1994: Modified for Router support 
 */
#ifndef __AT_LAP__
#define __AT_LAP__
#include <at/nbp.h>

typedef struct at_if_statstics {
	u_long	fwdBytes;			/* bytes received & forwarded */
	u_long	fwdPkts;			/* pkts  received & forwarded */
	u_long	droppedBytes;		/* bytes received & dropped */
	u_long	droppedPkts;		/* pkts  received & dropped */ 
	u_long 	outBytes;			/* bytes sent */
	u_long	outPkts;			/* pkts  sent */
	u_long	routes;				/* count of routes in rtmptable */
} at_if_statistics_t;

typedef char if_name_t[IF_NAME_LEN];


typedef struct at_if {
	/* The DDP sets these values: */
	struct at_if		*FwdLink;
	struct at_if		*BwdLink;

	u_char				ifState; 	/* State of the interface;
									 * See values below
						 		 	 */
	u_char				ifUnit;

	at_net_al			ifThisCableStart;
	at_net_al			ifThisCableEnd;

	/* Note: these are legacy fields used only in user mode */

	struct	atalk_addr	ifARouter;
	u_char				ifRouterState;
	u_long				ifARouterTimer;

	char				*ddpInputQueue;	/* Input queue for DDP, 
							 			 * used by lap 
						 				 */
	/* The LAP layer sets these values before calling 
	 * at_ddp_add_if(): 
	 */

	void			*tmo_1;
	void			*tmo_2;
	void			*tmo_3;
	void			*tmo_4;
	int					ifFlags;	/* Flags, see AT_IFF_*  */
	struct	atalk_addr	ifThisNode;	/* AppleTalk node ID */

	/* for use by ZIP */

	u_char				ifNumRetries;
	at_nvestr_t			ifZoneName;
	int					ifZipError;

	/* Added for routing support */

	void				*ifLapp;	/* parent lapp struct. Cast to 
					   				   correct type ( elap_specifics_t etc.) */
	int					ifPort;		/* the unique ddp logical port number */
	if_name_t			ifName; /* added to support LAP_IOC_GET_IFID */
	char				ifType;

	u_short				ifDefZone;			/*  Default Zone index in ZoneTable*/
	char				ifZipNeedQueries;	/* ZIP/RTMP Query flag */
	char				ifRoutingState;	/* Port (as a router) state */
	at_if_statistics_t	ifStatistics;		/* statistics */
} at_if_t;


typedef union  at_if_name {
at_if_t		ifID;
if_name_t	if_name;
}at_if_name_t;


/* for ifType above */
#define	IFTYPE_ETHERTALK	1
#define	IFTYPE_TOKENTALK	2
#define	IFTYPE_FDDITALK	3
#define	IFTYPE_NULLTALK	4
#define	IFTYPE_LOCALTALK	5
#define	IFTYPE_OTHERTALK	9

/* for ifRouterState field above */
#define	NO_ROUTER		1	/* there's no router around	*/
#define	ROUTER_WARNING	2	/* there's a router around that */
							/* we are ignoring, warning has */
							/* been issued to the user	*/
#define	ROUTER_AROUND	3	/* A router is around and we've */
							/* noted its presence		*/

#define ROUTER_UPDATED  4	/* for mh tracking of routers. Value decremented
                               with rtmp aging timer, a value of 4 allows a 
						       minimum of 40 secs to laps before we decide
  							   to revert to cable multicasts */
              
/* for ifState above */
#define	LAP_OFFLINE			0	/* LAP_OFFLINE MUST be 0 */	
#define	LAP_ONLINE			1
#define	LAP_ONLINE_FOR_ZIP	2
#define	LAP_HANGING_UP		3
#define	LAP_ONLINE_ZONELESS	4	/* for non-home router ports */

	/* addr lengths */
#define ETHERNET_ADDR_LEN	6

	/* macros */
#define IF_NO(c)    (atoi(&c[2]))       /* return i/f number from h/w name
						   				   (e.g. 'et2' returns 2) */

							/* returns elap_specfics_t ptr from ifID pointer */
#define IFID2ELAP(ifid)	 	((elap_specifics_t *)((ifid)->ifLapp))
							/* returns ptr to if_name from elap_specifics_t ptr */
#define ELAPP2IFNAME(elapp) (elapp->cfg.if_name)
							/* returns ptr to if_name from ifID ptr */
#define IFID2IFNAME(ifid)	(ELAPP2IFNAME(IFID2ELAP(ifid)))

							/* returns ddp port # from elap unit # */
#define ELAP_UNIT2PORT(n)	elap_specifics[(n)].elap_if.ifPort

							/* returns cable range end from
							   elap unit # */
#define ELAP_UNIT2NETSTOP(n)	\
						elap_specifics[(n)].elap_if.ifThisCableEnd

							/* sanity check for ifID validity. Evaluates
							   to TRUE if ifID is OK  */
#define IFID_VALID(ifID) \
	(&((elap_specifics_t *)ifID->ifLapp)->elap_if == ifID)

#endif /* __AT_LAP__ */

