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

#include	<mach/cthreads.h>
#include	<mach/mach.h>
#include	<stdio.h>
#include	<mach/boolean.h>

#ifndef WIN32
#include	<sys/time.h>
#endif

#include	"debug.h"
#include	"lock_queue.h"
#include	"ls_defs.h"
#include 	"mem.h"
#include	"netmsg.h"
#include	"network.h"
#include	"nm_extra.h"
#include	"port_defs.h"
#include	"portrec.h"
#include	"uid.h"
#include	"ipc.h"
#include	"po_defs.h"	/* MEM_POITEM */


#define	PORT_HASH_SIZE	32	/* Must be a multiple of 2. */
static struct lock_queue	port_rec_queue[PORT_HASH_SIZE];

int total_local_ports = 0;
int running_local_ports = 0;
int total_network_ports = 0;
int running_network_ports = 0;

network_port_t		null_network_port;

PRIVATE long		unique_id;
PRIVATE struct mutex	unique_id_lock;

extern int gettimeofday ();

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_PORTREC;
PUBLIC mem_objrec_t	MEM_PORTITEM;



/*
 * PR_LPORTHASHFORIDX(lport)
 *	Find the bucket for this local port according to some hash algorithm.
 */
#define PR_LPORTHASHFORIDX(lport) \
	(((((lport) >> 8) & 0xff) ^ ((lport) & 0xff)) & (PORT_HASH_SIZE - 1))


/*
 * PR_NPORTHASHFORIDX(nport_ptr)
 * PR_NPORTHASHFORPUIDX(puid)
 *	Find the bucket for this network port according to some hash algorithm.
 *	We hash on the top and the bottom byte of the low word of the PUID
 *	because these are the bytes most likely to change because they are
 *	generated from the unique id value.
 */
#define PR_NPORTHASHFORIDX(nport_ptr) 								\
	(((((nport_ptr)->np_puid.np_uid_low >> 24) & 0xff)					\
			^ ((nport_ptr)->np_puid.np_uid_low & 0xff)) & (PORT_HASH_SIZE - 1))

#define PR_NPORTHASHFORPUIDX(puid)								\
	(((((puid).np_uid_low >> 24) & 0xff) ^ ((puid).np_uid_low & 0xff)) & (PORT_HASH_SIZE - 1))


/*
 * pr_eqfn
 *	See if the two port records of two port hash records are the same (equal addresses).
 *
 * Parameters:
 *	pqh_1, pqh_2	: pointers to the two hash records.
 */
PRIVATE int pr_eqfn(pqh_1, pqh_2)
register cthread_queue_item_t	pqh_1;
register int		pqh_2;
{

	RETURN(((pq_hash_ptr_t)pqh_1)->pqh_portrec == ((pq_hash_ptr_t)pqh_2)->pqh_portrec);

}


/*
 * lport_test
 *	Test if lport is the same as the local port
 *	in the port record pointed to by this hash record.
 *
 * Parameters:
 *	pqh	: pointer to a hash record for the port record
 *	lport	: the local port
 */
PRIVATE int lport_test(pqh, lport)
register cthread_queue_item_t		pqh;
register int			lport;
{
	RETURN(((pq_hash_ptr_t)pqh)->pqh_portrec->portrec_local_port == (port_t)lport);
}


/*
 * nport_test
 *	Test to see if this network port is the same as the network port
 *	in the port record pointed to by this hash record.
 *
 * Parameters:
 *	pqh		: pointer to the port hash record
 *	nport_ptr	: pointer to the network port
 */
