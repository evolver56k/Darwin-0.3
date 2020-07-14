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
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "config.h"
#include "disp_hdr.h"
#include "key_defs.h"
#include "netmsg.h"
#include "network.h"
#include <servers/nm_defs.h>
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "ps_defs.h"
#include "rwlock.h"
#include "sbuf.h"
#include "transport.h"
#include "ipc.h"
#include "keyman.h"



/*
 * ps_auth_km_retry
 *	called if a key exchange for an authentication request completes.
 *
 * Parameters:
 *	event_ptr	: the event for which transmission previously failed.
 *
 * Results:
 *	ignored
 *
 * Design:
 *	Just call ps_authenticate_port.
 *
 */
PUBLIC int ps_auth_km_retry(event_ptr)
ps_event_ptr_t	event_ptr;
{
    port_rec_ptr_t	port_rec_ptr;

    if ((port_rec_ptr = pr_lportlookup(event_ptr->pse_lport)) == PORT_REC_NULL) {
	RETURN(0);
    }
    ps_authenticate_port(event_ptr, port_rec_ptr);
    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(0);

}


/*
 * ps_auth_cleanup
 *	called if a transmission of an authentication request failed for some reason.
 *
 * Parameters:
 *	event_ptr	: pointer to the event for which the transmission failed.
 *	reason		: reason why the transmission failed.
 *
 * Results:
 *	meaningless.
 *
 * Design:
 *	If reason is TR_CRYPT_FAILURE then call km_do_key_exchange
 *	otherwise try searching for this port once more.
 *
 */
PRIVATE int ps_auth_cleanup(event_ptr, reason)
ps_event_ptr_t	event_ptr;
int		reason;
{
    port_rec_ptr_t	port_rec_ptr;


    if (reason == TR_CRYPT_FAILURE) {
	km_do_key_exchange(event_ptr, ps_auth_km_retry, event_ptr->pse_destination);
    }
    else {
	if ((port_rec_ptr = pr_lportlookup(event_ptr->pse_lport)) == PORT_REC_NULL) {
	    RETURN(0);
	}
	/* port_rec_ptr LOCK RW/RW */
	event_ptr->pse_state &= ~PS_AUTHENTICATION;
	event_ptr->pse_destination = broadcast_address;
	ps_send_query(event_ptr, port_rec_ptr);
	lk_unlock(&port_rec_ptr->portrec_lock);
    }


    RETURN(0);

}



/*
 * ps_authenticate_port
 *	perform an network port authentication.
 *
 * Parameters:
 *	event_ptr	: pointer to event for which an authentication is required.
 *	port_rec_ptr	: pointer to port for which an authentication is required.
 *
 * Design:
 *	If we are the receiver or owner for this port then we must create a new token.
 *	Send off an authentication request to the destination contained in the event.
 *
 * Note:
 *	the port record should be locked.
 *
 */
PUBLIC void ps_authenticate_port(event_ptr, port_rec_ptr)
ps_event_ptr_t	event_ptr;
port_rec_ptr_t	port_rec_ptr;
{
    sbuf_t	sbuf;
    sbuf_seg_t	sbuf_seg;
    ps_auth_t	auth_data;
    int		tr;


    if (event_ptr->pse_state & PS_AUTHENTICATION) {
	RET;
    }

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &auth_data, sizeof(ps_auth_t));
    auth_data.psa_disp_hdr.disp_type = htons(DISP_PS_AUTH);
    auth_data.psa_disp_hdr.src_format = conf_own_format;
    auth_data.psa_puid = port_rec_ptr->portrec_network_port.np_puid;

    if (NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port)) {
	/*
	 * Create a token and stuff the new random number into the port record.
	 */
	port_rec_ptr->portrec_random = po_create_token(port_rec_ptr, &(auth_data.psa_token));
    }
    else {
	auth_data.psa_token = port_rec_ptr->portrec_secure_info;
    }

    tr = transport_switch[TR_SRR_ENTRY].send(event_ptr, &sbuf, event_ptr->pse_destination, TRSERV_NORMAL,
						port_rec_ptr->portrec_security_level, ps_auth_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "ps_authenticate_port.send fails, tr = %d.", tr));
	lk_unlock(&port_rec_ptr->portrec_lock);
	(void)ps_auth_cleanup(event_ptr, tr);
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    }
    else INCSTAT(ps_auth_requests_sent);

    RET;

}



