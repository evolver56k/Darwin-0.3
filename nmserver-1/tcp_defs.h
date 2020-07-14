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

#ifndef	_TCP_DEFS_
#define	_TCP_DEFS_

#include	"mem.h"
#include	"sbuf.h"
#include	"sys_queue.h"

/*
 * Transaction records.
 */
typedef struct tcp_trans {
	int			state;	/* see defines below */
	unsigned long		trid;
	int			client_id;
	sbuf_ptr_t		data;
	int			crypt_level;
	int			(*reply_proc)();
	sys_queue_chain_t	transq;	/* list of pending/waiting transactions */
} tcp_trans_t, *tcp_trans_ptr_t;

#define	TCP_TR_INVALID	0
#define	TCP_TR_PENDING	1	/* awaiting a reply */
#define	TCP_TR_WAITING	2	/* awaiting transmission */


/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_TCPTRANS;


#endif	_TCP_DEFS_
