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
 *	File:	ipc/mach_msg.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Exported message traps.  See mach/message.h.
 */

#import <mach/features.h>

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>
#include <kern/assert.h>
#include <kern/counters.h>
#include <kern/lock.h>
#include <kern/sched_prim.h>
#include <kern/ipc_sched.h>
#include <vm/vm_map.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_marequest.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_thread.h>
#include <ipc/ipc_entry.h>
#include <ipc/mach_msg.h>

/*
 * Forward declarations
 */

mach_msg_return_t mach_msg_send(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		send_size,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify);

mach_msg_return_t mach_msg_receive(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		rcv_size,
	mach_port_t		rcv_name,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify,
	mach_msg_size_t		list_size);

mach_msg_return_t mach_msg_receive_error(
	ipc_kmsg_t		kmsg,
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_port_seqno_t	seqno,
	ipc_space_t		space);

/* the size of each trailer has to be listed here for copyout purposes */
vm_size_t trailer_size[] = {
          sizeof (mach_msg_trailer_t), 
	  sizeof (mach_msg_seqno_trailer_t),
	  sizeof (mach_msg_security_trailer_t)
};

/*
 *	Routine:	mach_msg_send
 *	Purpose:
 *		Send a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Sent the message.
 *		MACH_SEND_MSG_TOO_SMALL	Message smaller than a header.
 *		MACH_SEND_NO_BUFFER	Couldn't allocate buffer.
 *		MACH_SEND_INVALID_DATA	Couldn't copy message data.
 *		MACH_SEND_INVALID_HEADER
 *			Illegal value in the message header bits.
 *		MACH_SEND_INVALID_DEST	The space is dead.
 *		MACH_SEND_INVALID_NOTIFY	Bad notify port.
 *		MACH_SEND_INVALID_DEST	Can't copyin destination port.
 *		MACH_SEND_INVALID_REPLY	Can't copyin reply port.
 *		MACH_SEND_TIMED_OUT	Timeout expired without delivery.
 *		MACH_SEND_INTERRUPTED	Delivery interrupted.
 *		MACH_SEND_NO_NOTIFY	Can't allocate a msg-accepted request.
 *		MACH_SEND_WILL_NOTIFY	Msg-accepted notif. requested.
 *		MACH_SEND_NOTIFY_IN_PROGRESS
 *			This space has already forced a message to this port.
 */

mach_msg_return_t
mach_msg_send(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		send_size,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify)
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;

	mr = ipc_kmsg_get(msg, option, send_size, MAX_TRAILER_SIZE, &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return mr;

	if (option & MACH_SEND_CANCEL) {
		if (notify == MACH_PORT_NULL)
			mr = MACH_SEND_INVALID_NOTIFY;
		else
			mr = ipc_kmsg_copyin(kmsg, space, map, notify);
	} else
		mr = ipc_kmsg_copyin(kmsg, space, map, MACH_PORT_NULL);
	if (mr != MACH_MSG_SUCCESS) {
		ikm_free(kmsg);
		return mr;
	}

	mr = ipc_mqueue_send(kmsg, option & MACH_SEND_TIMEOUT,
						timeout, IMQ_NULL_CONTINUE);
	if (mr != MACH_MSG_SUCCESS) {
		mr |= ipc_kmsg_copyout_pseudo(kmsg, space, map, IKM_NULL);

		assert(kmsg->ikm_marequest == IMAR_NULL);
		(void) ipc_kmsg_put(msg, kmsg, kmsg->ikm_header.msgh_size);
	}

	return mr;
}

#define FREE_SCATTER_LIST(l)	 			\
MACRO_BEGIN						\
	if((l) != IKM_NULL) {		 		\
		ikm_free((l));				\
	}						\
MACRO_END

/*
 *	Routine:	mach_msg_receive
 *	Purpose:
 *		Receive a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Received a message.
 *		MACH_RCV_INVALID_NAME	The name doesn't denote a right,
 *			or the denoted right is not receive or port set.
 *		MACH_RCV_IN_SET		Receive right is a member of a set.
 *		MACH_RCV_TOO_LARGE	Message wouldn't fit into buffer.
 *		MACH_RCV_TIMED_OUT	Timeout expired without a message.
 *		MACH_RCV_INTERRUPTED	Reception interrupted.
 *		MACH_RCV_PORT_DIED	Port/set died while receiving.
 *		MACH_RCV_PORT_CHANGED	Port moved into set while receiving.
 *		MACH_RCV_INVALID_DATA	Couldn't copy to user buffer.
 *		MACH_RCV_INVALID_NOTIFY	Bad notify port.
 *		MACH_RCV_HEADER_ERROR
 */

