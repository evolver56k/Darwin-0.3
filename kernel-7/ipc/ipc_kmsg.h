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
 *	File:	ipc/ipc_kmsg.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for kernel messages.
 */

#ifndef	_IPC_IPC_KMSG_H_
#define _IPC_IPC_KMSG_H_

#import <mach/features.h>

#include <mach/machine/vm_types.h>
#include <mach/message.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/macro_help.h>
#include <kern/kalloc.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_marequest.h>
#include <vm/vm_map.h>

/*
 *	This structure is only the header for a kmsg buffer;
 *	the actual buffer is normally larger.  The rest of the buffer
 *	holds the body of the message.
 *
 *	In a kmsg, the port fields hold pointers to ports instead
 *	of port names.  These pointers hold references.
 *
 *	The ikm_header.msgh_remote_port field is the destination
 *	of the message.
 *
 *	The  ikm_delta field is set such that (ikm_size + ikm_delta)
 *	indicates the *actual* length of the message and trailer which
 *	was passed by the sender and will be returned to the receiver.
 *	This has two uses in practice:
 *		if (> 0), it indicates the total length of the message
 *		trailer.  Note that due to alignment, this is always a
 *		4 byte aligned quantity.  In this case, the message
 *		*cannot* be in the old format since a trailer is never
 *		transmitted or generated (this also implies that messages
 *		in the new format always contains a trailer).
 *
 *		Otherwise (<= 0), it indicates that the length of the
 *		original sent message was not 4 byte aligned.  This
 *		can only occur in the old message format, and in this
 *		case ikm_size *will* be rounded normally to avoid
 *		breaking assertions elsewhere in the implemenatation.
 */

typedef struct ipc_kmsg {
	struct ipc_kmsg *ikm_next, *ikm_prev;
	vm_size_t ikm_size;
	ipc_marequest_t ikm_marequest;
	security_id_t ikm_sender;
	integer_t ikm_delta;
	mach_msg_header_t ikm_header;
} *ipc_kmsg_t;

#define	IKM_NULL		((ipc_kmsg_t) 0)

#define	IKM_OVERHEAD							\
		(sizeof(struct ipc_kmsg) - sizeof(mach_msg_header_t))

#define	ikm_plus_overhead(size)	((vm_size_t)((size) + IKM_OVERHEAD))
#define	ikm_less_overhead(size)	((mach_msg_size_t)((size) - IKM_OVERHEAD))

/*
 *	We keep a per-processor cache of kernel message buffers.
 *	The cache saves the overhead/locking of using kalloc/kfree.
 *	The per-processor cache seems to miss less than a per-thread cache,
 *	and it also uses less memory.  Access to the cache doesn't
 *	require locking.
 */

extern ipc_kmsg_t	ipc_kmsg_cache[NCPUS];

#define ikm_cache()     ipc_kmsg_cache[cpu_number()]

/*
 *	The size of the kernel message buffers that will be cached.
 *	IKM_SAVED_KMSG_SIZE includes overhead; IKM_SAVED_MSG_SIZE doesn't.
 */

#define	IKM_SAVED_KMSG_SIZE	((vm_size_t) 256)
#define	IKM_SAVED_MSG_SIZE	ikm_less_overhead(IKM_SAVED_KMSG_SIZE)

#define	ikm_alloc(size)							\
		((ipc_kmsg_t) kalloc(ikm_plus_overhead(size)))

#define	ikm_init(kmsg, size)						\
MACRO_BEGIN								\
	ikm_init_special((kmsg), ikm_plus_overhead(size));		\
MACRO_END

#define	ikm_init_special(kmsg, size)					\
MACRO_BEGIN								\
	(kmsg)->ikm_size = (size);					\
	(kmsg)->ikm_marequest = IMAR_NULL;				\
	(kmsg)->ikm_sender = ANONYMOUS_SECURITY_ID_VALUE;		\
	(kmsg)->ikm_delta = 0;						\
MACRO_END

#define	ikm_check_initialized(kmsg, size)				\
MACRO_BEGIN								\
	assert((kmsg)->ikm_size == (size));				\
	assert((kmsg)->ikm_marequest == IMAR_NULL);			\
	(kmsg)->ikm_sender = ANONYMOUS_SECURITY_ID_VALUE;		\
	(kmsg)->ikm_delta = 0;						\
MACRO_END

/*
 *	Non-positive message sizes are special.  They indicate that
 *	the message buffer doesn't come from ikm_alloc and
 *	requires some special handling to free.
 *
 *	ipc_kmsg_free is the non-macro form of ikm_free.
 *	It frees kmsgs of all varieties.
 */

#define	IKM_SIZE_NORMA		0
#define	IKM_SIZE_NETWORK	-1
#define	IKM_SIZE_DEVICE		-2
#define IKM_SIZE_NETIPC		-3

#define	ikm_free(kmsg)							\
MACRO_BEGIN								\
	register vm_size_t _size = (kmsg)->ikm_size;			\
									\
	if ((integer_t)_size > 0)					\
		kfree((vm_offset_t) (kmsg), _size);			\
	else								\
		ipc_kmsg_free(kmsg);					\
MACRO_END

/*
 *	struct ipc_kmsg_queue is defined in kern/thread.h instead of here,
 *	so that kern/thread.h doesn't have to include ipc/ipc_kmsg.h.
 */

#include <ipc/ipc_kmsg_queue.h>

typedef struct ipc_kmsg_queue *ipc_kmsg_queue_t;

#define	IKMQ_NULL		((ipc_kmsg_queue_t) 0)


/*
 * Exported interfaces
 */

#define	ipc_kmsg_queue_init(queue)		\
MACRO_BEGIN					\
	(queue)->ikmq_base = IKM_NULL;		\
