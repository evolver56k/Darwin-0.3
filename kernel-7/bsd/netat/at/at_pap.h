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
 *
 * ORIGINS: 82
 *
 * APPLE CONFIDENTIAL
 * (C) COPYRIGHT Apple Computer, Inc. 1992-1996
 * All Rights Reserved
 *
 */                                                                   

#define	NPAPSERVERS	10	/* the number of active PAP servers/node */
#define	NPAPSESSIONS	40	/* the number of active PAP sockets/node */

#define AT_PAP_HDR_SIZE	(DDP_X_HDR_SIZE + ATP_HDR_SIZE)

#define	 ATP_DDP_HDR(c)	((at_ddp_t *)(c))

#define PAP_SOCKERR 	"Unable to open PAP socket"
#define P_NOEXIST 	"Printer not found"
#define P_UNREACH	"Unable to establish PAP session"

struct pap_state {
	unsigned char pap_inuse; /* true if this one is allocated */
	unsigned char pap_tickle; /* true if we are tickling the other
				     end */
	unsigned char pap_request; /* bitmap from a received request */
	unsigned char pap_eof;	/* true if we have received an EOF */
	unsigned char pap_eof_sent; /* true if we have sent an EOF */
	unsigned char pap_sent; /* true if we have sent anything (and
				   therefore may have to send an eof
				   on close) */
	unsigned char pap_error; /* error message from read request */
	unsigned char pap_timer; /* a timeout is pending */
	unsigned char pap_closing; /* the link is closing and/or closed */
	unsigned char pap_request_count; /* number of outstanding requests */
	unsigned char pap_req_timer; /* the request timer is running */
	unsigned char pap_ending; /* we are waiting for atp to flush */
	unsigned char pap_read_ignore; /* we are in 'read with ignore' mode */

	at_socket     pap_req_socket;
	at_socket     pap_to_socket;
	at_node	      pap_to_node;
	at_net	      pap_to_net;
	int	      pap_flow;

	unsigned short pap_send_count; /* the sequence number to send on the
					  next send data request */
	unsigned short pap_rcv_count; /* the sequence number expected to
					 receive on the next request */
	unsigned short pap_tid; /* ATP transaction ID for responses */
	unsigned char  pap_connID; /* our connection ID */

 	int pap_ignore_id;	/* the transaction ID for read ignore */
	int pap_tickle_id;	/* the transaction ID for tickles */
};
