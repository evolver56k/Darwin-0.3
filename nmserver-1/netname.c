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

#include "config.h"


#include <sys/types.h>
#include <mach/cthreads.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#ifndef NeXT_PDO
#include <mach/boolean.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#endif

#include "debug.h"
#include "mem.h"
#include "netmsg.h"
#include "nm_extra.h"
#include "nn_defs.h"
#include "nn.h"


port_t	netname_port;

#define NETNAME_NAME	"NEW_NETWORK_NAME_SERVER"

#define NETNAME_SUBSYSTEM 1040  /* from netname.defs */

#define NN_MAX_MSG_SIZE	512
#define NN_NUM_THREADS	1

extern boolean_t netname_server();
extern void nn_procs_init();

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_NNREC;


/*
 * netname_main
 *	Main loop for network name service.
 *
 * Results:
 *	Should never return.
 *
 * Design:
 *	Wait for a message on the name service port.
 *	Call netname_server to process it.
 *	Send the reply message.
 *
 * Note:
 *	There may be multiple threads executing this main loop.
 *
 */
PRIVATE void netname_main()
{
    msg_header_t	*req_msg_ptr, *rep_msg_ptr;
    kern_return_t	kr;
    boolean_t		send_reply;
    boolean_t		req_ok;

    MEM_ALLOC(req_msg_ptr,msg_header_t *,NN_MAX_MSG_SIZE, FALSE);

    MEM_ALLOC(rep_msg_ptr,msg_header_t *,NN_MAX_MSG_SIZE, FALSE);

    while (TRUE) {
	/*
	 * Wait for a name service request.
	 */
	req_msg_ptr->msg_size = NN_MAX_MSG_SIZE;
	req_msg_ptr->msg_local_port = netname_port;
	kr = netmsg_receive(req_msg_ptr);

	if (kr == RCV_SUCCESS) {
		req_ok = netname_server((caddr_t)req_msg_ptr, (caddr_t)rep_msg_ptr);
		if (!req_ok) {
			ERROR((msg, "netname_main.netname_server fails, msg id = %d.", req_msg_ptr->msg_id));
			send_reply = FALSE;
		} else {
			send_reply = (((death_pill_t *)rep_msg_ptr)->RetCode != MIG_NO_REPLY);
		}
		if (send_reply) {
			/*
			 * Send the reply back.
			 */
			kr = msg_send(rep_msg_ptr, MSG_OPTION_NONE, 0);
			if (kr != SEND_SUCCESS) {
				port_type_t portType;
				
				ERROR((msg, "netname_main.msg_send fails, kr = %d.", kr));
				/* first, see if the reply port is valid */
				kr = port_type(task_self(),
					(port_name_t)rep_msg_ptr->msg_remote_port, &portType);
				if (kr != KERN_SUCCESS) { /* ERROR doesn't swallow semicolon */
					ERROR((msg, "netname_main.port_type fails, kr = %d.",kr));
				}
				else {					
					switch(req_msg_ptr->msg_id - NETNAME_SUBSYSTEM) {
						case 0: /* netname_check_in() */
							/* return value is NETNAME_INVALID_PORT,
								NETNAME_NOT_YOURS, or NETNAME_SUCCESS */
							break;
						case 1: /* netname_look_up() */
							{
							/* return value is NETNAME_NOT_CHECKED_IN,
								NETNAME_NO_SUCH_HOST,
								NETNAME_HOST_NOT_FOUND,
								NETNAME_NOT_CHECKED_IN, 
								or NETNAME_SUCCESS */
							/* if the reply port is valid, the returned
							   port must be invalid */
							/* from netname_server.c (build directory) */
							typedef struct {
								msg_header_t Head;
								msg_type_t RetCodeType;
								kern_return_t RetCode;
								msg_type_t port_idType;
								port_t port_id;
							} LookupReply;
							 	
							nn_remove_entries(((LookupReply *)
								rep_msg_ptr)->port_id);
							((LookupReply *)rep_msg_ptr)->port_id = PORT_NULL;
							((LookupReply *)rep_msg_ptr)->RetCode = 
								NETNAME_NOT_CHECKED_IN;
							kr = msg_send(rep_msg_ptr, MSG_OPTION_NONE, 0);
							if (kr != SEND_SUCCESS)
								ERROR((msg, "netname_main.msg_send fails "
									"on retry, kr = %d.", kr));
							}
							break;
						case 2: /* netname_check_out() */
							/* return value is NETNAME_NOT_CHECKED_IN,
								NETNAME_NOT_YOURS, or NETNAME_SUCCESS */
							break;
						case 3: /* netname_version() */
							/* return value is always NETNAME_SUCCESS */
							break;
					}
				}
			}
		}
	} else {
	    ERROR((msg, "netname_main.netmsg_receive fails, kr = %d.", kr));
	}

	LOGCHECK;
    }

}



/*
 * netname_init
 *	Initialises the network name service.
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Design:
 *	Initialise the name hash table.
 *	Somehow initialise the network name service receive port.
 *	Start up some number of threads to handle name service requests.
 *
 * Note:
 *	First try to do a service_checkin to get a name service port
 *	otherwise we check in a port with the old netmsgserver.
 *
 */
EXPORT boolean_t netname_init()
{
    kern_return_t	kr;
    int			i;
    cthread_t		new_thread;

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_NNREC,"Netname record",
				((sizeof(nn_entry_t)) > (sizeof(nn_req_rec_t)) ?
					sizeof(nn_entry_t) : sizeof(nn_req_rec_t)),
				FALSE,170,50);


    nn_procs_init();

#ifdef NeXT_PDO
    if ((kr = port_allocate(task_self(), &netname_port)) != KERN_SUCCESS) {
	ERROR((msg, "netname_init.port_allocate fails, kr = %d.", kr));
	RETURN(FALSE);
    }
    if ((kr = bootstrap_register(bootstrap_port, "NetMessage", netname_port)) != KERN_SUCCESS) {
	ERROR((msg, "netname_init.bootstrap_register fails, kr = %d.", kr));
	RETURN(FALSE);
    }
#else
    if (bootstrap_check_in(bootstrap_port, "NetMessage", &netname_port) == KERN_SUCCESS) {
        if ((kr = _netname_check_in(PORT_NULL, NETNAME_NAME, task_self(), netname_port)) != KERN_SUCCESS)
	{
	    ERROR((msg, "netname_init._netname_check_in fails, kr = %d.", kr));
	    RETURN(FALSE);
	}
    }
    else {
	if ((kr = port_allocate(task_self(), &netname_port)) != KERN_SUCCESS) {
	    ERROR((msg, "netname_init.port_allocate fails, kr = %d.", kr));
	    RETURN(FALSE);
	}
        if ((kr = netname_check_in(name_server_port, NETNAME_NAME, PORT_NULL, netname_port))
		!= KERN_SUCCESS)
	{
	    ERROR((msg, "netname_init.netname_check_in fails, kr = %d.", kr));
	    RETURN(FALSE);
	}
    }
#endif

    for (i = 0; i < NN_NUM_THREADS; i++) {
	new_thread = cthread_fork((cthread_fn_t)netname_main, (any_t)0);
	cthread_set_name(new_thread, "netname_main");
	cthread_detach(new_thread);
    }

    RETURN(TRUE);

}
