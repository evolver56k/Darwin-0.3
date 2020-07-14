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
 *	Copyright (c) 1995-1998 Apple Computer, Inc.
 *	All Rights Reserved.
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF APPLE COMPUTER, INC.
 *	The copyright notice above does not evidence any actual or
 *	intended publication of such source code.
 */

#include <h/adsp_local.h>
#include <h/adsp_ioctl.h>
#include <at/ddp.h>
#include <h/at_ddp.h>
#include <h/atlog.h>

void SndMsgUp();
void adsp_rput();
static void adsp_stop();
static void adsp_iocack();
static void adsp_iocnak();
static void adspgetcfg();
void adsp_dequeue_ccb();
unsigned char adspAssignSocket();
int adspallocate(), adsprelease();
static int adspInited = 0;

at_ddp_cfg_t ddpcfg;
atlock_t adspall_lock;
atlock_t adspgen_lock;
GLOBAL adspGlobal;

/**********/

int adsp_pidM[256];
char adsp_inputC[256];
CCB *adsp_inputQ[256];
extern char ddp_off_flag;
extern int ot_protoCnt;
extern char ot_protoT[];
extern char ot_adsp_socketM[];

static char adsp_off_flag;
static CCB *ccb_used_list;

void adsp_input(mp)
	gbuf_t *mp;
{
	gref_t *gref;
	CCBPtr sp;
	at_ddp_t *p;
	int s, l;
	gbuf_t *mb;

	switch (gbuf_type(mp)) {
	case MSG_DATA:
		p = (at_ddp_t *)gbuf_rptr(mp);
		ATDISABLE(s, adspall_lock);
		sp = adsp_inputQ[p->dst_socket];
		if ((sp == 0) || adsp_off_flag || (sp->gref==0) || (sp->state==sClosed))
		{
			ATENABLE(s, adspall_lock);
			gbuf_freem(mp);
			return;
		}
		else if (sp->otccbLink != 0) {
			do {
				if ((sp->remoteAddress.a.node == p->src_node)
					&& (sp->remoteAddress.a.socket == p->src_socket)
				&& (*(short *)sp->remoteAddress.a.net == *(short *)p->src_net))
					break;
			} while ((sp = sp->otccbLink) != 0);
			if (sp == 0)
			{
				ATENABLE(s, adspall_lock);
				gbuf_freem(mp);
				return;
			}
		}
		if (sp->lockFlag) {
			gbuf_next(mp) = 0;
			if (sp->deferred_mb) {
				for (mb=sp->deferred_mb; gbuf_next(mb); mb=gbuf_next(mb)) ; 
				gbuf_next(mb) = mp;
			} else
				sp->deferred_mb = mp;
			ATENABLE(s, adspall_lock);
			return;
		}
		ATDISABLE(l, sp->lockRemove);
		sp->lockFlag = 1;
		ATENABLE(l, adspall_lock);
		while (mp) {
			adsp_rput(sp->gref, mp);
			if ((mp = sp->deferred_mb) != 0) {
				sp->deferred_mb = gbuf_next(mp);
				gbuf_next(mp) = 0;
			}
		}
		sp->lockFlag = 0;
		ATENABLE(s, sp->lockRemove);
		return;

	case MSG_IOCACK:
	case MSG_IOCNAK:
		gref = (gref_t *)((ioc_t *)gbuf_rptr(mp))->ioc_private;
		break;

	case MSG_IOCTL:
		adsp_stop(mp, *gbuf_rptr(mp));
		gbuf_set_type(mp, MSG_IOCACK);
		DDP_OUTPUT(mp);
		return;

	default:
		gbuf_freem(mp);
		return;
	}

	adsp_rput(gref, mp);
}

/**********/
int adsp_readable(gref)
	gref_t *gref;
{
	int rc;
	CCBPtr sp;

	sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	rc = sp->rData;

	return rc;
}

int adsp_writeable(gref)
	gref_t *gref;
{
	int s, rc;
	CCBPtr sp;

	sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	ATDISABLE(s, sp->lock);
	rc = CalcSendQFree(sp);
	ATENABLE(s, sp->lock);

	return rc;
}

void adsp_init()
{
	adspInited++;
	InitGlobals();
	ccb_used_list = 0;
	adsp_off_flag = 0;
	bzero(adsp_pidM, sizeof(adsp_pidM));
	bzero(adsp_inputC, sizeof(adsp_inputC));
	bzero(adsp_inputQ, sizeof(adsp_inputQ));
}

