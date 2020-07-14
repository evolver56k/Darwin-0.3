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

/*
 * Port operations - used to manipulate information about port access rights.
 */


/*
 * Note: this code is augmented with comments indicating the lock status
 * of port records at various stages of the computation. The format is:
 *
 *	<port record ptr> LOCK <current lock>/<needed lock>
 *
 * where <current lock> and <needed lock> are one of "RW", "R" or "-".
 * A necessary condition for correctness is that:
 *
 *	<current lock> >= <needed lock>.
 */

#include <mach/boolean.h>

#ifndef WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "dispatcher.h"
#include "ipc.h"
#include "key_defs.h"
#include "lock_queue.h"
#include "ls_defs.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_extra.h"
#include "nn.h"
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "portsearch.h"
#include "po_defs.h"
#include "uid.h"

static struct lock_queue	po_transfers_in_progress;

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_POITEM;



/*
 * po_init
 *	Initialises the port operations module.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Side effects:
 *	Allocates the po_transfers_in_progress queue.
 *	Sets up our entries in the dispatcher switch.
 *	Calls po_notify_init to set up the po_notify thread.
 *
 */
EXPORT boolean_t po_init()
{

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_POITEM,"PO item",
			((sizeof(po_queue_t) > sizeof(po_host_info_t)) ? 
				sizeof(po_queue_t) : sizeof(po_host_info_t)),
			FALSE,200,50);

    /*
     * Set up the dispatcher entries.
     */
    dispatcher_switch[DISPE_PO_RO_HINT].disp_indata_simple = po_handle_ro_xfer_hint;
    dispatcher_switch[DISPE_PO_RO_XFER].disp_rr_simple = po_handle_ro_xfer_request;
    dispatcher_switch[DISPE_PO_RO_XFER].disp_indata_simple = po_handle_ro_xfer_reply;
    dispatcher_switch[DISPE_PO_TOKEN].disp_rr_simple = po_handle_token_request;
    dispatcher_switch[DISPE_PO_TOKEN].disp_indata_simple = po_handle_token_reply;
    dispatcher_switch[DISPE_PO_DEATH].disp_indata_simple = po_handle_nport_death;

    lq_init(&po_transfers_in_progress);

    po_notify_init();
    if (po_utils_init()) {
	RETURN(TRUE);
    }
    else {
	RETURN(FALSE);
    }

}



/*
 * po_check_ipc_seq_no
 *	Checks an incoming sequence number.
 *
 * Parameters:
 *	portrec_ptr	: the port record for the incoming message.
 *	host_id		: the host from which the sequence number was received.
 *	ipc_seq_no	: the sequence number to be checked.
 *
 * Results:
 *	TRUE	: the sequence number is ok.
 *	FALSE	: the sequence number is bad.
 *
 * Design:
 *	Look up the host_id in the po_host_list of the port record.
 *	If this host is found
 *	then check that the stored sequence number is less than the new one
 *		and update the sequence number,
 *	otherwise create a new entry for this host and store the sequence number.
 *
 * Note:
 *	the port record should be locked.
 *
 */
EXPORT boolean_t po_check_ipc_seq_no(portrec_ptr, host_id, ipc_seq_no)
port_rec_ptr_t	portrec_ptr;
netaddr_t	host_id;
unsigned long	ipc_seq_no;
{
    po_host_info_ptr_t	host_list;
    po_host_info_ptr_t	new_entry;

    host_list = (po_host_info_ptr_t)portrec_ptr->portrec_po_host_list;
    while (host_list != PO_HOST_INFO_NULL) {
	if (host_list->phi_host_id == host_id) {
	    if (host_list->phi_ipc_seq_no == 0) {
		/*
		 * This is the first sequence number that we have seen.
		 */
		host_list->phi_ipc_seq_no = ipc_seq_no;
		RETURN(TRUE);
	    }
	    else {
		/*
		 * Check the ipc sequence number.
		 */
		if (ipc_seq_no <= host_list->phi_ipc_seq_no) {
		    ERROR((msg, "po_check_ipc_seq_no: bad IPC sequence number, host = %x, ipc_seq_no = %x.",
				(unsigned)host_id, (unsigned)ipc_seq_no));
		    RETURN(FALSE);
		}
		else {
		    host_list->phi_ipc_seq_no = ipc_seq_no;
		    RETURN(TRUE);
		}
	    }
	}
	host_list = host_list->phi_next;
    }

    /*
     * If we got here then we know that there was no record for	this host
     * in the portrec_host_list and we must make a new entry for it.
     */
    MEM_ALLOCOBJ(new_entry,po_host_info_ptr_t,MEM_POITEM);
    new_entry->phi_host_id = host_id;
    new_entry->phi_sent_token = FALSE;
    new_entry->phi_ipc_seq_no = ipc_seq_no;
    new_entry->phi_next = (struct po_host_info *)portrec_ptr->portrec_po_host_list;
    portrec_ptr->portrec_po_host_list = (pointer_t)new_entry;

    RETURN(TRUE);

}