mach_msg_return_t
mach_msg_receive(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		rcv_size,
	mach_port_t		rcv_name,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify,
	mach_msg_size_t		list_size)
{
	ipc_thread_t self = current_thread();
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_object_t object;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	ipc_kmsg_t list;
	mach_msg_return_t mr;
	mach_msg_trailer_t *trailer;

	/*
 	 * If MACH_RCV_OVERWRITE was specified, both receive_msg (msg)
	 * and receive_msg_size (list_size) need to be non NULL.
	 */
	if (option & MACH_RCV_OVERWRITE) {
		if (list_size < sizeof(mach_msg_base_t)) 
			return MACH_RCV_SCATTER_SMALL;
		else 
		if (ipc_kmsg_get(msg, MACH_MSG_OPTION_NONE,
				list_size, 0, &list) != MACH_MSG_SUCCESS)
			return MACH_RCV_INVALID_DATA;

		else
		if ((((mach_msg_base_t *)
			&kmsg->ikm_header)->body.msgh_descriptor_count *
					sizeof (mach_msg_descriptor_t)) + 
					sizeof (mach_msg_base_t) > list_size) {
			FREE_SCATTER_LIST(list);
			return MACH_RCV_INVALID_TYPE;
		}
	}
	else
		list = IKM_NULL;

	mr = ipc_mqueue_copyin(space, rcv_name, &mqueue, &object);
	if (mr != MACH_MSG_SUCCESS) {
		FREE_SCATTER_LIST(list);
		return mr;
	}
	/* hold ref for object; mqueue is locked */

	/*
	 *	ipc_mqueue_receive may not return, because if we block
	 *	then our kernel stack may be discarded.  So we save
	 *	state here for mach_msg_receive_continue to pick up.
	 */

	self->ith_msg = msg;
	self->ith_option = option;
	self->ith_rcv_size = rcv_size;
	self->ith_timeout = timeout;
	self->ith_notify = notify;
	self->ith_object = object;
	self->ith_mqueue = mqueue;

	mr = ipc_mqueue_receive(mqueue,
				option & (MACH_RCV_TRAILER_MASK|
						MACH_RCV_TIMEOUT|
						MACH_RCV_LARGE),
				rcv_size, timeout,
				FALSE, mach_msg_receive_continue,
				&kmsg, &seqno, &list);
	/* mqueue is unlocked */
	ipc_object_release(object);

	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_HEADER_ERROR) {
			mach_msg_return_t	pr;

			pr = mach_msg_receive_error(kmsg, msg,
						    option, seqno, space);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
		}
		else
		if (mr == MACH_RCV_TOO_LARGE || mr == MACH_RCV_SCATTER_SMALL) {
			if (option & MACH_RCV_LARGE) {
				if (list != IKM_NULL) {
					mach_msg_return_t	pr;

					list_size = list->ikm_header.msgh_size;
					list->ikm_header.msgh_size =
						(mach_msg_size_t)kmsg;
					pr = ipc_kmsg_put(
						msg, list, list_size);
					if (pr != MACH_MSG_SUCCESS)
						mr = pr;
				}
				else					
				if (copyout((char *) &kmsg,
					       (char *) &msg->msgh_size,
					       sizeof(mach_msg_size_t)))
					mr = MACH_RCV_INVALID_DATA;

				return mr;
			}
			else {
				mach_msg_return_t	pr;

				pr = mach_msg_receive_error(kmsg, msg,
							option, seqno, space);
				if (pr != MACH_MSG_SUCCESS)
					mr = pr;
			}
		}

		FREE_SCATTER_LIST(list);
		return mr;
	}

	/*
	 * Generate requested trailer.
	 */
	trailer = (mach_msg_trailer_t *)
			((vm_offset_t)&kmsg->ikm_header +
					kmsg->ikm_header.msgh_size);

	if (option & MACH_RCV_NO_TRAILER) {
		trailer->msgh_trailer_size = 
				trailer->msgh_trailer_type = 0;
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_NO_TRAILER;
	}
	else
	if (kmsg->ikm_delta > 0) {
		/* MACH_SEND_TRAILER */;
	}
	else {
		int	elements = GET_RCV_ELEMENTS(option);
#define trailer0	((mach_msg_format_0_trailer_t *)trailer)
		
		if (elements == MACH_RCV_TRAILER_SENDER) {
			trailer0->msgh_sender = kmsg->ikm_sender;
			trailer0->msgh_seqno = seqno;
		}
		else
		if (elements == MACH_RCV_TRAILER_SEQNO)
			trailer0->msgh_seqno = seqno;

		trailer0->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
		trailer0->msgh_trailer_size = REQUESTED_TRAILER_SIZE(option);
#undef trailer0
	}

	if (option & MACH_RCV_NOTIFY) {
		if (notify == MACH_PORT_NULL)
			mr = MACH_RCV_INVALID_NOTIFY;
		else
			mr = ipc_kmsg_copyout(kmsg, space, map, notify, list);
	} else
		mr = ipc_kmsg_copyout(kmsg, space, map, MACH_PORT_NULL, list);
	if (mr != MACH_MSG_SUCCESS) {
		mach_msg_return_t	pr;

		if ((mr &~ MACH_MSG_MASK) == MACH_RCV_BODY_ERROR) {
			pr = ipc_kmsg_put(msg, kmsg,
					kmsg->ikm_header.msgh_size +
						trailer->msgh_trailer_size);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
					
		} else {
			pr = mach_msg_receive_error(kmsg, msg,
						option, seqno, space);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
		}

		FREE_SCATTER_LIST(list);
		return mr;
	}

	FREE_SCATTER_LIST(list);
	return ipc_kmsg_put(msg, kmsg,
			kmsg->ikm_header.msgh_size +
				trailer->msgh_trailer_size);
}

