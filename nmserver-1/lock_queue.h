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

#include <mach/mach.h>
#include <mach/cthreads.h>
#include <mach/boolean.h>

#include "mem.h"

#ifndef _LOCK_QUEUE_
#define _LOCK_QUEUE_


#ifdef NeXT_PDO

/*
 * Taken from the cthreads header on NEXTSTEP.
 */

typedef struct cthread_queue {
    struct cthread_queue_item *head;
    struct cthread_queue_item *tail;
} *cthread_queue_t;

typedef struct cthread_queue_item {
    struct cthread_queue_item *next;
} *cthread_queue_item_t;

#define	NO_QUEUE_ITEM ((cthread_queue_item_t) 0)

#define	QUEUE_INITIALIZER { NO_QUEUE_ITEM, NO_QUEUE_ITEM }

#define	cthread_queue_alloc() \
    ((cthread_queue_t) calloc(1, sizeof(struct cthread_queue)))

#define	cthread_queue_init(q) ((q)->head = (q)->tail = 0)
#define	cthread_queue_free(q) free((any_t) (q))

#define	cthread_queue_enq(q, x) \
    do { \
        (x)->next = 0; \
        if ((q)->tail == 0) \
            (q)->head = (cthread_queue_item_t) (x); \
        else \
            (q)->tail->next = (cthread_queue_item_t) (x); \
        (q)->tail = (cthread_queue_item_t) (x); \
    } while (0)

#define	cthread_queue_preq(q, x) \
    do { \
        if ((q)->tail == 0) \
            (q)->tail = (cthread_queue_item_t) (x); \
        ((cthread_queue_item_t) (x))->next = (q)->head; \
        (q)->head = (cthread_queue_item_t) (x); \
    } while (0)

#define	cthread_queue_head(q, t) ((t) ((q)->head))

#define	cthread_queue_deq(q, t, x) \
    do { \
        if (((x) = (t) ((q)->head)) != 0 \
            && ((q)->head = (cthread_queue_item_t) ((x)->next)) == 0) \
            (q)->tail = 0; \
    } while (0)

#define	cthread_queue_map(q, t, f) \
    do { \
        register cthread_queue_item_t x, next; \
        for (x = (cthread_queue_item_t) ((q)->head); x != 0; x = next) { \
            next = x->next; \
            (*(f))((t) x); \
        } \
    } while (0)

#endif NeXT_PDO


typedef struct lock_queue {
    struct mutex		lock;
    cthread_queue_item_t	head;
    cthread_queue_item_t	tail;
    struct condition		nonempty;
} *lock_queue_t;

#define LOCK_QUEUE_NULL	(lock_queue_t)0



/*
 * LQ_ALLOC:
 *	Allocates data space for the lock and the nonempty condition field.
 *	Calls lq_init to initialize the queue.
 */
extern lock_queue_t lq_alloc();


/*
 * LQ_INIT:
 *	Initializes the head and the tail of the queue to nil.
 *	Initialises the lock and condition of the queue.
 */
void lq_init(/*q*/);
/*
lock_queue_t	q;
*/



/*
 * LQ_PREQUEUE:
 *	Inserts queue_item at the head of the queue.
 *	Locks queue while accessing queue.
 *	Signals queue nonempty.
 */
extern void lq_prequeue(/*q, x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_ENQUEUE:
 *	Enters queue item at the tail of the queue.
 *	Locks queue while accessing queue.
 */
extern void lq_enqueue(/*q, x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


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
extern void lq_insert_in_queue(/*q, test, x, args*/);
/*
lock_queue_t		q;
int			(*test)();
cthread_queue_item_t	x;
int			args;
*/



/*
 * LQ_DEQUEUE:
 *	If queue is not empty then removes item from the head of the queue.
 *	Locks queue while accessing queue.
 */
extern cthread_queue_item_t lq_dequeue(/*q*/);
/*
lock_queue_t	q;
*/


/*
 * LQ_BLOCKING_DEQUEUE:
 *	If the queue is empty, a wait is done on the nonempty condition.
 *	Removes item from the head of the queue.
 *	Locks queue while accessing queue
 */
extern cthread_queue_item_t lq_blocking_dequeue(/*q*/);
/*
lock_queue_t	q;
*/
	

/*
 * LQ_REMOVE_FROM_QUEUE:
 *	Removes the queue_item from the queue if it is present on the queue.
 *	Returns whether the item was deleted from the queue.
 */
extern boolean_t lq_remove_from_queue(/*q,x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


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
extern cthread_queue_item_t lq_cond_delete_from_queue(/*q, test, args*/);
/*
lock_queue_t	q;
int		(*test)();
int		args;
*/



/*
 * LQ_ON_QUEUE:
 *	Locks queue while accessing queue.
 *	Checks to see if the cthread_queue_item_t is on the queue,
 *	if so returns true else returns false.
 */
extern boolean_t lq_on_queue(/*q,x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_FIND_IN_QUEUE:
 *	Returns a cthread_queue_item_t which is found by the function test.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	If no cthread_queue_item_t is found returns nil.
 *	Locks queue while accessing queue
 */
extern cthread_queue_item_t lq_find_in_queue(/*q, fn, args*/);
/*
lock_queue_t	q;
int		(*test)();
int		args;
*/


/*
 * LQ_MAP_QUEUE:
 *	Maps fn() onto each item on the queue;
 *	The parameters passed to the map function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue
 */
extern void lq_map_queue(/*q, fn, args*/);
/*
lock_queue_t	q;
int		(*fn)();
int		args;
*/

/*
 * The following routines are identical to their corresponding routines above,
 * but they expect the lock on the queue to be held before they are called, and
 * do not interact with this lock directly.
 */
extern void lqn_prequeue();
extern cthread_queue_item_t lqn_cond_delete_from_queue();
extern cthread_queue_item_t lqn_find_in_queue();

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_LQUEUE;


#endif _LOCK_QUEUE_

