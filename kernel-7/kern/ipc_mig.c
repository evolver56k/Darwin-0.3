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
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/thread_status.h>
#include <kern/ast.h>
#include <kern/ipc_tt.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/ipc_kobject.h>
#include <vm/vm_map.h>
#include <vm/vm_user.h>
#include <ipc/port.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_thread.h>


/*
 *	Routine:	mach_msg_send_from_kernel
 *	Purpose:
 *		Send a message from the kernel.
 *
 *		This is used by the client side of KernelUser interfaces
 *		to implement SimpleRoutines.  Currently, this includes
 *		device_reply and memory_object messages.
 *	Conditions:
 *		Nothing locked.
 *	Returns:
 *		MACH_MSG_SUCCESS	Sent the message.
 *		MACH_SEND_INVALID_DATA	Bad destination port.
 */

mach_msg_return_t
mach_msg_send_from_kernel(msg, send_size)
	mach_msg_header_t *msg;
	mach_msg_size_t send_size;
{
	ipc_kmsg_t kmsg;
	mach_msg_return_t mr;

	if (!MACH_PORT_VALID(msg->msgh_remote_port))
		return MACH_SEND_INVALID_DEST;

	mr = ipc_kmsg_get_from_kernel(msg, MACH_MSG_OPTION_NONE,
				      send_size, MAX_TRAILER_SIZE,
				      &kmsg);
	if (mr != MACH_MSG_SUCCESS)
		panic("mach_msg_send_from_kernel");

	ipc_kmsg_copyin_from_kernel(kmsg);
	ipc_mqueue_send_always(kmsg);

	return MACH_MSG_SUCCESS;
}

/*
 *	Routine:	mach_msg_abort_rpc
 *	Purpose:
 *		Destroy the thread's ith_rpc_reply port.
 *		This will interrupt a mach_msg_rpc_from_kernel
 *		with a MACH_RCV_PORT_DIED return code.
 *	Conditions:
 *		Nothing locked.
 */

void
mach_msg_abort_rpc(thread)
	ipc_thread_t thread;
{
	ipc_port_t reply = IP_NULL;

	ith_lock(thread);
	if (thread->ith_self != IP_NULL) {
		reply = thread->ith_rpc_reply;
		thread->ith_rpc_reply = IP_NULL;
	}
	ith_unlock(thread);

	if (reply != IP_NULL)
		ipc_port_dealloc_reply(reply);
}

msg_return_t
msg_send_from_kernel(msg, option, time_out)
	msg_header_t *msg;
	msg_option_t option;
	msg_timeout_t time_out;
{
	msg_size_t send_size = msg->msg_size;
	ipc_kmsg_t kmsg;
	integer_t send_delta = send_size;
	mach_msg_return_t mr;

	send_size = (send_size + 3) & ~3; /* round up */
	send_delta -= send_size;

	mr = ipc_kmsg_get_from_kernel((mach_msg_header_t *) msg,
					MACH_MSG_OPTION_NONE,
					(mach_msg_size_t) send_size,
					send_delta,
					&kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);

	ipc_kmsg_copyin_compat_from_kernel(kmsg);
	if (option & SEND_NOTIFY) {
		panic("msg_send_from_kernel");
	} else
		mr = ipc_mqueue_send(kmsg,
				     ((option & SEND_TIMEOUT) ?
				      MACH_SEND_TIMEOUT :
				      MACH_MSG_OPTION_NONE) |
				      MACH_SEND_ALWAYS | MACH_SEND_SWITCH,
				     (mach_msg_timeout_t) time_out,
				     IMQ_NULL_CONTINUE);

	if (mr != MACH_MSG_SUCCESS)
		ipc_kmsg_destroy(kmsg);

	return msg_return_translate(mr);
}

