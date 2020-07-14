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

#include <mach/mach.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/message.h>

#include "crypt.h"
#include "debug.h"
#include "ls_defs.h"
#include "netmsg.h"
#include <servers/nm_defs.h>
#include "po_defs.h"
#include "uid.h"

static port_t deallocate_port;



/*
 * po_utils_init
 *	allocates a port for use by po_port_deallocate.
 *
 * Returns:
 *	TRUE or FALSE.
 *
 */
PUBLIC boolean_t po_utils_init()
{
    kern_return_t	kr;

    if ((kr = port_allocate(task_self(), &deallocate_port)) != KERN_SUCCESS) {
	ERROR((msg, "po_utils_init.port_allocate fails, kr = %d.", kr));
	RETURN(FALSE);
    }
    RETURN(TRUE);

}


/*
 * po_create_token
 *	Creates a new token of authenticity of a receiver or an owner.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to the record of the port for which the token is to be created
 *	token_ptr	: pointer to the token to be created
 *
 * Returns:
 *	the random number that was used to construct the new token.
 *
 * Design:
 *	Encrypt the SID of the network port plus a 32bit random number with the RO key.
 *
 * Note:
 *	The token is [X,SID,X] so that we can encrypt in multiples of 8 bytes.
 *
 */
EXPORT long po_create_token(port_rec_ptr, token_ptr)
port_rec_ptr_t		port_rec_ptr;
secure_info_ptr_t	token_ptr;
{
    long	x;
    nmkey_t	ekey;

    x = uid_get_new_uid();
    /*
     * Fill in the token.
     */
    token_ptr->si_token.key_longs[0] = x;
    token_ptr->si_token.key_longs[1] = port_rec_ptr->portrec_network_port.np_sid.np_uid_high;
    token_ptr->si_token.key_longs[2] = port_rec_ptr->portrec_network_port.np_sid.np_uid_low;
    token_ptr->si_token.key_longs[3] = x;

    /*
     * Encrypt the token.
     */
    ekey = port_rec_ptr->portrec_secure_info.si_key;
    if (CHECK_ENCRYPT_ALGORITHM(param.crypt_algorithm)) {
	(void)crypt_functions[param.crypt_algorithm].encrypt
				(ekey, (pointer_t)&token_ptr->si_token.key_longs[0], 16); 
    }
    else {
	ERROR((msg, "po_create_token: illegal decryption algorithm %d.", param.crypt_algorithm));
	RETURN(0);
    }

    RETURN(x);

}



/*
 * po_port_deallocate
 *	deallocate a port locally but retain send rights to it.
 *
 * Parameters:
 *	lport	: the local port to be deallocated.
 *
 * Design:
 *	check that we have either receive or ownership rights to this port but not both.
 *	If so, send a message to ourselves to retain send rights to this port;
 *	otherwise do not send a message to ourselves.
 *
 * Note:
 *	we rely on the checkups module to later determine that this port is invalid.
 *
 */
EXPORT void po_port_deallocate(lport)
port_t	lport;
{
    msg_header_t	send_rights_message;
    kern_return_t	kr;
    port_set_name_t	enabled;
    boolean_t		ownership, receive_rights;
    int			num_msgs, backlog;

    if ((kr = port_status(task_self(), lport, &enabled, &num_msgs, &backlog,
			&ownership, &receive_rights)) != KERN_SUCCESS)
    {
	ERROR((msg, "po_port_deallocate.port_status fails, kr = %d.", kr));
	RET;
    }
    if (ownership && receive_rights) {
	/*
 	 * Just deallocate the port and exit.
	 */
	if ((kr = port_deallocate(task_self(), lport)) != KERN_SUCCESS) {
	    ERROR((msg, "po_port_deallocate.port_deallocate fails, kr = %d.", kr));
	}
	RET;
    }

    send_rights_message.msg_simple = TRUE;
    send_rights_message.msg_size = sizeof(msg_header_t);
    send_rights_message.msg_type = MSG_TYPE_NORMAL;
    send_rights_message.msg_local_port = lport;
    send_rights_message.msg_remote_port = deallocate_port;
    send_rights_message.msg_id = 0;
    if ((kr = msg_send(&send_rights_message, MSG_OPTION_NONE, 0)) != KERN_SUCCESS) {
	ERROR((msg, "po_port_deallocate.msg_send fails, kr = %d.", kr));
	RET;
    }

    if ((kr = port_deallocate(task_self(), lport)) != KERN_SUCCESS) {
	ERROR((msg, "po_port_deallocate.port_deallocate fails, kr = %d.", kr));
    }

    send_rights_message.msg_local_port = deallocate_port;
    send_rights_message.msg_size = sizeof(msg_header_t);
    if ((kr = msg_receive(&send_rights_message, MSG_OPTION_NONE, 0)) != KERN_SUCCESS) {
	ERROR((msg, "po_port_deallocate.msg_receive fails, kr = %d.", kr));
	RET;
    }

    if (lport != send_rights_message.msg_remote_port) {
    }

    RET;

}

