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
 *	File:	ipc/ipc_port.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions to manipulate IPC ports.
 */

#import <mach/features.h>

#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/lock.h>
#include <kern/ipc_sched.h>
#include <kern/ipc_kobject.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_thread.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_notify.h>

decl_simple_lock_data(, ipc_port_multiple_lock_data)

decl_simple_lock_data(, ipc_port_timestamp_lock_data)
ipc_port_timestamp_t ipc_port_timestamp_data;

/*
 *	Routine:	ipc_port_timestamp
 *	Purpose:
 *		Retrieve a timestamp value.
 */

ipc_port_timestamp_t
ipc_port_timestamp(void)
{
	ipc_port_timestamp_t timestamp;

	ipc_port_timestamp_lock();
	timestamp = ipc_port_timestamp_data++;
	ipc_port_timestamp_unlock();

	return timestamp;
}

/*
 *	Routine:	ipc_port_dnrequest
 *	Purpose:
 *		Try to allocate a dead-name request slot.
 *		If successful, returns the request index.
 *		Otherwise returns zero.
 *	Conditions:
 *		The port is locked and active.
 *	Returns:
 *		KERN_SUCCESS		A request index was found.
 *		KERN_NO_SPACE		No index allocated.
 */

kern_return_t
ipc_port_dnrequest(
	ipc_port_t			port,
	mach_port_t			name,
	ipc_port_t			soright,
	ipc_port_request_index_t	*indexp)
{
	ipc_port_request_t ipr, table;
	ipc_port_request_index_t index;

	assert(ip_active(port));
	assert(name != MACH_PORT_NULL);
	assert(soright != IP_NULL);

	table = port->ip_dnrequests;
	if (table == IPR_NULL)
		return KERN_NO_SPACE;

	index = table->ipr_next;
	if (index == 0)
		return KERN_NO_SPACE;

	ipr = &table[index];
	assert(ipr->ipr_name == MACH_PORT_NULL);

	table->ipr_next = ipr->ipr_next;
	ipr->ipr_name = name;
	ipr->ipr_soright = soright;

	*indexp = index;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_port_dngrow
 *	Purpose:
 *		Grow a port's table of dead-name requests.
 *	Conditions:
 *		The port must be locked and active.
 *		Nothing else locked; will allocate memory.
 *		Upon return the port is unlocked.
 *	Returns:
 *		KERN_SUCCESS		Grew the table.
 *		KERN_SUCCESS		Somebody else grew the table.
 *		KERN_SUCCESS		The port died.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate new table.
 *		KERN_NO_SPACE		Couldn't grow to desired size
 */

kern_return_t
ipc_port_dngrow(
	ipc_port_t	port,
	int		target_size)
{
	ipc_table_size_t its;
	ipc_port_request_t otable, ntable;

	assert(ip_active(port));

	otable = port->ip_dnrequests;
	if (otable == IPR_NULL)
		its = &ipc_table_dnrequests[0];
	else
		its = otable->ipr_size + 1;

	if (target_size != ITS_SIZE_NONE) {
		if ((otable != IPR_NULL) &&
		    (target_size <= otable->ipr_size->its_size)) {
			ip_unlock(port);
			return KERN_SUCCESS;
	        }
		while ((its->its_size) && (its->its_size < target_size)) {
			its++;
		}
		if (its->its_size == 0) {
			ip_unlock(port);
			return KERN_NO_SPACE;
		}
	}

	ip_reference(port);
	ip_unlock(port);

	if ((its->its_size == 0) ||
	    ((ntable = it_dnrequests_alloc(its)) == IPR_NULL)) {
		ipc_port_release(port);
		return KERN_RESOURCE_SHORTAGE;
	}

	ip_lock(port);
	ip_release(port);

	/*
	 *	Check that port is still active and that nobody else
	 *	has slipped in and grown the table on us.  Note that
	 *	just checking port->ip_dnrequests == otable isn't
	 *	sufficient; must check ipr_size.
	 */

	if (ip_active(port) &&
	    (port->ip_dnrequests == otable) &&
	    ((otable == IPR_NULL) || (otable->ipr_size+1 == its))) {
		ipc_table_size_t oits;
		ipc_table_elems_t osize, nsize;
		ipc_port_request_index_t free, i;

		/* copy old table to new table */

		if (otable != IPR_NULL) {
			oits = otable->ipr_size;
			osize = oits->its_size;
			free = otable->ipr_next;

			bcopy((char *)(otable + 1), (char *)(ntable + 1),
			      (osize - 1) * sizeof(struct ipc_port_request));
		} else {
			osize = 1;
			free = 0;
		}

		nsize = its->its_size;
		assert(nsize > osize);

		/* add new elements to the new table's free list */

		for (i = osize; i < nsize; i++) {
			ipc_port_request_t ipr = &ntable[i];

			ipr->ipr_name = MACH_PORT_NULL;
			ipr->ipr_next = free;
			free = i;
		}

		ntable->ipr_next = free;
		ntable->ipr_size = its;
		port->ip_dnrequests = ntable;
		ip_unlock(port);

		if (otable != IPR_NULL)
			it_dnrequests_free(oits, otable);
	} else {
		ip_check_unlock(port);
		it_dnrequests_free(its, ntable);
	}

	return KERN_SUCCESS;
}
 
/*
 *	Routine:	ipc_port_dncancel
 *	Purpose:
 *		Cancel a dead-name request and return the send-once right.
 *	Conditions:
 *		The port must locked and active.
 */

ipc_port_t
ipc_port_dncancel(
	ipc_port_t			port,
	mach_port_t			name,
	ipc_port_request_index_t	index)
{
	ipc_port_request_t ipr, table;
	ipc_port_t dnrequest;

	assert(ip_active(port));
	assert(name != MACH_PORT_NULL);
	assert(index != 0);

	table = port->ip_dnrequests;
	assert(table != IPR_NULL);

	ipr = &table[index];
	dnrequest = ipr->ipr_soright;
	assert(ipr->ipr_name == name);

	/* return ipr to the free list inside the table */

	ipr->ipr_name = MACH_PORT_NULL;
	ipr->ipr_next = table->ipr_next;
	table->ipr_next = index;

	return dnrequest;
}

/*
 *	Routine:	ipc_port_pdrequest
 *	Purpose:
 *		Make a port-deleted request, returning the
 *		previously registered send-once right.
 *		Just cancels the previous request if notify is IP_NULL.
 *	Conditions:
 *		The port is locked and active.  It is unlocked.
 *		Consumes a ref for notify (if non-null), and
 *		returns previous with a ref (if non-null).
 */

void
ipc_port_pdrequest(
	ipc_port_t	port,
	ipc_port_t	notify,
	ipc_port_t	*previousp)
{
	ipc_port_t previous;

	assert(ip_active(port));

	previous = port->ip_pdrequest;
	port->ip_pdrequest = notify;
	ip_unlock(port);

	*previousp = previous;
}

/*
 *	Routine:	ipc_port_nsrequest
 *	Purpose:
 *		Make a no-senders request, returning the
 *		previously registered send-once right.
 *		Just cancels the previous request if notify is IP_NULL.
 *	Conditions:
 *		The port is locked and active.  It is unlocked.
 *		Consumes a ref for notify (if non-null), and
 *		returns previous with a ref (if non-null).
 */

void
ipc_port_nsrequest(
	ipc_port_t		port,
	mach_port_mscount_t	sync,
	ipc_port_t		notify,
	ipc_port_t		*previousp)
{
	ipc_port_t previous;
	mach_port_mscount_t mscount;

	assert(ip_active(port));

	previous = port->ip_nsrequest;
	mscount = port->ip_mscount;

	if ((port->ip_srights == 0) &&
	    (sync <= mscount) &&
	    (notify != IP_NULL)) {
		port->ip_nsrequest = IP_NULL;
		ip_unlock(port);
		ipc_notify_no_senders(notify, mscount);
	} else {
		port->ip_nsrequest = notify;
		ip_unlock(port);
	}

	*previousp = previous;
}

/*
 *	Routine:	ipc_port_set_qlimit
 *	Purpose:
 *		Changes a port's queue limit; the maximum number
 *		of messages which may be queued to the port.
 *	Conditions:
 *		The port is locked and active.
 */

void
ipc_port_set_qlimit(
	ipc_port_t		port,
	mach_port_msgcount_t	qlimit)
{
	assert(ip_active(port));
	assert(qlimit <= MACH_PORT_QLIMIT_MAX);

	/* wake up senders allowed by the new qlimit */

	if (qlimit > port->ip_qlimit) {
		mach_port_msgcount_t i, wakeup;

		/* caution: wakeup, qlimit are unsigned */

		wakeup = qlimit - port->ip_qlimit;

		for (i = 0; i < wakeup; i++) {
			ipc_thread_t th;

			th = ipc_thread_dequeue(&port->ip_blocked);
			if (th == ITH_NULL)
				break;

			th->ith_state = MACH_MSG_SUCCESS;
			thread_go(th);
		}
	}

	port->ip_qlimit = qlimit;
}

/*
 *	Routine:	ipc_port_set_seqno
 *	Purpose:
 *		Changes a port's sequence number.
 *	Conditions:
 *		The port is locked and active.
 */

void
ipc_port_set_seqno(
	ipc_port_t		port,
	mach_port_seqno_t	seqno)
{
	if (port->ip_pset != IPS_NULL) {
		ipc_pset_t pset = port->ip_pset;

		ips_lock(pset);
		if (!ips_active(pset)) {
			ipc_pset_remove(pset, port);
			ips_check_unlock(pset);
			goto no_port_set;
		} else {
			imq_lock(&pset->ips_messages);
			port->ip_seqno = seqno;
			imq_unlock(&pset->ips_messages);
		}
	} else {
	    no_port_set:
		imq_lock(&port->ip_messages);
		port->ip_seqno = seqno;
		imq_unlock(&port->ip_messages);
	}
}

/*
 *	Routine:	ipc_port_clear_receiver
 *	Purpose:
 *		Prepares a receive right for transmission/destruction.
 *	Conditions:
 *		The port is locked and active.
 */

void
ipc_port_clear_receiver(
	ipc_port_t	port)
{
	ipc_pset_t pset;

	assert(ip_active(port));

	pset = port->ip_pset;
	if (pset != IPS_NULL) {
		/* No threads receiving from port, but must remove from set. */

		ips_lock(pset);
		ipc_pset_remove(pset, port);
		ips_check_unlock(pset);
	} else {
		/* Else, wake up all receivers, indicating why. */

		imq_lock(&port->ip_messages);
		ipc_mqueue_changed(&port->ip_messages, MACH_RCV_PORT_DIED);
		imq_unlock(&port->ip_messages);
	}

	ipc_port_set_mscount(port, 0);
	imq_lock(&port->ip_messages);
	port->ip_seqno = 0;
	imq_unlock(&port->ip_messages);
}

/*
 *	Routine:	ipc_port_init
 *	Purpose:
 *		Initializes a newly-allocated port.
 *		Doesn't touch the ip_object fields.
 */

void
ipc_port_init(
	ipc_port_t	port,
	ipc_space_t	space,
	mach_port_t	name)
{
	/* port->ip_kobject doesn't have to be initialized */

	port->ip_receiver = space;
	port->ip_receiver_name = name;

	port->ip_mscount = 0;
	port->ip_srights = 0;
	port->ip_sorights = 0;

	port->ip_nsrequest = IP_NULL;
	port->ip_pdrequest = IP_NULL;
	port->ip_dnrequests = IPR_NULL;

	port->ip_pset = IPS_NULL;
	port->ip_seqno = 0;
	port->ip_msgcount = 0;
	port->ip_qlimit = MACH_PORT_QLIMIT_DEFAULT;

	ipc_mqueue_init(&port->ip_messages);
	ipc_thread_queue_init(&port->ip_blocked);
}

/*
 *	Routine:	ipc_port_alloc
 *	Purpose:
 *		Allocate a port.
 *	Conditions:
 *		Nothing locked.  If successful, the port is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NO_SPACE		No room for an entry in the space.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_port_alloc(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_port_t	*portp)
{
	ipc_port_t port;
	mach_port_t name;
	kern_return_t kr;

	kr = ipc_object_alloc(space, IOT_PORT,
			      MACH_PORT_TYPE_RECEIVE, 0,
			      &name, (ipc_object_t *) &port);
	if (kr != KERN_SUCCESS)
		return kr;
	/* port is locked */

	ipc_port_init(port, space, name);

	*namep = name;
	*portp = port;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_port_alloc_name
 *	Purpose:
 *		Allocate a port, with a specific name.
 *	Conditions:
 *		Nothing locked.  If successful, the port is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NAME_EXISTS	The name already denotes a right.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_port_alloc_name(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_port_t	*portp)
{
	ipc_port_t port;
	kern_return_t kr;

	kr = ipc_object_alloc_name(space, IOT_PORT,
				   MACH_PORT_TYPE_RECEIVE, 0,
				   name, (ipc_object_t *) &port);
	if (kr != KERN_SUCCESS)
		return kr;
	/* port is locked */

	ipc_port_init(port, space, name);

	*portp = port;
	return KERN_SUCCESS;
}

#if	MACH_IPC_COMPAT
/*
 *	Routine:	ipc_port_delete_compat
 *	Purpose:
 *		Find and destroy a compat entry for a dead port.
 *		If successful, generate a port-deleted notification.
 *	Conditions:
 *		Nothing locked; the port is dead.
 *		Frees a ref for the space.
 */

void
ipc_port_delete_compat(
	ipc_port_t	port,
	ipc_space_t	space,
	mach_port_t	name)
{
	ipc_entry_t entry;
	kern_return_t kr;

	assert(!ip_active(port));

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr == KERN_SUCCESS) {
		ipc_port_t sright;

		/* space is write-locked and active */

		if ((ipc_port_t) entry->ie_object == port) {
			assert(entry->ie_bits & IE_BITS_COMPAT);

			sright = ipc_space_make_notify(space);

			kr = ipc_right_destroy(space, name, entry);
			/* space is unlocked */
			assert(kr == KERN_INVALID_NAME);
		} else {
			is_write_unlock(space);
			sright = IP_NULL;
		}

		if (IP_VALID(sright))
			ipc_notify_port_deleted_compat(sright, name);
	}

	is_release(space);
}
#endif	/* MACH_IPC_COMPAT */

