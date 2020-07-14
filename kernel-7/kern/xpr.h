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
 * HISTORY
 * Revision 1.1.1.1  1997/09/30 02:44:35  wsanchez
 * Import of kernel from umeshv/kernel
 *
 *
 * 21-Jan-92  Doug Mitchell at NeXT
 *	Added support for m88k uxpr module.
 *
 * 22-Jun-89  Mike DeMoney (mike) at NeXT.
 *	Put xpr macro in {}'s so it wouldn't steal a following "else" clause.
 *
 * Revision 2.10  89/05/30  10:38:29  rvb
 * 	Removed Mips crud.
 * 	[89/04/20            af]
 * 
 * Revision 2.9  89/03/09  20:17:45  rpd
 * 	More cleanup.
 * 
 * Revision 2.8  89/02/25  18:11:09  gm0w
 * 	Kernel code cleanup.
 * 	Made XPR_DEBUG stuff always true outside the kernel
 * 	[89/02/15            mrt]
 * 
 * Revision 2.7  89/02/07  01:06:11  mwyoung
 * Relocated from sys/xpr.h
 * 
 * Revision 2.6  89/01/23  22:30:39  af
 * 	Added more flags, specific to Mips.  Should eventually integrate,
 * 	but for now there are conflicts.
 * 	Also made it usable from assembly code.
 * 	[89/01/05            af]
 * 
 * Revision 2.5  88/12/19  02:51:59  mwyoung
 * 	Added VM system tags.
 * 	[88/11/22            mwyoung]
 * 
 * Revision 2.4  88/08/24  02:55:54  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:29:56  mwyoung]
 *
 *  9-Apr-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added flags for TCP and MACH_NP debugging.
 *
 *  6-Jan-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Make the event structure smaller to make it easier to read from
 *	kernel debuggers.
 *
 * 16-Mar-87  Mike Accetta (mja) at Carnegie-Mellon University
 *	MACH:  made XPR_DEBUG definition conditional on MACH
 *	since the routines invoked under it won't link without MACH.
 *	[ V5.1(F7) ]
 */
/*
 * Include file for xpr circular buffer silent tracing.  
 *
 */

/*
 * If the kernel flag XPRDEBUG is set, the XPR macro is enabled.  The 
 * macro should be invoked something like the following:
 *	XPR(XPR_SYSCALLS, ("syscall: %d, 0x%x\n", syscallno, arg1));
 * which will expand into the following code:
 *	if (xprflags & XPR_SYSCALLS)
 *		xpr("syscall: %d, 0x%x\n", syscallno, arg1);
 * Xpr will log the pointer to the printf string and up to 6 arguments,
 * along with a timestamp and cpuinfo (for multi-processor systems), into
 * a circular buffer.  The actual printf processing is delayed until after
 * the buffer has been collected.  It is assumed that the text/data segments
 * of the kernel can easily be reconstructed in a post-processor which
 * performs the printf processing.
 *
 * If the XPRDEBUG compilation switch is not set, the XPR macro expands 
 * to nothing.
 */

#ifndef	_KERN_XPR_H_
#define _KERN_XPR_H_

#ifdef	KERNEL_BUILD
#import <uxpr.h>
#import <xpr_debug.h>
#else	/* KERNEL_BUILD */
#import <mach/features.h>
#endif	/* KERNEL_BUILD */

#import <machdep/machine/xpr.h>

#if	XPR_DEBUG

#ifndef	__ASSEMBLER__

#if	UXPR
/*
 * Use IODevice-style xpr's. Kernel internal xpr's use index 1 into 
 * xprMask[].
 */
#ifndef	DDM_DEBUG
#define DDM_DEBUG	XPR_DEBUG
#endif	/* DDM_DEBUG */
#import <driverkit/debugging.h>

#define XPR_KERNEL_INDEX	1

#define XPR(flags, xprargs) { 				\
	if(IODDMMasks[XPR_KERNEL_INDEX] & flags) { 	\
		IOAddDDMEntry xprargs;			\
	}						\
}

#else	/* UXPR */

/*
 * The m68k way. Maybe this will be deleted eventually.
 */
extern unsigned int xprflags;
extern unsigned int xprwrap;
#define XPR(flags,xprargs) { if(xprflags&flags) xpr xprargs; }

#endif	/* UXPR */

#endif	/* __ASSEMBLER__ */

/*
 * Mach Kernel flags.
 */
#define XPR_SYSCALLS		(1 << 0)
#define XPR_TRAPS		(1 << 1)
#define XPR_SCHED		(1 << 2)
#define XPR_NPTCP		(1 << 3)
#define XPR_NP			(1 << 4)
#define XPR_TCP			(1 << 5)

#define XPR_VM_OBJECT		(1 << 8)
#define XPR_VM_OBJECT_CACHE	(1 << 9)
#define XPR_VM_PAGE		(1 << 10)
#define XPR_VM_PAGEOUT		(1 << 11)
#define XPR_MEMORY_OBJECT	(1 << 12)
#define XPR_VM_FAULT		(1 << 13)
#define XPR_VNODE_PAGER		(1 << 14)
#define XPR_VNODE_PAGER_DATA	(1 << 15)

/*
 * Other machine-independent flags.
 */
#define	XPR_FS			(1 << 16)
#define XPR_TIMER		(1 << 17)
#define	XPR_ALLOC		(1 << 18)
#define	XPR_LDD			(1 << 19)
#define	XPR_MEAS		(1 << 20)

#else	/* XPR_DEBUG */

#define XPR(flags,xprargs)

#endif	/* XPR_DEBUG */

#ifndef	__ASSEMBLER__
struct xprbuf {
	char 	*msg;
	int	arg1,arg2,arg3,arg4,arg5;
	int	timestamp;
	int	cpuinfo;
};
#endif	/* __ASSEMBLER__ */
#endif	/* _KERN_XPR_H_ */
