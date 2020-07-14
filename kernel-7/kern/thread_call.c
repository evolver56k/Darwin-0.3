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
 * Copyright (c) 1993, 1994, 1995 NeXT Computer, Inc.
 *
 * Thread-based callout module.
 *
 * 3 July 1993 ? at NeXT
 *	Created.
 */
 
#import <mach/mach_types.h>

#import <kern/sched_prim.h>
#import <kern/clock.h>

#import <kern/thread_call.h>
#import <kern/thread_call_private.h>

#include <kern/kdebug.h>

#define internal_call_num	768

static
struct _thread_call
	internal_call_storage[internal_call_num];

static
simple_lock_data_t
	thread_call_lock;

static
queue_head_t
	internal_call_free_queue,
	pending_call_queue, delayed_call_queue;

static int
	pending_call_num, active_call_num,
	call_thread_num;

#define call_thread_min		4
			
static boolean_t
	thread_call_initialized = FALSE;

static __inline__ _thread_call_t
	_internal_call_allocate(void);

static __inline__ _thread_call_t
thread_call_release(
	_thread_call_t		call
);

static __inline__ void
_pending_call_enqueue(
	_thread_call_t		call
),
_pending_call_dequeue(
	_thread_call_t		call
),
_delayed_call_enqueue(
	_thread_call_t		call
),
_delayed_call_dequeue(
	_thread_call_t		call
);

static void __inline__
_set_delayed_call_timer(
	_thread_call_t		call
);
					
static boolean_t
_remove_from_pending_queue(
	thread_call_func_t	func,
	thread_call_spec_t	spec,
	boolean_t		remove_all
),
_remove_from_delayed_queue(
	thread_call_func_t	func,
	thread_call_spec_t	spec,
	boolean_t		remove_all
);

static __inline__ void
	_call_thread_wake(void);

static void
	_call_thread(void),
	_call_activate_thread(void);

static void
_delayed_call_interrupt(
	tvalspec_t		timestamp
);

#define qe(x)		((queue_entry_t)(x))
#define TC(x)		((_thread_call_t)(x))

/*
 * Routine:	thread_call_initialized [public]
 *
 * Description:	Initialize this module, called
 *		early during system initialization.
 *
 * Preconditions:	None.
 *
 * Postconditions:	None.
 */

void
thread_call_init(void)
{
    _thread_call_t	call;

    if (thread_call_initialized)
    	panic("thread_call_init");

    simple_lock_init(&thread_call_lock);

    queue_init(&pending_call_queue);
    queue_init(&delayed_call_queue);

    queue_init(&internal_call_free_queue);
    for (
	    call = internal_call_storage;
	    call < &internal_call_storage[internal_call_num];
	    call++) {

	enqueue_tail(&internal_call_free_queue, qe(call));
    }

    (void) kernel_thread(kernel_task, _call_activate_thread, (void *)0);

    timer_set_expire_func(SystemWide, (timer_func_t)_delayed_call_interrupt);

    thread_call_initialized = TRUE;
}