/*
 *	Routine:	ipc_port_destroy
 *	Purpose:
 *		Destroys a port.  Cleans up queued messages.
 *
 *		If the port has a backup, it doesn't get destroyed,
 *		but is sent in a port-destroyed notification to the backup.
 *	Conditions:
 *		The port is locked and alive; nothing else locked.
 *		The caller has a reference, which is consumed.
 *		Afterwards, the port is unlocked and dead.
 */

void
ipc_port_destroy(
	ipc_port_t	port)
{
	ipc_port_t pdrequest, nsrequest;
	ipc_mqueue_t mqueue;
	ipc_kmsg_queue_t kmqueue;
	ipc_kmsg_t kmsg;
	ipc_thread_t sender;
	ipc_port_request_t dnrequests;

	assert(ip_active(port));
	/* port->ip_receiver_name is garbage */
	/* port->ip_receiver/port->ip_destination is garbage */
	assert(port->ip_pset == IPS_NULL);
	assert(port->ip_mscount == 0);
	assert(port->ip_seqno == 0);

	/* first check for a backup port */

	pdrequest = port->ip_pdrequest;
	if (pdrequest != IP_NULL) {
		/* we assume the ref for pdrequest */
		port->ip_pdrequest = IP_NULL;

		/* make port be in limbo */
		port->ip_receiver_name = MACH_PORT_NULL;
		port->ip_destination = IP_NULL;
		ip_unlock(port);

#if	MACH_IPC_COMPAT
		/*
		 *	pdrequest might actually be a send right instead
		 *	of a send-once right, indicated by the low bit
		 *	of the pointer value.  If this is the case,
		 *	we must use ipc_notify_port_destroyed_compat.
		 */

		if (ip_pdsendp(pdrequest)) {
			ipc_port_t sright = ip_pdsend(pdrequest);

			if (!ipc_port_check_circularity(port, sright)) {
				/* consumes our refs for port and sright */
				ipc_notify_port_destroyed_compat(sright, port);
				return;
			} else {
				/* consume sright and destroy port */
				ipc_port_release_send(sright);
			}
		} else
#endif	/* MACH_IPC_COMPAT */

		if (!ipc_port_check_circularity(port, pdrequest)) {
			/* consumes our refs for port and pdrequest */
			ipc_notify_port_destroyed(pdrequest, port);
			return;
		} else {
			/* consume pdrequest and destroy port */
			ipc_port_release_sonce(pdrequest);
		}

		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_pset == IPS_NULL);
		assert(port->ip_mscount == 0);
		assert(port->ip_seqno == 0);
		assert(port->ip_pdrequest == IP_NULL);
		assert(port->ip_receiver_name == MACH_PORT_NULL);
		assert(port->ip_destination == IP_NULL);

		/* fall through and destroy the port */
	}

	/*
	 *	rouse all blocked senders
	 *
	 *	This must be done with the port locked, because
	 *	ipc_mqueue_send can play with the ip_blocked queue
	 *	of a dead port.
	 */

	while ((sender = ipc_thread_dequeue(&port->ip_blocked)) != ITH_NULL) {
		sender->ith_state = MACH_MSG_SUCCESS;
		thread_go(sender);
	}

	/* once port is dead, we don't need to keep it locked */

	port->ip_object.io_bits &= ~IO_BITS_ACTIVE;
	port->ip_timestamp = ipc_port_timestamp();
	ip_unlock(port);

	/* throw away no-senders request */

	nsrequest = port->ip_nsrequest;
	if (nsrequest != IP_NULL)
		ipc_notify_send_once(nsrequest); /* consumes ref */

	/* destroy any queued messages */

	mqueue = &port->ip_messages;
	imq_lock(mqueue);
	assert(ipc_thread_queue_empty(&mqueue->imq_threads));
	kmqueue = &mqueue->imq_messages;

	while ((kmsg = ipc_kmsg_dequeue(kmqueue)) != IKM_NULL) {
		imq_unlock(mqueue);

		assert(kmsg->ikm_header.msgh_remote_port ==
						(mach_port_t) port);

		ipc_port_release(port);
		kmsg->ikm_header.msgh_remote_port = MACH_PORT_NULL;
		ipc_kmsg_destroy(kmsg);

		imq_lock(mqueue);
	}

	imq_unlock(mqueue);

	/* generate dead-name notifications */

	dnrequests = port->ip_dnrequests;
	if (dnrequests != IPR_NULL) {
		ipc_table_size_t its = dnrequests->ipr_size;
		ipc_table_elems_t size = its->its_size;
		ipc_port_request_index_t index;

		for (index = 1; index < size; index++) {
			ipc_port_request_t ipr = &dnrequests[index];
			mach_port_t name = ipr->ipr_name;
			ipc_port_t soright;

			if (name == MACH_PORT_NULL)
				continue;

			soright = ipr->ipr_soright;
			assert(soright != IP_NULL);

#if	MACH_IPC_COMPAT
			if (ipr_spacep(soright)) {
				ipc_port_delete_compat(port,
						ipr_space(soright), name);
				continue;
			}
#endif	/* MACH_IPC_COMPAT */

			ipc_notify_dead_name(soright, name);
		}

		it_dnrequests_free(its, dnrequests);
	}

	if (ip_kotype(port) != IKOT_NONE)
		ipc_kobject_destroy(port);

	ipc_port_release(port); /* consume caller's ref */
}