/*
 * po_test_client_id
 *	Returns TRUE if the client_id in the queue item matches the client_id parameter.
 */

PRIVATE int po_test_client_id(queue_item, client_id)
po_queue_ptr_t	queue_item;
int		client_id;
{

    RETURN(queue_item->poq_client_id == client_id);

}



/*
 * po_translate_lport_rights
 *	Translate access rights to a local port
 *	so that the right can be sent to a remote network server.
 *
 * Parameters:
 *	client_id	: the identity of the calling transaction;
 *			  this ID is remembered for use in po_port_rights_commit
 *	lport		: the local port to be translated
 *	right		: the right to be transferred
 *	security_level	: the security level of the message in which the port is being transferred
 *	destination_hint: what the IPC module currently thinks is the destination of the transfer
 *	port_data	: a pointer to a data area into which the data to
 *			  be transferred to the remote server is to be placed
 *
 * Results:
 *	the amount (in bytes) of data to be sent to the remote server
 *
 * Side effects:
 *	If receive or ownership rights are being transferred we remember the
 *	data that was transferred and associate it with the client_id for
 *	later use by po_port_rights_commit.
 *
 * Design:
 *	If this is a new network port and it is to be secure then create a RO key for it.
 *	Copy size, right and the network port into the port_data field.
 *	If the security level is not CRYPT_DONT_ENCRYPT then 
 *	   if receive or ownership rights are being transferred
 *		the RO key is included in the port data
 *	   else if send rights are being transferred and this server is the receiver or owner
 *		and we have not already sent the destination_hint network server a token
 *			then a token is included in the port data.
 *
 * Note:
 *	Whether we transfer a key/token, or even whether the transfer should be allowed
 *	must depend on the security level of both the port and the message.
 *	Currently I am only looking at the security of the message.
 *
 */