/*
 * Description:
 *	ADSP open and close routines.  These routines
 *	initalize and release the ADSP structures.  They do not
 *	have anything to do with "connections"
 */

int adsp_open(gref)
	gref_t *gref;
{
    register CCBPtr sp;
    int s;
    
    if (!adspInited)
		adsp_init();
	/*
	 * if not ready, return error
	 */
	if (adsp_off_flag)
		return ENOTREADY;

    if (!adspAllocateCCB(gref))
	return(ENOBUFS);	/* can't get buffers */
 
	sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	gref->readable = adsp_readable;
	gref->writeable = adsp_writeable;
	ATDISABLE(s, adspall_lock);
	if ((sp->otccbLink = ccb_used_list) != 0)
		sp->otccbLink->ccbLink = sp;
	ccb_used_list = sp;
	ATENABLE(s, adspall_lock);
	return 0;
}

int adsp_close(gref)
	gref_t *gref;
{
  int s, l;
  unsigned char localSocket;

  /* make sure we've not yet removed the CCB (e.g., due to TrashSession) */
  ATDISABLE(l, adspgen_lock);
  if (gref->info) {
	CCBPtr sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	ATDISABLE(s, sp->lock);
	ATENABLE(s, adspgen_lock);
	if (sp->ioDone)
	{
  		ATENABLE(l, sp->lock);
		return 0;
	}
	localSocket = sp->localSocket;
	ATENABLE(l, sp->lock);
	if (localSocket)
		adspRelease(gref);
	else
	{
		adsp_dequeue_ccb(sp);
		gbuf_freeb((gbuf_t *)gref->info);
	}
  } else
	ATENABLE(l, adspgen_lock);
    return 0;
}


/*
 * Name:
 * 	adsp_rput
 *
 * Description:
 *	ADSP streams read put and service routines.
 */

void adsp_rput(gref, mp)
    gref_t *gref;			/* READ queue */
    gbuf_t *mp;
{
  static first = 1;

  switch (gbuf_type(mp)) {
  case MSG_ERROR:
  case MSG_HANGUP:
  case MSG_IOCACK:
  case MSG_IOCNAK:

	/*
	 * ADSP needs to obtain the network number and node number 
	 * for the Host it is running on.
	 * In the AUX implementation it "peeked" inside the ddp 
	 * structure pointed to by at_ifDefault;
	 * In a strict streams module implementation "peeking" is 
	 * neither kosher, nor permitted.
	 * adspgetcfg() sends a DDP_IOC_GET_CFG ioctl request to DDP.
	 * This code picks up the response from DDP. From this
	 * response, the ADSP streams module obtains the network 
	 * and node number.
	 */

	/*
	** Examine the first packet coming from DDP stream.
	*/
	if (first) {
		first = 0;
		if (gbuf_type(mp) == MSG_IOCACK) {
			ioc_t *iocbp = (ioc_t *) gbuf_rptr(mp);

			if (iocbp->ioc_cmd == DDP_IOC_GET_CFG) {
				if (gbuf_rptr(gbuf_cont(mp))) {
					bcopy((caddr_t) gbuf_rptr(gbuf_cont(mp)), 
						(caddr_t) &ddpcfg, 
						sizeof(at_ddp_cfg_t));
					dPrintf(D_M_ADSP, D_L_INFO, 
						("ADSP: Net = 0x%x, Node = 0x%x\n",
						NET_VALUE(ddpcfg.node_addr.net),
						ddpcfg.node_addr.node));
				}
				gbuf_freem(mp);
				return;
			}
		}
	}
	switch (adspReadHandler(gref, mp)) {
	case STR_PUTNEXT:	
	    atalk_putnext(gref, mp); 
	    break;
	case STR_IGNORE:
	    break;
        }
	break;
  default:
	CheckReadQueue(gbuf_rptr(((gbuf_t *)gref->info)));
	CheckSend(gbuf_rptr(((gbuf_t *)gref->info)));

    	switch (gbuf_type(mp)) {
	case MSG_IOCTL:
	case MSG_DATA:
	case MSG_PROTO:
	    if (adspReadHandler(gref, mp) == STR_PUTNEXT)
		atalk_putnext(gref, mp);
	    break;
	default:
	    atalk_putnext(gref, mp);
	    break;
	}
  }
}

/*
 * Name:
 * 	adsp_wput
 *
 * Description:
 *	ADSP streams write put and service routines.
 *
 */

