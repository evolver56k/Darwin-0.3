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
 *	Copyright (c) 1995 Apple Computer, Inc. 
 *
 *	The information contained herein is subject to change without
 *	notice and  should not be  construed as a commitment by Apple
 *	Computer, Inc. Apple Computer, Inc. assumes no responsibility
 *	for any errors that may appear.
 *
 *	Confidential and Proprietary to Apple Computer, Inc.
 *
 */
#ifdef _AIX
#include <sys/sleep.h>
#include <sys/poll.h>
#endif
#include <sys/ioccom.h>
#include <h/sysglue.h>
#include <at/appletalk.h>
#include <at/elap.h>
#include <at/ddp.h>
#include <at/at_lap.h>
#include <at_elap.h>
#include <h/at_ddp.h>
#undef e_sleep_thread

/*definition of dbgBits moved here to avoid multiple redefinition */
dbgBits_t 	dbgBits;

int _ATsocket(proto, err, proc)
	int proto;
	int *err;
	void *proc;
{
	int fd;
	gref_t *gref;

	/* make sure the specified protocol id is valid */
	switch (proto) {
	case ATPROTO_DDP:
	case ATPROTO_LAP:
	case ATPROTO_ATP:
	case ATPROTO_ASP:
	case ATPROTO_AURP:
	case ATPROTO_ADSP:
		break;
	default:
		*err = EPROTOTYPE;
#ifdef APPLETALK_DEBUG
		kprintf("_ATsocket: error EPROTOTYPE =%d\n", *err);
#endif
		return -1;
	}

	/* allocate a protocol channel */
	if ((*err = gref_open(&gref)) != 0) {
#ifdef APPLETALK_DEBUG
		kprintf("_ATsocket: error gref_open =%d\n", *err);
#endif
		return -1;
	}
	gref->proto = proto;
#ifdef NEXT
	gref->pid = ((struct proc *)proc)->p_pid;
#endif

	/* open the specified protocol */
	switch (gref->proto) {
	case ATPROTO_DDP:
		*err = ddp_open(gref); break;
	case ATPROTO_LAP:
		*err = lap_open(gref); break;
	case ATPROTO_ATP:
		*err = atp_open(gref, 1); break;
	case ATPROTO_ASP:
		*err = asp_open(gref); break;
#ifdef AURP_SUPPORT
	case ATPROTO_AURP:
		*err = aurp_open(gref); break;
#endif
	case ATPROTO_ADSP:
		*err = adsp_open(gref); break;
	}

	/* create the descriptor for the channel */
	if (*err)
		gref->proto = ATPROTO_NONE;
	if (*err || (*err = atalk_openref(gref, &fd, proc))) {
#ifdef APPLETALK_DEBUG
		kprintf("_ATsocket: error atalk_openref =%d\n", *err);
#endif
		gref_close(gref);
		return -1;
	}
/*
	kprintf("_ATsocket: proto=%d return=%d fd=%d\n", proto, *err, fd);
*/
	return fd;
}

int _ATgetmsg(fd, ctlptr, datptr, flags, err, proc)
	int fd;
	strbuf_t *ctlptr;
	strbuf_t *datptr;
	int *flags;
	int *err;
	void *proc;
{
	int rc = -1;
	gref_t *gref;

	if ((*err = atalk_getref(0, fd, &gref, proc)) == 0) {
		switch (gref->proto) {
		case ATPROTO_ASP:
			rc = ASPgetmsg(gref, ctlptr, datptr, flags, err); break;
		case ATPROTO_AURP:
#ifdef AURP_SUPPORT
			rc = AURPgetmsg(err); break;
#endif
		default:
			*err = EPROTONOSUPPORT; break;
		}
	}


/*	kprintf("_ATgetmsg: return=%d\n", *err);*/
	return rc;
}

int _ATputmsg(fd, ctlptr, datptr, flags, err, proc)
	int fd;
	strbuf_t *ctlptr;
	strbuf_t *datptr;
	int flags;
	int *err;
	void *proc;
{
	int rc = -1;
	gref_t *gref;

	if ((*err = atalk_getref(0, fd, &gref, proc)) == 0) {
		switch (gref->proto) {
		case ATPROTO_ASP:
			rc = ASPputmsg(gref, ctlptr, datptr, flags, err); break;
		default:
			*err = EPROTONOSUPPORT; break;
		}
	}

/*	kprintf("_ATputmsg: return=%d\n", *err); */
	return rc;
}

