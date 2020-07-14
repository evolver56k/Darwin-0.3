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
 *	File:	ipc/ipc_kmsg.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Operations on kernel messages.
 */

#import <mach/features.h>

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/port.h>
#include <kern/assert.h>
#include <kern/kalloc.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <ipc/port.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_thread.h>
#include <ipc/ipc_marequest.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_right.h>

#include <ipc/ipc_machdep.h>

#define MSG_OOL_SIZE_SMALL	(PAGE_SIZE >> 2)

extern int copyinmap();
extern int copyoutmap();

#define is_misaligned(x)	\
	( ((vm_offset_t)(x)) & (sizeof(vm_offset_t)-1) )
#define ptr_align(x)		\
	( ( ((vm_offset_t)(x)) + (sizeof(vm_offset_t)-1) ) &	\
			~(sizeof(vm_offset_t)-1) )

ipc_kmsg_t ipc_kmsg_cache[NCPUS];

/*
 * Forward declarations
 */

void ipc_kmsg_clean(
	ipc_kmsg_t	kmsg);

void ipc_kmsg_clean_body(
    	ipc_kmsg_t	kmsg,
    	int		number);

void ipc_kmsg_clean_partial(
	ipc_kmsg_t		kmsg,
	mach_msg_type_number_t	number,
	vm_offset_t		paddr,
	vm_size_t		length);

mach_msg_return_t ipc_kmsg_copyout_body(
    	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	ipc_kmsg_t		list);

mach_msg_return_t ipc_kmsg_copyin_body(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map);

/*
 *	Routine:	ipc_kmsg_enqueue
 *	Purpose:
 *		Enqueue a kmsg.
 */

void
ipc_kmsg_enqueue(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg)
{
	ipc_kmsg_enqueue_macro(queue, kmsg);
}

/*
 *	Routine:	ipc_kmsg_dequeue
 *	Purpose:
 *		Dequeue and return a kmsg.
 */

ipc_kmsg_t
ipc_kmsg_dequeue(
	ipc_kmsg_queue_t	queue)
{
	ipc_kmsg_t first;

	first = ipc_kmsg_queue_first(queue);

	if (first != IKM_NULL)
		ipc_kmsg_rmqueue_first_macro(queue, first);

	return first;
}

/*
 *	Routine:	ipc_kmsg_rmqueue
 *	Purpose:
 *		Pull a kmsg out of a queue.
 */

void
ipc_kmsg_rmqueue(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg)
{
	ipc_kmsg_t next, prev;

	assert(queue->ikmq_base != IKM_NULL);

	next = kmsg->ikm_next;
	prev = kmsg->ikm_prev;

	if (next == kmsg) {
		assert(prev == kmsg);
		assert(queue->ikmq_base == kmsg);

		queue->ikmq_base = IKM_NULL;
	} else {
		if (queue->ikmq_base == kmsg)
			queue->ikmq_base = next;

		next->ikm_prev = prev;
		prev->ikm_next = next;
	}
}

/*
 *	Routine:	ipc_kmsg_queue_next
 *	Purpose:
 *		Return the kmsg following the given kmsg.
 *		(Or IKM_NULL if it is the last one in the queue.)
 */

ipc_kmsg_t
ipc_kmsg_queue_next(
	ipc_kmsg_queue_t	queue,
	ipc_kmsg_t		kmsg)
{
	ipc_kmsg_t next;

	assert(queue->ikmq_base != IKM_NULL);

	next = kmsg->ikm_next;
	if (queue->ikmq_base == next)
		next = IKM_NULL;

	return next;
}

/*
 *	Routine:	ipc_kmsg_destroy
 *	Purpose:
 *		Destroys a kernel message.  Releases all rights,
 *		references, and memory held by the message.
 *		Frees the message.
 *	Conditions:
 *		No locks held.
 */

void
ipc_kmsg_destroy(
	ipc_kmsg_t	kmsg)
{
	ipc_kmsg_queue_t queue;
	boolean_t empty;

	/*
	 *	ipc_kmsg_clean can cause more messages to be destroyed.
	 *	Curtail recursion by queueing messages.  If a message
	 *	is already queued, then this is a recursive call.
	 */

	queue = &current_thread()->ith_messages;
	empty = ipc_kmsg_queue_empty(queue);
	ipc_kmsg_enqueue(queue, kmsg);

	if (empty) {
		/* must leave kmsg in queue while cleaning it */

		while ((kmsg = ipc_kmsg_queue_first(queue)) != IKM_NULL) {
			ipc_kmsg_clean(kmsg);
			ipc_kmsg_rmqueue(queue, kmsg);
			ikm_free(kmsg);
		}
	}
}

/*
 *	Routine:	ipc_kmsg_clean_body
 *	Purpose:
 *		Cleans the body of a kernel message.
 *		Releases all rights, references, and memory.
 *
 *	Conditions:
 *		No locks held.
 */

void
ipc_kmsg_clean_body(
	ipc_kmsg_t	kmsg,
	int		number)
{
    mach_msg_descriptor_t 	*saddr, *eaddr;

    if ( number == 0 )
	return;

    saddr = (mach_msg_descriptor_t *) 
			((mach_msg_base_t *) &kmsg->ikm_header + 1);
    eaddr = saddr + number;

    for ( ; saddr < eaddr; saddr++ ) {
	
	switch (saddr->type.type) {
	    
	    case MACH_MSG_PORT_DESCRIPTOR: {
		mach_msg_port_descriptor_t *dsc;

		dsc = &saddr->port;

		/* 
		 * Destroy port rights carried in the message 
		 */
		if (!IO_VALID((ipc_object_t) dsc->name))
		    continue;
		ipc_object_destroy((ipc_object_t) dsc->name, dsc->disposition);
		break;
	    }
	    case MACH_MSG_OOL_DESCRIPTOR : {
		mach_msg_ool_descriptor_t *dsc;

		dsc = &saddr->out_of_line;
		
		/* 
		 * Destroy memory carried in the message 
		 */
		if (dsc->size == 0) {
		    assert(dsc->address == (void *) 0);
		} else {
#if	MACH_OLD_VM_COPY
		    (void) vm_deallocate(
		    			ipc_soft_map,
					(vm_offset_t)dsc->address,
					(vm_size_t)dsc->size);
#else
		    if (dsc->copy == MACH_MSG_PHYSICAL_COPY &&
			    dsc->size <= MSG_OOL_SIZE_SMALL) {
		        kfree((vm_offset_t)dsc->address,
			     (vm_size_t)dsc->size);
		    } else {
		    	vm_map_copy_discard((vm_map_copy_t) dsc->address);
		    }
#endif
		}
		break;
	    }
	    case MACH_MSG_OOL_PORTS_DESCRIPTOR : {
		ipc_object_t             	*objects;
		mach_msg_type_number_t   	j;
		mach_msg_ool_ports_descriptor_t	*dsc;

		dsc = &saddr->ool_ports;
		objects = (ipc_object_t *) dsc->address;

		if (dsc->count == 0) {
			break;
		}

		assert(objects != (ipc_object_t *) 0);
		
		/* destroy port rights carried in the message */
		
		for (j = 0; j < dsc->count; j++) {
		    ipc_object_t object = objects[j];
		    
		    if (!IO_VALID(object))
			continue;
		    
		    ipc_object_destroy(object, dsc->disposition);
		}

		/* destroy memory carried in the message */

		assert(dsc->count != 0);

		kfree((vm_offset_t) dsc->address, 
		     (vm_size_t) dsc->count * sizeof(mach_port_t));
		break;
	    }
	    default : {
		panic("ipc_kmsg_clean_body: bad descriptor type %d",
		      			saddr->type.type);
	    }
	}
    }
}

void
ipc_kmsg_clean_body_compat(
	vm_offset_t		saddr,
	vm_offset_t		eaddr)
{
	while (saddr < eaddr) {
		mach_msg_type_long_t *type;
		mach_msg_type_name_t name;
		mach_msg_type_size_t size;
		mach_msg_type_number_t number;
		boolean_t is_inline, is_port;
		vm_size_t length;

		type = (mach_msg_type_long_t *) saddr;
		is_inline = ((mach_msg_type_t*)type)->msgt_inline;
		if (((mach_msg_type_t*)type)->msgt_longform) {
			/* This must be aligned */
			if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
			    (is_misaligned(type))) {
				saddr = ptr_align(saddr);
				continue;
			}
			name = type->msgtl_name;
			size = type->msgtl_size;
			number = type->msgtl_number;
			saddr += sizeof(mach_msg_type_long_t);
		} else {
			name = ((mach_msg_type_t*)type)->msgt_name;
			size = ((mach_msg_type_t*)type)->msgt_size;
			number = ((mach_msg_type_t*)type)->msgt_number;
			saddr += sizeof(mach_msg_type_t);
		}

		/* padding (ptrs and ports) ? */
		if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
		    ((size >> 3) == sizeof(natural_t)))
			saddr = ptr_align(saddr);

		/* calculate length of data in bytes, rounding up */

		length = ((number * size) + 7) >> 3;

		is_port = MACH_MSG_TYPE_PORT_ANY(name);

		if (is_port) {
			ipc_object_t *objects;
			mach_msg_type_number_t i;

			if (is_inline) {
				objects = (ipc_object_t *) saddr;
				/* sanity check */
				while (eaddr < (vm_offset_t)&objects[number]) number--;
			} else {
				objects = (ipc_object_t *)
						* (vm_offset_t *) saddr;
			}

			/* destroy port rights carried in the message */

			for (i = 0; i < number; i++) {
				ipc_object_t object = objects[i];

				if (!IO_VALID(object))
					continue;

				ipc_object_destroy(object, name);
			}
		}

		if (is_inline) {
			/* inline data sizes round up to int boundaries */

			saddr += (length + 3) &~ 3;
		} else {
			vm_offset_t data = * (vm_offset_t *) saddr;

			/* destroy memory carried in the message */

			if (length == 0)
				assert(data == 0);
			else if (is_port)
				kfree(data, length);
			else
#if	MACH_OLD_VM_COPY
				(void) vm_deallocate(
						ipc_soft_map, data, length);
#else
				vm_map_copy_discard((vm_map_copy_t) data);
#endif

			saddr += sizeof(vm_offset_t);
		}
	}
}

/*
 *	Routine:	ipc_kmsg_clean_partial
 *	Purpose:
 *		Cleans a partially-acquired kernel message.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_kmsg_clean_partial(
	ipc_kmsg_t		kmsg,
	mach_msg_type_number_t	number,
	vm_offset_t		paddr,
	vm_size_t		length)
{
	ipc_object_t object;
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;

	object = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	assert(IO_VALID(object));
	ipc_object_destroy(object, MACH_MSGH_BITS_REMOTE(mbits));

	object = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	if (IO_VALID(object))
		ipc_object_destroy(object, MACH_MSGH_BITS_LOCAL(mbits));

	if (paddr) {
		(void) vm_deallocate(ipc_kernel_copy_map, paddr, length);
	}

	ipc_kmsg_clean_body(kmsg, number);
}

void
ipc_kmsg_clean_partial_compat(
	ipc_kmsg_t		kmsg,
	vm_offset_t		eaddr,
	boolean_t		dolast,
	mach_msg_type_number_t	number)
{
	ipc_object_t object;
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	vm_offset_t saddr;

	assert(kmsg->ikm_marequest == IMAR_NULL);

	object = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	assert(IO_VALID(object));
	ipc_object_destroy(object, MACH_MSGH_BITS_REMOTE(mbits));

	object = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	if (IO_VALID(object))
		ipc_object_destroy(object, MACH_MSGH_BITS_LOCAL(mbits));

	saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
	ipc_kmsg_clean_body_compat(saddr, eaddr);

	if (dolast) {
		mach_msg_type_long_t *type;
		mach_msg_type_name_t name;
		mach_msg_type_size_t size;
		mach_msg_type_number_t rnumber;
		boolean_t is_inline, is_port;
		vm_size_t length;

xxx:		type = (mach_msg_type_long_t *) eaddr;
		is_inline = ((mach_msg_type_t*)type)->msgt_inline;
		if (((mach_msg_type_t*)type)->msgt_longform) {
			/* This must be aligned */
			if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
			    (is_misaligned(type))) {
				eaddr = ptr_align(eaddr);
				goto xxx;
			}
			name = type->msgtl_name;
			size = type->msgtl_size;
			rnumber = type->msgtl_number;
			eaddr += sizeof(mach_msg_type_long_t);
		} else {
			name = ((mach_msg_type_t*)type)->msgt_name;
			size = ((mach_msg_type_t*)type)->msgt_size;
			rnumber = ((mach_msg_type_t*)type)->msgt_number;
			eaddr += sizeof(mach_msg_type_t);
		}

		/* padding (ptrs and ports) ? */
		if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
		    ((size >> 3) == sizeof(natural_t)))
			eaddr = ptr_align(eaddr);

		/* calculate length of data in bytes, rounding up */

		length = ((rnumber * size) + 7) >> 3;

		is_port = MACH_MSG_TYPE_PORT_ANY(name);

		if (is_port) {
			ipc_object_t *objects;
			mach_msg_type_number_t i;

			objects = (ipc_object_t *)
				(is_inline ? eaddr : * (vm_offset_t *) eaddr);

			/* destroy port rights carried in the message */

			for (i = 0; i < number; i++) {
				ipc_object_t obj = objects[i];

				if (!IO_VALID(obj))
					continue;

				ipc_object_destroy(obj, name);
			}
		}

		if (!is_inline) {
			vm_offset_t data = * (vm_offset_t *) eaddr;

			/* destroy memory carried in the message */

			if (length == 0)
				assert(data == 0);
			else if (is_port)
				kfree(data, length);
			else
#if	MACH_OLD_VM_COPY
				(void) vm_deallocate(
						ipc_soft_map, data, length);
#else
				vm_map_copy_discard((vm_map_copy_t) data);
#endif
		}
	}
}

/*
 *	Routine:	ipc_kmsg_clean
 *	Purpose:
 *		Cleans a kernel message.  Releases all rights,
 *		references, and memory held by the message.
 *	Conditions:
 *		No locks held.
 */

void
ipc_kmsg_clean(
	ipc_kmsg_t	kmsg)
{
	ipc_marequest_t marequest;
	ipc_object_t object;
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;

	marequest = kmsg->ikm_marequest;
	if (marequest != IMAR_NULL)
		ipc_marequest_destroy(marequest);

	object = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	if (IO_VALID(object))
		ipc_object_destroy(object, MACH_MSGH_BITS_REMOTE(mbits));

	object = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	if (IO_VALID(object))
		ipc_object_destroy(object, MACH_MSGH_BITS_LOCAL(mbits));

	if (mbits & MACH_MSGH_BITS_COMPLEX) {
	    if (mbits & MACH_MSGH_BITS_OLD_FORMAT) {
		vm_offset_t saddr, eaddr;

		saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
		eaddr = (vm_offset_t) &kmsg->ikm_header +
				kmsg->ikm_header.msgh_size;

		ipc_kmsg_clean_body_compat(saddr, eaddr);
	    }
	    else {
		mach_msg_body_t *body;

		body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
		ipc_kmsg_clean_body(kmsg, body->msgh_descriptor_count);
	    }
	}
}

/*
 *	Routine:	ipc_kmsg_free
 *	Purpose:
 *		Free a kernel message buffer.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_kmsg_free(
	ipc_kmsg_t	kmsg)
{
	vm_size_t size = kmsg->ikm_size;

	switch (size) {
	    case IKM_SIZE_NETWORK:
		/* return it to the network code */
		break;

#if	DRIVERKIT
	    case IKM_SIZE_DEVICE:
	    	KernDeviceInterruptMsgRelease(kmsg);
		break;
#endif

#if	MACH_NET		
	    case IKM_SIZE_NETIPC:
	    	netipc_msg_release(kmsg);
		break;
#endif

	    default:
		kfree((vm_offset_t) kmsg, size);
		break;
	}
}

/*
 *	Routine:	ipc_kmsg_get
 *	Purpose:
 *		Allocates a kernel message buffer.
 *		Copies a user message to the message buffer.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Acquired a message buffer.
 *		MACH_SEND_MSG_TOO_SMALL	Message smaller than a header.
 *		MACH_SEND_MSG_TOO_SMALL	Message size not long-word multiple.
 *		MACH_SEND_NO_BUFFER	Couldn't allocate a message buffer.
 *		MACH_SEND_INVALID_DATA	Couldn't copy message data.
 */

