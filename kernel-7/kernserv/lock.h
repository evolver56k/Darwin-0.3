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
 * 11-Jul-91  Gregg Kellogg (gk) at NeXT
 *	Initial version.
 */
/*
 *	File:	kernserv/lock.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Exported locking primitives definitions
 *
 */

#ifdef	KERNEL_PRIVATE
#warning Obsolete header file: <kernserv/lock.h>. use <kern/lock.h> \
	instead; or better yet, just say no!!
#import <kern/lock.h>

#else	/* KERNEL_PRIVATE */

#ifndef	_KERN_LOCK_H_
#define _KERN_LOCK_H_

#import <mach/boolean.h>

/*
 *	A simple spin lock.
 */

#import	<mach/machine/simple_lock.h>

typedef void	*lock_t;

/* Sleep locks must work even if no multiprocessing */

extern lock_t		lock_alloc();
extern void		lock_free();
extern void		lock_init();
extern void		lock_sleepable();
extern void		lock_write();
extern void		lock_read();
extern void		lock_done();
extern boolean_t	lock_read_to_write();
extern void		lock_write_to_read();
extern boolean_t	lock_try_write();
extern boolean_t	lock_try_read();
extern boolean_t	lock_try_read_to_write();

#define lock_read_done(l)	lock_done(l)
#define lock_write_done(l)	lock_done(l)

extern void		lock_set_recursive();
extern void		lock_clear_recursive();

#endif	/* _KERN_LOCK_H_ */

#endif	/* KERNEL_PRIVATE */