/*
 *	Routine:	mach_msg_receive_continue
 *	Purpose:
 *		Continue after blocking for a message.
 *	Conditions:
 *		Nothing locked.  We are running on a new kernel stack,
 *		with the receive state saved in the thread.  From here
 *		control goes back to user space.
 */

void
mach_msg_receive_continue(void)
{
	ipc_thread_t self = current_thread();
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	mach_msg_header_t *msg = self->ith_msg;
	mach_msg_option_t option = self->ith_option;
	mach_msg_trailer_t *trailer;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	ipc_kmsg_t list;
	mach_msg_return_t mr;

	mr = ipc_mqueue_receive(self->ith_mqueue,
				option & (MACH_RCV_TRAILER_MASK|
						MACH_RCV_TIMEOUT|
						MACH_RCV_LARGE),
				self->ith_rcv_size, self->ith_timeout,
				TRUE, mach_msg_receive_continue,
				&kmsg, &seqno, &list);
	/* mqueue is unlocked */
	ipc_object_release(self->ith_object);

	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_HEADER_ERROR) {
			mach_msg_return_t	pr;

			pr = mach_msg_receive_error(kmsg, msg,
						    option, seqno, space);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
		}
		else
		if (mr == MACH_RCV_TOO_LARGE || mr == MACH_RCV_SCATTER_SMALL) {
			if (option & MACH_RCV_LARGE) {
				if (list != IKM_NULL) {
					mach_msg_return_t	pr;
					mach_msg_size_t		list_size;

					list_size = list->ikm_header.msgh_size;
					list->ikm_header.msgh_size =
						(mach_msg_size_t)kmsg;
					pr = ipc_kmsg_put(
						msg, list, list_size);
					if (pr != MACH_MSG_SUCCESS)
						mr = pr;
				}
				else					
				if (copyout((char *) &kmsg,
					       (char *) &msg->msgh_size,
					       sizeof(mach_msg_size_t)))
					mr = MACH_RCV_INVALID_DATA;

				thread_syscall_return(mr);
				/*NOTREACHED*/
			}
			else {
				mach_msg_return_t	pr;

				pr = mach_msg_receive_error(kmsg, msg,
							option, seqno, space);
				if (pr != MACH_MSG_SUCCESS)
					mr = pr;
			}
		}

		FREE_SCATTER_LIST(list);
		thread_syscall_return(mr);
		/*NOTREACHED*/
	}

	/*
	 * Generate requested trailer.
	 */
	trailer = (mach_msg_trailer_t *)
			((vm_offset_t)&kmsg->ikm_header +
					kmsg->ikm_header.msgh_size);

	if (option & MACH_RCV_NO_TRAILER) {
		trailer->msgh_trailer_size = 
				trailer->msgh_trailer_type = 0;
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_NO_TRAILER;
	}
	else
	if (kmsg->ikm_delta > 0) {
		/* MACH_SEND_TRAILER */;
	}
	else {
		int	elements = GET_RCV_ELEMENTS(option);
#define trailer0	((mach_msg_format_0_trailer_t *)trailer)
		
		if (elements == MACH_RCV_TRAILER_SENDER) {
			trailer0->msgh_sender = kmsg->ikm_sender;
			trailer0->msgh_seqno = seqno;
		}
		else
		if (elements == MACH_RCV_TRAILER_SEQNO)
			trailer0->msgh_seqno = seqno;

		trailer0->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
		trailer0->msgh_trailer_size = REQUESTED_TRAILER_SIZE(option);
#undef trailer0
	}

	if (option & MACH_RCV_NOTIFY) {
		if (self->ith_notify == MACH_PORT_NULL)
			mr = MACH_RCV_INVALID_NOTIFY;
		else
			mr = ipc_kmsg_copyout(
					kmsg, space, map,
					self->ith_notify, 
					self->ith_list);
	} else
		mr = ipc_kmsg_copyout(
				kmsg, space, map,
				MACH_PORT_NULL,
				self->ith_list);
	if (mr != MACH_MSG_SUCCESS) {
		mach_msg_return_t	pr;

		if ((mr &~ MACH_MSG_MASK) == MACH_RCV_BODY_ERROR) {
			pr = ipc_kmsg_put(msg, kmsg,
					kmsg->ikm_header.msgh_size +
						trailer->msgh_trailer_size);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
					
		} else {
			pr = mach_msg_receive_error(kmsg, msg,
						option, seqno, space);
			if (pr != MACH_MSG_SUCCESS)
				mr = pr;
		}

		FREE_SCATTER_LIST(list);
		thread_syscall_return(mr);
		/*NOTREACHED*/
	}

	FREE_SCATTER_LIST(list);
	mr = ipc_kmsg_put(msg, kmsg,
			kmsg->ikm_header.msgh_size +
				trailer->msgh_trailer_size);
	thread_syscall_return(mr);
	/*NOTREACHED*/
}

/*
 *	Routine:	mach_msg_receive_error	[internal]
 *	Purpose:
 *		Builds a minimal header/trailer and copies it to
 *		the user message buffer.  Invoked when in the case of a
 *		MACH_RCV_TOO_LARGE or MACH_RCV_HEADER_ERROR error.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	minimal headere/trailer copied
 *		MACH_RCV_INVALID_DATA	copyout to user buffer failed
 */
	