/*
 *	Routine:	ipc_port_check_circularity
 *	Purpose:
 *		Check if queueing "port" in a message for "dest"
 *		would create a circular group of ports and messages.
 *
 *		If no circularity (FALSE returned), then "port"
 *		is changed from "in limbo" to "in transit".
 *
 *		That is, we want to set port->ip_destination == dest,
 *		but guaranteeing that this doesn't create a circle
 *		port->ip_destination->ip_destination->... == port
 *	Conditions:
 *		No ports locked.  References held for "port" and "dest".
 */

boolean_t
ipc_port_check_circularity(
	ipc_port_t	port,
	ipc_port_t	dest)
{
	ipc_port_t base;

	assert(port != IP_NULL);
	assert(dest != IP_NULL);

	if (port == dest)
		return TRUE;
	base = dest;

	/*
	 *	First try a quick check that can run in parallel.
	 *	No circularity if dest is not in transit.
	 */

	ip_lock(port);
	if (ip_lock_try(dest)) {
		if (!ip_active(dest) ||
		    (dest->ip_receiver_name != MACH_PORT_NULL) ||
		    (dest->ip_destination == IP_NULL))
			goto not_circular;

		/* dest is in transit; further checking necessary */

		ip_unlock(dest);
	}
	ip_unlock(port);

	ipc_port_multiple_lock(); /* massive serialization */

	/*
	 *	Search for the end of the chain (a port not in transit),
	 *	acquiring locks along the way.
	 */

	for (;;) {
		ip_lock(base);

		if (!ip_active(base) ||
		    (base->ip_receiver_name != MACH_PORT_NULL) ||
		    (base->ip_destination == IP_NULL))
			break;

		base = base->ip_destination;
	}

	/* all ports in chain from dest to base, inclusive, are locked */

	if (port == base) {
		/* circularity detected! */

		ipc_port_multiple_unlock();

		/* port (== base) is in limbo */

		assert(ip_active(port));
		assert(port->ip_receiver_name == MACH_PORT_NULL);
		assert(port->ip_destination == IP_NULL);

		while (dest != IP_NULL) {
			ipc_port_t next;

			/* dest is in transit or in limbo */

			assert(ip_active(dest));
			assert(dest->ip_receiver_name == MACH_PORT_NULL);

			next = dest->ip_destination;
			ip_unlock(dest);
			dest = next;
		}

		return TRUE;
	}

	/*
	 *	The guarantee:  lock port while the entire chain is locked.
	 *	Once port is locked, we can take a reference to dest,
	 *	add port to the chain, and unlock everything.
	 */

	ip_lock(port);
	ipc_port_multiple_unlock();

    not_circular:

	/* port is in limbo */

	assert(ip_active(port));
	assert(port->ip_receiver_name == MACH_PORT_NULL);
	assert(port->ip_destination == IP_NULL);

	ip_reference(dest);
	port->ip_destination = dest;

	/* now unlock chain */

	while (port != base) {
		ipc_port_t next;

		/* port is in transit */

		assert(ip_active(port));
		assert(port->ip_receiver_name == MACH_PORT_NULL);
		assert(port->ip_destination != IP_NULL);

		next = port->ip_destination;
		ip_unlock(port);
		port = next;
	}

	/* base is not in transit */

	assert(!ip_active(base) ||
	       (base->ip_receiver_name != MACH_PORT_NULL) ||
	       (base->ip_destination == IP_NULL));
	ip_unlock(base);

	return FALSE;
}

