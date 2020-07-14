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
 *	File: tickle.c
 */
#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <routing_tables.h>
#define _AURP
#include <at_aurp.h>
#include <at/aurp.h>

/* */
void AURPsndTickle(state)
	aurp_state_t *state;
{
	int msize;
	gbuf_t *m;
	aurp_hdr_t *hdrp;

	if (state->rcv_state == AURPSTATE_Unconnected)
		return;

	/* stop trying if the retry count exceeds the maximum retry value */
	if (++state->tickle_retry > AURP_MaxTickleRetry) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPsndTickle: no response, %d\n", state->rem_node));
		/*
		 * the tunnel peer seems to have disappeared, update state info
		 */
		state->snd_state = AURPSTATE_Unconnected;
		state->rcv_state = AURPSTATE_Unconnected;
		state->tickle_retry = 0;
		AURPcleanup(state);

		/* purge all routes associated with the tunnel peer */
		AURPpurgeri(state->rem_node);
		return;
	}

  if (state->tickle_retry > 1) {
	msize = sizeof(aurp_hdr_t);
	if ((m = (gbuf_t *)gbuf_alloc(msize, PRI_MED)) != 0) {
		gbuf_wset(m,msize);

		/* construct the tickle packet */
		hdrp = (aurp_hdr_t *)gbuf_rptr(m);
		hdrp->connection_id = state->rcv_connection_id;
		hdrp->sequence_number = 0;
		hdrp->command_code = AURPCMD_Tickle;
		hdrp->flags = 0;

		/* send the packet */
		AURPsend(m, AUD_AURP, state->rem_node);
	}
  }

	/* start the retry timer */
	atalk_timeout(AURPsndTickle, state, AURP_TickleRetryInterval*HZ);
}

/* */
void AURPrcvTickle(state, m)
	aurp_state_t *state;
	gbuf_t *m;
{
	aurp_hdr_t *hdrp = (aurp_hdr_t *)gbuf_rptr(m);

	/* make sure we're in a valid state to accept it */
	if (state->snd_state == AURPSTATE_Unconnected) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvTickle: unexpected request\n"));
		gbuf_freem(m);
		return;
	}

	/* construct the tickle ack packet */
	gbuf_wset(m,sizeof(aurp_hdr_t));
	hdrp->command_code = AURPCMD_TickleAck;
	hdrp->flags = 0;

	/* send the packet */
	AURPsend(m, AUD_AURP, state->rem_node);
}

/* */
void AURPrcvTickleAck(state, m)
	aurp_state_t *state;
	gbuf_t *m;
{
	aurp_hdr_t *hdrp = (aurp_hdr_t *)gbuf_rptr(m);

	/* make sure we're in a valid state to accept it */
	if (state->rcv_state == AURPSTATE_Unconnected) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvTickleAck: unexpected response\n"));
		gbuf_freem(m);
		return;
	}

	/* check for the correct connection id */
	if (hdrp->connection_id != state->rcv_connection_id) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvTickleAck: invalid connection id, r=%d, m=%d\n",
			hdrp->connection_id, state->rcv_connection_id));
		gbuf_freem(m);
		return;
	}
	gbuf_freem(m);

	/* update state info */
	state->tickle_retry = 0;
}