mach_msg_return_t
mach_msg_receive_error(
	ipc_kmsg_t		kmsg,
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_port_seqno_t	seqno,
	ipc_space_t		space)
{
	mach_msg_trailer_t *trailer;

	/*
	 * Copy out the destination port in the message.
 	 * Destroy all other rights and memory in the message.
	 */
	ipc_kmsg_copyout_dest(kmsg, space);

	/*
	 * Build a minimal message with a null trailer.
	 */
	kmsg->ikm_header.msgh_size = sizeof(mach_msg_header_t);
	trailer = (mach_msg_trailer_t *)
				((vm_offset_t)&kmsg->ikm_header +
						kmsg->ikm_header.msgh_size);

	if (option & MACH_RCV_NO_TRAILER) {
	    trailer->msgh_trailer_size = trailer->msgh_trailer_type = 0;
	    kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_NO_TRAILER;
	}
	else {
	    trailer->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
	    trailer->msgh_trailer_size = sizeof (mach_msg_trailer_t);
	}

	/*
	 * Copy the message to user space
	 */
	return ipc_kmsg_put(msg, kmsg,
			kmsg->ikm_header.msgh_size +
				trailer->msgh_trailer_size);
}

/*
 *	Routine:	mach_msg_overwrite_trap [mach trap]
 *	Purpose:
 *		Possibly send a message; possibly receive a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		All of mach_msg_send and mach_msg_receive error codes.
 */
static __inline__
mach_msg_return_t
_mach_msg_overwrite(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		send_size,
	mach_msg_size_t		rcv_size,
	mach_port_t		rcv_name,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify,
	mach_msg_header_t	*rcv_msg,
        mach_msg_size_t		list_size)
{
	mach_msg_return_t mr;

	if (option & MACH_SEND_MSG) {
		mr = mach_msg_send(msg, option, send_size,
				   timeout, notify);
		if (mr != MACH_MSG_SUCCESS)
			return mr;
	}

	if (option & MACH_RCV_MSG) {
		mach_msg_header_t *rcv;

		/*
		 * 1. MACH_RCV_OVERWRITE is on, and rcv_msg is our scatter list
		 *    and receive buffer
		 * 2. MACH_RCV_OVERWRITE is off, and rcv_msg might be the
		 *    alternate receive buffer (separate send and receive buffers).
		 */
		if (option & MACH_RCV_OVERWRITE) 
		    rcv = rcv_msg;
		else if (rcv_msg != MACH_MSG_NULL)
		    rcv = rcv_msg;
		else
		    rcv = msg;
		mr = mach_msg_receive(rcv, option, rcv_size, rcv_name, 
				      timeout, notify, list_size);
		if (mr != MACH_MSG_SUCCESS)
			return mr;
	}

	return MACH_MSG_SUCCESS;
}

mach_msg_return_t
mach_msg_overwrite_trap(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		sndsiz,
	mach_msg_size_t		rcvsiz,
	mach_port_t		rcvnam,
	mach_msg_timeout_t	tout,
	mach_port_t		notify,
	mach_msg_header_t	*rmsg,
	mach_msg_size_t		rmsgsiz)
{
	return
	(
		_mach_msg_overwrite(
				msg, option,
				sndsiz,
				rcvsiz, rcvnam,
				tout, notify,
				rmsg, rmsgsiz));
}

mach_msg_return_t
mach_msg_trap(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		sndsiz,
	mach_msg_size_t		rcvsiz,
	mach_port_t		rcvnam,
	mach_msg_timeout_t	tout,
	mach_port_t		notify)
{
	return
	(
		_mach_msg_overwrite(
				msg, option,
				sndsiz,
				rcvsiz, rcvnam,
				tout, notify,
				MACH_MSG_NULL,  (mach_msg_size_t) 0));
}

mach_msg_return_t
mach_msg_simple_trap(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		sndsiz,
	mach_msg_size_t		rcvsiz,
	mach_port_t		rcvnam)
{
	return
	(
		_mach_msg_overwrite(
				msg, option,
				sndsiz,
				rcvsiz, rcvnam,
				MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
				MACH_MSG_NULL,  (mach_msg_size_t) 0));
}

/*
 *	Routine:	mach_msg_interrupt
 *	Purpose:
 *		Attempts to force a thread waiting at mach_msg_continue or
 *		mach_msg_receive_continue into a clean point.  Returns TRUE
 *		if this was possible.
 *	Conditions:
 *		Nothing locked.  The thread must NOT be runnable.
 */

boolean_t
mach_msg_interrupt(
	thread_t thread)
{
	ipc_mqueue_t mqueue;

	assert((thread->swap_func == (void (*)()) mach_msg_continue) ||
	       (thread->swap_func == (void (*)()) mach_msg_receive_continue));

	mqueue = thread->ith_mqueue;
	imq_lock(mqueue);
	if (thread->ith_state != MACH_RCV_IN_PROGRESS) {
		/*
		 *	The thread is no longer waiting for a message.
		 *	It may have a message sitting in ith_kmsg.
		 *	We can't clean this up.
		 */

		imq_unlock(mqueue);
		return FALSE;
	}
	ipc_thread_rmqueue(&mqueue->imq_threads, thread);
	imq_unlock(mqueue);

	ipc_object_release(thread->ith_object);

	thread_set_syscall_return(thread, MACH_RCV_INTERRUPTED);
	thread->swap_func = thread_exception_return;
	return TRUE;
}

