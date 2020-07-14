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

#include <errno.h>

#include <mach/mach.h>

#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "mem.h"
#include "netipc.h"
#include "netmsg.h"
#include "network.h"
#include "srr_defs.h"
#include "transport.h"	/* MEM_TRBUFF */
#include "dispatcher.h"


#ifdef WIN32
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif

/*
 * srr_handle_crypt_failure
 *	handles a packet sent by the destination
 *	when the destination could not decrypt a request.
 *
 * Parameters:
 *	in_packet_ptr	: a pointer to the packet that was received.
 *
 * Returns:
 *	a pointer to a packet that srr_main can reuse.
 *
 * Side effects:
 *	Should abort a request.
 *
 * Design:
 *	Locate the request record.
 *	If it matches the crypt failure packet then abort the request.
 *
 */
PUBLIC srr_packet_ptr_t srr_handle_crypt_failure(in_packet_ptr, host_id)
srr_packet_ptr_t	in_packet_ptr;
netaddr_t		host_id;
{
    srr_host_info_ptr_t	host_info;

    /*
     * Lookup the host record.
     */
    if ((host_info = srr_hash_lookup(host_id)) == SRR_NULL_HOST_INFO) {
	ERROR((msg, "srr_handle_crypt_failure.srr_hash_lookup fails for %ld.", host_id))
	RETURN(in_packet_ptr);
    }

    /*
     * Lock the host record before proceeding.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Check that this crypt failure packet is in response to the current request.
     */
    if ((host_info->shi_request_uid.srr_uid_incarnation == in_packet_ptr->srr_pkt_uid.srr_uid_incarnation)
      && (host_info->shi_request_uid.srr_uid_sequence_no == in_packet_ptr->srr_pkt_uid.srr_uid_sequence_no)
	&& (host_info->shi_request_status == SRR_AWAITING_RESPONSE))
    {
	/*
	 * Abort the request by setting the request packet to NULL.
	 */
	MEM_DEALLOCOBJ(host_info->shi_request_q_head->srq_request_packet, MEM_TRBUFF);
	host_info->shi_request_status = SRR_REMOTE_CRYPT_FAILURE;
	host_info->shi_request_q_head->srq_request_packet = SRR_NULL_PACKET;
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(in_packet_ptr);


}


/*
 * srr_handle_request
 *	Handle an incoming request packet.
 *
 * Parameters:
 *	in_packet_ptr	: a pointer to the packet that was received.
 *	data_size	: how many bytes of srr data this packet contains.
 *	crypt_level	: the encryption level of this packet.
 *	broadcast	: was this a broadcast request.
 *
 * Results:
 *	Either a pointer to a packet that srr_main can use for
 *	receiving the next packet or a null pointer.
 *
 * Side effects:
 *	Should send out the response packet which is the result of this request.
 *
 * Design:
 *	Look up the host record and create a new one if necessary.
 *	If this is a duplicate request,
 *	then we just send the response packet that is contained in the host record.
 *	Otherwise the dispatcher module is called to execute the request
 *	and then we will send out the reply returned from the dispatcher call.
 *	If there is an old response sitting in the host record,
 *	then we return it as our result for reuse by srr_main.
 *	
 * Notes:
 *	We assume that the call to the dispatcher will be executed synchronously
 *	and that the results to be sent in response are returned to us in the
 *	same buffer that we passed in.
 *	If the request was broadcast then we must use the broadcast response uid
 *	and packet contained in the host information record.
 *	The host info record should not be locked when we call disp_rr_simple.
 *
 */
PUBLIC srr_packet_ptr_t srr_handle_request(in_packet_ptr, data_size, crypt_level, broadcast, host_id)
srr_packet_ptr_t	in_packet_ptr;
int			data_size;
int		 	crypt_level;
boolean_t		broadcast;
netaddr_t		host_id;
{
    srr_host_info_ptr_t	host_info;
    kern_return_t	kr;
    long		stored_incarnation, stored_sequence_no;
    long		incoming_incarnation, incoming_sequence_no;

    /*
     * Lookup the host record (maybe create a new one).
     */
    if ((host_info = srr_hash_lookup(host_id)) == SRR_NULL_HOST_INFO) {
	if ((host_info = srr_hash_enter(host_id)) == SRR_NULL_HOST_INFO) {
	    ERROR((msg, "srr_handle_request.srr_hash_enter fails for host = %ld.", host_id));
	    RETURN(in_packet_ptr);
	}
    }

    /*
     * Lock the host record before proceeding.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Check to see if this is a new or a duplicate request.
     * It is a new request if:
     *     this is a new host record,
     *     the stored incarnation number is less than the incoming value, or
     *     the incarnation numbers match and the stored sequence number is less than the incoming value.
     * It is a duplicate request if the incarnation and sequence numbers all match.
     * Otherwise the request is obsolete and is ignored.
     */
    if (broadcast) {
	stored_incarnation = host_info->shi_bcast_response_uid.srr_uid_incarnation;
	stored_sequence_no = host_info->shi_bcast_response_uid.srr_uid_sequence_no;
    }
    else {
	stored_incarnation = host_info->shi_response_uid.srr_uid_incarnation;
	stored_sequence_no = host_info->shi_response_uid.srr_uid_sequence_no;
    }
    incoming_incarnation = in_packet_ptr->srr_pkt_uid.srr_uid_incarnation;
    incoming_sequence_no = in_packet_ptr->srr_pkt_uid.srr_uid_sequence_no;
    if ((stored_incarnation == 0)
	|| (stored_incarnation < incoming_incarnation)
	|| ((stored_incarnation == incoming_incarnation)
	    && (stored_sequence_no < incoming_sequence_no)))
    {
	/*
	 * Handle a new request
	 * Call the dispatcher and pass it the incoming request.
	 * Replace the old response packet by the new one and send the new one.
	 */
	srr_packet_ptr_t	old_response_packet;
	netaddr_t		source_host;
	sbuf_t			sbuf;
	sbuf_seg_t		sbuf_seg;
	int			packet_type;

	source_host = host_id;
	SBUF_SEG_INIT(sbuf, &sbuf_seg);
	SBUF_APPEND(sbuf, in_packet_ptr->srr_pkt_data, data_size);
	mutex_unlock(&host_info->shi_lock);
	kr = disp_rr_simple(&sbuf, source_host, broadcast, crypt_level);
	if (kr == DISP_FAILURE) {
	    ERROR((msg, "srr_handle_request.disp_rr_simple fails (disp_hdr = %lx).", *(unsigned long *)in_packet_ptr->srr_pkt_data));
	    RETURN(in_packet_ptr);
	}
	else if (kr == DISP_IGNORE) {
	    RETURN(in_packet_ptr);
	}
	else {
	    /*
	     * Send a response packet (make it in place).
	     */
	    mutex_lock(&host_info->shi_lock);
	    SBUF_GET_SIZE(sbuf, data_size);
	    packet_type = (broadcast) ? SRR_BCAST_RESPONSE : SRR_RESPONSE;
	    SRR_SET_PKT_HEADER(in_packet_ptr, data_size, source_host, packet_type, crypt_level);
	    HTON_SRR_UID(in_packet_ptr->srr_pkt_uid);

	    if (crypt_level != CRYPT_DONT_ENCRYPT) {
		kr = crypt_encrypt_packet((netipc_ptr_t)in_packet_ptr, crypt_level);
	    }
	    else kr = CRYPT_SUCCESS;

	    if (kr == CRYPT_SUCCESS) {
		if (-1 == srr_send_packet(in_packet_ptr)) {
		    ERROR((msg, "srr_handle_request.srr_send_packet fails: %d", ERRNO));
		}
		else INCSTAT(srr_replies_sent);

		/*
		 * Replace the information about the previous response with the new status.
		*/
		if (broadcast) {
		    old_response_packet = host_info->shi_bcast_response_packet;
		    host_info->shi_bcast_response_packet = in_packet_ptr;
		    host_info->shi_bcast_response_uid.srr_uid_incarnation = incoming_incarnation;
		    host_info->shi_bcast_response_uid.srr_uid_sequence_no = incoming_sequence_no;
		}
		else {
		    old_response_packet = host_info->shi_response_packet;
		    host_info->shi_response_packet = in_packet_ptr;
		    host_info->shi_response_uid.srr_uid_incarnation = incoming_incarnation;
		    host_info->shi_response_uid.srr_uid_sequence_no = incoming_sequence_no;
		}
	    }
	    else {
		/*
		 * Encryption failed - cannot send this response.
		 * Since we were just now able to decrypt the request,
		 * there should be no reason why we should fail to encrypt the response.
		 */
		ERROR((msg, "srr_handle_request.crypt_encrypt_packet failed, kr = %d.", kr));
		old_response_packet = in_packet_ptr;
	    }

	    mutex_unlock(&host_info->shi_lock);
	    RETURN(old_response_packet);
	}
    }
    else if ((stored_incarnation == incoming_incarnation)
	    && (stored_sequence_no == incoming_sequence_no))
    {
	/*
	 * Handle a duplicate request.
	 * Resend the response packet stored in the host record.
	 */
	INCSTAT(srr_retries_rcvd);
	if (broadcast) {
	    if (-1 == srr_send_packet(host_info->shi_bcast_response_packet))
	    {
		ERROR((msg, "srr_handle_request.srr_send_packet fails: %d", ERRNO));
	    }
	}
	else {
	    if (-1 == srr_send_packet(host_info->shi_response_packet)) {
		ERROR((msg, "srr_handle_request.srr_send_packet fails: %d", ERRNO));
	    }
	    else INCSTAT(srr_replies_sent);
	}
	mutex_unlock(&host_info->shi_lock);
	RETURN(in_packet_ptr);
    }
    else {
	/*
	 * Ignore an obsolete request.
	 */
	mutex_unlock(&host_info->shi_lock);
	RETURN(in_packet_ptr);
    }

}

/*
 * srr_handle_response
 *	Handle an incoming response packet.
 *
 * Parameters:
 *	in_packet_ptr	: a pointer to the packet that was received.
 *	data_size	: how many bytes of srr data this packet contains.
 *	crypt_level	: the encryption level of this packet.
 *	broadcast	: whether this response was broadcast
 *
 * Results:
 *	Either a pointer to a packet that srr_main can use for
 *	receiving the next packet or a null pointer.
 *
 * Side effects:
 *	May send out another request packet if there is one waiting to be sent.
 *
 * Design:
 *	Look up the host record
 *	(use the host record for the broadcast address if this is a broadcast response).
 *	If this is an old or duplicate response then we just ignore it.
 *	Otherwise we return the response to the client
 *	and send out the next request if there is one waiting.
 *
 * Notes:
 *	We assume that the dispatcher call to the client returns synchronously
 *	so that we can free the incoming packet buffer immediately.
 *	An aborted transmission is indicated by a null request packet.
 *	The host info record should not be locked when we call disp_indata_simple.
 *
 */
PUBLIC srr_packet_ptr_t srr_handle_response(in_packet_ptr, data_size, crypt_level, broadcast, sender_addr)
srr_packet_ptr_t	in_packet_ptr;
int			data_size;
int			crypt_level;
boolean_t		broadcast;
netaddr_t		sender_addr;
{
    srr_host_info_ptr_t	host_info;
    netaddr_t		host_id;
    kern_return_t	kr;

    /*
     * Lookup the host record.
     */
    if (broadcast) {
	host_id = broadcast_address;
    } else {
	host_id = sender_addr;
    }
    if ((host_info = srr_hash_lookup(host_id)) == SRR_NULL_HOST_INFO) {
	ERROR((msg, "srr_handle_response.srr_hash_lookup fails for %d.", (int)host_id));
	RETURN(in_packet_ptr);
    }

    /*
     * Lock the host record before proceeding.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Check to see if this is the response that we are expecting.
     */
    if ((host_info->shi_request_uid.srr_uid_incarnation == in_packet_ptr->srr_pkt_uid.srr_uid_incarnation)
      && (host_info->shi_request_uid.srr_uid_sequence_no == in_packet_ptr->srr_pkt_uid.srr_uid_sequence_no)
	&& (host_info->shi_request_status == SRR_AWAITING_RESPONSE))
    {
	/*
	 * Check to see that this request was not already aborted.
         */
	if (host_info->shi_request_q_head->srq_request_packet != SRR_NULL_PACKET) {
	    /*
	     * Call the dispatcher with the response.
	     */
	    netaddr_t		source_host;
	    boolean_t		pkt_broadcast;
	    int			client_id;
	    sbuf_t		sbuf;
	    sbuf_seg_t		sbuf_seg;

	    source_host = sender_addr;
	    pkt_broadcast = broadcast;
	    client_id = host_info->shi_request_q_head->srq_client_id;

	    SBUF_SEG_INIT(sbuf, &sbuf_seg);
	    SBUF_APPEND(sbuf, in_packet_ptr->srr_pkt_data, data_size);

	    mutex_unlock(&host_info->shi_lock);
	    kr = disp_indata_simple(client_id, &sbuf, source_host, pkt_broadcast, crypt_level);
	    mutex_lock(&host_info->shi_lock);
	}
	else kr = DISP_SUCCESS;

	if (kr == DISP_SUCCESS) {
	    /*
	     * Dequeue the successful request.
	     */
	    srr_request_q_ptr_t	old_request;

	    if ((old_request = srr_dequeue(host_info)) == SRR_NULL_Q) {
		ERROR((msg, "srr_handle_response.srr_dequeue returns SRR_NULL_Q."));
		mutex_unlock(&host_info->shi_lock);
		RETURN(in_packet_ptr);
	    }
	    if (old_request->srq_request_packet != SRR_NULL_PACKET) {
		MEM_DEALLOCOBJ(old_request->srq_request_packet, MEM_TRBUFF);
	    }
	    MEM_DEALLOCOBJ(old_request, MEM_SRRREQ);
	    host_info->shi_request_status = SRR_HAVE_RESPONSE;

	    /*
	     * If there is another request waiting transmission
	     * send it off and queue it with the timer service.
	     */
	    if (host_info->shi_request_q_head != SRR_NULL_Q) {
		srr_process_queued_request(host_info);
	    }
	}
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(in_packet_ptr);

}


/*
 * srr_process_queued_request
 *
 * Parameters:
 *	host_info : the host record for which there is a new request to be sent off.
 *
 * Side effects:
 *	May send off a packet.
 *
 * Note:
 *	Assume that the host record is already locked.
 *
 */
PUBLIC void srr_process_queued_request(host_info)
srr_host_info_ptr_t host_info;
{
    srr_packet_ptr_t	request_packet_ptr;
    int			crypt_level;
    kern_return_t	kr;

    request_packet_ptr = host_info->shi_request_q_head->srq_request_packet;

    /*
     * Check to see if there is really a packet awaiting transmission.
     */
    if (request_packet_ptr != SRR_NULL_PACKET) {
	/*
	 * Update the srr uid.
	 */
	host_info->shi_request_uid.srr_uid_sequence_no ++;
	request_packet_ptr->srr_pkt_uid = host_info->shi_request_uid;
	HTON_SRR_UID(request_packet_ptr->srr_pkt_uid);

	/*
	 * Maybe this packet should be encrypted.
	 */
	crypt_level = ntohl(request_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level);
	if (crypt_level != CRYPT_DONT_ENCRYPT) {
	    kr = crypt_encrypt_packet((netipc_ptr_t)request_packet_ptr, crypt_level);
	}
	else kr = CRYPT_SUCCESS;

	if (kr == CRYPT_SUCCESS) {
	    /*
	     * Actually try to send this request packet.
	     */
	    if (-1 == srr_send_packet(request_packet_ptr)) {
		ERROR((msg, "srr_process_queued_request.srr_send_packet fails: %d", ERRNO));
	    }
	    else {
		INCSTAT(srr_requests_sent);
		if (request_packet_ptr->srr_pkt_type == SRR_BCAST_REQUEST) INCSTAT(srr_bcasts_sent);
	    }
	    host_info->shi_request_status = SRR_AWAITING_RESPONSE;
	    host_info->shi_request_tries = 1;
	    /* It would also be possible to explicitly remove the timeout
	     * entry on a response, and then do a timer_start here rather then
	     * the timer_restart. Both solve the problem at hand. We may want
	     * to adopt that approach.
	     */
	    timer_restart(&host_info->shi_timer);
	}
	else {
	    /*
	     * Abort the transmission of this request and set the retry interval to 0.
	     */
	    MEM_DEALLOCOBJ(request_packet_ptr, MEM_TRBUFF);
	    host_info->shi_request_q_head->srq_request_packet = SRR_NULL_PACKET;
	    host_info->shi_request_status = SRR_LOCAL_CRYPT_FAILURE;
	    host_info->shi_timer.interval.tv_sec = host_info->shi_timer.interval.tv_usec = 0;
	    timer_restart(&host_info->shi_timer);
	    timer_wake_up();
	}
    }

    RET;

}


/*
 * srr_send_crypt_failure
 *	Send out a packet to inform a source host that we were unable to decrypt a packet.
 *
 * Parameters:
 *	in_packet_ptr	: the packet that caused the decryption failure
 *	crypt_level	: the encryption level of that packet
 *
 * Side effects:
 *	Send out a packet.
 *
 * Note:
 *	We use the in_packet_ptr to send out the decryption failure packet.
 *	However we negate the crypt_level to inform the source of the failure.
 *
 */
PUBLIC void srr_send_crypt_failure(in_packet_ptr, crypt_level, source_host)
srr_packet_ptr_t	in_packet_ptr;
int			crypt_level;
netaddr_t		source_host;
{
    if (in_packet_ptr == SRR_NULL_PACKET) RET;

    in_packet_ptr->srr_pkt_dest = source_host;
    in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level = htonl((unsigned long)-crypt_level);
    if (-1 == srr_send_packet(in_packet_ptr)) {
	ERROR((msg, "srr_send_crypt_failure.srr_send_packet fails: %d", ERRNO));
    } else
	INCSTAT(srr_cfailures_sent);

    RET;

}