EXPORT int po_translate_lport_rights(client_id, lport, right, security_level, destination_hint, port_data)
int		client_id;
port_t		lport;
int		right;
int		security_level;
netaddr_t	destination_hint;
pointer_t	port_data;
{
    port_rec_ptr_t	port_rec_ptr;
    po_queue_ptr_t	client_rec_ptr;
    int			port_data_size;
    po_data_ptr_t	po_data_ptr;

    /*
     * Fill in default values for port data.
     */
    po_data_ptr = (po_data_ptr_t)port_data;
    po_data_ptr->pod_size = htons(2 * sizeof(short));
    po_data_ptr->pod_right = 0;

    if ((port_rec_ptr = pr_lportlookup(lport)) == PORT_REC_NULL) {
	if ((port_rec_ptr = pr_ltran(lport)) == PORT_REC_NULL) {
	    RETURN(4);
	}
	port_rec_ptr->portrec_security_level = security_level;
    }
    /* port_rec_ptr LOCK RW/R */

    /*
     * Check to see whether we really have the right that is being transferred.
     */
    switch (right) {
	case PORT_RECEIVE_RIGHTS: {
	    if (port_rec_ptr->portrec_network_port.np_receiver != my_host_id) {
		ERROR((msg, "po_translate_lport_rights: cannot send non-existent receive rights to %x.",
			lport));
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(4);
	    }
	    break;
	}
	case PORT_OWNERSHIP_RIGHTS: {
	    if (port_rec_ptr->portrec_network_port.np_owner != my_host_id) {
		ERROR((msg, "po_translate_lport_rights: cannot send non-existent ownership rights to %x.",
			lport));
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(4);
	    }
	    break;
	}
	case PORT_ALL_RIGHTS: {
	    if ((port_rec_ptr->portrec_network_port.np_receiver != my_host_id)
		|| (port_rec_ptr->portrec_network_port.np_owner != my_host_id))
	    {
		ERROR((msg, "po_translate_lport_rights: cannot send non-existent all rights to %x.", lport));
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(4);
	    }
	    break;
	}
	case PORT_SEND_RIGHTS: break;
	default: {
	    ERROR((msg, "po_translate_lport_rights: bogus rights %d to %x.", right, lport));
	    lk_unlock(&port_rec_ptr->portrec_lock);
	    RETURN(4);
	}
    }

    /*
     * Remember this transfer if receive or ownership rights are being transferred.
     */
    if ((right == PORT_RECEIVE_RIGHTS)
	|| (right == PORT_OWNERSHIP_RIGHTS)
	|| (right == PORT_ALL_RIGHTS))
    {
	MEM_ALLOCOBJ(client_rec_ptr,po_queue_ptr_t,MEM_POITEM);
	client_rec_ptr->poq_client_id = client_id;
	client_rec_ptr->poq_lport = lport;
	client_rec_ptr->poq_right = right;
	client_rec_ptr->poq_security_level = security_level;
	lq_enqueue(&po_transfers_in_progress, (cthread_queue_item_t)client_rec_ptr);
    }

    /*
     * Block the port if receive rights are being transferred.
     */
    if ((right == PORT_RECEIVE_RIGHTS) || (right == PORT_ALL_RIGHTS)) {
	port_rec_ptr->portrec_info |= PORT_INFO_BLOCKED;
    }

    /*
     * Start filling in the fields of port_data.
     */
    port_data_size = (2 * sizeof(short)) + sizeof(network_port_t);
    po_data_ptr->pod_right = htons((unsigned short)right);
    po_data_ptr->pod_nport = port_rec_ptr->portrec_network_port;

    /*
     * If this port is to be sent securely then we may have to send extra information.
     */
    po_data_ptr->pod_size = htons((unsigned short)port_data_size);
    switch (right) {
	case PORT_SEND_RIGHTS: INCPORTSTAT(port_rec_ptr, send_rights_sent); break;
	case PORT_RECEIVE_RIGHTS: INCPORTSTAT(port_rec_ptr, rcv_rights_xferd); break;
	case PORT_OWNERSHIP_RIGHTS: INCPORTSTAT(port_rec_ptr, own_rights_xferd); break;
	case PORT_ALL_RIGHTS: INCPORTSTAT(port_rec_ptr, all_rights_xferd); break;
    }
    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(port_data_size);

}



/*
 * po_port_rights_commit
 *	Notification that a message that transferred port rights has completed.
 *
 * Parameters:
 *	client_id	: the IPC identity of the message
 *	completion_code	: whether the message got through or not
 *	destination	: the network server to which the message was actually sent
 *
 * Side effects:
 *	Changes fields in various port records.
 *
 * Design:
 *	Look on the queue of transfers in progress to find ones with matching client ids.
 *	If the transfer was successful, process each of them according to the rights transferred:
 *		SEND: adds the destination to the list of network servers that have been sent tokens
 *		RECEIVE/OWNERSHIP: changes the network port accordingly
 *					and perhaps sends hint of transfer to owner/receiver.
 *
 */
