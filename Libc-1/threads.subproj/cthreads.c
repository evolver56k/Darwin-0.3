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
 * cthreads.c - by Eric Cooper
 *
 * Implementation of cthread_fork, cthread_join, cthread_exit, etc.
 * HISTORY
 * 22-July-93 Blaine Garst
 *	fixed association of read_thread info
 *	fixed kernel cache set up of cproc info
 *
 */
#include <stdlib.h>
#include "cthreads.h"
#include "cthread_internals.h"

/*
 * C Threads imports:
 */
extern void cproc_init();
extern thread_t cproc_create();
extern void mig_init();

/*
 * Mach imports:
 */
extern void mig_fork_child();

/*
 * C library imports:
 */
extern int _setjmp(jmp_buf env);
extern void _longjmp(jmp_buf env, int val);

/*
 * Thread status bits.
 */
#define	T_MAIN		0x1
#define	T_RETURNED	0x2
#define	T_DETACHED	0x4

#ifdef	CTHREADS_DEBUG
int cthread_debug = FALSE;
#endif	CTHREADS_DEBUG

private struct cthread_queue cthreads = QUEUE_INITIALIZER;
private struct mutex cthread_lock = MUTEX_INITIALIZER;
private struct condition cthread_needed = CONDITION_INITIALIZER;
private struct condition cthread_idle = CONDITION_INITIALIZER;
private int n_cprocs = 0;
private int n_cthreads = 0;
private int max_cprocs = 0;

private cthread_t free_cthreads = NO_CTHREAD;	/* free list */
private int cthread_free_lock = 0;		/* unlocked */

private struct cthread initial_cthread = { 0 };

private cthread_t
cthread_alloc(func, arg)
	cthread_fn_t func;
	any_t arg;
{
	register cthread_t t = NO_CTHREAD;

	if (free_cthreads != NO_CTHREAD) {
		/*
		 * Don't try for the lock unless
		 * the list is likely to be nonempty.
		 * We can't be sure, though, until we lock it.
		 */
		spin_lock(&cthread_free_lock);
		t = free_cthreads;
		if (t != NO_CTHREAD)
			free_cthreads = t->next;
		spin_unlock(&cthread_free_lock);
	}
	if (t == NO_CTHREAD) {
		/*
		 * The free list was empty.
		 * We may have only found this out after
		 * locking it, which is why this isn't an
		 * "else" branch of the previous statement.
		 */
		t = (cthread_t) malloc(sizeof(struct cthread));
	}
	*t = initial_cthread;
	t->func = func;
	t->arg = arg;
	return t;
}

typedef void (*frv_t)(void);
static frv_t  _cthread_free_callout_function = 0;

void _set_cthread_free_callout(frv_t x) {
	_cthread_free_callout_function = x;
}

private void
cthread_free(t)
	register cthread_t t;
{
	if (_cthread_free_callout_function)
		(*_cthread_free_callout_function)();
	spin_lock(&cthread_free_lock);
	t->next = free_cthreads;
	free_cthreads = t;
	spin_unlock(&cthread_free_lock);
}

void
cthread_init()
{
	static int cthreads_started = FALSE;
	register cthread_t t;

	if (cthreads_started)
		return;
	cproc_init();
	n_cprocs = 1;
	t = cthread_alloc((cthread_fn_t) 0, (any_t) 0);
	n_cthreads = 1;
	t->state |= T_MAIN;
	cthread_set_name(t, "main");
	cthread_assoc(cproc_self(), t);
	t->real_thread = thread_self();
	cthreads_started = TRUE;
	mig_init(1);		/* enable multi-threaded mig interfaces */
}

/*
 * Used for automatic initialization by crt0.
 */
extern int (*_cthread_init_routine)();

/*
 * Procedure invoked at the base of each cthread.
 */
void
cthread_body(self)
	cproc_t self;
{
	register cthread_t t;

	cthread_set_self(self);
	ASSERT(cproc_self() == self);
	TRACE(printf("[idle] cthread_body(%x)\n", self));
	mutex_lock(&cthread_lock);
	for (;;) {
		/*
		 * Dequeue a thread invocation request.
		 */
		cthread_queue_deq(&cthreads, cthread_t, t);
		if (t != NO_CTHREAD) {
			/*
			 * We have a thread to execute.
			 */
			mutex_unlock(&cthread_lock);
			cthread_assoc(self, t);		/* assume thread's identity */
			t->real_thread = self->id;
			if (_setjmp(t->catch) == 0) {	/* catch for cthread_exit() */
				/*
				 * Execute the fork request.
				 */
				t->result = (*(t->func))(t->arg);
			}
			/*
			 * Return result from thread.
			 */
			TRACE(printf("[%s] done()\n", cthread_name(t)));
			mutex_lock(&t->lock);

			if (t->state & T_MAIN) {
				/*
				 * Could have become main thread if we're the
				 * child of a thread that did a fork() call.
				 */
				mutex_unlock(&t->lock);
				cthread_exit(t->result);
			}

			if (t->state & T_DETACHED) {
				mutex_unlock(&t->lock);
				cthread_free(t);
			} else {
				t->state |= T_RETURNED;
				mutex_unlock(&t->lock);
				condition_signal(&t->done);
			}
			cthread_assoc(self, NO_CTHREAD);
			mutex_lock(&cthread_lock);
			n_cthreads -= 1;
		} else {
			/*
			 * Queue is empty.
			 * Signal that we're idle in case the main thread
			 * is waiting to exit, then wait for reincarnation.
			 */
			condition_signal(&cthread_idle);
			condition_wait(&cthread_needed, &cthread_lock);
		}
	}
}

