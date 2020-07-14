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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	vm/vm_pageout.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr.
 *
 *	Header file for pageout daemon.
 *
 ************************************************************************
 * HISTORY
 * 19-Jan-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Declare vm_pageout_page().
 *
 * 29-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Delinted.
 *
 * 15-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Converted to new include technology.
 *
 * 17-Feb-87  David Golub (dbg) at Carnegie-Mellon University
 *	Added lock (to avoid losing wakeups).  Moved VM_WAIT from
 *	vm_page.h to this file.
 *
 *  9-Mar-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 ************************************************************************
 */

#import <kern/lock.h>
#import <kern/sched_prim.h>

/*
 *	Exported data structures.
 */

extern int	vm_pages_needed;	/* should be some "event" structure */
extern simple_lock_data_t	vm_pages_needed_lock;


/*
 *	Exported routines.
 */

#if	MACH_XP
void	vm_pageout_page();
#endif	/* MACH_XP */

/*
 *	Signal pageout-daemon and wait for it.
 */

#define	VM_WAIT		{ \
			simple_lock(&vm_pages_needed_lock); \
			thread_wakeup(&vm_pages_needed); \
			thread_sleep(&vm_page_free_count, \
				&vm_pages_needed_lock, FALSE); \
			}
