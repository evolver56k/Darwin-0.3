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

#ifndef	_DISP_HDR_
#define	_DISP_HDR_

#include <mach/boolean.h>
#include "config.h"


typedef struct {
    unsigned short	disp_type;	/* type of message (network format) */
    unsigned short	src_format;	/* format for all following data */
} disp_hdr_t, *disp_hdr_ptr_t;


/*
 * Version number for messages.
 */
#define	DISPATCHER_VERSION	(100 * 1)


/*
 * Values for disp_type. DISPE_* represents the index of the
 * entry in dispatcher_switch.
 */
#define DISPE_IPC_MSG		1
#define DISPE_IPC_UNBLOCK	2
#define DISPE_PORTCHECK		3
#define DISPE_NETNAME		4
#define DISPE_PO_RO_HINT	5
#define DISPE_PO_RO_XFER	6
#define DISPE_PO_TOKEN		7
#define DISPE_PO_DEATH		8
#define DISPE_PORTSEARCH	9
#define DISPE_PS_AUTH		10
#define DISPE_STARTUP		11
#define	DISPE_IPC_ABORT		12

#define DISP_TYPE_MAX		(12 + 1)

#define DISP_IPC_MSG		(DISPE_IPC_MSG		+ DISPATCHER_VERSION)
#define DISP_IPC_UNBLOCK	(DISPE_IPC_UNBLOCK	+ DISPATCHER_VERSION)
#define DISP_PORTCHECK		(DISPE_PORTCHECK	+ DISPATCHER_VERSION)
#define DISP_NETNAME		(DISPE_NETNAME		+ DISPATCHER_VERSION)
#define DISP_PO_RO_HINT		(DISPE_PO_RO_HINT	+ DISPATCHER_VERSION)
#define DISP_PO_RO_XFER		(DISPE_PO_RO_XFER	+ DISPATCHER_VERSION)
#define DISP_PO_TOKEN		(DISPE_PO_TOKEN		+ DISPATCHER_VERSION)
#define DISP_PO_DEATH		(DISPE_PO_DEATH		+ DISPATCHER_VERSION)
#define DISP_PORTSEARCH		(DISPE_PORTSEARCH	+ DISPATCHER_VERSION)
#define DISP_PS_AUTH		(DISPE_PS_AUTH		+ DISPATCHER_VERSION)
#define DISP_STARTUP		(DISPE_STARTUP		+ DISPATCHER_VERSION)
#define	DISP_IPC_ABORT		(DISPE_IPC_ABORT	+ DISPATCHER_VERSION)

/*
 * Table for byte-swapping requirements 
 */
extern boolean_t disp_swap_table[DISP_FMT_MAX][DISP_FMT_MAX];

/*
 * Check if byte-swapping is needed 
 */
#define	DISP_CHECK_SWAP(sf) (disp_swap_table[sf][CONF_OWN_FORMAT])

/*
 * Dispatcher return codes. These codes are in the same
 * space as the TR_* and IPC_* codes.
 */
#define	DISP_WILL_REPLY	0
#define DISP_FAILURE	-1
#define DISP_SUCCESS	-2
#define DISP_IGNORE	-3

#endif	_DISP_HDR_
