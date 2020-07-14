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

#ifndef WIN32
#include <netdb.h>

#ifdef __svr4__
#include <string.h>
#else
#include <strings.h>
#endif

#endif

#include <stdio.h>
#include <mach/message.h>

#include "debug.h"
#include "dispatcher.h"
#include "lock_queue.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_extra.h"
#include "nn_defs.h"
#include "port_defs.h"
#include "transport.h"

struct lock_queue	nn_table[NN_TABLE_SIZE];


/*
 * nn_procs_init
 *	Initialise the local name hash table.
 *	Initialise the dispatcher with our request/response handling functions.
 *
 */
PUBLIC void nn_procs_init()
{
    int	i;

    for (i = 0; i < NN_TABLE_SIZE; i++) {
	lq_init(&nn_table[i]);
    }

    dispatcher_switch[DISPE_NETNAME].disp_rr_simple = nn_handle_request;
    dispatcher_switch[DISPE_NETNAME].disp_indata_simple = nn_handle_reply;

    RET;
}


/*
 * nn_name_test
 *	test to see if the name in an entry is equal to the input name
 *
 *  Parameters
 *	q_item	: the entry on the queue
 *	name	: the input name
 *
 */
PUBLIC int nn_name_test(q_item, name)
register lock_queue_t	q_item;
register int		name;
{

    RETURN((strcmp(((nn_entry_ptr_t)q_item)->nne_name, (char *)name)) == 0);

}


/*
 * _netname_check_in
 *	Performs a local name check in.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	port_name	: the name to be checked in
 * 	signature	: a port protecting this entry
 *	port_id		: the port associated with the name
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_YOURS	: this name is already checked in but with a different signature
 *
 * Design:
 *	See if this name has already been entered in the name table.
 *	If is has not, then just enter it.
 *	If it has, then replace the old entry if the signatures match.
 *
 */
PUBLIC int _netname_check_in(ServPort, port_name, signature, port_id)
port_t		ServPort;
netname_name_t	port_name;
port_t		signature;
port_t		port_id;
{
    int			hash_index;
    nn_entry_ptr_t	name_entry_ptr;
    int			still_exists = FALSE;

    /*
     * If a port is deallocated before the name service request
     * is received, it will appear as PORT_NULL.
     */
    if (port_id == PORT_NULL) {
	RETURN(NETNAME_INVALID_PORT);
    }


    /*
     * Convert the name to upper case and look it up in the name table.
     */
    NN_CONVERT_TO_UPPER(port_name);
    NN_NAME_HASH(hash_index, port_name);
    name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index], nn_name_test, (int)port_name);

    if (name_entry_ptr) {
	port_type_t portType;
	kern_return_t result = KERN_SUCCESS;
	
	/* 
	 * Overwrite if the signature matches.
	 */
	if (signature == name_entry_ptr->nne_signature) { 
	    name_entry_ptr->nne_port = port_id;
	    RETURN(NETNAME_SUCCESS);
	}
	/* 
	 * Attempt to overwrite if the port is no longer valid.
	 */
	else if ((result = port_type(task_self(),name_entry_ptr->nne_port,
		&portType) != KERN_SUCCESS)) { 
	    
	    /* 
	     * This will return FALSE if the cleanup routine already removed
	     * the entry and TRUE if it still exists and we have it. 
	     */
	    still_exists = lq_remove_from_queue(&nn_table[hash_index],
				(cthread_queue_item_t)name_entry_ptr);
	    /* 
	     * It's still there, so modify it and then put it back.
	     */
	    if (still_exists) {
		name_entry_ptr->nne_port = port_id;
		name_entry_ptr->nne_signature = signature;
		lq_enqueue(&nn_table[hash_index],
			(cthread_queue_item_t)name_entry_ptr);
		RETURN(NETNAME_SUCCESS);
	    }
	}
	else {
	    RETURN(NETNAME_NOT_YOURS);
	}
    }
    if (!name_entry_ptr || !still_exists) {
	/*
	 * Make a new name entry and add it into the name table.
	 */
	MEM_ALLOCOBJ(name_entry_ptr,nn_entry_ptr_t,MEM_NNREC);
	(void)strncpy(name_entry_ptr->nne_name, port_name, sizeof(netname_name_t)-1);
	name_entry_ptr->nne_name[sizeof(netname_name_t)-1] = '\0';

	name_entry_ptr->nne_port = port_id;
	name_entry_ptr->nne_signature = signature;
	lq_enqueue(&nn_table[hash_index],(cthread_queue_item_t)name_entry_ptr);
    }
    RETURN(NETNAME_SUCCESS);
}


/*
 * _netname_check_out
 *	Checks out a name that was previously checked in.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	port_name	: the name to be checked out
 * 	signature	: a port protecting this entry
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_CHECKED_IN	: this name is not checked in
 *	NETNAME_NOT_YOURS	: this name is checked in but with a different signature
 *
 * Design:
 *	Check to see if this name has been entered into the local name table.
 *	If it has and the input signature makes the signature in the entry then remove the entry.
 *
 */
PUBLIC int _netname_check_out(ServPort, port_name, signature)
port_t		ServPort;
netname_name_t	port_name;
port_t		signature;
{
    int			hash_index;
    nn_entry_ptr_t	name_entry_ptr;

    /*
     * Convert the name to upper case and look it up in the name table.
     */
    NN_CONVERT_TO_UPPER(port_name);
    NN_NAME_HASH(hash_index, port_name);
    name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index], nn_name_test, (int)port_name);

    if (name_entry_ptr == (nn_entry_ptr_t)0) {
	RETURN(NETNAME_NOT_CHECKED_IN);
    }
    else {
	if (signature != name_entry_ptr->nne_signature) {
	    RETURN(NETNAME_NOT_YOURS);
	}
	else {
	    /*
	     * Remove the entry from the queue of hashed entries and deallocate it.
	     */
	    (void)lq_remove_from_queue(&nn_table[hash_index], (cthread_queue_item_t)name_entry_ptr);
	    MEM_DEALLOCOBJ(name_entry_ptr, MEM_NNREC);
	    RETURN(NETNAME_SUCCESS);
	}
    }
}


