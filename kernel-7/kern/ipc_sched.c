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
 * Copyright (c) 1993, 1992,1991,1990 Carnegie Mellon University
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

#include <cpus.h>
#include <mach_host.h>

#include <mach/message.h>
#include <kern/counters.h>
#include <kern/cpu_number.h>
#include <kern/lock.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/processor.h>
#include <kern/time_out.h>
#include <kern/thread_swap.h>
#include <kern/ipc_sched.h>
#include <machine/machspl.h>	/* for splsched/splx */
#include <machine/pmap.h>

#import <kern/assert.h>


/*
 *	These functions really belong in kern/sched_prim.c.
 */

/*
 *	Routine:	thread_go
 *	Purpose:
 *		Start a thread running.
 *	Conditions:
 *		IPC locks may be held.
 */

void
thread_go(
	thread_t thread)
{
	int	state;
	spl_t	s;

	s = splsched();
	thread_lock(thread);

	reset_timeout_check(&thread->timer);

	state = thread->state;
	switch (state & TH_SCHED_STATE) {

	    case TH_WAIT | TH_SUSP | TH_UNINT:
	    case TH_WAIT	   | TH_UNINT:
	    case TH_WAIT:
		/*
		 *	Sleeping and not suspendable - put
		 *	on run queue.
		 */
		thread->state = (state &~ TH_WAIT) | TH_RUN;
		thread->wait_result = THREAD_AWAKENED;
		thread_setrun(thread, TRUE);
		break;

	    case	  TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT:
	    case TH_RUN | TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT	    | TH_UNINT:
	    case TH_RUN | TH_WAIT | TH_SUSP | TH_UNINT:
		/*
		 *	Either already running, or suspended.
		 */
		thread->state = state & ~TH_WAIT;
		thread->wait_result = THREAD_AWAKENED;
		break;

	    default:
		/*
		 *	Not waiting.
		 */
		break;
	}

	thread_unlock(thread);
	splx(s);
}

/*
 *	Routine:	thread_go_and_switch
 *	Purpose:
 *		Start a thread running.
 *	Conditions:
 *		No IPC locks held.
 */

void
thread_go_and_switch(
	continuation_t continuation,
	thread_t thread)
{
	int	state;
	spl_t	s;

	s = splsched();
	thread_lock(thread);

	reset_timeout_check(&thread->timer);

	state = thread->state;
	switch (state & TH_SCHED_STATE) {

	    case TH_WAIT | TH_SUSP | TH_UNINT:
	    case TH_WAIT	   | TH_UNINT:
	    case TH_WAIT:
		/*
		 *	Sleeping and not suspendable - put
		 *	on run queue.
		 */
		thread->state = (state &~ TH_WAIT) | TH_RUN;
		thread->wait_result = THREAD_AWAKENED;
		if ((thread->processor_set->idle_count > 0) ||
		    (thread->processor_set !=
		     current_thread()->processor_set)) {
		     	/*
			 *	Other cpus can/must run thread.
			 *	Put it on the run queues.
			 */
			thread_setrun(thread, TRUE);
			break;
		}
		else {
		    /*
		     *	Switch immediately to new thread.
		     */
		    thread_unlock(thread);
		    thread_run(continuation, thread);
		    splx(s);
		    return;
		}
		break;

	    case	  TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT:
	    case TH_RUN | TH_WAIT | TH_SUSP:
	    case TH_RUN | TH_WAIT	    | TH_UNINT:
	    case TH_RUN | TH_WAIT | TH_SUSP | TH_UNINT:
		/*
		 *	Either already running, or suspended.
		 */
		thread->state = state & ~TH_WAIT;
		thread->wait_result = THREAD_AWAKENED;
		break;

	    default:
		/*
		 *	Not waiting.
		 */
		break;
	}

	thread_unlock(thread);
	if (continuation != (void (*)()) 0) {
	    (void) spl0();
	    call_continuation(continuation);
	}
	splx(s);
}

/*
 *	Routine:	thread_will_wait
 *	Purpose:
 *		Assert that the thread intends to block.
 */

void
thread_will_wait(
	thread_t thread)
{
	spl_t	s;

	s = splsched();
	thread_lock(thread);

	assert(thread->wait_result = -1);	/* for later assertions */
	thread->state |= TH_WAIT;

	thread_unlock(thread);
	splx(s);
}

