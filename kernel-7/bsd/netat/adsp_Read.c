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
 * CheckReadQueue
 *
 * Checks to see if there is any data in the receive queue.  If there
 * is data, a pb and the data are queued to the user.
 * 
 * 	
 */
extern int adsp_check;

int CheckReadQueue(sp)		/* (CCBPtr sp) */
    register CCBPtr sp;
{
    register struct adspcmd *pb;
    int s;
    unsigned short cnt;
    char eom;
    register gbuf_t *mp;
    register gbuf_t *tmp;
	gref_t *gref;
	
    ATDISABLE(s, sp->lock);
    while (sp->rData && (pb = sp->rpb)) {		/* have data */
	if (pb->u.ioParams.reqCount == 0) {
	    pb->ioResult = 0;
	    sp->rpb = pb->qLink;
	    if (pb->ioc) {
		adspioc_ack(0, pb->ioc, pb->gref);
	    } else {
		completepb(sp, pb);
	    }
	    continue;
	}
	    
	if (mp = sp->rbuf_mb) {	/* Get header for oldest data */
	    sp->rbuf_mb = gbuf_next(mp);
	    gbuf_next(mp) = 0;
	    eom = 1;
	} else if (mp = sp->crbuf_mb) {
	    sp->crbuf_mb = 0;
	    eom = 0;
	}
	cnt = gbuf_msgsize(mp);	/* # of data bytes in it. */
	if (cnt > (unsigned short) (pb->u.ioParams.reqCount - pb->u.ioParams.actCount)) {
	    cnt = pb->u.ioParams.reqCount - pb->u.ioParams.actCount;
	    pb->u.ioParams.actCount += cnt;
	    while (cnt >= (unsigned short) gbuf_len(mp)) {
		cnt -= gbuf_len(mp);
		tmp = mp;
		mp = gbuf_cont(mp);
		gbuf_cont(tmp) = 0;
		gbuf_linkb(pb->mp,tmp);
	    }
	    if (cnt) {
		tmp = gbuf_dupb(mp);
		if (tmp == NULL) {
		    pb->u.ioParams.actCount -= cnt; /* can't deliver it now */
	        } else {
			gbuf_wset(tmp,cnt);
			gbuf_rinc(mp,cnt);
			gbuf_linkb(pb->mp,tmp);
		}
	    }
	    if (eom) {
		gbuf_next(mp) = sp->rbuf_mb;
		sp->rbuf_mb = mp;
		eom = 0;
	    } else
		sp->crbuf_mb = mp;
	} else {
	    pb->u.ioParams.actCount += cnt;
	    gbuf_linkb(pb->mp,mp);
	}
	pb->u.ioParams.eom = eom;
	    
	/*
	 * Now clean up receive buffer to remove all of the data 
	 * we just copied
	 */
	if ((sp->rbuf_mb == 0) && 
	    (sp->crbuf_mb == 0)) /* no more data blocks */
	    sp->rData = 0;
	
	/*
	 * If we've filled the parameter block, unlink it from read 
	 * queue and complete it. We also need to do this if the connection
	 * is closed && there is no more stuff to read.
	 */
	if (eom || (pb->u.ioParams.actCount >= pb->u.ioParams.reqCount) ||
	    ((sp->state == sClosed) && (!sp->rData))) {
	    			/* end of message, message is full, connection
				 * is closed and all data has been delivered,
				 * or we are not to "delay" data delivery.
				 */
	    pb->ioResult = 0;
	    sp->rpb = pb->qLink; /* dequeue request */
	    if (pb->ioc) {	/* data to be delivered at the time of the */
		mp = gbuf_cont(pb->mp); /* ioctl call */
		gbuf_cont(pb->mp) = 0;
		gref = (gref_t *)pb->gref;
		adspioc_ack(0, pb->ioc, pb->gref);
		SndMsgUp(gref, mp);
	    } else		/* complete an queued async request */
		completepb(sp, pb);
	} 
    }				/* while */

    if (pb = sp->rpb) {		/* if there is an outstanding request */
	if (sp->state == sClosed) {
	    while (pb) {
		    pb->ioResult = 0;
		    pb->u.ioParams.actCount = 0;
		    pb->u.ioParams.eom = 0;
		    sp->rpb = pb->qLink;
		    if (pb->ioc) {
			    adspioc_ack(0, pb->ioc, pb->gref);
		    } else {
			    completepb(sp, pb);
		    }
		    pb = sp->rpb;
	    }
	} else if (pb->ioc) {	/* if request not complete and this
				 * is an active ioctl, release user */
	    sp->rpb = pb->qLink;
	    pb->ioResult = 1;
	    tmp = gbuf_cont(pb->mp); /* detatch perhaps delayed data */
	    gbuf_cont(pb->mp) = 0;
	    if (mp = gbuf_copym(pb->mp)) { /* otherwise, duplicate user request */
		    adspioc_ack(0, pb->ioc, pb->gref); 	/* release user */
		    pb = (struct adspcmd *)gbuf_rptr(mp); 	/* get new parameter block */
		    pb->ioc = 0;
		    pb->mp = mp;
		    gbuf_cont(pb->mp) = tmp; /* reattach data */
		    pb->qLink = sp->rpb; /* requeue the duplicate at the head */
		    sp->rpb = pb;
	    } else {		/* there is no data left, but no space
				 * to duplicate the parameter block, so
				 * put what must be a non EOM message 
				 * back on the current receive queue, and
				 * error out the user
				 */
		    if (tmp) {
			    sp->crbuf_mb = tmp;
			    sp->rData = 1;
		    }
		    pb->ioResult = errDSPQueueSize;
		    adspioc_ack(ENOBUFS, pb->ioc, pb->gref);
	    }
	} 
    }
    /* 
     * The receive window has opened.  If was previously closed, then we
     * need to notify the other guy that we now have room to receive more
     * data.  But, in order to cut down on lots of small data packets,
     * we'll wait until the recieve buffer  is /14 empy before telling
     * him that there's room in our receive buffer.
     */
    if (sp->rbufFull && (CalcRecvWdw(sp) > (sp->rbuflen >> 2))) {
	sp->rbufFull = 0;
	sp->sendDataAck = 1;
	sp->callSend = 1;
    }

    ATENABLE(s, sp->lock);
    return 0;
}

