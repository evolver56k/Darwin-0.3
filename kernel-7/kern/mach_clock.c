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
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *	File:	clock_prim.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1986
 *
 *	Clock primitives.
 */
#import <mach/features.h>

#include <mach/boolean.h>
#include <mach/machine.h>
#include <mach/time_value.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <kern/counters.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/host.h>
#include <kern/lock.h>
#include <kern/mach_param.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/time_out.h>
#include <kern/time_stamp.h>
#include <vm/vm_kern.h>
#include <machine/mach_param.h>	/* HZ */
#include <machine/machspl.h>



extern void	thread_quantum_update();

int		hz = HZ;		/* number of ticks per second */
int		tick = (1000000 / HZ);	/* number of usec per tick */

tvalspec_t	tick_stamp;		/* timestamp of the last 'tick' */

/*
 *	Handle clock interrupts.
 *
 *	The clock interrupt is assumed to be called at a (more or less)
 *	constant rate.  The rate must be identical on all CPUS (XXX - fix).
 *
 *	Usec is the number of microseconds that have elapsed since the
 *	last clock tick.  It may be constant or computed, depending on
 *	the accuracy of the hardware clock.
 *
 */
void clock_interrupt(usec, usermode, basepri)
	register int	usec;		/* microseconds per tick */
	boolean_t	usermode;	/* executing user code */
	boolean_t	basepri;	/* at base priority */
{
	register int		my_cpu = cpu_number();
	register thread_t	thread = current_thread();

	counter(c_clock_ticks++);
	counter(c_threads_total += c_threads_current);
	counter(c_stacks_total += c_stacks_current);

#if	STAT_TIME
	/*
	 *	Increment the thread time, if using
	 *	statistical timing.
	 */
	if (usermode) {
	    timer_bump(&thread->user_timer, usec);
	}
	else {
	    timer_bump(&thread->system_timer, usec);
	}
#endif	STAT_TIME

	/*
	 *	Increment the CPU time statistics.
	 */
	{
	    register int	state;

	    if (usermode)
		state = CPU_STATE_USER;
	    else if (!cpu_idle(my_cpu))
		state = CPU_STATE_SYSTEM;
	    else
		state = CPU_STATE_IDLE;

	    machine_slot[my_cpu].cpu_ticks[state]++;

	    /*
	     *	Adjust the thread's priority and check for
	     *	quantum expiration.
	     */

	    if (!(thread->state & TH_IDLE))
		thread_quantum_update(my_cpu, thread, 1, state);
	}

	if (my_cpu == master_cpu) {

	    /*
	     *	Perform scheduler housekeeping activities.
	     */
	    recompute_priorities();

	    /*
	     *	Save a timestamp for this clock tick.  This
	     *	is used by set_timeout(), et al. to apply
	     *	appropriate rounding to timeout intervals.
	     *	This is really a hack which assumes that
	     *	'timer interrupts' have the same period and
	     *	phase as a clock tick.
	     */
	    tick_stamp = clock_get_counter(System);

#if	TS_FORMAT == 1
	    /*
	     *	Increment the tick count for the timestamping routine.
	     */
	    ts_tick_count++;
#endif	TS_FORMAT == 1

	}
}

#if	SIMPLE_CLOCK

int
sched_usec_elapsed(void)
{
	static tvalspec_t	sched_stamp;
	tvalspec_t		delta_stamp,
				new_stamp = clock_get_counter(System);
	int			new_usec;

	delta_stamp = new_stamp;
	SUB_TVALSPEC(&delta_stamp, &sched_stamp);
	new_usec = (delta_stamp.tv_sec * USEC_PER_SEC) +
			(delta_stamp.tv_nsec / NSEC_PER_USEC);
	sched_stamp = new_stamp;
	
	return (new_usec);
}
#endif	/* SIMPLE_CLOCK */

tvalspec_t
ticks_to_tvalspec(
    	unsigned int	ticks)
{
	tvalspec_t		result;
    
	result.tv_sec = ticks / TICKS_PER_SEC;
	result.tv_nsec = (ticks % TICKS_PER_SEC) * NSEC_PER_TICK;
    
	return (result);
}

decl_simple_lock_data(static,	timer_lock)	/* lock for ... */

static void
service_timer(
	timer_elt_t	telt)
{
	spl_t		s;
	int		(*fcn)();
	char		*param;	
	
	s = splsched();
	simple_lock(&timer_lock);
	
	fcn = telt->fcn;
	param = telt->param;

	telt->set = TELT_UNSET;
	
	simple_unlock(&timer_lock);
	splx(s);
	
	(*fcn)(param);
}

static __inline__ void
init_timeout_element(
	timer_elt_t	telt)
{
	telt->call.func = (thread_call_func_t)service_timer;
	telt->call.spec_proto = telt;
	telt->call.status = IDLE;
}

/*
 *	Set timeout.
 *
 *	Parameters:
 *		telt	 timer element.  Function and param are already set.
 *		interval time-out interval, in hz.
 */
void set_timeout(telt, interval)
	register timer_elt_t	telt;	/* already loaded */
	register unsigned int	interval;
{
	spl_t		s;
	tvalspec_t	deadline;

	s = splsched();
	simple_lock(&timer_lock);

	if (telt->call.spec_proto != telt)
	    init_timeout_element(telt);

	deadline = ticks_to_tvalspec(interval);
	ADD_TVALSPEC(&deadline, &tick_stamp);
	
	thread_call_enter_delayed(&telt->call, deadline);

	telt->set = TELT_SET;

	simple_unlock(&timer_lock);
	splx(s);
}

boolean_t reset_timeout(telt)
	register timer_elt_t	telt;
{
	spl_t		s;

	s = splsched();
	simple_lock(&timer_lock);

	if (telt->set) {
	    thread_call_cancel(&telt->call);

	    telt->set = TELT_UNSET;

	    simple_unlock(&timer_lock);
	    splx(s);
	    return TRUE;
	}
	else {
	    simple_unlock(&timer_lock);
	    splx(s);
	    return FALSE;
	}
}

void init_timeout()
{
	simple_lock_init(&timer_lock);
}
