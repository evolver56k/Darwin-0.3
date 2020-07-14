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
 *	Copyright (c) 1996 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 *	File: misc.c
 */
#include <sys/socketvar.h>
#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <rtmp.h>
#include <routing_tables.h>
#include <at/aurp.h>
#define _AURP
#define _KERNSYS
#include <at_aurp.h>

/* */
void AURPiocack(gref, m)
	gref_t *gref;
	gbuf_t *m;
{
	/* send ok reply to ioctl command */
	gbuf_set_type(m, MSG_IOCACK);
	atalk_putnext(gref, m);
}

void AURPiocnak(gref, m, error)
	gref_t *gref;
	gbuf_t *m;
	int error;
{
	ioc_t *iocbp = (ioc_t *)gbuf_rptr(m);

	/* send error reply to ioctl command */
	if (gbuf_cont(m)) {
        gbuf_freem(gbuf_cont(m));
        gbuf_cont(m) = 0;
	}
	iocbp->ioc_error = error;
	iocbp->ioc_count = 0;
	iocbp->ioc_rval = -1;
	gbuf_set_type(m, MSG_IOCNAK);
	atalk_putnext(gref, m);
}

/* */
void AURPupdate(arg)
	void *arg;
{
	unsigned char node;
	aurp_state_t *state = (aurp_state_t *)&aurp_state[1];

	if (aurp_gref == 0)
		return;

	/*
	 * for every tunnel peer, do the following periodically:
	 * 1. send zone requests to determine zone names of networks
	 *    that still do not have asssociated zone names.
	 * 2. send any RI update that are pending
	 */
	for (node=1; node <= dst_addr_cnt; node++, state++) {
		AURPsndZReq(state);
		AURPsndRIUpd(state);
	}

	/* restart the periodic update timer */
	update_tmo = (void *)atalk_timeout(AURPupdate, arg, AURP_UpdateRate*10*HZ);
}

/* */
void AURPfreemsg(m)
	gbuf_t *m;
{
	gbuf_t *tmp_m;

	while ((tmp_m = m) != 0) {
		m = gbuf_next(m);
		gbuf_next(tmp_m) = 0;
		gbuf_freem(tmp_m);
	}
}

/* */
int AURPinit()
{
	unsigned char node;
	aurp_state_t *state = (aurp_state_t *)&aurp_state[1];
	short entry_num;
	RT_entry *entry = (RT_entry *)RT_table;

	/* start the periodic update timer */
	update_tmo = (void *)atalk_timeout(AURPupdate, 0, AURP_UpdateRate*10*HZ);

	/* initialize AURP flags for entries in the RT table */
	for (entry_num=0; entry_num < RT_maxentry; entry_num++,entry++)
		entry->AURPFlag = 0;

	/* initiate connections to peers */
	for (node=1; node <= dst_addr_cnt; node++, state++) {
		bzero((char *)state, sizeof(*state));
		state->rem_node = node;
		state->snd_state = AURPSTATE_Unconnected;
		state->rcv_state = AURPSTATE_Unconnected;
		AURPsndOpenReq(state);
	}

	return 0;
}

/* */
void AURPcleanup(state)
	aurp_state_t *state;
{
	if (state->rsp_m) {
		gbuf_freem(state->rsp_m);
		state->rsp_m = 0;
	}

	if (state->upd_m) {
		gbuf_freem(state->upd_m);
		state->upd_m = 0;
	}
}

/*
 *
 */
void AURPshutdown()
{
	unsigned char node;
	aurp_state_t *state = (aurp_state_t *)&aurp_state[1];

	/* cancel the periodic update timer */
	atalk_untimeout(AURPupdate, 0, update_tmo);
	update_tmo = 0;

	/* notify tunnel peers of router going-down */
	for (node=1; node <= dst_addr_cnt; node++, state++) {
		AURPcleanup(state);
		AURPsndRDReq(state);
	}

	/* bring down the router */ 
	aurp_wakeup(AE_SHUTDOWN, 0);
}

void AURPaccess()
{
	unsigned char i;
	short entry_num;
	RT_entry *entry;

		entry = (RT_entry *)RT_table;
		for (entry_num=0; entry_num < RT_maxentry; entry_num++,entry++)
			entry->AURPFlag = net_export ? AURP_NetHiden : 0;

	for (i=0; i < net_access_cnt; i++) {
		/* export or hide networks as configured */
		if ((entry = RT_lookup(net_access[i])) != 0)
			entry->AURPFlag = net_export ? 0 : AURP_NetHiden;
	}
}
