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
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */


#import <cpus.h>
#import <mach_host.h>

#if	NCPUS > 1

#import <kern/processor.h>
#import <kern/thread.h>
#import <kern/sched_prim.h>
#import <kern/parallel.h>

void unix_master()
{
	register thread_t t = current_thread();
	
	if (! (++( t->unix_lock )))	{

		/* thread_bind(t, master_processor); */
		t->bound_processor = master_processor;

		if (cpu_number() != master_cpu) {
			t->interruptible = FALSE;
			thread_block();
		}
	}
}

void unix_release()
{
	register thread_t t = current_thread();

	t->unix_lock--;
	if (t->unix_lock < 0) {
		/* thread_bind(t, PROCESSOR_NULL); */
		t->bound_processor = PROCESSOR_NULL;
#if	MACH_HOST
		if (t->processor_set != &default_pset)
			thread_block();
#endif	MACH_HOST
	}
}

void unix_reset()
{
	register thread_t	t = current_thread();

	if (t->unix_lock != -1)
		t->unix_lock = 0;
}

#endif	NCPUS > 1
