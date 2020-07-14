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
 *	Copyright (c) 1993-1998 Apple Computer, Inc.
 *	All Rights Reserved.
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
 *	The copyright notice above does not evidence any actual or
 *	intended publication of such source code.
 */

#include <sysglue.h>

#include <at/appletalk.h>
#include <at/ddp.h>
#include <rtmp.h>
#include <lap.h>

#include <at/at_lap.h>
#include <at_ddp.h>
#include <at_zip.h>
#include <routing_tables.h>
#include <atlog.h>

extern void *atalk_timeout();
extern void atalk_untimeout();
extern void rtmp_router_input();

/****************************************************************/
/*								*/
/*								*/
/*			RTMP Protocol				*/
/*								*/
/*								*/
/****************************************************************/


/* rtmp.c: , 1.6; 2/26/93; Apple Computer, Inc." */


#define	NROUTERS2TRAK	8
#define	FIFTYSECS	10
#define NODE(r)		((r)->ifARouter.atalk_node)
#define NET(r)		((r)->ifARouter.atalk_net)
#define	INUSE(r)	(NODE(r))

void ddp_age_router();



static struct routerinfo {
	struct atalk_addr ifARouter;
	at_if_t		  *ifID;
	void		  *tmo;
} trackedrouters[NROUTERS2TRAK];

void trackrouter_rem_if(ifID)
register at_if_t *ifID;
{
	register i;
	register struct routerinfo *router;

	for (i = NROUTERS2TRAK; --i >= 0;) {
		router = &trackedrouters[i];
		if (trackedrouters[i].ifID == ifID) {
			atalk_untimeout(ddp_age_router, (caddr_t) router, router->tmo);
			break;
		}
	}
}


void routershutdown()
{
	register i;

	for (i = NROUTERS2TRAK; --i >= 0;) {
		register struct routerinfo *router;

		router = &trackedrouters[i];
		if (INUSE(router)) {
			atalk_untimeout(ddp_age_router, (caddr_t) router, router->tmo);
			bzero((caddr_t) router, sizeof(struct routerinfo));
		}
	}
}

int router_added  = 0;
int router_killed = 0;



void trackrouter(ifID, net, node)
register at_if_t *ifID;
register unsigned short	net;
register unsigned char	node;
{
	register struct routerinfo *unused = NULL;
	register i;

	for (i = NROUTERS2TRAK; --i >= 0;) {
		register struct routerinfo *router;

		router = &trackedrouters[(i + node) & (NROUTERS2TRAK-1)];
		if ((NODE(router) == node) && (NET_VALUE(NET(router)) == net)) {
			atalk_untimeout(ddp_age_router, (caddr_t) router, router->tmo);
			router->tmo = atalk_timeout(ddp_age_router, (caddr_t) router, 50*SYS_HZ);
			unused = NULL;
			break;
		}
		else if (!INUSE(router) && !unused)
			unused = router;
	}
	if (unused) {
	        router_added++;

		unused->ifID = ifID;
		NET_ASSIGN(NET(unused),  net);
		NODE(unused) = node;
		unused->tmo = atalk_timeout(ddp_age_router, (caddr_t) unused, 50*SYS_HZ);
		if (NET_EQUAL0(NET(ifID)) && NODE(ifID) == 0) {
			NET_ASSIGN(NET(ifID), net);
			NODE(ifID) = node;
			ifID->ifRouterState = ROUTER_AROUND;
		}
	}
}



void ddp_age_router(deadrouter)
register struct routerinfo *deadrouter;
{
	register at_if_t *ourrouter = deadrouter->ifID;

	dPrintf(D_M_RTMP, D_L_INFO, 
		("ddp_age_router called deadrouter=%d:%d\n", NODE(deadrouter), NET(deadrouter)));

	router_killed++;

	if (NODE(ourrouter) == NODE(deadrouter) && 
			NET_EQUAL(NET(ourrouter), NET(deadrouter))) {
		register unsigned long	atrandom = random();
		register struct routerinfo *newrouter;
		register i;

		bzero((caddr_t) deadrouter, sizeof(struct routerinfo));
		for (i = NROUTERS2TRAK; --i >= 0;) {
			newrouter = &trackedrouters[(i + atrandom) & (NROUTERS2TRAK-1)];
			if (INUSE(newrouter))
				break;
			else
				newrouter = NULL;
		}
		if (newrouter) {
			NET_NET(NET(ourrouter), NET(newrouter));
			NODE(ourrouter) = NODE(newrouter);
		}
		else {
			void gorouterless();
			gorouterless(ourrouter);
		}
	} else
	        bzero((caddr_t) deadrouter, sizeof(struct routerinfo));
}



/*
 * This is the timeout function that is called after 50 seconds, 
 * if no router packets come in. That way we won't send extended 
 * frames to something that is not there. Untimeout is called if 
 * an RTMP packet comes in so this routine will not be called.
 */