/*
 *	Routine:	ipc_port_lookup_notify
 *	Purpose:
 *		Make a send-once notify port from a receive right.
 *		Returns IP_NULL if name doesn't denote a receive right.
 *	Conditions:
 *		The space must be locked (read or write) and active.
 */

ipc_port_t
ipc_port_lookup_notify(
	ipc_space_t	space,
	mach_port_t	name)
{
	ipc_port_t port;
	ipc_entry_t entry;

	assert(space->is_active);

	entry = ipc_entry_lookup(space, name);
	if (entry == IE_NULL)
		return IP_NULL;

	if ((entry->ie_bits & MACH_PORT_TYPE_RECEIVE) == 0)
		return IP_NULL;

	port = (ipc_port_t) entry->ie_object;
	assert(port != IP_NULL);

	ip_lock(port);
	assert(ip_active(port));
	assert(port->ip_receiver_name == name);
	assert(port->ip_receiver == space);

	ip_reference(port);
	port->ip_sorights++;
	ip_unlock(port);

	return port;
}

/*
 *	Routine:	ipc_port_make_send
 *	Purpose:
 *		Make a naked send right from a receive right.
 *	Conditions:
 *		The port is not locked but it is active.
 */

ipc_port_t
ipc_port_make_send(
	ipc_port_t	port)
{
	assert(IP_VALID(port));

	ip_lock(port);
	assert(ip_active(port));
	port->ip_mscount++;
	port->ip_srights++;
	ip_reference(port);
	ip_unlock(port);

	return port;
}