EXPORT void po_port_rights_commit(client_id, completion_code, destination)
int		client_id;
int		completion_code;
netaddr_t	destination;
{
    port_rec_ptr_t	port_rec_ptr;
    po_queue_ptr_t	client_rec_ptr;

    if ((completion_code != PO_RIGHTS_XFER_SUCCESS)
	&& (completion_code != PO_RIGHTS_XFER_FAILURE))
    {
	ERROR((msg, "po_port_rights_commit: bogus completion code (%d).", completion_code));
	RET;
    }

    while ((client_rec_ptr
	= (po_queue_ptr_t)lq_cond_delete_from_queue(&po_transfers_in_progress,po_test_client_id,client_id))
		!= (po_queue_ptr_t)0)
    {
	/*
	 * Find the corresponding port record.
	 */
	if ((port_rec_ptr = pr_lportlookup(client_rec_ptr->poq_lport)) == PORT_REC_NULL) {
	    ERROR((msg, "po_port_rights_commit.pr_lportlookup fails, lport = %d.",
			client_rec_ptr->poq_lport));
	    MEM_DEALLOCOBJ(client_rec_ptr, MEM_POITEM);
	    continue;
	}
	/* port_rec_ptr LOCK RW/RW */

	if (completion_code == PO_RIGHTS_XFER_SUCCESS) {

	    if (client_rec_ptr->poq_right == PORT_SEND_RIGHTS) {
		/*
		 * Add the destination to the portrec_po_host_list.
		 */
		po_host_info_ptr_t	host_entry_ptr;
		MEM_ALLOCOBJ(host_entry_ptr,po_host_info_ptr_t,MEM_POITEM);
		host_entry_ptr->phi_host_id = destination;
		host_entry_ptr->phi_sent_token = TRUE;
		host_entry_ptr->phi_ipc_seq_no = 0;
		host_entry_ptr->phi_next = (struct po_host_info *)port_rec_ptr->portrec_po_host_list;
		port_rec_ptr->portrec_po_host_list = (pointer_t)host_entry_ptr;
	    }
	    else {
		/*
		 * Change the network port identifier to reflect the transfer.
		 */
		if ((client_rec_ptr->poq_right == PORT_RECEIVE_RIGHTS)
		    || (client_rec_ptr->poq_right == PORT_ALL_RIGHTS))
		{
		    port_rec_ptr->portrec_network_port.np_receiver = destination;
		    /*
		     * Unblock the port, add the port to the port set, and call ipc_retry or ipc_msg_accepted.
		     */
		    if (port_set_add(task_self(), nm_port_set, port_rec_ptr->portrec_local_port) != KERN_SUCCESS) {
			ERROR((msg, "po_port_rights_commit.port_enables fails."));
		    }
		    port_rec_ptr->portrec_info &= ~PORT_INFO_BLOCKED;
		    (void)ipc_retry(port_rec_ptr);
		    ipc_msg_accepted(port_rec_ptr);
		}

		if ((client_rec_ptr->poq_right == PORT_OWNERSHIP_RIGHTS)
		    || (client_rec_ptr->poq_right == PORT_ALL_RIGHTS))
		{
		    port_rec_ptr->portrec_network_port.np_owner = destination;
		}

		/*
		 * Inform the receiver or owner if the complementary rights were transferred.
		 */
		if ((client_rec_ptr->poq_right == PORT_RECEIVE_RIGHTS)
			&& (port_rec_ptr->portrec_network_port.np_owner != my_host_id))
		{
		    po_send_ro_xfer_hint(port_rec_ptr, port_rec_ptr->portrec_network_port.np_owner,
						client_rec_ptr->poq_security_level);
		}
		if ((client_rec_ptr->poq_right == PORT_OWNERSHIP_RIGHTS)
			&& (port_rec_ptr->portrec_network_port.np_receiver != my_host_id))
		{
		    po_send_ro_xfer_hint(port_rec_ptr, port_rec_ptr->portrec_network_port.np_receiver,
						client_rec_ptr->poq_security_level);
		}

		/*
		 * If we now have only send rights to this port, create a token for ourself.
		 */
		ipc_port_moved(port_rec_ptr);
	     }
	}
	else {
	    /*
	     * Rights transfer failed.
	     */
	    if (client_rec_ptr->poq_right == PORT_ALL_RIGHTS) {
		/*
		 * Better kill this port.
		 */
		po_notify_port_death(port_rec_ptr);
		ipc_port_dead(port_rec_ptr);
		nn_remove_entries(port_rec_ptr->portrec_local_port);
	    }
	    else if (client_rec_ptr->poq_right == PORT_RECEIVE_RIGHTS) {
		if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
		    /*
		     * Hand back receive rights to the local owner.
		     */
		    po_port_deallocate(port_rec_ptr->portrec_local_port);
		    port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
	            ipc_port_moved(port_rec_ptr);
		}
		else {
		    (void)po_xfer_receive(port_rec_ptr);
		}
	    }
	    else if (client_rec_ptr->poq_right == PORT_OWNERSHIP_RIGHTS) {
		if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
		    /*
		     * Hand back receive rights to the local owner.
		     */
		    po_port_deallocate(port_rec_ptr->portrec_local_port);
		    port_rec_ptr->portrec_network_port.np_owner = my_host_id;
	            ipc_port_moved(port_rec_ptr);
		}
		else {
		    (void)po_xfer_ownership(port_rec_ptr);
		}
	    }
	}
	MEM_DEALLOCOBJ(client_rec_ptr, MEM_POITEM);
	lk_unlock(&port_rec_ptr->portrec_lock);
	/* port_rec_ptr LOCK -/- */
    }
    RET;

}



