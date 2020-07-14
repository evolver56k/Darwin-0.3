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
 * 22-July-93 Blaine Garst
 *	fixed kernel cache set up of cproc info
 *
 * 05-April-90  Morris Meyer (mmeyer) at NeXT
 * 	Fixed bug in cproc_fork_child() where the first cproc would
 *	try doing a msg_rpc() with an invalid reply port.
 *
 * Revision 1.5  89/06/21  13:02:10  mbj
 * 	Fixed the performance bug that HP reported regarding spin locks
 * 	held around msg_send() calls.
 * 
 * 	Removed the old (non-IPC) form of condition waiting/signalling.
 * 	It hasn't worked since thread_wait/thread_suspend/thread_resume
 * 	changed, many months ago.
 * 
 * Revision 1.4  89/05/19  13:02:28  mbj
 * 	Initialize cproc flags.
 * 
 * Revision 1.3  89/05/05  18:47:55  mrt
 * 	Cleanup for Mach 2.5
 * 
 * 24-Mar-89  Michael Jones (mbj) at Carnegie-Mellon University
 *	Implement fork() for multi-threaded programs.
 *	Made MTASK version work correctly again.
 *
 * 01-Apr-88  Eric Cooper (ecc) at Carnegie Mellon University
 *	Changed condition_clear(c) to acquire c->lock,
 *	to serialize after any threads still doing condition_signal(c).
 *	Suggested by Dan Julin.
 *
 * 19-Feb-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Extended the inline scripts to handle spin_unlock() and mutex_unlock().
 *
 * 28-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed thread_data argument from thread_create (for new
 *	interface).
 *
 */
/*
 * cprocs.c - by Eric Cooper
 *
 * Implementation of cprocs (lightweight processes)
 * and primitive synchronization operations.
 */
#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include "cthreads.h"
#include "cthread_internals.h"
#include <mach/message.h>

/*
 * C Threads imports:
 */
extern void stack_init();
#if	NeXT
extern void alloc_stack(), _dealloc_stack();
#else
extern void alloc_stack(), dealloc_stack();
#endif	NeXT

/*
 * Mach imports:
 */
extern port_t thread_self();
extern boolean_t swtch_pri();

private int cprocs_started = FALSE;

#ifdef	CTHREADS_DEBUG
private void
print_cproc(p)
	cproc_t p;
{
	char *s;

	switch (p->state) {
	    case CPROC_RUNNING:
		s = "";
		break;
	    case CPROC_SPINNING:
		s = "+";
		break;
	    case CPROC_BLOCKED:
		s = "*";
		break;
	    default:
		ASSERT(SHOULDNT_HAPPEN);
	}
	printf(" %x(%s)%s",
		p->id,
		cthread_name(p->incarnation), s);
}

private void
print_cproc_queue(name, queue)
	const char * name;
	cthread_queue_t queue;
{
	printf("[%s] %s:", cthread_name(cthread_self()), name);
	cthread_queue_map(queue, cproc_t, print_cproc);
	printf("\n");
}
#endif	CTHREADS_DEBUG

private int cproc_lock = 0;		/* unlocked */
private cproc_t cprocs = NO_CPROC;	/* linked list of cprocs */

#ifdef	CTHREADS_DEBUG
private void
print_all_cprocs()
{
	cproc_t p;

	printf("[%s] cprocs:", cthread_name(cthread_self()));
	for (p = cprocs; p != NO_CPROC; p = p->link)
		print_cproc(p);
	printf("\n");
}
#endif	CTHREADS_DEBUG

private cproc_t
cproc_alloc()
{
	register cproc_t p = (cproc_t) malloc(sizeof(struct cproc));
	kern_return_t r;

	p->incarnation = NO_CPROC;
	p->state = CPROC_RUNNING;
	p->reply_port = PORT_NULL;

	MACH_CALL(port_allocate(task_self(), &p->wait_port), r);
	MACH_CALL(port_set_backlog(task_self(), p->wait_port, 1), r);

	p->flags = 0;

	spin_lock(&cproc_lock);
	p->link = cprocs;
	cprocs = p;
	spin_unlock(&cproc_lock);

	return p;
}

void
cproc_init()
{
	cproc_t p = cproc_alloc();

	ASSERT(!cprocs_started);

	p->id = thread_self();

#if	NeXT
	cthread_set_self(p);
#endif	NeXT

	stack_init(p);

	cprocs_started = TRUE;
}