/*
 *	Routine:	ipc_port_copy_send
 *	Purpose:
 *		Make a naked send right from another naked send right.
 *			IP_NULL		-> IP_NULL
 *			IP_DEAD		-> IP_DEAD
 *			dead port	-> IP_DEAD
 *			live port	-> port + ref
 *	Conditions:
 *		Nothing locked except possibly a space.
 */

ipc_port_t
ipc_port_copy_send(
	ipc_port_t	port)
{
	ipc_port_t sright;

	if (!IP_VALID(port))
		return port;

	ip_lock(port);
	if (ip_active(port)) {
		assert(port->ip_srights > 0);

		ip_reference(port);
		port->ip_srights++;
		sright = port;
	} else
		sright = IP_DEAD;
	ip_unlock(port);

	return sright;
}

/*
 *	Routine:	ipc_port_copyout_send
 *	Purpose:
 *		Copyout a naked send right (possibly null/dead),
 *		or if that fails, destroy the right.
 *	Conditions:
 *		Nothing locked.
 */

mach_port_t
ipc_port_copyout_send(
	ipc_port_t	sright,
	ipc_space_t	space)
{
	mach_port_t name;

	if (IP_VALID(sright)) {
		kern_return_t kr;

		kr = ipc_object_copyout(space, (ipc_object_t) sright,
					MACH_MSG_TYPE_PORT_SEND, TRUE, &name);
		if (kr != KERN_SUCCESS) {
			ipc_port_release_send(sright);

			if (kr == KERN_INVALID_CAPABILITY)
				name = MACH_PORT_DEAD;
			else
				name = MACH_PORT_NULL;
		}
	} else
		name = (mach_port_t) sright;

	return name;
}

