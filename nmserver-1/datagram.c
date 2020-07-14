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

#include <errno.h>

#include "crypt.h"
#include "debug.h"
#include "datagram.h"
#include "disp_hdr.h"
#include "mem.h"
#include "netipc.h"
#include "network.h"
#include "nm_extra.h"
#include "sbuf.h"
#include "transport.h"
#include "uid.h"
#include "dispatcher.h"

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <string.h>
#ifdef NEXT_CRT
#include <winsock.h>
#endif NEXT_CRT
#endif WIN32

static int			datagram_socket;
static cthread_t		datagram_listen_thread;
static netipc_ptr_t		datagram_out_message;

PRIVATE void datagram_main();

int datagram_max_data_size = OLD_NETIPC_MAX_DATA_SIZE;

#ifdef WIN32
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif


/*
 * Minimum size of a meaningful datagram packet.
 */
#define MIN_DATAGRAM_PACKET_LENGTH (sizeof(crypt_header_t) + sizeof(disp_hdr_t))


/*
 * datagram_init
 *	Initialises the datagram transport protocol.
 *
 * Results:
 *	FALSE : we failed to initialise the datagram transport protocol.
 *	TRUE  : we were successful.
 *
 * Side effects:
 *	Initialises the datagram protocol entry point in the switch array.
 *	Initialises the template for sending datagrams.
 *	Allocates the datagram socket and creates a thread to listen to the network.
 *
 */
EXPORT boolean_t datagram_init()
{
    struct sockaddr_in	addr;

    /*
     * Setup the transport layer switching.
     */
    transport_switch[TR_DATAGRAM_ENTRY].send = datagram_send;

    /*
     * Create a socket bound to the datagram port.
     */
    datagram_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == datagram_socket) {
	ERROR((msg, "datagram_init.socket fails: %d", ERRNO));
	RETURN(FALSE);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DATAGRAM_UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (-1 == bind(datagram_socket, (struct sockaddr *)&addr, sizeof(addr))) {
	ERROR((msg, "datagram_init.bind fails: %d", ERRNO));
	RETURN(FALSE);
    }

    /*
     * Need to specifically allow broadcasts.
     */
    {
	int flag = 1;
	if (-1 == setsockopt(datagram_socket, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag))) {
	    ERROR((msg, "datagram_init.setsockopt fails: can't allow broadcasts: %d", ERRNO));
	    RETURN(FALSE);
	}
    }

    /*
     * Pre-allocate a buffer for use when sending datagrams.
     */
    MEM_ALLOCOBJ(datagram_out_message,netipc_ptr_t,MEM_TRBUFF);

    /*
     * Now fork a thread to execute the receive loop of the datagram transport protocol.
     */
    datagram_listen_thread = cthread_fork((cthread_fn_t)datagram_main, (any_t)0);
    cthread_set_name(datagram_listen_thread, "datagram_main");
    cthread_detach(datagram_listen_thread);

    RETURN(TRUE);

}


/*
 * datagram_main
 *	Main loop of datagram transport protocol
 *	Waits for incoming packets contained in IPC messages
 *	and calls the dispatcher to handle them.
 *
 * Note:
 *	It is assumed that the data in the incoming packet is no longer
 *	needed by the higher-level routine to which it was given by the
 *	disp_inmsg_simple function after disp_inmsg_simple returns.
 *	In other words we can reuse the buffer for the next datagram.
 */
