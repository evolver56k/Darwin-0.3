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
 *	File: open.c
 */
#include <sysglue.h>
#include <at/appletalk.h>
#include <lap.h>
#include <routing_tables.h>
#define _AURP
#include <at_aurp.h>
#include <at/aurp.h>

/* */
void AURPsndOpenReq(state)
	aurp_state_t *state;
{
	int msize;
	gbuf_t *m;
	aurp_hdr_t *hdrp;
	char *datp;

	if (aurp_gref == 0)
		return;
	if (state->rcv_retry && (state->rcv_state != AURPSTATE_WaitingForOpenRsp))
		return;

	/* stop trying if the retry count exceeds the maximum value */
	if (++state->rcv_retry > AURP_MaxRetry) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPsndOpenReq: no response, %d\n", state->rem_node));
		state->rcv_state = AURPSTATE_Unconnected;
		state->rcv_tmo = 0;
		state->rcv_retry = 0;
		return;
	}

	msize = sizeof(aurp_hdr_t) + 3;
	if ((m = (gbuf_t *)gbuf_alloc(msize, PRI_MED)) != 0) {
		gbuf_wset(m,msize);

		/* construct the open request packet */
		hdrp = (aurp_hdr_t *)gbuf_rptr(m);
		if (state->rcv_retry > 1)
			hdrp->connection_id = state->rcv_connection_id;
		else {
			if (++rcv_connection_id == 0)
				rcv_connection_id = 1;
			hdrp->connection_id = rcv_connection_id;
		}
		hdrp->sequence_number = 0;
		hdrp->command_code = AURPCMD_OpenReq;
		hdrp->flags = (AURPFLG_NA | AURPFLG_ND | AURPFLG_NDC | AURPFLG_ZC);
		*(short *)(hdrp+1) = AURP_Version;
		((char *)(hdrp+1))[2] = 0; /* option count */

		/* update state info */
		state->rcv_connection_id = hdrp->connection_id;
		state->rcv_state = AURPSTATE_WaitingForOpenRsp;

		/* send the packet */
		AURPsend(m, AUD_AURP, state->rem_node);
	}

	/* start the retry timer */
	state->rcv_tmo = (void *)atalk_timeout(
		AURPsndOpenReq, state, AURP_RetryInterval*HZ);
}

/* */
void AURPrcvOpenReq(state, m)
	aurp_state_t *state;
	gbuf_t *m;
{
	short rc, version;
	aurp_hdr_t *hdrp = (aurp_hdr_t *)gbuf_rptr(m);
	unsigned short sui = hdrp->flags;

	/* make sure we're in a valid state to accept it */
	if ((update_tmo == 0) || ((state->snd_state != AURPSTATE_Unconnected) &&
			(state->snd_state != AURPSTATE_Connected))) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvOpenReq: unexpected request\n"));
		gbuf_freem(m);
		return;
	}

	/* check for the correct version number */
	version = *(short *)(hdrp+1);
	if (version != AURP_Version) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvOpenReq: invalid version number %d\n"));
		rc = AURPERR_InvalidVersionNumber;
	} else
		rc = (short)AURP_UpdateRate;

	/* construct the open response packet */
	gbuf_wset(m,sizeof(aurp_hdr_t)+sizeof(short));
	hdrp->command_code = AURPCMD_OpenRsp;
	hdrp->flags = 0;
	*(short *)(hdrp+1) = rc;
	((char *)(hdrp+1))[2] = 0; /* option count */

	/*
	 * reset if we're in the Connected state and this is
	 * a completely new open request
	 */
	if ((state->snd_state == AURPSTATE_Connected) &&
		((state->snd_connection_id != hdrp->connection_id) ||
			(state->snd_sequence_number != AURP_FirstSeqNum))) {
		extern void AURPsndTickle();
		if (state->rcv_state == AURPSTATE_Connected) {
			state->rcv_state = AURPSTATE_Unconnected;
			atalk_untimeout(AURPsndTickle, state, 0);
		}
		state->snd_state = AURPSTATE_Unconnected;
		AURPcleanup(state);
		AURPpurgeri(state->rem_node);
	}

	/* update state info */
	if (state->snd_state == AURPSTATE_Unconnected) {
		state->snd_state = AURPSTATE_Connected;
		state->snd_sui = sui;
		state->snd_connection_id = hdrp->connection_id;
		state->snd_sequence_number = AURP_FirstSeqNum;
	}

	/* send the packet */
	AURPsend(m, AUD_AURP, state->rem_node);

	/* open connection for the data receiver side if not yet connected */
	if (state->rcv_state == AURPSTATE_Unconnected) {
		state->rcv_retry = 0;
		state->tickle_retry = 0;
		state->rcv_sequence_number = 0;
		AURPsndOpenReq(state);
	}
}

/* */
void AURPrcvOpenRsp(state, m)
	aurp_state_t *state;
	gbuf_t *m;
{
	extern void AURPsndTickle();
	short rc;
	aurp_hdr_t *hdrp = (aurp_hdr_t *)gbuf_rptr(m);

	/* make sure we're in a valid state to accept it */
	if (state->rcv_state != AURPSTATE_WaitingForOpenRsp) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvOpenRsp: unexpected response\n"));
		gbuf_freem(m);
		return;
	}

	/* check for the correct connection id */
	if (hdrp->connection_id != state->rcv_connection_id) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvOpenRsp: invalid connection id, r=%d, m=%d\n",
			hdrp->connection_id, state->rcv_connection_id));
		gbuf_freem(m);
		return;
	}

	/* cancel the retry timer */
	atalk_untimeout(AURPsndOpenReq, state, state->rcv_tmo);
	state->rcv_tmo = 0;
	state->rcv_retry = 0;

	/* update state info */
	state->rcv_sequence_number = AURP_FirstSeqNum;
	state->rcv_env = hdrp->flags;

	/* check for error */
	rc = *(short *)(hdrp+1);
	gbuf_freem(m);
	if (rc < 0) {
		dPrintf(D_M_AURP, D_L_WARNING,
			("AURPrcvOpenRsp: error=%d\n", rc));
		return;
	}

	/* update state info */
	state->rcv_update_rate = (unsigned short)rc;
	state->rcv_state = AURPSTATE_Connected;

	/* start tickle */
	atalk_timeout(AURPsndTickle, state, AURP_TickleRetryInterval*HZ);

	/* get routing info */
	AURPsndRIReq(state);
}
