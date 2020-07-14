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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * EventShmemLock.h -	Shared memory area locks for use between the
 *			WindowServer and the Event Driver.
 *
 * HISTORY
 * 30 Nov   1992    Ben Fathi (benf@next.com)
 *      Ported to m98k.
 *
 * 29 April 1992    Mike Paquette at NeXT
 *      Created. 
 *
 * Multiprocessor locks used within the shared memory area between the
 * kernel and event system.  These must work in both user and kernel mode.
 * The locks are defined in an include file so they get exported to the local
 * include file area.
 */
#ifdef KERNEL_PRIVATE
#import	<architecture/ppc/asm_help.h>
#import	<architecture/ppc/pseudo_inst.h>

/*
 *	void
 *	ev_lock(p)
 *		register int *p;
 *
 *	Lock the lock pointed to by p.  Spin (possibly forever) until
 *		the lock is available.  Test and test and set logic used.
 */
	TEXT

LEAF(_ev_lock)
	li	a6,1		// lock value
9:
	sync
	lwarx	a7,0,a0		// read the lock
	cmpwi	cr0,a7,0	// is it busy?
	bne-	9b		// yes, spin
	sync
	stwcx.	a6,0,a0		// try to get the lock
	bne-	9b		// failed, try again
	isync
	blr			// got it, return
END(_ev_lock)

/*
 *	void
 *	spin_unlock(p)
 *		int *p;
 *
 *	Unlock the lock pointed to by p.
 */

LEAF(_ev_unlock)
	sync
	li	a7,0
	stw	a7,0(a0)
	blr
END(_ev_unlock)


/*
 *	ev_try_lock(p)
 *		int *p;
 *
 *	Try to lock p.  Return TRUE if successful in obtaining lock.
 */

LEAF(_ev_try_lock)
	li	a6,1		// lock value
8:
	sync
	lwarx	a7,0,a0		// read the lock
	cmpwi	cr0,a7,0	// is it busy?
	bne-	9f		// yes, give up
	sync
	stwcx.	a6,0,a0		// try to get the lock
	bne-	8b		// failed, try again
	li	a0,1		// return TRUE
	isync
	blr
9:
	li	a0,0		// return FALSE
	blr
END(_ev_try_lock)
#endif /* KERNEL_PRIVATE */