msg_return_t
msg_send(msg, option, time_out)
	msg_header_t *msg;
	msg_option_t option;
	msg_timeout_t time_out;
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	msg_size_t send_size = msg->msg_size;
	ipc_kmsg_t kmsg;
	integer_t send_delta = send_size;
	mach_msg_return_t mr;

	send_size = (send_size + 3) & ~3; /* round up */
	send_delta -= send_size;

	if (send_size > MSG_SIZE_MAX)
		return SEND_MSG_TOO_LARGE;

	mr = ipc_kmsg_get_from_kernel((mach_msg_header_t *) msg,
					MACH_MSG_OPTION_NONE,
					(mach_msg_size_t) send_size,
					send_delta,
					&kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);

	mr = ipc_kmsg_copyin_compat(kmsg, space, map);
	if (mr != MACH_MSG_SUCCESS) {
		ikm_free(kmsg);
		return msg_return_translate(mr);
	}
	if (option & SEND_NOTIFY) {
		panic("msg_send notify");
	} else
		do {
			if (option & SEND_SWITCH)
				mr = ipc_mqueue_send(kmsg,
						((option & SEND_TIMEOUT) ?
						MACH_SEND_TIMEOUT :
						MACH_MSG_OPTION_NONE) |
						MACH_SEND_SWITCH,
						(mach_msg_timeout_t) time_out,
						IMQ_NULL_CONTINUE);
			else
				mr = ipc_mqueue_send(kmsg,
						((option & SEND_TIMEOUT) ?
						MACH_SEND_TIMEOUT :
						MACH_MSG_OPTION_NONE),
						(mach_msg_timeout_t) time_out,
						IMQ_NULL_CONTINUE);
			if (mr == MACH_SEND_INTERRUPTED) {
				while (thread_should_halt(current_thread()))
					thread_halt_self_with_continuation(
							(void (*)(void)) 0);

				if (option & SEND_INTERRUPT)
					break;
			}
		} while (mr == MACH_SEND_INTERRUPTED);

	if (mr != MACH_MSG_SUCCESS)
		ipc_kmsg_destroy(kmsg);

	return msg_return_translate(mr);
}

msg_return_t
msg_receive(msg, option, time_out)
	msg_header_t *msg;
	msg_option_t option;
	msg_timeout_t time_out;
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	port_name_t rcv_name = msg->msg_local_port;
	msg_size_t rcv_size = msg->msg_size;
	ipc_object_t object;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	do {
		mr = ipc_mqueue_copyin(space, (mach_port_t) rcv_name,
					&mqueue, &object);
		if (mr != MACH_MSG_SUCCESS)
			return msg_return_translate(mr);
		/* hold ref for object; mqueue is locked */

		mr = ipc_mqueue_receive(mqueue,
					((option & RCV_TIMEOUT) ?
					MACH_RCV_TIMEOUT :
					MACH_MSG_OPTION_NONE) |
					MACH_RCV_OLD_FORMAT,
					(option & RCV_LARGE) ?
					(mach_msg_size_t) rcv_size :
					MACH_MSG_SIZE_MAX,
					(mach_msg_timeout_t) time_out,
					FALSE, IMQ_NULL_CONTINUE,
					&kmsg, &seqno, 0);
		/* mqueue is unlocked */
		ipc_object_release(object);
		if (mr == MACH_RCV_INTERRUPTED) {
			while (thread_should_halt(current_thread()))
				thread_halt_self_with_continuation(
						(void (*)(void)) 0);
			
			if (option & RCV_INTERRUPT)
				break;
		}
	} while (mr == MACH_RCV_INTERRUPTED);
	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_TOO_LARGE) {
			msg_size_t real_size =
				(msg_size_t) (mach_msg_size_t) kmsg;

			assert(real_size > rcv_size);
			
			msg->msg_size = real_size;
		}

		return msg_return_translate(mr);
	}
	
	if (kmsg->ikm_header.msgh_size > (mach_msg_size_t) rcv_size) {
		assert(!(option & RCV_LARGE));
		
		ipc_kmsg_destroy(kmsg);
		return msg_return_translate(MACH_RCV_TOO_LARGE);
	}

	assert(kmsg->ikm_header.msgh_size <= (mach_msg_size_t) rcv_size);

	mr = ipc_kmsg_copyout_compat(kmsg, space, map);
	assert(mr == MACH_MSG_SUCCESS);

	kmsg->ikm_header.msgh_size += kmsg->ikm_delta;
	ipc_kmsg_put_to_kernel((mach_msg_header_t *) msg, kmsg,
			  	kmsg->ikm_header.msgh_size);
	return msg_return_translate(mr);
}

