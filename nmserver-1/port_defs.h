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

#ifndef	_PORT_DEFS_
#define	_PORT_DEFS_

#include <mach/mach_types.h>

#include "key_defs.h"
#include "ls_defs.h"
#include <servers/nm_defs.h>
#include "rwlock.h"
#include "mem.h"

#include "sys_queue.h"

typedef network_port_t	*network_port_ptr_t;

/*
 * Macros to determine the rights that we have to a port.
 */
#define NPORT_HAVE_SEND_RIGHTS(nport) \
    (((nport).np_receiver != my_host_id) && ((nport).np_owner != my_host_id))
#define NPORT_HAVE_REC_RIGHTS(nport) \
    ((nport).np_receiver == my_host_id)
#define NPORT_HAVE_RO_RIGHTS(nport) \
    (((nport).np_receiver == my_host_id) || ((nport).np_owner == my_host_id))
#define NPORT_HAVE_ALL_RIGHTS(nport) \
    (((nport).np_receiver == my_host_id) && ((nport).np_owner == my_host_id))

/*
 * Macro to test for network port equality.
 */
#define NPORT_EQUAL(nport1,nport2) (					\
	((nport1).np_puid.np_uid_high == (nport2).np_puid.np_uid_high)	\
	&& ((nport1).np_puid.np_uid_low == (nport2).np_puid.np_uid_low)	\
	&& ((nport1).np_sid.np_uid_low == (nport2).np_sid.np_uid_low)	\
	&& ((nport1).np_sid.np_uid_low == (nport2).np_sid.np_uid_low))


/*
 * Stucture used by pr_list.
 */
typedef struct port_item {
	struct port_item	*next;
	port_t			pi_port;
} port_item_t, *port_item_ptr_t;


/*
 * Information maintained about ports.
 */
typedef struct pq_hash {
    struct pq_hash	*next;
    struct port_rec_	*pqh_portrec;
} pq_hash_t, *pq_hash_ptr_t;

typedef struct port_rec_{
    pq_hash_t		portrec_localitem;
    pq_hash_t		portrec_networkitem;
    int			portrec_refcount;
    int			portrec_info;
    int			portrec_port_rights;
    port_t		portrec_local_port;
    network_port_t	portrec_network_port;
    secure_info_t	portrec_secure_info;
    long		portrec_random;
    long		portrec_clock;
    pointer_t		portrec_po_host_list;	/* List of network servers for port ops module. */
    short		portrec_security_level;
    short		portrec_aliveness;
    long		portrec_retry_level;
    long		portrec_waiting_count;
    long		portrec_transit_count;
    sys_queue_head_t	portrec_out_ipcrec;
    pointer_t		portrec_lazy_ipcrec;
    pointer_t		portrec_reply_ipcrec;
    pointer_t		portrec_block_queue;	/* List of senders waiting if the port is blocked. */
    struct lock		portrec_lock;
    port_stat_ptr_t	portrec_stat;
} port_rec_t, *port_rec_ptr_t;

#define PORT_REC_NULL	(port_rec_ptr_t)0
#define PORT_REC_OWNER(port_rec_ptr) ((port_rec_ptr)->portrec_network_port.np_owner)
#define PORT_REC_RECEIVER(port_rec_ptr) ((port_rec_ptr)->portrec_network_port.np_receiver)


/*
 *  Values of portrec_info field.
 */
#define	PORT_INFO_SUSPENDED	0x1
#define PORT_INFO_DISABLED	0x2
#define PORT_INFO_BLOCKED	0x4
#define	PORT_INFO_PROBED	0x8
#define PORT_INFO_DEAD		0x10
#define PORT_INFO_ACTIVE	0x20
#define	PORT_INFO_NOLOOKUP	0x40

#define	PORT_BUSY(port_rec_ptr) (				\
	((port_rec_ptr->portrec_info) & 			\
		(PORT_INFO_SUSPENDED | PORT_INFO_BLOCKED)) ||	\
	(port_rec_ptr->portrec_transit_count > 0))		\


/*
 * Values of portrec_aliveness field.
 */
#define PORT_ACTIVE		2
#define PORT_INACTIVE		0

/*
 * Values for port access rights.
 * These are the same as the values in /usr/mach/include/sys/message.h
 */
#define PORT_OWNERSHIP_RIGHTS	3
#define PORT_RECEIVE_RIGHTS	4
#define PORT_ALL_RIGHTS		5
#define PORT_SEND_RIGHTS	6


/*
 * Port reference counts.
 *
 * Any entity that intends to use a port record after having released
 * the lock on it must make a reference to it before releasing that lock.
 * There is normally a single reference for all the lookup queues, one
 * reference for each ipc_rec pending on the destination port, and one
 * reference for the reply_ipcrec, if any (for simplicity, that last
 * reference is kept until the ipc_rec is deallocated, even if the ipc_rec
 * is unlinked from the port record long before that).
 *
 * The port lookup procedures do not create any extra references, but use
 * the one made when the port is created. This is acceptable since they
 * keep the port record locked on exit. The port record is protected from
 * early removal from the queues by holding the queue lock(s) while acquiring
 * the port record lock, and by checking for the PORT_INFO_NOLOOKUP flag when
 * this is not possible. To avoid deadlocks, all procedures must acquire queue
 * locks before port locks (and only one port lock should ever be held at
 * a time).
 */
#define	pr_reference(port_rec_ptr) {					\
	/* port_rec_ptr LOCK RW/RW */					\
	port_rec_ptr->portrec_refcount++;				\
	/* port_rec_ptr LOCK RW/RW */					\
}

#define	pr_release(port_rec_ptr) {					\
	/* port_rec_ptr LOCK RW/RW */					\
	if (--port_rec_ptr->portrec_refcount) {				\
		lk_unlock(&port_rec_ptr->portrec_lock);			\
		/* port_rec_ptr LOCK -/- */				\
	} else {							\
		lk_clear(&port_rec_ptr->portrec_lock);			\
		MEM_DEALLOCOBJ(port_rec_ptr, MEM_PORTREC);		\
	}								\
	/* port_rec_ptr LOCK -/- */					\
}

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_PORTREC;
extern mem_objrec_t		MEM_PORTITEM;


#endif	_PORT_DEFS_