MACRO_END

#define	ipc_kmsg_queue_empty(queue)	((queue)->ikmq_base == IKM_NULL)

/* Enqueue a kmsg */
extern void ipc_kmsg_enqueue(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg);

/* Dequeue and return a kmsg */
extern ipc_kmsg_t ipc_kmsg_dequeue(
	ipc_kmsg_queue_t        queue);

/* Pull a kmsg out of a queue */
extern void ipc_kmsg_rmqueue(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg);

#define	ipc_kmsg_queue_first(queue)		((queue)->ikmq_base)

/* Return the kmsg following the given kmsg */
extern ipc_kmsg_t ipc_kmsg_queue_next(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg);

#define	ipc_kmsg_rmqueue_first_macro(queue, kmsg)			\
MACRO_BEGIN								\
	register ipc_kmsg_t _next;					\
									\
	assert((queue)->ikmq_base == (kmsg));				\
									\
	_next = (kmsg)->ikm_next;					\
	if (_next == (kmsg)) {						\
		assert((kmsg)->ikm_prev == (kmsg));			\
		(queue)->ikmq_base = IKM_NULL;				\
	} else {							\
		register ipc_kmsg_t _prev = (kmsg)->ikm_prev;		\
									\
		(queue)->ikmq_base = _next;				\
		_next->ikm_prev = _prev;				\
		_prev->ikm_next = _next;				\
	}								\
MACRO_END

#define	ipc_kmsg_enqueue_macro(queue, kmsg)				\
MACRO_BEGIN								\
	register ipc_kmsg_t _first = (queue)->ikmq_base;		\
									\
	if (_first == IKM_NULL) {					\
		(queue)->ikmq_base = (kmsg);				\
		(kmsg)->ikm_next = (kmsg);				\
		(kmsg)->ikm_prev = (kmsg);				\
	} else {							\
		register ipc_kmsg_t _last = _first->ikm_prev;		\
									\
		(kmsg)->ikm_next = _first;				\
		(kmsg)->ikm_prev = _last;				\
		_first->ikm_prev = (kmsg);				\
		_last->ikm_next = (kmsg);				\
	}								\
MACRO_END

/* Destroy kernel message */
extern void ipc_kmsg_destroy(
	ipc_kmsg_t	kmsg);

/* Free a kernel message buffer */
extern void ipc_kmsg_free(
	ipc_kmsg_t	kmsg);

/* Allocate a kernel message buffer and copy a user message to the buffer */
extern mach_msg_return_t ipc_kmsg_get(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		size,
	integer_t		delta,
	ipc_kmsg_t		*kmsgp);

/* Allocate a kernel message buffer and copy a kernel message to the buffer */
extern mach_msg_return_t ipc_kmsg_get_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		size,
	integer_t		delta,
	ipc_kmsg_t		*kmsgp);

/* Copy a kernel message buffer to a user message */
extern mach_msg_return_t ipc_kmsg_put(
	mach_msg_header_t	*msg,
	ipc_kmsg_t		kmsg,
	mach_msg_size_t		size);

/* Copy a kernel message buffer to a kernel message */
extern void ipc_kmsg_put_to_kernel(
	mach_msg_header_t	*msg,
	ipc_kmsg_t		kmsg,
	mach_msg_size_t		size);

/* Copyin port rights in the header of a message */
extern mach_msg_return_t ipc_kmsg_copyin_header(
	mach_msg_header_t	*msg,
	ipc_space_t		space,
	mach_port_t		notify);

/* Copyin port rights and out-of-line memory from a user message */
extern mach_msg_return_t ipc_kmsg_copyin(
	ipc_kmsg_t	kmsg,
	ipc_space_t	space,
	vm_map_t	map,
	mach_port_t	notify);

/* Copyin port rights and out-of-line memory from a kernel message */
extern void ipc_kmsg_copyin_from_kernel(
	ipc_kmsg_t	kmsg);

/* Copyout port rights in the header of a message */
extern mach_msg_return_t ipc_kmsg_copyout_header(
	mach_msg_header_t	*msg,
	ipc_space_t		space,
	mach_port_t		notify);

/* Copyout a port right returning a name */
extern mach_msg_return_t ipc_kmsg_copyout_object(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	mach_port_t		*namep);

/* Copyout the header and body to a user message */
extern mach_msg_return_t ipc_kmsg_copyout(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	mach_port_t		notify,
	ipc_kmsg_t		list);

/* Copyout port rights and out-of-line memory from the body of a message */
extern mach_msg_return_t ipc_kmsg_copyout_body(
    	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	ipc_kmsg_t		list);

/* Copyout port rights and out-of-line memory to a user message,
   not reversing the ports in the header */
extern mach_msg_return_t ipc_kmsg_copyout_pseudo(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	ipc_kmsg_t		list);

/* Copyout the destination port in the message */
extern void ipc_kmsg_copyout_dest( 
	ipc_kmsg_t	kmsg,
	ipc_space_t	space);

/* kernel's version of ipc_kmsg_copyout_dest */
extern void ipc_kmsg_copyout_to_kernel(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space);

/* Check scatter and gather lists for consistency */
extern mach_msg_return_t ipc_kmsg_check_scatter(
	ipc_kmsg_t		kmsg,
	mach_msg_option_t	option,
	ipc_kmsg_t		*list);

#if	MACH_IPC_COMPAT

extern mach_msg_return_t
ipc_kmsg_copyin_compat(/* ipc_kmsg_t, ipc_space_t, vm_map_t */);

extern mach_msg_return_t
ipc_kmsg_copyout_compat(/* ipc_kmsg_t, ipc_space_t, vm_map_t */);

#endif	/* MACH_IPC_COMPAT */

#endif	/* _IPC_IPC_KMSG_H_ */