/*
 *	Routine:	ipc_port_release_send
 *	Purpose:
 *		Release a (valid) naked send right.
 *		Consumes a ref for the port.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_port_release_send(
	ipc_port_t	port)
{
	ipc_port_t nsrequest = IP_NULL;
	mach_port_mscount_t mscount;

	assert(IP_VALID(port));

	ip_lock(port);
	ip_release(port);

	if (!ip_active(port)) {
		ip_check_unlock(port);
		return;
	}

	assert(port->ip_srights > 0);

	if (--port->ip_srights == 0) {
		nsrequest = port->ip_nsrequest;
		if (nsrequest != IP_NULL) {
			port->ip_nsrequest = IP_NULL;
			mscount = port->ip_mscount;
		}
	}

	ip_unlock(port);

	if (nsrequest != IP_NULL)
		ipc_notify_no_senders(nsrequest, mscount);
}

/*
 *	Routine:	ipc_port_make_sonce
 *	Purpose:
 *		Make a naked send-once right from a receive right.
 *	Conditions:
 *		The port is not locked but it is active.
 */

ipc_port_t
ipc_port_make_sonce(
	ipc_port_t	port)
{
	assert(IP_VALID(port));

	ip_lock(port);
	assert(ip_active(port));
	port->ip_sorights++;
	ip_reference(port);
	ip_unlock(port);

	return port;
}

