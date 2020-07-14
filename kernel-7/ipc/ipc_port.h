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
 *	File:	ipc/ipc_port.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for ports.
 */

#ifndef	_IPC_IPC_PORT_H_
#define _IPC_IPC_PORT_H_

#import <mach/features.h>

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <kern/lock.h>
#include <kern/macro_help.h>
#include <kern/ipc_kobject.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_thread.h>

/*
 *  A receive right (port) can be in four states:
 *	1) dead (not active, ip_timestamp has death time)
 *	2) in a space (ip_receiver_name != 0, ip_receiver points
 *	to the space but doesn't hold a ref for it)
 *	3) in transit (ip_receiver_name == 0, ip_destination points
 *	to the destination port and holds a ref for it)
 *	4) in limbo (ip_receiver_name == 0, ip_destination == IP_NULL)
 *
 *  If the port is active, and ip_receiver points to some space,
 *  then ip_receiver_name != 0, and that space holds receive rights.
 *  If the port is not active, then ip_timestamp contains a timestamp
 *  taken when the port was destroyed.
 */

typedef unsigned int ipc_port_timestamp_t;

struct ipc_port {
	struct ipc_object ip_object;

	union {
		struct ipc_space *receiver;
		struct ipc_port *destination;
		ipc_port_timestamp_t timestamp;
	} data;
	mach_port_t ip_receiver_name;

	ipc_kobject_t ip_kobject;

	mach_port_mscount_t ip_mscount;
	mach_port_rights_t ip_srights;
	mach_port_rights_t ip_sorights;

	struct ipc_port *ip_nsrequest;
	struct ipc_port *ip_pdrequest;
	struct ipc_port_request *ip_dnrequests;

	struct ipc_pset *ip_pset;
	mach_port_seqno_t ip_seqno;		/* locked by message queue */
	mach_port_msgcount_t ip_msgcount;
	mach_port_msgcount_t ip_qlimit;
	struct ipc_mqueue ip_messages;
	struct ipc_thread_queue ip_blocked;
};

#define	ip_references		ip_object.io_references
#define	ip_bits			ip_object.io_bits
#define	ip_receiver		data.receiver
#define	ip_destination		data.destination
#define	ip_timestamp		data.timestamp

#define	IP_NULL			((ipc_port_t) IO_NULL)
#define	IP_DEAD			((ipc_port_t) IO_DEAD)

#define	IP_VALID(port)		IO_VALID(&(port)->ip_object)

#define	ip_active(port)		io_active(&(port)->ip_object)
#define	ip_lock_init(port)	io_lock_init(&(port)->ip_object)
#define	ip_lock(port)		io_lock(&(port)->ip_object)
#define	ip_lock_try(port)	io_lock_try(&(port)->ip_object)
#define	ip_unlock(port)		io_unlock(&(port)->ip_object)
#define	ip_check_unlock(port)	io_check_unlock(&(port)->ip_object)
#define	ip_reference(port)	io_reference(&(port)->ip_object)
#define	ip_release(port)	io_release(&(port)->ip_object)

#define	ip_alloc()		((ipc_port_t) io_alloc(IOT_PORT))
#define	ip_free(port)		io_free(IOT_PORT, &(port)->ip_object)

#define	ip_kotype(port)		io_kotype(&(port)->ip_object)

typedef ipc_table_index_t ipc_port_request_index_t;

typedef struct ipc_port_request {
	union {
		struct ipc_port *port;
		ipc_port_request_index_t index;
	} notify;

	union {
		mach_port_t name;
		struct ipc_table_size *size;
	} name;
} *ipc_port_request_t;

#define	ipr_next		notify.index
#define	ipr_size		name.size

#define	ipr_soright		notify.port
#define	ipr_name		name.name

#define	IPR_NULL		((ipc_port_request_t) 0)

#if	MACH_IPC_COMPAT
/*
 *	For backwards compatibility, the ip_pdrequest field can hold a
 *	send right instead of a send-once right.  This is indicated by
 *	the low bit of the pointer.  This works because the zone package
 *	guarantees that the two low bits of port pointers are zero.
 */

