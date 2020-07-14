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
 * Copyright (c) 1993-1987 Carnegie Mellon University
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
/*
 *	File:	kern/lock.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Locking primitives implementation
 */

#include <cpus.h>

#include <kern/lock.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>


#if (DIAGNOSTIC && defined(__ppc__))
void
simple_lock(
    simple_lock_t       slock)
{
	real_simple_lock(slock); /* inline in mach/{ppc,i386}/simple_lock.h */
	slock->lock_data[1] = (int)(current_thread());
}
#endif /* DIAGNOSTIC */

#if	NCPUS > 1

/*
 *	Module:		lock
 *	Function:
 *		Provide reader/writer sychronization.
 *	Implementation:
 *		Simple interlock on a bit.  Readers first interlock,
 *		increment the reader count, then let go.  Writers hold
 *		the interlock (thus preventing further readers), and
 *		wait for already-accepted readers to go away.
 */

/*
 *	The simple-lock routines are the primitives out of which
 *	the lock package is built.  The implementation is left
 *	to the machine-dependent code.
 */

#ifdef	notdef
/*
 *	A sample implementation of simple locks.
 *	assumes:
 *		boolean_t test_and_set(boolean_t *)
 *			indivisibly sets the boolean to TRUE
 *			and returns its old value
 *		and that setting a boolean to FALSE is indivisible.
 */
/*
 *	simple_lock_init initializes a simple lock.  A simple lock
 *	may only be used for exclusive locks.
 */

void simple_lock_init(simple_lock_t l)
{
	*(boolean_t *)l = FALSE;
}

void simple_lock(simple_lock_t l)
{
	while (test_and_set((boolean_t *)l))
		continue;
}

void simple_unlock(simple_lock_t l)
{
	*(boolean_t *)l = FALSE;
}

boolean_t simple_lock_try(simple_lock_t l)
{
    	return (!test_and_set((boolean_t *)l));
}
#endif	/* notdef */
#endif	/* NCPUS > 1 */

#if	NCPUS > 1
int lock_wait_time = 100;
#else	/* NCPUS > 1 */

	/*
	 * 	It is silly to spin on a uni-processor as if we
	 *	thought something magical would happen to the
	 *	want_write bit while we are executing.
	 */
int lock_wait_time = 0;
#endif	/* NCPUS > 1 */

simple_lock_t simple_lock_alloc(void)
{
#if	MACH_SLOCKS
	return (simple_lock_t)kalloc(sizeof(simple_lock_data_t));
#else	MACH_SLOCKS
	return 0;
#endif	MACH_SLOCKS
}

void simple_lock_free(
	simple_lock_t l)
{
#if	MACH_SLOCKS
	kfree(l, sizeof(*l));
#endif	MACH_SLOCKS
}

/*
 *	Routine:	lock_alloc
 *	Function:
 *		Allocate a lock_t data structure.  Used by loadable
 *		servers that can't allocate a lock statically.
 */
lock_t lock_alloc(void)
{
	return (lock_t)kalloc(sizeof(lock_data_t));
}

/*
 *	Routine:	lock_free
 *	Function:
 *		Free a lock allocated by lock_alloc()
 */
void lock_free(
	lock_t	l)
{
	kfree(l, sizeof(lock_data_t));
}

/*
 *	Routine:	lock_init
 *	Function:
 *		Initialize a lock; required before use.
 *		Note that clients declare the "struct lock"
 *		variables and then initialize them, rather
 *		than getting a new one from this module.
 */
void lock_init(
	lock_t		l,
	boolean_t	can_sleep)
{
	bzero((char *)l, sizeof(lock_data_t));
	simple_lock_init(&l->interlock);
	l->want_write = FALSE;
	l->want_upgrade = FALSE;
	l->read_count = 0;
	l->can_sleep = can_sleep;
	l->thread = (struct thread *)-1;	/* XXX */
	l->recursion_depth = 0;
}

void lock_sleepable(
	lock_t		l,
	boolean_t	can_sleep)
{
	simple_lock(&l->interlock);
	l->can_sleep = can_sleep;
	simple_unlock(&l->interlock);
}


