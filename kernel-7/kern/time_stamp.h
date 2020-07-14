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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifndef	_KERN_TIME_STAMP_H_
#define _KERN_TIME_STAMP_H_

#import <mach/machine/kern_return.h>
#import <machdep/machine/time_stamp.h>
/*
 *	time_stamp.h -- definitions for low-overhead timestamps.
 */

struct tsval {
	unsigned	low_val;	/* least significant word */
	unsigned	high_val;	/* most significant word */
};

/*
 *	Format definitions.
 */

#ifndef	TS_FORMAT
/*
 *	Default case - Just return a tick count for machines that
 *	don't support or haven't implemented this.  Assume 100Hz ticks.
 *
 *	low_val - Always 0.
 *	high_val - tick count.
 */
#define	TS_FORMAT	1

#if	KERNEL 
extern unsigned	ts_tick_count;
#endif	/* KERNEL */
#endif	/* TS_FORMAT */

extern kern_return_t kern_timestamp(struct tsval *);

/*
 *	List of all format definitions for convert_ts_to_tv.
 */

#define	TS_FORMAT_DEFAULT	1
#define TS_FORMAT_MMAX		2
#define TS_FORMAT_NeXT		3

#endif	/* _KERN_TIME_STAMP_H_ */
