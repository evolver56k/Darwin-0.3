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

#ifndef	_LOCK_QUEUE_MACROS_
#define	_LOCK_QUEUE_MACROS_ 1

#ifdef NeXT_PDO
#ifndef ASSERT
#define ASSERT(cond)
#endif
#endif


/*
 * LQ_PREQUEUE:
 *	Inserts queue_item at the head of the queue.
 *	Locks queue while accessing queue.
 */
#define lq_prequeue(q, x)						\
{									\
    	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	if (((lock_queue_t)(q))->tail == 0) {				\
		((lock_queue_t)(q))->head = 				\
			((lock_queue_t)(q))->tail = 			\
					(cthread_queue_item_t)(x);	\
		((cthread_queue_item_t)(x))->next = 0;			\
	} else {							\
		((cthread_queue_item_t)(x))->next = 			\
					((lock_queue_t)(q))->head;	\
		((lock_queue_t)(q))->head = (cthread_queue_item_t)(x);	\
	}								\
    	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}

#define lqn_prequeue(q, x)						\
{									\
	if (((lock_queue_t)(q))->tail == 0) {				\
		((lock_queue_t)(q))->head = 				\
			((lock_queue_t)(q))->tail = 			\
					(cthread_queue_item_t)(x);	\
		((cthread_queue_item_t)(x))->next = 0;			\
	} else {							\
		((cthread_queue_item_t)(x))->next = 			\
					((lock_queue_t)(q))->head;	\
		((lock_queue_t)(q))->head = (cthread_queue_item_t)(x);	\
	}								\
}


/*
 * LQ_ENQUEUE:
 *	Enters queue item at the tail of the queue.
 *	Locks queue while accessing queue.
 */
#define lq_enqueue(q, x)						\
{									\
	((cthread_queue_item_t)(x))->next = 0;				\
	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	if (((lock_queue_t)(q))->tail == 0)				\
		((lock_queue_t)(q))->head = 				\
		((lock_queue_t)(q))->tail = ((cthread_queue_item_t)(x));\
	else {								\
		((lock_queue_t)(q))->tail->next = 			\
					((cthread_queue_item_t)(x)); 	\
		((lock_queue_t)(q))->tail = ((cthread_queue_item_t)(x));\
	}								\
	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}

/*
 * LQ_INSERT_MACRO:
 *	Inserts queue_item in the correct positiion on the queue.
 *	Does so by calling a test function to do a comparison.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		x	- the queue_item passed in, and
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue.
 */
#define lq_insert_macro(q,test,x,args) {				\
	register cthread_queue_item_t	prev, cur;			\
	mutex_lock(&((q)->lock));					\
	cur = (q)->head;						\
	if (!cur){							\
		(q)->head = (q)->tail = (x);				\
		(x)->next = 0;						\
	}								\
	else if (test(cur, (x), (args))) {				\
		(x)->next = (q)->head;					\
		(q)->head = (x);					\
	}								\
	else {								\
		do {							\
			prev = cur;					\
			cur = cur->next;				\
			if ((cur) && (test(cur, (x), (args)))) {	\
				prev->next = (x);			\
				(x)->next = cur;			\
				cur = 0;				\
			}						\
		} while (cur != 0);					\
		if (prev == (q)->tail) {				\
			prev->next = (x);				\
			(q)->tail = (x);				\
			(x)->next = 0;					\
		}							\
	}								\
	mutex_unlock(&((q)->lock));					\
}


/*
 * LQ_DEQUEUE_MACRO:
 *	If queue is not empty then removes item from the head of the queue.
 *	Locks queue while accessing queue.
 */
#define lq_dequeue_macro(q, x)						\
{	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	((cthread_queue_item_t)(x)) = ((lock_queue_t)(q))->head;	\
	if (((cthread_queue_item_t)(x)) != 0) {				\
		if ((((lock_queue_t)(q))->head = 			\
			((cthread_queue_item_t)(x))->next) == 0)	\
			((lock_queue_t)(q))->tail = 0;			\
		else							\
			((cthread_queue_item_t)(x))->next = 0;		\
	}								\
	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}

/*
 * LQ_REMOVE_MACRO:
 *	Removes the queue_item from the queue if it is present on the queue.
 *	Returns whether the item was deleted from the queue.
 */
#define lq_remove_macro(q, x, ret)					     \
{	register cthread_queue_item_t	prev, cur;			     \
	ret = FALSE;							     \
	mutex_lock(&(((lock_queue_t)(q))->lock)); 			     \
	for (prev = cur = ((lock_queue_t)(q))->head; cur != 0; 		     \
		prev = cur, cur = cur->next)				     \
		if (((cthread_queue_item_t)(x)) == cur) {		     \
			if (cur == ((lock_queue_t)(q))->head){		     \
			    if ((((lock_queue_t)(q))->head=cur->next)==0)    \
			  	    ((lock_queue_t)(q))->tail = 0;	     \
			    else					     \
				cur->next = 0;			     	     \
			}						     \
			else{						     \
			    if ((prev->next = cur->next) == 0) {	     \
			       ASSERT(cur==((lock_queue_t)(q))->tail);	     \
				     ((lock_queue_t)(q))->tail = prev;       \
			    } else				    	     \
				    cur->next = 0;			     \
			}						     \
			ret = TRUE;					     \
			break;						     \
		}							     \
	mutex_unlock(&(((lock_queue_t)(q))->lock));		    	     \
}
			

