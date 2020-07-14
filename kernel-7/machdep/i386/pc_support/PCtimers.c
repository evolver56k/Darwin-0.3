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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * Timeout and tick support.
 *
 * HISTORY
 *
 * 30 Mar 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <kern/thread_call.h>

#import "PCprivate.h"

static
void PCpendTimeout(
	PCcontext_t		context);

static
void PCpendTick(
	PCcontext_t		context);

#define TVALSPEC_USEC(usec)		\
	((tvalspec_t) { ((usec) / USEC_PER_SEC), ((usec) * NSEC_PER_USEC) } )

static __inline__
void
PCscheduleTimeout(
    PCcontext_t		context
)
{
    context->pendingTimers &= ~PC_CALL_TIMEOUT;

    if (context->expectedTimers & PC_CALL_TIMEOUT)
    	thread_call_func_cancel(
			(thread_call_func_t)PCpendTimeout, context, FALSE);
    
    if (context->runOptions & PC_RUN_TIMEOUT) {
	thread_call_func_delayed(
		(thread_call_func_t)PCpendTimeout, context,
			deadline_from_interval(
				TVALSPEC_USEC(context->timeout)));
    
	context->expectedTimers |= PC_CALL_TIMEOUT;
    }
    else
    	context->expectedTimers &= ~PC_CALL_TIMEOUT;
}

static
void
PCpendTimeout(
    PCcontext_t		context
)
{
    if (context->expectedTimers & PC_CALL_TIMEOUT) {
	if (context->running)
	    context->pendingTimers |= PC_CALL_TIMEOUT;
	
	context->expectedTimers &= ~PC_CALL_TIMEOUT;
    }
}

static __inline__
void
PCscheduleTick(
    PCcontext_t		context
)
{
    if (context->pendingTimers & PC_CALL_TICK) {
	context->pendingCallbacks |= PC_CALL_TICK;

	context->pendingTimers &= ~PC_CALL_TICK;
    }
    
    if (context->expectedTimers & PC_CALL_TICK)
    	/* do nothing */;
    else if (context->runOptions & PC_RUN_TICK) {	
	thread_call_func_delayed(
		(thread_call_func_t)PCpendTick, context,
			deadline_from_interval(
				TVALSPEC_USEC(context->tick)));
	
	context->expectedTimers |= PC_CALL_TICK;
    }
}

static
void
PCpendTick(
    PCcontext_t		context
)
{
    if (context->expectedTimers & PC_CALL_TICK) {
	context->pendingTimers |= PC_CALL_TICK;
	
	context->expectedTimers &= ~PC_CALL_TICK;
    }
}
	
void
PCscheduleTimers(
    PCcontext_t		context
)
{
    PCscheduleTimeout(context);
	
    PCscheduleTick(context);
}

void
PCdeliverTimers(
    PCcontext_t		context
)
{
    context->pendingCallbacks |= 
    	context->pendingTimers & (PC_CALL_TIMEOUT | PC_CALL_TICK);
	
    context->pendingTimers &= ~(PC_CALL_TIMEOUT | PC_CALL_TICK);
}

boolean_t
PCtimersPending(
    PCcontext_t		context
)
{
    boolean_t		pending;
    
    pending = (context->pendingTimers & (PC_CALL_TIMEOUT | PC_CALL_TICK)) != 0;
    
    return (pending);
}

void
PCcancelTimers(
    PCcontext_t		context
)
{
    thread_call_func_cancel(
    		(thread_call_func_t)PCpendTimeout, context, FALSE);
    thread_call_func_cancel(
    		(thread_call_func_t)PCpendTick, context, FALSE);
}

void
PCcancelAllTimers(
    thread_t		thread
)
{
    PCshared_t		shared = threadPCShared(thread);
    int			i;
	
    for (i = 0; i < PCMAXCONTEXT; i++)
	PCcancelTimers(&shared->contexts[i]);
}