/*
 * po_translate_nport_rights
 *	translate port rights received over the network
 *
 * Parameters:
 *	source		: the network server sending the rights
 *	port_data	: the data about the transfer
 *	security_level	: the security level of the message
 *	lport		: set to be the resulting local port
 *	right		: set to be the right that was transferred
 *
 * Results:
 *	the amount of data that was processed
 *
 * Side effects:
 *	May create a new port record.
 *	May change the information in an existing port record.
 *
 * Design:
 *	Does a pr_ntran to get a port record.
 *	If the transfer is secure then
 *		if send rights were received but no token, then request a token
 *		if receive or ownership rights were transferred, then check the RO key
 *
 * Note:
 *	We treat the incoming network port securely if the security_level is not CRYPT_DONT_ENCRYPT.
 *
 */
EXPORT int po_translate_nport_rights(source, port_data, security_level, lport, right)
netaddr_t	source;
pointer_t	port_data;
int		security_level;
port_t		*lport;
int		*right;
{
    int			port_data_size, port_right;
    port_rec_ptr_t	port_rec_ptr;
    boolean_t		new_port;
    po_data_ptr_t	po_data_ptr;

    po_data_ptr = (po_data_ptr_t)port_data;
    port_data_size = ntohs(po_data_ptr->pod_size);
    port_right = ntohs(po_data_ptr->pod_right);
    *right = port_right;
    if (port_right == 0) {
	/*
	 * This is a null port.
	 */
	*lport = PORT_NULL;
	RETURN(port_data_size);
    }
    else if (port_data_size < PO_MIN_NPD_ENTRY_SIZE) {
	ERROR((msg, "po_translate_nport_rights: not enough port data received (%d).", port_data_size));
	RETURN(port_data_size);
    }

    if ((port_rec_ptr = pr_nportlookup(&(po_data_ptr->pod_nport))) == PORT_REC_NULL) {
	if ((port_rec_ptr = pr_ntran(&(po_data_ptr->pod_nport))) == PORT_REC_NULL) {
	    ERROR((msg, "po_translate_nport_rights.pr_ntran fails."));
	    RETURN(port_data_size);
	}
	port_rec_ptr->portrec_security_level = security_level;
	new_port = TRUE;
    }
    else {
	new_port = FALSE;
	/*
	 * Check to see whether the rights that we received are sensible.
	 */
	switch(port_right) {
	    case PORT_SEND_RIGHTS: break;
	    case PORT_RECEIVE_RIGHTS: {
		if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
		    ERROR((msg, "po_translate_nport_rights: received receive rights to "));
		    ERROR((msg, "%d but already receiver.", port_rec_ptr->portrec_local_port));
		    lk_unlock(&port_rec_ptr->portrec_lock);
		    RETURN(port_data_size);
		}
		break;
	    }
	    case PORT_OWNERSHIP_RIGHTS: {
		if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
		    ERROR((msg, "po_translate_nport_rights: received ownership rights to "));
		    ERROR((msg, "%d but already owner.", port_rec_ptr->portrec_local_port));
		    lk_unlock(&port_rec_ptr->portrec_lock);
		    RETURN(port_data_size);
		}
		break;
	    }
	    case PORT_ALL_RIGHTS: {
		boolean_t bad = FALSE;
		if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
		    ERROR((msg, "po_translate_nport_rights: received all rights to "));
		    ERROR((msg, "%d but already receiver.", port_rec_ptr->portrec_local_port));
		    bad = TRUE;
		}
		if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
		    ERROR((msg, "po_translate_nport_rights: received all rights to "));
		    ERROR((msg, "%d but already owner.", port_rec_ptr->portrec_local_port));
		    bad = TRUE;
		}
		if (bad) {
		    lk_unlock(&port_rec_ptr->portrec_lock);
		    RETURN(port_data_size);
		}
		break;
	    }
	    default: {
		ERROR((msg, "po_translate_nport_rights: received bogus rights (%d).", port_right));
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(port_data_size);
/*		break; */
	    }
	}
    }

    /* port_rec_ptr LOCK RW/RW */
    *lport = port_rec_ptr->portrec_local_port;

