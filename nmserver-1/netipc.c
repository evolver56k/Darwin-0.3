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
 *
 * Copyright (c) 1994 NeXT Computer, Inc. All rights reserved.
 */


#define NETIPC_DEBUG	0

#include <mach/mach.h>
#include <mach/message.h>

#if 0

#include "crypt.h"
#include "debug.h"
#include "netipc.h"
#include "netmsg.h"
#include "network.h"
#include <servers/nm_defs.h>
#include "nm_extra.h"


/*
 * netipc_receive
 *	Receive a packet over the network using netmsg_receive.
 *	Reject a packet from ourself otherwise check its UDP checksum
 *	if it is not encrypted.
 *
 * Parameters:
 *	pkt_ptr	: pointer to a buffer to hold the packet
 *
 * Results:
 *	NETIPC_BAD_UDP_CHECKSUM or the return code from netmsg_receive.
 *
 */
EXPORT netipc_receive(pkt_ptr)
register netipc_header_ptr_t	pkt_ptr;
{
    register kern_return_t	mr;
    register int		old_msg_size;

    old_msg_size = pkt_ptr->nih_msg_header.msg_size;
    while (1)
    {
	mr = netmsg_receive((msg_header_t *)pkt_ptr);
	if (mr != RCV_SUCCESS) {
	    RETURN(mr);
	}
	else if (pkt_ptr->nih_ip_header.ip_src.s_addr == my_host_id) {
	    pkt_ptr->nih_msg_header.msg_size = old_msg_size;
	}
	else if ((pkt_ptr->nih_crypt_header.ch_crypt_level == CRYPT_DONT_ENCRYPT)
		&& (pkt_ptr->nih_udp_header.uh_sum))
	{
	    /*
	     * Not from ourself - check the UDP Checksum.
	     */
	    pkt_ptr->nih_ip_header.ip_ttl = 0;
	    pkt_ptr->nih_ip_header.ip_sum = pkt_ptr->nih_udp_header.uh_ulen;
	    if (pkt_ptr->nih_udp_header.uh_sum = udp_checksum(
	    	(unsigned short *) &(pkt_ptr->nih_ip_header.ip_ttl),
		(pkt_ptr->nih_ip_header.ip_len - 8)))
	    {
		RETURN(NETIPC_BAD_UDP_CHECKSUM);
	    }
	    else {
		RETURN(RCV_SUCCESS);
	    }
	}
	else {
	    RETURN(RCV_SUCCESS);
	}
    }

}


/*
 * netipc_send
 *	Calculate a UDP checksum for a packet if the packet is not encrypted.
 *	Insert a new ip_id into the IP header.
 *	Send the packet over the network using msg_send.
 *
 * Parameters:
 *	pkt_ptr	: pointer to the packet to be sent.
 *
 * Results:
 *	value returned by msg_send.
 *
 */
EXPORT netipc_send(pkt_ptr)
register netipc_header_ptr_t	pkt_ptr;
{
    register short		saved_ttl;
    register msg_return_t	mr;

    if (pkt_ptr->nih_crypt_header.ch_crypt_level == CRYPT_DONT_ENCRYPT) {
	/*
	* Stuff the ip header with the right values for the UDP checksum.
	*/
	saved_ttl = pkt_ptr->nih_ip_header.ip_ttl;
	pkt_ptr->nih_ip_header.ip_ttl = 0;
	pkt_ptr->nih_ip_header.ip_sum = pkt_ptr->nih_udp_header.uh_ulen;
	pkt_ptr->nih_udp_header.uh_sum = 0;

	/*
	 * Calculate the checksum and restore the values in the ip header.
	 */
	pkt_ptr->nih_udp_header.uh_sum = udp_checksum(
		(unsigned short *) &(pkt_ptr->nih_ip_header.ip_ttl),
		(pkt_ptr->nih_ip_header.ip_len - 8));
	pkt_ptr->nih_ip_header.ip_ttl = saved_ttl;
	pkt_ptr->nih_ip_header.ip_sum = 0;
    }
    else {
	/*
	 * Tell the destination to ignore the UDP Checksum.
	 */
	pkt_ptr->nih_udp_header.uh_sum = 0;
    }

    /*
     * Insert a new ip id into the header and send the packet.
     */
    pkt_ptr->nih_ip_header.ip_id = last_ip_id ++;
    	NETIPC_DEBUG, 0, 1030, pkt_ptr->nih_msg_header.msg_simple,
	pkt_ptr->nih_msg_header.msg_size,
	pkt_ptr->nih_msg_header.msg_id, pkt_ptr->nih_msg_header.msg_type);
    if ((mr = msg_send((msg_header_t *)pkt_ptr, SEND_TIMEOUT, 0)) != KERN_SUCCESS) {
    }
    RETURN(mr);

}

#endif

