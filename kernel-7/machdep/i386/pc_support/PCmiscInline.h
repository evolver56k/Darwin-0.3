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
 * Miscellaneous inline functions.
 *
 * HISTORY
 *
 * 15 Mar 1993 ? at NeXT
 *	Created.
 */

#if	KERNEL_PRIVATE

#import <mach/mach_types.h>

#import "PCprivate.h"

static __inline__
boolean_t
maskIsSet(
    void *		mask,
    int			n
)
{
#define cmask		((unsigned char *)mask)

    if (n < (sizeof (unsigned int) * BYTE_SIZE))
	return ((*(unsigned int *)cmask & (1 << n)) != 0);
	
    return ((*(cmask + (n / BYTE_SIZE)) & (1 << (n % BYTE_SIZE))) != 0);
#undef cmask
}

static __inline__
PCcontext_t
currentContext(
    PCshared_t		shared
)
{    
    if (shared->currentContext >= 0 &&
	    shared->currentContext < PCMAXCONTEXT)
	return (&shared->contexts[shared->currentContext]);
    else
	return ((PCcontext_t) 0);
}

static __inline__
PCshared_t
threadPCShared(
    thread_t		thread
)
{
    if (thread->pcb->PCpriv)
	return ((PCshared_t)thread->pcb->PCpriv->shared);
    else
    	return ((PCshared_t) 0);
}

static __inline__
PCcontext_t
threadPCContext(
    thread_t		thread
)
{
    PCshared_t		shared = threadPCShared(thread);

    if (shared)
    	return (currentContext(shared));
    else
	return ((PCcontext_t) 0);
}

static __inline__
boolean_t
threadPCException(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    PCshared_t		shared = threadPCShared(thread);
    
    if (shared && !maskIsSet(&shared->machMask, state->trapno))
    	return (PCexception(thread, state));
    else
    	return (FALSE);
}

static __inline__
void
threadPCInterrupt(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    PCcontext_t		context = threadPCContext(thread);
    
    if (context &&
    	(context->pendingTimers || context->pendingCallbacks) &&
							context->running) {
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
}

#endif
