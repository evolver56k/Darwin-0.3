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
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * xpr silent tracing circular buffer.
 */

#if	NeXT
#define NO_IPLMEAS	1
#endif	NeXT

#import <kern/xpr.h>
#import <kern/lock.h>
#import <bsd/machine/cpu.h>
#import <machine/param.h>		/* for splhigh */
#import <vm/vm_kern.h>
#import <machine/spl.h>

decl_simple_lock_data(,xprlock)

#if	NeXT
/* always allocate xpr buffers so xprs can be enabled anytime */
int nxprbufs = 512;	/* Number of contiguous xprbufs allocated */
#else	NeXT
int nxprbufs = 0;	/* Number of contiguous xprbufs allocated */
#endif	NeXT
unsigned int xprflags = 0;	/* Bit mask of xpr flags enabled */
unsigned int xprwrap = TRUE;	/* xpr's wrap in buf or just stop at end */
struct xprbuf *xprbase;	/* Pointer to circular buffer nxprbufs*sizeof(xprbuf)*/
struct xprbuf *xprptr;	/* Currently allocated xprbuf */
struct xprbuf *xprlast;	/* Pointer to end of circular buffer */
unsigned xprlocked = 0;	/* when TRUE, disables logging */

/*VARARGS1*/
xpr(msg, arg1, arg2, arg3, arg4, arg5, arg6)
	char *msg;
{
	register s;
	register struct xprbuf *x;

#ifdef	lint
	arg6++;
#endif	lint

	/* If we aren't initialized, ignore trace request */
	if (!nxprbufs || xprptr == 0 || xprlocked)
		return;
	/* Guard against all interrupts and allocate next buffer */
	s = splhigh();
	simple_lock(&xprlock);
	x = xprptr++;
	if (xprptr >= xprlast) {
		/* wrap around */
		xprptr = xprwrap ? xprbase : xprlast - 1;
	}
	x->timestamp = XPR_TIMESTAMP;
	simple_unlock(&xprlock);
	splx(s);
	x->msg = msg;
	x->arg1 = arg1;
	x->arg2 = arg2;
	x->arg3 = arg3;
	x->arg4 = arg4;
	x->arg5 = arg5;
	x->cpuinfo = cpu_number();
}

xprbootstrap()
{
	simple_lock_init(&xprlock);
	if (nxprbufs == 0 && xprflags == 0)
		return;	/* assume XPR support not desired */
	else if (nxprbufs == 0)
		nxprbufs=512;	/* if flags are set must do something */
	xprbase = (struct xprbuf *)kmem_alloc(kernel_map, 
			(vm_size_t)(nxprbufs * sizeof(struct xprbuf)));
	xprlast = &xprbase[nxprbufs];
	xprptr = xprbase;
}

int		xprinitial = 0;

xprinit()
{
	xprflags |= xprinitial;
}

/*
 *	Save 'number' of xpr buffers to the area provided by buffer.
 */

xpr_save(buffer,number)
struct xprbuf *buffer;
int number;
{
    int i,s;
    struct xprbuf *x;
    s = splhigh();
    simple_lock(&xprlock);
    if (number > nxprbufs) number = nxprbufs;
    x = xprptr;
    
    for (i = number - 1; i >= 0 ; i-- ) {
	if (--x < xprbase) {
	    /* wrap around */
	    x = xprlast - 1;
	}
	bcopy(x,&buffer[i],sizeof(struct xprbuf));
    }
    simple_unlock(&xprlock);
    splx(s);
}
