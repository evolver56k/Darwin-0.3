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


#ifndef	_NETIPC_
#define	_NETIPC_

#include <mach/message.h>

#ifndef WIN32
#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#endif

#include "crypt.h"

/*
 * I must say, this is a really brilliant scheme for network computing in
 * heterogenous environments.  Here's the original header, along with the
 * sizes of each field (taken from an i386 running NS 4.0):
 *
 *  typedef struct {
 *	msg_header_t   nih_msg_header;   24 bytes
 *	struct ip      nih_ip_header;    20 bytes
 *	struct udphdr  nih_udp_header;   8 bytes
 *	crypt_header_t nih_crypt_header; 8 bytes
 *  } netipc_header_t, *netipc_header_ptr_t;
 *
 * We now talk to the network through a socket interface rather than going
 * through the netipc module; toast the extraneous fields in the header...
 *
 */

#define MSG_HEADER_SIZE	24
#define IP_HEADER_SIZE	20
#define UDP_HEADER_SIZE	8

typedef struct {
    crypt_header_t nih_crypt_header;
} netipc_header_t, *netipc_header_ptr_t;


/*
 * Hard code these values for those systems with (1) different basic type sizes
 * (such as an alpha) and/or (2) different or missing header types (Windoze has
 * no struct ip or struct udphdr, and the msg_header_t type is different).  (We
 * can also change the netipc_header_t since we're now going through a socket
 * interface to talk to the network and don't need most of the headers.)  The
 * original values were:
 *
 
#define NETIPC_MAX_PACKET_SIZE		(1500)	Should be ETHERMTU????
#define NETIPC_PACKET_HEADER_SIZE	(sizeof(struct ip) + sizeof(struct udphdr) + CRYPT_HEADER_SIZE)
#define NETIPC_MAX_DATA_SIZE		(NETIPC_MAX_PACKET_SIZE - NETIPC_PACKET_HEADER_SIZE)
#define NETIPC_SWAPPED_HEADER_SIZE	(sizeof(struct udphdr) + CRYPT_HEADER_SIZE)
#define NETIPC_MAX_MSG_SIZE		(NETIPC_MAX_PACKET_SIZE + sizeof(msg_header_t) + 8)

 *
 * The new values (hard-coded) are:
 */

#define OLD_NETIPC_MAX_PACKET_SIZE	1500
#define OLD_NETIPC_PACKET_HEADER_SIZE	(IP_HEADER_SIZE + UDP_HEADER_SIZE + CRYPT_HEADER_SIZE)
#define OLD_NETIPC_MAX_DATA_SIZE	(OLD_NETIPC_MAX_PACKET_SIZE - OLD_NETIPC_PACKET_HEADER_SIZE)
#define OLD_NETIPC_SWAPPED_HEADER_SIZE	(UDP_HEADER_SIZE + CRYPT_HEADER_SIZE)
#define OLD_NETIPC_MAX_MSG_SIZE		(OLD_NETIPC_MAX_PACKET_SIZE + MSG_HEADER_SIZE + 8)

#define NETIPC_MAX_PACKET_SIZE		(1500 - IP_HEADER_SIZE - UDP_HEADER_SIZE)
#define NETIPC_PACKET_HEADER_SIZE	(CRYPT_HEADER_SIZE)
#define NETIPC_MAX_DATA_SIZE		(OLD_NETIPC_MAX_DATA_SIZE)
#define NETIPC_SWAPPED_HEADER_SIZE	(CRYPT_HEADER_SIZE)
#define NETIPC_MAX_MSG_SIZE		(NETIPC_MAX_PACKET_SIZE + 8)

typedef struct {
    netipc_header_t	ni_header;
    char		ni_data[OLD_NETIPC_MAX_DATA_SIZE];
} netipc_t, *netipc_ptr_t;

#define NETIPC_MSG_ID	1959



#endif	_NETIPC_