int adsp_wput(gref, mp)
    gref_t *gref;			/* WRITE queue */
    gbuf_t *mp;
{
	int rc;
	int s;
	gbuf_t *xm;
	ioc_t *iocbp;

	if (gbuf_type(mp) == MSG_IOCTL) {
		iocbp = (ioc_t *)gbuf_rptr(mp);
		switch (iocbp->ioc_cmd) {
		case AT_ADSP_LINK:
			adsp_dequeue_ccb((CCBPtr)gbuf_rptr(((gbuf_t *)gref->info)));
			ot_protoT[DDP_ADSP] = 1;
			ot_protoCnt++;

			iocbp->ioc_rval = 0;
			adsp_iocack(gref, mp);
			return 0;

		case AT_ADSP_UNLINK:
			ot_protoT[DDP_ADSP] = 0;
			ot_protoCnt--;
			ddp_off_flag = 0; /* why do this on an unlink? */

			if (adspInited) {
			  CleanupGlobals(); /* for 2225395 */
				adspInited = 0;
			}
			iocbp->ioc_rval = 0;
			adsp_iocack(gref, mp);
			return 0;

		case ADSPBINDREQ: 
			{
			unsigned char v;
			CCBPtr sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));

			if (gbuf_cont(mp) == NULL) {
				iocbp->ioc_rval = -1;
				adsp_iocnak(gref, mp, EINVAL);
			}
			v = *(unsigned char *)gbuf_rptr(gbuf_cont(mp));
			ATDISABLE(s, adspall_lock);
			if ( (v != 0)
				&& ((v > DDP_SOCKET_LAST) || (v < 2)
					|| (adsp_inputQ[v] != 0) || (ot_adsp_socketM[v] != 0)) ) {
				ATENABLE(s, adspall_lock);
				iocbp->ioc_rval = -1;
				adsp_iocnak(gref, mp, EINVAL);
			}
			else {
				if (v == 0) {
					ATENABLE(s, adspall_lock);
					if ((v = adspAssignSocket(gref, 0)) == 0) {
						iocbp->ioc_rval = -1;
						adsp_iocnak(gref, mp, EINVAL);
						return 0;
					}
				} else {
					adsp_inputC[v] = 1;
					adsp_inputQ[v] = sp;
					adsp_pidM[v] = sp->pid;
					ATENABLE(s, adspall_lock);
					adsp_dequeue_ccb(sp);
				}
				*(unsigned char *)gbuf_rptr(gbuf_cont(mp)) = v;
				sp->localSocket = v;
				iocbp->ioc_rval = 0;
				adsp_iocack(gref, mp);
			}
			return 0;
			}

		case ADSPGETSOCK:
		case ADSPGETPEER:
			{
			at_inet_t *addr;
			CCBPtr sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));

			if (((xm = gbuf_cont(mp)) == NULL)
				&& ((xm = gbuf_alloc(sizeof(at_inet_t), PRI_MED)) == NULL)) {
				iocbp->ioc_rval = -1;
				adsp_iocnak(gref, mp, ENOSR);
				return 0;
			}
			gbuf_cont(mp) = xm;
			gbuf_wset(xm,sizeof(at_inet_t));
			addr = (at_inet_t *)gbuf_rptr(xm);
			if (iocbp->ioc_cmd == ADSPGETSOCK) {
					adspgetcfg(gref);
				*addr = ddpcfg.node_addr;
				addr->socket = sp->localSocket;
			} else
				*addr = sp->remoteAddress.a;
			iocbp->ioc_rval = 0;
			adsp_iocack(gref, mp);
			return 0;
			}
		} /* switch */
	}

	/* Obtain Network and Node Id's from DDP */
    	adspgetcfg(gref);

	if (!gref->info)
	    gbuf_freem(mp);
	else {
	    CCBPtr sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	    ATDISABLE(s, sp->lockClose);
	    rc = adspWriteHandler(gref, mp);
	    ATENABLE(s, sp->lockClose);

	    switch (rc) {
	    case STR_PUTNEXT:
		if (gbuf_type(mp) == MSG_IOCTL) {
		    iocbp = (ioc_t *)gbuf_rptr(mp);
		    if (iocbp->ioc_cmd == DDP_IOC_GET_CFG) {
		        if ((xm = gbuf_alloc(sizeof(at_ddp_cfg_t), PRI_MED)) == NULL) {
			    adsp_iocnak(gref, mp, ENOBUFS);
			    break;
			}
			if (gbuf_cont(mp) != NULL)
			    gbuf_freem(gbuf_cont(mp));
			gbuf_cont(mp) = xm;
			gbuf_wset(xm,sizeof(at_ddp_cfg_t));
			*(at_ddp_cfg_t *)gbuf_rptr(xm) = ddpcfg;
			((at_ddp_cfg_t *)gbuf_rptr(xm))->node_addr.socket =
			    ((CCBPtr)gbuf_rptr(((gbuf_t *)gref->info)))->localSocket;
			adsp_iocack(gref, mp);
			break;
		    }
		    iocbp->ioc_private = (void *)gref;
		}
		DDP_OUTPUT(mp);
		break;
	    case STR_IGNORE:
	    case STR_IGNORE+99:
		break;
	    default:
		gbuf_freem(mp);
		break;
	    }
	}

	return 0;
} /* adsp_wput */

