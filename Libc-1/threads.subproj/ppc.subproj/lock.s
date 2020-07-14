/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	libc/threads/m98k/lock.s
 *
 *	Cthreads locking routines.
 *
 * HISTORY
 * 25-Nov-92	Derek B Clegg (dclegg@next.com)
 *	Ported to m98k.
 * 14-Jan-92  Peter King (king@next.com)
 *	Created.
 *
 * NOTE: A lock is held if it is any non-zero value, a lock is unlocked
 * only if it is zero.
 *
 * Boolean_t valued functions defined here return only 0 or 1.
 */
#import	<architecture/ppc/asm_help.h>
#import	<architecture/ppc/pseudo_inst.h>

/* void spin_lock(int *p);
 *
 * Lock the lock pointed to by `p'.  Spin (possibly forever) until
 * the lock is available.  Test and test and set logic used.
 */

.text

LEAF(_spin_lock)
	addi    r4,0,0x1	// Lock value
1:
	lwarx   r5,0,r3		// Read the lock
	sync			// Fix for 3.2 and older CPUs
	stwcx.  r4,0,r3		// Try to lock the lock
	bne-    1b		// Lost reservation, try again
	cmpwi   r5,0x0		// Is it busy?
	bne-    1b		// Yes, spin
	isync			// Workaround some buggy CPUs
	blr			// Got it, return
END(_spin_lock)

/* void spin_unlock(int *p);
 *
 *	Unlock the lock pointed to by p.
 */
LEAF(_spin_unlock)
	sync
	li32	r4,0
	stw	r4,0(r3)
	blr
END(_spin_unlock)

/* boolean_t mutex_try_lock(int *p);
 *
 *	Try to lock p.  Return TRUE if successful in obtaining lock.
 */
LEAF(_mutex_try_lock)
	addi    r4,0,0x1	// Lock value
1:
	lwarx   r5,0,r3		// Read the lock
	sync			// Fix for 3.2 and older CPUs
	stwcx.  r4,0,r3		// Try to lock the lock
	bne-    1b		// Lost reservation, try again
	isync			// Workaround some buggy CPUs
	xori    r3,r5,0x1	// Return opposite of what read
	blr
END(_mutex_try_lock)

/* boolean_t mutex_unlock_try(int *p);
 *
 *	Unlock the lock pointed to by p.  Return non-zero if the
 *	lock was already unlocked.
 */
LEAF(_mutex_unlock_try)
	sync			// Ensure critical section complete
	addi    r4,0,0x0	// Unlock value
1:
	lwarx   r5,0,r3		// Read the lock
	sync			// Fix for 3.2 and older CPUs
	stwcx.  r4,0,r3		// Try to unlock the lock
	bne-    1b		// Lost reservation, try again
	isync			// Workaround some buggy CPUs
	xori    r3,r5,0x1	// Return opposite of what read
	blr
END(_mutex_unlock_try)

#if 0
/* void cthread_set_self(cproc_t p)
 *
 *	Traps to kernel to set's thread state "user_value"
 */
LEAF(_cthread_set_self)
        mr	r13, r3	
	blr
END(_cthread_set_self)

/* ur_cthread_t ur_cthread_self(void)
 *
 * Traps to kernel to return thread state "user_value"
 */
LEAF(_ur_cthread_self)
	mr	r3, r13
	blr
END(_ur_cthread_self)
#endif

/* int cthread_sp(void);
 *
 * Returns current stack pointer
 */
LEAF(_cthread_sp)
	mr	r3,r1
	blr
END(_cthread_sp)
