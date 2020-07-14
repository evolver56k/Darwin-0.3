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

#ifndef	_SYS_QUEUE_
#define	_SYS_QUEUE_


/*
 *	Queue of abstract objects.  Queue is maintained
 *	within that object.
 *
 *	Supports fast removal from within the sys_queue.
 *
 *	How to declare a sys_queue of elements of type "foo_t":
 *		In the "*foo_t" type, you must have a field of
 *		type "sys_queue_chain_t" to hold together this sys_queue.
 *		There may be more than one chain through a
 *		"foo_t", for use by different sys_queues.
 *
 *		Declare the sys_queue as a "sys_queue_t" type.
 *
 *		Elements of the sys_queue (of type "foo_t", that is)
 *		are referred to by reference, and cast to type
 *		"sys_queue_entry_t" within this module.
 */

/*
 *	A generic doubly-linked list (sys_queue).
 */

struct sys_queue_entry {
	struct sys_queue_entry	*next;		/* next element */
	struct sys_queue_entry	*prev;		/* previous element */
};

typedef struct sys_queue_entry	*sys_queue_t;
typedef	struct sys_queue_entry	sys_queue_head_t;
typedef	struct sys_queue_entry	sys_queue_chain_t;
typedef	struct sys_queue_entry	*sys_queue_entry_t;

#define round_sys_queue(size)	(((size)+7) & (~7))

/*
 *	Macro:		sys_queue_init
 *	Function:
 *		Initialize the given sys_queue.
 *	Header:
 *		void sys_queue_init(q)
 *			sys_queue_t		q;
 */
#define	sys_queue_init(q)	((q)->next = (q)->prev = q)

/*
 *	Macro:		sys_queue_first
 *	Function:
 *		Returns the first entry in the sys_queue,
 *	Header:
 *		sys_queue_entry_t sys_queue_first(q)
 *			sys_queue_t	q;
 */
#define	sys_queue_first(q)	((q)->next)

/*
 *	Macro:		sys_queue_next
 *	Header:
 *		sys_queue_entry_t sys_queue_next(qc)
 *			sys_queue_t qc;
 */
#define	sys_queue_next(qc)	((qc)->next)

/*
 *	Macro:		sys_queue_end
 *	Header:
 *		boolean_t sys_queue_end(q, qe)
 *			sys_queue_t q;
 *			sys_queue_entry_t qe;
 */
#define	sys_queue_end(q, qe)	((q) == (qe))

#define	sys_queue_empty(q)		sys_queue_end((q), sys_queue_first(q))

/*
 *	Macro:		sys_queue_enter
 *	Header:
 *		void sys_queue_enter(q, elt, type, field)
 *			sys_queue_t q;
 *			<type> elt;
 *			<type> is what's in our sys_queue
 *			<field> is the chain field in (*<type>)
 */
#define sys_queue_enter(head, elt, type, field)			\
{ 								\
	if (sys_queue_empty((head))) {				\
		(head)->next = (sys_queue_entry_t) elt;		\
		(head)->prev = (sys_queue_entry_t) elt;		\
		(elt)->field.next = head;			\
		(elt)->field.prev = head;			\
	}							\
	else {							\
		register sys_queue_entry_t prev;			\
								\
		prev = (head)->prev;				\
		(elt)->field.prev = prev;			\
		(elt)->field.next = head;			\
		(head)->prev = (sys_queue_entry_t)(elt);		\
		((type)prev)->field.next = (sys_queue_entry_t)(elt);\
	}							\
}

/*
 *	Macro:		sys_queue_field [internal use only]
 *	Function:
 *		Find the sys_queue_chain_t (or sys_queue_t) for the
 *		given element (thing) in the given sys_queue (head)
 */
#define	sys_queue_field(head, thing, type, field)			\
		(((head) == (thing)) ? (head) : &((type)(thing))->field)

/*
 *	Macro:		sys_queue_remove
 *	Header:
 *		void sys_queue_remove(q, qe, type, field)
 *			arguments as in sys_queue_enter
 */
#define	sys_queue_remove(head, elt, type, field)			\
{								\
	register sys_queue_entry_t	next, prev;			\
								\
	next = (elt)->field.next;				\
	prev = (elt)->field.prev;				\
								\
	sys_queue_field((head), next, type, field)->prev = prev;	\
	sys_queue_field((head), prev, type, field)->next = next;	\
}

/*
 *	Macro:		sys_queue_assign
 */
#define	sys_queue_assign(to, from, type, field)			\
{								\
	((type)((from)->prev))->field.next = (to);		\
	((type)((from)->next))->field.prev = (to);		\
	*to = *from;						\
}

#define	sys_queue_remove_first(h, e, t, f)				\
{								\
	e = (t) sys_queue_first((h));				\
	sys_queue_remove((h), (e), t, f);				\
}

/*
 *	Macro:		sys_queue_enter_first
 *	Header:
 *		void sys_queue_enter_first(q, elt, type, field)
 *			sys_queue_t q;
 *			<type> elt;
 *			<type> is what's in our sys_queue
 *			<field> is the chain field in (*<type>)
 */
#define sys_queue_enter_first(head, elt, type, field)			\
{ 								\
	if (sys_queue_empty((head))) {				\
		(head)->next = (sys_queue_entry_t) elt;		\
		(head)->prev = (sys_queue_entry_t) elt;		\
		(elt)->field.next = head;			\
		(elt)->field.prev = head;			\
	}							\
	else {							\
		register sys_queue_entry_t next;			\
								\
		next = (head)->next;				\
		(elt)->field.prev = head;			\
		(elt)->field.next = next;			\
		(head)->next = (sys_queue_entry_t)(elt);		\
		((type)next)->field.prev = (sys_queue_entry_t)(elt);\
	}							\
}

/*
 *	Macro:		sys_queue_last
 *	Function:
 *		Returns the last entry in the sys_queue,
 *	Header:
 *		sys_queue_entry_t sys_queue_last(q)
 *			sys_queue_t	q;
 */
#define	sys_queue_last(q)	((q)->prev)

/*
 *	Macro:		sys_queue_prev
 *	Header:
 *		sys_queue_entry_t sys_queue_prev(qc)
 *			sys_queue_t qc;
 */
#define	sys_queue_prev(qc)	((qc)->prev)


#endif	_SYS_QUEUE_
