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
 * HISTORY
 * 30-Jul-90  Morris Meyer (mmeyer) at NeXT
 *	Renamed dealloc_stack to cope with it being a private extern symbol.
 *
 * stack.c  - by Eric Cooper
 *
 * C Thread stack allocation.
 */

//extern void exit(int);
#include "cthreads.h"
#include "cthread_internals.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

/*
 * C library imports:
 */
extern getrlimit();

#define	BYTES_TO_PAGES(b)	(((b) + vm_page_size - 1) / vm_page_size)

extern int cthread_stack_mask;
private vm_size_t cthread_stack_size = 0;

extern vm_address_t cthread_stack_base;	/* Base for stack allocation */

private vm_address_t next_stack_base = 0;

#if	NeXT
/*
 * Move to threads_data.c for SHLIB purposes.
 */
extern vm_address_t initial_stack_boundary;
#else
vm_address_t initial_stack_boundary;
#endif	NeXT
private vm_address_t initial_stack_deallocated = 0;
private int initial_stack_lock = 0;

/*
 * Set up a stack segment for a thread.
 * Segment has a red zone (invalid page)
 * for early detection of stack overflow.
 * The cproc_self pointer is stored at the base.
 *
 *	--------- (high address)
 *	|	|
 *	|  ...	|
 *	|	|
 *	| stack	|
 *	|	|
 *	|  ...	|
 *	|	|
 *	--------- (stack base)
 *	|	|
 *	|invalid|
 *	|	|
 *	---------
 *	|	|
 *	|	|
 *	| self	|
 *	--------- (low address)
 */

private void
setup_stack(p, base)
	register cproc_t p;
	register vm_address_t base;
{
	register kern_return_t r;

	/*
	 * Check alignment.
	 */
	ASSERT((base & cthread_stack_mask) == base);
	ASSERT(((base + cthread_stack_size - 1) & cthread_stack_mask) == base);
#if	NeXT
	p->stack_size = cthread_stack_size - vm_page_size;
#ifdef STACK_GROWTH_UP
	p->stack_base = (unsigned int) base ;
	MACH_CALL(vm_protect(task_self(), base+p->stack_size, vm_page_size, FALSE,  VM_PROT_NONE), r);
#else
	p->stack_base = (unsigned int) (base + vm_page_size);
	MACH_CALL(vm_protect(task_self(), base, vm_page_size, FALSE,  VM_PROT_NONE), r);
#endif STACK_GROWTH_UP

#else	NeXT
	/*
	 * Stack base is two pages from bottom.
	 */
	p->stack_base = (unsigned int) (base + 2*vm_page_size);
	/*
	 * Stack size is segment size minus two pages.
	 */
	if (p->flags & CPROC_INITIAL_STACK) {
		p->stack_size = initial_stack_boundary - p->stack_base;
	} else {
		p->stack_size = cthread_stack_size - 2*vm_page_size;
	}
	/*
	 * Protect red zone.
	 */
	MACH_CALL(vm_protect(task_self(), base + vm_page_size, vm_page_size, FALSE, VM_PROT_NONE), r);
	/*
	 * Store self pointer.
	 */
	*((cproc_t *) base) = p;
#endif	NeXT
}