static void gorouterless(int_ifID)
{
	register at_if_t   *ifID = (at_if_t *)int_ifID;

	ATTRACE(AT_MID_DDP, AT_SID_TIMERS, AT_LV_WARNING, FALSE,
		"ddp_age_router entry : ARouter = 0x%x, RouterState = 0x%x",
		ATALK_VALUE(ifID->ifARouter), ifID->ifRouterState, 0);

		switch (ifID->ifRouterState) {
		case ROUTER_AROUND :
			ATALK_ASSIGN(ifID->ifARouter, 0, 0, 0);
			dPrintf(D_M_RTMP,D_L_INFO,
				("rtmp.c Gorouterless!!!!!!!!\n"));
			ifID->ifThisCableStart = 2;
			ifID->ifThisCableEnd = 0xfffd;
			ifID->ifRouterState = NO_ROUTER;
			zip_control (ifID, ZIP_NO_ROUTER);
			break;
		case ROUTER_WARNING :
			/* there was a router that we were ignoring...
			 * now, even that's gone.  But we want to tackle the
			 * case where another router may come up after all
			 * of them have died...
			 */
			ifID->ifRouterState = NO_ROUTER;
			break;
		}
}



void
rtmp_input (mp, ifID)
register gbuf_t	*mp;
register at_if_t	*ifID;
{
	register at_net_al	this_net;
	register at_net_al	range_start, range_end;
	register at_ddp_t	*ddp = (at_ddp_t *)gbuf_rptr(mp);
				/* NOTE: there is an assumption here that the 
				 * DATA follows the header. */
	register at_rtmp	*rtmp = (at_rtmp *)ddp->data;

	if (gbuf_type(mp) != MSG_DATA) {
		/* If this is a M_ERROR message, DDP is shutting down, 
		 * nothing to do here...If it's something else, we don't 
		 * understand what it is
		 */
		gbuf_freem(mp);
		return;
	}

	if (!ifID) {
		gbuf_freem(mp);
		return;
	}
	if (gbuf_len(mp) < (DDP_X_HDR_SIZE + sizeof(at_rtmp))) {
		gbuf_freem(mp);
		return;
	}
	this_net = NET_VALUE(ifID->ifThisNode.atalk_net);
	if (rtmp->at_rtmp_id_length  != 8) {
		gbuf_freem(mp);
		return;
	}

	{
		at_rtmp_tuple *tp;
		tp = ((at_rtmp_tuple *)&rtmp->at_rtmp_id[1]);
		range_start = NET_VALUE(tp->at_rtmp_net);
		tp = ((at_rtmp_tuple *)&rtmp->at_rtmp_id[4]);
		range_end = NET_VALUE(tp->at_rtmp_net);

		if (ifID->ifRouterState == ROUTER_AROUND) {
			if ((ifID->ifThisCableStart == range_start) &&
			    (ifID->ifThisCableEnd == range_end)) {
				trackrouter(
					ifID,
					NET_VALUE(rtmp->at_rtmp_this_net),
					rtmp->at_rtmp_id[0]
				);
			}
		} else {
			/* There was no router around earlier, one
			 * probably just came up.
			 */
			if ((this_net >= DDP_STARTUP_LOW) && 
			    (this_net <= DDP_STARTUP_HIGH)) {
				/* we're operating in the startup range,
				 * ignore the presence of router
				 */
				if (ifID->ifRouterState == NO_ROUTER) {
					dPrintf(D_M_RTMP, D_L_STARTUP,
						("Warning: new router came up: invalid startup net/node\n"));
					trackrouter(
						ifID,
						NET_VALUE(rtmp->at_rtmp_this_net),
						rtmp->at_rtmp_id[0]
					);
					ifID->ifRouterState = ROUTER_WARNING;
				}
			} else {
				/* our address
				 * is not in startup range; Is our
				 * address good for the cable??
				 */
				if ((this_net >= range_start) &&
				    (this_net <= range_end)) {
					/* Our address is in the range
					 * valid for this cable... Note
					 * the router address and then
					 * get ZIP rolling to get the
					 * zone info.
					 */
					ifID->ifThisCableStart = range_start;
					ifID->ifThisCableEnd = range_end;
					trackrouter(
						ifID,
						NET_VALUE(rtmp->at_rtmp_this_net),
						rtmp->at_rtmp_id[0]
					);
					zip_control(ifID, ZIP_LATE_ROUTER);
				} else {
					/* Our address is not in the
					 * range valid for this cable..
					 * ignore presence of the 
					 * router
					 */
					if (ifID->ifRouterState == NO_ROUTER) {
						dPrintf(D_M_RTMP,D_L_ERROR, 
							("Warning: new router came up: invalid net/node\n"));
						trackrouter(
							ifID,
							NET_VALUE(rtmp->at_rtmp_this_net),
							rtmp->at_rtmp_id[0]
						);
						ifID->ifRouterState = 
							ROUTER_WARNING;
					}
				}
			}
		}
	}

	gbuf_freem(mp);
	return;
}


int	rtmp_init()
{
	bzero((caddr_t) trackedrouters, sizeof(struct routerinfo)*NROUTERS2TRAK);

	if (!ROUTING_MODE) 
		return((int)rtmp_input);

	else 
		return ((int)rtmp_router_input);
}


