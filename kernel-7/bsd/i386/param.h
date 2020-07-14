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

/* Copyright (c) 1993,1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 *	@(#)param.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Machine dependent constants for Intel 386.
 */

#ifndef _I386_PARAM_H_
#define _I386_PARAM_H_

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 */
#define	ALIGNBYTES	3
#define	ALIGN(p)	(((u_int)(p) + ALIGNBYTES) &~ ALIGNBYTES)

#define	NBPG		8192		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		13		/* LOG2(NBPG) */

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	STACK_GROWS_UP	0		/* stack grows to lower addresses */

#define	CLSIZE		1
#define	CLSIZELOG2	0

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		128		/* size of an mbuf */
#define	MCLBYTES	2048		/* large enough for ether MTU */
#define	MCLSHIFT	11
#define	MCLOFSET	(MCLBYTES - 1)
#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS	((1024 * 1024) / MCLBYTES)	/* cl map size: 1MB */
#else
#define	NMBCLUSTERS	((1024 * 512) / MCLBYTES)	/* cl map size: 0.5MB */
#endif
#endif

/*
 * Some macros for units conversion
 */
/* Core clicks (NeXT_page_size bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (4096 bytes) to disk blocks */
#define	ctod(x)	((x)<<(PGSHIFT-DEV_BSHIFT))
#define	dtoc(x)	((x)>>(PGSHIFT-DEV_BSHIFT))
#define	dtob(x)	((x)<<DEV_BSHIFT)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	(((unsigned)(x)+(NBPG-1))>>PGSHIFT)

#ifdef __APPLE__
#define  btodb(bytes, devBlockSize)         \
        ((unsigned)(bytes) / devBlockSize)
#define  dbtob(db, devBlockSize)            \
        ((unsigned)(db) * devBlockSize)
#else
#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)
#endif

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be if we
 * add an entry to cdevsw/bdevsw for that purpose.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * Macros to decode (and encode) processor status word.
 */
#define STATUS_WORD(rpl, ipl)	(((ipl) << 8) | (rpl))
#define	USERMODE(x)		(((x) & 3) == 3)
#define	BASEPRI(x)		(((x) & (255 << 8)) == 0)

#ifndef __ASSEMBLER__
/*
 * Pull in our definition of simple_lock_data_t.
 */
#import <mach/machine/simple_lock.h>

#endif	/* __ASSEMBLER__ */

#if	defined(KERNEL) || defined(STANDALONE)
#define	DELAY(n) us_spin(n)

#else	/* defined(KERNEL) || defined(STANDALONE) */
#define	DELAY(n)	{ register int N = (n); while (--N > 0); }
#endif	/* defined(KERNEL) || defined(STANDALONE) */

#endif /* _I386_PARAM_H_ */
