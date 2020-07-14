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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#define TIMER_DEBUG	0

#include <mach/cthreads.h>
#include <mach/mach.h>
#include <mach/message.h>

#include "debug.h"
#include "lock_queue.h"
#include "lock_queue_macros.h"
#include "ls_defs.h"
#include "mem.h"
#include "timer.h"
#include "netmsg.h"
#include "trace.h"

#define TIMER_QUANTUM	param.timer_quantum


static struct lock_queue	timer_queue;		/* Structure used to queue timers. */
static struct mutex		timer_lock;		/* Lock for timer global values. */
static port_t			timer_port;		/* Port used to wait for messages. */
static long			timer_time;		/* The time that timer_run uses. */

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_TIMER;



/*
 * GET_ABSOLUTE_TIME
 *	Adds now to the interval of timer t and assigns the result to the deadline of t.
 */
#define GET_ABSOLUTE_TIME(t) (t)->deadline.tv_sec = timer_time + ((t)->interval.tv_sec << 1)



/*
 * timer_run
 *	main loop of timer package.
 *
 * Design:
 *	Loops doing the following:
 *		Tries to get the first item on the timer queue;
 *		If there is nothing there then sleeps for TIMER_QUANTUM using msg_receive
 *		else if the first item is expired then do it.
 *
 */
void timer_run (void)
{
	register nmtimer_t	first_timer;
	boolean_t		removed;
	register msg_header_t	*timer_msg_ptr;
	msg_header_t		timer_msg;

	timer_msg_ptr = &timer_msg;

	while (1) {
		mutex_lock(&timer_queue.lock);
		first_timer = (nmtimer_t)timer_queue.head;
		mutex_unlock(&timer_queue.lock);
		if ((first_timer) && (timer_time >= first_timer->deadline.tv_sec)) {
			lq_remove_macro(&timer_queue, first_timer, removed);
			if (removed) {
				(first_timer->action)(first_timer);
			}
		}
		else {
			/*
			 * Sleep for TIMER_INTERVAL.
			 */
			timer_msg_ptr->msg_size = sizeof(msg_header_t);
			timer_msg_ptr->msg_local_port = timer_port;
			(void)msg_receive(timer_msg_ptr, RCV_TIMEOUT, TIMER_QUANTUM);
			mutex_lock(&timer_lock);
			timer_time++;
			mutex_unlock(&timer_lock);
		}
	}

}



/*
 * timer_wake_up
 *	Sends a wake-up message to the timer thread.
 *
 */
void timer_wake_up()
{
	kern_return_t	kr;
	msg_header_t	wake_up_msg;

	wake_up_msg.msg_simple = TRUE;
	wake_up_msg.msg_type = MSG_TYPE_NORMAL;
	wake_up_msg.msg_size = sizeof(msg_header_t);
	wake_up_msg.msg_remote_port = timer_port;
	wake_up_msg.msg_local_port = PORT_NULL;
	wake_up_msg.msg_id = 1111;

	kr = msg_send(&wake_up_msg, SEND_TIMEOUT, 0);
	if (kr != KERN_SUCCESS) {
	    if (kr == SEND_TIMED_OUT) {
		ERROR((msg, "timer_wake_up.msg_send timed out."));
	    }
	    else {
		ERROR((msg, "timer_wake_up.msg_send fails, kr = %d.", kr));
	    }
	}

	RET;
}



/*
 * insert_timer_test
 *	compares two timers - returns TRUE if t has a closer deadline than cur.
 */
#define insert_timer_test(cur,t,args) \
	(((nmtimer_t)(t))->deadline.tv_sec < ((nmtimer_t)(cur))->deadline.tv_sec)


/*
 * TIMER_START:
 *	If timer t is not already on the timer queue
 *	then the absolute deadline of t is computed and t is inserted in the timer queue.
 *
 * Note:
 *	Assumes that timer is NOT on the queue - timer_restart should be used if it is.
 */
