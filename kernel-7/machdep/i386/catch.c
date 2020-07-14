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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Interrupt/Trap Catch routines.
 *
 * HISTORY
 *
 * 29 August 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <pc_support.h>

#if	PC_SUPPORT
#import <machdep/i386/pc_support/PCmiscInline.h>
#endif

void
catch_interrupt(
    thread_saved_state_t	*state	/* by reference */
)
{
    intr_handler(state);
    
    if ((state->frame.eflags & EFL_VM) || state->frame.cs.rpl == USER_PRIV) {
#if	PC_SUPPORT
	threadPCInterrupt(current_thread(), state);
#endif
	check_for_ast(state);
    }
}

void
catch_trap(
    thread_saved_state_t	*state	/* by reference */
)
{
    if ((state->frame.eflags & EFL_VM) || state->frame.cs.rpl == USER_PRIV) {
#if	PC_SUPPORT
	if (!threadPCException(current_thread(), state))
#endif
	    user_trap(state);
	/* NOTREACHED */
    }
    else
    	kernel_trap(state);
}
