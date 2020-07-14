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

/*
 * The functions contained in this file are
 * lq_alloc();
 * lq_init();
 * lq_prequeue();
 * lq_enqueue();
 * lq_insert_in_queue();
 * lq_dequeue();
 * lq_blocking_dequeue();
 * lq_remove_from_queue();
 * lq_cond_delete_from_queue();
 * lq_on_queue();
 * lq_find_in_queue();
 * lq_map_queue();
 */


#define DEBUGOFF	1
#define	NO_CONDITION	1

#include <mach/mach.h>
#include <mach/cthreads.h>
#include <stdio.h>
#include <mach/boolean.h>

#include "netmsg.h"
#include "debug.h"
#include "trace.h"
#include "mem.h"
#include "lock_queue.h"

#ifdef NeXT_PDO
#ifndef ASSERT
#define ASSERT(cond)
#endif
#endif

#if	NO_CONDITION
#ifdef	condition_signal
#undef	condition_signal
#endif	condition_signal
#define condition_signal(foo)
#endif	NO_CONDITION

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_LQUEUE;


/*
 * LQ_ALLOC:
 *	Allocates data space for the lock and the nonempty condition field.
 *	Calls lq_init to initialize the queue.
 */
lock_queue_t lq_alloc()
{
	register lock_queue_t	q;

	MEM_ALLOCOBJ(q,lock_queue_t,MEM_LQUEUE);
	lq_init(q);
	RETURN(q);
}


/*
 * LQ_INIT:
 *	Initializes the head and the tail of the queue to nil.
 *	Initialises the lock and condition of the queue.
 */
void lq_init(q)
	register lock_queue_t	q;
{
        q->head = 0;
        q->tail = 0;
        condition_init(&(q->nonempty));
	mutex_init(&(q->lock));
	RET;
}



/*
 * LQ_PREQUEUE:
 *	Inserts queue_item at the head of the queue.
 *	Locks queue while accessing queue.
 *	Signals queue nonempty.
 */
void lq_prequeue(q, x)
	register lock_queue_t		q;
	register cthread_queue_item_t	x;
{
    	mutex_lock(&(q->lock));
	if (q->tail == 0) {
		q->head = q->tail = x;
		x->next = 0;
	} else {
		x->next = q->head;
		q->head = x;
	}
    	mutex_unlock(&(q->lock));
	condition_signal(&(q->nonempty));
	RET;
}

void lqn_prequeue(q, x)
	register lock_queue_t		q;
	register cthread_queue_item_t	x;
{
	if (q->tail == 0) {
		q->head = q->tail = x;
		x->next = 0;
	} else {
		x->next = q->head;
		q->head = x;
	}
/*	condition_signal(&(q->nonempty)); */
	RET;
}


/*
 * LQ_ENQUEUE:
 *	Enters queue item at the tail of the queue.
 *	Locks queue while accessing queue.
 */
void lq_enqueue(q, x)
	register lock_queue_t		q;
	register cthread_queue_item_t	x;	
{
	x->next = 0;
	mutex_lock(&(q->lock));
	if (q->tail == 0)
		q->head = q->tail = x;
	else {
		q->tail->next = x;
		q->tail = x;
	}
	mutex_unlock(&(q->lock));
	condition_signal(&(q->nonempty));
	RET;
}


/*
 * LQ_INSERT_IN_QUEUE:
 *	Inserts queue_item in the correct positiion on the queue.
 *	Does so by calling a test function to do a comparison.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		x	- the queue_item passed in, and
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue.
 *	Signals queue nonempty.
 */
void lq_insert_in_queue(q, test, x, args)
	register lock_queue_t		q;
	int				(*test)();
	register cthread_queue_item_t	x;
	int				args;
{
	register cthread_queue_item_t	prev, cur;

	mutex_lock(&(q->lock)); 
	cur = q->head;
	if (!cur){
		q->head = q->tail = x;
		x->next = 0;
		mutex_unlock(&(q->lock)); 
		condition_signal(&(q->nonempty));
		RET;
	}
	if ((*test)(cur, x,args)) {
		x->next = q->head;
		q->head = x;
		mutex_unlock(&(q->lock)); 
		condition_signal(&(q->nonempty));
		RET;
	}
	do{
		prev = cur;
		cur = cur->next;
		if ((cur) && ((*test)(cur, x, args))) {
			prev->next = x;
			x->next = cur;
			mutex_unlock(&(q->lock)); 
			condition_signal(&(q->nonempty));
			RET;
		}
	} while (cur != 0);
	ASSERT(prev == q->tail);
	prev->next = x;
	q->tail = x;
	x->next = 0;

	mutex_unlock(&(q->lock)); 
	condition_signal(&(q->nonempty));
	RET;
}



/*
 * LQ_DEQUEUE:
 *	If queue is not empty then removes item from the head of the queue.
 *	Locks queue while accessing queue.
 */
cthread_queue_item_t lq_dequeue(q)
	register lock_queue_t		q;
{
	register cthread_queue_item_t	x;

	mutex_lock(&(q->lock));
	x = q->head;
	if (x != 0) {
		if ((q->head = x->next) == 0)
			q->tail = 0;
		else
			x->next = 0;
	}
	mutex_unlock(&(q->lock));
	RETURN(x);
}


#if	NO_CONDITION
#else	NO_CONDITION
/*
 * LQ_BLOCKING_DEQUEUE:
 *	If the queue is empty, a wait is done on the nonempty condition.
 *	Removes item from the head of the queue.
 *	Locks queue while accessing queue
 */