/*
 * Routine:	_internal_call_allocate [private, inline]
 *
 * Purpose:	Allocate an internal callout entry.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__ _thread_call_t
_internal_call_allocate(void)
{
    _thread_call_t	call;
    
    if (queue_empty(&internal_call_free_queue))
    	panic("_internal_call_allocate");
	
    call = TC(dequeue_head(&internal_call_free_queue));
    
    return (call);
}

/*
 * Routine:	thread_call_release [private, inline]
 *
 * Purpose:	Release a callout entry which is no
 *		longer pending (or delayed).  Returns
 *		its argument for external entries, zero
 *		otherwise.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__
_thread_call_t
thread_call_release(
    _thread_call_t	call
)
{
    if (
	    call >= internal_call_storage
	&&
	    call < &internal_call_storage[internal_call_num]) {
    
	enqueue_tail(&internal_call_free_queue, qe(call)); call = 0;
    }
    
    return (call);
}

/*
 * Routine:	_pending_call_enqueue [private, inline]
 *
 * Purpose:	Place an entry at the end of the
 *		pending queue, to be executed soon.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__
void
_pending_call_enqueue(
    _thread_call_t	call
)
{
    enqueue_tail(&pending_call_queue, qe(call)); pending_call_num++;

    call->status = PENDING;
}

/*
 * Routine:	_pending_call_dequeue [private, inline]
 *
 * Purpose:	Remove an entry from the pending queue,
 *		effectively unscheduling it.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__
void
_pending_call_dequeue(
    _thread_call_t	call
)
{
    remqueue(&pending_call_queue, qe(call)); pending_call_num--;
    
    call->status = IDLE;
}

/*
 * Routine:	_delayed_call_enqueue [private, inline]
 *
 * Purpose:	Place an entry on the delayed queue,
 *		after existing entries with an earlier
 * 		(or identical) deadline.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__
void
_delayed_call_enqueue(
    _thread_call_t	call
)
{
    _thread_call_t	current;
    int			deadline_cmp;
    
    current = TC(queue_first(&delayed_call_queue));
    
    while (TRUE) {
    	if (
		queue_end(&delayed_call_queue, qe(current))
	    ||
		(deadline_cmp = CMP_TVALSPEC(&call->deadline,
						&current->deadline)) < 0) {
	    current = TC(queue_prev(qe(current)));
	    break;
	}
	else if (deadline_cmp == 0)
	    break;
	    
	current = TC(queue_next(qe(current)));
    }

    insque(qe(call), qe(current));
    
    call->status = DELAYED;
}

/*
 * Routine:	_delayed_call_dequeue [private, inline]
 *
 * Purpose:	Remove an entry from the delayed queue,
 *		effectively unscheduling it.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__
void
_delayed_call_dequeue(
    _thread_call_t	call
)
{
    remqueue(&delayed_call_queue, qe(call));
    
    call->status = IDLE;
}

/*
 * Routine:	_set_delayed_call_timer [private]
 *
 * Purpose:	Reset the timer so that it
 *		next expires when the entry is due.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static __inline__ void
_set_delayed_call_timer(
    _thread_call_t	call
)
{
    timer_set_deadline(SystemWide, call->deadline);
}

/*
 * Routine:	_remove_from_pending_queue [private]
 *
 * Purpose:	Remove the first (or all) matching
 *		entries	from the pending queue,
 *		effectively unscheduling them.
 *		Returns	whether any matching entries
 *		were found for the !removeAll case,
 *		FALSE otherwise.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static
boolean_t
_remove_from_pending_queue(
    thread_call_func_t	func,
    thread_call_spec_t	spec,
    boolean_t		remove_all
)
{
    boolean_t		call_removed = FALSE;
    _thread_call_t	call;
    
    call = TC(queue_first(&pending_call_queue));
    
    while (!queue_end(&pending_call_queue, qe(call))) {
    	if (
		call->func == func
	    &&
	    	call->spec == spec) {
	    _thread_call_t	next = TC(queue_next(qe(call)));
		
	    _pending_call_dequeue(call);
	    
	    thread_call_release(call);
	    
	    call_removed = TRUE;
	    if (!remove_all)
		break;
		
	    call = next;
	}
	else	
	    call = TC(queue_next(qe(call)));
    }
    
    return (!remove_all? call_removed : FALSE);
}

/*
 * Routine:	_remove_from_delayed_queue [private]
 *
 * Purpose:	Remove the first (or all) matching
 *		entries	from the delayed queue,
 *		effectively unscheduling them.
 *		Returns	whether any matching entries
 *		were found for the !removeAll case,
 *		FALSE otherwise.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	None.
 */

static
boolean_t
_remove_from_delayed_queue(
    thread_call_func_t	func,
    thread_call_spec_t	spec,
    boolean_t		remove_all
)
{
    boolean_t		call_removed = FALSE;
    _thread_call_t	call;
    
    call = TC(queue_first(&delayed_call_queue));
    
    while (!queue_end(&delayed_call_queue, qe(call))) {
    	if (
		call->func == func
	    &&
	    	call->spec == spec) {
	    _thread_call_t	next = TC(queue_next(qe(call)));
		
	    _delayed_call_dequeue(call);
	    
	    thread_call_release(call);
	    
	    call_removed = TRUE;
	    if (!remove_all)
		break;
		
	    call = next;
	}
	else	
	    call = TC(queue_next(qe(call)));
    }
    
    return (!remove_all? call_removed : FALSE);
}

