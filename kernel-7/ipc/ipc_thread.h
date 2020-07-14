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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	ipc/ipc_thread.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for the IPC component of threads.
 */

#ifndef	_IPC_IPC_THREAD_H_
#define _IPC_IPC_THREAD_H_

#include <kern/thread.h>

typedef thread_t ipc_thread_t;

#define	ITH_NULL		THREAD_NULL

#define	ith_lock_init(thread)	simple_lock_init(&(thread)->ith_lock_data)
#define	ith_lock(thread)	simple_lock(&(thread)->ith_lock_data)
#define	ith_unlock(thread)	simple_unlock(&(thread)->ith_lock_data)

typedef struct ipc_thread_queue {
	ipc_thread_t ithq_base;
} *ipc_thread_queue_t;

#define	ITHQ_NULL		((ipc_thread_queue_t) 0)


#define	ipc_thread_links_init(thread)		\
MACRO_BEGIN					\
	(thread)->ith_next = (thread);		\
	(thread)->ith_prev = (thread);		\
MACRO_END

#define	ipc_thread_queue_init(queue)		\
MACRO_BEGIN					\
	(queue)->ithq_base = ITH_NULL;		\
MACRO_END

#define	ipc_thread_queue_empty(queue)	((queue)->ithq_base == ITH_NULL)

#define	ipc_thread_queue_first(queue)	((queue)->ithq_base)

#define	ipc_thread_rmqueue_first_macro(queue, thread)			\
MACRO_BEGIN								\
	register ipc_thread_t _next;					\
									\
	assert((queue)->ithq_base == (thread));				\
									\
	_next = (thread)->ith_next;					\
	if (_next == (thread)) {					\
		assert((thread)->ith_prev == (thread));			\
		(queue)->ithq_base = ITH_NULL;				\
	} else {							\
		register ipc_thread_t _prev = (thread)->ith_prev;	\
									\
		(queue)->ithq_base = _next;				\
		_next->ith_prev = _prev;				\
		_prev->ith_next = _next;				\
		ipc_thread_links_init(thread);				\
	}								\
MACRO_END

#define	ipc_thread_enqueue_macro(queue, thread)				\
MACRO_BEGIN								\
	register ipc_thread_t _first = (queue)->ithq_base;		\
									\
	if (_first == ITH_NULL) {					\
		(queue)->ithq_base = (thread);				\
		assert((thread)->ith_next == (thread));			\
		assert((thread)->ith_prev == (thread));			\
	} else {							\
		register ipc_thread_t _last = _first->ith_prev;		\
									\
		(thread)->ith_next = _first;				\
		(thread)->ith_prev = _last;				\
		_first->ith_prev = (thread);				\
		_last->ith_next = (thread);				\
	}								\
MACRO_END

/* Enqueue a thread on a message queue */
extern void ipc_thread_enqueue(
	ipc_thread_queue_t	queue,
	ipc_thread_t		thread);

/* Dequeue a thread from a message queue */
extern ipc_thread_t ipc_thread_dequeue(
	ipc_thread_queue_t	queue);

/* Remove a thread from a message queue */
extern void ipc_thread_rmqueue(
	ipc_thread_queue_t	queue,
	ipc_thread_t		thread);

#endif	/* _IPC_IPC_THREAD_H_ */