#if	MACH_IPC_COMPAT

/*
 *	Routine:	msg_return_translate
 *	Purpose:
 *		Translate from new error code to old error code.
 */

msg_return_t
msg_return_translate(
	mach_msg_return_t mr)
{
	switch (mr &~ MACH_MSG_MASK) {
	    case MACH_MSG_SUCCESS:
		return 0;	/* SEND_SUCCESS/RCV_SUCCESS/RPC_SUCCESS */

	    case MACH_SEND_NO_BUFFER:
	    case MACH_SEND_NO_NOTIFY:
		printf("msg_return_translate: %x -> interrupted\n", mr);
		return SEND_INTERRUPTED;

	    case MACH_SEND_MSG_TOO_SMALL:
		return SEND_MSG_TOO_SMALL;
	    case MACH_SEND_INVALID_DATA:
	    case MACH_SEND_INVALID_MEMORY:
		return SEND_INVALID_MEMORY;
	    case MACH_SEND_TIMED_OUT:
		return SEND_TIMED_OUT;
	    case MACH_SEND_INTERRUPTED:
		return SEND_INTERRUPTED;
	    case MACH_SEND_INVALID_DEST:
	    case MACH_SEND_INVALID_REPLY:
	    case MACH_SEND_INVALID_RIGHT:
	    case MACH_SEND_INVALID_TYPE:
		return SEND_INVALID_PORT;
	    case MACH_SEND_WILL_NOTIFY:
		return SEND_WILL_NOTIFY;
	    case MACH_SEND_NOTIFY_IN_PROGRESS:
		return SEND_NOTIFY_IN_PROGRESS;

	    case MACH_RCV_INVALID_NAME:
	    case MACH_RCV_IN_SET:
	    case MACH_RCV_PORT_DIED:
		return RCV_INVALID_PORT;
	    case MACH_RCV_TOO_LARGE:
		return RCV_TOO_LARGE;
	    case MACH_RCV_TIMED_OUT:
		return RCV_TIMED_OUT;
	    case MACH_RCV_INTERRUPTED:
		return RCV_INTERRUPTED;
	    case MACH_RCV_PORT_CHANGED:
		return RCV_PORT_CHANGE;
	    case MACH_RCV_INVALID_DATA:
		return RCV_INVALID_MEMORY;

	    case MACH_SEND_IN_PROGRESS:
	    case MACH_SEND_INVALID_NOTIFY:
	    case MACH_SEND_INVALID_HEADER:
	    case MACH_RCV_IN_PROGRESS:
	    case MACH_RCV_INVALID_NOTIFY:
	    case MACH_RCV_HEADER_ERROR:
	    case MACH_RCV_BODY_ERROR:
	    default:
		panic("msg_return_translate");
	}
}

/*
 *	Routine:	msg_send_trap [mach trap]
 *	Purpose:
 *		Send a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 */

msg_return_t
msg_send_trap(
	msg_header_t *msg,
	msg_option_t option,
	msg_size_t send_size,
	msg_timeout_t timeout)
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_kmsg_t kmsg;
	integer_t send_delta = send_size;
	mach_msg_return_t mr;

	send_size = (send_size + 3) & ~3; /* round up */
	send_delta -= send_size;

	if (send_size > MSG_SIZE_MAX)
		return SEND_MSG_TOO_LARGE;

	mr = ipc_kmsg_get((mach_msg_header_t *) msg, MACH_MSG_OPTION_NONE,
			  (mach_msg_size_t) send_size, send_delta,
			  &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);

	mr = ipc_kmsg_copyin_compat(kmsg, space, map);
	if (mr != MACH_MSG_SUCCESS) {
		ikm_free(kmsg);
		return msg_return_translate(mr);
	}

	if (option & SEND_NOTIFY) {
		mr = ipc_mqueue_send(kmsg, 
				     ((option & SEND_SWITCH) ?
				      MACH_SEND_SWITCH :
				      MACH_MSG_OPTION_NONE) |
				      MACH_SEND_TIMEOUT,
				     ((option & SEND_TIMEOUT) ?
				      (mach_msg_timeout_t) timeout :
				      MACH_MSG_TIMEOUT_NONE),
				     IMQ_NULL_CONTINUE);
		if (mr == MACH_SEND_TIMED_OUT) {
			ipc_port_t dest = (ipc_port_t)
				kmsg->ikm_header.msgh_remote_port;

			mr = ipc_marequest_create(space, dest, MACH_PORT_NULL,
						  &kmsg->ikm_marequest);
			if (mr == MACH_MSG_SUCCESS) {
				ipc_mqueue_send_always(kmsg);
				return SEND_WILL_NOTIFY;
			}
		}
	}
	else if (option & SEND_SWITCH)
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_TIMEOUT) ?
				      MACH_SEND_TIMEOUT :
				      MACH_MSG_OPTION_NONE) |
				      MACH_SEND_SWITCH,
				     (mach_msg_timeout_t) timeout,
				     msg_send_switch_continue);
	else
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_TIMEOUT) ?
				      MACH_SEND_TIMEOUT :
				      MACH_MSG_OPTION_NONE),
				     (mach_msg_timeout_t) timeout,
				     IMQ_NULL_CONTINUE);

	if (mr != MACH_MSG_SUCCESS)
		ipc_kmsg_destroy(kmsg);

	return msg_return_translate(mr);
}