tvalspec_t
deadline_from_interval(
    tvalspec_t		interval
)
{
    tvalspec_t		result = clock_get_counter(System);

    ADD_TVALSPEC(&result, &interval);

    return (result);
}

/*
 * Routine:	thread_call_func [public]
 *
 * Purpose:	Schedule a function callout.
 *		Guarantees { function, argument }
 *		uniqueness if unique_call is TRUE.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_func(
    thread_call_func_t	func,
    thread_call_spec_t	spec,
    boolean_t		unique_call
)
{
    _thread_call_t	call;
    int			s;
    
    if (!thread_call_initialized)
    	panic("thread_call_func_unique");
	
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    call = TC(queue_first(&pending_call_queue));
    
    if (unique_call) while (!queue_end(&pending_call_queue, qe(call))) {
    	if (
		call->func == func
	    &&
	    	call->spec == spec) {
		
	    break;
	}
	
	call = TC(queue_next(qe(call)));
    }
    
    if (!unique_call || queue_end(&pending_call_queue, qe(call))) {
    
	call = _internal_call_allocate();
    
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALLOUT),
		     0, call->func, call->spec,
		     0, 0);
	call->func		= func;
	call->spec		= spec;
	call->spec_proto	= 0;
	
	_pending_call_enqueue(call);
		
	_call_thread_wake();
    }
    else
    	simple_unlock(&thread_call_lock);
    
    splx(s);
}

/*
 * Routine:	thread_call_func_delayed [public]
 *
 * Purpose:	Schedule a function callout to
 *		occur at the stated time.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_func_delayed(
    thread_call_func_t	func,
    thread_call_spec_t	spec,
    tvalspec_t		deadline
)
{
    _thread_call_t	call;
    int			s;
    
    if (!thread_call_initialized)
    	panic("thread_call_func_delayed");

    if (BAD_TVALSPEC(&deadline))
    	deadline = TVALSPEC_ZERO;

    s = splsched();

    simple_lock(&thread_call_lock);
    
    call = _internal_call_allocate();

    KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALLOUT),
		 1, func, spec,
		 deadline.tv_sec, deadline.tv_nsec);
    call->func		= func;
    call->spec		= spec;
    call->spec_proto	= 0;
    call->deadline	= deadline;
    
    _delayed_call_enqueue(call);
    
    if (queue_first(&delayed_call_queue) == qe(call))
    	_set_delayed_call_timer(call);
    
    simple_unlock(&thread_call_lock);
    
    splx(s);
}

/*
 * Routine:	thread_call_func_cancel [public]
 *
 * Purpose:	Unschedule a function callout.
 *		Removes one (or all)
 *		{ function, argument }
 *		instance(s) from either (or both)
 *		the pending and	the delayed queue,
 *		in that order.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_func_cancel(
    thread_call_func_t	func,
    thread_call_spec_t	spec,
    boolean_t		cancel_all
)
{
    int			s;
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALLOUT),
		 2, func, spec,
		 cancel_all, 0);
    _remove_from_pending_queue(func, spec, cancel_all) ||
	_remove_from_delayed_queue(func, spec, cancel_all);
    
    simple_unlock(&thread_call_lock);
    
    splx(s);
}

/*
 * Routine:	thread_call_allocate [public]
 *
 * Purpose:	Allocate an external callout
 *		entry.
 *
 * Preconditions:	None.
 *
 * Postconditions:	None.
 */

thread_call_t
thread_call_allocate(
    thread_call_func_t	func,
    thread_call_spec_t	spec
)
{
    _thread_call_t	call = (void *)kalloc(sizeof (struct _thread_call));
    
    call->func		= func;
    call->spec		= 0;
    call->spec_proto	= spec;
    call->deadline	= TVALSPEC_ZERO;
    call->status	= IDLE;
    
    return (call);
}

/*
 * Routine:	thread_call_free [public]
 *
 * Purpose:	Free an external callout
 *		entry.
 *
 * Preconditions:	None.
 *
 * Postconditions:	None.
 */