msg_return_t
msg_rpc(msg, option, rcv_size, send_timeout, rcv_timeout)
	msg_header_t *msg;	/* in/out */
	msg_option_t option;
	msg_size_t rcv_size;
	msg_timeout_t send_timeout;
	msg_timeout_t rcv_timeout;
{
	ipc_space_t space = current_space();
	vm_map_t map = current_map();
	msg_size_t send_size = msg->msg_size;
	ipc_port_t reply;
	ipc_pset_t pset;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	integer_t send_delta = send_size;
	mach_port_seqno_t seqno;
	mach_msg_return_t mr;

	/*
	 *	Instead of using msg_send and msg_receive,
	 *	we implement msg_rpc directly.  The difference
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

	mr = ipc_kmsg_get_from_kernel((mach_msg_header_t *) msg,
					MACH_MSG_OPTION_NONE,
					(mach_msg_size_t) send_size,
					send_delta,
					&kmsg);
	if (mr != MACH_MSG_SUCCESS)
		return msg_return_translate(mr);

	mr = ipc_kmsg_copyin_compat(kmsg, space, map);
	if (mr != MACH_MSG_SUCCESS) {
		ikm_free(kmsg);
		return msg_return_translate(mr);
	}
	
	reply = (ipc_port_t)kmsg->ikm_header.msgh_local_port;
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
		panic("msg_rpc notify");
	} else
		do {
			if (option & SEND_SWITCH)
				mr = ipc_mqueue_send(kmsg,
						    ((option & SEND_TIMEOUT) ?
						    MACH_SEND_TIMEOUT :
						    MACH_MSG_OPTION_NONE) |
						    MACH_SEND_SWITCH,
						    (mach_msg_timeout_t)
							    send_timeout,
						    IMQ_NULL_CONTINUE);
			else
				mr = ipc_mqueue_send(kmsg,
						    ((option & SEND_TIMEOUT) ?
						    MACH_SEND_TIMEOUT :
						    MACH_MSG_OPTION_NONE),
						    (mach_msg_timeout_t)
							    send_timeout,
						    IMQ_NULL_CONTINUE);
			if (mr == MACH_SEND_INTERRUPTED) {
				while (thread_should_halt(current_thread()))
					thread_halt_self_with_continuation(
							(void (*)(void)) 0);

				if (option & SEND_INTERRUPT)
					break;
			}
		} while (mr == MACH_SEND_INTERRUPTED);

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

	mr = ipc_mqueue_receive(mqueue,
				((option & RCV_TIMEOUT) ?
				MACH_RCV_TIMEOUT :
				MACH_MSG_OPTION_NONE) |
				MACH_RCV_OLD_FORMAT,
				(option & RCV_LARGE) ?
				(mach_msg_size_t) rcv_size :
				MACH_MSG_SIZE_MAX,
				(mach_msg_timeout_t) rcv_timeout,
				FALSE, IMQ_NULL_CONTINUE,
				&kmsg, &seqno, 0);
	/* mqueue is unlocked */
	ipc_port_release(reply);
	if (mr != MACH_MSG_SUCCESS) {
		if (mr == MACH_RCV_INTERRUPTED) {
			while (thread_should_halt(current_thread()))
				thread_halt_self_with_continuation(
						(void (*)(void)) 0);
				
			msg->msg_size = rcv_size;	/* XXX */
			
			if (!(option & RCV_INTERRUPT))
				return msg_receive(msg, option, rcv_timeout);
		} else if (mr == MACH_RCV_TOO_LARGE) {
			msg_size_t real_size =
				(msg_size_t) (mach_msg_size_t) kmsg;

			assert(real_size > rcv_size);
			
			msg->msg_size = real_size;
		}

		return msg_return_translate(mr);
	}
	
	if (kmsg->ikm_header.msgh_size > (mach_msg_size_t) rcv_size) {
		assert(!(option & RCV_LARGE));
		
		ipc_kmsg_destroy(kmsg);
		return msg_return_translate(MACH_RCV_TOO_LARGE);
	}

copyout_reply:
	assert(kmsg->ikm_header.msgh_size <= (mach_msg_size_t) rcv_size);

	mr = ipc_kmsg_copyout_compat(kmsg, space, map);
	assert(mr == MACH_MSG_SUCCESS);

	kmsg->ikm_header.msgh_size += kmsg->ikm_delta;
	ipc_kmsg_put_to_kernel((mach_msg_header_t *) msg, kmsg,
			  	kmsg->ikm_header.msgh_size);
	return msg_return_translate(mr);

}

/*
 *	Routine:	mig_get_reply_port
 *	Purpose:
 *		Called by client side interfaces living in the kernel
 *		to get a reply port.  This port is used for
 *		mach_msg() calls which are kernel calls.
 */

mach_port_t
mig_get_reply_port()
{
	ipc_thread_t self = current_thread();

	if (self->ith_mig_reply == MACH_PORT_NULL)
		self->ith_mig_reply = mach_reply_port();

	return self->ith_mig_reply;
}