mach_msg_return_t
ipc_kmsg_get(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		size,
	integer_t		delta,
	ipc_kmsg_t		*kmsgp)
{
	ipc_kmsg_t		kmsg;
	mach_msg_size_t		send_size, msg_size;

	if ((size < sizeof(mach_msg_header_t)) || (size & 3))
		return MACH_SEND_MSG_TOO_SMALL;

	/*
	 * Three different notions of message size:
	 * 1)	'size' is the value passed through the API,
	 *	which is the actual size of the message to the
	 *	sender.  In the case of the old API, this value
	 *	will have been rounded up to a 4 byte boundary,
	 *	and delta will be set to a negative value such
	 *	that (size + delta) is the original value passed
	 *	through the API.
	 * 2)	'msg_size' is the minimum sized kmsg needed to
	 *	hold the message.  This is equal to the size of
	 *	the message plus the *maximum* sized trailer which
	 *	can be generated by this kernel.  In the case of
	 *	the old API, this will be equal to 'size'.
	 * 3)	'send_size' is the actual amount of data which
	 *	will be copied from the sender.  In the case of
	 *	MACH_SEND_TRAILER, this will include a *minimum*
	 *	sized trailer in order to acquire the size and
	 *	type fields of the entire trailer.  The actual
	 *	trailer data will then be fetched using a separate
	 *	copy operation.
	 */
	if (delta <= 0) {
		msg_size = size;
		send_size = size + delta;
	}
	else {
		if ((delta & 3) ||
		    delta < MACH_MSG_TRAILER_MINIMUM_SIZE ||
		    delta > MAX_TRAILER_SIZE)
		    	return MACH_SEND_INVALID_TRAILER;

		msg_size = size + delta;
		send_size = size + ((option & MACH_SEND_TRAILER) ?
					sizeof (mach_msg_trailer_t) : 0);
	}

	if (msg_size <= IKM_SAVED_MSG_SIZE) {
		kmsg = ikm_cache();
		if (kmsg != IKM_NULL) {
			ikm_cache() = IKM_NULL;
			ikm_check_initialized(kmsg, IKM_SAVED_KMSG_SIZE);
		} else {
			kmsg = ikm_alloc(IKM_SAVED_MSG_SIZE);
			if (kmsg == IKM_NULL)
				return MACH_SEND_NO_BUFFER;
			ikm_init(kmsg, IKM_SAVED_MSG_SIZE);
		}
	} else {
		kmsg = ikm_alloc(msg_size);
		if (kmsg == IKM_NULL)
			return MACH_SEND_NO_BUFFER;
		ikm_init(kmsg, msg_size);
	}

	if (copyinmsg(
		    (char *) msg, (char *) &kmsg->ikm_header, send_size)) {
		ikm_free(kmsg);
		return MACH_SEND_INVALID_DATA;
	}

	/*
	 * Process trailers, excepting old
	 * message format.
	 */
	if (delta > 0) {
		if (option & MACH_SEND_TRAILER) {
		    /*
		     * The send operation includes the
		     * MACH_SEND_TRAILER option.
		     */
		    mach_msg_trailer_t		*trailer;
		    mach_msg_trailer_size_t	trailer_size;

		    trailer = (mach_msg_trailer_t *)
		    		((vm_offset_t)&kmsg->ikm_header + size);

		    if (trailer->msgh_trailer_type != 0) {
			    ikm_free(kmsg);
			    return MACH_SEND_INVALID_TRAILER;
		    }

		    trailer_size = trailer->msgh_trailer_size;
		    if (trailer_size > delta || (trailer_size & 3) ||
		    	trailer_size < MACH_MSG_TRAILER_MINIMUM_SIZE ||
		    	trailer_size > MACH_MSG_TRAILER_FORMAT_0_SIZE) {
			    ikm_free(kmsg);
			    return MACH_SEND_INVALID_TRAILER;
		    }

		    if (trailer_size > sizeof (*trailer) &&
			    copyinmsg(
		    		(char *)((vm_offset_t)msg + size +
							sizeof (*trailer)),
				(char *)((vm_offset_t)trailer +
							sizeof (*trailer)),
				trailer_size - sizeof (*trailer))) {
			    ikm_free(kmsg);
			    return MACH_SEND_INVALID_TRAILER;
		    }

		    delta = trailer_size;
		}
		else
		    delta = 0;
	}

	kmsg->ikm_delta = delta;
	kmsg->ikm_header.msgh_size = size;

	*kmsgp = kmsg;
	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	ipc_kmsg_get_from_kernel
 *	Purpose:
 *		Allocates a kernel message buffer.
 *		Copies a kernel message to the message buffer.
 *		Only resource errors are allowed.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Acquired a message buffer.
 *		MACH_SEND_NO_BUFFER	Couldn't allocate a message buffer.
 */

extern mach_msg_return_t
ipc_kmsg_get_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		size,
	integer_t		delta,
	ipc_kmsg_t		*kmsgp)
{
	ipc_kmsg_t		kmsg;
	mach_msg_size_t		send_size, msg_size;

	assert(size >= sizeof(mach_msg_header_t));
	assert((size & 3) == 0 && (delta <= 0));

	if (delta <= 0) {
		msg_size = size;
		send_size = size + delta;
	}
	else {
		if ((delta & 3) ||
		    delta < MACH_MSG_TRAILER_MINIMUM_SIZE ||
		    delta > MAX_TRAILER_SIZE)
		    	return MACH_SEND_INVALID_TRAILER;

		msg_size = size + delta;
		send_size = size + ((option & MACH_SEND_TRAILER) ?
					sizeof (mach_msg_trailer_t) : 0);
	}

	kmsg = ikm_alloc(msg_size);

	if (kmsg == IKM_NULL)
		return MACH_SEND_NO_BUFFER;
	ikm_init(kmsg, msg_size);
	kmsg->ikm_sender = KERNEL_SECURITY_ID_VALUE;

	bcopy((char *) msg, (char *) &kmsg->ikm_header, send_size);

	/*
	 * Process trailers, excepting old
	 * message format.
	 */
	if (delta > 0) {
		if (option & MACH_SEND_TRAILER) {
		    /*
		     * The send operation includes the
		     * MACH_SEND_TRAILER option.
		     */
		    mach_msg_trailer_t		*trailer;
		    mach_msg_trailer_size_t	trailer_size;

		    trailer = (mach_msg_trailer_t *)
		    		((vm_offset_t)&kmsg->ikm_header + size);

		    if (trailer->msgh_trailer_type != 0) {
			    ikm_free(kmsg);
			    return MACH_SEND_INVALID_TRAILER;
		    }

		    trailer_size = trailer->msgh_trailer_size;
		    if (trailer_size > delta || (trailer_size & 3) ||
		    	trailer_size < MACH_MSG_TRAILER_MINIMUM_SIZE ||
		    	trailer_size > MACH_MSG_TRAILER_FORMAT_0_SIZE) {
			    ikm_free(kmsg);
			    return MACH_SEND_INVALID_TRAILER;
		    }

		    if (trailer_size > sizeof (*trailer)) {
			    bcopy(
		    		(char *)((vm_offset_t)msg + size +
							sizeof (*trailer)),
				(char *)((vm_offset_t)trailer +
							sizeof (*trailer)),
				trailer_size - sizeof (*trailer));
		    }

		    delta = trailer_size;
		}
		else
		    delta = 0;
	}

	kmsg->ikm_delta = delta;
	kmsg->ikm_header.msgh_size = size;

	*kmsgp = kmsg;
	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	ipc_kmsg_put
 *	Purpose:
 *		Copies a message buffer to a user message.
 *		Copies only the specified number of bytes.
 *		Frees the message buffer.
 *	Conditions:
 *		Nothing locked.  The message buffer must have clean
 *		header (ikm_marequest) fields.
 *	Returns:
 *		MACH_MSG_SUCCESS	Copied data out of message buffer.
 *		MACH_RCV_INVALID_DATA	Couldn't copy to user message.
 */

mach_msg_return_t
ipc_kmsg_put(
	mach_msg_header_t	*msg,
	ipc_kmsg_t		kmsg,
	mach_msg_size_t		size)
{
	mach_msg_return_t mr;

	ikm_check_initialized(kmsg, kmsg->ikm_size);

	if (copyoutmsg((const char *) &kmsg->ikm_header, (char *) msg, size))
		mr = MACH_RCV_INVALID_DATA;
	else
		mr = MACH_MSG_SUCCESS;

	if ((kmsg->ikm_size == IKM_SAVED_KMSG_SIZE) &&
	    (ikm_cache() == IKM_NULL))
		ikm_cache() = kmsg;
	else
		ikm_free(kmsg);

	return mr;
}

/*
 *	Routine:	ipc_kmsg_put_to_kernel
 *	Purpose:
 *		Copies a message buffer to a kernel message.
 *		Frees the message buffer.
 *		No errors allowed.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_kmsg_put_to_kernel(
	mach_msg_header_t	*msg,
	ipc_kmsg_t		kmsg,
	mach_msg_size_t		size)
{
	bcopy((char *) &kmsg->ikm_header, (char *) msg, size);

	ikm_free(kmsg);
}

/*
 *	Routine:	ipc_kmsg_copyin_header
 *	Purpose:
 *		"Copy-in" port rights in the header of a message.
 *		Operates atomically; if it doesn't succeed the
 *		message header and the space are left untouched.
 *		If it does succeed the remote/local port fields
 *		contain object pointers instead of port names,
 *		and the bits field is updated.  The destination port
 *		will be a valid port pointer.
 *
 *		The notify argument implements the MACH_SEND_CANCEL option.
 *		If it is not MACH_PORT_NULL, it should name a receive right.
 *		If the processing of the destination port would generate
 *		a port-deleted notification (because the right for the
 *		destination port is destroyed and it had a request for
 *		a dead-name notification registered), and the port-deleted
 *		notification would be sent to the named receive right,
 *		then it isn't sent and the send-once right for the notify
 *		port is quietly destroyed.
 *
 *		[MACH_IPC_COMPAT] There is an atomicity problem if the
 *		reply port is a compat entry and dies at an inopportune
 *		time.  This doesn't have any serious consequences
 *		(an observant user task might conceivably notice that
 *		the destination and reply ports were handled inconsistently),
 *		only happens in compat mode, and is extremely unlikely.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successful copyin.
 *		MACH_SEND_INVALID_HEADER
 *			Illegal value in the message header bits.
 *		MACH_SEND_INVALID_DEST	The space is dead.
 *		MACH_SEND_INVALID_NOTIFY
 *			Notify is non-null and doesn't name a receive right.
 *			(Either KERN_INVALID_NAME or KERN_INVALID_RIGHT.)
 *		MACH_SEND_INVALID_DEST	Can't copyin destination port.
 *			(Either KERN_INVALID_NAME or KERN_INVALID_RIGHT.)
 *		MACH_SEND_INVALID_REPLY	Can't copyin reply port.
 *			(Either KERN_INVALID_NAME or KERN_INVALID_RIGHT.)
 */

mach_msg_return_t
ipc_kmsg_copyin_header(
	mach_msg_header_t	*msg,
	ipc_space_t		space,
	mach_port_t		notify)
{
	mach_msg_bits_t mbits = msg->msgh_bits &~
		(MACH_MSGH_BITS_CIRCULAR | MACH_MSGH_BITS_OLD_FORMAT);
	mach_port_t dest_name = msg->msgh_remote_port;
	mach_port_t reply_name = msg->msgh_local_port;
	kern_return_t kr;

	/* first check for common cases */

	if (notify == MACH_PORT_NULL) switch (MACH_MSGH_BITS_PORTS(mbits)) {
	    case MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0): {
		ipc_entry_t entry;
		ipc_entry_bits_t bits;
		ipc_port_t dest_port;

		/* sending an asynchronous message */

		if (reply_name != MACH_PORT_NULL)
			break;

		is_read_lock(space);
		if (!space->is_active)
			goto abort_async;

		/* optimized ipc_entry_lookup */

	    {
		mach_port_index_t index = MACH_PORT_INDEX(dest_name);
		mach_port_gen_t gen = MACH_PORT_GEN(dest_name);

		if (index >= space->is_table_size)
			goto abort_async;

		entry = &space->is_table[index];
		bits = entry->ie_bits;

		/* check generation number and type bit */

		if ((bits & (IE_BITS_GEN_MASK|MACH_PORT_TYPE_SEND)) !=
		    (gen | MACH_PORT_TYPE_SEND))
			goto abort_async;
	    }

		/* optimized ipc_right_copyin */

		assert(IE_BITS_UREFS(bits) > 0);

		dest_port = (ipc_port_t) entry->ie_object;
		assert(dest_port != IP_NULL);

		ip_lock(dest_port);
		/* can unlock space now without compromising atomicity */
		is_read_unlock(space);

		if (!ip_active(dest_port)) {
			ip_unlock(dest_port);
			break;
		}

		assert(dest_port->ip_srights > 0);
		dest_port->ip_srights++;
		ip_reference(dest_port);
		ip_unlock(dest_port);

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				  MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0));
		msg->msgh_remote_port = (mach_port_t) dest_port;
		return MACH_MSG_SUCCESS;

	    abort_async:
		is_read_unlock(space);
		break;
	    }

	    case MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
				MACH_MSG_TYPE_MAKE_SEND_ONCE): {
		ipc_entry_num_t size;
		ipc_entry_t table;
		ipc_entry_t entry;
		ipc_entry_bits_t bits;
		ipc_port_t dest_port, reply_port;

		/* sending a request message */

		is_read_lock(space);
		if (!space->is_active)
			goto abort_request;

		size = space->is_table_size;
		table = space->is_table;

		/* optimized ipc_entry_lookup of dest_name */

	    {
		mach_port_index_t index = MACH_PORT_INDEX(dest_name);
		mach_port_gen_t gen = MACH_PORT_GEN(dest_name);

		if (index >= size)
			goto abort_request;

		entry = &table[index];
		bits = entry->ie_bits;

		/* check generation number and type bit */

		if ((bits & (IE_BITS_GEN_MASK|MACH_PORT_TYPE_SEND)) !=
		    (gen | MACH_PORT_TYPE_SEND))
			goto abort_request;
	    }

		assert(IE_BITS_UREFS(bits) > 0);

		dest_port = (ipc_port_t) entry->ie_object;
		assert(dest_port != IP_NULL);

		/* optimized ipc_entry_lookup of reply_name */

	    {
		mach_port_index_t index = MACH_PORT_INDEX(reply_name);
		mach_port_gen_t gen = MACH_PORT_GEN(reply_name);

		if (index >= size)
			goto abort_request;

		entry = &table[index];
		bits = entry->ie_bits;

		/* check generation number and type bit */

		if ((bits & (IE_BITS_GEN_MASK|MACH_PORT_TYPE_RECEIVE)) !=
		    (gen | MACH_PORT_TYPE_RECEIVE))
			goto abort_request;
	    }

		reply_port = (ipc_port_t) entry->ie_object;
		assert(reply_port != IP_NULL);

		/*
		 *	To do an atomic copyin, need simultaneous
		 *	locks on both ports and the space.  If
		 *	dest_port == reply_port, and simple locking is
		 *	enabled, then we will abort.  Otherwise it's
		 *	OK to unlock twice.
		 */

		ip_lock(dest_port);
		if (!ip_active(dest_port) || !ip_lock_try(reply_port)) {
			ip_unlock(dest_port);
			goto abort_request;
		}
		/* can unlock space now without compromising atomicity */
		is_read_unlock(space);

		assert(dest_port->ip_srights > 0);
		dest_port->ip_srights++;
		ip_reference(dest_port);
		ip_unlock(dest_port);

		assert(ip_active(reply_port));
		assert(reply_port->ip_receiver_name == reply_name);
		assert(reply_port->ip_receiver == space);

		reply_port->ip_sorights++;
		ip_reference(reply_port);
		ip_unlock(reply_port);

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
			MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND,
				       MACH_MSG_TYPE_PORT_SEND_ONCE));
		msg->msgh_remote_port = (mach_port_t) dest_port;
		msg->msgh_local_port = (mach_port_t) reply_port;
		return MACH_MSG_SUCCESS;

	    abort_request:
		is_read_unlock(space);
		break;
	    }

	    case MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, 0): {
		mach_port_index_t index;
		mach_port_gen_t gen;
		ipc_entry_t table;
		ipc_entry_t entry;
		ipc_entry_bits_t bits;
		ipc_port_t dest_port;

		/* sending a reply message */

		if (reply_name != MACH_PORT_NULL)
			break;

		is_write_lock(space);
		if (!space->is_active)
			goto abort_reply;

		/* optimized ipc_entry_lookup */

		table = space->is_table;

		index = MACH_PORT_INDEX(dest_name);
		gen = MACH_PORT_GEN(dest_name);

		if (index >= space->is_table_size)
			goto abort_reply;

		entry = &table[index];
		bits = entry->ie_bits;

		/* check generation number, collision bit, and type bit */

		if ((bits & (IE_BITS_GEN_MASK|IE_BITS_COLLISION|
			     MACH_PORT_TYPE_SEND_ONCE)) !=
		    (gen | MACH_PORT_TYPE_SEND_ONCE))
			goto abort_reply;

		/* optimized ipc_right_copyin */

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
		assert(IE_BITS_UREFS(bits) == 1);
		assert((bits & IE_BITS_MAREQUEST) == 0);

		if (entry->ie_request != 0)
			goto abort_reply;

		dest_port = (ipc_port_t) entry->ie_object;
		assert(dest_port != IP_NULL);

		ip_lock(dest_port);
		if (!ip_active(dest_port)) {
			ip_unlock(dest_port);
			goto abort_reply;
		}

		assert(dest_port->ip_sorights > 0);
		ip_unlock(dest_port);

		/* optimized ipc_entry_dealloc */

		entry->ie_next = table->ie_next;
		table->ie_next = index;
		entry->ie_bits = gen;
		entry->ie_object = IO_NULL;
		is_write_unlock(space);

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				  MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE,
						 0));
		msg->msgh_remote_port = (mach_port_t) dest_port;
		return MACH_MSG_SUCCESS;

	    abort_reply:
		is_write_unlock(space);
		break;
	    }

	    default:
		/* don't bother optimizing */
		break;
	}

    {
	mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	ipc_object_t dest_port, reply_port;
	ipc_port_t dest_soright, reply_soright;
	ipc_port_t notify_port = 0; /* '=0' to quiet gcc warnings */

	if (!MACH_MSG_TYPE_PORT_ANY_SEND(dest_type))
		return MACH_SEND_INVALID_HEADER;

	if ((reply_type == 0) ?
	    (reply_name != MACH_PORT_NULL) :
	    !MACH_MSG_TYPE_PORT_ANY_SEND(reply_type))
		return MACH_SEND_INVALID_HEADER;

	is_write_lock(space);
	if (!space->is_active)
		goto invalid_dest;

	if (notify != MACH_PORT_NULL) {
		ipc_entry_t entry;

		if (((entry = ipc_entry_lookup(space, notify)) == IE_NULL) ||
		    ((entry->ie_bits & MACH_PORT_TYPE_RECEIVE) == 0)) {
			is_write_unlock(space);
			return MACH_SEND_INVALID_NOTIFY;
		}

		notify_port = (ipc_port_t) entry->ie_object;
	}

	if (dest_name == reply_name) {
		ipc_entry_t entry;
		mach_port_t name = dest_name;

		/*
		 *	Destination and reply ports are the same!
		 *	This is a little tedious to make atomic, because
		 *	there are 25 combinations of dest_type/reply_type.
		 *	However, most are easy.  If either is move-sonce,
		 *	then there must be an error.  If either are
		 *	make-send or make-sonce, then we must be looking
		 *	at a receive right so the port can't die.
		 *	The hard cases are the combinations of
		 *	copy-send and make-send.
		 */

		entry = ipc_entry_lookup(space, name);
		if (entry == IE_NULL)
			goto invalid_dest;

		assert(reply_type != 0); /* because name not null */

		if (!ipc_right_copyin_check(space, name, entry, reply_type))
			goto invalid_reply;

		if ((dest_type == MACH_MSG_TYPE_MOVE_SEND_ONCE) ||
		    (reply_type == MACH_MSG_TYPE_MOVE_SEND_ONCE)) {
			/*
			 *	Why must there be an error?  To get a valid
			 *	destination, this entry must name a live
			 *	port (not a dead name or dead port).  However
			 *	a successful move-sonce will destroy a
			 *	live entry.  Therefore the other copyin,
			 *	whatever it is, would fail.  We've already
			 *	checked for reply port errors above,
			 *	so report a destination error.
			 */

			goto invalid_dest;
		} else if ((dest_type == MACH_MSG_TYPE_MAKE_SEND) ||
			   (dest_type == MACH_MSG_TYPE_MAKE_SEND_ONCE) ||
			   (reply_type == MACH_MSG_TYPE_MAKE_SEND) ||
			   (reply_type == MACH_MSG_TYPE_MAKE_SEND_ONCE)) {
			kr = ipc_right_copyin(space, name, entry,
					      dest_type, FALSE,
					      &dest_port, &dest_soright);
			if (kr != KERN_SUCCESS)
				goto invalid_dest;

			/*
			 *	Either dest or reply needs a receive right.
			 *	We know the receive right is there, because
			 *	of the copyin_check and copyin calls.  Hence
			 *	the port is not in danger of dying.  If dest
			 *	used the receive right, then the right needed
			 *	by reply (and verified by copyin_check) will
			 *	still be there.
			 */

			assert(IO_VALID(dest_port));
			assert(entry->ie_bits & MACH_PORT_TYPE_RECEIVE);
			assert(dest_soright == IP_NULL);

			kr = ipc_right_copyin(space, name, entry,
					      reply_type, TRUE,
					      &reply_port, &reply_soright);

			assert(kr == KERN_SUCCESS);
			assert(reply_port == dest_port);
			assert(entry->ie_bits & MACH_PORT_TYPE_RECEIVE);
			assert(reply_soright == IP_NULL);
		} else if ((dest_type == MACH_MSG_TYPE_COPY_SEND) &&
			   (reply_type == MACH_MSG_TYPE_COPY_SEND)) {
			/*
			 *	To make this atomic, just do one copy-send,
			 *	and dup the send right we get out.
			 */

			kr = ipc_right_copyin(space, name, entry,
					      dest_type, FALSE,
					      &dest_port, &dest_soright);
			if (kr != KERN_SUCCESS)
				goto invalid_dest;

			assert(entry->ie_bits & MACH_PORT_TYPE_SEND);
			assert(dest_soright == IP_NULL);

			/*
			 *	It's OK if the port we got is dead now,
			 *	so reply_port is IP_DEAD, because the msg
			 *	won't go anywhere anyway.
			 */

			reply_port = (ipc_object_t)
				ipc_port_copy_send((ipc_port_t) dest_port);
			reply_soright = IP_NULL;
		} else if ((dest_type == MACH_MSG_TYPE_MOVE_SEND) &&
			   (reply_type == MACH_MSG_TYPE_MOVE_SEND)) {
			/*
			 *	This is an easy case.  Just use our
			 *	handy-dandy special-purpose copyin call
			 *	to get two send rights for the price of one.
			 */

			kr = ipc_right_copyin_two(space, name, entry,
						  &dest_port, &dest_soright);
			if (kr != KERN_SUCCESS)
				goto invalid_dest;

			/* the entry might need to be deallocated */

			if (IE_BITS_TYPE(entry->ie_bits)
						== MACH_PORT_TYPE_NONE)
				ipc_entry_dealloc(space, name, entry);

			reply_port = dest_port;
			reply_soright = IP_NULL;
		} else {
			ipc_port_t soright;

			assert(((dest_type == MACH_MSG_TYPE_COPY_SEND) &&
				(reply_type == MACH_MSG_TYPE_MOVE_SEND)) ||
			       ((dest_type == MACH_MSG_TYPE_MOVE_SEND) &&
				(reply_type == MACH_MSG_TYPE_COPY_SEND)));

			/*
			 *	To make this atomic, just do a move-send,
			 *	and dup the send right we get out.
			 */

			kr = ipc_right_copyin(space, name, entry,
					      MACH_MSG_TYPE_MOVE_SEND, FALSE,
					      &dest_port, &soright);
			if (kr != KERN_SUCCESS)
				goto invalid_dest;

			/* the entry might need to be deallocated */

			if (IE_BITS_TYPE(entry->ie_bits)
						== MACH_PORT_TYPE_NONE)
				ipc_entry_dealloc(space, name, entry);

			/*
			 *	It's OK if the port we got is dead now,
			 *	so reply_port is IP_DEAD, because the msg
			 *	won't go anywhere anyway.
			 */

			reply_port = (ipc_object_t)
				ipc_port_copy_send((ipc_port_t) dest_port);

			if (dest_type == MACH_MSG_TYPE_MOVE_SEND) {
				dest_soright = soright;
				reply_soright = IP_NULL;
			} else {
				dest_soright = IP_NULL;
				reply_soright = soright;
			}
		}
	} else if (!MACH_PORT_VALID(reply_name)) {
		ipc_entry_t entry;

		/*
		 *	No reply port!  This is an easy case
		 *	to make atomic.  Just copyin the destination.
		 */

		entry = ipc_entry_lookup(space, dest_name);
		if (entry == IE_NULL)
			goto invalid_dest;

		kr = ipc_right_copyin(space, dest_name, entry,
				      dest_type, FALSE,
				      &dest_port, &dest_soright);
		if (kr != KERN_SUCCESS)
			goto invalid_dest;

		/* the entry might need to be deallocated */

		if (IE_BITS_TYPE(entry->ie_bits) == MACH_PORT_TYPE_NONE)
			ipc_entry_dealloc(space, dest_name, entry);

		reply_port = (ipc_object_t) reply_name;
		reply_soright = IP_NULL;
	} else {
		ipc_entry_t dest_entry, reply_entry;
		ipc_port_t saved_reply;

		/*
		 *	This is the tough case to make atomic.
		 *	The difficult problem is serializing with port death.
		 *	At the time we copyin dest_port, it must be alive.
		 *	If reply_port is alive when we copyin it, then
		 *	we are OK, because we serialize before the death
		 *	of both ports.  Assume reply_port is dead at copyin.
		 *	Then if dest_port dies/died after reply_port died,
		 *	we are OK, because we serialize between the death
		 *	of the two ports.  So the bad case is when dest_port
		 *	dies after its copyin, reply_port dies before its
		 *	copyin, and dest_port dies before reply_port.  Then
		 *	the copyins operated as if dest_port was alive
		 *	and reply_port was dead, which shouldn't have happened
		 *	because they died in the other order.
		 *
		 *	We handle the bad case by undoing the copyins
		 *	(which is only possible because the ports are dead)
		 *	and failing with MACH_SEND_INVALID_DEST, serializing
		 *	after the death of the ports.
		 *
		 *	Note that it is easy for a user task to tell if
		 *	a copyin happened before or after a port died.
		 *	For example, suppose both dest and reply are
		 *	send-once rights (types are both move-sonce) and
		 *	both rights have dead-name requests registered.
		 *	If a port dies before copyin, a dead-name notification
		 *	is generated and the dead name's urefs are incremented,
		 *	and if the copyin happens first, a port-deleted
		 *	notification is generated.
		 *
		 *	Note that although the entries are different,
		 *	dest_port and reply_port might still be the same.
		 */

		dest_entry = ipc_entry_lookup(space, dest_name);
		if (dest_entry == IE_NULL)
			goto invalid_dest;

		reply_entry = ipc_entry_lookup(space, reply_name);
		if (reply_entry == IE_NULL)
			goto invalid_reply;

		assert(dest_entry != reply_entry); /* names are not equal */
		assert(reply_type != 0); /* because reply_name not null */

		if (!ipc_right_copyin_check(space, reply_name, reply_entry,
					    reply_type))
			goto invalid_reply;

		kr = ipc_right_copyin(space, dest_name, dest_entry,
				      dest_type, FALSE,
				      &dest_port, &dest_soright);
		if (kr != KERN_SUCCESS)
			goto invalid_dest;

		assert(IO_VALID(dest_port));

		saved_reply = (ipc_port_t) reply_entry->ie_object;
		/* might be IP_NULL, if this is a dead name */
		if (saved_reply != IP_NULL)
			ipc_port_reference(saved_reply);

		kr = ipc_right_copyin(space, reply_name, reply_entry,
				      reply_type, TRUE,
				      &reply_port, &reply_soright);
#if	MACH_IPC_COMPAT
		if (kr != KERN_SUCCESS) {
			assert(kr == KERN_INVALID_NAME);

			/*
			 *	Oops.  This must have been a compat entry
			 *	and the port died after the check above.
			 *	We should back out the copyin of dest_port,
			 *	and report MACH_SEND_INVALID_REPLY, but
			 *	if dest_port is alive we can't always do that.
			 *	Punt and pretend we got IO_DEAD, skipping
			 *	further hairy atomicity problems.
			 */

			reply_port = IO_DEAD;
			reply_soright = IP_NULL;
			goto skip_reply_checks;
		}
#else	MACH_IPC_COMPAT
		assert(kr == KERN_SUCCESS);
#endif	MACH_IPC_COMPAT

		if ((saved_reply != IP_NULL) && (reply_port == IO_DEAD)) {
			ipc_port_t dest = (ipc_port_t) dest_port;
			ipc_port_timestamp_t timestamp;
			boolean_t must_undo;

			/*
			 *	The reply port died before copyin.
			 *	Check if dest port died before reply.
			 */

			ip_lock(saved_reply);
			assert(!ip_active(saved_reply));
			timestamp = saved_reply->ip_timestamp;
			ip_unlock(saved_reply);

			ip_lock(dest);
			must_undo = (!ip_active(dest) &&
				     IP_TIMESTAMP_ORDER(dest->ip_timestamp,
							timestamp));
			ip_unlock(dest);

			if (must_undo) {
				/*
				 *	Our worst nightmares are realized.
				 *	Both destination and reply ports
				 *	are dead, but in the wrong order,
				 *	so we must undo the copyins and
				 *	possibly generate a dead-name notif.
				 */

				ipc_right_copyin_undo(
						space, dest_name, dest_entry,
						dest_type, dest_port,
						dest_soright);
				/* dest_entry may be deallocated now */

				ipc_right_copyin_undo(
						space, reply_name, reply_entry,
						reply_type, reply_port,
						reply_soright);
				/* reply_entry may be deallocated now */

				is_write_unlock(space);

				if (dest_soright != IP_NULL)
					ipc_notify_dead_name(dest_soright,
							     dest_name);
				assert(reply_soright == IP_NULL);

				ipc_port_release(saved_reply);
				return MACH_SEND_INVALID_DEST;
			}
		}

		/* the entries might need to be deallocated */

		if (IE_BITS_TYPE(reply_entry->ie_bits) == MACH_PORT_TYPE_NONE)
			ipc_entry_dealloc(space, reply_name, reply_entry);

#if	MACH_IPC_COMPAT
	    skip_reply_checks:
		/*
		 *	We jump here if the reply entry was a compat entry
		 *	and the port died on us.  In this case, the copyin
		 *	code already deallocated reply_entry.
		 */
#endif	MACH_IPC_COMPAT

		if (IE_BITS_TYPE(dest_entry->ie_bits) == MACH_PORT_TYPE_NONE)
			ipc_entry_dealloc(space, dest_name, dest_entry);

		if (saved_reply != IP_NULL)
			ipc_port_release(saved_reply);
	}

	/*
	 *	At this point, dest_port, reply_port,
	 *	dest_soright, reply_soright are all initialized.
	 *	Any defunct entries have been deallocated.
	 *	The space is still write-locked, and we need to
	 *	make the MACH_SEND_CANCEL check.  The notify_port pointer
	 *	is still usable, because the copyin code above won't ever
	 *	deallocate a receive right, so its entry still exists
	 *	and holds a ref.  Note notify_port might even equal
	 *	dest_port or reply_port.
	 */

	if ((notify != MACH_PORT_NULL) &&
	    (dest_soright == notify_port)) {
		ipc_port_release_sonce(dest_soright);
		dest_soright = IP_NULL;
	}

	is_write_unlock(space);

	if (dest_soright != IP_NULL)
		ipc_notify_port_deleted(dest_soright, dest_name);

	if (reply_soright != IP_NULL)
		ipc_notify_port_deleted(reply_soright, reply_name);

	dest_type = ipc_object_copyin_type(dest_type);
	reply_type = ipc_object_copyin_type(reply_type);

	msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
			  MACH_MSGH_BITS(dest_type, reply_type));
	msg->msgh_remote_port = (mach_port_t) dest_port;
	msg->msgh_local_port = (mach_port_t) reply_port;
    }

	return MACH_MSG_SUCCESS;

    invalid_dest:
	is_write_unlock(space);
	return MACH_SEND_INVALID_DEST;

    invalid_reply:
	is_write_unlock(space);
	return MACH_SEND_INVALID_REPLY;
}

