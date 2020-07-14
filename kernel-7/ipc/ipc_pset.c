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
 *	File:	ipc/ipc_pset.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions to manipulate IPC port sets.
 */

#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_space.h>

/*
 * Forward declarations
 */
void ipc_pset_add(
	ipc_pset_t	pset,
	ipc_port_t	port);

/*
 *	Routine:	ipc_pset_alloc
 *	Purpose:
 *		Allocate a port set.
 *	Conditions:
 *		Nothing locked.  If successful, the port set is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port set is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NO_SPACE		No room for an entry in the space.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_pset_alloc(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_pset_t	*psetp)
{
	ipc_pset_t pset;
	mach_port_t name;
	kern_return_t kr;

	kr = ipc_object_alloc(space, IOT_PORT_SET,
			      MACH_PORT_TYPE_PORT_SET, 0,
			      &name, (ipc_object_t *) &pset);
	if (kr != KERN_SUCCESS)
		return kr;
	/* pset is locked */

	pset->ips_local_name = name;
	ipc_mqueue_init(&pset->ips_messages);

	*namep = name;
	*psetp = pset;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_pset_alloc_name
 *	Purpose:
 *		Allocate a port set, with a specific name.
 *	Conditions:
 *		Nothing locked.  If successful, the port set is returned
 *		locked.  (The caller doesn't have a reference.)
 *	Returns:
 *		KERN_SUCCESS		The port set is allocated.
 *		KERN_INVALID_TASK	The space is dead.
 *		KERN_NAME_EXISTS	The name already denotes a right.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
ipc_pset_alloc_name(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_pset_t	*psetp)
{
	ipc_pset_t pset;
	kern_return_t kr;

	kr = ipc_object_alloc_name(space, IOT_PORT_SET,
				   MACH_PORT_TYPE_PORT_SET, 0,
				   name, (ipc_object_t *) &pset);
	if (kr != KERN_SUCCESS)
		return kr;
	/* pset is locked */

	pset->ips_local_name = name;
	ipc_mqueue_init(&pset->ips_messages);

	*psetp = pset;
	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_pset_add
 *	Purpose:
 *		Puts a port into a port set.
 *		The port set gains a reference.
 *	Conditions:
 *		Both port and port set are locked and active.
 *		The port isn't already in a set.
 *		The owner of the port set is also receiver for the port.
 */

void
ipc_pset_add(
	ipc_pset_t	pset,
	ipc_port_t	port)
{
	assert(ips_active(pset));
	assert(ip_active(port));
	assert(port->ip_pset == IPS_NULL);

	port->ip_pset = pset;
	ips_reference(pset);

	imq_lock(&port->ip_messages);
	imq_lock(&pset->ips_messages);

	/* move messages from port's queue to the port set's queue */

	ipc_mqueue_move(&pset->ips_messages, &port->ip_messages, port);
	imq_unlock(&pset->ips_messages);
	assert(ipc_kmsg_queue_empty(&port->ip_messages.imq_messages));

	/* wake up threads waiting to receive from the port */

	ipc_mqueue_changed(&port->ip_messages, MACH_RCV_PORT_CHANGED);
	assert(ipc_thread_queue_empty(&port->ip_messages.imq_threads));
	imq_unlock(&port->ip_messages);
}

/*
 *	Routine:	ipc_pset_remove
 *	Purpose:
 *		Removes a port from a port set.
 *		The port set loses a reference.
 *	Conditions:
 *		Both port and port set are locked.
 *		The port must be active.
 */

void
ipc_pset_remove(
	ipc_pset_t	pset,
	ipc_port_t	port)
{
	assert(ip_active(port));
	assert(port->ip_pset == pset);

	port->ip_pset = IPS_NULL;
	ips_release(pset);

	imq_lock(&port->ip_messages);
	imq_lock(&pset->ips_messages);

	/* move messages from port set's queue to the port's queue */

	ipc_mqueue_move(&port->ip_messages, &pset->ips_messages, port);

	imq_unlock(&pset->ips_messages);
	imq_unlock(&port->ip_messages);
}

/*
 *	Routine:	ipc_pset_move
 *	Purpose:
 *		If nset is IPS_NULL, removes port
 *		from the port set it is in.  Otherwise, adds
 *		port to nset, removing it from any set
 *		it might already be in.
 *	Conditions:
 *		The space is read-locked.
 *	Returns:
 *		KERN_SUCCESS		Moved the port.
 *		KERN_NOT_IN_SET		nset is null and port isn't in a set.
 */

kern_return_t
ipc_pset_move(
	ipc_space_t	space,
	ipc_port_t	port,
	ipc_pset_t	nset)
{
	ipc_pset_t oset;

	/*
	 *	While we've got the space locked, it holds refs for
	 *	the port and nset (because of the entries).  Also,
	 *	they must be alive.  While we've got port locked, it
	 *	holds a ref for oset, which might not be alive.
	 */

	ip_lock(port);
	assert(ip_active(port));

	oset = port->ip_pset;

	if (oset == nset) {
		/* the port is already in the new set:  a noop */

		is_read_unlock(space);
	} else if (oset == IPS_NULL) {
		/* just add port to the new set */

		ips_lock(nset);
		assert(ips_active(nset));
		is_read_unlock(space);

		ipc_pset_add(nset, port);

		ips_unlock(nset);
	} else if (nset == IPS_NULL) {
		/* just remove port from the old set */

		is_read_unlock(space);
		ips_lock(oset);

		ipc_pset_remove(oset, port);

		if (ips_active(oset))
			ips_unlock(oset);
		else {
			ips_check_unlock(oset);
			oset = IPS_NULL; /* trigger KERN_NOT_IN_SET */
		}
	} else {
		/* atomically move port from oset to nset */

		if (oset < nset) {
			ips_lock(oset);
			ips_lock(nset);
		} else {
			ips_lock(nset);
			ips_lock(oset);
		}

		is_read_unlock(space);
		assert(ips_active(nset));

		ipc_pset_remove(oset, port);
		ipc_pset_add(nset, port);

		ips_unlock(nset);
		ips_check_unlock(oset);	/* KERN_NOT_IN_SET not a possibility */
	}

	ip_unlock(port);

	return (((nset == IPS_NULL) && (oset == IPS_NULL)) ?
		KERN_NOT_IN_SET : KERN_SUCCESS);
}

/*
 *	Routine:	ipc_pset_destroy
 *	Purpose:
 *		Destroys a port_set.
 *
 *		Doesn't remove members from the port set;
 *		that happens lazily.  As members are removed,
 *		their messages are removed from the queue.
 *	Conditions:
 *		The port_set is locked and alive.
 *		The caller has a reference, which is consumed.
 *		Afterwards, the port_set is unlocked and dead.
 */

void
ipc_pset_destroy(
	ipc_pset_t	pset)
{
	assert(ips_active(pset));

	pset->ips_object.io_bits &= ~IO_BITS_ACTIVE;

	imq_lock(&pset->ips_messages);
	ipc_mqueue_changed(&pset->ips_messages, MACH_RCV_PORT_DIED);
	imq_unlock(&pset->ips_messages);

	ips_release(pset);	/* consume the ref our caller gave us */
	ips_check_unlock(pset);
}