/*
 *	Routine:	thread_will_wait_with_timeout
 *	Purpose:
 *		Assert that the thread intends to block,
 *		with a timeout.
 */

void
thread_will_wait_with_timeout(
	thread_t thread,
	mach_msg_timeout_t msecs)
{
	natural_t ticks = convert_ipc_timeout_to_ticks(msecs);
	spl_t	s;

	s = splsched();
	thread_lock(thread);

	assert(thread->wait_result = -1);	/* for later assertions */
	thread->state |= TH_WAIT;

	if (ticks != 0 || msecs == 0)
		set_timeout(&thread->timer, ticks);

	thread_unlock(thread);
	splx(s);
}

#if	MACH_HOST
#define check_processor_set(thread)	\
	    (current_processor()->processor_set == (thread)->processor_set)
#else	/* MACH_HOST */
#define	check_processor_set(thread)	TRUE
#endif	/* MACH_HOST */

#if	NCPUS > 1
#define	check_bound_processor(thread) \
	    ((thread)->bound_processor == PROCESSOR_NULL || \
	     (thread)->bound_processor == current_processor())
#else	/* NCPUS > 1 */
#define	check_bound_processor(thread)	TRUE
#endif	/* NCPUS > 1 */

/*
 *	Routine:	thread_handoff
 *	Purpose:
 *		Switch to a new thread (new), leaving the current
 *		thread (old) blocked.  If successful, moves the
 *		kernel stack from old to new and returns as the
 *		new thread.  An explicit continuation for the old thread
 *		must be supplied.
 *
 *		NOTE:  Although we wakeup new, we don't set new->wait_result.
 *	Returns:
 *		TRUE if the handoff happened.
 */

boolean_t
thread_handoff(
	register thread_t old,
	register continuation_t continuation,
	register thread_t new)
{
	spl_t	s;

	assert(current_thread() == old);

	/*
	 *	XXX Dubious things here:
	 *	I don't check the idle_count on the processor set.
	 *	No scheduling priority or policy checks.
	 *	I assume the new thread is interruptible.
	 */

	s = splsched();
	thread_lock(new);

	/*
	 *	The first thing we must do is check the state
	 *	of the threads, to ensure we can handoff.
	 *	This check uses current_processor()->processor_set,
	 *	which we can read without locking.
	 */

	if ((old->stack_privilege == current_stack()) ||
	    (new->state != (TH_WAIT|TH_SWAPPED)) ||
	     !check_processor_set(new) ||
	     !check_bound_processor(new)) {
		thread_unlock(new);
		(void) splx(s);

		counter_always(c_thread_handoff_misses++);
		return FALSE;
	}

	reset_timeout_check(&new->timer);

	new->state = TH_RUN;
	thread_unlock(new);

#if	NCPUS > 1
	new->last_processor = current_processor();
#endif	/* NCPUS > 1 */

	ast_context(new, cpu_number());
	timer_switch(&new->system_timer);

	/*
	 *	stack_handoff is machine-dependent.  It does the
	 *	machine-dependent components of a context-switch, like
	 *	changing address spaces.  It updates active_threads.
	 */

	stack_handoff(old, new);

	/*
	 *	Now we must dispose of the old thread.
	 *	This is like thread_continue, except
	 *	that the old thread isn't waiting yet.
	 */

	thread_lock(old);
	old->swap_func = continuation;
	assert(old->wait_result = -1);		/* for later assertions */

	if (old->state == TH_RUN) {
		/*
		 *	This is our fast path.
		 */

		old->state = TH_WAIT|TH_SWAPPED;
	}
	else if (old->state == (TH_RUN|TH_SUSP)) {
		/*
		 *	Somebody is trying to suspend the thread.
		 */

		old->state = TH_WAIT|TH_SUSP|TH_SWAPPED;
		if (old->wake_active) {
			/*
			 *	Someone wants to know when the thread
			 *	really stops.
			 */
			old->wake_active = FALSE;
			thread_unlock(old);
			thread_wakeup((event_t)&old->wake_active);
			goto after_old_thread;
		}
	} else
		panic("thread_handoff");

	thread_unlock(old);
    after_old_thread:
	(void) splx(s);

	counter_always(c_thread_handoff_hits++);
	return TRUE;
}
