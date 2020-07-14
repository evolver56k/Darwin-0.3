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
 *	File:	ipc/ipc_mqueue.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for message queues.
 */

#ifndef	_IPC_IPC_MQUEUE_H_
#define _IPC_IPC_MQUEUE_H_

#import <mach/features.h>

#include <mach/message.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/macro_help.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_thread.h>
#include <ipc/ipc_types.h>

typedef struct ipc_mqueue {
	decl_simple_lock_data(, imq_lock_data)
	struct ipc_kmsg_queue imq_messages;
	struct ipc_thread_queue imq_threads;
} *ipc_mqueue_t;

#define	IMQ_NULL		((ipc_mqueue_t) 0)

#define	imq_lock_init(mq)	simple_lock_init(&(mq)->imq_lock_data)
#define	imq_lock(mq)		simple_lock(&(mq)->imq_lock_data)
#define	imq_lock_try(mq)	simple_lock_try(&(mq)->imq_lock_data)
#define	imq_unlock(mq)		simple_unlock(&(mq)->imq_lock_data)

#define	IMQ_NULL_CONTINUE	((void (*)(void)) 0)

/*
 * Exported interfaces
 */

/* Initialize a newly-allocated message queue */
extern void ipc_mqueue_init(
	ipc_mqueue_t	mqueue);

/* Move messages from one queue to another */
extern void ipc_mqueue_move(
	ipc_mqueue_t	dest,
	ipc_mqueue_t	source,
	ipc_port_t	port);

/* Wake up receivers waiting in a message queue */
extern void ipc_mqueue_changed(
	ipc_mqueue_t		mqueue,
	mach_msg_return_t	mr);

/* Send a message to a port */
extern mach_msg_return_t ipc_mqueue_send(
	ipc_kmsg_t		kmsg,
	mach_msg_option_t	option,
	mach_msg_timeout_t	timeout,
	void			(*continuation)(void));

/* Convert a name in a space to a message queue */
extern mach_msg_return_t ipc_mqueue_copyin(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_mqueue_t	*mqueuep,
	ipc_object_t	*objectp);

/* Receive a message from a message queue */
extern mach_msg_return_t ipc_mqueue_receive(
	ipc_mqueue_t		mqueue,
	mach_msg_option_t	option,
	mach_msg_size_t		max_size,
	mach_msg_timeout_t	timeout,
	boolean_t		resume,
	void			(*continuation)(void),
	ipc_kmsg_t		*kmsgp,
	mach_port_seqno_t	*seqnop,
	ipc_kmsg_t		*list);

/*
 *	extern void
 *	ipc_mqueue_send_always(ipc_kmsg_t);
 *
 *	Unfortunately, to avoid warnings/lint about unused variables
 *	when assertions are turned off, we need two versions of this.
 */

#if	MACH_ASSERT

#define	ipc_mqueue_send_always(kmsg)					\
MACRO_BEGIN								\
	mach_msg_return_t mr;						\
									\
	mr = ipc_mqueue_send((kmsg), MACH_SEND_ALWAYS,			\
			     MACH_MSG_TIMEOUT_NONE,			\
			     IMQ_NULL_CONTINUE);			\
	assert(mr == MACH_MSG_SUCCESS);					\
MACRO_END

#else	/* MACH_ASSERT */

#define	ipc_mqueue_send_always(kmsg)					\
MACRO_BEGIN								\
	(void) ipc_mqueue_send((kmsg), MACH_SEND_ALWAYS,		\
			       MACH_MSG_TIMEOUT_NONE,			\
			       IMQ_NULL_CONTINUE);			\
MACRO_END

#endif	/* MACH_ASSERT */

#endif	/* _IPC_IPC_MQUEUE_H_ */