PRIVATE int nport_test(pqh, nport_ptr)
register cthread_queue_item_t		pqh;
register int			nport_ptr;
{
	register network_port_ptr_t	other_nport_ptr;

	other_nport_ptr = &((pq_hash_ptr_t)pqh)->pqh_portrec->portrec_network_port;
	RETURN(
		(((network_port_ptr_t)nport_ptr)->np_puid.np_uid_high
						== other_nport_ptr->np_puid.np_uid_high)
		&&
		(((network_port_ptr_t)nport_ptr)->np_puid.np_uid_low == other_nport_ptr->np_puid.np_uid_low)
		&&
		(((network_port_ptr_t)nport_ptr)->np_sid.np_uid_low == other_nport_ptr->np_sid.np_uid_low)
		&&
		(((network_port_ptr_t)nport_ptr)->np_sid.np_uid_low == other_nport_ptr->np_sid.np_uid_low)
	);
}

/*
 * np_puid_test
 *	Test to see if this PUID is equal to the PUID in the port record.
 *
 * Parameters:
 *	pqh		: pointer to the port hash record
 *	puid_ptr	: pointer to the PUID
 *
 */
PRIVATE int np_puid_test(pqh, puid_ptr)
register cthread_queue_item_t pqh;
register np_uid_t * puid_ptr;
{
	RETURN((((pq_hash_ptr_t)pqh)->pqh_portrec->portrec_network_port.np_puid.np_uid_high
						== puid_ptr->np_uid_high)
		&& (((pq_hash_ptr_t)pqh)->pqh_portrec->portrec_network_port.np_puid.np_uid_low
						== puid_ptr->np_uid_low));
}



/*
 * pr_init()
 *	Initialize variables:
 *		the unique identifier used to construct network port PUIDs
 *		the hash queues
 */
EXPORT boolean_t
pr_init()
{
	register int	i;
	struct timeval	now;

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_PORTREC,"Port record",sizeof(port_rec_t),
								FALSE,100,10);
	mem_initobj(&MEM_PORTITEM,"Port item",sizeof(port_item_t),
								FALSE,500,50);

	(void)gettimeofday(&now, NULL);
	unique_id = now.tv_sec;

	mutex_init(&unique_id_lock);

	for (i = 0; i < PORT_HASH_SIZE; ++i)
		lq_init(&port_rec_queue[i]);

	RETURN(TRUE);
}



/*
 * pr_create
 *	Create a new port record.
 *	Put it on the hash tables and allocate lock and condition variables.
 *
 * Parameters:
 *	lport		: the local port
 *	nport_ptr	: pointer to the network port
 *
 * Returns:
 *	Pointer to the new record locked.
 *
 */
PRIVATE port_rec_ptr_t
pr_create(lport, nport_ptr)
port_t			lport;
network_port_ptr_t	nport_ptr;
{
	register port_rec_ptr_t	newport_rec;
	register int		networkidx, localidx;

	MEM_ALLOCOBJ(newport_rec,port_rec_ptr_t,MEM_PORTREC);
	memset(newport_rec, 0, sizeof(port_rec_t));
	newport_rec->portrec_local_port = lport;
	newport_rec->portrec_network_port = *nport_ptr;

	localidx = PR_LPORTHASHFORIDX(lport);
	networkidx = PR_NPORTHASHFORIDX(nport_ptr);

	lk_init(&newport_rec->portrec_lock);

	newport_rec->portrec_refcount = 1;

	newport_rec->portrec_retry_level = 0;
	newport_rec->portrec_waiting_count = 0;
	newport_rec->portrec_transit_count = 0;
	sys_queue_init(&newport_rec->portrec_out_ipcrec);
	newport_rec->portrec_lazy_ipcrec = (pointer_t)0;
	newport_rec->portrec_reply_ipcrec = (pointer_t)0;
	newport_rec->portrec_block_queue = (pointer_t)0;

	newport_rec->portrec_networkitem.pqh_portrec = newport_rec;
	newport_rec->portrec_localitem.pqh_portrec = newport_rec;

	newport_rec->portrec_aliveness = PORT_ACTIVE;

	if (networkidx > localidx) {
		mutex_lock(&port_rec_queue[networkidx].lock);
		mutex_lock(&port_rec_queue[localidx].lock);
	} else {
		mutex_lock(&port_rec_queue[localidx].lock);
		if (networkidx != localidx) {
			mutex_lock(&port_rec_queue[networkidx].lock);
		}
	}

	lqn_prequeue(&port_rec_queue[networkidx], (cthread_queue_item_t) &newport_rec->portrec_networkitem);
	lqn_prequeue(&port_rec_queue[localidx], (cthread_queue_item_t) &newport_rec->portrec_localitem);

	lk_lock(&newport_rec->portrec_lock, PERM_READWRITE, BLOCK);

	mutex_unlock(&port_rec_queue[networkidx].lock);
	if (networkidx != localidx) {
		mutex_unlock(&port_rec_queue[localidx].lock);
	}

	RETURN(newport_rec);
}