/*
 *	Routine:	ipc_kmsg_copyin_body
 *	Purpose:
 *		"Copy-in" port rights and out-of-line memory
 *		in the message body.
 *
 *		In all failure cases, the message is left holding
 *		no rights or memory.  However, the message buffer
 *		is not deallocated.  If successful, the message
 *		contains a valid destination port.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successful copyin.
 *		MACH_SEND_INVALID_MEMORY	Can't grab out-of-line memory.
 *		MACH_SEND_INVALID_RIGHT	Can't copyin port right in body.
 *		MACH_SEND_INVALID_TYPE	Bad type specification.
 *		MACH_SEND_MSG_TOO_SMALL	Body is too small for types/data.
 */

mach_msg_return_t
ipc_kmsg_copyin_body(
	ipc_kmsg_t	kmsg,
	ipc_space_t	space,
	vm_map_t	map)
{
    ipc_object_t       		dest;
    mach_msg_body_t		*body;
    mach_msg_descriptor_t	*saddr, *eaddr;
    boolean_t 			complex;
    boolean_t 			use_page_lists, steal_pages;
    int				i;
    kern_return_t		kr;
    vm_size_t			space_needed = 0;
    vm_offset_t			paddr = 0;
    mach_msg_descriptor_t	*sstart;
    vm_map_copy_t		copy = VM_MAP_COPY_NULL;
    
    /*
     * Determine if the target is a kernel port.
     */
    dest = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
    complex = FALSE;
    use_page_lists = ipc_kobject_vm_page_list(ip_kotype((ipc_port_t)dest));
    steal_pages = ipc_kobject_vm_page_steal(ip_kotype((ipc_port_t)dest));
    
    body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
    saddr = (mach_msg_descriptor_t *) (body + 1);
    eaddr = saddr + body->msgh_descriptor_count;
    
    /* make sure the message does not ask for more msg descriptors
     * than the message can hold.
     */
    
    if (eaddr <= saddr ||
	eaddr > (mach_msg_descriptor_t *) (&kmsg->ikm_header + 
			    kmsg->ikm_header.msgh_size)) {
	ipc_kmsg_clean_partial(kmsg, 0, 0, 0);
	return MACH_SEND_MSG_TOO_SMALL;
    }
    
    /*
     * Make an initial pass to determine kernal VM space requirements for
     * physical copies.
     */
    for (sstart = saddr; sstart <  eaddr; sstart++) {

	if (sstart->type.type == MACH_MSG_OOL_DESCRIPTOR) {

		assert(!(sstart->out_of_line.copy == MACH_MSG_PHYSICAL_COPY &&
		       (use_page_lists || steal_pages)));

		if (sstart->out_of_line.copy != MACH_MSG_PHYSICAL_COPY &&
		    sstart->out_of_line.copy != MACH_MSG_VIRTUAL_COPY) {
		    /*
		     * Invalid copy option
		     */
		    ipc_kmsg_clean_partial(kmsg, 0, 0, 0);
		    return MACH_SEND_INVALID_TYPE;
		}
		
		if (sstart->out_of_line.copy == MACH_MSG_PHYSICAL_COPY && 
#if	MACH_OLD_VM_COPY
#else
		    sstart->out_of_line.size > MSG_OOL_SIZE_SMALL &&
#endif
		    !sstart->out_of_line.deallocate) {

		     	/*
		      	 * Out-of-line memory descriptor, accumulate kernel
		      	 * memory requirements
		      	 */
		    	space_needed += round_page(sstart->out_of_line.size);
	        }
	}
    }

    /*
     * Allocate space in the pageable kernel ipc copy map for all the
     * ool data that is to be physically copied.  Map is marked wait for
     * space.
     */
    if (space_needed > 0) {
	if (vm_allocate(ipc_kernel_copy_map, &paddr, space_needed, 
				TRUE) != KERN_SUCCESS) {
		ipc_kmsg_clean_partial(kmsg, 0, 0, 0);
		return MACH_MSG_VM_KERNEL;
	}
    }

    /* 
     * handle the OOL regions and port descriptors.
     * the check for complex messages was done earlier.
     */
    
    for (i = 0, sstart = saddr; sstart <  eaddr; sstart++) {
	
	switch (sstart->type.type) {
	    
	    case MACH_MSG_PORT_DESCRIPTOR: {
		mach_msg_type_name_t 		name;
		ipc_object_t 			object;
		mach_msg_port_descriptor_t 	*dsc;
		
		dsc = &sstart->port;
		
		/* this is really the type SEND, SEND_ONCE, etc. */
		name = dsc->disposition;
		if (!MACH_MSG_TYPE_PORT_ANY(name)) {
		    ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		    return MACH_SEND_INVALID_TYPE;
		}
		dsc->disposition = ipc_object_copyin_type(name);
		
		if (!MACH_PORT_VALID(dsc->name)) {
		    complex = TRUE;
		    break;
		}
		kr = ipc_object_copyin(space, dsc->name, name, &object);
		if (kr != KERN_SUCCESS) {
		    ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		    return MACH_SEND_INVALID_RIGHT;
		}
		if ((dsc->disposition == MACH_MSG_TYPE_PORT_RECEIVE) &&
		    ipc_port_check_circularity((ipc_port_t) object,
					       (ipc_port_t) dest)) {
		    kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_CIRCULAR;
		}
		dsc->name = (mach_port_t) object;
		complex = TRUE;
		break;
	    }
	    case MACH_MSG_OOL_DESCRIPTOR: {
		vm_size_t            		length;
		boolean_t            		dealloc;
		vm_offset_t          		addr;
		vm_offset_t          		kaddr;
		mach_msg_ool_descriptor_t 	*dsc;
		
		dsc = &sstart->out_of_line;
		dealloc = dsc->deallocate;
		addr = (vm_offset_t) dsc->address;
		
		length = dsc->size;
		
		if (length == 0) {
		    dsc->address = 0;
#if	OLD_VM_CODE
#else
		} else if (use_page_lists) {
		    int	options;

		    /*
		     * Use page list copy mechanism if specified. Since the
		     * destination is a kernel port, no RT handling is
		     * necessary. 
		     */
		    if (steal_pages == FALSE) {
			/*
			 * XXX Temporary Hackaround.
			 * XXX Because the same page
			 * XXX might be in more than one
			 * XXX out of line region, steal
			 * XXX (busy) pages from previous
			 * XXX region so that this copyin
			 * XXX won't block (permanently).
			 */
			if (copy != VM_MAP_COPY_NULL)
			    vm_map_copy_steal_pages(copy);
		    }

		    /*
		     *	Set up options for copying in page list.
		     *  If deallocating, steal pages to prevent
		     *  vm code from lazy evaluating deallocation.
		     */
		    options = VM_PROT_READ;
		    if (dealloc) {
			options |= VM_MAP_COPYIN_OPT_SRC_DESTROY |
		    			VM_MAP_COPYIN_OPT_STEAL_PAGES;
		    }
		    else if (steal_pages) {
		    	options |= VM_MAP_COPYIN_OPT_STEAL_PAGES;
		    }

		    if (vm_map_copyin_page_list(map, addr, length, options,
						&copy, FALSE)
			!= KERN_SUCCESS) {

			ipc_kmsg_clean_partial(kmsg, i, paddr, 
						       space_needed);
			return MACH_SEND_INVALID_MEMORY;
		    }

		    dsc->address = (void *) copy;
		    dsc->copy = MACH_MSG_PAGE_LIST_COPY_T;
#endif
#if	MACH_OLD_VM_COPY
#else
		} else if (length <= MSG_OOL_SIZE_SMALL &&
		    	   dsc->copy == MACH_MSG_PHYSICAL_COPY) {

		    /*
		     * If the data is 'small' enough, always kalloc space for
		     * it and copy it in.  The data will be copied out
		     * on the  message receive.  This is a performance
		     * optimization that assumes the cost of VM operations
		     * dominates the copyin/copyout overhead for 'small'
		     * regions.
		     * If the kernel is the message target, a consistent data
		     * repesentation is needed for ool data since kernel 
		     * functions may deallocate the ool data.  In this case
		     * a vm_map_copy_t is allocated along with the space for
		     * the data as an optimization. No RT handling is needed.
		     */ 
		    if (is_ipc_kobject(ip_kotype((ipc_port_t)dest))) {
			vm_map_copy_t copy;

		        copy = (vm_map_copy_t) kalloc(
					sizeof(struct vm_map_copy) + length);
			if (copy == VM_MAP_COPY_NULL) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
							space_needed);
			    return MACH_MSG_VM_KERNEL;
			}
			copy->type = VM_MAP_COPY_KERNEL_BUFFER;
		        if (copyin((const char *) addr, (char *) (copy + 1), 
								length)) {
			    kfree((vm_offset_t) copy,
				  sizeof(struct vm_map_copy) + length);
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
						 	space_needed);
			    return MACH_SEND_INVALID_MEMORY;
		        }	
		        dsc->address = (void *) copy;
			dsc->copy = MACH_MSG_KALLOC_COPY_T;
			copy->size = length;
			copy->offset = 0;
			copy->cpy_data = (vm_offset_t) (copy + 1);
		    } else {
		        if ((kaddr = kalloc(length)) == (vm_offset_t) 0) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
							space_needed);
			    return MACH_MSG_VM_KERNEL;
			}

		        if (copyin((const char *) addr, (char *) kaddr, 
								length)) {
			    kfree(kaddr, length);
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
							space_needed);
			    return MACH_SEND_INVALID_MEMORY;
		        }	
		        dsc->address = (void *) kaddr;
		    }
		    if (dealloc) {
#if	OLD_VM_CODE
			(void) vm_deallocate(map, addr, length);
#else
	    	        (void) vm_map_remove(map, trunc_page(addr), 
					     round_page(addr + length),
 					     VM_MAP_REMOVE_WAIT_FOR_KWIRE|
 					     VM_MAP_REMOVE_INTERRUPTIBLE);
#endif
		    }