#define	ip_pdsendp(soright)	((unsigned int)(soright) & 1)
#define ip_pdsend(soright)	((ipc_port_t)((unsigned int)(soright) &~ 1))
#define	ip_pdsendm(sright)	((ipc_port_t)((unsigned int)(sright) | 1))

/*
 *	For backwards compatibility, the ipr_soright field can hold
 *	a space pointer.  This is indicated by the low bit of the pointer.
 *	This works because the zone package guarantees that the two low
 *	bits of port and space pointers are zero.
 */

#define	ipr_spacep(soright)	((unsigned int)(soright) & 1)
#define ipr_space(soright)	((ipc_space_t)((unsigned int)(soright) &~ 1))
#define	ipr_spacem(space)	((ipc_port_t)((unsigned int)(space) | 1))
#endif	/* MACH_IPC_COMPAT */

/*
 *	Taking the ipc_port_multiple lock grants the privilege
 *	to lock multiple ports at once.  No ports must locked
 *	when it is taken.
 */

decl_simple_lock_data(extern, ipc_port_multiple_lock_data)

#define	ipc_port_multiple_lock_init()					\
		simple_lock_init(&ipc_port_multiple_lock_data)

#define	ipc_port_multiple_lock()					\
		simple_lock(&ipc_port_multiple_lock_data)

#define	ipc_port_multiple_unlock()					\
		simple_unlock(&ipc_port_multiple_lock_data)

/*
 *	The port timestamp facility provides timestamps
 *	for port destruction.  It is used to serialize
 *	mach_port_names with port death.
 */

decl_simple_lock_data(extern, ipc_port_timestamp_lock_data)
extern ipc_port_timestamp_t ipc_port_timestamp_data;

#define	ipc_port_timestamp_lock_init()					\
		simple_lock_init(&ipc_port_timestamp_lock_data)

#define	ipc_port_timestamp_lock()					\
		simple_lock(&ipc_port_timestamp_lock_data)

#define	ipc_port_timestamp_unlock()					\
		simple_unlock(&ipc_port_timestamp_lock_data)

/* Retrieve a port timestamp value */
extern ipc_port_timestamp_t ipc_port_timestamp(void);

/*
 *	Compares two timestamps, and returns TRUE if one
 *	happened before two.  Note that this formulation
 *	works when the timestamp wraps around at 2^32,
 *	as long as one and two aren't too far apart.
 */

#define	IP_TIMESTAMP_ORDER(one, two)	((int) ((one) - (two)) < 0)

#define	ipc_port_translate_receive(space, name, portp)			\
		ipc_object_translate((space), (name),			\
				     MACH_PORT_RIGHT_RECEIVE,		\
				     (ipc_object_t *) (portp))

#define	ipc_port_translate_send(space, name, portp)			\
		ipc_object_translate((space), (name),			\
				     MACH_PORT_RIGHT_SEND,		\
				     (ipc_object_t *) (portp))

/* Allocate a dead-name request slot */
extern kern_return_t
ipc_port_dnrequest(
	ipc_port_t			port,
	mach_port_t			name,
	ipc_port_t			soright,
	ipc_port_request_index_t	*indexp);

/* Grow a port's table of dead-name requests */
extern kern_return_t ipc_port_dngrow(
	ipc_port_t	port,
	int		target_size);

/* Cancel a dead-name request and return the send-once right */
extern ipc_port_t ipc_port_dncancel(
	ipc_port_t			port,
	mach_port_t			name,
	ipc_port_request_index_t	index);

#define	ipc_port_dnrename(port, index, oname, nname)			\
MACRO_BEGIN								\
	ipc_port_request_t ipr, table;					\
									\
	assert(ip_active(port));					\
									\
	table = port->ip_dnrequests;					\
	assert(table != IPR_NULL);					\
									\
	ipr = &table[index];						\
	assert(ipr->ipr_name == oname);					\
									\
	ipr->ipr_name = nname;						\
MACRO_END

/* Make a port-deleted request */
extern void ipc_port_pdrequest(
	ipc_port_t	port,
	ipc_port_t	notify,
	ipc_port_t	*previousp);