/*
 * pr_destroy
 *	Destroy a port record.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to the port record to be destroyed
 *
 * Assumes:
 *	PERM_READWRITE has already been acquired.
 *
 */
EXPORT void
pr_destroy(port_rec_ptr)
register port_rec_ptr_t	port_rec_ptr;
{
	register int	networkidx, localidx;
	pointer_t	temp_ptr, next_ptr;
	cthread_queue_item_t	network_ret, local_ret;

	if (port_rec_ptr == PORT_REC_NULL)
		RET;

	if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
		/*
		 * Some other thread is in the process of destroying
		 * this port record.
		 */
		lk_unlock(&port_rec_ptr->portrec_lock);
		RET;
	}
	port_rec_ptr->portrec_info |= PORT_INFO_NOLOOKUP;

	localidx = PR_LPORTHASHFORIDX(port_rec_ptr->portrec_local_port);
	networkidx = PR_NPORTHASHFORIDX(&(port_rec_ptr->portrec_network_port));

	(void)port_deallocate(task_self(), port_rec_ptr->portrec_local_port);
	port_rec_ptr->portrec_local_port = PORT_NULL;

	/*
	 * Deallocate the portrec_po_host_list.
	 */
	temp_ptr = port_rec_ptr->portrec_po_host_list;
	while (temp_ptr != (pointer_t)0) {
		next_ptr = *(pointer_t *)temp_ptr;
		MEM_DEALLOCOBJ(temp_ptr, MEM_POITEM);
		temp_ptr = next_ptr;
	}
	port_rec_ptr->portrec_po_host_list = (pointer_t)NULL;

	/*
	 * At this point, we are still holding one reference in the port record.
	 * The NOLOOKUP flag guarantees that nobody else will be able to
	 * lookup the port record and use that same reference. All other threads
	 * that still use this port record have their own reference for it.
	 */

	lk_unlock(&port_rec_ptr->portrec_lock);

	if (networkidx > localidx) {
		mutex_lock(&port_rec_queue[networkidx].lock);
		mutex_lock(&port_rec_queue[localidx].lock);
	} else {
		mutex_lock(&port_rec_queue[localidx].lock);
		if (networkidx != localidx) {
			mutex_lock(&port_rec_queue[networkidx].lock);
		}
	}

	lk_lock(&port_rec_ptr->portrec_lock,PERM_READWRITE,TRUE);

	lqn_cond_delete_macro(&port_rec_queue[localidx], pr_eqfn,
				(int)&port_rec_ptr->portrec_localitem,local_ret);
	lqn_cond_delete_macro(&port_rec_queue[networkidx], pr_eqfn,
				(int)&port_rec_ptr->portrec_networkitem,network_ret);

	mutex_unlock(&port_rec_queue[networkidx].lock);
	if (networkidx != localidx) {
		mutex_unlock(&port_rec_queue[localidx].lock);
	}

	pr_release(port_rec_ptr);

	RET;

}



/*
 * pr_np_puid_lookup
 *	Look up a network port given its PUID.
 *
 * Parameters:
 *	np_puid		: the PUID for the network port.
 *
 * Returns:
 *	PORT_REC_NULL if no such port record exists.
 *	otherwise pointer to the appropriate port record.
 *	Locks the port record.
 *
 */