/*
 *	Routine:	ipc_port_release_sonce
 *	Purpose:
 *		Release a naked send-once right.
 *		Consumes a ref for the port.
 *
 *		In normal situations, this is never used.
 *		Send-once rights are only consumed when
 *		a message (possibly a send-once notification)
 *		is sent to them.
 *	Conditions:
 *		Nothing locked except possibly a space.
 */

void
ipc_port_release_sonce(
	ipc_port_t	port)
{
	assert(IP_VALID(port));

	ip_lock(port);
	ip_release(port);

	if (!ip_active(port)) {
		ip_check_unlock(port);
		return;
	}

	assert(port->ip_sorights > 0);

	port->ip_sorights--;
	ip_unlock(port);
}

/*
 *	Routine:	ipc_port_release_receive
 *	Purpose:
 *		Release a naked (in limbo or in transit) receive right.
 *		Consumes a ref for the port; destroys the port.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_port_release_receive(
	ipc_port_t	port)
{
	ipc_port_t dest;

	assert(IP_VALID(port));

	ip_lock(port);
	assert(ip_active(port));
	assert(port->ip_receiver_name == MACH_PORT_NULL);
	dest = port->ip_destination;

	ipc_port_destroy(port); /* consumes ref, unlocks */

	if (dest != IP_NULL)
		ipc_port_release(dest);
}

/*
 *	Routine:	ipc_port_alloc_special
 *	Purpose:
 *		Allocate a port in a special space.
 *		The new port is returned with one ref.
 *		If unsuccessful, IP_NULL is returned.
 *	Conditions:
 *		Nothing locked.
 */

ipc_port_t
ipc_port_alloc_special(
	ipc_space_t	space)
{
	ipc_port_t port;

	port = (ipc_port_t) io_alloc(IOT_PORT);
	if (port == IP_NULL)
		return IP_NULL;

	io_lock_init(&port->ip_object);
	port->ip_references = 1;
	port->ip_object.io_bits = io_makebits(TRUE, IOT_PORT, 0);

	/*
	 *	The actual values of ip_receiver_name aren't important,
	 *	as long as they are valid (not null/dead).
	 */

	ipc_port_init(port, space, 1);

	return port;
}

/*
 *	Routine:	ipc_port_dealloc_special
 *	Purpose:
 *		Deallocate a port in a special space.
 *		Consumes one ref for the port.
 *	Conditions:
 *		Nothing locked.
 */

void
ipc_port_dealloc_special(
	ipc_port_t	port,
	ipc_space_t	space)
{
	ip_lock(port);
	assert(ip_active(port));
	assert(port->ip_receiver_name != MACH_PORT_NULL);
	assert(port->ip_receiver == space);

	/*
	 *	We clear ip_receiver_name and ip_receiver to simplify
	 *	the ipc_space_kernel check in ipc_mqueue_send.
	 */

	port->ip_receiver_name = MACH_PORT_NULL;
	port->ip_receiver = IS_NULL;

	/*
	 *	For ipc_space_kernel, all ipc_port_clear_receiver does
	 *	is clean things up for the assertions in ipc_port_destroy.
	 *	For ipc_space_reply, there might be a waiting receiver.
	 */

	ipc_port_clear_receiver(port);
	ipc_port_destroy(port);
}