#if	NeXT
#else	NeXT
#ifndef	ur_cthread_self
/*
 * Find self by masking stack pointer
 * to find value stored at base of stack.
 */
ur_cthread_t
ur_cthread_self()
{
	register cproc_t p;

	register int sp = cthread_sp();

	p = *((cproc_t *) (sp & cthread_stack_mask));
	ASSERT(p != NO_CPROC && p->stack_base <= sp && sp < p->stack_base + p->stack_size);
	return (ur_cthread_t) p;
}
#endif	ur_cthread_self
#endif	NeXT

#ifdef	unused
void
cproc_exit()
{
	TRACE(printf("[%s] cproc_exit()\n", cthread_name(cthread_self())));
	/*
	 * Terminate thread.
	 * Note that the stack is never freed.
	 */
	(void) thread_terminate(thread_self());
}
#endif	unused

/*
 * Implement C threads using MACH threads.
 */
#if	NeXT
thread_t
#else
void
#endif	NeXT
cproc_create()
{
	register cproc_t child = cproc_alloc();
	register kern_return_t r;
	extern void cproc_setup();
#if defined(NeXT) && !defined(SHLIB)
      	extern void set_malloc_singlethreaded(int single);
#endif
        alloc_stack(child);
	MACH_CALL(thread_create(task_self(), &child->id), r);
	cproc_setup(child);	/* machine dependent */
#if defined(NeXT) && !defined(SHLIB)
        set_malloc_singlethreaded(0);
#endif
        MACH_CALL(thread_resume(child->id), r);
	TRACE(print_all_cprocs());
#if	NeXT
	return(child->id);
#endif	NeXT
}

#ifdef	NeXT
/*
 * Global data moved to threads_data.c for SHLIB purposes.
 */
extern int condition_spin_limit;
extern int condition_yield_limit;
#else	NeXT
int condition_spin_limit = 0;
int condition_yield_limit = 7;
#endif	NeXT

void
condition_wait(c, m)
	register condition_t c;
	mutex_t m;
{
	register cproc_t p;
	register int i;
	register kern_return_t r;
	msg_header_t msg;

	TRACE(printf("[%s] wait(%s,%s)\n",
		     cthread_name(cthread_self()),
		     condition_name(c), mutex_name(m)));

	p = cproc_self();
	spin_lock(&c->lock);
	p->state = CPROC_SPINNING;
	cthread_queue_enq(&c->queue, p);
	TRACE(print_cproc_queue("wait", &c->queue));
	spin_unlock(&c->lock);

	/*
	 * Release the mutex while we wait for the condition.
	 */
	mutex_unlock(m);

#if	SCHED_HINT
	/*
	 * First spin, yielding the processor.
	 * If swtch_pri() returns TRUE, be a good citizen and block.
	 */
	do {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
	} while (! swtch_pri(0));
#else
	/*
	 * First, try busy-waiting.
	 */
	for (i = 0; i < condition_spin_limit; i += 1) {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
	}
	/*
	 * Next, try yielding the processor.
	 */
	for (i = 0; i < condition_yield_limit; i += 1) {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
		(void) swtch_pri(0);
	}
#endif	SCHED_HINT
	spin_lock(&c->lock);
	/*
	 * Check again to avoid race.
	 */
	if (p->state == CPROC_RUNNING) {
		/*
		 * We've been woken up.
		 */
		spin_unlock(&c->lock);
		goto done;
	}
	/*
	 * The kernel has someone else to run, so we block.
	 */
	p->state = CPROC_BLOCKED;
	msg.msg_size = sizeof(msg);
	msg.msg_local_port = p->wait_port;
	TRACE(printf("[%s] receive(%x)\n",
		     cthread_name(cthread_self()), p->wait_port));
	spin_unlock(&c->lock);
	MACH_CALL(msg_receive(&msg, MSG_OPTION_NONE, 0), r);

done:

	ASSERT(p->state == CPROC_RUNNING);
	/*
	 * Re-acquire the mutex and return.
	 */
	mutex_lock(m);
}

/*
 * Continue a waiting thread.
 * Called from signal or broadcast with the condition variable locked.
 */