EXPORT port_rec_ptr_t pr_np_puid_lookup(np_puid)
np_uid_t	np_puid;
{
	register int		idx;
	register port_rec_ptr_t	port_rec_ptr;
	register cthread_queue_item_t	p;

	idx = PR_NPORTHASHFORPUIDX(np_puid);

	mutex_lock(&port_rec_queue[idx].lock);

/*	lqn_find_macro(&port_rec_queue[idx], np_puid_test, (int)&np_puid, p); */

	for (p = port_rec_queue[idx].head; p != NULL; p = p->next)
	{
		if (np_puid_test(p, &np_puid)) break;
	}

	if (p == (cthread_queue_item_t)NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		RETURN(PORT_REC_NULL);
	}

	if ((port_rec_ptr = ((pq_hash_ptr_t)p)->pqh_portrec) == PORT_REC_NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		panic("pr_np_puid_lookup");
	}

	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, BLOCK);

	if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
		lk_unlock(&port_rec_ptr->portrec_lock);
		port_rec_ptr = NULL;
	}

	mutex_unlock(&port_rec_queue[idx].lock);

	RETURN(port_rec_ptr);
}



/*
 * pr_nportlookup
 *	Look up a network port.
 *
 * Parameters:
 *	nport_ptr	: pointer to the network port
 *
 * Returns:
 *	PORT_REC_NULL if no such port record exists.
 *	otherwise pointer to the appropriate port record.
 *	Locks the port record.
 *
 */
EXPORT port_rec_ptr_t pr_nportlookup(nport_ptr)
register network_port_ptr_t	nport_ptr;
{
	register int		idx;
	register port_rec_ptr_t	port_rec_ptr;
	register cthread_queue_item_t	p;

	idx = PR_NPORTHASHFORIDX(nport_ptr);

	mutex_lock(&port_rec_queue[idx].lock);

	lqn_find_macro(&port_rec_queue[idx], nport_test, (int)nport_ptr, p);

	if (p == (cthread_queue_item_t)NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		RETURN(PORT_REC_NULL);
	}

	if ((port_rec_ptr = ((pq_hash_ptr_t)p)->pqh_portrec) == PORT_REC_NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		panic("pr_nportlookup");
	}

	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, BLOCK);

	if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
		lk_unlock(&port_rec_ptr->portrec_lock);
		port_rec_ptr = NULL;
	}

	mutex_unlock(&port_rec_queue[idx].lock);

	RETURN(port_rec_ptr);
}



/*
 * pr_ntran
 *	Return the port record associated with a network port.
 *	If no such record exists, create a new one.
 *
 * Parameters:
 *	nport_ptr	: pointer to the network port
 *
 * Returns:
 *	Pointer to the (new) record locked (by pr_create or pr_nportlookup).
 *
 */
EXPORT port_rec_ptr_t pr_ntran(nport_ptr)
register network_port_ptr_t	nport_ptr;
{
	port_t			lport;
	kern_return_t		kr;
	register int		idx;
	register port_rec_ptr_t	port_rec_ptr;
	register cthread_queue_item_t	p;

	if (nport_ptr == NULL) RETURN(PORT_REC_NULL);
	if ((nport_ptr->np_puid.np_uid_high == null_network_port.np_puid.np_uid_high)
		&& (nport_ptr->np_puid.np_uid_low == null_network_port.np_puid.np_uid_low)
		&& (nport_ptr->np_sid.np_uid_low == null_network_port.np_sid.np_uid_low)
		&& (nport_ptr->np_sid.np_uid_low == null_network_port.np_sid.np_uid_low))
	{
		RETURN(PORT_REC_NULL);
	}

	idx = PR_NPORTHASHFORIDX(nport_ptr);

	mutex_lock(&port_rec_queue[idx].lock);

	lqn_find_macro(&port_rec_queue[idx], nport_test, (int)nport_ptr, p);
	if (p == (cthread_queue_item_t)NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);

		/*
		 * Allocate a new local port and create a port record.
		 */
		if ((kr = port_allocate(task_self(), &lport)) != KERN_SUCCESS) {
			ERROR((msg, "pr_ntran.port_allocate fails, kr = %d.", kr));
			RETURN(PORT_REC_NULL);
		}
		(void) port_set_add(task_self(), nm_port_set, lport);
		port_rec_ptr = pr_create(lport, nport_ptr);

		port_rec_ptr->portrec_info = 0;
		total_network_ports++;
		running_network_ports++;
	}
	else if ((port_rec_ptr = ((pq_hash_ptr_t)p)->pqh_portrec) == PORT_REC_NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		panic("pr_nportlookup");
	}
	else {
		lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, BLOCK);
		mutex_unlock(&port_rec_queue[idx].lock);
		if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
			lk_unlock(&port_rec_ptr->portrec_lock);
			panic("pr_ntran trying to resuscitate a port");
		}
	}

	RETURN(port_rec_ptr);
}



