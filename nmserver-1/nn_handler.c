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
#include <mach/cthreads.h>

#ifndef WIN32
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>

#ifdef __svr4__
#include <string.h>
#else
#include <strings.h>
#endif

#endif

#include "crypt.h"
#include "debug.h"
#include "dispatcher.h"
#include "mem.h"
#include "netmsg.h"
#include <servers/netname_defs.h>
#include "nm_extra.h"
#include "nn_defs.h"
#include "portrec.h"
#include "port_defs.h"
#include "sbuf.h"
#include "transport.h"
#include "network.h"
#include "ipc.h"
#include "access_list.h"

/*
 * Flag for debugging conditions handling.
 */
#define	NN_COND		0x1000


/*
 * nn_cleanup
 *	Called by the transport module if a request has failed.
 *
 * Parameters:
 *	client_id	: a pointer to the original request record.
 *	completion_code	: the reason for calling cleanup
 *
 * Design:
 *	Sets the result in the request record to NETNAME_HOST_NOT_FOUND
 *	and signals on the condition in the record.
 *
 */
PRIVATE int nn_cleanup(client_id, completion_code)
int	client_id;
int	completion_code;
{
    nn_req_rec_ptr_t	req_rec_ptr;

    req_rec_ptr = (nn_req_rec_ptr_t)client_id;
    mutex_lock(&req_rec_ptr->nnrr_lock);
    req_rec_ptr->nnrr_lport = 0;
    req_rec_ptr->nnrr_result = NETNAME_HOST_NOT_FOUND;
    condition_signal(&req_rec_ptr->nnrr_condition);
    mutex_unlock(&req_rec_ptr->nnrr_lock);
    RETURN(0);

}



/*
 * nn_handle_reply
 *	Handles the results of a network name lookup request.
 *	It is called by the srr transport protocol.
 *
 * Parameters:
 *	client_id	: pointer to the request record which generated this reply
 *	reply		: pointer to the reply sbuf
 *	from		: host from which the reply was received - ignored.
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Design:
 *	If the network port returned is not the null network port,
 *	call pr_ntran to get the corresponding local port.
 *	Fill in the fields of the request record to reflect the result.
 *	Signals on the condition in the request record.
 *
 */
/* ARGSUSED */
PUBLIC int nn_handle_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    nn_req_ptr_t	rep_ptr;
    nn_req_rec_ptr_t	req_rec_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(nn_replies_rcvd);
    SBUF_GET_SEG(*reply, rep_ptr, nn_req_ptr_t);
    req_rec_ptr = (nn_req_rec_ptr_t)client_id;
    mutex_lock(&req_rec_ptr->nnrr_lock);

    if (NPORT_EQUAL(null_network_port, rep_ptr->nnr_nport)) {
	req_rec_ptr->nnrr_result = NETNAME_NOT_CHECKED_IN;
	req_rec_ptr->nnrr_lport = 0;
    }
    else {
	if ((port_rec_ptr = pr_ntran((network_port_ptr_t)&(rep_ptr->nnr_nport))) == PORT_REC_NULL) {
	    char nport_string[40];
	    pr_nporttostring(nport_string, (network_port_ptr_t)&(rep_ptr->nnr_nport));
	    ERROR((msg, "nn_handle_reply.pr_ntran fails, network port = %s.\n", nport_string));
	    req_rec_ptr->nnrr_result = NETNAME_HOST_NOT_FOUND;
	    req_rec_ptr->nnrr_lport = 0;
	}
	else {
	    req_rec_ptr->nnrr_result = NETNAME_SUCCESS;
	    req_rec_ptr->nnrr_lport = port_rec_ptr->portrec_local_port;
	    lk_unlock(&port_rec_ptr->portrec_lock);
	}
    }

    condition_signal(&req_rec_ptr->nnrr_condition);
    mutex_unlock(&req_rec_ptr->nnrr_lock);
    RETURN(DISP_SUCCESS);

}



/*
 * nn_handle_request
 *	Handles and replies to a name look request.
 *	It is called by the srr transport protocol.
 *
 * Parameters:
 *	request		: the incoming checkup request
 *	from		: the sender of the request (ignored)
 *	broadcast	: whether the request was broadcast
 *	crypt_level	: ignored
 *
 * Design:
 *	Looks to see if the name can be found in our local name table.
 *	If it can be found,
 *		call pr_ltran and place the corresponding local port into the request.
 *	If it cannot be found and the request was broadcast
 *	then return DISP_FAILURE else place the null network port into the request.
 *
 */
