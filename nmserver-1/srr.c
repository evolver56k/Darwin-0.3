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

#include "netmsg.h"
#include <servers/nm_defs.h>

#include <mach/mach.h>
#include <mach/message.h>
#include <mach/cthreads.h>

#include "crypt.h"
#include "debug.h"
#include "ls_defs.h"
#include "mem.h"
#include "netipc.h"
#include "network.h"
#include "nm_extra.h"
#include "sbuf.h"
#include "srr.h"
#include "srr_defs.h"
#include "timer.h"
#include "transport.h"
#include "disp_hdr.h"

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include <string.h>
#include <errno.h>

#ifdef WIN32
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif

static int		srr_socket;
static cthread_t	srr_listen_thread;


int			srr_max_data_size = SRR_MAX_DATA_SIZE;

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_SRRREQ;


/*
 * Minimum size of a meaningful srr packet.
 */
#define MIN_SRR_PACKET_LENGTH (sizeof(crypt_header_t) + sizeof(unsigned long) + sizeof(srr_uid_t))	


/*
 * srr_main
 *	The main reception loop for the simple request-response protocol.
 *	Allocates a reception buffer and waits for incoming messages.
 *	Calls appropriate handling function.
 *
 * Parameters:
 *	None
 *
 * Results:
 *	Should never return
 *
 * Note:
 *	A new buffer must be allocated
 *	if the handling routine does not return one that can be reused.
 *
 */
PRIVATE int srr_main()
{
    kern_return_t	kr;
    srr_packet_ptr_t	in_packet_ptr;
    srr_packet_ptr_t	old_packet_ptr;
    long		srr_packet_type;
    int			crypt_level;
    int			data_size;
    boolean_t		crypt_remote_failure;

    MEM_ALLOCOBJ(in_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);

    while (TRUE) {
	struct sockaddr_in addr;
	netaddr_t source_host;
	int length, addr_length;
	
	addr_length = sizeof(addr);
	length = recvfrom(srr_socket, (char *)in_packet_ptr, sizeof(netipc_t),
			  0, (struct sockaddr *)&addr, &addr_length);
	if (-1 == length) {
	    ERROR((msg, "srr_main.recvfrom fails: %d", ERRNO));
	    continue;
	}

	if (length < MIN_SRR_PACKET_LENGTH) {
	    ERROR((msg, "srr_main.recvfrom packet too short, len = %d", length));
	    continue;
	}
	
	source_host = addr.sin_addr.s_addr;

	crypt_level =
	    ntohl(in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level);
	if (crypt_level < 0) {
	    /*
	     * This is a remote crypt failure packet.
	     */
	    crypt_remote_failure = TRUE;
	    crypt_level = - crypt_level;
	    in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level =
			    htonl((unsigned long)crypt_level);
	} else
	    crypt_remote_failure = FALSE;

	if (crypt_level != CRYPT_DONT_ENCRYPT) {
	    kr = crypt_decrypt_packet((netipc_ptr_t)in_packet_ptr, crypt_level);
	} else
	    kr = CRYPT_SUCCESS;

	if (kr == CRYPT_SUCCESS) {
	    /*
	     * Swap the srr header and see what type of packet we got.
	     */
	    NTOH_SRR_UID(in_packet_ptr->srr_pkt_uid);
	    if (crypt_remote_failure) {
		/*
		 * This packet was sent by us but could not be decrypted by the
		 * remote network server.
		 */
		INCSTAT(srr_cfailures_rcvd);
		if (decryption_enabled() == CRYPT_ENCRYPT) {
		    old_packet_ptr = srr_handle_crypt_failure(in_packet_ptr, source_host);
		} else {
		    old_packet_ptr = in_packet_ptr; 	/* Ignore bogus packet */
		}
	    } else {
		srr_packet_type = ntohl(in_packet_ptr->srr_pkt_type);
		old_packet_ptr = SRR_NULL_PACKET;
		data_size =
		    ntohs(in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size);
		data_size -= SRR_HEADER_SIZE;
		if (data_size < 0)
		    data_size = 0;
		if ((data_size < sizeof(disp_hdr_t)) || (data_size > length)) {
		    ERROR((msg, "srr_main.netipc_receive invalid data_size %d from host %s", ntohs(in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size), inet_ntoa(addr.sin_addr)));
			old_packet_ptr = in_packet_ptr;
		}
		else switch ((int)srr_packet_type) {
		    case SRR_REQUEST: {
			INCSTAT(srr_requests_rcvd);
			old_packet_ptr = srr_handle_request(in_packet_ptr, data_size, crypt_level, 0, source_host);
			break;
		    }
		    case SRR_RESPONSE: {
			INCSTAT(srr_replies_rcvd);
			old_packet_ptr = srr_handle_response(in_packet_ptr, data_size, crypt_level, 0, source_host);
			break;
		    }
		    case SRR_BCAST_REQUEST: {
			INCSTAT(srr_bcasts_rcvd);
			INCSTAT(srr_requests_rcvd);
			old_packet_ptr = srr_handle_request(in_packet_ptr, data_size, crypt_level, 1, source_host);
			break;
		    }
		    case SRR_BCAST_RESPONSE: {
			INCSTAT(srr_replies_rcvd);
			old_packet_ptr = srr_handle_response(in_packet_ptr, data_size, crypt_level, 1, source_host);
			break;
		    }
		    default: {
			ERROR((msg, "srr_main unknown packet type %d.", (int) srr_packet_type));
			old_packet_ptr = in_packet_ptr;
			break;
		    }
		}
	    }
	} else {
	    if ((decryption_enabled() == CRYPT_ENCRYPT) 
		&& !crypt_remote_failure) srr_send_crypt_failure(in_packet_ptr, crypt_level, source_host);
	    old_packet_ptr = in_packet_ptr;
	}

	    if (old_packet_ptr == SRR_NULL_PACKET) {
		/*
		 *Allocate a new buffer.
		 */
		MEM_ALLOCOBJ(in_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);
	    }
	    else in_packet_ptr = old_packet_ptr;
	LOGCHECK;
    }
}