void
msg_send_switch_continue(void)
{
	thread_syscall_return(SEND_SUCCESS);
	/*NOTREACHED*/
}

mach_msg_return_t
msg_receive_error(
	ipc_kmsg_t	kmsg,
	msg_header_t	*msg,
	ipc_space_t	space);

/*
 *	Routine:	msg_receive_trap [mach trap]
 *	Purpose:
 *		Receive a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 */

msg_return_t
msg_receive_trap(
	msg_header_t *msg,
	msg_option_t option,
	msg_size_t rcv_size,
	port_name_t rcv_name,
	msg_timeout_t timeout)
{
	ipc_thread_t self;
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_object_t object;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	mr = ipc_mqueue_copyin(space, (mach_port_t) rcv_name,
			       &mqueue, &object);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);
	/* hold ref for object; mqueue is locked */

	/*
	 *	ipc_mqueue_receive may not return, because if we block
	 *	then our kernel stack may be discarded.  So we save
	 *	state here for msg_receive_continue to pick up.
	 */

	self = current_thread();
	self->ith_msg = (mach_msg_header_t *) msg;
	self->ith_option = (mach_msg_option_t) option;
	self->ith_rcv_size = (mach_msg_size_t) rcv_size;
	self->ith_timeout = (mach_msg_timeout_t) timeout;
	self->ith_object = object;
	self->ith_mqueue = mqueue;

	mr = ipc_mqueue_receive(mqueue,
				((option & RCV_TIMEOUT) ?
				MACH_RCV_TIMEOUT :
				MACH_MSG_OPTION_NONE) |
				MACH_RCV_OLD_FORMAT,
				(option & RCV_LARGE) ?
				(mach_msg_size_t) rcv_size :
				MACH_MSG_SIZE_MAX,
				(mach_msg_timeout_t) timeout,
				FALSE, msg_receive_continue,
				&kmsg, &seqno, 0);
	/* mqueue is unlocked */
	ipc_object_release(object);

	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_HEADER_ERROR)
			mr = msg_receive_error(kmsg, msg, space);
		else
		if (mr == MACH_RCV_TOO_LARGE) {
			msg_size_t real_size =
				(msg_size_t) (mach_msg_size_t) kmsg;

			assert(real_size > rcv_size);

			(void) copyout((vm_offset_t) &real_size,
				       (vm_offset_t) &msg->msg_size,
				       sizeof(msg_size_t));
		}

		return msg_return_translate(mr);
	}
	
	if (kmsg->ikm_header.msgh_size > (mach_msg_size_t) rcv_size) {
		assert(!(option & RCV_LARGE));
		
		ipc_kmsg_destroy(kmsg);
		return (RCV_TOO_LARGE);
	}

	assert(kmsg->ikm_header.msgh_size <= (mach_msg_size_t) rcv_size);

	mr = ipc_kmsg_copyout_compat(kmsg, space, map);
	assert(mr == MACH_MSG_SUCCESS);

	kmsg->ikm_header.msgh_size += kmsg->ikm_delta;
	mr = ipc_kmsg_put((mach_msg_header_t *) msg, kmsg,
			  kmsg->ikm_header.msgh_size);
	return msg_return_translate(mr);
}

/*
 *	Routine:	msg_rpc_trap [mach trap]
 *	Purpose:
 *		Send and receive a message.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 */