int _ATclose(fp, proc)
	void *fp;
	void *proc;
{
	int err;
	gref_t *gref;

	if ((err = atalk_closeref(fp, &gref)) != 0)
		return err;

	gref_close(gref);
	return 0;
}

int _ATrw(fp, rw, uio, ext)
	void *fp;
	enum uio_rw rw;
	struct uio *uio;
	int ext;
{
	void gref_wput();
	int s, err, len, rlen, clen, res;
	gref_t *gref;
	gbuf_t *m, *mhead, *mprev;

	if ((err = atalk_getref(fp, 0, &gref, 0)) != 0)
		return err;

	if ((len = uio->uio_resid) == 0)
		return 0;

	ATDISABLE(s, gref->lock);
	if (gref->errno) {
		ATENABLE(s, gref->lock);
		return (int)gref->errno;
	}

  if (rw == UIO_READ) {
	while ((gref->errno == 0) && ((mhead = gref->rdhead) == 0)) {
		gref->sevents |= POLLMSG;
		err = tsleep(&gref->event, PSOCK | PCATCH, "AT read", 0);
		gref->sevents &= ~POLLMSG;
		if (err != 0) {
			ATENABLE(s, gref->lock);
			return err;
		}
	}

	if (gref->errno) {
		ATENABLE(s, gref->lock);
		return EPIPE;
	}
	if ((gref->rdhead = gbuf_next(mhead)) == 0)
		gref->rdtail = 0;

	ATENABLE(s, gref->lock);

	gbuf_next(mhead) = 0;

	for (mprev=0, m=mhead, rlen=0; m && len; rlen+=clen, len-=clen) {
		if ((clen = gbuf_len(m)) > 0) {
			if (clen > len)
				clen = len;
#ifdef _AIX
			if (uiomove((caddr_t)gbuf_rptr(m), clen, UIO_READ, uio) == -1)
				break;
#else
			uio->uio_rw = UIO_READ;
			res= uiomove((caddr_t)gbuf_rptr(m), clen, uio);
			/* kprintf("_ATrw: UIO_READ: res=%d\n", res); */
			if (res == -1)
				break;
#endif
			if (gbuf_len(m) > len) {
				gbuf_rinc(m,clen);
				break;
			}
		}
		mprev = m;
		m = gbuf_cont(m);
	}
	if (m) {
		if (mprev)
			gbuf_cont(mprev) = 0;
		else
			mhead = 0;
		ATDISABLE(s, gref->lock);
		if (gref->rdhead == 0)
			gref->rdtail = m;
		gbuf_next(m) = gref->rdhead;
		gref->rdhead = m;
		ATENABLE(s, gref->lock);
	}
	if (mhead)
		gbuf_freem(mhead);

  } else {
	if (gref->writeable) {
		while (!(*gref->writeable)(gref)) {
			/* flow control on, wait to be enabled to write */ 
			gref->sevents |= POLLSYNC;
			err = tsleep(&gref->event, PSOCK | PCATCH, "AT write", 0);
			gref->sevents &= ~POLLSYNC;
			if (err != 0) {
				ATENABLE(s, gref->lock);
				return err;
			}
		}
	}

	ATENABLE(s, gref->lock);

	/* allocate a buffer to copy in the write data */
	if ((m = gbuf_alloc(AT_WR_OFFSET+len, PRI_MED)) == 0)
		return ENOBUFS;
	gbuf_rinc(m,AT_WR_OFFSET);
	gbuf_wset(m,len);

	/* copy in the write data */
#ifdef _AIX
	if (uiomove((caddr_t)gbuf_rptr(m), len, UIO_WRITE, uio) == -1) {
#else
	uio->uio_rw = UIO_WRITE;
	res= uiomove((caddr_t)gbuf_rptr(m), len, uio);
	/* kprintf("_ATrw: UIO_WRITE: res=%d\n", res); */
	if (res == -1) {
#endif
		gbuf_freeb(m);
		return EIO;
	}

	/* forward the write data to the appropriate protocol module */
	gref_wput(gref, m);
  }

	return 0;
}

int _ATread(fp, uio, cred)
	void *fp;
	struct uio *uio;
	void *cred;
{
	return _ATrw(fp, UIO_READ, uio, 0);
}

int _ATwrite(fp, uio, cred)
	void *fp;
	struct uio *uio;
	void *cred;
{
	return _ATrw(fp, UIO_WRITE, uio, 0);
}
/*
struct at_ioctl_args {
	int fdes;
	u_long cmd;
	caddr_t arg;
};
int _ATioctl(p, uap, retval)
	struct proc *p;
	register struct at_ioctl_args *uap;
	register_t *retval;
*/
int _ATioctl(fp, cmd, arg, proc)
	void *fp;
	u_long cmd;
	register caddr_t arg;
	void *proc;
{
	void gref_wput();
	int s, err, len;
	gref_t *gref;
	gbuf_t *m, *mdata;
	ioc_t *ioc;
	ioccmd_t ioccmd;


	if ((err = atalk_getref(fp, 0, &gref, 0)) != 0) {
#ifdef APPLETALK_DEBUG
		kprintf("_ATioctl: atalk_getref=%d\n", err);
#endif
		return err;
	}

	/* error if not for us */
	if ((cmd  & 0xffff) != 0xff99)
		return EOPNOTSUPP;

	/* copy in ioc command info */
/*
	kprintf("_ATioctl: arg ioccmd.ic_cmd=%x ic_len=%x gref->lock=%x, gref->event=%x\n",
		((ioccmd_t *)arg)->ic_cmd, ((ioccmd_t *)arg)->ic_len, gref->lock, gref->event);
*/
	if ((err = copyin((caddr_t)arg,
			(caddr_t)&ioccmd, sizeof(ioccmd_t))) != 0) { 
#ifdef APPLETALK_DEBUG
	  kprintf("_ATioctl: err = %d, copyin(%x, %x, %d)\n", err, (caddr_t)arg,
		  (caddr_t)&ioccmd, sizeof(ioccmd_t));
#endif
		return err;
	} 

	/* allocate a buffer to create an ioc command */
	if ((m = gbuf_alloc(sizeof(ioc_t), PRI_HI)) == 0)
		return ENOBUFS;
	gbuf_wset(m,sizeof(ioc_t));
	gbuf_set_type(m, MSG_IOCTL);

	/* create the ioc command */
	if (ioccmd.ic_len) {
		if ((gbuf_cont(m) = gbuf_alloc(ioccmd.ic_len, PRI_HI)) == 0) {
			gbuf_freem(m);
#ifdef APPLETALK_DEBUG
			kprintf("_ATioctl: gbuf_alloc err=%d\n",ENOBUFS);
#endif
			return ENOBUFS;
		}
		gbuf_wset(gbuf_cont(m),ioccmd.ic_len);
		if ((err = copyin((caddr_t)ioccmd.ic_dp,
				(caddr_t)gbuf_rptr(gbuf_cont(m)), ioccmd.ic_len)) != 0) { 
			gbuf_freem(m);
			return err;
		}
	}
	ioc = (ioc_t *)gbuf_rptr(m);
	ioc->ioc_cmd = ioccmd.ic_cmd;
	ioc->ioc_count = ioccmd.ic_len;
	ioc->ioc_error = 0;
	ioc->ioc_rval = 0;

	/* send the ioc command to the appropriate recipient */
	gref_wput(gref, m);

	/* wait for the ioc ack */
	ATDISABLE(s, gref->lock);
	while ((m = gref->ichead) == 0) {
		gref->sevents |= POLLPRI;
		err = tsleep(&gref->iocevent, PSOCK | PCATCH, "AT ioctl", 0);
		gref->sevents &= ~POLLPRI;
		if (err != 0) {
			ATENABLE(s, gref->lock);
#ifdef APPLETALK_DEBUG
			kprintf("_ATioctl: EINTR\n");
#endif
			return err;
		}
	}

/* PR-2224797 */
 	if (gbuf_next(m) == m)		/* error case */
		gbuf_next(m) = 0; 

	gref->ichead = gbuf_next(m);

	ATENABLE(s, gref->lock);


/*	kprintf("_ATioctl: woke up from tread sleep\n"); */
	/* process the ioc response */
	ioc = (ioc_t *)gbuf_rptr(m);
	if ((err = ioc->ioc_error) == 0) {
		ioccmd.ic_timout = ioc->ioc_rval;
		ioccmd.ic_len = 0;
		mdata = gbuf_cont(m);
		if (mdata && ioccmd.ic_dp) {
			ioccmd.ic_len = gbuf_msgsize(mdata);
		  for (len=0; mdata; mdata=gbuf_cont(mdata)) {
			if ((err = copyout((caddr_t)gbuf_rptr(mdata),
					(caddr_t)&ioccmd.ic_dp[len], gbuf_len(mdata))) < 0) {
#ifdef APPLETALK_DEBUG
				kprintf("_ATioctl: len=%d error copyout=%d from=%x to=%x gbuf_len=%x\n",
					 len, err, (caddr_t)gbuf_rptr(mdata), 
					 (caddr_t)&ioccmd.ic_dp[len], gbuf_len(mdata));
#endif
				goto l_done;
			}
			len += gbuf_len(mdata);
		  }
		}
		if ((err = copyout((caddr_t)&ioccmd,
				(caddr_t)arg, sizeof(ioccmd_t))) != 0) {
#ifdef APPLETALK_DEBUG
				kprintf("_ATioctl: error copyout2=%d from=%x to=%x len=%d\n",
					 err, &ioccmd, arg, sizeof(ioccmd_t));
#endif
			goto l_done;
		}
	}

l_done:
	gbuf_freem(m);
	/*kprintf("_ATioctl: I_done=%d\n", err);*/
	return err;
}

#ifndef _AIX
int _ATselect(fp, which, proc)
	void *fp;
	int which;
	void *proc;
{
	int s, err, rc = 0;
	gref_t *gref;

	if ((err = atalk_getref(fp, 0, &gref, 0)) != 0)
		return err;

	ATDISABLE(s, gref->lock);
	if (which == FREAD) {
		if (gref->rdhead || (gref->readable && (*gref->readable)(gref)))
			rc = 1;
		else {
			gref->sevents |= POLLIN;
			selrecord(proc, &gref->si);
		}
	}

	else if (which == POLLOUT) {
		if (gref->writeable) {
			if ((*gref->writeable)(gref))
				rc = 1;
			else {
				gref->sevents |= POLLOUT;
				selrecord(proc, &gref->si);
			}
		} else
			rc = 1;
	}
	ATENABLE(s, gref->lock);

	return rc;
}

#else  /* AIX code is no longer supported. */

int _ATselect(fp, corl, reqevents, retevents, notify)
	void *fp;
	int corl;
	unsigned short reqevents;
	unsigned short *retevents;
	void (*notify)();
{
	int s, err, rc = 0;
	gref_t *gref;
	unsigned short sevents = 0;

	if ((err = atalk_getref(fp, 0, &gref, 0)) != 0)
		return err;

	ATDISABLE(s, gref->lock);
	if (reqevents & POLLIN) {
		if (gref->rdhead || (gref->readable && (*gref->readable)(gref)))
			sevents |= POLLIN;
	}

	if (reqevents & POLLOUT) {
		if (gref->writeable) {
			if ((*gref->writeable)(gref))
				sevents |= POLLOUT;
		} else
			sevents |= POLLOUT;
	}

	if ((sevents == 0) && ((reqevents & POLLSYNC) == 0)) {
		if (rc = selreg(corl, 99, gref, reqevents, notify)) {
			ATENABLE(s, gref->lock);
			goto l_done;
		}

          if (reqevents & POLLIN) {
			if (gref->rdhead || (gref->readable && (*gref->readable)(gref)))
				sevents |= POLLIN;
			else
				gref->sevents |= POLLIN;
          }

          if (reqevents & POLLOUT) {
			if (gref->writeable) {
				if ((*gref->writeable)(gref))
					sevents |= POLLOUT;
				else
					gref->sevents |= POLLOUT;
			} else
				sevents |= POLLOUT;
          }
     }
	ATENABLE(s, gref->lock);
     *retevents = sevents;

l_done:
	return rc;
}

int _ATstat()
{
	return 0;
}
#endif  /* end AIX section */

void atalk_putnext(gref, m)
	gref_t *gref;
	gbuf_t *m;
{
	int s;

	ATDISABLE(s, gref->lock);

	gbuf_next(m) = 0;

	if ((gbuf_type(m) == MSG_IOCACK) || (gbuf_type(m) == MSG_IOCNAK)) {
		if (gref->ichead)
			gbuf_next(gref->ichead) = m;
		else {
			gref->ichead = m;
			if (gref->sevents & POLLPRI) {
				/*kprintf("atalk_putnext: wake up gref->event=%x\n", &gref->event);*/
				thread_wakeup(&gref->iocevent);
			}
			atalk_notify_sel(gref);
		}

	} else {
	  if (gref->errno == 0) {
		if (gbuf_type(m) == MSG_ERROR) {
	  		gref->errno = *gbuf_rptr(m);
			if (gref->rdhead) {
				gbuf_freel(gref->rdhead);
				gref->rdhead = 0;
			}
			if (gref->sevents & POLLMSG) {
				gref->sevents &= ~POLLMSG;
				/*kprintf("atalk_putnext: wake up gref->event=%x\n", &gref->event);*/
				thread_wakeup(&gref->event);
			}
			if (gref->sevents & POLLIN) {
				gref->sevents &= ~POLLIN;
				/*kprintf("atalk_putnext: selwakeup gref->si=%x\n", &gref->si); */
				selwakeup(&gref->si);
			}
		} else if (gref->rdhead) {
			gbuf_next(gref->rdtail) = m;
			gref->rdtail = m;
		} else {
			gref->rdhead = m;
			if (gref->sevents & POLLMSG) {
				gref->sevents &= ~POLLMSG;
				/*kprintf("atalk_putnext: wake up2 gref->event=%x\n", &gref->event);*/
				thread_wakeup(&gref->event);
			}
			if (gref->sevents & POLLIN) {
				gref->sevents &= ~POLLIN;
				/*kprintf("atalk_putnext: selwakeup2 gref->si=%x\n", &gref->si);*/
				selwakeup(&gref->si);
			}
			gref->rdtail = m;
		}
	  }
	}

	ATENABLE(s, gref->lock);
}

void atalk_enablew(gref)
	gref_t *gref;
{
	if (gref->sevents & POLLSYNC)
#ifdef _AIX
		e_wakeup(&gref->event);
#else
		thread_wakeup(&gref->event);
#endif
}

void atalk_flush(gref)
	gref_t *gref;
{
	int s;

	ATDISABLE(s, gref->lock);
	if (gref->rdhead) {
		gbuf_freel(gref->rdhead);
		gref->rdhead = 0;
	}
	if (gref->ichead) {
		gbuf_freel(gref->ichead);
		gref->ichead = 0;
	}
	ATENABLE(s, gref->lock);
}

void atalk_notify_sel(gref)
	gref_t *gref;
{
	int s;

	ATDISABLE(s, gref->lock);
	if (gref->sevents & POLLIN) {
		gref->sevents &= ~POLLIN;
		selwakeup(&gref->si);
	}
	ATENABLE(s, gref->lock);
}

int atalk_peek(gref, event)
	gref_t *gref;
	unsigned char *event;
{
	int s, rc;

	ATDISABLE(s, gref->lock);
	if (gref->rdhead) {
		*event = *gbuf_rptr(gref->rdhead);
		rc = 0;
	} else
		rc = -1;
	ATENABLE(s, gref->lock);

	return rc;
}

static gbuf_t *trace_msg;

void atalk_settrace(str, p1, p2, p3, p4, p5)
	char *str;
{
	int len;
	gbuf_t *m, *nextm;
	char trace_buf[256];

	sprintf(trace_buf, str, p1, p2, p3, p4, p5);
	len = strlen(trace_buf);
#ifdef APPLETALK_DEBUG
	kprintf("atalk_settrace: gbufalloc size=%d\n", len+1);
#endif
	if ((m = gbuf_alloc(len+1, PRI_MED)) == 0)
		return;
	gbuf_wset(m,len);
	strcpy(gbuf_rptr(m), trace_buf);
	if (trace_msg) {
		for (nextm=trace_msg; gbuf_cont(nextm); nextm=gbuf_cont(nextm)) ;
		gbuf_cont(nextm) = m;
	} else
		trace_msg = m;
}

void atalk_gettrace(m)
	gbuf_t *m;
{
	if (trace_msg) {
		gbuf_cont(m) = trace_msg;
		trace_msg = 0;
	}
}

#define GREF_PER_BLK 32
static gref_t *gref_free_list = 0;
atlock_t refall_lock;

void gref_init()
{
	ATLOCKINIT(refall_lock);
}

gref_t *gref_alloc()
{
	extern gbuf_t *atp_resource_m;
	int i, s;
	gbuf_t *m;
	gref_t *gref, *gref_array;

	ATDISABLE(s, refall_lock);
	if (gref_free_list == 0) {
		ATENABLE(s, refall_lock);
#ifdef APPLETALK_DEBUG
		kprintf("gref_alloc: gbufalloc size=%d\n", GREF_PER_BLK*sizeof(gref_t));
#endif
		if ((m = gbuf_alloc(GREF_PER_BLK*sizeof(gref_t),PRI_HI)) == 0)
			return (gref_t *)0;
		bzero(gbuf_rptr(m), GREF_PER_BLK*sizeof(gref_t));
		gref_array = (gref_t *)gbuf_rptr(m);
		for (i=0; i < GREF_PER_BLK-1; i++)
			gref_array[i].next = (gref_t *)&gref_array[i+1];
		ATDISABLE(s, refall_lock);
		gbuf_cont(m) = atp_resource_m;
		atp_resource_m = m;
		gref_array[i].next = gref_free_list;
		gref_free_list = (gref_t *)&gref_array[0];
	}

	gref = gref_free_list;
	gref_free_list = gref->next;
	ATENABLE(s, refall_lock);
	ATLOCKINIT(gref->lock);
	ATEVENTINIT(gref->event);
	ATEVENTINIT(gref->iocevent);

	return gref;
}

void gref_free(gref)
	gref_t *gref;
{
	int s;

	ATDISABLE(s, refall_lock);
	bzero((char *)gref, sizeof(gref_t));
	gref->next = gref_free_list;
	gref_free_list = gref;
	ATENABLE(s, refall_lock);
}

int gref_open(grefp)
	gref_t **grefp;
{
	gref_t *gref;

	/*
	 * if no gref structure available, return failure
	 */
	if ((gref = (gref_t *)gref_alloc()) == 0)
		return ENOBUFS;
#ifndef NEXT
	gref->pid = getpid();
#endif
	*grefp = gref;

	return 0;
}

int gref_close(gref)
	gref_t *gref;
{
	int rc;

	switch (gref->proto) {
	case ATPROTO_DDP:
		rc = ddp_close(gref); break;
	case ATPROTO_LAP:
		rc = lap_close(gref); break;
	case ATPROTO_ATP:
		rc = atp_close(gref, 1); break;
	case ATPROTO_ASP:
	  	rc = asp_close(gref); break;
#ifdef AURP_SUPPORT
	case ATPROTO_AURP:
		rc = aurp_close(gref); break;
		break;
#endif
	case ATPROTO_ADSP:
		rc = adsp_close(gref); break;
	default:
		rc = 0;
		break;
	}

	if (rc == 0) {
		atalk_flush(gref);
		selthreadclear(&gref->si);
		gref_free(gref);
	}

	return rc;
}

void gref_wput(gref, m)
	gref_t *gref;
	gbuf_t *m;
{

	switch (gref->proto) {
	case ATPROTO_DDP:
		ddp_putmsg(gref, m); break;
	case ATPROTO_LAP:
		elap_wput(gref, m); break;
	case ATPROTO_ATP:
		atp_wput(gref, m); break;
	case ATPROTO_ASP:
		asp_wput(gref, m); break;
#ifdef AURP_SUPPORT
	case ATPROTO_AURP:
		aurp_wput(gref, m); break;
#endif
	case ATPROTO_ADSP:
		adsp_wput(gref, m); break;
	case ATPROTO_NONE:
		if (gbuf_type(m) == MSG_IOCTL) {
			gbuf_freem(gbuf_cont(m));
			gbuf_cont(m) = 0;
			((ioc_t *)gbuf_rptr(m))->ioc_rval = -1;
			((ioc_t *)gbuf_rptr(m))->ioc_error = EPROTO;
			gbuf_set_type(m, MSG_IOCNAK);
			atalk_putnext(gref, m);
		} else
			gbuf_freem(m);
		break;
	default:
		gbuf_freem(m);
		break;
	}
}

int gbuf_msgsize(m)
	gbuf_t *m;
{
	int size;

	for (size=0; m; m=gbuf_cont(m))
		size += gbuf_len(m);
	return size;
}

/* Duplicate a single mbuf, attaching existing external storage. */
gbuf_t *gbuf_dupb(m)
     gbuf_t *m;
{
	return((gbuf_t *)m_copym(m, 0, gbuf_len(m), M_DONTWAIT));
}

gbuf_t *gbuf_dupb_wait(m, wait)
     gbuf_t *m;
     int wait;
{
	return((gbuf_t *)m_copym(m, 0, gbuf_len(m), (wait)? M_WAIT: M_DONTWAIT));
}

/* Duplicate an mbuf chain, attaching existing external storage. */
gbuf_t *gbuf_dupm(mlist)
     gbuf_t *mlist;
{
	return((gbuf_t *)m_copym(mlist, 0, M_COPYALL, M_DONTWAIT));
}

gbuf_t *gbuf_copym(mlist)
	gbuf_t *mlist;
{
	int len, offset;
	gbuf_t *m, *mcurr, *mprev, *mhead;

	offset = AT_WR_OFFSET;
	for (mhead = 0, m = mlist; m; m = gbuf_cont(m)) {
		if ((len = gbuf_len(m)) == 0)
			continue;
		if ((mcurr = gbuf_alloc(offset+len, PRI_MED)) == 0) {
			if (mhead)
				gbuf_freem(mhead);
			return (gbuf_t *)0;
		}
		gbuf_rinc(mcurr,offset);
		gbuf_wset(mcurr,len);
		gbuf_set_type(mcurr, gbuf_type(m));
		bcopy(gbuf_rptr(m), gbuf_rptr(mcurr), len);
		if (mhead)
			gbuf_cont(mprev) = mcurr;
		else {
			mhead = mcurr;
			offset = 0;
		}
		mprev = mcurr;
	}

	return (gbuf_t *)mhead;
}

void gbuf_freem(mlist)
	gbuf_t *mlist;
{
	m_freem((struct mbuf *)mlist);
}

void gbuf_linkb(m1, m2)
	gbuf_t *m1;
	gbuf_t *m2;
{
	while (gbuf_cont(m1) != 0)
		m1 = gbuf_cont(m1);
	gbuf_cont(m1) = m2;
}

/**************************************/

int ddp_adjmsg(m, len)
	gbuf_t 		*m;
	int 	len;
{
	int buf_len;
	gbuf_t *curr_m, *prev_m;

	if (m == (gbuf_t *)0)
		return 0;

	if (len > 0) {
		for (curr_m=m; curr_m;) {
			buf_len = gbuf_len(curr_m);
			if (len < buf_len) {
				gbuf_rinc(curr_m,len);
				return 1;
			}
			len -= buf_len;
			gbuf_rinc(curr_m,buf_len);
			if ((curr_m = gbuf_cont(curr_m)) == 0) {
				gbuf_freem(m);
				return 0;
			}
		}

	} else if (len < 0) {
		len = -len;
l_cont:	prev_m = 0;
		for (curr_m=m; gbuf_cont(curr_m);
			prev_m=curr_m, curr_m=gbuf_cont(curr_m)) ;
		buf_len = gbuf_len(curr_m);
		if (len < buf_len) {
			gbuf_wdec(curr_m,len);
			return 1;
		}
		if (prev_m == 0)
			return 0;
		gbuf_cont(prev_m) = 0;
		gbuf_freeb(curr_m);
		len -= buf_len;
		goto l_cont;

	} else
		return 1;
}

/*
 * The message chain, m is grown in size by len contiguous bytes.
 * If len is non-negative, len bytes are added to the
 * end of the gbuf_t chain.  If len is negative, the
 * bytes are added to the front. ddp_growmsg only adds bytes to 
 * message blocks of the same type.
 * It returns a pointer to the new gbuf_t on sucess, 0 on failure.
 */

gbuf_t *ddp_growmsg(mp, len)
	gbuf_t 	*mp;
	int 	len;
{
	gbuf_t	*m, *d;

	if ((m = mp) == (gbuf_t *) 0)
		return ((gbuf_t *) 0);

	if (len <= 0) {
		len = -len;
		if ((d = gbuf_alloc(len, PRI_MED)) == 0)
			return ((gbuf_t *) 0);
		gbuf_set_type(d, gbuf_type(m));
		gbuf_wset(d,len);
		/* link in new gbuf_t */
		gbuf_cont(d) = m;
		return (d);

	} else {
	        register int	count;
		/*
		 * Add to tail.
		 */
		if ((count = gbuf_msgsize(m)) < 0)
			return ((gbuf_t *) 0);
		/* find end of chain */
		for ( ; m; m = gbuf_cont(m)) {
			if (gbuf_len(m) >= count) 
				break;
			count -= gbuf_len(m);
		}
		/* m now points to gbuf_t to add to */
		if ((d = gbuf_alloc(len, PRI_MED)) == 0)
			return ((gbuf_t *) 0);
		gbuf_set_type(d, gbuf_type(m));
		/* link in new gbuf_t */
		gbuf_cont(d) = gbuf_cont(m);
		gbuf_cont(m) = d;
		gbuf_wset(d,len);
		return (d);
	}
}



/*
 *	return an MSG_ERROR on the read stream. Note that the same message
 *	block that caused the problem is used as the vehicle. So chained 
 *	blocks to this message will be free'd; i.e. ASSUMES no multiple
 *	references.
 *	Used by other appletalk modules, so it is not static!
 */

void data_error(errno,mp,gref)
int		errno;
register gbuf_t	*mp;
register gref_t	*gref;
{

        gbuf_set_type(mp, MSG_ERROR);
	
	*gbuf_rptr(mp) = (unsigned char)errno;
	gbuf_wset(mp,1);
	if (gbuf_cont(mp)) {
		gbuf_freem(gbuf_cont(mp));
		gbuf_cont(mp) = 0;
	}

	atalk_putnext(gref, mp);
}

/*
 *	return the MSG_IOCACK/MSG_IOCNAK. Note that the same message
 *	block is used as the vehicle, and that if there is an error return,
 *	then linked blocks are lopped off. BEWARE of multiple references.
 *	Used by other appletalk modules, so it is not static!
 */

void ioc_ack(errno, m, gref)
int		errno;
register gbuf_t	*m;
register gref_t	*gref;

{
	ioc_t *iocbp = (ioc_t *)gbuf_rptr(m);
	
	/*kprintf("ioc_ack: m=%x gref=%x errno=%d\n", m, gref, errno);*/

	if (gref->info == NULL) {
		gbuf_freem(m);
#ifdef APPLETALK_DEBUG
		kprintf("ioc_ack: gref->info == NULL: free m\n");
#endif
		return;
	}
	if ((iocbp->ioc_error = errno) != 0)
	{	/* errno != 0, then there is an error, get rid of linked blocks! */

		if (gbuf_cont(m)) {
		        gbuf_freem(gbuf_cont(m));
		        gbuf_cont(m) = 0;
		}
	        gbuf_set_type(m, MSG_IOCNAK);
		iocbp->ioc_count = 0;	/* only make zero length if error */
		iocbp->ioc_rval = -1;
	} else
	        gbuf_set_type(m, MSG_IOCACK);

	atalk_putnext(gref, m);
}

/* Queue management functions */

void
ddp_remque(elem)
	register LIB_QELEM_T    *elem;
{
	elem->q_back->q_forw = elem->q_forw;
	elem->q_forw->q_back = elem->q_back;
}

void
ddp_insque(elem, prev)
	register LIB_QELEM_T 	*elem, *prev;
{ 
	elem->q_back = prev;
	elem->q_forw = prev->q_forw;
	prev->q_forw->q_back = elem;
	prev->q_forw = elem;
}
