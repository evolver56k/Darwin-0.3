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
 * mig_support.c  - by Mary Thompson
 *
 * Routines to set and deallocate the mig reply port for the current thread.
 * Called from mig-generated interfaces.
 */
#include <mach/mach.h>
#include "cthreads.h"
#include "cthread_internals.h"

private struct mutex reply_port_lock = MUTEX_INITIALIZER;
#if	NeXT
#else	NeXT
private int multithreaded = 0;
#endif	NeXT

#if NeXT
/*
 * called in new child...
 * clear lock to cover case where the parent had
 * a thread holding this lock while another thread
 * did the fork()
 */
void mig_fork_child()
{
	mutex_unlock(&reply_port_lock);
}
#endif

/*
 * Called by mach_init with 0 before cthread_init is
 * called and again with 1 at the end of cthread_init.
 */
void
mig_init(init_done)
	int init_done;
{
#if	NeXT
#else	NeXT
	multithreaded = init_done;
#endif	NeXT
}

/*
 * Called by mig interface code whenever a reply port is needed.
 * Tracing is masked during this call; otherwise, a call to printf()
 * can result in a call to malloc() which eventually reenters
 * mig_get_reply_port() and deadlocks.
 */
port_t
mig_get_reply_port()
{
	register cproc_t self;
	register kern_return_t r;
	port_t port;
#ifdef	CTHREADS_DEBUG
	int d = cthread_debug;
#endif	CTHREADS_DEBUG

#if	NeXT
#else	NeXT
	if (! multithreaded)
		return thread_reply();
#endif	NeXT
#ifdef	CTHREADS_DEBUG
	cthread_debug = FALSE;
#endif	CTHREADS_DEBUG
	self = cproc_self();
#if	NeXT
	if (self == NO_CPROC) {
#ifdef	CTHREADS_DEBUG
		cthread_debug = d;
#endif	CTHREADS_DEBUG
		return(thread_reply());
	}
#endif	NeXT
	if (self->reply_port == PORT_NULL) {
		mutex_lock(&reply_port_lock);
		if (self->reply_port == PORT_NULL) {
			self->reply_port = thread_reply();
			MACH_CALL(port_allocate(task_self(), &port), r);
			self->reply_port = port;
		}
		mutex_unlock(&reply_port_lock);
	}
#ifdef	CTHREADS_DEBUG
	cthread_debug = d;
#endif	CTHREADS_DEBUG
	return self->reply_port;
}

/*
 * Called by mig interface code after a timeout on the reply port.
 * May also be called by user.
 */
void
mig_dealloc_reply_port()
{
	register cproc_t self;
	register port_t port;
#ifdef	CTHREADS_DEBUG
	int d = cthread_debug;
#endif	CTHREADS_DEBUG

#if	NeXT
#else	NeXT
	if (! multithreaded)
		return;
#endif	NeXT
#ifdef	CTHREADS_DEBUG
	cthread_debug = FALSE;
#endif	CTHREADS_DEBUG
	self = cproc_self();
#if	NeXT
	if (self == NO_CPROC) {
#ifdef	CTHREADS_DEBUG
		cthread_debug = d;
#endif	CTHREADS_DEBUG
		return;
	}
#endif	NeXT
	ASSERT(self != NO_CPROC);
	port = self->reply_port;
	if (port != PORT_NULL && port != thread_reply()) {
		mutex_lock(&reply_port_lock);
		if (self->reply_port != PORT_NULL && self->reply_port != thread_reply()) {
			self->reply_port = thread_reply();
			(void) port_deallocate(task_self(), port);
			self->reply_port = PORT_NULL;
		}
		mutex_unlock(&reply_port_lock);
	}
#ifdef	CTHREADS_DEBUG
	cthread_debug = d;
#endif	CTHREADS_DEBUG
}