#endif
		} else {
		    if ((dsc->copy == MACH_MSG_PHYSICAL_COPY) && !dealloc) {

		        /*
		         * If the request is a physical copy and the source
		         * is not being deallocated, then allocate space
		         * in the kernel's pageable ipc copy map and copy
		         * the data in.  The semantics guarantee that the
			 * data will have been physically copied before
			 * the send operation terminates.  Thus if the data
			 * is not being deallocated, we must be prepared
			 * to page if the region is sufficiently large.
		         */
		     	if (copyin((const char *) addr, (char *) paddr, 
								length)) {
			        ipc_kmsg_clean_partial(kmsg, i, paddr, 
							   space_needed);
			        return MACH_SEND_INVALID_MEMORY;
		        }	

			/*
			 * The kernel ipc copy map is marked no_zero_fill.
			 * If the transfer is not a page multiple, we need
			 * to zero fill the balance.
			 */
			if (!page_aligned(length)) {
				(void) bzero((void *) (paddr + length),
					round_page(length) - length);
			}
#if	MACH_OLD_VM_COPY
			if (vm_move(
				ipc_kernel_copy_map, paddr,
				ipc_soft_map, length, FALSE,
				(vm_offset_t *)&copy) != KERN_SUCCESS) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
						       space_needed);
			    return MACH_MSG_VM_KERNEL;
		        }
			(void) vm_deallocate(
					ipc_kernel_copy_map, paddr, length);
#else
			if (vm_map_copyin(ipc_kernel_copy_map, paddr, length,
					   TRUE, &copy) != KERN_SUCCESS) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
						       space_needed);
			    return MACH_MSG_VM_KERNEL;
		        }
#endif
			paddr += round_page(length);
			space_needed -= round_page(length);
		    } else {

			/*
			 * Make a virtual copy of the of the data if requested
			 * or if a physical copy was requested but the source
			 * is being deallocated.
			 */
#if	MACH_OLD_VM_COPY
			if (vm_move(
				map, addr,
				ipc_soft_map, length, FALSE,
				(vm_offset_t *)&copy) != KERN_SUCCESS) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
						       space_needed);
			    return MACH_SEND_INVALID_MEMORY;
		        }
			if (dealloc)
				(void) vm_deallocate(
						map, addr, length);
#else
			if (vm_map_copyin(map, addr, length,
					   dealloc, &copy) != KERN_SUCCESS) {
			    ipc_kmsg_clean_partial(kmsg, i, paddr, 
						       space_needed);
			    return MACH_SEND_INVALID_MEMORY;
		        }
#endif
		    }
		    dsc->address = (void *) copy;
		}
		complex = TRUE;
		break;
	    }
	    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
		vm_size_t 				length;
		vm_offset_t             		data;
		vm_offset_t             		addr;
		ipc_object_t            		*objects;
		int					j;
		mach_msg_type_name_t    		name;
		mach_msg_ool_ports_descriptor_t 	*dsc;
		
		dsc = &sstart->ool_ports;
		addr = (vm_offset_t) dsc->address;

		/* calculate length of data in bytes, rounding up */
		length = dsc->count * sizeof(mach_port_t);
		
		if (length == 0) {
		    complex = TRUE;
		    dsc->address = (void *) 0;
		    break;
		}

		data = kalloc(length);

		if (data == 0) {
		    ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		    return MACH_SEND_NO_BUFFER;
		}
		
		if (copyinmap(map, addr, data, length)) {
		    kfree(data, length);
		    ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		    return MACH_SEND_INVALID_MEMORY;
		}
		
		/* this is really the type SEND, SEND_ONCE, etc. */
		name = dsc->disposition;
		if (!MACH_MSG_TYPE_PORT_ANY(name)) {
		    kfree(data, length);
		    ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		    return MACH_SEND_INVALID_TYPE;
		}
		dsc->disposition = ipc_object_copyin_type(name);

		if (dsc->deallocate) {
			(void) vm_deallocate(map, addr, length);
		}
		
		dsc->address = (void *) data;
		objects = (ipc_object_t *) data;
		
		for ( j = 0; j < dsc->count; j++) {
		    mach_port_t port = (mach_port_t) objects[j];
		    ipc_object_t object;
		    
		    if (!MACH_PORT_VALID(port))
			continue;
		    
		    kr = ipc_object_copyin(space, port, name, &object);

		    if (kr != KERN_SUCCESS) {
			int k;

			for(k = 0; k < j; k++) {
			    object = objects[k];
		    	    if (!MACH_PORT_VALID(port))
			 	continue;
		    	    ipc_object_destroy(object, dsc->disposition);
			}
			kfree(data, length);
			ipc_kmsg_clean_partial(kmsg, i, paddr, 
						   space_needed);
			return MACH_SEND_INVALID_RIGHT;
		    }
		    
		    if ((dsc->disposition == MACH_MSG_TYPE_PORT_RECEIVE) &&
			ipc_port_check_circularity(
						   (ipc_port_t) object,
						   (ipc_port_t) dest))
			kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_CIRCULAR;
		    
		    objects[j] = object;
		}
		
		complex = TRUE;
		break;
	    }
	    default: {
		/*
		 * Invalid descriptor
		 */
		ipc_kmsg_clean_partial(kmsg, i, paddr, space_needed);
		return MACH_SEND_INVALID_TYPE;
	    }
	}
	i++ ;
    }
    
    if (!complex)
	kmsg->ikm_header.msgh_bits &= ~MACH_MSGH_BITS_COMPLEX;
    
    return MACH_MSG_SUCCESS;
}