/*
 *	Sleep locks.  These use the same data structure and algorithm
 *	as the spin locks, but the process sleeps while it is waiting
 *	for the lock.  These work on uniprocessor systems.
 */

void lock_write(
	register lock_t	l)
{
	register int	i;

	simple_lock(&l->interlock);

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock.
		 */
		l->recursion_depth++;
		simple_unlock(&l->interlock);
		return;
	}

	/*
	 *	Try to acquire the want_write bit.
	 */
	while (l->want_write) {
		if ((i = lock_wait_time) > 0) {
			simple_unlock(&l->interlock);
			while (--i > 0 && l->want_write)
				continue;
			simple_lock(&l->interlock);
		}

		if (l->can_sleep && l->want_write) {
			l->waiting = TRUE;
			thread_sleep(l,
				simple_lock_addr(l->interlock), FALSE);
			simple_lock(&l->interlock);
		}
	}
	l->want_write = TRUE;

	/* Wait for readers (and upgrades) to finish */

	while ((l->read_count != 0) || l->want_upgrade) {
		if ((i = lock_wait_time) > 0) {
			simple_unlock(&l->interlock);
			while (--i > 0 && (l->read_count != 0 ||
					l->want_upgrade))
				continue;
			simple_lock(&l->interlock);
		}

		if (l->can_sleep && (l->read_count != 0 || l->want_upgrade)) {
			l->waiting = TRUE;
			thread_sleep(l,
				simple_lock_addr(l->interlock), FALSE);
			simple_lock(&l->interlock);
		}
	}
	simple_unlock(&l->interlock);
}

void lock_done(
	register lock_t	l)
{
	simple_lock(&l->interlock);

	if (l->read_count != 0)
		l->read_count--;
	else
	if (l->recursion_depth != 0)
		l->recursion_depth--;
	else
	if (l->want_upgrade)
	 	l->want_upgrade = FALSE;
	else
	 	l->want_write = FALSE;

	/*
	 *	There is no reason to wakeup a waiting thread
	 *	if the read-count is non-zero.  Consider:
	 *		we must be dropping a read lock
	 *		threads are waiting only if one wants a write lock
	 *		if there are still readers, they can't proceed
	 */

	if (l->waiting && (l->read_count == 0)) {
		l->waiting = FALSE;
		thread_wakeup(l);
	}

	simple_unlock(&l->interlock);
}

void lock_read(
	register lock_t	l)
{
	register int	i;

	simple_lock(&l->interlock);

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock.
		 */
		l->read_count++;
		simple_unlock(&l->interlock);
		return;
	}

	while (l->want_write || l->want_upgrade) {
		if ((i = lock_wait_time) > 0) {
			simple_unlock(&l->interlock);
			while (--i > 0 && (l->want_write || l->want_upgrade))
				continue;
			simple_lock(&l->interlock);
		}

		if (l->can_sleep && (l->want_write || l->want_upgrade)) {
			l->waiting = TRUE;
			thread_sleep(l,
				simple_lock_addr(l->interlock), FALSE);
			simple_lock(&l->interlock);
		}
	}

	l->read_count++;
	simple_unlock(&l->interlock);
}

/*
 *	Routine:	lock_read_to_write
 *	Function:
 *		Improves a read-only lock to one with
 *		write permission.  If another reader has
 *		already requested an upgrade to a write lock,
 *		no lock is held upon return.
 *
 *		Returns TRUE if the upgrade *failed*.
 */
boolean_t lock_read_to_write(
	register lock_t	l)
{
	register int	i;

	simple_lock(&l->interlock);

	l->read_count--;

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock.
		 */
		l->recursion_depth++;
		simple_unlock(&l->interlock);
		return(FALSE);
	}

	if (l->want_upgrade) {
		/*
		 *	Someone else has requested upgrade.
		 *	Since we've released a read lock, wake
		 *	him up.
		 */
		if (l->waiting && (l->read_count == 0)) {
			l->waiting = FALSE;
			thread_wakeup(l);
		}

		simple_unlock(&l->interlock);
		return TRUE;
	}

	l->want_upgrade = TRUE;

	while (l->read_count != 0) {
		if ((i = lock_wait_time) > 0) {
			simple_unlock(&l->interlock);
			while (--i > 0 && l->read_count != 0)
				continue;
			simple_lock(&l->interlock);
		}

		if (l->can_sleep && l->read_count != 0) {
			l->waiting = TRUE;
			thread_sleep(l,
				simple_lock_addr(l->interlock), FALSE);
			simple_lock(&l->interlock);
		}
	}

	simple_unlock(&l->interlock);
	return FALSE;
}

