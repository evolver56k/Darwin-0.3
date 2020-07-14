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


#include	<adsp_local.h>


/*
 * dspAttention
 * 
 * INPUTS:
 * 	--> ccbRefNum		refnum of connection end
 *	--> attnCode		client attention code
 *	--> attnSize		size in bytes of attention data
 *	--> attnData		pointer to attention data
 *	--> attnInterval	attention retransmit interval 
 *				(ignored by ADSP 1.5 & up)
 *
 * OUTPUTS:
 *	none
 *
 * ERRORS:
 *	errRefNum		bad connection refnum
 *	errState		connection is not open
 *	errAttention		attention message too long
 *	errAborted		request aborted by Remove or Close call
 */
int adspAttention(sp, pb)	/* (DSPPBPtr pb) */
    register struct adspcmd *pb;
    register CCBPtr sp;
{
    int	s;
    OSErr err;
    register gbuf_t *mp, *nmp;
    unsigned char uerr;
	
    if (sp == 0) {
	pb->ioResult = errRefNum;
	return EINVAL;
    }
	
    if (sp->state != sOpen) {	/* If we're not open, tell user to go away */
	pb->ioResult = errState;
	uerr = ENOTCONN;
l_err:
	gbuf_set_type(pb->mp, MSG_ERROR);
	*gbuf_rptr(pb->mp) = uerr;
	gbuf_wset(pb->mp,1);
	completepb(sp, pb);
	return 0;
    }
	
    if (pb->u.attnParams.attnSize > attnBufSize) /* If data too big, bye-bye */
    {
	pb->ioResult = errAttention;
	uerr = ERANGE;
	goto l_err;
    }

    /*
     * Copy in attention data.
     */
    mp = pb->mp;
    if (pb->u.attnParams.attnSize) {

  nmp = gbuf_cont(mp);
  if (gbuf_len(mp) > sizeof(struct adspcmd)) {
	if ((nmp = gbuf_dupb(mp)) == 0) {
		gbuf_wset(mp,sizeof(struct adspcmd));
	    uerr = ENOSR;
	    goto l_err;
	}
	gbuf_wset(mp,sizeof(struct adspcmd));
	gbuf_rinc(nmp,sizeof(struct adspcmd));
	gbuf_cont(nmp) = gbuf_cont(mp);
	gbuf_cont(mp) = nmp;
  }
    }
    pb->ioDirection = 1;	/* outgoing attention data */
    ATDISABLE(s, sp->lock);
    if (sp->sapb) {		/* Pending attentions already? */
	qAddToEnd(&sp->sapb, pb); /* Just add to end of queue */
	ATENABLE(s, sp->lock);
    } else {
	sp->sendAttnData = 1;	/* Start off this attention */
	pb->qLink = 0;
	sp->sapb = pb;
	ATENABLE(s, sp->lock);
	CheckSend(sp);
    }
    pb->ioResult = 1;	/* indicate that the IO is not complete */
    return 0;
}