PRIVATE void datagram_main()
{
    netipc_ptr_t	in_pkt_ptr;
    kern_return_t	kr;
    sbuf_t		in_sbuf;
    sbuf_seg_t		in_sbuf_seg;

    MEM_ALLOCOBJ(in_pkt_ptr,netipc_ptr_t,MEM_TRBUFF);

    SBUF_SEG_INIT(in_sbuf, &in_sbuf_seg);

    while (TRUE) {
	int data_size, crypt_level, length, addr_length;
	netaddr_t source_host;
	struct sockaddr_in addr;

	addr_length = sizeof(addr);
	length = recvfrom(datagram_socket, (char *)in_pkt_ptr, sizeof(netipc_t), 0, (struct sockaddr *)&addr, &addr_length);

	if (-1 == length) {
	    ERROR((msg, "datagram_main.recvfrom fails: %d", ERRNO));
	    continue;
	}
		
	if (length < MIN_DATAGRAM_PACKET_LENGTH) {
	    ERROR((msg, "datagram_main.recvfrom packet too short, len = %d", length));
	    continue;
	}

	source_host = addr.sin_addr.s_addr;
	if (source_host == my_host_id) {
	    /* Ignore local broadcasts.  What if we have >1 network interface? */
	    continue;
	}
	    
	INCSTAT(datagram_pkts_rcvd);

	crypt_level = ntohl(in_pkt_ptr->ni_header.nih_crypt_header.ch_crypt_level);
	if (crypt_level != CRYPT_DONT_ENCRYPT) {
	    kr = crypt_decrypt_packet(in_pkt_ptr, crypt_level);
	} else
	    kr = CRYPT_SUCCESS;

	if (kr == CRYPT_SUCCESS) {
	    data_size = ntohs(in_pkt_ptr->ni_header.nih_crypt_header.ch_data_size);
	    
	    /*
	     * XXX way too many dependencies on type sizes being the same
	     * across heterogenous architectures.  Network types should be totally
	     * seperate from the local language types.
	     */
	    if ((data_size < sizeof(disp_hdr_t))
		|| (data_size > OLD_NETIPC_MAX_DATA_SIZE)
		|| (data_size > length)) {
		ERROR((msg, "datagram_main.netipc_receive data_size invalid, data_size = %d.",
		       data_size));
	    } else {
		/*
		 * XXX I'm assuming datagram packets are always broadcast; there is no
		 * way using the socket api to detect whether a packet was received
		 * directly or as the result of a broadcast.
		 */
		SBUF_REINIT(in_sbuf);
		SBUF_APPEND(in_sbuf, in_pkt_ptr->ni_data, data_size);
		kr = disp_indata_simple(0, (sbuf_ptr_t)&(in_sbuf), source_host, TRUE, crypt_level);
		if (kr != DISP_SUCCESS) {
		}
	    }
	}

	LOGCHECK;
    }
}


/*
 * datagram_send
 *	Sends a datagram over the network.
 *
 * Parameters:
 *	to		: the host to which the datagram is to be sent.
 *	crypt_level	: whether this packet should be encrypted.
 *
 * Returns:
 *	TR_SUCCESS or a specific failure code.
 *
 * Note:
 *	cleanup is never called as this routine is guaranteed to have done
 *	with the input sbuf after it returns.
 *
 */
/* ARGSUSED */
EXPORT int datagram_send(client_id, data, to, service, crypt_level, cleanup)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	to;
int		service;
int		crypt_level;
int		(*cleanup)();
{
    unsigned short	size;
    int			length;
    struct sockaddr_in	addr;

    /*
     * Copy the input sbuf into our local buffer (datagram_out_message).
     */
    SBUF_FLATTEN(data, datagram_out_message->ni_data, size);

    /*
     * Sanity check.
     */
    if (size > OLD_NETIPC_MAX_DATA_SIZE) {
	RETURN(DATAGRAM_TOO_LARGE);
    }

    /*
     * Fill in the netipc header.
     */
    datagram_out_message->ni_header.nih_crypt_header.ch_crypt_level = htonl((unsigned long)crypt_level);
    datagram_out_message->ni_header.nih_crypt_header.ch_data_size = htons(size);
    datagram_out_message->ni_header.nih_crypt_header.ch_checksum = 0;
    
    /*
     * Fill out the destination address.  (XXX could always cache it.)
     */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DATAGRAM_UDP_PORT);
    addr.sin_addr.s_addr = to;

    /*
     * Check to see whether we should encrypt this datagram.
     */
    if (crypt_level != CRYPT_DONT_ENCRYPT) {
	if (CRYPT_SUCCESS != crypt_encrypt_packet(datagram_out_message, crypt_level))
	    RETURN(TR_CRYPT_FAILURE);
    }
    
    /*
     * Hit the airwaves...  Since we're going through the socket interface,
     */
    length = sendto(datagram_socket, (char *)datagram_out_message, NETIPC_PACKET_HEADER_SIZE + size, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == length) {
	ERROR((msg, "datagram_send.sendto fails: %d", ERRNO));
	RETURN(TR_SEND_FAILURE);
    } else if (length < NETIPC_PACKET_HEADER_SIZE + size) {
	ERROR((msg, "datagram_send.sendto fails: %d/%d bytes sent", length, sizeof(crypt_header_t) + size));
	RETURN(TR_SEND_FAILURE);
    }

    INCSTAT(datagram_pkts_sent);
    RETURN(TR_SUCCESS);
}