void
thread_call_free(
    thread_call_t	_call
)
{
    _thread_call_t	call = _call;
    int			s;
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    if (call->status != IDLE) {
    	simple_unlock(&thread_call_lock);
	panic("thread_call_free");
    }
    
    simple_unlock(&thread_call_lock);
    
    splx(s);
    
    kfree((void *)call, sizeof (struct _thread_call));
}

/*
 * Routine:	thread_call_enter [public]
 *
 * Purpose:	Schedule an external callout 
 *		entry.  There is no effect if
 *		the entry is not idle.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_enter(
    thread_call_t	_call
)
{
    _thread_call_t	call = _call;
    int			s;
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    if (call->status == IDLE) {
    	call->spec = call->spec_proto;

    	_pending_call_enqueue(call);
	
	_call_thread_wake();
    }
    else
	simple_unlock(&thread_call_lock);

    splx(s);
}

void
thread_call_enter_spec(
    thread_call_t	_call,
    thread_call_spec_t	spec
)
{
    _thread_call_t	call = _call;
    int			s;
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    if (call->status == IDLE) {
    	call->spec = spec;

    	_pending_call_enqueue(call);
	
	_call_thread_wake();
    }
    else
	simple_unlock(&thread_call_lock);

    splx(s);
}

/*
 * Routine:	thread_call_enter_delayed [public]
 *
 * Purpose:	Schedule an external callout 
 *		entry to occur at the stated time.
 *		There is no effect if the entry
 *		is not idle.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_enter_delayed(
    thread_call_t	_call,
    tvalspec_t		deadline
)
{
    _thread_call_t	call = _call;
    int			s;

    if (BAD_TVALSPEC(&deadline))
    	deadline = TVALSPEC_ZERO;

    s = splsched();

    simple_lock(&thread_call_lock);

    if (call->status == IDLE) {
    	call->spec	= call->spec_proto;
    	call->deadline	= deadline;

    	_delayed_call_enqueue(call);

	if (queue_first(&delayed_call_queue) == qe(call))
	    _set_delayed_call_timer(call);
    }

    simple_unlock(&thread_call_lock);

    splx(s);
}

void
thread_call_enter_spec_delayed(
    thread_call_t	_call,
    thread_call_spec_t	spec,
    tvalspec_t		deadline
)
{
    _thread_call_t	call = _call;
    int			s;

    if (BAD_TVALSPEC(&deadline))
    	deadline = TVALSPEC_ZERO;

    s = splsched();

    simple_lock(&thread_call_lock);

    if (call->status == IDLE) {
    	call->spec	= spec;
    	call->deadline	= deadline;

    	_delayed_call_enqueue(call);

	if (queue_first(&delayed_call_queue) == qe(call))
	    _set_delayed_call_timer(call);
    }

    simple_unlock(&thread_call_lock);

    splx(s);
}

/*
 * Routine:	thread_call_cancel [public]
 *
 * Purpose:	Unschedule a callout entry.
 *		There is no effect if the
 *		entry is already idle.
 *
 * Preconditions:	Callable from an interrupt context
 *			below splsched.
 *
 * Postconditions:	None.
 */

void
thread_call_cancel(
    thread_call_t	_call
)
{
    _thread_call_t	call = _call;
    int			s;
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    if (call->status == PENDING) {
    	_pending_call_dequeue(call);
	
	thread_call_release(call);
    }
    else if (call->status == DELAYED) {
    	_delayed_call_dequeue(call);
	
	thread_call_release(call);
    }
    else
    	/* do nothing */;
	
    simple_unlock(&thread_call_lock);
    
    splx(s);
}

/*
 * Routine:	_call_thread_wake [private]
 *
 * Purpose:	Wakeup one callout thread to handle
 *		a new callout entry.  May wakeup
 *		the callout activate thread to
 *		create a new callout thread.
 *
 * Preconditions:	thread_call_lock held.
 *
 * Postconditions:	thread_call_lock released.
 */