void lock_write_to_read(
	register lock_t	l)
{
	simple_lock(&l->interlock);

	l->read_count++;
	if (l->recursion_depth != 0)
		l->recursion_depth--;
	else
	if (l->want_upgrade)
		l->want_upgrade = FALSE;
	else
	 	l->want_write = FALSE;

	if (l->waiting) {
		l->waiting = FALSE;
		thread_wakeup(l);
	}

	simple_unlock(&l->interlock);
}


/*
 *	Routine:	lock_try_write
 *	Function:
 *		Tries to get a write lock.
 *
 *		Returns FALSE if the lock is not held on return.
 */

boolean_t lock_try_write(
	register lock_t	l)
{
	simple_lock(&l->interlock);

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock
		 */
		l->recursion_depth++;
		simple_unlock(&l->interlock);
		return TRUE;
	}

	if (l->want_write || l->want_upgrade || l->read_count) {
		/*
		 *	Can't get lock.
		 */
		simple_unlock(&l->interlock);
		return FALSE;
	}

	/*
	 *	Have lock.
	 */

	l->want_write = TRUE;
	simple_unlock(&l->interlock);
	return TRUE;
}

/*
 *	Routine:	lock_try_read
 *	Function:
 *		Tries to get a read lock.
 *
 *		Returns FALSE if the lock is not held on return.
 */

boolean_t lock_try_read(
	register lock_t	l)
{
	simple_lock(&l->interlock);

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock
		 */
		l->read_count++;
		simple_unlock(&l->interlock);
		return TRUE;
	}

	if (l->want_write || l->want_upgrade) {
		simple_unlock(&l->interlock);
		return FALSE;
	}

	l->read_count++;
	simple_unlock(&l->interlock);
	return TRUE;
}

/*
 *	Routine:	lock_try_read_to_write
 *	Function:
 *		Improves a read-only lock to one with
 *		write permission.  If another reader has
 *		already requested an upgrade to a write lock,
 *		the read lock is still held upon return.
 *
 *		Returns FALSE if the upgrade *failed*.
 */
boolean_t lock_try_read_to_write(
	register lock_t	l)
{
	simple_lock(&l->interlock);

	if (l->thread == current_thread()) {
		/*
		 *	Recursive lock
		 */
		l->read_count--;
		l->recursion_depth++;
		simple_unlock(&l->interlock);
		return TRUE;
	}

	if (l->want_upgrade) {
		simple_unlock(&l->interlock);
		return FALSE;
	}
	l->want_upgrade = TRUE;
	l->read_count--;

	while (l->read_count != 0) {
		l->waiting = TRUE;
		thread_sleep(l,
			simple_lock_addr(l->interlock), FALSE);
		simple_lock(&l->interlock);
	}

	simple_unlock(&l->interlock);
	return TRUE;
}

/*
 *	Allow a process that has a lock for write to acquire it
 *	recursively (for read, write, or update).
 */
void lock_set_recursive(
	lock_t		l)
{
	simple_lock(&l->interlock);
	if (!l->want_write) {
		panic("lock_set_recursive: don't have write lock");
	}
	l->thread = current_thread();
	simple_unlock(&l->interlock);
}

/*
 *	Prevent a lock from being re-acquired.
 */
void lock_clear_recursive(
	lock_t		l)
{
	simple_lock(&l->interlock);
	if (l->thread != current_thread()) {
		panic("lock_clear_recursive: wrong thread");
	}
	if (l->recursion_depth == 0)
		l->thread = (struct thread *)-1;	/* XXX */
	simple_unlock(&l->interlock);
}