private void
cproc_continue(p, c)
	register cproc_t p;
	register condition_t c;
{
	register int old_state;
	register kern_return_t r;
	msg_header_t msg;
	spin_lock(&c->lock);
	old_state = p->state;

	p->state = CPROC_RUNNING;
	spin_unlock(&c->lock);
	/*
	 * If thread is just spinning,
	 * setting its state to CPROC_RUNNING
	 * will suffice to wake it up.
	 */
	if (old_state != CPROC_BLOCKED)
		return;
	msg.msg_simple = TRUE;
	msg.msg_size = sizeof(msg);
	msg.msg_type = MSG_TYPE_NORMAL;
	msg.msg_local_port = PORT_NULL;
	msg.msg_remote_port = p->wait_port;
	msg.msg_id = 0;
	TRACE(printf("[%s] send(%x)\n",
		     cthread_name(cthread_self()), p->wait_port));
	r = msg_send(&msg, SEND_TIMEOUT, 0);
	if (r != SEND_SUCCESS && r != SEND_TIMED_OUT) {
		mach_error("msg_send", r);
		ASSERT(SHOULDNT_HAPPEN);
		exit(1);
	}
}

void
cond_signal(c)
	register condition_t c;
{
	register cproc_t p;

	TRACE(printf("[%s] signal(%s)\n",
		     cthread_name(cthread_self()), condition_name(c)));
	TRACE(print_cproc_queue("signal", &c->queue));

	spin_lock(&c->lock);
	cthread_queue_deq(&c->queue, cproc_t, p);
	spin_unlock(&c->lock);
	if (p != NO_CPROC)
		cproc_continue(p, c);
}

void
cond_broadcast(c)
	register condition_t c;
{
	register cproc_t p;
	struct cthread_queue queue;

	TRACE(printf("[%s] broadcast(%s)\n",
		     cthread_name(cthread_self()), condition_name(c)));
	TRACE(print_cproc_queue("broadcast", &c->queue));

	spin_lock(&c->lock);
	queue = c->queue;
	cthread_queue_init(&c->queue);
	spin_unlock(&c->lock);
	for (;;) {
		cthread_queue_deq(&queue, cproc_t, p);
		if (p == NO_CPROC)
			break;
		cproc_continue(p, c);
	}
}

int condition_wait_timeout(condition_t c, mutex_t m, unsigned int millisecs)
{
	register cproc_t p;
        register kern_return_t r;
        int messaged = 0;
	msg_header_t msg;
	    
	p = cproc_self();
	spin_lock(&c->lock);
        cthread_queue_enq(&c->queue, p);
	p->state = CPROC_BLOCKED;
	spin_unlock(&c->lock);
        mutex_unlock(m);
eat:
	msg.msg_size = sizeof(msg);
	msg.msg_local_port = p->wait_port;
        r = msg_receive(&msg, RCV_TIMEOUT, millisecs);
	if (-21 == r) goto eat;		// see 60870
	if (r == RCV_TIMED_OUT) {
		/* re-establish our state */
                struct cthread_queue queue;
                cproc_t item;
                
                if (p->state == CPROC_EATING) {
                    /* someone dequeued us & msg'ed us! */
                    goto eat;
                }
		spin_lock(&c->lock);
                if (p->state == CPROC_EATING) {
                    /* great, someone dequeued us already! */
                    spin_unlock(&c->lock);
                    goto eat;
                }
                /* dequeue ourself */
                queue = c->queue;
                cthread_queue_init(&c->queue);
                for (;;) {
                    cthread_queue_deq(&queue, cproc_t, item);
                    if (item == (cproc_t)0) break;
                    if (item != p) cthread_queue_enq(&c->queue, item);
                }
                messaged = 0;
		p->state = CPROC_RUNNING;
		spin_unlock(&c->lock);
	}
        else if (r == RCV_SUCCESS) {
            	// ASSERT(p->state == CPROC_EATING);
		p->state = CPROC_RUNNING;
                messaged = 1;
        }
        else {
            mach_error("msg_receive", r);
            ASSERT(SHOULDNT_HAPPEN);
            abort();
        }

	/*
	 * Re-acquire the mutex and return.
	 */
	mutex_lock(m);
	return messaged;
}

/*
 * Continue a waiting thread.
 * Called from signal or broadcast with the condition variable locked.
 */
static void cproc_continue_timeout(cproc_t p) {
	register kern_return_t r;
	msg_header_t msg;

	msg.msg_simple = TRUE;
	msg.msg_size = sizeof(msg);
	msg.msg_type = MSG_TYPE_NORMAL;
	msg.msg_local_port = PORT_NULL;
	msg.msg_remote_port = p->wait_port;
	msg.msg_id = 0;
	r = msg_send(&msg, SEND_TIMEOUT, 0);
	if (r != SEND_SUCCESS && r != SEND_TIMED_OUT) {
            mach_error("msg_send", r);
            ASSERT(SHOULDNT_HAPPEN);
            abort();
	}
}

