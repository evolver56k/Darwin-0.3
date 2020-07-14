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

#ifndef	_IPC_INTERNAL_
#define	_IPC_INTERNAL_

#include	"ipc.h"
#include	"mem.h"

/*
 * Macros to test ipc/rpc status.
 */
#define	awaiting_local_reply(pr)						\
	((pr->portrec_reply_ipcrec != 0) &&					\
		(((ipc_rec_ptr_t)pr->portrec_reply_ipcrec)->status ==		\
							IPC_REC_REPLY))

#define	awaiting_remote_reply(pr)						\
	((pr->portrec_reply_ipcrec != 0)					\
		&&								\
	(((ipc_rec_ptr_t)pr->portrec_reply_ipcrec)->type == IPC_REC_TYPE_CLIENT)\
		&&								\
	(((ipc_rec_ptr_t)pr->portrec_reply_ipcrec)->status == IPC_REC_ACTIVE))

/*
 * Macro to clean up the resources used for processing an incoming message.
 */
#define	ipc_in_gc(ipc_ptr) {							\
	switch ((ipc_ptr)->in.assem_type) {					\
		case IPC_REC_ASSEM_PKT:						\
			break;							\
		case IPC_REC_ASSEM_OBJ:						\
			MEM_DEALLOCOBJ(((ipc_ptr)->in.assem_buff),	\
				MEM_ASSEMBUFF);					\
			break;							\
		case IPC_REC_ASSEM_MEMALLOC:					\
			MEM_DEALLOC((pointer_t)((ipc_ptr)->in.assem_buff),	\
						(ipc_ptr)->in.assem_len);	\
			break;							\
		default:							\
			ERROR((msg,						\
				"ipc_in_gc: unknown type for assembly area: %d",\
				(int) (ipc_ptr)->in.assem_type));	     	\
			break;							\
	}									\
}


/*
 * Extern declarations for this module only.
 */
extern void	ipc_inmsg();
extern void	ipc_outmsg();
extern void	ipc_client_abort();
extern void	ipc_server_abort();
extern void	ipc_out_gc();

/*
 * Memory management for MEM_IPCBUFF.
 */
extern mem_objrec_t		MEM_IPCBUFF;


#endif	_IPC_INTERNAL_