void adspioc_ack(errno, m, gref)
    int errno;
    gbuf_t *m;
    gref_t *gref;
{
    ioc_t *iocbp;

    if (m == NULL)
	return;
    iocbp = (ioc_t *) gbuf_rptr(m);

    iocbp->ioc_error = errno;	/* set the errno */
    iocbp->ioc_count = gbuf_msgsize(gbuf_cont(m));
    if (gbuf_type(m) == MSG_IOCTL)	/* if an ioctl, this is an ack */
	gbuf_set_type(m, MSG_IOCACK);	/* and ALWAYS update the user */
    					/* ioctl structure */
	SndMsgUp(gref, m);
}

static void adspgetcfg(gref)
     gref_t *gref;
{
	static adspgetcfgdone = 0;
	register gbuf_t *mp;
	ioc_t *iocbp;

	if (adspgetcfgdone)
		return;

	if (gref && (mp = gbuf_alloc(sizeof(ioc_t), PRI_HI))) {
		adspgetcfgdone = 1;
		gbuf_set_type(mp, MSG_IOCTL);
		gbuf_wset(mp,sizeof(ioc_t));
		iocbp = (ioc_t *) gbuf_rptr(mp);
		iocbp->ioc_private = (void *)gref;
		iocbp->ioc_count = ((CCBPtr)gbuf_rptr(((gbuf_t *)gref->info)))->localSocket;
		iocbp->ioc_error = 7;

		iocbp->ioc_cmd = DDP_IOC_GET_CFG;
		DDP_OUTPUT(mp);
	}
}

static void adsp_iocack(gref, m)
     gref_t *gref;
     register gbuf_t *m;
{
	if (gbuf_type(m) == MSG_IOCTL)
		gbuf_set_type(m, MSG_IOCACK);

	if (gbuf_cont(m))
		((ioc_t *)gbuf_rptr(m))->ioc_count = gbuf_msgsize(gbuf_cont(m));
	else
		((ioc_t *)gbuf_rptr(m))->ioc_count = 0;

	SndMsgUp(gref, m);
}


static void adsp_iocnak(gref, m, err)
     gref_t *gref;
     register gbuf_t *m;
     register int err;
{
	if (gbuf_type(m) == MSG_IOCTL)
		gbuf_set_type(m, MSG_IOCNAK);
	((ioc_t *)gbuf_rptr(m))->ioc_count = 0;

	if (err == 0)
		err = ENXIO;
	((ioc_t *)gbuf_rptr(m))->ioc_error = err;

	if (gbuf_cont(m)) {
		gbuf_freem(gbuf_cont(m));
		gbuf_cont(m) = NULL;
	}
	SndMsgUp(gref, m);
}

unsigned char
adspAssignSocket(gref, flag)
	gref_t *gref;
	int flag;
{
	unsigned char sVal, sMax, sMin, sSav, inputC;
	CCBPtr sp;
	int s;

	sMax = flag ? DDP_SOCKET_LAST-46 : DDP_SOCKET_LAST-6;
	sMin = DDP_SOCKET_1st_DYNAMIC-64;

	ATDISABLE(s, adspall_lock);
	for (inputC=255, sVal=sMax; sVal >= sMin; sVal--) {
		if (!ot_adsp_socketM[sVal]) {
			if (!adsp_inputQ[sVal])
				break;
			else if (flag) {
				if ((adsp_inputC[sVal] < inputC)
						&& (adsp_inputQ[sVal]->state == sOpen)) {
					inputC = adsp_inputC[sVal];
					sSav = sVal;
				}
			}
		}
	}

	if (sVal < sMin) {
		if (!flag || (inputC == 255)) {
			ATENABLE(s, adspall_lock);
			return 0;
		}
		sVal = sSav;
	}
	sp = (CCBPtr)gbuf_rptr(((gbuf_t *)gref->info));
	ATENABLE(s, adspall_lock);
	adsp_dequeue_ccb(sp);
	ATDISABLE(s, adspall_lock);
	adsp_inputC[sVal]++;
	sp->otccbLink = adsp_inputQ[sVal];
	adsp_inputQ[sVal] = sp;
	if (!flag)
		adsp_pidM[sVal] = sp->pid;
	ATENABLE(s, adspall_lock);
	return sVal;
}