/*
 *	Routine:	ipc_kmsg_copyin
 *	Purpose:
 *		"Copy-in" port rights and out-of-line memory
 *		in the message.
 *
 *		In all failure cases, the message is left holding
 *		no rights or memory.  However, the message buffer
 *		is not deallocated.  If successful, the message
 *		contains a valid destination port.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successful copyin.
 *		MACH_SEND_INVALID_HEADER
 *			Illegal value in the message header bits.
 *		MACH_SEND_INVALID_NOTIFY	Bad notify port.
 *		MACH_SEND_INVALID_DEST	Can't copyin destination port.
 *		MACH_SEND_INVALID_REPLY	Can't copyin reply port.
 *		MACH_SEND_INVALID_MEMORY	Can't grab out-of-line memory.
 *		MACH_SEND_INVALID_RIGHT	Can't copyin port right in body.
 *		MACH_SEND_INVALID_TYPE	Bad type specification.
 *		MACH_SEND_MSG_TOO_SMALL	Body is too small for types/data.
 */

mach_msg_return_t
ipc_kmsg_copyin(
	ipc_kmsg_t	kmsg,
	ipc_space_t	space,
	vm_map_t	map,
	mach_port_t	notify)
{
    mach_msg_return_t 		mr;
    
    mr = ipc_kmsg_copyin_header(&kmsg->ikm_header, space, notify);
    if (mr != MACH_MSG_SUCCESS)
	return mr;
    
    if ((kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX) == 0)
	return MACH_MSG_SUCCESS;
    
    return( ipc_kmsg_copyin_body( kmsg, space, map ) );
}

/*
 *	Routine:	ipc_kmsg_copyin_from_kernel
 *	Purpose:
 *		"Copy-in" port rights and out-of-line memory
 *		in a message sent from the kernel.
 *
 *		Because the message comes from the kernel,
 *		the implementation assumes there are no errors
 *		or peculiarities in the message.
 *
 *		Returns TRUE if queueing the message
 *		would result in a circularity.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_kmsg_copyin_from_kernel(
	ipc_kmsg_t	kmsg)
{
	mach_msg_bits_t bits = kmsg->ikm_header.msgh_bits &~
			MACH_MSGH_BITS_OLD_FORMAT;
	mach_msg_type_name_t rname = MACH_MSGH_BITS_REMOTE(bits);
	mach_msg_type_name_t lname = MACH_MSGH_BITS_LOCAL(bits);
	ipc_object_t remote = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	ipc_object_t local = (ipc_object_t) kmsg->ikm_header.msgh_local_port;

	/* translate the destination and reply ports */

	ipc_object_copyin_from_kernel(remote, rname);
	if (IO_VALID(local))
		ipc_object_copyin_from_kernel(local, lname);

	/*
	 *	The common case is a complex message with no reply port,
	 *	because that is what the memory_object interface uses.
	 */

	if (bits == (MACH_MSGH_BITS_COMPLEX |
		     MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0))) {
		bits = (MACH_MSGH_BITS_COMPLEX |
			MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0));

		kmsg->ikm_header.msgh_bits = bits;
	} else {
		bits = (MACH_MSGH_BITS_OTHER(bits) |
			MACH_MSGH_BITS(ipc_object_copyin_type(rname),
				       ipc_object_copyin_type(lname)));

		kmsg->ikm_header.msgh_bits = bits;
		if ((bits & MACH_MSGH_BITS_COMPLEX) == 0)
			return;
	}
    {
    	mach_msg_descriptor_t	*saddr, *eaddr;
    	mach_msg_body_t		*body;

    	body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
    	saddr = (mach_msg_descriptor_t *) (body + 1);
    	eaddr = (mach_msg_descriptor_t *) saddr + body->msgh_descriptor_count;

    	for ( ; saddr <  eaddr; saddr++) {

	    switch (saddr->type.type) {
	    
	        case MACH_MSG_PORT_DESCRIPTOR: {
		    mach_msg_type_name_t 	name;
		    ipc_object_t 		object;
		    mach_msg_port_descriptor_t 	*dsc;
		
		    dsc = &saddr->port;
		
		    /* this is really the type SEND, SEND_ONCE, etc. */
		    name = dsc->disposition;
		    object = (ipc_object_t) dsc->name;
		    dsc->disposition = ipc_object_copyin_type(name);
		
		    if (!IO_VALID(object)) {
		        break;
		    }

		    ipc_object_copyin_from_kernel(object, name);
		    
		    if ((dsc->disposition == MACH_MSG_TYPE_PORT_RECEIVE) &&
		        ipc_port_check_circularity((ipc_port_t) object, 
						(ipc_port_t) remote)) {
		        kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_CIRCULAR;
		    }
		    break;
	        }
	        case MACH_MSG_OOL_DESCRIPTOR: {
		    /*
		     * The sender should supply ready-made memory, i.e.
		     * a vm_map_copy_t, so we don't need to do anything.
		     */
		    break;
	        }
	        case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
		    ipc_object_t            		*objects;
		    int					j;
		    mach_msg_type_name_t    		name;
		    mach_msg_ool_ports_descriptor_t 	*dsc;
		
		    dsc = &saddr->ool_ports;

		    /* this is really the type SEND, SEND_ONCE, etc. */
		    name = dsc->disposition;
		    dsc->disposition = ipc_object_copyin_type(name);
	    	
		    objects = (ipc_object_t *) dsc->address;
	    	
		    for ( j = 0; j < dsc->count; j++) {
		        ipc_object_t object = objects[j];
		        
		        if (!IO_VALID(object))
			    continue;
		        
		        ipc_object_copyin_from_kernel(object, name);
    
		        if ((dsc->disposition == MACH_MSG_TYPE_PORT_RECEIVE) &&
			    ipc_port_check_circularity(
						       (ipc_port_t) object,
						       (ipc_port_t) remote))
			    kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_CIRCULAR;
		    }
		    break;
	        }
	        default: {
#if	MACH_ASSERT
		   panic("ipc_kmsg_copyin_from_kernel: bad descriptor type %d",
						saddr->type.type);
#endif	/* MACH_ASSERT */
		}
	    }
	}
    }
}

/*
 *	Routine:	ipc_kmsg_copyout_header
 *	Purpose:
 *		"Copy-out" port rights in the header of a message.
 *		Operates atomically; if it doesn't succeed the
 *		message header and the space are left untouched.
 *		If it does succeed the remote/local port fields
 *		contain port names instead of object pointers,
 *		and the bits field is updated.
 *
 *		The notify argument implements the MACH_RCV_NOTIFY option.
 *		If it is not MACH_PORT_NULL, it should name a receive right.
 *		If the process of receiving the reply port creates a
 *		new right in the receiving task, then the new right is
 *		automatically registered for a dead-name notification,
 *		with the notify port supplying the send-once right.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Copied out port rights.
 *		MACH_RCV_INVALID_NOTIFY	
 *			Notify is non-null and doesn't name a receive right.
 *			(Either KERN_INVALID_NAME or KERN_INVALID_RIGHT.)
 *		MACH_RCV_HEADER_ERROR|MACH_MSG_IPC_SPACE
 *			The space is dead.
 *		MACH_RCV_HEADER_ERROR|MACH_MSG_IPC_SPACE
 *			No room in space for another name.
 *		MACH_RCV_HEADER_ERROR|MACH_MSG_IPC_KERNEL
 *			Couldn't allocate memory for the reply port.
 *		MACH_RCV_HEADER_ERROR|MACH_MSG_IPC_KERNEL
 *			Couldn't allocate memory for the dead-name request.
 */

mach_msg_return_t
ipc_kmsg_copyout_header(
	mach_msg_header_t	*msg,
	ipc_space_t		space,
	mach_port_t		notify)
{
	mach_msg_bits_t mbits = msg->msgh_bits;
	ipc_port_t dest = (ipc_port_t) msg->msgh_remote_port;

	assert(IP_VALID(dest));

	/* first check for common cases */

	if (notify == MACH_PORT_NULL) switch (MACH_MSGH_BITS_PORTS(mbits)) {
	    case MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND, 0): {
		mach_port_t dest_name;
		ipc_port_t nsrequest;

		/* receiving an asynchronous message */

		ip_lock(dest);
		if (!ip_active(dest)) {
			ip_unlock(dest);
			break;
		}

		/* optimized ipc_object_copyout_dest */

		assert(dest->ip_srights > 0);
		ip_release(dest);

		if (dest->ip_receiver == space)
			dest_name = dest->ip_receiver_name;
		else
			dest_name = MACH_PORT_NULL;

		if ((--dest->ip_srights == 0) &&
		    ((nsrequest = dest->ip_nsrequest) != IP_NULL)) {
			mach_port_mscount_t mscount;

			dest->ip_nsrequest = IP_NULL;
			mscount = dest->ip_mscount;
			ip_unlock(dest);

			ipc_notify_no_senders(nsrequest, mscount);
		} else
			ip_unlock(dest);

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				  MACH_MSGH_BITS(0, MACH_MSG_TYPE_PORT_SEND));
		msg->msgh_local_port = dest_name;
		msg->msgh_remote_port = MACH_PORT_NULL;
		return MACH_MSG_SUCCESS;
	    }

	    case MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND,
				MACH_MSG_TYPE_PORT_SEND_ONCE): {
		ipc_entry_t table;
		mach_port_index_t index;
		ipc_entry_t entry;
		ipc_port_t reply = (ipc_port_t) msg->msgh_local_port;
		mach_port_t dest_name, reply_name;
		ipc_port_t nsrequest;

		/* receiving a request message */

		if (!IP_VALID(reply))
			break;

		is_write_lock(space);
		if (!space->is_active ||
		    ((index = (table = space->is_table)->ie_next) == 0)) {
			is_write_unlock(space);
			break;
		}

		/*
		 *	To do an atomic copyout, need simultaneous
		 *	locks on both ports and the space.  If
		 *	dest == reply, and simple locking is
		 *	enabled, then we will abort.  Otherwise it's
		 *	OK to unlock twice.
		 */

		ip_lock(dest);
		if (!ip_active(dest) || !ip_lock_try(reply)) {
			ip_unlock(dest);
			is_write_unlock(space);
			break;
		}

		if (!ip_active(reply)) {
			ip_unlock(reply);
			ip_unlock(dest);
			is_write_unlock(space);
			break;
		}

		assert(reply->ip_sorights > 0);
		ip_unlock(reply);

		/* optimized ipc_entry_get */

		entry = &table[index];
		table->ie_next = entry->ie_next;
		entry->ie_request = 0;

	    {
		mach_port_gen_t gen;

		assert((entry->ie_bits &~ IE_BITS_GEN_MASK) == 0);
		gen = entry->ie_bits + IE_BITS_GEN_ONE;

		reply_name = MACH_PORT_MAKE(index, gen);

		/* optimized ipc_right_copyout */

		entry->ie_bits = gen | (MACH_PORT_TYPE_SEND_ONCE | 1);
	    }

		assert(MACH_PORT_VALID(reply_name));
		entry->ie_object = (ipc_object_t) reply;
		is_write_unlock(space);

		/* optimized ipc_object_copyout_dest */

		assert(dest->ip_srights > 0);
		ip_release(dest);

		if (dest->ip_receiver == space)
			dest_name = dest->ip_receiver_name;
		else
			dest_name = MACH_PORT_NULL;

		if ((--dest->ip_srights == 0) &&
		    ((nsrequest = dest->ip_nsrequest) != IP_NULL)) {
			mach_port_mscount_t mscount;

			dest->ip_nsrequest = IP_NULL;
			mscount = dest->ip_mscount;
			ip_unlock(dest);

			ipc_notify_no_senders(nsrequest, mscount);
		} else
			ip_unlock(dest);

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				  MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE,
						 MACH_MSG_TYPE_PORT_SEND));
		msg->msgh_local_port = dest_name;
		msg->msgh_remote_port = reply_name;
		return MACH_MSG_SUCCESS;
	    }

	    case MACH_MSGH_BITS(MACH_MSG_TYPE_PORT_SEND_ONCE, 0): {
		mach_port_t dest_name;

		/* receiving a reply message */

		ip_lock(dest);
		if (!ip_active(dest)) {
			ip_unlock(dest);
			break;
		}

		/* optimized ipc_object_copyout_dest */

		assert(dest->ip_sorights > 0);

		if (dest->ip_receiver == space) {
			ip_release(dest);
			dest->ip_sorights--;
			dest_name = dest->ip_receiver_name;
			ip_unlock(dest);
		} else {
			ip_unlock(dest);

			ipc_notify_send_once(dest);
			dest_name = MACH_PORT_NULL;
		}

		msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
			MACH_MSGH_BITS(0, MACH_MSG_TYPE_PORT_SEND_ONCE));
		msg->msgh_local_port = dest_name;
		msg->msgh_remote_port = MACH_PORT_NULL;
		return MACH_MSG_SUCCESS;
	    }

	    default:
		/* don't bother optimizing */
		break;
	}

    {
	mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	ipc_port_t reply = (ipc_port_t) msg->msgh_local_port;
	mach_port_t dest_name, reply_name;

	if (IP_VALID(reply)) {
		ipc_port_t notify_port;
		ipc_entry_t entry;
		kern_return_t kr;

		/*
		 *	Handling notify (for MACH_RCV_NOTIFY) is tricky.
		 *	The problem is atomically making a send-once right
		 *	from the notify port and installing it for a
		 *	dead-name request in the new entry, because this
		 *	requires two port locks (on the notify port and
		 *	the reply port).  However, we can safely make
		 *	and consume send-once rights for the notify port
		 *	as long as we hold the space locked.  This isn't
		 *	an atomicity problem, because the only way
		 *	to detect that a send-once right has been created
		 *	and then consumed if it wasn't needed is by getting
		 *	at the receive right to look at ip_sorights, and
		 *	because the space is write-locked status calls can't
		 *	lookup the notify port receive right.  When we make
		 *	the send-once right, we lock the notify port,
		 *	so any status calls in progress will be done.
		 */

		is_write_lock(space);

		for (;;) {
			ipc_port_request_index_t request;

			if (!space->is_active) {
				is_write_unlock(space);
				return (MACH_RCV_HEADER_ERROR|
					MACH_MSG_IPC_SPACE);
			}

			if (notify != MACH_PORT_NULL) {
				notify_port = ipc_port_lookup_notify(space,
								     notify);
				if (notify_port == IP_NULL) {
					is_write_unlock(space);
					return MACH_RCV_INVALID_NOTIFY;
				}
			} else
				notify_port = IP_NULL;

			if ((reply_type != MACH_MSG_TYPE_PORT_SEND_ONCE) &&
			    ipc_right_reverse(space, (ipc_object_t) reply,
					      &reply_name, &entry)) {
				/* reply port is locked and active */

				/*
				 *	We don't need the notify_port
				 *	send-once right, but we can't release
				 *	it here because reply port is locked.
				 *	Wait until after the copyout to
				 *	release the notify port right.
				 */

				assert(entry->ie_bits &
						MACH_PORT_TYPE_SEND_RECEIVE);
				break;
			}

			ip_lock(reply);
			if (!ip_active(reply)) {
				ip_release(reply);
				ip_check_unlock(reply);

				if (notify_port != IP_NULL)
					ipc_port_release_sonce(notify_port);

				ip_lock(dest);
				is_write_unlock(space);

				reply = IP_DEAD;
				reply_name = MACH_PORT_DEAD;
				goto copyout_dest;
			}

			kr = ipc_entry_get(space, &reply_name, &entry);
			if (kr != KERN_SUCCESS) {
				ip_unlock(reply);

				if (notify_port != IP_NULL)
					ipc_port_release_sonce(notify_port);

				/* space is locked */
				kr = ipc_entry_grow_table(space,
							  ITS_SIZE_NONE);
				if (kr != KERN_SUCCESS) {
					/* space is unlocked */

					if (kr == KERN_RESOURCE_SHORTAGE)
						return (MACH_RCV_HEADER_ERROR|
							MACH_MSG_IPC_KERNEL);
					else
						return (MACH_RCV_HEADER_ERROR|
							MACH_MSG_IPC_SPACE);
				}
				/* space is locked again; start over */

				continue;
			}

			assert(IE_BITS_TYPE(entry->ie_bits)
						== MACH_PORT_TYPE_NONE);
			assert(entry->ie_object == IO_NULL);

			if (notify_port == IP_NULL) {
				/* not making a dead-name request */

				entry->ie_object = (ipc_object_t) reply;
				break;
			}

			kr = ipc_port_dnrequest(reply, reply_name,
						notify_port, &request);
			if (kr != KERN_SUCCESS) {
				ip_unlock(reply);

				ipc_port_release_sonce(notify_port);

				ipc_entry_dealloc(space, reply_name, entry);
				is_write_unlock(space);

				ip_lock(reply);
				if (!ip_active(reply)) {
					/* will fail next time around loop */

					ip_unlock(reply);
					is_write_lock(space);
					continue;
				}

				kr = ipc_port_dngrow(reply, ITS_SIZE_NONE);
				/* port is unlocked */
				if (kr != KERN_SUCCESS)
					return (MACH_RCV_HEADER_ERROR|
						MACH_MSG_IPC_KERNEL);

				is_write_lock(space);
				continue;
			}

			notify_port = IP_NULL; /* don't release right below */

			entry->ie_object = (ipc_object_t) reply;
			entry->ie_request = request;
			break;
		}

		/* space and reply port are locked and active */

		ip_reference(reply);	/* hold onto the reply port */

		kr = ipc_right_copyout(space, reply_name, entry,
				       reply_type, TRUE, (ipc_object_t) reply);
		/* reply port is unlocked */
		assert(kr == KERN_SUCCESS);

		if (notify_port != IP_NULL)
			ipc_port_release_sonce(notify_port);

		ip_lock(dest);
		is_write_unlock(space);
	} else {
		/*
		 *	No reply port!  This is an easy case.
		 *	We only need to have the space locked
		 *	when checking notify and when locking
		 *	the destination (to ensure atomicity).
		 */

		is_read_lock(space);
		if (!space->is_active) {
			is_read_unlock(space);
			return MACH_RCV_HEADER_ERROR|MACH_MSG_IPC_SPACE;
		}

		if (notify != MACH_PORT_NULL) {
			ipc_entry_t entry;

			/* must check notify even though it won't be used */

			if (((entry = ipc_entry_lookup(space, notify))
								== IE_NULL) ||
			    ((entry->ie_bits & MACH_PORT_TYPE_RECEIVE) == 0)) {
				is_read_unlock(space);
				return MACH_RCV_INVALID_NOTIFY;
			}
		}

		ip_lock(dest);
		is_read_unlock(space);

		reply_name = (mach_port_t) reply;
	}

	/*
	 *	At this point, the space is unlocked and the destination
	 *	port is locked.  (Lock taken while space was locked.)
	 *	reply_name is taken care of; we still need dest_name.
	 *	We still hold a ref for reply (if it is valid).
	 *
	 *	If the space holds receive rights for the destination,
	 *	we return its name for the right.  Otherwise the task
	 *	managed to destroy or give away the receive right between
	 *	receiving the message and this copyout.  If the destination
	 *	is dead, return MACH_PORT_DEAD, and if the receive right
	 *	exists somewhere else (another space, in transit)
	 *	return MACH_PORT_NULL.
	 *
	 *	Making this copyout operation atomic with the previous
	 *	copyout of the reply port is a bit tricky.  If there was
	 *	no real reply port (it wasn't IP_VALID) then this isn't
	 *	an issue.  If the reply port was dead at copyout time,
	 *	then we are OK, because if dest is dead we serialize
	 *	after the death of both ports and if dest is alive
	 *	we serialize after reply died but before dest's (later) death.
	 *	So assume reply was alive when we copied it out.  If dest
	 *	is alive, then we are OK because we serialize before
	 *	the ports' deaths.  So assume dest is dead when we look at it.
	 *	If reply dies/died after dest, then we are OK because
	 *	we serialize after dest died but before reply dies.
	 *	So the hard case is when reply is alive at copyout,
	 *	dest is dead at copyout, and reply died before dest died.
	 *	In this case pretend that dest is still alive, so
	 *	we serialize while both ports are alive.
	 *
	 *	Because the space lock is held across the copyout of reply
	 *	and locking dest, the receive right for dest can't move
	 *	in or out of the space while the copyouts happen, so
	 *	that isn't an atomicity problem.  In the last hard case
	 *	above, this implies that when dest is dead that the
	 *	space couldn't have had receive rights for dest at
	 *	the time reply was copied-out, so when we pretend
	 *	that dest is still alive, we can return MACH_PORT_NULL.
	 *
	 *	If dest == reply, then we have to make it look like
	 *	either both copyouts happened before the port died,
	 *	or both happened after the port died.  This special
	 *	case works naturally if the timestamp comparison
	 *	is done correctly.
	 */

    copyout_dest:

	if (ip_active(dest)) {
		ipc_object_copyout_dest(space, (ipc_object_t) dest,
					dest_type, &dest_name);
		/* dest is unlocked */
	} else {
		ipc_port_timestamp_t timestamp;

		timestamp = dest->ip_timestamp;
		ip_release(dest);
		ip_check_unlock(dest);

		if (IP_VALID(reply)) {
			ip_lock(reply);
			if (ip_active(reply) ||
			    IP_TIMESTAMP_ORDER(timestamp,
					       reply->ip_timestamp))
				dest_name = MACH_PORT_DEAD;
			else
				dest_name = MACH_PORT_NULL;
			ip_unlock(reply);
		} else
			dest_name = MACH_PORT_DEAD;
	}

	if (IP_VALID(reply))
		ipc_port_release(reply);

	msg->msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
			  MACH_MSGH_BITS(reply_type, dest_type));
	msg->msgh_local_port = dest_name;
	msg->msgh_remote_port = reply_name;
    }

	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	ipc_kmsg_copyout_object
 *	Purpose:
 *		Copy-out a port right.  Always returns a name,
 *		even for unsuccessful return codes.  Always
 *		consumes the supplied object.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	The space acquired the right
 *			(name is valid) or the object is dead (MACH_PORT_DEAD).
 *		MACH_MSG_IPC_SPACE	No room in space for the right,
 *			or the space is dead.  (Name is MACH_PORT_NULL.)
 *		MACH_MSG_IPC_KERNEL	Kernel resource shortage.
 *			(Name is MACH_PORT_NULL.)
 */

