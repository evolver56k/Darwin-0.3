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
 * Revision 2.3  89/03/09  20:14:51  rpd
 * 	More cleanup.
 * 
 * Revision 2.2  89/02/25  18:07:31  gm0w
 * 	Kernel code cleanup.
 * 	Put entire file under #indef KERNEL.
 * 	[89/02/15            mrt]
 * 
 *  9-Oct-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Define unix_reset for longjmp/setjmp reset.
 *
 * 21-Sep-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifndef	_KERN_PARALLEL_H_
#define _KERN_PARALLEL_H_

#ifdef	KERNEL_BUILD
#import <cpus.h>
#else	/* KERNEL_BUILD */
#import <mach/features.h>
#endif	/* KERNEL_BUILD */

#if	NCPUS > 1

#define unix_master()  _unix_master()
#define unix_release() _unix_release()
#define unix_reset()   _unix_reset()
extern void _unix_master(), _unix_release(), _unix_reset();

#else	/* NCPUS > 1 */

#define unix_master()
#define unix_release()
#define unix_reset()

#endif	/* NCPUS > 1 */

#endif	/* _KERN_PARALLEL_H_ */
