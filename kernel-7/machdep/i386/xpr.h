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
 * Copyright (c) 1987, 1988, 1989 NeXT, Inc.
 *
 * HISTORY
 * 16-Mar-93    Curtis Galloway at NeXT
 *      Removed obsolete eventc.h.
 *
 * 02-Mar-89	Doug Mitchell at NeXT
 *	Added XPR_SCCMD.
 *
 * 25-Feb-88  Gregg Kellogg (gk) at NeXT
 *	Changed timestamp to use event counter
 */ 

/*
 *	File:	i386/xpr.h
 *
 *	Machine dependent module for the XPR tracing facility.
 */

#ifdef	KERNEL_BUILD
#import "uxpr.h"
#import "xpr_debug.h"
#else	/* KERNEL_BUILD */
#import <mach/features.h>
#endif	/* KERNEL_BUILD */

#ifndef	DDM_DEBUG
#define DDM_DEBUG	XPR_DEBUG
#endif	/* DDM_DEBUG */

#import <driverkit/debugging.h>
#import <driverkit/ddmPrivate.h>
#import <driverkit/Device_ddm.h>
#import <driverkit/xpr_mi.h>

/* 
 * This stuff is old, probably should be deleted.
 */
#define XPR_TIMESTAMP	event_get()

#define	XPR_OD		(1 << 21)
#define	XPR_PRINTER	(1 << 22)
#define XPR_SOUND	(1 << 23)
#define XPR_DSP		(1 << 24)
#define XPR_MIDI	(1 << 25)
#define XPR_DMA		(1 << 26)
#define XPR_EVENT	(1 << 27)
#define	XPR_SCSI	(1 << 28)
#define XPR_SCC		(1 << 29)
#define XPR_FD		(1 << 30)
#define	XPR_KM		(1 << 31)

/*
 * Real driverkit style DDM defines.
 */
 
/*
 * Compile-time flags:
 *
 * UXPR - this enables driverkit-style xpr functionality. The module which
 *	  implements this, machdep/nrw/ddm.c, is compiled into the kernel 
 *	  when this flag is true. Currently this is true in all nrw builds.
 * DDM_DEBUG - enables actual uxpr() macros. Currrently this is true only
 *	  in nrw DEBUG builds. 
 *
 * RELEASE kernels compile in ddm.c to allow loadable servers to use the 
 *	  uxpr mechanism.
 */
 
#define XPR_DMABUF	0x00000100	// IOEISADirectDevice dma buffers

#define xpr_dmabuf(x, a, b, c, d, e) 					\
	IODEBUG(XPR_IODEVICE_INDEX, XPR_DMABUF, x, a, b, c, d, e)