cthread_t
cthread_fork(func, arg)
	cthread_fn_t func;
	any_t arg;
{
	register cthread_t t;

	TRACE(printf("[%s] fork()\n", cthread_name(cthread_self())));
	mutex_lock(&cthread_lock);
	t = cthread_alloc(func, arg);
	cthread_queue_enq(&cthreads, t);
	if (++n_cthreads > n_cprocs && (max_cprocs == 0 || n_cprocs < max_cprocs)) {
		n_cprocs += 1;
		cproc_create();
	}
	mutex_unlock(&cthread_lock);
	condition_signal(&cthread_needed);
	return t;
}

void
cthread_detach(t)
	cthread_t t;
{
	TRACE(printf("[%s] detach(%s)\n", cthread_name(cthread_self()), cthread_name(t)));
	mutex_lock(&t->lock);
	if (t->state & T_RETURNED) {
		mutex_unlock(&t->lock);
		cthread_free(t);
	} else {
		t->state |= T_DETACHED;
		mutex_unlock(&t->lock);
	}
}

any_t
cthread_join(t)
	cthread_t t;
{
	any_t result;

	TRACE(printf("[%s] join(%s)\n", cthread_name(cthread_self()), cthread_name(t)));
	mutex_lock(&t->lock);
	ASSERT(! (t->state & T_DETACHED));
	while (! (t->state & T_RETURNED))
		condition_wait(&t->done, &t->lock);
	result = t->result;
	mutex_unlock(&t->lock);
	cthread_free(t);
	return result;
}

void
cthread_exit(result)
	any_t result;
{
	register cthread_t t = cthread_self();

	TRACE(printf("[%s] exit()\n", cthread_name(t)));
	t->result = result;
	if (t->state & T_MAIN) {
		mutex_lock(&cthread_lock);
		while (n_cthreads > 1)
			condition_wait(&cthread_idle, &cthread_lock);
		mutex_unlock(&cthread_lock);
		exit((int) result);
	} else {
		_longjmp(t->catch, TRUE);
	}
}

/*
 * Used for automatic finalization by crt0.
 */
extern int (*_cthread_exit_routine)();

void
cthread_set_name(t, name)
	cthread_t t;
	const char *name;
{
	t->name = name;
}

const char *
cthread_name(t)
	cthread_t t;
{
	return (t == NO_CTHREAD
		? "idle"
		: (t->name == 0 ? "?" : t->name));
}

int
cthread_limit()
{
	return max_cprocs;
}

void
cthread_set_limit(n)
	int n;
{
	max_cprocs = n;
}

int
cthread_count()
{
	return n_cthreads;
}

/*
 * Routines for supporting fork() of multi-threaded programs.
 */


extern void _malloc_fork_prepare(), _malloc_fork_parent();
extern void _malloc_fork_child();
extern void _cproc_fork_child(), _stack_fork_child();
/*
 *
 * extern void _lu_fork_child();
 *
 * _lu_port was originally known only to netinfo library, but that left
 * us with a reference from libc (_lu_fork_child_, which was not
 * desirable. So, now rather than a function libc calls in netinfo,
 * libc knows about the lookup port.
 *
 * AOF (freier@next.com)  Mon Jun 26 14:42:59 PDT 1995
 *
 */
port_t _lu_port = NULL;

static cproc_t	saved_self = 0;
void _cthread_fork_prepare()
/*
 * Prepare cthreads library to fork() a multi-threaded program.  All cthread
 * library critical section locks are acquired before a fork() and released
 * afterwards to insure no cthread data structure is left in an inconsistent
 * state in the child, which comes up with only the forking thread running.
 */
{
	mutex_lock(&cthread_lock);
	spin_lock(&cthread_free_lock);
	saved_self = cproc_self();
	_malloc_fork_prepare();
}

void _cthread_fork_parent()
/*
 * Called in the parent process after a fork syscall.
 * Releases locks acquired by cthread_fork_prepare().
 */
{
	_malloc_fork_parent();
	spin_unlock(&cthread_free_lock);
	mutex_unlock(&cthread_lock);
}

void _cthread_fork_child()
/*
 * Called in the child process after a fork syscall.  Releases locks acquired
 * by cthread_fork_prepare().  Deallocates cthread data structures which
 * described other threads in our parent.  Makes this thread the main thread.
 * 
 * The mach_init() routine must be called in the child before this routine.
 */
{
	cthread_t t;

	cthread_set_self(saved_self);
	mig_fork_child();
	_malloc_fork_child();
	spin_unlock(&cthread_free_lock);
	mutex_unlock(&cthread_lock);

	condition_init(&cthread_needed);
	condition_init(&cthread_idle);
	max_cprocs = 0;

	/*
	 * Reinit other modules.
	 */

	_stack_fork_child();
	_cproc_fork_child();
	_lu_port = NULL;  /* _lu_fork_child(); */
				/* AOF Mon Jun 26 14:45:13 PDT 1995 */

	while (TRUE) {		/* Free cthread runnable list */
		cthread_queue_deq(&cthreads, cthread_t, t);
		if (t == NO_CTHREAD) break;
		free((char *) t);
	}

	while (free_cthreads != NO_CTHREAD) {	/* Free cthread free list */
		t = free_cthreads;
		free_cthreads = free_cthreads->next;
		free((char *) t);
	}

	/*
	 * From cthread_init() ...
	 */

	t = cthread_self();

	n_cprocs = 1;
	n_cthreads = 1;
	t->state = T_MAIN;
	t->real_thread = thread_self();
	cthread_set_name(t, "main");
	mig_init(1);		/* enable multi-threaded mig interfaces */
	
}

