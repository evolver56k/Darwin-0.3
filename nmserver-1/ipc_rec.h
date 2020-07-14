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

#ifndef	_IPC_REC_
#define	_IPC_REC_

#include <mach/boolean.h>
#include <mach/message.h>

#include	"sys_queue.h"
#include	"ipc_hdr.h"
#include	"sbuf.h"
#include	"mem.h"

#define IPC_OUT_NUM_SEGS	5


typedef struct {
	msg_header_t	*ipcbuff;	/* IPC receive buffer */
	sbuf_t		msg;		/* complete msg to transmit */
	sbuf_seg_t	segs[IPC_OUT_NUM_SEGS];	/* statically alloc'd sbuf segs. */
	boolean_t	ool_exists;	/* is there out-of-line data */
	sbuf_t		ool;		/* list of out-of-line sections (for GC) */
	sbuf_seg_t	ool_seg;	/* optimisation for message with 1 ool item */
	boolean_t	npd_exists;	/* is there a NPD */
	sbuf_t		npd;		/* Network Port Dictionary (for GC) */
	ipc_netmsg_hdr_t netmsg_hdr;	/* IPC netmsg header */
	netaddr_t	dest;		/* destination address (machine) */
	int		crypt_level;	/* The encryption level of the message. */
} ipc_outrec_t;


typedef struct {
	sbuf_ptr_t	msg;		/* data from the transport level */
	msg_header_t	*assem_buff;	/* buffer for assembly */
	int		assem_len;	/* length of assem_buff to dealloc */
			/* or 0 if MEM_ASSEMBUF, -1 if no dealloc needed */
	int		assem_type;	/* type of assembly area */
	netaddr_t	from;		/* message origin (machine) */
	unsigned long	ipc_seq_no;	/* sequence number */
	ipc_netmsg_hdr_t *netmsg_hdr_ptr;	/* IPC netmsg header */
} ipc_inrec_t;


typedef struct {
	int			type;		/* type of transaction */
	int			status;		/* current progress status */
	int			trid;		/* transport-level ID */
	int			trmod;		/* index of transport mod used */
	int			retry_level;	/* port info at last xmit */
	sys_queue_chain_t	re_send_q;	/* link in queue for re-send */
	sys_queue_chain_t	out_q;		/* link in queue of out reqs */
	port_rec_ptr_t		server_port_ptr;
	port_rec_ptr_t		reply_port_ptr;
	ipc_outrec_t		out;
	ipc_inrec_t		in;
} ipc_rec_t;

typedef	ipc_rec_t *ipc_rec_ptr_t;


/*
 * Structure for queue of senders waiting on a blocked port.
 */
typedef struct ipc_block {
	struct ipc_block	*next;
	netaddr_t		addr;
} ipc_block_t, *ipc_block_ptr_t;

#define IPC_BLOCK_NULL		((ipc_block_ptr_t)0)

/*
 * Values for assem_type field in ipc_outrec.
 */
#define	IPC_REC_ASSEM_OBJ	0	/* MEM_ALLOCOBJ(MEM_ASSEMBUFF) */
#define	IPC_REC_ASSEM_MEMALLOC	1	/* mem_alloc(assem_len) */
#define	IPC_REC_ASSEM_PKT	2	/* (virtual) copy of packet buffer */

/*
 * Values for type field in ipc_rec
 */
#define	IPC_REC_TYPE_UNKNOWN	0	/* not yet specified */
#define	IPC_REC_TYPE_SINGLE	1	/* single message */
#define	IPC_REC_TYPE_CLIENT	2	/* RPC, client side */
#define	IPC_REC_TYPE_SERVER	3	/* RPC, server side */

/*
 * Values for status field in ipc_rec
 */
#define	IPC_REC_ACTIVE		1	/* waiting for ack or reply */
#define	IPC_REC_READY		2	/* ready to be transmitted */
#define	IPC_REC_WAITING		3	/* waiting for new information */
#define	IPC_REC_REPLY		4	/* waiting for local reply */

/*
 * Standard size for an assembly buffer 
 */
#define	IPC_ASSEM_SIZE		(16384)


/*
 * Return codes for IPC operations. These codes are in the same space as
 * the DISP_* and TR_* codes.
 */
#define	IPC_SUCCESS		1
#define	IPC_FAILURE		2
#define	IPC_PORT_BLOCKED	3
#define	IPC_PORT_NOT_HERE	4
#define IPC_BAD_SEQ_NO		5
#define	IPC_REQUEST		6
#define	IPC_ABORT_REPLY		7
#define	IPC_ABORT_REQUEST	8
#define	IPC_PORT_BUSY		9


/*
 * Packet used for client abort.
 */
typedef struct {
	struct abort_pkt {
		disp_hdr_t	disp_hdr;
		np_uid_t	np_puid;
		unsigned long	ipc_seq_no;
	} abort_pkt;
	sbuf_t		msg;
	sbuf_seg_t	segs[5];
} ipc_abort_rec_t, *ipc_abort_rec_ptr_t;


/*
 * Extern definitions internal to the IPC module.
 */
extern void ipc_in_init();

extern void ipc_in_block();
/*
port_rec_ptr	dp_ptr;
netaddr_t	from;
*/

extern int ipc_in_cancel();
/*
int		ipcid;
int		reason;
*/

extern ipc_in_unblock();
/*
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
*/

extern void ipc_out_init();


/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_ASSEMBUFF;
extern mem_objrec_t		MEM_IPCBLOCK;
extern mem_objrec_t		MEM_IPCREC;
extern mem_objrec_t		MEM_IPCABORT;


#endif	_IPC_REC_