int
adspDeassignSocket(sp)
	CCBPtr sp;
{
	unsigned char sVal;
	CCBPtr curr_sp;
	CCBPtr prev_sp;
	int pid = 0;
	int s, l;

	dPrintf(D_M_ADSP, D_L_TRACE, ("adspDeassignSocket: pid=%d,s=%d\n",
		sp->pid, sp->localSocket));
	ATDISABLE(s, adspall_lock);
	sVal = sp->localSocket;
	if ((curr_sp = adsp_inputQ[sVal]) != 0) {
		prev_sp = 0;
		while (curr_sp != sp) {
			prev_sp = curr_sp;
			curr_sp = curr_sp->otccbLink;
		}
		if (curr_sp) {
			ATDISABLE(l, sp->lockRemove);
			if (prev_sp)
				prev_sp->otccbLink = sp->otccbLink;
			else
				adsp_inputQ[sVal] = sp->otccbLink;
			ATENABLE(l, sp->lockRemove);
			if (adsp_inputQ[sVal])
				adsp_inputC[sVal]--;
			else {
				pid = adsp_pidM[sVal];
				adsp_inputC[sVal] = 0;
				adsp_pidM[sVal] = 0;
			}
			sp->ccbLink = 0;
			sp->otccbLink = 0;
			sp->localSocket = 0;
			ATENABLE(s, adspall_lock);
		    return pid ? 0 : 1;
		}
	}
	ATENABLE(s, adspall_lock);

	dPrintf(D_M_ADSP, D_L_ERROR, 
		("adspDeassignSocket: closing, no CCB block, trouble ahead\n"));
	return -1;
}

/*
 * stop ADSP now
 */
static void
adsp_stop(m, flag)
	gbuf_t *m;
	int flag;
{
	int k, s, *x_wptr;
	CCB *sp;
	
	ATDISABLE(s, adspall_lock);
	gbuf_rptr(m)[1]++;

	if (flag)
		adsp_off_flag = 1;

	x_wptr = (int *)gbuf_wptr(m);
	for (sp = ccb_used_list; sp; sp = sp->otccbLink) {
	  if (flag == 2) {
		sp->ioDone = 1;
	  } else {
		*(int *)gbuf_wptr(m) = sp->pid;
		gbuf_winc(m,sizeof(int));
	  }
	}

	for (k=0; k < 256; k++) {
		if ((sp = adsp_inputQ[k]) == 0)
			continue;
		do {
		  if (flag == 2) {
			sp->ioDone = 1;
		  } else {
			*(int *)gbuf_wptr(m) = sp->pid;
			gbuf_winc(m,sizeof(int));
		  }
		} while ((sp = sp->otccbLink) != 0);
	}
	ATENABLE(s, adspall_lock);

	while (x_wptr != (int *)gbuf_wptr(m)) {
		dPrintf(D_M_ADSP, D_L_TRACE, ("adsp_stop: pid=%d\n", *x_wptr));
		x_wptr++;
	}
}

/*
 * remove CCB from the use list
 */
void
adsp_dequeue_ccb(sp)
	CCB *sp;
{
	int s;

	ATDISABLE(s, adspall_lock);
	if (sp == ccb_used_list) {
		if ((ccb_used_list = sp->otccbLink) != 0)
			sp->otccbLink->ccbLink = 0;
	} else if (sp->ccbLink) {
		if ((sp->ccbLink->otccbLink = sp->otccbLink) != 0)
			sp->otccbLink->ccbLink = sp->ccbLink;
	}

	sp->otccbLink = 0;
	sp->ccbLink = 0;
	ATENABLE(s, adspall_lock);
}

void SndMsgUp(gref, mp)
    gref_t *gref;			/* WRITE queue */
	gbuf_t *mp;
{
    atalk_putnext(gref, mp);
}
