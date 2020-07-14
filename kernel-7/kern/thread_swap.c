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
/*
 *
 *	File:	kern/thread_swap.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1987
 *
 *	Mach thread swapper:
 *		Find idle threads to swap, freeing up kernel stack resources
 *		at the expense of allowing them to execute.
 *
 *		Swap in threads that need to be run.  This is done here
 *		by the swapper thread since it cannot be done (in general)
 *		when the kernel tries to place a thread on a run queue.
 *
 *	Note: The act of swapping a thread in Mach does not mean that
 *	its memory gets forcibly swapped to secondary storage.  The memory
 *	for the task corresponding to a swapped thread is paged out
 *	through the normal paging mechanism.
 *
 */

#include <ipc/ipc_kmsg.h>
#include <kern/counters.h>
#include <kern/thread.h>
#include <kern/lock.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <mach/vm_param.h>
#include <kern/sched_prim.h>
#include <kern/processor.h>
#include <kern/thread_swap.h>
#include <machine/machspl.h>		/* for splsched */



queue_head_t		swapin_queue;
decl_simple_lock_data(,	swapper_lock_data)

#define swapper_lock()		simple_lock(&swapper_lock_data)
#define swapper_unlock()	simple_unlock(&swapper_lock_data)

/*
 *	swapper_init: [exported]
 *
 *	Initialize the swapper module.
 */
void swapper_init()
{
	queue_init(&swapin_queue);
	simple_lock_init(&swapper_lock_data);
}

/*
 *	thread_swapin: [exported]
 *
 *	Place the specified thread in the list of threads to swapin.  It
 *	is assumed that the thread is locked, therefore we are at splsched.
 *
 *	We don't bother with stack_alloc_try to optimize swapin;
 *	our callers have already tried that route.
 */

void thread_swapin(thread)
	thread_t	thread;
{
	switch (thread->state & TH_SWAP_STATE) {
	    case TH_SWAPPED:
		/*
		 *	Swapped out - queue for swapin thread.
		 */
		thread->state = (thread->state & ~TH_SWAP_STATE)
				| TH_SW_COMING_IN;
		swapper_lock();
		enqueue_tail(&swapin_queue, (queue_entry_t) thread);
		swapper_unlock();
		thread_wakeup((event_t) &swapin_queue);
		break;

	    case TH_SW_COMING_IN:
		/*
		 *	Already queued for swapin thread, or being
		 *	swapped in.
		 */
		break;

	    default:
		/*
		 *	Already swapped in.
		 */
		panic("thread_swapin");
	}
}

/*
 *	thread_doswapin:
 *
 *	Swapin the specified thread, if it should be runnable, then put
 *	it on a run queue.  No locks should be held on entry, as it is
 *	likely that this routine will sleep (waiting for stack allocation).
 */
void thread_doswapin(thread)
	register thread_t thread;
{
	spl_t	s;

	/*
	 *	Allocate the kernel stack.
	 */

	stack_alloc(thread, thread_continue);

	/*
	 *	Place on run queue.  
	 */

	s = splsched();
	thread_lock(thread);
	thread->state &= ~(TH_SWAPPED | TH_SW_COMING_IN);
	if (thread->state & TH_RUN)
		thread_setrun(thread, TRUE);
	thread_unlock(thread);
	(void) splx(s);
}

/*
 *	swapin_thread: [exported]
 *
 *	This procedure executes as a kernel thread.  Threads that need to
 *	be swapped in are swapped in by this thread.
 */
void swapin_thread_continue()
{
	for (;;) {
		register thread_t thread;
		spl_t s;

		s = splsched();
		swapper_lock();

		while ((thread = (thread_t) dequeue_head(&swapin_queue))
							!= THREAD_NULL) {
			swapper_unlock();
			(void) splx(s);

			thread_doswapin(thread);		/* may block */

			s = splsched();
			swapper_lock();
		}

		assert_wait((event_t) &swapin_queue, FALSE);
		swapper_unlock();
		(void) splx(s);
		counter(c_swapin_thread_block++);
		thread_block_with_continuation(swapin_thread_continue);
	}
}

void swapin_thread()
{
	stack_privilege(current_thread());
	current_thread()->vm_privilege = TRUE;

	swapin_thread_continue();
	/*NOTREACHED*/
}