/*
 * LQ_COND_DELETE_MACRO:
 *	Performs the test function with each element of the queue, 
 *	until the function returns true, or the tail of the queue is reached.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	The item is then removed from the queue.
 *	Locks queue while accessing queue.
 *	Returns the item that was deleted from the queue.
 */
#define lq_cond_delete_macro(q, test, args, ret)			     \
{	register cthread_queue_item_t	prev, cur;			     \
	ret = NULL;					  		     \
	mutex_lock(&(((lock_queue_t)(q))->lock)); 			     \
	for (prev = cur = ((lock_queue_t)(q))->head; 			     \
			cur != 0; prev = cur, cur = cur->next)		     \
		if (test(cur, args)) {					     \
			if (cur == ((lock_queue_t)(q))->head) {		     \
			    if ((((lock_queue_t)(q))->head = cur->next) == 0)\
				((lock_queue_t)(q))->tail = 0;		     \
			    else					     \
				cur->next = 0;				     \
			}						     \
			else{						     \
			    if ((prev->next = cur->next) == 0) {	     \
				ASSERT(cur == ((lock_queue_t)(q))->tail);    \
				((lock_queue_t)(q))->tail = prev;	     \
			    } else					     \
				cur->next = 0;				     \
			}						     \
			ret = cur;			     \
			break;						     \
		}							     \
	mutex_unlock(&(((lock_queue_t)(q))->lock));			     \
}

#define lqn_cond_delete_macro(q, test, args, ret)			     \
{	register cthread_queue_item_t	prev, cur;			     \
	ret = NULL;							     \
	for (prev = cur = ((lock_queue_t)(q))->head; 			     \
			cur != 0; prev = cur, cur = cur->next)		     \
		if (test(cur, args)) {					     \
			if (cur == ((lock_queue_t)(q))->head) {		     \
			    if ((((lock_queue_t)(q))->head = cur->next) == 0)\
				((lock_queue_t)(q))->tail = 0;		     \
			    else					     \
				cur->next = 0;				     \
			}						     \
			else{						     \
			    if ((prev->next = cur->next) == 0) {	     \
				ASSERT(cur == ((lock_queue_t)(q))->tail);    \
				((lock_queue_t)(q))->tail = prev;	     \
			    } else					     \
				cur->next = 0;				     \
			}						     \
			ret = cur;			     \
			break;						     \
		}							     \
}



/*
 * LQ_ON_QUEUE:
 *	Locks queue while accessing queue.
 *	Checks to see if the cthread_queue_item_t is on the queue,
 *	if so returns true else returns false.
 */
#define lq_on_macro(q, x, ret)						\
{	register cthread_queue_item_t	cur;				\
	ret = FALSE;							\
	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	for (cur = ((lock_queue_t)(q))->head; cur != 0;cur = cur->next){\
		if (cur == ((cthread_queue_item_t)(x))){		\
			ret = TRUE;					\
			break;						\
		}							\
	}								\
	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}


/*
 * LQ_FIND_MACRO:
 *	Returns a cthread_queue_item_t which is found by the function test.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	If no cthread_queue_item_t is found returns nil.
 *	Locks queue while accessing queue
 */
#define lq_find_macro(q, test, args, ret)				\
{									\
	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	for (ret = ((lock_queue_t)(q))->head; 				\
	     ret != NULL; 						\
	     ret = ((cthread_queue_item_t)(ret))->next)			\
		if (test((ret), args)){					\
			break;						\
		}							\
	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}

#define lqn_find_macro(q, test, args, ret)			\
{								\
	for (ret = ((lock_queue_t)(q))->head; 			\
	     ret != NULL; 					\
	     ret = ((cthread_queue_item_t)(ret))->next)		\
		if (test((ret), args)){				\
			break;					\
		}						\
}


/*
 * LQ_MAP_MACRO:
 *	Maps fn() onto each item on the queue;
 *	The parameters passed to the map function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue
 */
#define lq_map_macro(q, fn, args)					\
{	register cthread_queue_item_t	cur;				\
	mutex_lock(&(((lock_queue_t)(q))->lock));			\
	for (cur = ((lock_queue_t)(q))->head; cur != 0; cur = cur->next)\
		(void)fn(cur, args);					\
	mutex_unlock(&(((lock_queue_t)(q))->lock));			\
}

#endif	_LOCK_QUEUE_MACROS_