/* ARGSUSED */
PUBLIC int nn_handle_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    int			hash_index;
    nn_entry_ptr_t	name_entry_ptr;
    netname_name_t	port_name;
    nn_req_ptr_t	req_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(nn_requests_rcvd);
    SBUF_GET_SEG(*request, req_ptr, nn_req_ptr_t);

    /*
     * Set src_format to be our own source format.
     */
    req_ptr->nnr_disp_hdr.src_format = conf_own_format;

    (void)strncpy(port_name, req_ptr->nnr_name, sizeof(netname_name_t)-1);
    port_name[sizeof(netname_name_t)-1] = '\0';
    NN_NAME_HASH(hash_index, port_name);
    name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index], nn_name_test, (int)port_name);
    if (secure_flag || (local_flag && !access_allowed(from, port_name)) || name_entry_ptr == (nn_entry_ptr_t)0) {
	if (broadcast) {
	    RETURN(DISP_IGNORE);
	}
	else {
	    req_ptr->nnr_nport = null_network_port;
	    RETURN(DISP_SUCCESS);
	}
    }
    else {
	if ((port_rec_ptr = pr_ltran(name_entry_ptr->nne_port)) == PORT_REC_NULL) {
	    ERROR((msg, "nn_handle_request.pr_lportlookup fails, port = %d.\n", name_entry_ptr->nne_port));
	    RETURN(DISP_FAILURE);
	}
	/* port_rec_ptr LOCK RW/R */
	req_ptr->nnr_nport = port_rec_ptr->portrec_network_port;
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_SUCCESS);
    }
}



/*
 * nn_network_look_up
 *	Perform a network name look up.
 *
 * Parameters:
 *	host_id		: the host to which the request should be directed
 *	port_name	: the name to be looked up
 *	port_ptr	: returns the local port found
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_CHECKED_IN
 *	NETNAME_HOST_NOT_FOUND
 *
 * Design:
 *	Construct a name request and send if off using the SRR transport protocol.
 *	Wait for the request to be answered (request record becomes unlocked).
 *
 */
PUBLIC int nn_network_look_up(host_id, port_name, port_ptr)
netaddr_t	host_id;
netname_name_t	port_name;
port_t		*port_ptr;
{
    nn_req_rec_ptr_t	req_rec_ptr;
    nn_req_ptr_t	req_ptr;
    sbuf_t		req_buf;
    sbuf_seg_t		req_buf_seg;
    int			rc;

    MEM_ALLOCOBJ(req_rec_ptr,nn_req_rec_ptr_t,MEM_NNREC);
    condition_init(&req_rec_ptr->nnrr_condition);
    mutex_init(&req_rec_ptr->nnrr_lock);
    mutex_lock(&req_rec_ptr->nnrr_lock);

    MEM_ALLOCOBJ(req_ptr,nn_req_ptr_t,MEM_TRBUFF);
    req_ptr->nnr_disp_hdr.src_format = conf_own_format;
    req_ptr->nnr_disp_hdr.disp_type = htons(DISP_NETNAME);
    (void)strncpy(req_ptr->nnr_name, port_name, sizeof(netname_name_t)-1);
    req_ptr->nnr_name[sizeof(netname_name_t)-1] = '\0';

    req_rec_ptr->nnrr_result = NETNAME_PENDING;

    SBUF_SEG_INIT(req_buf, &req_buf_seg);
    SBUF_APPEND(req_buf, req_ptr, sizeof(nn_req_t));

    mutex_unlock(&req_rec_ptr->nnrr_lock);
    if ((rc = transport_switch[TR_SRR_ENTRY].send((int)req_rec_ptr, (sbuf_ptr_t)&req_buf, host_id,
			TRSERV_NORMAL, CRYPT_DONT_ENCRYPT, nn_cleanup)) != TR_SUCCESS)
    {
	ERROR((msg, "nn_network_look_up.srr_send fails, rc = %d.\n", rc));
	*port_ptr = 0;
	rc = NETNAME_HOST_NOT_FOUND;
    }
    else {
	INCSTAT(nn_requests_sent);
	/*
	 * Wait for the request record to become unlocked.
	 */
	mutex_lock(&req_rec_ptr->nnrr_lock);
        while (req_rec_ptr->nnrr_result == NETNAME_PENDING)
            condition_wait(&req_rec_ptr->nnrr_condition, &req_rec_ptr->nnrr_lock);
	mutex_unlock(&req_rec_ptr->nnrr_lock);
	*port_ptr = req_rec_ptr->nnrr_lport;
	rc = req_rec_ptr->nnrr_result;
    }

    mutex_clear(&req_rec_ptr->nnrr_lock);
    condition_clear(&req_rec_ptr->nnrr_condition);
    MEM_DEALLOCOBJ(req_rec_ptr, MEM_NNREC);
    MEM_DEALLOCOBJ(req_ptr, MEM_TRBUFF);

    RETURN(rc);

}

