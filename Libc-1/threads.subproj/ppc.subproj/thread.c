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
 *	File:	libc/threads/m98k/thread.c
 *
 *	m98k dependent thread support routines.
 *
 * HISTORY
 * 25-Nov-92	Derek B Clegg (dclegg@next.com)
 *	Ported to m98k.
 * 14-Jan-92  Peter King (king@next.com)
 *	Created from M68K sources.
 */
#import <string.h>
#import	<mach/cthreads.h>
#import <mach/ppc/thread_status.h>
#import <architecture/ppc/cframe.h>
#import	"cthread_internals.h"

/* `cthread_set_self', `ur_cthread_self', and `cthread_sp' are defined
 * in `lock.s'.
 */

#define PPC_RED_ZONE_SIZE 224 /* Comes from the ABI - DO NOT CHANGE */

extern void cthread_body();

/* Set up the initial state of a MACH thread so that it will invoke
 * cthread_body(child) when it is resumed.
 */
void
cproc_setup(cproc_t child)
{
    int *top;
    ppc_thread_state_t state;
    kern_return_t r;

    top = (int *)(child->stack_base + child->stack_size);

    /* Set up the call frame and registers. */

    memset(&state, 0, sizeof(state));
    state.srr0 = (unsigned)cthread_body;
    state.r3 = (unsigned)child;	/* argument to function */

    /* Align stack to a stack boundary since cthread_sp() is somewhat
     * bogus. */

    state.r1 = (unsigned)top & ~(C_STACK_ALIGN - 1);	/* align */
    state.r1 -= C_ARGSAVE_LEN;
    state.r1 -= PPC_RED_ZONE_SIZE;

    MACH_CALL(thread_set_state(child->id,
			       PPC_THREAD_STATE,
			       (thread_state_t)&state,
			       PPC_THREAD_STATE_COUNT), r);
}