msg_return_t
msg_rpc_trap(
	msg_header_t *msg,
	msg_option_t option,
	msg_size_t send_size,
	msg_size_t rcv_size,
	msg_timeout_t send_timeout,
	msg_timeout_t rcv_timeout)
{
	ipc_thread_t self;
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	ipc_port_t reply;
	ipc_pset_t pset;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	integer_t send_delta = send_size;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	/*
	 *	Instead of using msg_send_trap and msg_receive_trap,
	 *	we implement msg_rpc_trap directly.  The difference
	 *	is how the reply port is handled.  Instead of using
	 *	ipc_mqueue_copyin, we save a reference for the reply
	 *	port carried in the sent message.  For example,
	 *	consider a rename kernel call which changes the name
	 *	of the call's own reply port.  This is the behaviour
	 *	of the Mach 2.5 msg_rpc_trap.
	 */

	send_size = (send_size + 3) & ~3; /* round up */
	send_delta -= send_size;

	if (send_size > MSG_SIZE_MAX)
		return SEND_MSG_TOO_LARGE;

	mr = ipc_kmsg_get((mach_msg_header_t *) msg, MACH_MSG_OPTION_NONE,
			  (mach_msg_size_t) send_size, send_delta,
			  &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);

	mr = ipc_kmsg_copyin_compat(kmsg, space, map);
	if (mr != MACH_MSG_SUCCESS) {
		ikm_free(kmsg);
		return msg_return_translate(mr);
	}

	reply = (ipc_port_t) kmsg->ikm_header.msgh_local_port;
	if (IP_VALID(reply)) {
		ipc_port_t	dest = (ipc_port_t)
					kmsg->ikm_header.msgh_remote_port;
		
		ipc_port_reference(reply);
		assert(IP_VALID(dest));
		
		ip_lock(dest);
		
		if (dest->ip_receiver == ipc_space_kernel) {
			ipc_mqueue_t	mqueue;

			assert(ip_active(dest));
			ip_unlock(dest);
			
			kmsg = ipc_kobject_server(kmsg);
			if (kmsg == IKM_NULL)
				goto receive_reply;
			
			ip_lock(reply);
			
			if ((!ip_active(reply)) ||
			    (reply->ip_receiver != space) ||
			    (reply->ip_pset != IPS_NULL) ||
			    (rcv_size < (kmsg->ikm_header.msgh_size + 
			    				kmsg->ikm_delta)))
			{
				ip_unlock(reply);
				ipc_mqueue_send_always(kmsg);
				goto receive_reply;
			}

			mqueue = &reply->ip_messages;
			imq_lock(mqueue);
			
			if ((ipc_thread_queue_first(&mqueue->imq_threads)
				!= ITH_NULL) ||
			    (ipc_kmsg_queue_first(&mqueue->imq_messages)
				!= IKM_NULL))
			{
				imq_unlock(mqueue);
				ip_unlock(reply);
				ipc_mqueue_send_always(kmsg);
				goto receive_reply;
			}

			assert(kmsg->ikm_marequest == IMAR_NULL);
			assert(ipc_thread_queue_first(&reply->ip_blocked)
				== ITH_NULL);

			reply->ip_seqno++;
			imq_unlock(mqueue);

			ip_release(reply);
			ip_unlock(reply);
			
			goto copyout_reply;
		}
		
		ip_unlock(dest);
	}

	if (option & SEND_NOTIFY) {
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_SWITCH) ?
				      MACH_SEND_SWITCH :
				      MACH_MSG_OPTION_NONE) |
				      MACH_SEND_TIMEOUT,
				     ((option & SEND_TIMEOUT) ?
				      (mach_msg_timeout_t) send_timeout :
				      MACH_MSG_TIMEOUT_NONE),
				     IMQ_NULL_CONTINUE);
		if (mr == MACH_SEND_TIMED_OUT) {
			ipc_port_t dest = (ipc_port_t)
				kmsg->ikm_header.msgh_remote_port;

			mr = ipc_marequest_create(space, dest, MACH_PORT_NULL,
						  &kmsg->ikm_marequest);
			if (mr == MACH_MSG_SUCCESS) {
				ipc_mqueue_send_always(kmsg);
				if (IP_VALID(reply))
					ipc_port_release(reply);
				return SEND_WILL_NOTIFY;
			}
		}
	}
	else if (option & SEND_SWITCH)
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_TIMEOUT) ?
				      MACH_SEND_TIMEOUT :
				      MACH_MSG_OPTION_NONE) |
				      MACH_SEND_SWITCH,
				     (mach_msg_timeout_t) send_timeout,
				     IMQ_NULL_CONTINUE);
	else
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_TIMEOUT) ?
				      MACH_SEND_TIMEOUT :
				      MACH_MSG_OPTION_NONE),
				     (mach_msg_timeout_t) send_timeout,
				     IMQ_NULL_CONTINUE);

	if (mr != MACH_MSG_SUCCESS) {
		ipc_kmsg_destroy(kmsg);
		if (IP_VALID(reply))
			ipc_port_release(reply);
		return msg_return_translate(mr);
	}

receive_reply:
	if (!IP_VALID(reply))
		return RCV_INVALID_PORT;

	ip_lock(reply);
	if (reply->ip_receiver != space) {
		ip_release(reply);
		ip_check_unlock(reply);
		return RCV_INVALID_PORT;
	}

	assert(ip_active(reply));
	pset = reply->ip_pset;

	if (pset != IPS_NULL) {
		ips_lock(pset);
		if (ips_active(pset)) {
			ips_unlock(pset);
			ip_release(reply);
			ip_unlock(reply);
			return RCV_INVALID_PORT;
		}

		ipc_pset_remove(pset, reply);
		ips_check_unlock(pset);
		assert(reply->ip_pset == IPS_NULL);
	}

	mqueue = &reply->ip_messages;
	imq_lock(mqueue);
	ip_unlock(reply);

	/*
	 *	ipc_mqueue_receive may not return, because if we block
	 *	then our kernel stack may be discarded.  So we save
	 *	state here for msg_receive_continue to pick up.
	 */

	self = current_thread();
	self->ith_msg = (mach_msg_header_t *) msg;
	self->ith_option = (mach_msg_option_t) option;
	self->ith_rcv_size = (mach_msg_size_t) rcv_size;
	self->ith_timeout = (mach_msg_timeout_t) rcv_timeout;
	self->ith_object = (ipc_object_t) reply;
	self->ith_mqueue = mqueue;

	mr = ipc_mqueue_receive(mqueue,
				((option & RCV_TIMEOUT) ?
				MACH_RCV_TIMEOUT :
				MACH_MSG_OPTION_NONE) |
				MACH_RCV_OLD_FORMAT,
				(option & RCV_LARGE) ?
				(mach_msg_size_t) rcv_size :
				MACH_MSG_SIZE_MAX,
				(mach_msg_timeout_t) rcv_timeout,
				FALSE, msg_receive_continue,
				&kmsg, &seqno, 0);
	/* mqueue is unlocked */
	ipc_port_release(reply);

	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_HEADER_ERROR)
			mr = msg_receive_error(kmsg, msg, space);
		else
		if (mr == MACH_RCV_TOO_LARGE) {
			msg_size_t real_size =
				(msg_size_t) (mach_msg_size_t) kmsg;

			assert(real_size > rcv_size);

			(void) copyout((vm_offset_t) &real_size,
				       (vm_offset_t) &msg->msg_size,
				       sizeof(msg_size_t));
		}

		return msg_return_translate(mr);
	}
	
	if (kmsg->ikm_header.msgh_size > (mach_msg_size_t) rcv_size) {
		assert(!(option & RCV_LARGE));
		
		ipc_kmsg_destroy(kmsg);
		return (RCV_TOO_LARGE);
	}