void
stack_init(p)
	cproc_t p;
{
	struct rlimit rlim;

	/*
	 * Set cthread stack size from UNIX stack limit.
	 */
	if (getrlimit(RLIMIT_STACK, &rlim) != 0) {
		perror("getrlimit");
		exit(1);
	}
	cthread_stack_size = BYTES_TO_PAGES(rlim.rlim_cur) * vm_page_size;
	cthread_stack_mask = ~(cthread_stack_size - 1);

	/*
	 * Guess at first available region for stack.
	 */
	next_stack_base = (cthread_stack_base + cthread_stack_size-1) &
				cthread_stack_mask;

	/*
	 * This is a pessimistic approximation of the top of the initial stack
	 * not including the thread trampoline code, the arguments and the
	 * initial environment.  It's pessimistic in that it contains a few
	 * stack frames which will be wasted when the stack is reallocated.
	 */
	initial_stack_boundary = (vm_address_t) cthread_sp();
	p->flags |= CPROC_INITIAL_STACK;	/* We're "special" */

#if	NeXT
	/*
	 * Don't muck with default stack, it would prevent it from growing.
	 * Note: This would cause problems if we actually deallocated these
	 * things and the user increased his stack size.
	 */
	p->stack_base = (unsigned int) (cthread_sp() & cthread_stack_mask);
	p->stack_base = (unsigned int) (p->stack_base + vm_page_size);
	p->stack_size = cthread_stack_size - vm_page_size;
#else	NeXT
	/*
	 * Set up stack for main thread.
	 */
	setup_stack(p, (vm_address_t) (cthread_sp() & cthread_stack_mask));
#endif	NeXT
}

#if	NeXT
/* cthread_sp() is now architecture-dependant */
#else	NeXT
int
cthread_sp()
{
	int x;

	return (int) &x;
}
#endif	NeXT

/*
 * Allocate a stack segment for a thread.  Stacks are never deallocated
 * except by the child process of a fork() during re-initialization.
 *
 * The variable next_stack_base is used to align stacks.
 * It may be updated by several threads in parallel,
 * but mutual exclusion is unnecessary: at worst,
 * the vm_allocate will fail and the thread will try again.
 */

void
alloc_stack(p)
	cproc_t p;
{
	vm_address_t base;

	if (initial_stack_deallocated) {
		vm_address_t boundary_page;
		register kern_return_t r;

		spin_lock(&initial_stack_lock);
		if (! initial_stack_deallocated) {
			spin_unlock(&initial_stack_lock);
			goto normal_alloc_stack;
		}
		initial_stack_deallocated = 0;
		spin_unlock(&initial_stack_lock);

		p->flags |= CPROC_INITIAL_STACK;

		boundary_page = initial_stack_boundary &~ (vm_page_size - 1);
		base = initial_stack_boundary & cthread_stack_mask;

		if (initial_stack_boundary != boundary_page)
			bzero(boundary_page,
				initial_stack_boundary - boundary_page);

		MACH_CALL(vm_deallocate(task_self(), base, boundary_page - base), r);
		MACH_CALL(vm_allocate(task_self(), &base, boundary_page - base, FALSE), r);

		setup_stack(p, base);
		return;
	}

normal_alloc_stack:
	for (base = next_stack_base; ; ) {
	    ASSERT(cthread_stack_base == 0 || base != 0);
	    if (vm_allocate(task_self(), &base, cthread_stack_size, FALSE)
		== KERN_SUCCESS) break;
	    base += cthread_stack_size;
	}

	next_stack_base = base + cthread_stack_size;
	setup_stack(p, base);
}

#if	NeXT
void _dealloc_stack(p)
#else
void dealloc_stack(p)
#endif	NeXT
	cproc_t p;
{
	register kern_return_t r;

	if (p->flags & CPROC_INITIAL_STACK) {
		/*
		 * Don't deallocate the signal trampoline code, the args or
		 * the initial environment even though they're on a stack.
		 * They're actually per-task rather than per-thread data.
		 */
		initial_stack_deallocated = p->stack_base;
	} else {
		MACH_CALL(vm_deallocate(task_self(), p->stack_base & cthread_stack_mask, cthread_stack_size), r);
	}
}

#if	NeXT
void _stack_fork_child()
#else
void stack_fork_child()
#endif	NeXT
/*
 * Called in the child after a fork().  Resets stack data structures to
 * coincide with the reality that we now have a single cproc and cthread.
 */
{
	/*
	 * Allocate stacks from the beginning of cthread_stack_base again.
	 */
	next_stack_base = (cthread_stack_base + cthread_stack_size-1) & cthread_stack_mask;
}