/*
 * nn_host_address
 *	returns a host address for a given host name.
 *
 */
PRIVATE netaddr_t nn_host_address(host_name)
char *host_name;
{
    register struct hostent *hp;

    extern char my_host_name[];

#ifdef WIN32
    if (!param.conf_network) {
        RETURN(INADDR_ANY); /* Assume what we are looking up is on the local host. */
    }
#endif
    /*
     * Shortcut to bypass lookup services
     */
    if (strcmp(host_name, my_host_name) == 0) {
	    return (my_host_id);
    }
    if ((hp = gethostbyname(host_name)) == 0) {
	RETURN(0);
    }
    else {
	RETURN(*(long *)(hp->h_addr_list[0]));
    }
}


/*
 * _netname_look_up
 *	Performs a name look up - could be local or over the network.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	host_name	: the host where the name is to be looked up.
 *	port_name	: the name to be looked up.
 *	port_ptr	: returns the port associated with the name.
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_CHECKED_IN	: the name was not found
 *	NETNAME_NO_SUCH_HOST	: the host_name was invalid
 *	NETNAME_HOST_NOT_FOUND	: the named host did not respond
 *
 * Design:
 *	See if the host name can be treated as an IP address.
 *	Look at the host name to see if we should do a local, directed or broadcast look up.
 *	If local, just see if the name is entered in out local name table.
 *	If directed or broadcast, call nn_network_look_up.
 *
 * Note:
 *	We cannot hear our own broadcasts.
 *
 */
PUBLIC int _netname_look_up(ServPort, host_name, port_name, port_ptr)
port_t		ServPort;
netname_name_t	host_name;
netname_name_t	port_name;
port_t		*port_ptr;
{
    int			hash_index, rc;
    nn_entry_ptr_t	name_entry_ptr;
    netaddr_t		host_id;

    *port_ptr = 0;
    /*
     * Convert the name to upper case.
     */
    NN_CONVERT_TO_UPPER(port_name);

    /*
     * Check to see if this is a local look up.
     */

    if ((sscanf(host_name, "0x%x", (unsigned *)&host_id)) != 1) {
	if (host_name[0] == '\0') host_id = my_host_id;
	else if ((host_name[1] == '\0') && (host_name[0] == '*')) host_id = broadcast_address;
	else host_id = nn_host_address(host_name);
    }

    if ((host_id == my_host_id) || (host_id == broadcast_address)) {
	/*
	 * See if the name is in our local name table.
	 */
	NN_NAME_HASH(hash_index, port_name);
	name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index],
						nn_name_test,(int)port_name);
	if (name_entry_ptr == (nn_entry_ptr_t)0) {
	    if ((host_id == broadcast_address) && (param.conf_network)) {
		/*
		 * Try broadcasting.
		 */
		rc = nn_network_look_up(host_id, port_name, port_ptr);
		RETURN(rc);
	    }
	    else {
		RETURN(NETNAME_NOT_CHECKED_IN);
	    }
	}
	else {
	    *port_ptr = name_entry_ptr->nne_port;
	    RETURN(NETNAME_SUCCESS);
	}
    }
    else if (host_id == 0) {
	RETURN(NETNAME_NO_SUCH_HOST);
    }
    else {
	if (param.conf_network) {
		rc = nn_network_look_up(host_id, port_name, port_ptr);
		RETURN(rc);
	} else {
		RETURN(NETNAME_NOT_CHECKED_IN);
	}
    }
}


/*
 * nn_port_test
 *	Sees whether a given queue entry contains the input port.
 *
 * Parameters:
 *	q_item	: the queue entry in question
 *	port_id	: the port in question
 *
 * Returns:
 *	TRUE if the port_id matches either the named or signature port if the queued item.
 *
 */
PRIVATE int nn_port_test(q_item, port_id)
register lock_queue_t	q_item;
register int		port_id;
{


    RETURN((((nn_entry_ptr_t)q_item)->nne_port == (port_t)port_id)
		|| (((nn_entry_ptr_t)q_item)->nne_signature == (port_t)port_id));

}
 

/*
 * nn_remove_entries
 *	Remove entries in the local name table for a given port.
 *
 * Parameters:
 *	port_id	: the port in question
 *
 * Design:
 *	Looks for entries in the name table for which this port is either
 *	the signature port or the named port.
 *	These entries are deleted.
 *
 */
EXPORT void nn_remove_entries(port_id)
port_t		port_id;
{
    int			index;
    lock_queue_t	lq;
    cthread_queue_item_t	q_item;

    for (index = 0; index < NN_TABLE_SIZE; index++) {
	lq = &nn_table[index];
	while ((q_item = lq_cond_delete_from_queue(lq, nn_port_test, (int)port_id)) != (cthread_queue_item_t)0) {
	    MEM_DEALLOCOBJ(q_item, MEM_NNREC);
	}
    }

    RET;

}


/*
 * _netname_version
 *	Returns the version of this network server.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	version		: where to put the version string,
 *
 * Design:
 *	Just return the rcsid of this file.
 *
 */
PUBLIC int _netname_version(ServPort, version)
port_t		ServPort;
netname_name_t	version;
{
    static const char* nm_version = "Teflon v0.01";
    (void)strcpy((char *)version, nm_version);
    RETURN(NETNAME_SUCCESS);

}