/*
 * CheckAttn
 *
 * Checks to see if there is any attention data and passes the data back
 * in the passed in pb.
 * 
 * INPUTS:
 *	sp
 *	pb
 * 	
 * OUTPUTS:
 * 	
 */
int CheckAttn(sp, pb)		/* (CCBPtr sp) */
    register CCBPtr sp;
    register struct adspcmd *pb;
{
    int s;
    gbuf_t *mp;
	gref_t *gref;
	
    ATDISABLE(s, sp->lock);
    if (mp = sp->attn_mb) {

	/*
	 * Deliver the attention data to the user. 
	 */
	gref = (gref_t *)pb->gref;
	pb->u.attnParams.attnSize = sp->attnSize;
	pb->u.attnParams.attnCode = sp->attnCode;
	if (!sp->attnSize) {
	    gbuf_freem(mp);
	    mp = 0;
	}
	sp->userFlags &= ~eAttention;
	/*
	 * Now clean up receive buffer to remove all of the data 
	 * we just copied
	 */
	sp->attn_mb = 0;
	pb->ioResult = 0;
    } else {
	/*
	 * No data...
	 */
	pb->u.attnParams.attnSize = 0;
	pb->u.attnParams.attnCode = 0;
	pb->ioResult = 1;	/* not done */
    }
    adspioc_ack(0, pb->ioc, pb->gref);
    if (mp) {
	SndMsgUp(gref, mp);
	}
    ATENABLE(s, sp->lock);
    return 0;
}

/*
 * adspRead
 * 
 * INPUTS:
 *	--> sp			stream pointer
 *	--> pb			user request parameter block
 *
 * OUTPUTS:
 *	<-- actCount		actual number of bytes read
 *	<-- eom			one if end-of-message, zero otherwise
 *
 * ERRORS:
 *	errRefNum		bad connection refnum
 *	errState
 *	errFwdReset		read terminated by forward reset
 *	errAborted		request aborted by Remove or Close call
 */
int adspRead(sp, pb)		/* (DSPPBPtr pb) */
    register CCBPtr sp;
    register struct adspcmd *pb;
{
    register gbuf_t *mp;
    int	s;

    if (sp == 0) {
	pb->ioResult = errRefNum;
	return EINVAL;
    }
	
    /*
     * It's OK to read on a closed, or closing session
     */
    ATDISABLE(s, sp->lock);
    if (sp->state != sOpen && sp->state != sClosing && sp->state != sClosed) {
	ATENABLE(s, sp->lock);
	pb->ioResult = errState;
	return EINVAL;
    }

    if (sp->rData && (sp->rpb == 0)) { /* if data, and no queue of pbs */
	qAddToEnd(&sp->rpb, pb); /* deliver data to user directly */
	ATENABLE(s, sp->lock);
	CheckReadQueue(sp);
    } else if ((pb->u.ioParams.reqCount == 0) && (sp->rpb == 0)) {
	    /* empty read */
	    ATENABLE(s, sp->lock);
	    pb->ioResult = 0;
	    adspioc_ack(0, pb->ioc, pb->gref);
	    return 0;
    } else {
	pb->ioResult = 1;
	if (mp = gbuf_copym(pb->mp)) { /* otherwise, duplicate user request */
		adspioc_ack(0, pb->ioc, pb->gref); 	/* release user */
		pb = (struct adspcmd *)gbuf_rptr(mp); 	/* get new parameter block */
		pb->ioc = 0;
		pb->mp = mp;
		qAddToEnd(&sp->rpb, pb); /* and queue it for later */
		ATENABLE(s, sp->lock);
	} else {
		ATENABLE(s, sp->lock);
		pb->ioResult = errDSPQueueSize;
		return ENOBUFS;
	}
    }
    if (sp->callSend) {
	CheckSend(sp);		/* If recv window opened, we might */
				/* send an unsolicited ACK. */
    }
    return 0;
}

/*
 * dspReadAttention
 * 
 * INPUTS:
 *	--> sp			stream pointer
 *	--> pb			user request parameter block
 *
 * OUTPUTS:
 *	<-- NONE
 *
 * ERRORS:
 *	errRefNum		bad connection refnum
 *	errState		connection is not in the right state
 */
int adspReadAttention(sp, pb)		/* (DSPPBPtr pb) */
    register CCBPtr sp;
    register struct adspcmd *pb;
{
    OSErr err;

    if (sp == 0) {
	pb->ioResult = errRefNum;
	return EINVAL;
    }
	
    /*
     * It's OK to read on a closed, or closing session
     */
    if (sp->state != sOpen && sp->state != sClosing && sp->state != sClosed) {
	pb->ioResult = errState;
	return EINVAL;
    }

    CheckAttn(sp, pb);		/* Anything in the attention queue */
    CheckReadQueue(sp);		/* check to see if receive window has opened */
    if (sp->callSend) {
	CheckSend(sp);		/* If recv window opened, we might */
				/* send an unsolicited ACK. */
	}
    return 0;
}
