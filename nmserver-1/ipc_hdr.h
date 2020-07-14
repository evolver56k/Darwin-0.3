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

#ifndef	_IPC_HDR_
#define	_IPC_HDR_

#include	"disp_hdr.h"
#include	"port_defs.h"

/*
 * Header for network IPC messages 
 */
typedef struct {
	disp_hdr_t      disp_hdr;	/* dispatcher header */
	network_port_t  local_port;
	network_port_t  remote_port;
	unsigned long   info;		/* info bits */
	unsigned long   npd_size;	/* size of Network Port Dictionary */
	unsigned long   inline_size;	/* size of inline part of message */
	unsigned long   ool_size;	/* size of ool part of message */
	unsigned long   ool_num;	/* number of ool sections (for assembly) */
	unsigned long	ipc_seq_no;	/* the IPC sequence number of this message */
}               ipc_netmsg_hdr_t;

/*
 * Bits for info field 
 */
#define IPC_INFO_SIMPLE		0x1
#define	IPC_INFO_RPC		0x2

#endif	_IPC_HDR_