copyout_reply:
	assert(kmsg->ikm_header.msgh_size <= (mach_msg_size_t) rcv_size);

	mr = ipc_kmsg_copyout_compat(kmsg, space, map);
	assert(mr == MACH_MSG_SUCCESS);

	kmsg->ikm_header.msgh_size += kmsg->ikm_delta;
	mr = ipc_kmsg_put((mach_msg_header_t *) msg,
			  kmsg, kmsg->ikm_header.msgh_size);
	return msg_return_translate(mr);
}

/*
 *	Routine:	msg_receive_continue
 *	Purpose:
 *		Continue after blocking for a message.
 *	Conditions:
 *		Nothing locked.  We are running on a new kernel stack,
 *		with the receive state saved in the thread.  From here
 *		control goes back to user space.
 */

void
msg_receive_continue(void)
{
	ipc_thread_t self = current_thread();
	msg_header_t *msg = (msg_header_t *) self->ith_msg;
	msg_option_t option = (msg_option_t) self->ith_option;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	mr = ipc_mqueue_receive(self->ith_mqueue,
				((option & RCV_TIMEOUT) ?
				MACH_RCV_TIMEOUT :
				MACH_MSG_OPTION_NONE) |
				MACH_RCV_OLD_FORMAT,
				(option & RCV_LARGE) ?
				self->ith_rcv_size :
				MACH_MSG_SIZE_MAX,
				self->ith_timeout,
				TRUE, msg_receive_continue,
				&kmsg, &seqno, 0);
	/* mqueue is unlocked */
	ipc_object_release(self->ith_object);

	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_HEADER_ERROR)
			mr = msg_receive_error(kmsg, msg, current_space());
		else
		if (mr == MACH_RCV_TOO_LARGE) {
			msg_size_t real_size =
				(msg_size_t) (mach_msg_size_t) kmsg;

			assert(real_size > rcv_size);

			(void) copyout((vm_offset_t) &real_size,
				       (vm_offset_t) &msg->msg_size,
				       sizeof(msg_size_t));
		}

		thread_syscall_return(msg_return_translate(mr));
		/*NOTREACHED*/
	}
	
	if (kmsg->ikm_header.msgh_size > self->ith_rcv_size) {
		assert(!(option & RCV_LARGE));
		
		ipc_kmsg_destroy(kmsg);
		thread_syscall_return(RCV_TOO_LARGE);
		/*NOTREACHED*/
	}

	assert(kmsg->ikm_header.msgh_size <= self->ith_rcv_size);

	mr = ipc_kmsg_copyout_compat(kmsg, current_space(), current_map());
	assert(mr == MACH_MSG_SUCCESS);

	kmsg->ikm_header.msgh_size += kmsg->ikm_delta;
	mr = ipc_kmsg_put((mach_msg_header_t *) msg, kmsg,
			  kmsg->ikm_header.msgh_size);
	thread_syscall_return(msg_return_translate(mr));
	/*NOTREACHED*/
}

mach_msg_return_t
msg_receive_error(
	ipc_kmsg_t	kmsg,
	msg_header_t	*msg,
	ipc_space_t	space)
{
	mach_msg_bits_t mbits = kmsg->ikm_header.msgh_bits;
	ipc_object_t reply = (ipc_object_t) kmsg->ikm_header.msgh_local_port;
	mach_msg_type_name_t reply_type = MACH_MSGH_BITS_LOCAL(mbits);
	mach_msg_return_t mr;

	if (IO_VALID(reply))
		ipc_object_destroy(reply, reply_type);
	(ipc_object_t)kmsg->ikm_header.msgh_local_port = IO_NULL;

	assert(!(mbits & MACH_MSGH_BITS_OLD_FORMAT));
	if (mbits & MACH_MSGH_BITS_COMPLEX) {
		mach_msg_body_t *body;

		body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
		ipc_kmsg_clean_body(kmsg, body->msgh_descriptor_count);
	}

	kmsg->ikm_header.msgh_size = sizeof (mach_msg_header_t);
	kmsg->ikm_header.msgh_reserved = 0;	/* copied to msg_type */
	mr = ipc_kmsg_copyout_compat(kmsg, space, VM_MAP_NULL);
	assert(mr == MACH_MSG_SUCCESS);

	return ipc_kmsg_put((mach_msg_header_t *)msg, kmsg,
					kmsg->ikm_header.msgh_size);
}

#endif	MACH_IPC_COMPAT
