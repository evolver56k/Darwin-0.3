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

#ifndef WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif WIN32

#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "ipc.h"
#include "ipc_rec.h"
#include "mem.h"
#include "netmsg.h"
#include <servers/nm_defs.h>
#include "nm_extra.h"
#include "port_defs.h"
#include "portrec.h"
#include "sbuf.h"
#include "transport.h"

typedef struct {
	disp_hdr_t	ipc_unblock_disp_hdr;
	np_uid_t	ipc_unblock_np_puid;
} ipc_unblock_t, *ipc_unblock_ptr_t;

#define IPC_MAX_UNBLOCKS	3



/*
 * ipc_in_block
 *	Add a host to the waiting list for a blocked port.
 *
 * Parameters:
 *	dp_ptr	: port record for the blocked port.
 *	from	: address of host to add to the waiting list.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	should add an entry to the waiting list.
 *
 * Note:
 *	the dp_ptr should be locked and it is left locked at exit.
 *
 */
PUBLIC void ipc_in_block(IN dp_ptr, IN from)
	port_rec_ptr_t	dp_ptr;
	netaddr_t	from;
{
	ipc_block_ptr_t		blk_ptr, current, prev;

	/* dp_ptr LOCK RW/RW */
	/*
	 * Find where to put the new block entry.
	 */
	prev = current = (ipc_block_ptr_t)dp_ptr->portrec_block_queue;
	while (current != IPC_BLOCK_NULL) {
		if (current->addr == from) {
			/*
			 * No point in storing another block record for this host.
			 */
			RET;
		}
		prev = current;
		current = current->next;
	}

	MEM_ALLOCOBJ(blk_ptr,ipc_block_ptr_t,MEM_IPCBLOCK);
	blk_ptr->addr = from;
	blk_ptr->next = IPC_BLOCK_NULL;

	if (prev == IPC_BLOCK_NULL) {
		dp_ptr->portrec_block_queue = (pointer_t)blk_ptr;
	}
	else {
		prev->next = blk_ptr;
	}

	/* dp_ptr LOCK RW/- */

	RET;

}



/*
 * ipc_in_unblock --
 *	accepts an unblock packet from over the network.
 *
 * Parameters:
 *	client_id	: ignored.
 *	data		: the unblock data.
 *	from		: the host sending the unblock.
 *	broadcast	: ignored.
 *	crypt_level	: ignored.
 *
 * Results:
 *	DISP_SUCCESS.
 *
 * Side effects:
 *	Calls ipc_retry to try resending a message to the unblocked port.
 *
 * Note:
 *	The unblock data just contains a network port PUID.
 *
 */
/* ARGSUSED */
PUBLIC int ipc_in_unblock(client_id,data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
	ipc_unblock_ptr_t	unblock_ptr;
	port_rec_ptr_t		port_rec_ptr;

	INCSTAT(ipc_unblocks_rcvd);
	SBUF_GET_SEG(*data, unblock_ptr, ipc_unblock_ptr_t);

	if ((port_rec_ptr = pr_np_puid_lookup(unblock_ptr->ipc_unblock_np_puid)) == PORT_REC_NULL) {
		RETURN(DISP_SUCCESS);
	}

	/* port_rec_ptr LOCK RW/RW */
	if (port_rec_ptr->portrec_network_port.np_receiver != from) {
	}
	else {
		port_rec_ptr->portrec_info &= ~PORT_INFO_BLOCKED;
		(void)ipc_retry(port_rec_ptr);
	}
	lk_unlock(&port_rec_ptr->portrec_lock);
	/* port_rec_ptr LOCK -/- */

	RETURN(DISP_SUCCESS);

}




/*
 * ipc_msg_accepted
 *	called when we have received a message accepted notification from the kernel.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to record for relevant port.
 *
 * Design:
 *	Sends a port unblock datagram to some blocked senders.
 *
 * Notes:
 *	Should not send an unblock to too many senders.
 *	Assumes that the port record is locked.
 *
 */
EXPORT void ipc_msg_accepted(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
{
	sbuf_t		sbuf;
	sbuf_seg_t	sbuf_seg;
	ipc_unblock_t	message;
	int		tr, i;
	ipc_block_ptr_t	block_ptr;

	/* port_rec_ptr LOCK RW/RW */
	if (port_rec_ptr->portrec_block_queue == (pointer_t)0) {
		/*
		 * No unblocks to send.
		 */
		RET;
	}

	SBUF_SEG_INIT(sbuf, &sbuf_seg);
	SBUF_APPEND(sbuf, &message, sizeof(ipc_unblock_t));
	message.ipc_unblock_disp_hdr.disp_type = htons(DISP_IPC_UNBLOCK);
	message.ipc_unblock_disp_hdr.src_format = conf_own_format;
	message.ipc_unblock_np_puid = port_rec_ptr->portrec_network_port.np_puid;

	for (i = 0; i < IPC_MAX_UNBLOCKS; i++) {
		block_ptr = (ipc_block_ptr_t)port_rec_ptr->portrec_block_queue;
		if (block_ptr == IPC_BLOCK_NULL) {
		    	break;
		}
		port_rec_ptr->portrec_block_queue = (pointer_t)block_ptr->next;

		tr = transport_switch[TR_DATAGRAM_ENTRY].send(0, &sbuf, block_ptr->addr,
					TRSERV_NORMAL, CRYPT_DONT_ENCRYPT, 0);
		if (tr != TR_SUCCESS) {
			ERROR((msg, "ipc_msg_accepted.send fails, tr = %d.", tr));
		}
		else INCSTAT(ipc_unblocks_sent);

		MEM_DEALLOCOBJ(block_ptr, MEM_IPCBLOCK);
	}

	RET;

}