/*
 *	Routine:	mig_dealloc_reply_port
 *	Purpose:
 *		Called by client side interfaces to get rid of a reply port.
 *		Shouldn't ever be called inside the kernel, because
 *		kernel calls shouldn't prompt Mig to call it.
 */

void
mig_dealloc_reply_port()
{
	panic("mig_dealloc_reply_port");
}

/*
 * mig_strncpy.c - by Joshua Block
 *
 * mig_strncp -- Bounded string copy.  Does what the library routine strncpy
 * OUGHT to do:  Copies the (null terminated) string in src into dest, a 
 * buffer of length len.  Assures that the copy is still null terminated
 * and doesn't overflow the buffer, truncating the copy if necessary.
 *
 * Parameters:
 * 
 *     dest - Pointer to destination buffer.
 * 
 *     src - Pointer to source string.
 * 
 *     len - Length of destination buffer.
 */
vm_size_t mig_strncpy(
	char		*dest,
	const char	*src,
	vm_size_t	len)
{
    vm_size_t i;

    if (len <= 0)
	return 0;

    for (i=1; i<len; i++)
	if (! (*dest++ = *src++))
	    return i;

    *dest = '\0';
    return i;
}

#if	0

#define	fast_send_right_lookup(name, port, abort)			\
MACRO_BEGIN								\
	register ipc_space_t space = current_space();			\
	register ipc_entry_t entry;					\
	register mach_port_index_t index = MACH_PORT_INDEX(name);	\
									\
	is_read_lock(space);						\
	assert(space->is_active);					\
									\
	if ((index >= space->is_table_size) ||				\
	    (((entry = &space->is_table[index])->ie_bits &		\
	      (IE_BITS_GEN_MASK|MACH_PORT_TYPE_SEND)) !=		\
	     (MACH_PORT_GEN(name) | MACH_PORT_TYPE_SEND))) {		\
		is_read_unlock(space);					\
		abort;							\
	}								\
									\
	port = (ipc_port_t) entry->ie_object;				\
	assert(port != IP_NULL);					\
									\
	ip_lock(port);							\
	/* can safely unlock space now that port is locked */		\
	is_read_unlock(space);						\
MACRO_END

thread_t
port_name_to_thread(name)
	mach_port_t name;
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_THREAD)) {
		register thread_t thread;

		thread = (thread_t) port->ip_kobject;
		assert(thread != THREAD_NULL);

		/* thread referencing is a bit complicated,
		   so don't bother to expand inline */
		thread_reference(thread);
		ip_unlock(port);

		return thread;
	}

	ip_unlock(port);
	return THREAD_NULL;

    abort: {
	thread_t thread;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return THREAD_NULL;

	thread = convert_port_to_thread(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return thread;
    }
}

task_t
port_name_to_task(name)
	mach_port_t name;
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_TASK)) {
		register task_t task;

		task = (task_t) port->ip_kobject;
		assert(task != TASK_NULL);

		task_lock(task);
		/* can safely unlock port now that task is locked */
		ip_unlock(port);

		task->ref_count++;
		task_unlock(task);

		return task;
	}

	ip_unlock(port);
	return TASK_NULL;

    abort: {
	task_t task;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return TASK_NULL;

	task = convert_port_to_task(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return task;
    }
}

vm_map_t
port_name_to_map(name)
	mach_port_t name;
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_TASK)) {
		register vm_map_t map;

		map = ((task_t) port->ip_kobject)->map;
		assert(map != VM_MAP_NULL);

		simple_lock(&map->ref_lock);
		/* can safely unlock port now that map is locked */
		ip_unlock(port);

		map->ref_count++;
		simple_unlock(&map->ref_lock);

		return map;
	}

	ip_unlock(port);
	return VM_MAP_NULL;

    abort: {
	vm_map_t map;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return VM_MAP_NULL;

	map = convert_port_to_map(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return map;
    }
}

