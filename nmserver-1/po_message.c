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

#include <mach/mach_types.h>

#ifndef WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "key_defs.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include <servers/nm_defs.h>
#include "po_defs.h"
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "portsearch.h"
#include "rwlock.h"
#include "sbuf.h"
#include "transport.h"
#include "ipc.h"
#include "keyman.h"



/*
 * po_handle_token_reply
 *	Handles a reply to a token request.
 *
 * Parameters:
 *	client_id	: ignored.
 *	reply		: pointer to the reply sbuf
 *	from		: host from which the reply was received - ignored.
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Returns:
 *	DISP_SUCCESS.
 *
 * Design:
 *	Store the received token.
 *
 */
/* ARGSUSED */
PUBLIC int po_handle_token_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_token_replies_rcvd);
    SBUF_GET_SEG(*reply, message_ptr, po_message_ptr_t);

    if ((port_rec_ptr = pr_nportlookup(&(message_ptr->pom_po_data.pod_nport))) == PORT_REC_NULL) {
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */
    if (!(NPORT_HAVE_SEND_RIGHTS(port_rec_ptr->portrec_network_port))) {
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_SUCCESS);
    }

    /*
     * Remember the new token.
     */
    port_rec_ptr->portrec_secure_info = message_ptr->pom_po_data.pod_sinfo;
    port_rec_ptr->portrec_random = message_ptr->pom_po_data.pod_extra;

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

}



/*
 * po_handle_token_request
 *	handles a request for a token.
 *
 * Parameters:
 *	request		: the incoming token request
 *	from		: the sender of the request (ignored)
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Results:
 *	DISP_SUCCESS or DISP_FAILURE.
 *
 * Design:
 *	Looks up the port.
 *	Creates a new token and places it in the request.
 *
 * Note:
 *	Assume that the transport module is doing the encryption.
 *
 */
/* ARGSUSED */
PUBLIC int po_handle_token_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_token_requests_rcvd);
    SBUF_GET_SEG(*request, message_ptr, po_message_ptr_t);

    if ((port_rec_ptr = pr_nportlookup(&(message_ptr->pom_po_data.pod_nport))) == PORT_REC_NULL) {
	RETURN(DISP_IGNORE);
    }
    /* port_rec_ptr LOCK RW/RW */
    if (!(NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port))) {
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_IGNORE);
    }

    /*
     * Create the reply.
     */
    message_ptr->pom_po_data.pod_extra = po_create_token(port_rec_ptr,
						&(message_ptr->pom_po_data.pod_sinfo));
    message_ptr->pom_disp_hdr.src_format = conf_own_format;

    INCPORTSTAT(port_rec_ptr, tokens_sent);

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

}



/*
 * po_token_km_retry
 *	called if a key exchange for a token request completes.
 *
 * Parameters:
 *	port_rec_ptr	: the port for which the token was requested.
 *
 * Results:
 *	ignored
 *
 * Design:
 *	Just call po_token_request.
 *
 */
PUBLIC int po_token_km_retry(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
{

    lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    po_request_token(port_rec_ptr, CRYPT_ENCRYPT);
    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(0);

}


/*
 * po_token_cleanup
 *	called if a token request failed.
 *
 * Parameters:
 *	port_rec_ptr	: the port for which the token was requested.
 *	reason		: the reason for the failure
 *
 * Results:
 *	ignored
 *
 * Design:
 *	If the reason is TR_CRYPT_FAILURE then call km_do_key_exchange.
 *
 */
PUBLIC int po_token_cleanup(port_rec_ptr, reason)
port_rec_ptr_t	port_rec_ptr;
int		reason;
{
    netaddr_t	destination;

    if (reason == TR_CRYPT_FAILURE) {
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
	destination = PORT_REC_RECEIVER(port_rec_ptr);
	lk_unlock(&port_rec_ptr->portrec_lock);
	km_do_key_exchange(port_rec_ptr, po_token_km_retry, destination);
    }

    RETURN(0);
}




/*
 * po_request_token
 *	requests a token of authenticity of a receiver or owner
 *
 * Parameters:
 *	port_rec_ptr	: the record of the port that needs the token
 *	security_level	: the security level that should be used
 *
 * Design:
 *	Submits a token request using the SRR transport protocol.
 *
 */
PUBLIC void po_request_token(port_rec_ptr, security_level)
port_rec_ptr_t	port_rec_ptr;
int		security_level;
{
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;
    netaddr_t		destination;

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));
    message.pom_disp_hdr.disp_type = htons(DISP_PO_TOKEN);
    message.pom_disp_hdr.src_format = conf_own_format;
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
    destination = PORT_REC_RECEIVER(port_rec_ptr);


    tr = transport_switch[TR_SRR_ENTRY].send(port_rec_ptr, &sbuf, destination,
				TRSERV_NORMAL, security_level, po_token_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_request_token.send fails, tr = %d.", tr));
    }
    else INCSTAT(po_token_requests_sent);

    RET;

}