void timer_start(t)
	register nmtimer_t	t;
{

	mutex_lock(&timer_lock);
	GET_ABSOLUTE_TIME(t);
	mutex_unlock(&timer_lock);
	lq_insert_macro(&timer_queue, insert_timer_test, (cthread_queue_item_t)t, 0);
	RET;

}



/*
 * TIMER_STOP:
 *	If timer t is present on the queue then it is removed from the queue.
 *	Returns whether there was a timer to be removed or not.
 */
boolean_t timer_stop(t)
	register nmtimer_t	t;
{
	boolean_t	ret;

	lq_remove_macro(&timer_queue, (cthread_queue_item_t)t, ret);
	if (ret) {
		RETURN(TRUE);
	} 
	else {
		RETURN(FALSE);
	}

}



/*
 * TIMER_RESTART:
 *	If timer t is present on the queue then it is removed.
 *	Queues timer t up as in timer_start.
 */
void timer_restart(t)
	register nmtimer_t	t;
{
	boolean_t	ret;

	lq_remove_macro(&timer_queue, (cthread_queue_item_t)t, ret);
	mutex_lock(&timer_lock);
	GET_ABSOLUTE_TIME(t);
	mutex_unlock(&timer_lock);
	lq_insert_macro(&timer_queue, insert_timer_test, (cthread_queue_item_t)t, 0);

	RET;

}




/*
 * TIMER_ALLOC:
 *	allocates space for a timer and initialises it.
 */
nmtimer_t timer_alloc()
{
	register nmtimer_t	new_timer;

	MEM_ALLOCOBJ(new_timer,nmtimer_t,MEM_TIMER);
	new_timer->link = (struct timer *)0;
	new_timer->interval.tv_sec = 0;
	new_timer->interval.tv_usec = 0;
	new_timer->action = (void (*)())0;
	new_timer->info = (char *)0;
	new_timer->deadline.tv_sec = 0;
	new_timer->deadline.tv_usec = 0;
	RETURN(new_timer);
}



/*
 * TIMER_INIT:
 *	Initilizes the timer package.
 *	Allocates the port used for wake-up messages.
 *	Creates a thread which waits for timers to expire.
 */
boolean_t timer_init()
{
	kern_return_t	kr;
	cthread_t	new_thread;

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_TIMER,"Timer record",sizeof(struct timer),
								FALSE,140,50);


	timer_time = 0;
	lq_init(&timer_queue);
	mutex_init(&timer_lock);

	if ((kr = port_allocate(task_self(), &timer_port)) != KERN_SUCCESS) {
		ERROR((msg, "timer_init.port_allocate fails, kr = %d.\n", kr));
		RETURN(FALSE);
	}

	new_thread = cthread_fork((cthread_fn_t)timer_run, (any_t)0);
	cthread_set_name(new_thread, "timer_run");
	cthread_detach(new_thread);

	RETURN(TRUE);
}



/*
 * timer_cthread_exit
 *	just calls cthread_exit.
 *
 */
void timer_cthread_exit (void) {

    cthread_exit((any_t)0);
}


/*
 * timer_always_true
 *	used when calling lq_find_in_queue.
 *
 */
#define timer_always_true(item,args) TRUE


/*
 * TIMER_KILL:
 *	Terminates the background timer thread by tricking it into suicide.
 *	Note that the timer thread may not be terminated upon return --
 *	the purpose of this routine is to clean up when a program is
 *	ready to terminate so that the threads package will not dump core.
 */
void timer_kill()
{
	static struct timer	suicide;
	cthread_queue_item_t		ret;

	do {
		cthread_yield();
		lq_find_macro(&timer_queue, timer_always_true, 0, ret);
	} while (ret);

	suicide.interval.tv_sec  =    0L;   /* An arbitrary time which is */
	suicide.interval.tv_usec = 1000L;   /* instantaneous to humans... */
	suicide.action = (void (*)())timer_cthread_exit;
	timer_start(&suicide);
	RET;
}

