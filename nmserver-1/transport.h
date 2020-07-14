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

#ifndef	_TRANSPORT_
#define	_TRANSPORT_

#include "mem.h"

/*
 * Type of transport service required by a client.
 */
#define	TRSERV_NORMAL		1
#define	TRSERV_URGENT		2
#define TRSERV_IPC		3
#define	TRSERV_RPC		4

/*
 * Defined entry points to transport protocols.
 */
#define	TR_NOOP_ENTRY		0
#define TR_DELTAT_ENTRY		1
#define TR_VMTP_ENTRY		2
#define TR_DATAGRAM_ENTRY	3
#define TR_SRR_ENTRY		4
#define	TR_VMTP1_ENTRY		5
#define	TR_VMTP2_ENTRY		6
#define	TR_TCP_ENTRY		7
#define	TR_MAX_ENTRY		(TR_TCP_ENTRY + 1)

/*
 * Generic codes returned by a transport module. These codes are in the same
 * space as the DISP_* and IPC_* codes.
 */
#define	TR_SUCCESS		-10
#define	TR_FAILURE		-11
#define TR_REMOTE_ACCEPT	-12
#define	TR_REMOTE_REJECT	-13
#define TR_CRYPT_FAILURE	-14
#define TR_SEND_FAILURE		-15
#define	TR_OVERLOAD		-16


/*
 * Entry points to transport level protocols.
 */
typedef struct {
    int		(*send)();
    int		(*sendrequest)();
    int		(*sendreply)();
} transport_sw_entry_t;

extern transport_sw_entry_t transport_switch[TR_MAX_ENTRY];

extern int	transport_no_function();

#define	tr_default_entry 	param.transport_default

/*
 * UDP Ports used by the transport level protocols.
 */
#define DELTAT_UDP_PORT		7654
#define VMTP_UDP_PORT		7655
#define DATAGRAM_UDP_PORT	7656
#define SRR_UDP_PORT		7657

/*
 * Macros to call transport modules.
 */
#define	transport_sendrequest(trmod,clid,data,to,crypt,reply)			\
	(transport_switch[(trmod)].sendrequest((clid),(data),(to),(crypt),(reply)))

#define	transport_sendreply(trmod,trid,code,data,crypt)				\
	(transport_switch[(trmod)].sendreply((trid),(code),(data),(crypt)))

/*
 * Memory management for MEM_TRBUFF.
 */
extern mem_objrec_t		MEM_TRBUFF;


#endif	_TRANSPORT_