#if	MACH_IPC_COMPAT

/*
 *	Routine:	ipc_port_alloc_compat
 *	Purpose:
 *		Allocate a port.
 *	Conditions:
 *		Nothing locked.  If successful, the port is returned
 *		locked.  (The caller doesn't have a reference.)
 *
 *		Like ipc_port_alloc, except that the new entry
 *		is IE_BITS_COMPAT.
 *	Returns:
 *		KERN_SUCCESS		The port is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NO_SPACE		No room for an entry in the space.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_port_alloc_compat(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_port_t	*portp)
{
	ipc_port_t port;
	ipc_entry_t entry;
	mach_port_t name;
	ipc_table_size_t its;
	ipc_port_request_t table;
	ipc_table_elems_t size;
	ipc_port_request_index_t free, i;
	kern_return_t kr;

	port = ip_alloc();
	if (port == IP_NULL)
		return KERN_RESOURCE_SHORTAGE;

	its = &ipc_table_dnrequests[0];
	table = it_dnrequests_alloc(its);
	if (table == IPR_NULL) {
		ip_free(port);
		return KERN_RESOURCE_SHORTAGE;
	}

	kr = ipc_entry_alloc(space, &name, &entry);
	if (kr != KERN_SUCCESS) {
		ip_free(port);
		it_dnrequests_free(its, table);
		return kr;
	}
	/* space is write-locked */

	entry->ie_object = (ipc_object_t) port;
	entry->ie_request = 1;
	entry->ie_bits |= IE_BITS_COMPAT|MACH_PORT_TYPE_RECEIVE;

	ip_lock_init(port);
	ip_lock(port);
	is_write_unlock(space);

	port->ip_references = 1; /* for entry, not caller */
	port->ip_bits = io_makebits(TRUE, IOT_PORT, 0);

	ipc_port_init(port, space, name);

	size = its->its_size;
	assert(size > 1);
	free = 0;

	for (i = 2; i < size; i++) {
		ipc_port_request_t ipr = &table[i];

		ipr->ipr_name = MACH_PORT_NULL;
		ipr->ipr_next = free;
		free = i;
	}

	table->ipr_next = free;
	table->ipr_size = its;
	port->ip_dnrequests = table;

	table[1].ipr_name = name;
	table[1].ipr_soright = ipr_spacem(space);
	is_reference(space);

	*namep = name;
	*portp = port;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_port_copyout_send_compat
 *	Purpose:
 *		Copyout a naked send right (possibly null/dead),
 *		or if that fails, destroy the right.
 *		Like ipc_port_copyout_send, except that if a
 *		new translation is created it has the compat bit.
 *	Conditions:
 *		Nothing locked.
 */

mach_port_t
ipc_port_copyout_send_compat(
	ipc_port_t	sright,
	ipc_space_t	space)
{
	mach_port_t name;

	if (IP_VALID(sright)) {
		kern_return_t kr;

		kr = ipc_object_copyout_compat(space, (ipc_object_t) sright,
					       MACH_MSG_TYPE_PORT_SEND, &name);
		if (kr != KERN_SUCCESS) {
			ipc_port_release_send(sright);
			name = MACH_PORT_NULL;
		}
	} else
		name = (mach_port_t) sright;

	return name;
}

/*
 *	Routine:	ipc_port_copyout_receiver
 *	Purpose:
 *		Copyout a port reference (possibly null)
 *		by giving the caller his name for the port,
 *		if he is the receiver.
 *	Conditions:
 *		Nothing locked.  Consumes a ref for the port.
 */

mach_port_t
ipc_port_copyout_receiver(
	ipc_port_t	port,
	ipc_space_t	space)
{
	mach_port_t name;

	if (!IP_VALID(port))
		return MACH_PORT_NULL;

	ip_lock(port);
	if (port->ip_receiver == space) {
		name = port->ip_receiver_name;
		assert(MACH_PORT_VALID(name));
	} else
		name = MACH_PORT_NULL;

	ip_release(port);
	ip_check_unlock(port);

	return name;
}

#endif	/* MACH_IPC_COMPAT */