mach_msg_return_t
ipc_kmsg_copyout_object(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	mach_port_t		*namep)
{
	if (!IO_VALID(object)) {
		*namep = (mach_port_t) object;
		return MACH_MSG_SUCCESS;
	}

	/*
	 *	Attempt quick copyout of send rights.  We optimize for a
	 *	live port for which the receiver holds send (and not
	 *	receive) rights in his local table.
	 */

	if (msgt_name != MACH_MSG_TYPE_PORT_SEND)
		goto slow_copyout;

    {
	register ipc_port_t port = (ipc_port_t) object;
	ipc_entry_t entry;

	is_write_lock(space);
	if (!space->is_active) {
		is_write_unlock(space);
		goto slow_copyout;
	}

	ip_lock(port);
	if (!ip_active(port) ||
	    !ipc_hash_local_lookup(space, (ipc_object_t) port,
				   namep, &entry)) {
		ip_unlock(port);
		is_write_unlock(space);
		goto slow_copyout;
	}

	/*
	 *	Copyout the send right, incrementing urefs
	 *	unless it would overflow, and consume the right.
	 */

	assert(port->ip_srights > 1);
	port->ip_srights--;
	ip_release(port);
	ip_unlock(port);

	assert(entry->ie_bits & MACH_PORT_TYPE_SEND);
	assert(IE_BITS_UREFS(entry->ie_bits) > 0);
	assert(IE_BITS_UREFS(entry->ie_bits) < MACH_PORT_UREFS_MAX);

    {
	register ipc_entry_bits_t bits = entry->ie_bits + 1;

	if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX)
		entry->ie_bits = bits;
    }

	is_write_unlock(space);
	return MACH_MSG_SUCCESS;
    }

    slow_copyout: {
	kern_return_t kr;

	kr = ipc_object_copyout(space, object, msgt_name, TRUE, namep);
	if (kr != KERN_SUCCESS) {
		ipc_object_destroy(object, msgt_name);

		if (kr == KERN_INVALID_CAPABILITY)
			*namep = MACH_PORT_DEAD;
		else {
			*namep = MACH_PORT_NULL;

			if (kr == KERN_RESOURCE_SHORTAGE)
				return MACH_MSG_IPC_KERNEL;
			else
				return MACH_MSG_IPC_SPACE;
		}
	}

	return MACH_MSG_SUCCESS;
    }
}

#define SKIP_PORT_DESCRIPTORS(s, e)					\
MACRO_BEGIN								\
	if ((s) != MACH_MSG_DESCRIPTOR_NULL) {				\
		while ((s) < (e)) {					\
			if ((s)->type.type != MACH_MSG_PORT_DESCRIPTOR)	\
				break;					\
			(s)++;						\
		}							\
		if ((s) >= (e))						\
			(s) = MACH_MSG_DESCRIPTOR_NULL;			\
	}								\
MACRO_END

#define INCREMENT_SCATTER(s)						\
MACRO_BEGIN								\
	if ((s) != MACH_MSG_DESCRIPTOR_NULL) {				\
		(s)++;							\
	}								\
MACRO_END

/*
 *	Routine:	ipc_kmsg_copyout_body
 *	Purpose:
 *		"Copy-out" port rights and out-of-line memory
 *		in the body of a message.
 *
 *		The error codes are a combination of special bits.
 *		The copyout proceeds despite errors.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successfull copyout.
 *		MACH_MSG_IPC_SPACE	No room for port right in name space.
 *		MACH_MSG_VM_SPACE	No room for memory in address space.
 *		MACH_MSG_IPC_KERNEL	Resource shortage handling port right.
 *		MACH_MSG_VM_KERNEL	Resource shortage handling memory.
 */

mach_msg_return_t
ipc_kmsg_copyout_body(
    	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	ipc_kmsg_t		list)
{
    mach_msg_body_t 		*body;
    mach_msg_descriptor_t 	*saddr, *eaddr;
    mach_msg_return_t 		mr = MACH_MSG_SUCCESS;
    kern_return_t 		kr;
    vm_offset_t         	data;
    mach_msg_descriptor_t 	*sstart, *send;

    body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
    saddr = (mach_msg_descriptor_t *) (body + 1);
    eaddr = saddr + body->msgh_descriptor_count;

    /*
     * Do scatter list setup
     */
    if (list != IKM_NULL) {
    	mach_msg_body_t		*sbody =
		(mach_msg_body_t *)(&list->ikm_header + 1);

	sstart = (mach_msg_descriptor_t *) (sbody + 1);
	send = sstart + sbody->msgh_descriptor_count;
    }
    else
	sstart = MACH_MSG_DESCRIPTOR_NULL;

    for ( ; saddr < eaddr; saddr++ ) {
	
	switch (saddr->type.type) {
	    
	    case MACH_MSG_PORT_DESCRIPTOR: {
		mach_msg_port_descriptor_t *dsc;
		
		/* 
		 * Copyout port right carried in the message 
		 */
		dsc = &saddr->port;
		mr |= ipc_kmsg_copyout_object(space, 
					      (ipc_object_t) dsc->name, 
					      dsc->disposition, 
					      (mach_port_t *) &dsc->name);

		break;
	    }
	    case MACH_MSG_OOL_DESCRIPTOR : {
		vm_offset_t			rcv_addr;
		vm_offset_t			snd_addr;
		mach_msg_ool_descriptor_t	*dsc;
		mach_msg_copy_options_t		copy_option;

		SKIP_PORT_DESCRIPTORS(sstart, send);

		dsc = &saddr->out_of_line;

		assert(dsc->copy != MACH_MSG_KALLOC_COPY_T);
		assert(dsc->copy != MACH_MSG_PAGE_LIST_COPY_T);

		copy_option = dsc->copy;

		if ((snd_addr = (vm_offset_t) dsc->address) != 0) {
		    if (sstart != MACH_MSG_DESCRIPTOR_NULL &&
			sstart->out_of_line.copy == MACH_MSG_OVERWRITE) {

			/*
			 * There is an overwrite descriptor specified in the
			 * scatter list for this ool data.  The descriptor
			 * has already been verified
			 */
			rcv_addr = (vm_offset_t) sstart->out_of_line.address;
			dsc->copy = MACH_MSG_OVERWRITE;
		    } else {
			dsc->copy = MACH_MSG_ALLOCATE;
		    }

#if	MACH_OLD_VM_COPY
		    if (dsc->copy == MACH_MSG_OVERWRITE) {
		    	kr = vm_map_copy(
					map, ipc_soft_map,
					rcv_addr, dsc->size, snd_addr,
					FALSE, FALSE);
		    }
		    else {
		    	kr = vm_move(
				ipc_soft_map, snd_addr,
				map, dsc->size, FALSE, &rcv_addr);
		    }
		    (void) vm_deallocate(ipc_soft_map, snd_addr, dsc->size);
		    if (kr != KERN_SUCCESS) {
			if (kr == KERN_RESOURCE_SHORTAGE)
			    mr |= MACH_MSG_VM_KERNEL;
			else
			    mr |= MACH_MSG_VM_SPACE;
			dsc->address = 0;
			INCREMENT_SCATTER(sstart);
			break;
		    }
#else
		    if (copy_option == MACH_MSG_PHYSICAL_COPY &&
			    dsc->size <= MSG_OOL_SIZE_SMALL(rt)) {

			/* 
			 * Sufficiently 'small' data was copied into a kalloc'ed
			 * buffer copy was requested.  Just copy it out and 
			 * free the buffer.
			 */
			if (dsc->copy == MACH_MSG_ALLOCATE) {

			    /*
			     * If there is no overwrite region, allocate
			     * space in receiver's address space for the
			     * data
			     */
			    if ((kr = vm_allocate(map, &rcv_addr, dsc->size, 
					TRUE)) != KERN_SUCCESS) {
		    	        if (kr == KERN_RESOURCE_SHORTAGE)
				        mr |= MACH_MSG_VM_KERNEL;
		    	        else
				        mr |= MACH_MSG_VM_SPACE;
			        kfree(snd_addr, dsc->size);
			        dsc->address = (void *) 0;
				INCREMENT_SCATTER(sstart);
			        break;
			    }
			}
			(void) copyoutmap(map, snd_addr, rcv_addr, dsc->size);
			kfree(snd_addr, dsc->size);
		    } else {

			/*
			 * Whether the data was virtually or physically
			 * copied we have a vm_map_copy_t for it.
			 * If there's an overwrite region specified
			 * overwrite it, otherwise do a virtual copy out.
			 */
			if (dsc->copy == MACH_MSG_OVERWRITE) {
			    kr = vm_map_copy_overwrite(map, rcv_addr,
					(vm_map_copy_t) dsc->address, TRUE);
			} else {
			    kr = vm_map_copyout(map, &rcv_addr, 
						(vm_map_copy_t) dsc->address);
			}	
			if (kr != KERN_SUCCESS) {
		    	    if (kr == KERN_RESOURCE_SHORTAGE)
				mr |= MACH_MSG_VM_KERNEL;
		    	    else
				mr |= MACH_MSG_VM_SPACE;
		    	    vm_map_copy_discard((vm_map_copy_t) dsc->address);
			    dsc->address = 0;
			    INCREMENT_SCATTER(sstart);
			    break;
			}
		    }
#endif
		    dsc->address = (void *) rcv_addr;
		}
		INCREMENT_SCATTER(sstart);
		break;
	    }
	    case MACH_MSG_OOL_PORTS_DESCRIPTOR : {
		vm_offset_t            		addr;
		mach_port_t            		*objects;
		mach_msg_type_number_t 		j;
    		vm_size_t           		length;
		mach_msg_ool_ports_descriptor_t	*dsc;

		SKIP_PORT_DESCRIPTORS(sstart, send);

		dsc = &saddr->ool_ports;

		length = dsc->count * sizeof(mach_port_t);

		if (length != 0) {
		    if (sstart != MACH_MSG_DESCRIPTOR_NULL &&
			sstart->ool_ports.copy == MACH_MSG_OVERWRITE) {

			/*
			 * There is an overwrite descriptor specified in the
			 * scatter list for this ool data.  The descriptor
			 * has already been verified
			 */
			addr = (vm_offset_t) sstart->out_of_line.address;
			dsc->copy = MACH_MSG_OVERWRITE;
		    } 
		    else {

		   	/*
			 * Dynamically allocate the region
			 */
			dsc->copy = MACH_MSG_ALLOCATE;
		    	if ((kr = vm_allocate(map, &addr, length, TRUE)) !=
		    						KERN_SUCCESS) {
			    ipc_kmsg_clean_body(kmsg,
						body->msgh_descriptor_count);
			    dsc->address = 0;

			    if (kr == KERN_RESOURCE_SHORTAGE){
			    	mr |= MACH_MSG_VM_KERNEL;
			    } else {
			    	mr |= MACH_MSG_VM_SPACE;
			    }
			    INCREMENT_SCATTER(sstart);
			    break;
		    	}
		    }
		} else {
		    assert(dsc->address == 0);
		    INCREMENT_SCATTER(sstart);
		    break;
		}

		
		objects = (mach_port_t *) dsc->address ;
		
		/* copyout port rights carried in the message */
		
		for ( j = 0; j < dsc->count ; j++) {
		    ipc_object_t object =
			(ipc_object_t) objects[j];
		    
		    mr |= ipc_kmsg_copyout_object(space, object,
					dsc->disposition, &objects[j]);
		}

		/* copyout to memory allocated above */
		
		data = (vm_offset_t) dsc->address;
		(void) copyoutmap(map, data, addr, length);
		kfree(data, length);
		
		dsc->address = (void *) addr;
		INCREMENT_SCATTER(sstart);
		break;
	    }
	    default : {
		panic("ipc_kmsg_copyout_body: bad descriptor type %d",
	      					saddr->type.type);
	    }
	}
    }
    return mr;
}