void condition_signal_timeout(condition_t c) {
	register cproc_t p;

        if (!c->queue.head) return;
	spin_lock(&c->lock);
	cthread_queue_deq(&c->queue, cproc_t, p);
        if (p) {
                p->state = CPROC_EATING;
                spin_unlock(&c->lock);
                cproc_continue_timeout(p);
        }
        else
		spin_unlock(&c->lock);
}


static void dequeue_unlock_send(condition_t c) {
	register cproc_t p;

        cthread_queue_deq(&c->queue, cproc_t, p);
        if (p) {
		p->state = CPROC_EATING;
            	dequeue_unlock_send(c);
        	cproc_continue_timeout(p);
        }
        else
		spin_unlock(&c->lock);
}

void condition_broadcast_timeout(condition_t c) {
        if (!c->queue.head) return;
        // for each element on the queue we need to
        //	1) remove it
        //	2) mark it EATING
        // (while the lock is held)
        //	3) send a msg to it
        // We could copy c->queue, mark EATING by iterating, unlock
        // and then re-iterate and send, but there are no non-destructive macros
        // to iterate.  Instead, we remove and mark it via recursion and send on the way
        // back from recursion.
	spin_lock(&c->lock);
        dequeue_unlock_send(c);
}

void
cthread_yield()
{
	/* let the kernel reschedule us */
	swtch_pri(0);
}

/*
 * Routines for supporting fork() of multi-threaded programs.
 */

#if	NeXT
void _cproc_fork_child()
#else
void cproc_fork_child()
#endif	NeXT
/*
 * Called in the child after a fork().  Resets cproc data structures to
 * coincide with the reality that we now have a single cproc and cthread.
 */
{
	cproc_t p, n, self;
	kern_return_t r;

	/*
	 * From cprocs.c ...
	 */

	cproc_lock = 0;		/* unlocked */

#if	NeXT
	/*
	 * Fix for the case where the first cproc in "cprocs" is not
	 * cproc_self() and we go onto dealloc_stack() which does a 
	 * vm_deallocate.  The vm_deallocate will then fail because the
	 * reply port is invalid (SEND_INVALID_PORT).
	 */
	self = cproc_self();	// pointer stashed in kernel thread structure
	self->reply_port = PORT_NULL;
#endif	NeXT
	/*
	 * Free all cprocs.
	 */
	for (p = cprocs; p != NO_CPROC; p = n) {
#if NeXT
		if (cproc_self() == p) {	/* Found ourself */
#else
		if (p == self) {	/* found our parent's thread cproc */
#endif
			/*
			 * From cproc_alloc() ...
			 */
			p->state = CPROC_RUNNING;
			p->reply_port = PORT_NULL;


			MACH_CALL(port_allocate(task_self(), &p->wait_port), r);
			MACH_CALL(port_set_backlog(task_self(), p->wait_port, 1), r);

			cprocs = p;		/* We become the only cproc */
			n = p->link;		/* No next one */
			p->link = NO_CPROC;	/* No more cprocs */

			/*
			 * From cproc_init() ...
			 */
#if	NeXT
			/*
			 * reset our notion of our thread_self port
			 */
			p->id = thread_self();
#endif	NeXT

			continue;		/* Don't deallocate self */
		}

		if (p->incarnation != NO_CPROC) {
			free((char *) p->incarnation);	/* Free cthread */
		}

		/*
		 * Deallocate the cproc's stack.  Note that this destroys any
		 * local variables which might have been there, including argc,
		 * argv, and envp if the forking cproc was not the main one.
		 */
#if	NeXT
		/* We no longer do this as of Release 3.2 */
#else
		dealloc_stack(p);
#endif	NeXT

		n = p->link;
		free((char *) p);	/* Free cproc */
	}
}

#if	NeXT
/*
 *	Support for a per-thread UNIX errno.
 */

void cthread_set_errno_self(error)
	int	error;
{
	cproc_t	c;

	c = cproc_self();
	if (c)
		c->error = error;
}

int cthread_errno()
{
	cproc_t	c;
	extern int errno;

	c = cproc_self();
	if (c)
		return(c->error);
	else
		return(errno);
}
#endif	NeXT