static __inline__
void
_call_thread_wake(void)
{
    boolean_t		wake_activate_thread = FALSE;
    
    if (call_thread_num < (active_call_num + pending_call_num))
    	wake_activate_thread = TRUE;
	
    simple_unlock(&thread_call_lock);

    thread_wakeup_one(&pending_call_num);
    
    if (wake_activate_thread)
    	thread_wakeup_one(&call_thread_num);
}

/*
 * Routine:	_call_thread [private]
 *
 * Purpose:	Executed by a callout thread.
 *
 * Preconditions:	None.
 *
 * Postconditions:	None.
 */

static
void
_call_thread_continue(void)
{
    thread_t		self = current_thread();
    
    (void) splsched();
    
    simple_lock(&thread_call_lock);

    while (pending_call_num > 0) {
	_thread_call_t		call;
	thread_call_func_t		func;
	thread_call_spec_t		spec;

	call = TC(dequeue_head(&pending_call_queue)); pending_call_num--;

	func = call->func;
	spec = call->spec;
	
	call->status = IDLE;

	call = thread_call_release(call);

	active_call_num++; simple_unlock(&thread_call_lock);
	
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALLOUT) | DBG_FUNC_START,
		     3, self, func, 
		     spec, 0);

	(void) spl0(); (*func)(spec, call); (void) splsched();
	
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_SCHED,MACH_CALLOUT) | DBG_FUNC_END,
		     4, self, 
		     func, spec,
		     0);

	simple_lock(&thread_call_lock); active_call_num--;
    }
	
    if ((call_thread_num - active_call_num) <= call_thread_min) {
	    
	assert_wait(&pending_call_num, FALSE);
	
	simple_unlock(&thread_call_lock);
	
	thread_block_with_continuation(_call_thread_continue);
	/* NOTREACHED */
    }
    
    call_thread_num--;
    
    simple_unlock(&thread_call_lock);
    
    (void) spl0();
    
    (void) thread_terminate(self); thread_halt_self();
}

static
void
_call_thread(void)
{
    stack_privilege(current_thread());
    
    _call_thread_continue();
    /* NOTREACHED */
}

/*
 * Routine:	_call_activate_thread [private]
 *
 * Purpose:	Executed by the callout activate thread.
 *
 * Preconditions:	None.
 *
 * Postconditions:	Never terminates.
 */

static
void
_call_activate_thread_continue(void)
{
    thread_t		self = current_thread();
        
    (void) splsched();
    
    simple_lock(&thread_call_lock);
        
    if (call_thread_num < (active_call_num + pending_call_num)) {

	call_thread_num++; simple_unlock(&thread_call_lock);
	
	(void) kernel_thread(self->task, _call_thread, (void *)0);
	
	thread_block_with_continuation(_call_activate_thread_continue);
	/* NOTREACHED */
    }
		
    assert_wait(&call_thread_num, FALSE);
    
    simple_unlock(&thread_call_lock);
    
    thread_block_with_continuation(_call_activate_thread_continue);
    /* NOTREACHED */
}

static
void
_call_activate_thread(void)
{
    stack_privilege(current_thread());
    
    _call_activate_thread_continue();
    /* NOTREACHED */
}

static
void
_delayed_call_interrupt(
    tvalspec_t		timestamp
)
{
    _thread_call_t	call;
    queue_head_t	intransit_call_queue;
    int			s;
    
    queue_init(&intransit_call_queue);
    
    s = splsched();
    
    simple_lock(&thread_call_lock);
    
    call = TC(queue_first(&delayed_call_queue));
    
    while (!queue_end(&delayed_call_queue, qe(call))) {
    	if (CMP_TVALSPEC(&call->deadline, &timestamp) <= 0) {
	    _delayed_call_dequeue(call);
	    
	    enqueue_tail(&intransit_call_queue, qe(call));
	}
	else
	    break;
	    
	call = TC(queue_first(&delayed_call_queue));
    }
    
    if (!queue_end(&delayed_call_queue, qe(call)))
    	_set_delayed_call_timer(call);
    
    while (call = TC(dequeue_head(&intransit_call_queue))) {
	_pending_call_enqueue(call);
	    
	_call_thread_wake();
	    
	simple_lock(&thread_call_lock);
    }
    
    simple_unlock(&thread_call_lock);
    
    splx(s);
}