/*
 *	Routine:	ipc_kmsg_copyout
 *	Purpose:
 *		"Copy-out" port rights and out-of-line memory
 *		in the message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Copied out all rights and memory.
 *		MACH_RCV_INVALID_NOTIFY	Bad notify port.
 *			Rights and memory in the message are intact.
 *		MACH_RCV_HEADER_ERROR + special bits
 *			Rights and memory in the message are intact.
 *		MACH_RCV_BODY_ERROR + special bits
 *			The message header was successfully copied out.
 *			As much of the body was handled as possible.
 */

mach_msg_return_t
ipc_kmsg_copyout(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	mach_port_t		notify,
	ipc_kmsg_t		list)
{
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	mach_msg_return_t mr;

	mr = ipc_kmsg_copyout_header(&kmsg->ikm_header, space, notify);
	if (mr != MACH_MSG_SUCCESS)
		return mr;

	if (mbits & MACH_MSGH_BITS_COMPLEX) {
		mr = ipc_kmsg_copyout_body(kmsg, space, map, list);

		if (mr != MACH_MSG_SUCCESS)
			mr |= MACH_RCV_BODY_ERROR;
	}

	return mr;
}

/*
 *	Routine:	ipc_kmsg_copyout_pseudo
 *	Purpose:
 *		Does a pseudo-copyout of the message.
 *		This is like a regular copyout, except
 *		that the ports in the header are handled
 *		as if they are in the body.  They aren't reversed.
 *
 *		The error codes are a combination of special bits.
 *		The copyout proceeds despite errors.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successful copyout.
 *		MACH_MSG_IPC_SPACE	No room for port right in name space.
 *		MACH_MSG_VM_SPACE	No room for memory in address space.
 *		MACH_MSG_IPC_KERNEL	Resource shortage handling port right.
 *		MACH_MSG_VM_KERNEL	Resource shortage handling memory.
 */

mach_msg_return_t
ipc_kmsg_copyout_pseudo(
	ipc_kmsg_t		kmsg,
	ipc_space_t		space,
	vm_map_t		map,
	ipc_kmsg_t		list)
{
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	ipc_object_t dest = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	ipc_object_t reply = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	mach_port_t dest_name, reply_name;
	mach_msg_return_t mr;

	assert(IO_VALID(dest));

	mr = (ipc_kmsg_copyout_object(space, dest, dest_type, &dest_name) |
	      ipc_kmsg_copyout_object(space, reply, reply_type, &reply_name));

	kmsg->ikm_header.msgh_bits = mbits &~ MACH_MSGH_BITS_CIRCULAR;
	kmsg->ikm_header.msgh_remote_port = dest_name;
	kmsg->ikm_header.msgh_local_port = reply_name;

	if (mbits & MACH_MSGH_BITS_COMPLEX) {
		mr |= ipc_kmsg_copyout_body(kmsg, space, map, list);
	}

	return mr;
}

/*
 *	Routine:	ipc_kmsg_copyout_dest
 *	Purpose:
 *		Copies out the destination port in the message.
 *		Destroys all other rights and memory in the message.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_kmsg_copyout_dest(
	ipc_kmsg_t	kmsg,
	ipc_space_t	space)
{
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	ipc_object_t dest = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	ipc_object_t reply = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	mach_port_t dest_name, reply_name;

	assert(IO_VALID(dest));

	io_lock(dest);
	if (io_active(dest)) {
		ipc_object_copyout_dest(space, dest, dest_type, &dest_name);
		/* dest is unlocked */
	} else {
		io_release(dest);
		io_check_unlock(dest);
		dest_name = MACH_PORT_DEAD;
	}

	if (IO_VALID(reply)) {
		ipc_object_destroy(reply, reply_type);
		reply_name = MACH_PORT_NULL;
	} else
		reply_name = (mach_port_t) reply;

	kmsg->ikm_header.msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				      MACH_MSGH_BITS(reply_type, dest_type));
	kmsg->ikm_header.msgh_local_port = dest_name;
	kmsg->ikm_header.msgh_remote_port = reply_name;

	if (mbits & MACH_MSGH_BITS_COMPLEX) {
	    if (mbits & MACH_MSGH_BITS_OLD_FORMAT) {
		vm_offset_t saddr, eaddr;

		saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
		eaddr = (vm_offset_t) &kmsg->ikm_header +
				kmsg->ikm_header.msgh_size;

		ipc_kmsg_clean_body_compat(saddr, eaddr);
	    }
	    else {
		mach_msg_body_t *body;

		body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
		ipc_kmsg_clean_body(kmsg, body->msgh_descriptor_count);
	    }
	}
}

/*
 *	Routine:	ipc_kmsg_check_scatter
 *	Purpose:
 *		Checks scatter and gather lists for consistency.
 *		
 *	Algorithm:
 *		The gather is assumed valid since it has been copied in.
 *		The scatter list has only been range checked.
 *		Gather list descriptors are sequentially paired with scatter 
 *		list descriptors, with port descriptors in either list ignored.
 *		Descriptors are consistent if the type fileds match and size
 *		of the scatter descriptor is less than or equal to the
 *		size of the gather descriptor.  A MACH_MSG_ALLOCATE copy 
 *		strategy in a scatter descriptor matches any size in the 
 *		corresponding gather descriptor assuming they are the same type.
 *		Either list may be larger than the other.  During the
 *		subsequent copy out, excess scatter descriptors are ignored
 *		and excess gather descriptors default to dynamic allocation.
 *		
 *		In the case of a size error, a new scatter list is formed
 *		from the gather list copying only the size and type fields.
 *		
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS		Lists are consistent
 *		MACH_RCV_INVALID_TYPE		Scatter type does not match
 *						gather type
 *		MACH_RCV_SCATTER_SMALL		Scatter size less than gather
 *						size
 */

mach_msg_return_t
ipc_kmsg_check_scatter(
	ipc_kmsg_t		kmsg,
	mach_msg_option_t	option,
	ipc_kmsg_t		*list)
{
    	mach_msg_body_t		*gbody, *sbody;
	mach_msg_descriptor_t 	*gstart, *gend;
	mach_msg_descriptor_t 	*sstart, *send;
	boolean_t 		size_error = FALSE;

	assert(*list != IKM_NULL);

    	gbody = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
    	gstart = (mach_msg_descriptor_t *) (gbody + 1);
    	gend = gstart + gbody->msgh_descriptor_count;

	sbody = (mach_msg_body_t *) (&(*list)->ikm_header + 1);
	sstart = (mach_msg_descriptor_t *) (sbody + 1);
	send = sstart + sbody->msgh_descriptor_count;

	while (gstart < gend) {

	    /*
	     * Skip port descriptors in gather list. 
	     */
	    if (gstart->type.type != MACH_MSG_PORT_DESCRIPTOR) {

		/*
	 	 * A scatter list with a 0 descriptor count is treated as an
	 	 * automatic size mismatch.
	 	 */
		 if (sbody->msgh_descriptor_count == 0) {
		    if ((option & MACH_RCV_LARGE) == 0)
			return MACH_RCV_SCATTER_SMALL;
	    	    size_error = TRUE;
		    break;
		 }

		/*
		 * Skip port descriptors in  scatter list.
		 */
		while (sstart < send) {
		    if (sstart->type.type != MACH_MSG_PORT_DESCRIPTOR)
			break;
		    sstart++;
		}

		/*
		 * No more scatter descriptors, we're done
		 */
		if (sstart >= send)
		    break;

		/*
		 * Check type, copy and size fields
		 */
		if (gstart->type.type == MACH_MSG_OOL_DESCRIPTOR) {
		    if (sstart->type.type != MACH_MSG_OOL_DESCRIPTOR)
			return MACH_RCV_INVALID_TYPE;

		    if (sstart->out_of_line.copy == MACH_MSG_OVERWRITE && 
			gstart->out_of_line.size > sstart->out_of_line.size) {
			if ((option & MACH_RCV_LARGE) == 0)
				return MACH_RCV_SCATTER_SMALL;
			size_error = TRUE;
		    }
		}
		else {
		    if (sstart->type.type != MACH_MSG_OOL_PORTS_DESCRIPTOR)
			return MACH_RCV_INVALID_TYPE;

		    if (sstart->ool_ports.copy == MACH_MSG_OVERWRITE &&
			gstart->ool_ports.count > sstart->ool_ports.count) {
			if ((option & MACH_RCV_LARGE) == 0)
				return MACH_RCV_SCATTER_SMALL;
			size_error = TRUE;
		    }
		}
	        sstart++;
	    }
	    gstart++;
	}

	/*
   	 * If there is a size error, form a new scatter list from the
	 * gather list.
	 */
	if (size_error) {
	    ipc_kmsg_t		new_list;
	    mach_msg_size_t	new_size = (unsigned)gend - (unsigned)gbody;

	    new_list = ikm_alloc(new_size);
	    if (new_list == IKM_NULL)
	    	return MACH_MSG_VM_KERNEL|MACH_RCV_BODY_ERROR;

	    new_list->ikm_header = (*list)->ikm_header;
	    new_list->ikm_header.msgh_size = new_size;

	    ikm_free(*list); (*list) = new_list;

	    /*
	     * Now fill in the  descriptor count and the type and size 
	     * fields for each descriptor
	     */
	    sbody = (mach_msg_body_t *)(&new_list->ikm_header + 1);
	    sbody->msgh_descriptor_count = gbody->msgh_descriptor_count;
	    sstart = (mach_msg_descriptor_t *) (sbody + 1);

    	    gstart = (mach_msg_descriptor_t *) (gbody + 1);
    	    gend = gstart + gbody->msgh_descriptor_count;

	    for (; gstart < gend; gstart++, sstart++) {
		sstart->type.type = gstart->type.type;
		switch (gstart->type.type) {
		    case MACH_MSG_PORT_DESCRIPTOR:
			break;
		    case MACH_MSG_OOL_DESCRIPTOR:
			sstart->out_of_line.size = gstart->out_of_line.size;
			break;
		    case MACH_MSG_OOL_PORTS_DESCRIPTOR:
			sstart->ool_ports.count = gstart->ool_ports.count;
			break;
		}
	    }

	    return MACH_RCV_SCATTER_SMALL;
	}

	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	ipc_kmsg_copyout_to_kernel
 *	Purpose:
 *		Copies out the destination and reply ports in the message.
 *		Leaves all other rights and memory in the message alone.
 *	Conditions:
 *		Nothing locked.
 *
 *	Derived from ipc_kmsg_copyout_dest.
 *	Use by mach_msg_rpc_from_kernel (which used to use copyout_dest).
 *	We really do want to save rights and memory.
 */

void
ipc_kmsg_copyout_to_kernel(
	ipc_kmsg_t	kmsg,
	ipc_space_t	space)
{
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	ipc_object_t dest = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	ipc_object_t reply = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	mach_port_t dest_name, reply_name;

	assert(IO_VALID(dest));

	io_lock(dest);
	if (io_active(dest)) {
		ipc_object_copyout_dest(space, dest, dest_type, &dest_name);
		/* dest is unlocked */
	} else {
		io_release(dest);
		io_check_unlock(dest);
		dest_name = MACH_PORT_DEAD;
	}

	reply_name = (mach_port_t) reply;

	kmsg->ikm_header.msgh_bits = (MACH_MSGH_BITS_OTHER(mbits) |
				      MACH_MSGH_BITS(reply_type, dest_type));
	kmsg->ikm_header.msgh_local_port = dest_name;
	kmsg->ikm_header.msgh_remote_port = reply_name;
}

#if	MACH_IPC_COMPAT

/*
 *	Routine:	ipc_kmsg_copyin_compat
 *	Purpose:
 *		"Copy-in" port rights and out-of-line memory
 *		in the message.
 *
 *		In all failure cases, the message is left holding
 *		no rights or memory.  However, the message buffer
 *		is not deallocated.  If successful, the message
 *		contains a valid destination port.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Successful copyin.
 *		MACH_SEND_INVALID_DEST	Can't copyin destination port.
 *		MACH_SEND_INVALID_REPLY	Can't copyin reply port.
 *		MACH_SEND_INVALID_MEMORY	Can't grab out-of-line memory.
 *		MACH_SEND_INVALID_RIGHT	Can't copyin port right in body.
 *		MACH_SEND_INVALID_TYPE	Bad type specification.
 *		MACH_SEND_MSG_TOO_SMALL	Body is too small for types/data.
 */

mach_msg_return_t
ipc_kmsg_copyin_compat(kmsg, space, map)
	ipc_kmsg_t kmsg;
	ipc_space_t space;
	vm_map_t map;
{
	msg_header_t msg;
	mach_port_t dest_name;
	mach_port_t reply_name;
	ipc_object_t dest, reply;
	mach_msg_type_name_t dest_type, reply_type;
	vm_offset_t saddr, eaddr;
	boolean_t complex;
	kern_return_t kr;
	boolean_t use_page_lists, steal_pages;

	msg = * (msg_header_t *) &kmsg->ikm_header;
	dest_name = (mach_port_t) msg.msg_remote_port;
	reply_name = (mach_port_t) msg.msg_local_port;

	/* translate the destination and reply ports */

	kr = ipc_object_copyin_header(space, dest_name, &dest, &dest_type);
	if (kr != KERN_SUCCESS)
		return MACH_SEND_INVALID_DEST;

	if (reply_name == MACH_PORT_NULL) {
		reply = IO_NULL;
		reply_type = 0;
	} else {
		kr = ipc_object_copyin_header(space, reply_name,
					      &reply, &reply_type);
		if (kr != KERN_SUCCESS) {
			ipc_object_destroy(dest, dest_type);
			return MACH_SEND_INVALID_REPLY;
		}
	}

	kmsg->ikm_header.msgh_bits = MACH_MSGH_BITS(dest_type, reply_type) |
			MACH_MSGH_BITS_OLD_FORMAT;
	kmsg->ikm_header.msgh_size = (mach_msg_size_t) msg.msg_size;
	kmsg->ikm_header.msgh_remote_port = (mach_port_t) dest;
	kmsg->ikm_header.msgh_local_port = (mach_port_t) reply;
	kmsg->ikm_header.msgh_reserved = (mach_port_seqno_t) msg.msg_type;
	kmsg->ikm_header.msgh_id = (mach_msg_id_t) msg.msg_id;

	if (msg.msg_simple)
		return MACH_MSG_SUCCESS;

	complex = FALSE;
	use_page_lists = ipc_kobject_vm_page_list(ip_kotype((ipc_port_t)dest));
	steal_pages = ipc_kobject_vm_page_steal(ip_kotype((ipc_port_t)dest));

	saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
	eaddr = (vm_offset_t) &kmsg->ikm_header + kmsg->ikm_header.msgh_size;

	while (saddr < eaddr) {
		vm_offset_t taddr = saddr;
		mach_msg_type_long_t *type;
		mach_msg_type_name_t name;
		mach_msg_type_size_t size;
		mach_msg_type_number_t number;
		boolean_t is_inline, longform, dealloc, is_port;
		vm_offset_t data;
		vm_size_t length;

		type = (mach_msg_type_long_t *) saddr;

		if (((eaddr - saddr) < sizeof(mach_msg_type_t)) ||
		    ((longform = ((mach_msg_type_t*)type)->msgt_longform) &&
		     ((eaddr - saddr) < sizeof(mach_msg_type_long_t)))) {
			ipc_kmsg_clean_partial_compat(kmsg, taddr, FALSE, 0);
			return MACH_SEND_MSG_TOO_SMALL;
		}

		is_inline = ((mach_msg_type_t*)type)->msgt_inline;
		dealloc = ((mach_msg_type_t*)type)->msgt_deallocate;
		if (longform) {
			/* This must be aligned */
			if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
			    (is_misaligned(type))) {
				saddr = ptr_align(saddr);
				continue;
			}
			name = type->msgtl_name;
			size = type->msgtl_size;
			number = type->msgtl_number;
			saddr += sizeof(mach_msg_type_long_t);
		} else {
			name = ((mach_msg_type_t*)type)->msgt_name;
			size = ((mach_msg_type_t*)type)->msgt_size;
			number = ((mach_msg_type_t*)type)->msgt_number;
			saddr += sizeof(mach_msg_type_t);
		}

		is_port = MSG_TYPE_PORT_ANY(name);

		if (is_port && (size != PORT_T_SIZE_IN_BITS)) {
			ipc_kmsg_clean_partial_compat(kmsg, taddr, FALSE, 0);
			return MACH_SEND_INVALID_TYPE;
		}

		/*
		 *	New IPC says these should be zero, but old IPC
		 *	tasks often leave them with random values.  So
		 *	we have to clear them.
		 */

		((mach_msg_type_t*)type)->msgt_unused = 0;
		if (longform) {
			type->msgtl_header.msgt_name = 0;
			type->msgtl_header.msgt_size = 0;
			type->msgtl_header.msgt_number = 0;
		}

		/* padding (ptrs and ports) ? */
		if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
		    ((size >> 3) == sizeof(natural_t)))
			saddr = ptr_align(saddr);

		/* calculate length of data in bytes, rounding up */

		length = ((number * size) + 7) >> 3;

		if (is_inline) {
			vm_size_t amount;

			/* inline data sizes round up to int boundaries */

			amount = (length + 3) &~ 3;
			if ((eaddr - saddr) < amount) {
				ipc_kmsg_clean_partial_compat(
							kmsg, taddr, FALSE, 0);
				return MACH_SEND_MSG_TOO_SMALL;
			}

			data = saddr;
			saddr += amount;
		} else {
			vm_offset_t addr;

			if ((eaddr - saddr) < sizeof(vm_offset_t)) {
				ipc_kmsg_clean_partial_compat(
							kmsg, taddr, FALSE, 0);
				return MACH_SEND_MSG_TOO_SMALL;
			}

			/* grab the out-of-line data */

			addr = * (vm_offset_t *) saddr;

			if (length == 0)
				data = 0;
			else if (is_port) {
				data = kalloc(length);
				if (data == 0)
					goto invalid_memory;

				if (copyinmap(map, (char *) addr,
					      (char *) data, length) ||
				    (dealloc &&
				     (vm_deallocate(map, addr, length) !=
							KERN_SUCCESS))) {
					kfree(data, length);
					goto invalid_memory;
				}
			} else {
#if	MACH_OLD_VM_COPY
				vm_offset_t copy;
				
				kr = vm_move(map, addr,
						ipc_soft_map, length,
						dealloc, &copy);
#else
				vm_map_copy_t copy;

		      		if (use_page_lists) {
					kr = vm_map_copyin_page_list(map,
				        	addr, length, dealloc,
						steal_pages, &copy, FALSE);
				} else {
					kr = vm_map_copyin(map, addr, length,
							   dealloc,
							   &copy);
				}
#endif
				if (kr != KERN_SUCCESS) {
				    invalid_memory:
					ipc_kmsg_clean_partial_compat(
							kmsg, taddr, FALSE, 0);
					return MACH_SEND_INVALID_MEMORY;
				}

				data = (vm_offset_t) copy;
			}

			* (vm_offset_t *) saddr = data;
			saddr += sizeof(vm_offset_t);
			complex = TRUE;
		}

		if (is_port) {
			mach_msg_type_name_t newname =
					ipc_object_copyin_type(name);
			ipc_object_t *objects = (ipc_object_t *) data;
			mach_msg_type_number_t i;

			if (longform)
				type->msgtl_name = newname;
			else
				((mach_msg_type_t*)type)->msgt_name = newname;

			for (i = 0; i < number; i++) {
				mach_port_t port = (mach_port_t) objects[i];
				ipc_object_t object;

				if (!MACH_PORT_VALID(port))
					continue;

				kr = ipc_object_copyin_compat(space, port,
						name, dealloc, &object);
				if (kr != KERN_SUCCESS) {
					ipc_kmsg_clean_partial_compat(
							kmsg, taddr, TRUE, i);
					return MACH_SEND_INVALID_RIGHT;
				}

				if ((newname == MACH_MSG_TYPE_PORT_RECEIVE) &&
				    ipc_port_check_circularity(
							(ipc_port_t) object,
							(ipc_port_t) dest))
					kmsg->ikm_header.msgh_bits |=
						MACH_MSGH_BITS_CIRCULAR;

				objects[i] = object;
			}

			complex = TRUE;
		}
	}

	if (complex)
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_COMPLEX;

	return MACH_MSG_SUCCESS;
}

