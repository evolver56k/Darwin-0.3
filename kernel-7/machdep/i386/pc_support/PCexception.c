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
 * PC support exception handling.
 *
 * HISTORY
 *
 * 22 Mar 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <mach/exception.h>

#import <machdep/i386/trap.h>
#import <machdep/i386/cpu_inline.h>
#import <machdep/i386/err_inline.h>

#import "PCprivate.h"

static void	PCpagefaultContinue(void);

boolean_t
PCexception(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    PCcontext_t		context = threadPCContext(thread);
    
    if (!context->running)
    	return (FALSE);

    if (state->trapno == T_PAGE_FAULT) {
	kern_return_t	result;
	vm_offset_t	va;
		
	va = cr2();
	result = vm_fault(thread->task->map, trunc_page(va),
			    state->frame.err.pgfault.wrtflt?
			    	VM_PROT_READ | VM_PROT_WRITE:
				VM_PROT_READ,
			    FALSE, 0);
	if (result != KERN_SUCCESS) {
	    context->trapNum = state->trapno;
	    context->errCode = err_to_error_code(state->frame.err);
	    context->exceptionResult = result;

	    exception_with_continuation(
	    		EXC_BAD_ACCESS, result, va, PCpagefaultContinue);
	    /* NOTREACHED */
	    
	    if (context->exceptionResult != KERN_SUCCESS) {
		context->callHandler = PC_HANDLE_FAULT;
	
		PCcallMonitor(thread, state);
		/* NOTREACHED */
	    }
	}
    }
    else if (state->trapno == T_NOEXTENSION && !context->EM_bit)
    	fp_noextension(state);
    else if (state->frame.eflags & EFL_VM) {
    	if (!PCemulateREAL(thread, state))
	    return (FALSE);
    }
    else {
    	if (!PCemulatePROT(thread, state))
	    return (FALSE);
    }

    if (context->callHandler == PC_HANDLE_NONE &&
    	(context->pendingCallbacks || PCtimersPending(context))) {
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
    
    thread_exception_return();
    /* NOTREACHED */
}

static
void
PCpagefaultContinue(void)
{
    thread_t			thread = current_thread();
    thread_saved_state_t	*state = USER_REGS(thread);
    PCcontext_t			context = threadPCContext(thread);
    
    if (context->exceptionResult != KERN_SUCCESS) {
    	context->callHandler = PC_HANDLE_FAULT;
	
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }

    if (context->callHandler == PC_HANDLE_NONE &&
    	(context->pendingCallbacks || PCtimersPending(context))) {
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
    
    thread_exception_return();
    /* NOTREACHED */
}
