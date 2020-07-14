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

#include <mach/mach.h>
#include <mach/boolean.h>
#include <mach/message.h>

#include "ipc.h"
#include "ipc_rec.h"
#include "netmsg.h"
#include "ipc_internal.h"

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_IPCBUFF;
PUBLIC mem_objrec_t	MEM_ASSEMBUFF;
PUBLIC mem_objrec_t	MEM_IPCBLOCK;
PUBLIC mem_objrec_t	MEM_IPCREC;
PUBLIC mem_objrec_t	MEM_IPCABORT;



/*
 * ipc_init
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Design:
 *
 */
EXPORT boolean_t ipc_init()
{

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_IPCBUFF,"IPC receive buffer",MSG_SIZE_MAX,TRUE,7,1);
	mem_initobj(&MEM_ASSEMBUFF,"IPC assembly buffer",IPC_ASSEM_SIZE,
								TRUE,3,1);
	mem_initobj(&MEM_IPCBLOCK,"IPC block record",sizeof(ipc_block_t),
								FALSE,500,5);
	mem_initobj(&MEM_IPCREC,"IPC record",sizeof(ipc_rec_t),FALSE,12,2);
	mem_initobj(&MEM_IPCABORT,"IPC abort record",sizeof(ipc_abort_rec_t),
								FALSE,50,2);

	/*
	 * Start the IPC module.
	 */
	RETURN(ipc_rpc_init());

}