#define PO_CHECK_PORT_DATA_SIZE(actual_size, required_size)			\
    if ((actual_size) < (required_size)) {					\
	ERROR((msg, "po_translate_nport_rights: not enough port data:"));	\
	ERROR((msg, " actual size = %d but required size = %d.",		\
			(actual_size), (required_size)));			\
	break;									\
    }

    /*
     * Act according to which rights we have received.
     */
    switch(port_right) {
	case PORT_SEND_RIGHTS: {

	    if ((source == PORT_REC_OWNER(port_rec_ptr)) || (source == PORT_REC_RECEIVER(port_rec_ptr))) {
		INCPORTSTAT(port_rec_ptr, send_rights_rcvd_recown);
	    }
	    else {
		INCPORTSTAT(port_rec_ptr, send_rights_rcvd_sender);
	    }
	    if (NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port)) {
		break;
	    }
	    if (new_port) break;

	    /*
	     * Check that the receiver/owner pair that we received match what we have.
	     */
	    if ((port_rec_ptr->portrec_network_port.np_receiver != po_data_ptr->pod_nport.np_receiver)
		|| (port_rec_ptr->portrec_network_port.np_owner != po_data_ptr->pod_nport.np_owner))
	    {
		if ((source == port_rec_ptr->portrec_network_port.np_receiver)
		    || (source == port_rec_ptr->portrec_network_port.np_owner)) 
		{
		    /*
		     * Believe the (old) owner/receiver.
		     */
		    port_rec_ptr->portrec_network_port.np_receiver = po_data_ptr->pod_nport.np_receiver;
		    port_rec_ptr->portrec_network_port.np_owner = po_data_ptr->pod_nport.np_owner;
	            ipc_port_moved(port_rec_ptr);
		}
		else {
		    ERROR((msg, "po_translate_nport_rights: received conflicting information."));
		    ps_do_port_search(port_rec_ptr, TRUE, &(po_data_ptr->pod_nport), (int(*)())0);
		}
	    }
	    break;
	}

	case PORT_OWNERSHIP_RIGHTS: {
	    INCPORTSTAT(port_rec_ptr, own_rights_xferd);
	    if (new_port) {
		/*
		 * Can only believe what we have been given!
		 */
		port_rec_ptr->portrec_network_port.np_owner = my_host_id;
		break;
	    }
	    /*
	     * Adjust the network port in the record to reflect the rights received.
	     * Check to see whether the information about the receiver matches what we know.
	     */
	    port_rec_ptr->portrec_network_port.np_owner = my_host_id;
	    if (port_rec_ptr->portrec_network_port.np_receiver != po_data_ptr->pod_nport.np_receiver) {
		if (source == port_rec_ptr->portrec_network_port.np_receiver) {
		    /*
		     * Believe the old receiver.
		     */
		    port_rec_ptr->portrec_network_port.np_receiver = po_data_ptr->pod_nport.np_receiver;
	            ipc_port_moved(port_rec_ptr);
		}
		else {
		    ERROR((msg, "po_translate_nport_rights: received conflicting information."));
		    ps_do_port_search(port_rec_ptr, TRUE, &(po_data_ptr->pod_nport), (int(*)())0);
		}
	    }
	    break;
	}

	case PORT_RECEIVE_RIGHTS: {
	    INCPORTSTAT(port_rec_ptr, rcv_rights_xferd);
	    if (new_port) {
		/*
		 * Can only believe what we have been given!
		 */
		port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
		break;
	    }
	    /*
	     * Adjust the network port in the record to reflect the rights received.
	     * Check to see whether the information about the receiver matches what we know.
	     */
	    port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
	    if (port_rec_ptr->portrec_network_port.np_owner != po_data_ptr->pod_nport.np_owner) {
		if (source == port_rec_ptr->portrec_network_port.np_owner) {
		    /*
		     * Believe the old owner.
		     */
		    port_rec_ptr->portrec_network_port.np_owner = po_data_ptr->pod_nport.np_owner;
	            ipc_port_moved(port_rec_ptr);
		}
		else {
		    ERROR((msg, "po_translate_nport_rights: received conflicting information."));
		    ps_do_port_search(port_rec_ptr, TRUE, &(po_data_ptr->pod_nport), (int(*)())0);
		}
	    }
	    break;
	}

	case PORT_ALL_RIGHTS: {
	    INCPORTSTAT(port_rec_ptr, all_rights_xferd);
	    if (new_port) {
		/*
		 * Can only believe what we have been given!
		 */
		port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
		port_rec_ptr->portrec_network_port.np_owner = my_host_id;
		break;
	    }
	    /*
	     * Adjust the network port in the record to reflect the rights received.
	     */
	    port_rec_ptr->portrec_network_port.np_owner = my_host_id;
	    port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
	            ipc_port_moved(port_rec_ptr);
	    break;
	}
    }

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(port_data_size);

}