void
ipc_kmsg_copyin_compat_from_kernel(kmsg)
	ipc_kmsg_t kmsg;
{
	msg_header_t msg;
	ipc_object_t remote, local;
	mach_msg_type_name_t rname, lname;
	vm_offset_t saddr, eaddr;
	boolean_t complex;

	msg = * (msg_header_t *) &kmsg->ikm_header;
	remote = (ipc_object_t) msg.msg_remote_port;
	rname = MACH_MSG_TYPE_COPY_SEND;
	local = (ipc_object_t) msg.msg_local_port;
	lname = MACH_MSG_TYPE_MAKE_SEND;

	/* translate the destination and reply ports */

	ipc_object_copyin_from_kernel(remote, rname);
	if (IO_VALID(local))
		ipc_object_copyin_from_kernel(local, lname);

	kmsg->ikm_header.msgh_bits =
			MACH_MSGH_BITS(
				ipc_object_copyin_type(rname),
				ipc_object_copyin_type(lname)) |
			MACH_MSGH_BITS_OLD_FORMAT;
	kmsg->ikm_header.msgh_size = (mach_msg_size_t) msg.msg_size;
	kmsg->ikm_header.msgh_remote_port = (mach_port_t) remote;
	kmsg->ikm_header.msgh_local_port = (mach_port_t) local;
	kmsg->ikm_header.msgh_reserved = (mach_port_seqno_t) msg.msg_type;
	kmsg->ikm_header.msgh_id = (mach_msg_id_t) msg.msg_id;

	if (msg.msg_simple)
		return;

	complex = FALSE;

	saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
	eaddr = (vm_offset_t) &kmsg->ikm_header + kmsg->ikm_header.msgh_size;

	while (saddr < eaddr) {
		mach_msg_type_long_t *type;
		mach_msg_type_name_t name;
		mach_msg_type_size_t size;
		mach_msg_type_number_t number;
		boolean_t is_inline, longform, is_port;
		vm_offset_t data;
		vm_size_t length;

		type = (mach_msg_type_long_t *) saddr;
		is_inline = ((mach_msg_type_t*)type)->msgt_inline;
		longform = ((mach_msg_type_t*)type)->msgt_longform;
		/* type->msgtl_header.msgt_deallocate not used */
		if (longform) {
			/* This must be aligned */
			if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
			    (is_misaligned(type))) {
				saddr = ptr_align(saddr);
				continue;
			}
			name = type->msgtl_name;
			size = type->msgtl_size;
			number = type->msgtl_number;
			saddr += sizeof(mach_msg_type_long_t);
		} else {
			name = ((mach_msg_type_t*)type)->msgt_name;
			size = ((mach_msg_type_t*)type)->msgt_size;
			number = ((mach_msg_type_t*)type)->msgt_number;
			saddr += sizeof(mach_msg_type_t);
		}

		is_port = MSG_TYPE_PORT_ANY(name);

		/*
		 *	New IPC says these should be zero, but old IPC
		 *	tasks often leave them with random values.  So
		 *	we have to clear them.
		 */

		((mach_msg_type_t*)type)->msgt_unused = 0;
		if (longform) {
			type->msgtl_header.msgt_name = 0;
			type->msgtl_header.msgt_size = 0;
			type->msgtl_header.msgt_number = 0;
		}

		/* padding (ptrs and ports) ? */
		if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
		    ((size >> 3) == sizeof(natural_t)))
			saddr = ptr_align(saddr);

		/* calculate length of data in bytes, rounding up */

		length = ((number * size) + 7) >> 3;

		if (is_inline) {
			/* inline data sizes round up to int boundaries */

			data = saddr;
			saddr += (length + 3) &~ 3;
		} else {
			/*
			 *	The sender should supply ready-made memory
			 *	for us, so we don't need to do anything.
			 */

			data = * (vm_offset_t *) saddr;
			saddr += sizeof(vm_offset_t);
			complex = TRUE;
		}

		if (is_port) {
			mach_msg_type_name_t newname =
					ipc_object_copyin_type(name);
			ipc_object_t *objects = (ipc_object_t *) data;
			mach_msg_type_number_t i;

			if (longform)
				type->msgtl_name = newname;
			else
				((mach_msg_type_t*)type)->msgt_name = newname;

			for (i = 0; i < number; i++) {
				ipc_object_t object = objects[i];

				if (!IO_VALID(object))
					continue;

				ipc_object_copyin_from_kernel(object, name);

				if ((newname == MACH_MSG_TYPE_PORT_RECEIVE) &&
				    ipc_port_check_circularity(
							(ipc_port_t) object,
							(ipc_port_t) remote))
					kmsg->ikm_header.msgh_bits |=
						MACH_MSGH_BITS_CIRCULAR;
			}

			complex = TRUE;
		}
	}

	if (complex)
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_COMPLEX;
}

/*
 *	Routine:	ipc_kmsg_copyout_compat
 *	Purpose:
 *		"Copy-out" port rights and out-of-line memory
 *		in the message, producing an old IPC message.
 *
 *		Doesn't bother to handle the header atomically.
 *		Skips over errors.  Problem ports produce MACH_PORT_NULL
 *		(MACH_PORT_DEAD is never produced), and problem memory
 *		produces a zero address.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Copied out rights and memory.
 */

mach_msg_return_t
ipc_kmsg_copyout_compat(kmsg, space, map)
	ipc_kmsg_t kmsg;
	ipc_space_t space;
	vm_map_t map;
{
	msg_header_t msg;
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	ipc_object_t dest = (ipc_object_t) kmsg->ikm_header.msgh_remote_port;
	ipc_object_t reply = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	mach_port_t dest_name, reply_name;
	vm_offset_t saddr, eaddr;
	kern_return_t kr;

	assert(IO_VALID(dest));

	io_lock(dest);
	if (io_active(dest)) {
		mach_msg_type_name_t dest_type = MACH_MSGH_BITS_REMOTE(mbits);

		ipc_object_copyout_dest(space, dest, dest_type, &dest_name);
		/* dest is unlocked */
	} else {
		io_release(dest);
		io_check_unlock(dest);
		dest_name = MACH_PORT_NULL;
	}

	if (IO_VALID(reply)) {
		mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);

		kr = ipc_object_copyout_compat(space, reply, reply_type,
					       &reply_name);
		if (kr != KERN_SUCCESS) {
			ipc_object_destroy(reply, reply_type);
			reply_name = MACH_PORT_NULL;
		}
	} else
		reply_name = MACH_PORT_NULL;

	msg.msg_unused = 0;
	msg.msg_simple = (mbits & MACH_MSGH_BITS_COMPLEX) ? FALSE : TRUE;
	msg.msg_size = (msg_size_t) kmsg->ikm_header.msgh_size;
	msg.msg_type = (integer_t) kmsg->ikm_header.msgh_reserved;
	msg.msg_local_port = (port_name_t) dest_name;
	msg.msg_remote_port = (port_name_t) reply_name;
	msg.msg_id = (integer_t) kmsg->ikm_header.msgh_id;
	* (msg_header_t *) &kmsg->ikm_header = msg;

	if (msg.msg_simple)
		return MACH_MSG_SUCCESS;

	saddr = (vm_offset_t) (&kmsg->ikm_header + 1);
	eaddr = (vm_offset_t) &kmsg->ikm_header + kmsg->ikm_header.msgh_size;

	while (saddr < eaddr) {
		vm_offset_t taddr = saddr;
		mach_msg_type_long_t *type;
		mach_msg_type_name_t name;
		mach_msg_type_size_t size;
		mach_msg_type_number_t number;
		boolean_t is_inline, longform, is_port;
		vm_size_t length;
		vm_offset_t addr;

		type = (mach_msg_type_long_t *) saddr;
		is_inline = ((mach_msg_type_t*)type)->msgt_inline;
		longform = ((mach_msg_type_t*)type)->msgt_longform;
		if (longform) {
			/* This must be aligned */
			if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
			    (is_misaligned(type))) {
				saddr = ptr_align(saddr);
				continue;
			}
			name = type->msgtl_name;
			size = type->msgtl_size;
			number = type->msgtl_number;
			saddr += sizeof(mach_msg_type_long_t);
		} else {
			name = ((mach_msg_type_t*)type)->msgt_name;
			size = ((mach_msg_type_t*)type)->msgt_size;
			number = ((mach_msg_type_t*)type)->msgt_number;
			saddr += sizeof(mach_msg_type_t);
		}

		/* padding (ptrs and ports) ? */
		if ((sizeof(natural_t) > sizeof(mach_msg_type_t)) &&
		    ((size >> 3) == sizeof(natural_t)))
			saddr = ptr_align(saddr);

		/* calculate length of data in bytes, rounding up */

		length = ((number * size) + 7) >> 3;

		is_port = MACH_MSG_TYPE_PORT_ANY(name);

		if (is_port) {
			mach_port_t *objects;
			mach_msg_type_number_t i;
			mach_msg_type_name_t newname;

			if (!is_inline && (length != 0)) {
				/* first allocate memory in the map */

				kr = vm_allocate(map, &addr, length, TRUE);
				if (kr != KERN_SUCCESS) {
					ipc_kmsg_clean_body_compat(
								taddr, saddr);
					goto vm_copyout_failure;
				}
			}

			newname = ipc_object_copyout_type_compat(name);
			if (longform)
				type->msgtl_name = newname;
			else
				((mach_msg_type_t*)type)->msgt_name = newname;

			objects = (mach_port_t *)
				(is_inline ? saddr : * (vm_offset_t *) saddr);

			/* copyout port rights carried in the message */

			for (i = 0; i < number; i++) {
				ipc_object_t object =
					(ipc_object_t) objects[i];

				if (!IO_VALID(object)) {
					objects[i] = MACH_PORT_NULL;
					continue;
				}

				kr = ipc_object_copyout_compat(space, object,
							name, &objects[i]);
				if (kr != KERN_SUCCESS) {
					ipc_object_destroy(object, name);
					objects[i] = MACH_PORT_NULL;
				}
			}
		}

		if (is_inline) {
			/* inline data sizes round up to int boundaries */

			saddr += (length + 3) &~ 3;
		} else {
			vm_offset_t data = * (vm_offset_t *) saddr;

			/* copyout memory carried in the message */

			if (length == 0) {
				assert(data == 0);
				addr = 0;
			} else if (is_port) {
				/* copyout to memory allocated above */

				(void) copyoutmap(map, (char *) data,
						  (char *) addr, length);
				kfree(data, length);
			} else {
#if	MACH_OLD_VM_COPY
				kr = vm_move(
					ipc_soft_map, data,
					map, length,
					FALSE, &addr);
				(void) vm_deallocate(
						ipc_soft_map, data, length);
				if (kr != KERN_SUCCESS) {
#else
				vm_map_copy_t copy = (vm_map_copy_t) data;

				kr = vm_map_copyout(map, &addr, copy);
				if (kr != KERN_SUCCESS) {
					vm_map_copy_discard(copy);
#endif

				    vm_copyout_failure:

					addr = 0;
				}
			}

			* (vm_offset_t *) saddr = addr;
			saddr += sizeof(vm_offset_t);
		}
	}

	return MACH_MSG_SUCCESS;
}

#endif	MACH_IPC_COMPAT