/* Make a no-senders request */
extern void ipc_port_nsrequest(
	ipc_port_t		port,
	mach_port_mscount_t	sync,
	ipc_port_t		notify,
	ipc_port_t		*previousp);

/* Change a port's queue limit */
extern void ipc_port_set_qlimit(
	ipc_port_t		port,
	mach_port_msgcount_t	qlimit);

#define	ipc_port_set_mscount(port, mscount)				\
MACRO_BEGIN								\
	assert(ip_active(port));					\
									\
	(port)->ip_mscount = (mscount);					\
MACRO_END

/* Change a port's sequence number */
extern void ipc_port_set_seqno(
	ipc_port_t		port, 
	mach_port_seqno_t 	seqno);

/* Prepare a receive right for transmission/destruction */
extern void ipc_port_clear_receiver(
	ipc_port_t	port);

/* Initialize a newly-allocated port */
extern void ipc_port_init(
	ipc_port_t	port,
	ipc_space_t	space,
	mach_port_t	name);

/* Allocate a port */
extern kern_return_t ipc_port_alloc(
	ipc_space_t	space,
	mach_port_t	*namep,
	ipc_port_t	*portp);

/* Allocate a port, with a specific name */
extern kern_return_t ipc_port_alloc_name(
	ipc_space_t	space,
	mach_port_t	name,
	ipc_port_t	*portp);

/* Destroy a port */
extern void ipc_port_destroy(
	ipc_port_t	port);

/* Check if queueing "port" in a message for "dest" would create a circular 
   group of ports and messages */
extern boolean_t
ipc_port_check_circularity(
	ipc_port_t	port,
	ipc_port_t	dest);

/* Make a send-once notify port from a receive right */
extern ipc_port_t ipc_port_lookup_notify(
	ipc_space_t	space, 
	mach_port_t 	name);

/* Make a naked send right from a receive right */
extern ipc_port_t ipc_port_make_send(
	ipc_port_t	port);

/* Make a naked send right from another naked send right */
extern ipc_port_t ipc_port_copy_send(
	ipc_port_t	port);

/* Copyout a naked send right */
extern mach_port_t ipc_port_copyout_send(
	ipc_port_t	sright,
	ipc_space_t	space);

/* Release a (valid) naked send right */
extern void ipc_port_release_send(
	ipc_port_t	port);

/* Make a naked send-once right from a receive right */
extern ipc_port_t ipc_port_make_sonce(
	ipc_port_t	port);

/* Release a naked send-once right */
extern void ipc_port_release_sonce(
	ipc_port_t	port);

/* Release a naked (in limbo or in transit) receive right */
extern void ipc_port_release_receive(
	ipc_port_t	port);

/* Allocate a port in a special space */
extern ipc_port_t ipc_port_alloc_special(
	ipc_space_t	space);

/* Deallocate a port in a special space */
extern void ipc_port_dealloc_special(
	ipc_port_t	port,
	ipc_space_t	space);

#define	ipc_port_alloc_kernel()		\
		ipc_port_alloc_special(ipc_space_kernel)
#define	ipc_port_dealloc_kernel(port)	\
		ipc_port_dealloc_special((port), ipc_space_kernel)

#define	ipc_port_alloc_reply()		\
		ipc_port_alloc_special(ipc_space_reply)
#define	ipc_port_dealloc_reply(port)	\
		ipc_port_dealloc_special((port), ipc_space_reply)

#define	ipc_port_reference(port)	\
		ipc_object_reference(&(port)->ip_object)

#define	ipc_port_release(port)		\
		ipc_object_release(&(port)->ip_object)

#if	MACH_IPC_COMPAT

extern kern_return_t
ipc_port_alloc_compat(/* ipc_space_t, mach_port_t *, ipc_port_t * */);

extern mach_port_t
ipc_port_copyout_send_compat(/* ipc_port_t, ipc_space_t */);

extern mach_port_t
ipc_port_copyout_receiver(/* ipc_port_t, ipc_space_t */);

#endif	/* MACH_IPC_COMPAT */

extern void
ipc_port_print(/* ipc_port_t */);

#endif	/* _IPC_IPC_PORT_H_ */