/*
 * pr_lportlookup
 *	Look up a local port.
 *
 * Parameters:
 *	lport	: the local port
 *
 * Returns:
 *	PORT_REC_NULL if no such port record exists.
 *	otherwise pointer to the appropriate port record.
 *	Locks the port record.
 *
 */
EXPORT port_rec_ptr_t pr_lportlookup(lport)
register port_t	lport;
{
	register int		idx;
	register port_rec_ptr_t	port_rec_ptr;
	register cthread_queue_item_t	p;

	idx = PR_LPORTHASHFORIDX(lport);

	mutex_lock(&port_rec_queue[idx].lock);

	lqn_find_macro(&port_rec_queue[idx], lport_test, (int) lport, p);
	if (p == (cthread_queue_item_t)NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		RETURN(PORT_REC_NULL);
	}

	if ((port_rec_ptr = ((pq_hash_ptr_t)p)->pqh_portrec) == NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		panic("pr_lportlookup");
	}

	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, BLOCK);

	if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
		lk_unlock(&port_rec_ptr->portrec_lock);
		port_rec_ptr = NULL;
	}

	mutex_unlock(&port_rec_queue[idx].lock);

	RETURN(port_rec_ptr);
}



/*
 * pr_ltran
 *	Return the port record associated with a local port.
 *	If no such port record exists, create one.
 *
 * Parameters:
 *	lport	: the local port
 *
 * Returns:
 *	Pointer to the (new) port record locked by (pr_create or pr_lportlookup).
 */
EXPORT port_rec_ptr_t pr_ltran(lport)
register port_t	lport;
{
	network_port_t		new_nport;
	register int		idx;
	register port_rec_ptr_t	port_rec_ptr;
	register cthread_queue_item_t	p;

	if (lport == PORT_NULL) RETURN(PORT_REC_NULL);

	idx = PR_LPORTHASHFORIDX(lport);
	mutex_lock(&port_rec_queue[idx].lock);
	lqn_find_macro(&port_rec_queue[idx], lport_test, (int)lport, p);

	if (p == (cthread_queue_item_t)NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		/*
		 * Create a new network port and port record.
		 */
		new_nport.np_owner = my_host_id;
		new_nport.np_receiver = my_host_id;
		new_nport.np_puid.np_uid_high = my_host_id;
		mutex_lock(&unique_id_lock);
		new_nport.np_puid.np_uid_low = ++unique_id;
		mutex_unlock(&unique_id_lock);
		new_nport.np_sid.np_uid_high = uid_get_new_uid();
		new_nport.np_sid.np_uid_low = uid_get_new_uid();

		port_rec_ptr = pr_create(lport, (network_port_ptr_t)&new_nport);

		port_rec_ptr->portrec_info = 0;

		total_local_ports++;
		running_local_ports++;
	}
	else if ((port_rec_ptr = ((pq_hash_ptr_t)p)->pqh_portrec) == PORT_REC_NULL) {
		mutex_unlock(&port_rec_queue[idx].lock);
		panic("pr_lportlookup");
	}
	else {
		lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, BLOCK);
		mutex_unlock(&port_rec_queue[idx].lock);
		if (port_rec_ptr->portrec_info & PORT_INFO_NOLOOKUP) {
			lk_unlock(&port_rec_ptr->portrec_lock);
			panic("pr_ltran trying to resuscitate a port");
		}
	}

	RETURN(port_rec_ptr);
}




