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


/*
 * Title:	ddp_brt.c
 *
 * Facility:	Best Router Caching.
 *
 * Author:	Kumar Vora, Creation Date: June-15-1989
 *
 */

#include <sysglue.h>
#include <at/appletalk.h>
#include <at/ddp.h>
#include <at/elap.h>
#include <at/at_lap.h>
#include <at_ddp.h>
#include <at_ddp_brt.h>
#include <atlog.h>


extern void *atalk_timeout();
extern void atalk_untimeout();
static void *sweep_tmo;

/* Best Router Cache */
ddp_brt_t	at_ddp_brt[BRTSIZE];
static	u_long	ddp_brt_sweep_timer;

void
ddp_glean(mp, ifID, src_addr)
register gbuf_t	  *mp;
register at_if_t  *ifID;
struct etalk_addr  *src_addr;
{
	register at_net_al	     src_net;

	/* NOT assuming that the incoming packet is in one contiguous
	 * buffer.
	 */

	{
		/* The interface is ethertalk, so the message is
		 * of the form {802.3, 802.2, ddp.... }. Extract the
		 * 802.3 source address if necessary.  Assuming, 
		 * however, that 802.3 and 802.2 headers are in
		 * one contiguous piece.
		 */
		{       register at_ddp_t    *dgp;

			dgp = (at_ddp_t *)(gbuf_rptr(mp));
			src_net = NET_VALUE(dgp->src_net);
		}
		if (src_net >= ifID->ifThisCableStart && src_net <= ifID->ifThisCableEnd) 
			/* the packet has come from a net on this cable,
			 * no need to glean router info.
			 */
			return;

		if (src_addr != NULL)
		{	register ddp_brt_t   *brt;

			BRT_LOOK (brt, src_net);
			if (brt == NULL) {
			        /* There's no BRT entry corresponding to this 
				 * net. Allocate a new entry.
				 */
			        NEW_BRT(brt, src_net);
				if (brt == NULL)
				        /* No space available in the BRT; 
					 * can't glean info.
					 */
				        return;
				brt->net = src_net;
		        }
			/*
			 * update the router info in either case
			 */
			brt->et_addr = *src_addr;
			brt->age_flag = BRT_VALID;
			brt->ifID = ifID;
		}
	}
}


static void	ddp_brt_sweep();

void
ddp_brt_init()
{
	bzero(at_ddp_brt, sizeof(at_ddp_brt));
	ddp_brt_sweep_timer = 1;
	sweep_tmo = atalk_timeout(ddp_brt_sweep, (long)0, BRT_SWEEP_INT * SYS_HZ);
}

void
ddp_brt_shutdown()
{
	bzero(at_ddp_brt, sizeof(at_ddp_brt));
	if (ddp_brt_sweep_timer)
		atalk_untimeout(ddp_brt_sweep, 0, sweep_tmo);
	ddp_brt_sweep_timer = 0;
}

StaticProc void
ddp_brt_sweep ()
{
        register ddp_brt_t      *brt;
	register int		i;

	brt = at_ddp_brt;
	for (i = 0; i < BRTSIZE; i++, brt++) {
		switch (brt->age_flag) {
		case BRT_EMPTY :
			break;
		case BRT_VALID :
			brt->age_flag = BRT_GETTING_OLD;
			break;
		case BRT_GETTING_OLD :
			bzero(brt, sizeof(ddp_brt_t));
			break;
		default :
			ATTRACE(AT_MID_DDP,AT_SID_RESOURCE, AT_LV_ERROR, FALSE,
				"ddp_brt_sweep : corrupt age flag %d", 
				brt->age_flag, 0,0);
			break;
		}
	}
	
	/* set up the next sweep... */
	sweep_tmo = atalk_timeout(ddp_brt_sweep, (long)0, BRT_SWEEP_INT * SYS_HZ);
}