/*
 * po_handle_ro_xfer_hint
 *	Handles an unreliable notification of a transfer of receive/ownership rights.
 *
 * Parameters:
 *	client_id	: ignored.
 *	data		: data received over the network.
 *	from		: host from which the data was received (ignored).
 *	broadcast	: ignored.
 *	crypt_level	: the security level of the incoming data.
 *
 * Returns:
 *	DISP_SUCCESS
 *
 * Side effects:
 *	May initiate a port search.
 *
 * Design:
 *	If we have no rights to the port, then ignore it.
 *	If we have both rights, then something is wrong.
 *	If we are the owner, then take note of the new receiver.
 *	If we are the receiver, then take note of the new owner.
 *	If we just have send rights, then trigger a port search.
 *
 * Note:
 *	Maybe we should not trigger a port search.
 *
 */
/* ARGSUSED */
PUBLIC int po_handle_ro_xfer_hint(client_id, data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    po_message_ptr_t	message_ptr;
    network_port_t	nport;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_ro_hints_rcvd);
    SBUF_GET_SEG(*data, message_ptr, po_message_ptr_t);
    nport = message_ptr->pom_po_data.pod_nport;
    if ((port_rec_ptr = pr_nportlookup(&nport)) == PORT_REC_NULL) {
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */
    INCPORTSTAT(port_rec_ptr, xfer_hints_rcvd);


    if (NPORT_HAVE_ALL_RIGHTS(port_rec_ptr->portrec_network_port)) {
	/*
	 * We are the receiver and the owner.
	 */
    }
    else if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
	/*
	 * We are the receiver.
	 */
        port_rec_ptr->portrec_network_port.np_owner = message_ptr->pom_po_data.pod_nport.np_owner;
        ipc_port_moved(port_rec_ptr);
    }
    else if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
	/*
	 * We are the owner.
	 */
        port_rec_ptr->portrec_network_port.np_receiver = message_ptr->pom_po_data.pod_nport.np_receiver;
        ipc_port_moved(port_rec_ptr);
    }
    else {
	/*
	 * We must have send rights to this port - start a port search.
	 */
	/* XXX See research notes about how we should use the new information. XXX */
	ps_do_port_search(port_rec_ptr, TRUE, &nport, (int(*)())0);
    }

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

}



/*
 * po_send_ro_xfer_hint
 *	Send an unreliable notification of a transfer of receive or ownership rights.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to the record for the port in question.
 *	destination	: network server to be sent the message.
 *	security_level	: the security level at which this notification should be sent.
 *
 * Design:
 *	Check that the destination is either the receiver or the owner.
 *	Construct a packet containing the data and send it using the datagram transport protocol.
 *
 */
PUBLIC void po_send_ro_xfer_hint(port_rec_ptr, destination, security_level)
port_rec_ptr_t		port_rec_ptr;
netaddr_t		destination;
int			security_level;
{
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;

    if ((destination != port_rec_ptr->portrec_network_port.np_receiver)
	&& (destination != port_rec_ptr->portrec_network_port.np_owner))
    {
	RET;
    }

    /*
     * Now send the network port identifier and the RO key.
     * The datagram should be sent at security_level.
     */
    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));
    message.pom_disp_hdr.disp_type = htons(DISP_PO_RO_HINT);
    message.pom_disp_hdr.src_format = conf_own_format;
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
    message.pom_po_data.pod_sinfo = port_rec_ptr->portrec_secure_info;
    if (param.crypt_algorithm == CRYPT_MULTPERM) NTOH_KEY(message.pom_po_data.pod_sinfo.si_key);

    tr = transport_switch[TR_DATAGRAM_ENTRY].send(0, &sbuf, destination, TRSERV_NORMAL,
							security_level, 0);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_send_ro_xfer_hint.send fails, tr = %d.", tr));
    }
    else {
	INCSTAT(po_ro_hints_sent);
	INCPORTSTAT(port_rec_ptr, xfer_hints_sent);
    }

    RET;

}