ipc_space_t
port_name_to_space(name)
	mach_port_t name;
{
	register ipc_port_t port;

	fast_send_right_lookup(name, port, goto abort);
	/* port is locked */

	if (ip_active(port) &&
	    (ip_kotype(port) == IKOT_TASK)) {
		register ipc_space_t space;

		space = ((task_t) port->ip_kobject)->itk_space;
		assert(space != IS_NULL);

		simple_lock(&space->is_ref_lock_data);
		/* can safely unlock port now that space is locked */
		ip_unlock(port);

		space->is_references++;
		simple_unlock(&space->is_ref_lock_data);

		return space;
	}

	ip_unlock(port);
	return IS_NULL;

    abort: {
	ipc_space_t space;
	ipc_port_t kern_port;
	kern_return_t kr;

	kr = ipc_object_copyin(current_space(), name,
			       MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &kern_port);
	if (kr != KERN_SUCCESS)
		return IS_NULL;

	space = convert_port_to_space(kern_port);
	if (IP_VALID(kern_port))
		ipc_port_release_send(kern_port);

	return space;
    }
}

/*
 * Hack to translate a thread port to a thread pointer for calling
 * thread_get_state and thread_set_state.  This is only necessary
 * because the IPC message for these two operations overflows the
 * kernel stack.
 *
 * AARGH!
 */

kern_return_t thread_get_state_KERNEL(thread_port, flavor,
			old_state, old_state_count)
	mach_port_t	thread_port;	/* port right for thread */
	int		flavor;
	thread_state_t	old_state;	/* pointer to OUT array */
	natural_t	*old_state_count;	/* IN/OUT */
{
	thread_t	thread;
	kern_return_t	result;

	thread = port_name_to_thread(thread_port);
	result = thread_get_state(thread, flavor, old_state, old_state_count);
	thread_deallocate(thread);

	return result;
}

kern_return_t thread_set_state_KERNEL(thread_port, flavor,
			new_state, new_state_count)
	mach_port_t	thread_port;	/* port right for thread */
	int		flavor;
	thread_state_t	new_state;
	natural_t	new_state_count;
{
	thread_t	thread;
	kern_return_t	result;

	thread = port_name_to_thread(thread_port);
	result = thread_set_state(thread, flavor, new_state, new_state_count);
	thread_deallocate(thread);

	return result;
}

/*
 *	Things to keep in mind:
 *
 *	The idea here is to duplicate the semantics of the true kernel RPC.
 *	The destination port/object should be checked first, before anything
 *	that the user might notice (like ipc_object_copyin).  Return
 *	MACH_SEND_INTERRUPTED if it isn't correct, so that the user stub
 *	knows to fall back on an RPC.  For other return values, it won't
 *	retry with an RPC.  The retry might get a different (incorrect) rc.
 *	Return values are only set (and should only be set, with copyout)
 *	on successfull calls.
 */

kern_return_t syscall_vm_map(
		target_map,
		address, size, mask, anywhere,
		memory_object, offset,
		copy,
		cur_protection, max_protection,	inheritance)
	mach_port_t	target_map;
	vm_offset_t	*address;
	vm_size_t	size;
	vm_offset_t	mask;
	boolean_t	anywhere;
	mach_port_t	memory_object;
	vm_offset_t	offset;
	boolean_t	copy;
	vm_prot_t	cur_protection;
	vm_prot_t	max_protection;
	vm_inherit_t	inheritance;
{
	vm_map_t		map;
	ipc_port_t		port;
	vm_offset_t		addr;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(memory_object)) {
		result = ipc_object_copyin(current_space(), memory_object,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			vm_map_deallocate(map);
			return result;
		}
	} else
		port = (ipc_port_t) memory_object;

	copyin((char *)address, (char *)&addr, sizeof(vm_offset_t));
	result = vm_map(map, &addr, size, mask, anywhere,
			port, offset, copy,
			cur_protection, max_protection,	inheritance);
	if (result == KERN_SUCCESS)
		copyout((char *)&addr, (char *)address, sizeof(vm_offset_t));
	if (IP_VALID(port))
		ipc_port_release_send(port);
	vm_map_deallocate(map);

	return result;
}

kern_return_t syscall_vm_allocate(target_map, address, size, anywhere)
	mach_port_t		target_map;
	vm_offset_t		*address;
	vm_size_t		size;
	boolean_t		anywhere;
{
	vm_map_t		map;
	vm_offset_t		addr;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	copyin((char *)address, (char *)&addr, sizeof(vm_offset_t));
	result = vm_allocate(map, &addr, size, anywhere);
	if (result == KERN_SUCCESS)
		copyout((char *)&addr, (char *)address, sizeof(vm_offset_t));
	vm_map_deallocate(map);

	return result;
}

