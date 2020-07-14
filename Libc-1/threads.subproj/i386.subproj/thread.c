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
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * 386/thread.c
 *
 * Cproc startup for 386 MTHREAD implementation.
 */

#ifndef	lint
static char rcs_id[] = "$Header: /cvs/Darwin/CoreOS/Libraries/NeXT/libc/threads.subproj/i386.subproj/thread.c,v 1.1.1.1 1999/04/14 23:19:43 wsanchez Exp $";
#endif	not lint



#include <mach/cthreads.h>
#include "cthread_internals.h"

#include <mach/mach.h>

#import <mach/i386/thread_status.h>
#import <machdep/i386/seg.h>


/*
 * C library imports:
 */
extern bzero();

/*
 * Set up the initial state of a MACH thread
 * so that it will invoke cthread_body(child)
 * when it is resumed.
 */
void
cproc_setup(child)
	register cproc_t child;
{
	register int *top = (int *) (child->stack_base + child->stack_size);
	i386_thread_state_t state;
	register i386_thread_state_t *ts = &state;
	unsigned int state_count = i386_THREAD_STATE_COUNT;
	kern_return_t r;
	extern void cthread_body();

	/*
	 * Set up I386 call frame and registers.
	 * See I386 Architecture Handbook.
	 */
	MACH_CALL(thread_get_state(child->id, \
				   i386_THREAD_STATE, \
				   (thread_state_t) &state, \
				   &state_count),
		  r);
	/*
	 * Inner cast needed since too many C compilers choke on the type void (*)().
	 */
	ts->eip = (int) (int (*)()) cthread_body;
	*--top = (int) child;	/* argument to function */
	*--top = 0;
	ts->esp = (int) top;

	MACH_CALL(thread_set_state(child->id, \
				   i386_THREAD_STATE, \
				   (thread_state_t) &state, \
				   state_count),
		  r);
}


void
cthread_set_self(p)
	cproc_t	p;
{
	asm("pushl	%0" : : "m" (p));
	asm("pushl	$0");			// fake the kernel out
	asm("movl	$1, %%eax" : : : "ax");
	asm("lcall	$0x3b, $0");
}

ur_cthread_t
ur_cthread_self()
{
	asm("movl	$0, %eax");
	asm("lcall	$0x3b, $0");
}


int
cthread_sp(void)
{
	int	ret;

	asm("movl	%%esp,%0" : "=g" (ret));
	return	ret;
}