cthread_queue_item_t lq_blocking_dequeue(q)
	register lock_queue_t		q;
{
	register cthread_queue_item_t	x;

	mutex_lock(&(q->lock));

	while ((x = q->head) == 0){
		condition_wait(&(q->nonempty),&(q->lock));
		}
	if ((q->head = x->next) == 0)
		q->tail = 0;
	else
		x->next = 0;

	mutex_unlock(&(q->lock));
	RETURN(x);
}
#endif	NO_CONDITION


/*
 * LQ_REMOVE_FROM_QUEUE:
 *	Removes the queue_item from the queue if it is present on the queue.
 *	Returns whether the item was deleted from the queue.
 */
boolean_t lq_remove_from_queue(q,x)
	register lock_queue_t		q;
	register cthread_queue_item_t	x;
{
	register cthread_queue_item_t	prev, cur;

	mutex_lock(&(q->lock)); 
	for (prev = cur = q->head; cur != 0; prev = cur, cur = cur->next)
		if (x == cur) {
			if (cur == q->head){
				if ((q->head = cur->next) == 0)
					q->tail = 0;
				else
					cur->next = 0;
			}
			else{
				if ((prev->next = cur->next) == 0) {
					ASSERT(cur == q->tail);
					q->tail = prev;
				} else
					cur->next = 0;
			}
			mutex_unlock(&(q->lock));	
			RETURN(TRUE);
		}
	mutex_unlock(&(q->lock));	
	RETURN(FALSE);
}
			

/*
 * LQ_COND_DELETE_FROM_QUEUE:
 *	Performs the test function with each element of the queue, 
 *	until the function returns true, or the tail of the queue is reached.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	The item is then removed from the queue.
 *	Locks queue while accessing queue.
 *	Returns the item that was deleted from the queue.
 */
cthread_queue_item_t lq_cond_delete_from_queue(q, test, args)
	register lock_queue_t		q;
	int				(*test)();
	int				args;
{
	register cthread_queue_item_t	prev, cur;

	mutex_lock(&(q->lock)); 
	for (prev = cur = q->head; cur != 0; prev = cur, cur = cur->next)
		if ((*test)(cur, args)) {
			if (cur == q->head){
				if ((q->head = cur->next) == 0)
					q->tail = 0;
				else
					cur->next = 0;
			}
			else{
				if ((prev->next = cur->next) == 0) {
					ASSERT(cur == q->tail);
					q->tail = prev;
				} else
					cur->next = 0;
			}
			mutex_unlock(&(q->lock));	
			RETURN(cur);
		}
	mutex_unlock(&(q->lock));
	RETURN((cthread_queue_item_t)0);
}

cthread_queue_item_t lqn_cond_delete_from_queue(q, test, args)
	register lock_queue_t	q;
	int			(*test)();
	int			args;
{
	register cthread_queue_item_t	prev, cur;

	for (prev = cur = q->head; cur != 0; prev = cur, cur = cur->next)
		if ((*test)(cur, args)) {
			if (cur == q->head){
				if ((q->head = cur->next) == 0)
					q->tail = 0;
				else
					cur->next = 0;
			}
			else{
				if ((prev->next = cur->next) == 0) {
					ASSERT(cur == q->tail);
					q->tail = prev;
				} else
					cur->next = 0;
			}
			RETURN(cur);
		}
	RETURN((cthread_queue_item_t)0);
}



/*
 * LQ_ON_QUEUE:
 *	Locks queue while accessing queue.
 *	Checks to see if the cthread_queue_item_t is on the queue,
 *	if so returns true else returns false.
 */
boolean_t lq_on_queue(q,x)
	register lock_queue_t		q;
	register cthread_queue_item_t	x;
{
	register cthread_queue_item_t	cur;

	mutex_lock(&(q->lock));	
	for (cur = q->head; cur != 0;cur = cur->next){
		if (cur == x){
			mutex_unlock(&(q->lock));
			RETURN(TRUE);
		}
	}
	mutex_unlock(&(q->lock));
	RETURN(FALSE);
}


/*
 * LQ_FIND_IN_QUEUE:
 *	Returns a cthread_queue_item_t which is found by the function test.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	If no cthread_queue_item_t is found returns nil.
 *	Locks queue while accessing queue
 */
cthread_queue_item_t lq_find_in_queue(q, test, args)
	register lock_queue_t	q;
	register int		(*test)();
	register int		args;
{
	register cthread_queue_item_t	cur;

	mutex_lock(&(q->lock));
	for (cur = q->head; cur != 0; cur = cur->next)
		if ((*test)(cur, args)){
			mutex_unlock(&(q->lock));
			RETURN(cur);
		}
	mutex_unlock(&(q->lock));
	RETURN((cthread_queue_item_t)0);
}

cthread_queue_item_t lqn_find_in_queue(q, test, args)
	register lock_queue_t	q;
	register int		(*test)();
	register int		args;
{
	register cthread_queue_item_t	cur;

	for (cur = q->head; cur != 0; cur = cur->next)
		if ((*test)(cur, args)){
			RETURN(cur);
		}
	RETURN((cthread_queue_item_t)0);
}


/*
 * LQ_MAP_QUEUE:
 *	Maps fn() onto each item on the queue;
 *	The parameters passed to the map function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue
 */
void lq_map_queue(q, fn, args)
	register lock_queue_t	q;
	register int		(*fn)();
	register int		args;
{
	register cthread_queue_item_t	cur;

	mutex_lock(&(q->lock));
	for (cur = q->head; cur != 0; cur = cur->next)
		(*fn)(cur, args);
	mutex_unlock(&(q->lock));
	RET;
}


/*
 * lock_queue_init --
 *
 * Initialize the lock_queue package.
 */
EXPORT boolean_t lock_queue_init()
{

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_LQUEUE,"Locked queue head",sizeof(struct lock_queue),
								FALSE,125,50);

	RETURN(TRUE);
}