kern_return_t syscall_vm_deallocate(target_map, start, size)
	mach_port_t		target_map;
	vm_offset_t		start;
	vm_size_t		size;
{
	vm_map_t		map;
	kern_return_t		result;

	map = port_name_to_map(target_map);
	if (map == VM_MAP_NULL)
		return MACH_SEND_INTERRUPTED;

	result = vm_deallocate(map, start, size);
	vm_map_deallocate(map);

	return result;
}

kern_return_t syscall_task_create(parent_task, inherit_memory, child_task)
	mach_port_t	parent_task;
	boolean_t	inherit_memory;
	mach_port_t	*child_task;		/* OUT */
{
	task_t		t, c;
	ipc_port_t	port;
	mach_port_t 	name;
	kern_return_t	result;

	t = port_name_to_task(parent_task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_create(t, inherit_memory, &c);
	if (result == KERN_SUCCESS) {
		port = (ipc_port_t) convert_task_to_port(c);
		/* always returns a name, even for non-success return codes */
		(void) ipc_kmsg_copyout_object(current_space(),
					       (ipc_object_t) port,
					       MACH_MSG_TYPE_PORT_SEND, &name);
		copyout((char *)&name, (char *)child_task,
			sizeof(mach_port_t));
	}
	task_deallocate(t);

	return result;
}

kern_return_t syscall_task_terminate(task)
	mach_port_t	task;
{
	task_t		t;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_terminate(t);
	task_deallocate(t);

	return result;
}

kern_return_t syscall_task_suspend(task)
	mach_port_t	task;
{
	task_t		t;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	result = task_suspend(t);
	task_deallocate(t);

	return result;
}

kern_return_t syscall_task_set_special_port(task, which_port, port_name)
	mach_port_t	task;
	int		which_port;
	mach_port_t	port_name;
{
	task_t		t;
	ipc_port_t	port;
	kern_return_t	result;

	t = port_name_to_task(task);
	if (t == TASK_NULL)
		return MACH_SEND_INTERRUPTED;

	if (MACH_PORT_VALID(port_name)) {
		result = ipc_object_copyin(current_space(), port_name,
					   MACH_MSG_TYPE_COPY_SEND,
					   (ipc_object_t *) &port);
		if (result != KERN_SUCCESS) {
			task_deallocate(t);
			return result;
		}
	} else
		port = (ipc_port_t) port_name;

	result = task_set_special_port(t, which_port, port);
	if ((result != KERN_SUCCESS) && IP_VALID(port))
		ipc_port_release_send(port);
	task_deallocate(t);

	return result;
}

kern_return_t
syscall_mach_port_allocate(task, right, namep)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t *namep;
{
	ipc_space_t space;
	mach_port_t name;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_allocate(space, right, &name);
	if (kr == KERN_SUCCESS)
		copyout((char *)&name, (char *)namep, sizeof(mach_port_t));
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_allocate_name(task, right, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t name;
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_allocate_name(space, right, name);
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_deallocate(task, name)
	mach_port_t task;
	mach_port_t name;
{
	ipc_space_t space;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	kr = mach_port_deallocate(space, name);
	is_release(space);

	return kr;
}

kern_return_t
syscall_mach_port_insert_right(task, name, right, rightType)
	mach_port_t task;
	mach_port_t name;
	mach_port_t right;
	mach_msg_type_name_t rightType;
{
	ipc_space_t space;
	ipc_object_t object;
	mach_msg_type_name_t newtype;
	kern_return_t kr;

	space = port_name_to_space(task);
	if (space == IS_NULL)
		return MACH_SEND_INTERRUPTED;

	if (!MACH_MSG_TYPE_PORT_ANY(rightType)) {
		is_release(space);
		return KERN_INVALID_VALUE;
	}

	if (MACH_PORT_VALID(right)) {
		kr = ipc_object_copyin(current_space(), right, rightType,
				       &object);
		if (kr != KERN_SUCCESS) {
			is_release(space);
			return kr;
		}
	} else
		object = (ipc_object_t) right;
	newtype = ipc_object_copyin_type(rightType);

	kr = mach_port_insert_right(space, name, object, newtype);
	if ((kr != KERN_SUCCESS) && IO_VALID(object))
		ipc_object_destroy(object, newtype);
	is_release(space);

	return kr;
}

kern_return_t syscall_thread_depress_abort(thread)
	mach_port_t	thread;
{
	thread_t	t;
	kern_return_t	result;

	t = port_name_to_thread(thread);
	if (t == THREAD_NULL)
		return MACH_SEND_INTERRUPTED;

	result = thread_depress_abort(t);
	thread_deallocate(t);

	return result;
}

#endif