/*
 * srr_retry
 *	Called by the timer service to retransmit a request packet.
 *
 * Parameters:
 *	timer_record	: the information previously supplied to the timer service
 *
 * Results:
 *	SRR_SUCCESS of SRR_FAILURE
 *
 * Side effects:
 *	May decide that this request should be aborted
 *	in which case cleanup is called to inform the client.
 *	May send a request packet over the network and schedule another retransmission.
 *
 * Note:
 *	If the request packet is empty, then this request has already been aborted.
 *	Host info record should not be locked whilst srq_cleanup is called.
 *
 */
PUBLIC int srr_retry(timer_record)
nmtimer_t timer_record;
{
    srr_host_info_ptr_t	host_info;
    srr_packet_ptr_t	request_packet_ptr;

    /*
     * Sanity check.
     */
    host_info = srr_hash_lookup(((srr_host_info_ptr_t)(timer_record->info))->shi_host_id);
    if (host_info != (srr_host_info_ptr_t)(timer_record->info)) {
	panic("srr_retry.srr_hash_lookup");
    }

    /*
     * Lock the host information record before attempting to update it.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Check that the request actually needs to be retransmitted.
     */
    if (host_info->shi_request_status == SRR_HAVE_RESPONSE) {
	mutex_unlock(&host_info->shi_lock);
	RETURN(SRR_FAILURE);
    }

    /*
     * Check to see if this request has exceeded the maximum number of tries
     * and that it has not been aborted.
     */
    if ((host_info->shi_request_tries < param.srr_max_tries)
	&& (host_info->shi_request_q_head->srq_request_packet != SRR_NULL_PACKET))
    {
	/*
	 * Resend this request and requeue it with the timer service.
	 */
	if (host_info->shi_request_status == SRR_AWAITING_RESPONSE) {
	    int length;
	    struct sockaddr_in addr;
	    
	    request_packet_ptr = host_info->shi_request_q_head->srq_request_packet;
	    
	    /*
	     * Build the destination address.
	     */
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(SRR_UDP_PORT);
	    addr.sin_addr.s_addr = request_packet_ptr->srr_pkt_dest;
	    
	    length = sendto(srr_socket, (char *)request_packet_ptr,
			    NETIPC_PACKET_HEADER_SIZE + ntohs(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size),
			    0, (struct sockaddr *)&addr, sizeof(addr));
	    if (-1 == length) {
		ERROR((msg, "srr_retry.sendto fails: %d, %d + %d = %d", ERRNO, (int) NETIPC_PACKET_HEADER_SIZE, ntohs(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size), (int) NETIPC_PACKET_HEADER_SIZE + ntohs(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size)));
	    } else if (length < NETIPC_PACKET_HEADER_SIZE + ntohs(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size)) {
		ERROR((msg, "srr_retry.sendto fails: %d/%d bytes written", length, (int)NETIPC_PACKET_HEADER_SIZE + ntohs(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size)));
	    } else
		   INCSTAT(srr_retries_sent);
	}
	host_info->shi_request_tries ++;

	/*
	 * Also queue this request up for retransmission.
	 */
	timer_start(&host_info->shi_timer);
    }
    else {
	srr_request_q_ptr_t	old_request;

	/*
	 * Dequeue the request and call cleanup to inform the client of the failure.
	 */
	if ((old_request = srr_dequeue(host_info)) == SRR_NULL_Q) {
	    mutex_unlock(&host_info->shi_lock);
	    RETURN(SRR_FAILURE);
	}
	/*
	 * Must mark inactive before unlocking
	 */
	host_info->shi_request_status = SRR_INACTIVE;

	switch (host_info->shi_request_status) {
	    case SRR_LOCAL_CRYPT_FAILURE: case SRR_REMOTE_CRYPT_FAILURE: {
		if (old_request->srq_cleanup) {
		    mutex_unlock(&host_info->shi_lock);
		    old_request->srq_cleanup(old_request->srq_client_id, TR_CRYPT_FAILURE);
		    mutex_lock(&host_info->shi_lock);
		}
		break;
	    }
	    default: {
		/*
		 * Only call cleanup if the request packet is not null.
		 */
		if (old_request->srq_request_packet != SRR_NULL_PACKET) {
		    if (old_request->srq_cleanup) {
			mutex_unlock(&host_info->shi_lock);
			old_request->srq_cleanup(old_request->srq_client_id, TR_FAILURE);
			mutex_lock(&host_info->shi_lock);
		    }
		    MEM_DEALLOCOBJ(old_request->srq_request_packet, MEM_TRBUFF);
		}
		break;
	    }
	}

	MEM_DEALLOCOBJ(old_request, MEM_SRRREQ);

	/*
	 * If there is another request waiting transmission
	 * send it off and queue it with the timer service.
	 */
	if (host_info->shi_request_q_head != SRR_NULL_Q) {
	    srr_process_queued_request(host_info);
	}
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(SRR_SUCCESS);
}



/*
 * srr_send
 *	Either sends a request packet out or queues it up for sending later.
 *
 * Parameters:
 *	client_id	: an identifier assigned by the client to this transaction
 *	data		: the data to be sent
 *	to		: the destination of the request
 *	service		: ignored 
 *	crypt_level	: whether the data should be encrypted
 *	cleanup		: a function to be called when this transaction has finished
 *
 * Returns:
 *	TR_SUCCESS or a specific failure code.
 *
 * Side effects:
 *	May send a packet out over the network and with the timer module.
 *	Will queue a packet on the information record associated with the destination host.
 *	May create a new host record if there is no prior information for this host.
 *
 * Design:
 *	Construct a packet and a request record.
 *	Maybe create a record for the destination host.
 *	Lock the record for the destination host and enqueue this request.
 *	If there are no other requests waiting to go out
 *		then send the request off and queue it up for retransmission
 *
 * Note:
 *	We only call cleanup if the request-response interaction failed.
 *	Otherwise the response provided directly to the client is the
 *	indication to the client that it can now cleanup its records.
 *
 */
/*ARGSUSED*/
EXPORT int srr_send(client_id,data,to,service,crypt_level,cleanup)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	to;
int		service;
int		crypt_level;
int		(*cleanup)();
{
    srr_packet_ptr_t		request_packet_ptr;
    int				size, packet_type;
    srr_host_info_ptr_t		host_info;
    srr_request_q_ptr_t 	q_record;

    /*
     * Allocate a buffer to hold the request packet.
     */
    MEM_ALLOCOBJ(request_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);

    /*
     * Copy the input sbuf into our local buffer.
     */
    SBUF_FLATTEN(data, request_packet_ptr->srr_pkt_data, size);

    /*
     * Sanity check.
     */
    if (size > SRR_MAX_DATA_SIZE) {
	ERROR((msg, "srr_send fails, size of data (%d) is too large.", size));
	MEM_DEALLOCOBJ(request_packet_ptr, MEM_TRBUFF);
	RETURN(SRR_TOO_LARGE);
    }

    /*
     * Fill in the netipc header.
     */
    packet_type = (to == broadcast_address) ? SRR_BCAST_REQUEST : SRR_REQUEST;
    SRR_SET_PKT_HEADER(request_packet_ptr, size, to, packet_type, crypt_level);

    /*
     * Find the host record for the destination host.
     */
    host_info = srr_hash_lookup(to);
    if (host_info == SRR_NULL_HOST_INFO) host_info = srr_hash_enter(to);
    if (host_info == SRR_NULL_HOST_INFO) {
	ERROR((msg, "srr_send.srr_hash_enter fails."));
	RETURN(SRR_FAILURE);
    }

    /*
     * Create a queue record for this new request.
     */
    MEM_ALLOCOBJ(q_record,srr_request_q_ptr_t,MEM_SRRREQ);
    q_record->srq_request_packet = request_packet_ptr;
    q_record->srq_destination = to;
    q_record->srq_client_id = client_id;
    q_record->srq_cleanup = cleanup;

    /*
     * Lock the host information record before attempting to update it.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Queue the new request.
     * It it is queued at the head of the queue
     * then try to send this request off right now.
     */
    srr_enqueue(q_record, host_info);
    if (host_info->shi_request_q_head == q_record) {
	srr_process_queued_request(host_info);
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(TR_SUCCESS);
}


/*
 * srr_send_packet
 *	Write an srr packet to the network.
 *
 * Parameters:
 *	packet	: the srr formatted packet
 *
 * Results:
 *	-1 if an error occurred; 0 otherwise
 *
 */
EXPORT int srr_send_packet (packet)
srr_packet_ptr_t	packet;
{
    struct sockaddr_in addr;
    int length;
    
    /*
     * Build the destination address.
     */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SRR_UDP_PORT);
    addr.sin_addr.s_addr = packet->srr_pkt_dest;
    
    length = sendto(srr_socket, (char *)packet,
		    NETIPC_PACKET_HEADER_SIZE + ntohs(packet->srr_pkt_header.nih_crypt_header.ch_data_size),
		    0, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == length
	|| length < NETIPC_PACKET_HEADER_SIZE + ntohs(packet->srr_pkt_header.nih_crypt_header.ch_data_size))
	RETURN(-1);
    RETURN(0);
}



/*
 * srr_init
 *	Initialises the srr transport protocol.
 *
 * Parameters:
 *
 * Results:
 *	FALSE : we failed to initialise the srr transport protocol.
 *	TRUE  : we were successful.
 *
 * Side effects:
 *	Initialises the srr protocol entry point in the switch array.
 *	Initialises the template for sending network messages.
 *	Allocates the srr socket and creates a thread to listen to the network.
 *
 */
EXPORT boolean_t srr_init()
{
    struct sockaddr_in	addr;

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_SRRREQ,"SRR request",sizeof(srr_request_q_t), FALSE,200,10);

    srr_utils_init();

    transport_switch[TR_SRR_ENTRY].send = srr_send;

    /*
     * Create a socket bound to the srr port.
     */
    srr_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == srr_socket) {
	ERROR((msg, "srr_init.socket fails: %d", ERRNO));
	RETURN(FALSE);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SRR_UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;	// XXX should use my_host_id
    if (-1 == bind(srr_socket, (struct sockaddr *)&addr, sizeof(addr))) {
	ERROR((msg, "srr_init.bind fails: %d", ERRNO));
	RETURN(FALSE);
    }
    
    /*
     * Need to specifically allow broadcasts.
     */
    {
        int flag = 1;
        if (-1 == setsockopt(srr_socket, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag))) {
            ERROR((msg, "srr_init.setsockopt fails: can't allow broadcasts: %d", ERRNO));
            RETURN(FALSE);
        }
    }

    /*
     * Now fork a thread to execute the receive loop of the srr transport protocol.
     */
    srr_listen_thread = cthread_fork((cthread_fn_t)srr_main, (any_t)0);
    cthread_set_name(srr_listen_thread, "srr_main");
    cthread_detach(srr_listen_thread);

    RETURN(TRUE);

}