/*
 * pr_nport_equal
 *	Check for equality of two network ports.
 *
 * Parameters:
 *	nport_ptr1, nport_ptr2	: pointers to the two network ports
 *
 * Returns:
 *	TRUE if the SIDs and PUIDs of the network ports are equal.
 *	FALSE otherwise.
 *
 */
EXPORT boolean_t pr_nport_equal(nport_ptr1, nport_ptr2)
register network_port_ptr_t	nport_ptr1, nport_ptr2;
{
	RETURN
	(
		(nport_ptr1->np_puid.np_uid_high == nport_ptr2->np_puid.np_uid_high)
		&&
		(nport_ptr1->np_puid.np_uid_low == nport_ptr2->np_puid.np_uid_low)
		&&
		(nport_ptr1->np_sid.np_uid_low == nport_ptr2->np_sid.np_uid_low)
		&&
		(nport_ptr1->np_sid.np_uid_low == nport_ptr2->np_sid.np_uid_low)
	);
}



/*
 * pr_nporttostring
 *	Print contents of the network port into a string.
 *
 * Parameters:
 *	nport_str	: char * pointer
 *	nport_ptr	: pointer to the network port
 */
EXPORT void pr_nporttostring(nport_str, nport_ptr)
char			*nport_str;
network_port_ptr_t	nport_ptr;
{
	char		receiver_string[30];
	char		owner_string[30];

	ipaddr_to_string(receiver_string, nport_ptr->np_receiver);
	ipaddr_to_string(owner_string, nport_ptr->np_owner);
	(void)sprintf(nport_str, "%d.%d-%d.%d-%s-%s",
			(int)nport_ptr->np_puid.np_uid_high, (int)nport_ptr->np_puid.np_uid_low,
			(int)nport_ptr->np_sid.np_uid_high, (int)nport_ptr->np_sid.np_uid_low,
			receiver_string, owner_string);
	RET;

}



/*
 * pr_list_subfn
 *	Do gathering for pr_list.
 *
 * Parameters:
 *	p	: pointer to a hash entry
 *	lq	: lock queue that is being used to form a port list
 *
 * Note:
 *	The value returned is ignored.
 */

PRIVATE int pr_list_subfn(p, lq)
register cthread_queue_item_t	p;
register int		lq;
{
	register port_item_ptr_t	portitem;

	/*
	 * Only add this port to the list if the hash entry is the network hash entry.
	 * This stops us from adding the port twice.
	 */
	if ((&((pq_hash_ptr_t)p)->pqh_portrec->portrec_networkitem) != (pq_hash_ptr_t)p)
		RETURN(1);

	MEM_ALLOCOBJ(portitem,port_item_ptr_t,MEM_PORTITEM);
	portitem->pi_port = ((pq_hash_ptr_t)p)->pqh_portrec->portrec_local_port;

	lq_prequeue((lock_queue_t)lq, (cthread_queue_item_t)portitem);
	RETURN(1);
}


/*
 * pr_list()
 *	Return a list of all local ports that are elements of port records.
 */
EXPORT lock_queue_t pr_list()
{
	register lock_queue_t	portlist;
	register int		index;

	portlist = lq_alloc();

	for (index = 0; index < PORT_HASH_SIZE; index++) {
		/*
		 * Avoid call lq_map_queue if there is nothing there.
		 */
		if (port_rec_queue[index].head != (cthread_queue_item_t)0) {
			lq_map_macro(&port_rec_queue[index], pr_list_subfn, (int)portlist);
		}
	}

	RETURN(portlist);
}

