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

#ifndef	_SRR_DEFS_
#define	_SRR_DEFS_

/*
 * Definitions for packets sent out over the network.
 */
#include "mem.h"
#include "netipc.h"

typedef struct srr_uid_ {
    unsigned long		srr_uid_incarnation;
    unsigned long		srr_uid_sequence_no;
} srr_uid_t;

#define HTON_SRR_UID(uid) {						\
    (uid).srr_uid_incarnation = htonl((uid).srr_uid_incarnation);	\
    (uid).srr_uid_sequence_no = htonl((uid).srr_uid_sequence_no);	\
}

#define NTOH_SRR_UID(uid) {						\
    (uid).srr_uid_incarnation = ntohl((uid).srr_uid_incarnation);	\
    (uid).srr_uid_sequence_no = ntohl((uid).srr_uid_sequence_no);	\
}


#define SRR_HEADER_SIZE		(sizeof(long) + sizeof(srr_uid_t))
#define SRR_MAX_DATA_SIZE	(OLD_NETIPC_MAX_DATA_SIZE - SRR_HEADER_SIZE)

typedef struct srr_packet_ {
    netipc_header_t	srr_pkt_header;
    unsigned long	srr_pkt_type;
    srr_uid_t		srr_pkt_uid;
    char		srr_pkt_data[SRR_MAX_DATA_SIZE+8];
    netaddr_t		srr_pkt_dest;
} srr_packet_t, *srr_packet_ptr_t;

#define SRR_NULL_PACKET	((srr_packet_ptr_t)0)

/*
 * SRR Packet Types
 */
#define SRR_REQUEST		0
#define SRR_RESPONSE		1
#define SRR_BCAST_REQUEST	2
#define SRR_BCAST_RESPONSE	3
#define SRR_CRYPT_FAILURE	4

/*
 * Template for srr packet header.
 */
extern netipc_header_t		srr_template;

/*
 * Macro for filling in an srr packet header.

 */

#define SRR_SET_PKT_HEADER(pkt_ptr,size,dest,type,crypt_level) {			\
    (pkt_ptr)->srr_pkt_type = (long)htonl((unsigned long)(type));			\
    (pkt_ptr)->srr_pkt_header.nih_crypt_header.ch_checksum = 0;				\
    (pkt_ptr)->srr_pkt_header.nih_crypt_header.ch_crypt_level =				\
	htonl((unsigned long)crypt_level);						\
    (pkt_ptr)->srr_pkt_header.nih_crypt_header.ch_data_size =				\
	htons((unsigned short)(size) + SRR_HEADER_SIZE);				\
    (pkt_ptr)->srr_pkt_dest = dest;							\
}




/*
 * Definitions for information maintained about hosts.
 */
#include <mach/mach.h>
#include <mach/cthreads.h>
#include "sbuf.h"
#include "timer.h"

/*
 * The queue of requests awaiting transmission.
 */
typedef struct srr_request_q_ {
    struct srr_request_q_	*srq_next;
    srr_packet_ptr_t		srq_request_packet;
    netaddr_t			srq_destination;
    int				srq_client_id;
    int				(*srq_cleanup)();
} srr_request_q_t, *srr_request_q_ptr_t;

#define SRR_NULL_Q	((srr_request_q_ptr_t)0)

/*
 * Information about one host.
 */
typedef struct srr_host_info_ {
    netaddr_t		shi_host_id;
    struct mutex	shi_lock;
    /* Information about outstanding requests. */
    int			shi_request_status;
    int			shi_request_tries;
    srr_uid_t		shi_request_uid;
    srr_request_q_ptr_t	shi_request_q_head;
    srr_request_q_ptr_t	shi_request_q_tail;
    struct timer	shi_timer;
    /* Information for a response. */
    srr_uid_t		shi_response_uid;
    srr_packet_ptr_t	shi_response_packet;
    /* Information for a response to a broadcast request. */
    srr_uid_t		shi_bcast_response_uid;
    srr_packet_ptr_t	shi_bcast_response_packet;
} srr_host_info_t, *srr_host_info_ptr_t;

#define SRR_NULL_HOST_INFO	((srr_host_info_ptr_t)0)

/*
 * Status codes for host records.
 */
#define SRR_INACTIVE			0
#define SRR_AWAITING_RESPONSE		1
#define SRR_HAVE_RESPONSE		2
#define SRR_LOCAL_CRYPT_FAILURE		3
#define SRR_REMOTE_CRYPT_FAILURE	4
#define SRR_LOCAL_ABORT			5
#define SRR_REMOTE_ABORT		5

/*
 * The retry function.
 */
extern int srr_retry();

/*
 * Send a packet to the network (using the socket interface).
 */
extern int srr_send_packet ();
/*
srr_packet_ptr_t packet_ptr;
*/



/*
 * External definitions of funtions implemented by srr_handler.c
 */

extern srr_packet_ptr_t srr_handle_crypt_failure();
/*
srr_packet_ptr_t in_packet_ptr;
int crypt_level;
netaddr_t source_host;
*/

extern srr_packet_ptr_t srr_handle_request();
/*
srr_packet_ptr_t	in_packet_ptr;
int			data_size;
int			crypt_level;
boolean_t		broadcast;
netaddr_t source_host;
*/

extern srr_packet_ptr_t srr_handle_response();
/*
srr_packet_ptr_t	in_packet_ptr;
int			data_size;
int			crypt_level;
boolean_t		broadcast;
netaddr_t source_host;
*/

extern void srr_process_queued_request();
/*
srr_host_info_ptr_t	host_info;
*/

extern void srr_send_crypt_failure();
/*
srr_packet_ptr_t in_packet_ptr;
int crypt_level;
netaddr_t source_host;
*/


/*
 * External definitions of funtions implemented by srr_utils.c
 */

void srr_utils_init();

srr_host_info_ptr_t srr_hash_lookup();
/*
netaddr_t host_id;
*/

srr_host_info_ptr_t srr_hash_enter();
/*
netaddr_t host_id;
*/

void srr_enqueue();
/*
srr_request_q_ptr_t	request_ptr;
srr_host_info_ptr_t	host_record;
*/

srr_request_q_ptr_t srr_dequeue();
/*
srr_host_info_ptr_t	host_record;
*/

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_SRRREQ;


#endif	_SRR_DEFS_
