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

#include "netmsg.h"
#include <servers/nm_defs.h>

#ifndef WIN32
#include	<sys/types.h>
#include	<netinet/in.h>
#endif

#include "debug.h"
#include "disp_hdr.h"
#include "ipc.h"
#include "ls_defs.h"
#include "network.h"
#include "nm_extra.h"
#include "pc_defs.h"
#include "portcheck.h"
#include "portrec.h"
#include "portsearch.h"
#include "srr.h"
#include "ipc_swap.h"



/*
 * pc_clientequal
 *	Checks to see whether the client_id of a host record matches the input client_id.
 *
 */
PRIVATE int pc_clientequal(hp, client_id)
pc_host_list_ptr_t	hp;
int		client_id;
{
	RETURN(hp->pchl_client_id == client_id);
}

/*
 * pc_cleanup
 *	Called by the transport module if a request failed.
 *
 * Parameters:
 *	client_id	: the client_id of the failed request
 *	completion_code	: why the request failed
 *
 * Design:
 *	Just feeds this request into pc_handle_checkup_reply
 *	with all the port statuses set to be bad.
 *
 */
PUBLIC int pc_cleanup(client_id, completion_code)
int	client_id;
int	completion_code;
{
	sbuf_t			msgbuf;
	sbuf_seg_t		msgbuf_seg;
	pc_host_list_ptr_t	hp;
	int			index;


	hp = (pc_host_list_ptr_t)lq_find_in_queue(&pc_request_queue, pc_clientequal, client_id);

	if (hp == NULL) {
		RETURN(0);
	}

	for (index = 0; index < PC_MAX_ENTRIES; index++) {
		hp->pchl_portcheck->pc_status[index] = PORTCHECK_NOTOK;
	}

	SBUF_SEG_INIT(msgbuf, &msgbuf_seg);
	SBUF_APPEND(msgbuf, hp->pchl_portcheck, srr_max_data_size);

	(void)pc_handle_checkup_reply(client_id, (sbuf_ptr_t)&msgbuf, hp->pchl_destination, FALSE, 0);
	RETURN(0);

}



/*
 * pc_handle_checkup_request
 *	Handles and replies to a port checkup request.
 *	It is called by the srr transport protocol.
 *
 * Parameters:
 *	request		: the incoming checkup request
 *	from		: the sender of the request (ignored)
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Design:
 *	Compares the information about a network port contained in the request
 *	with the information about the port held by the port records module.
 *	The status byte is set accordingly.
 */
/* ARGSUSED */
PUBLIC int pc_handle_checkup_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
	network_port_t	nport;
	portcheck_ptr_t pc_ptr;
	int		num_entries, index, size;
	port_rec_ptr_t	port_rec_ptr;

	INCSTAT(pc_requests_rcvd);
	SBUF_GET_SEG(*request, pc_ptr, portcheck_ptr_t);
	SBUF_GET_SIZE(*request, size);
	if (DISP_CHECK_SWAP(pc_ptr->pc_disp_hdr.src_format)) {
		SWAP_DECLS;

		num_entries = SWAP_LONG(pc_ptr->pc_num_entries,
						pc_ptr->pc_num_entries);
	} else {
		num_entries = pc_ptr->pc_num_entries;
	}
	if (size < (num_entries * sizeof(pc_network_port_t))) {
		RETURN(DISP_FAILURE);
	}

	LOGCHECK;

	for(index = 0; index < num_entries; index++) {
		PC_EXTRACT_NPORT(nport, pc_ptr->pc_nports[index]);
		port_rec_ptr = pr_np_puid_lookup(pc_ptr->pc_nports[index].pc_np_puid);
		if (port_rec_ptr == PORT_REC_NULL) {
			pc_ptr->pc_status[index] = PORTCHECK_NOTOK;
		}
		else if (port_rec_ptr->portrec_info & PORT_INFO_DEAD) {
			pc_ptr->pc_status[index] = PORTCHECK_DEAD;
			lk_unlock(&port_rec_ptr->portrec_lock);
		}
		else {
			if ((port_rec_ptr->portrec_network_port.np_receiver != nport.np_receiver)
				|| (port_rec_ptr->portrec_network_port.np_owner != nport.np_owner))
			{
				pc_ptr->pc_status[index] |= PORTCHECK_O_R_CHANGED;
			}
			if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
				if ((pc_ptr->pc_status[index] & PORTCHECK_BLOCK)
					!= (port_rec_ptr->portrec_info & PORT_INFO_BLOCKED))
				{
					/*
					 * Either the requestor thought the port was blocked and it is not
					 * or it thought that the port was not blocked and it is.
					 * Reverse the value of the block status.
					 */
					pc_ptr->pc_status[index] ^= PORTCHECK_BLOCK;
				}
			}
			lk_unlock(&port_rec_ptr->portrec_lock);
		}
		LOGCHECK;
	}

	/*
	 * Set the headers in preparation for responding to the request.
	 */
	pc_ptr->pc_disp_hdr.src_format = conf_own_format;
	RETURN(DISP_SUCCESS);
}



/*
 * pc_handle_checkup_reply
 *	Handles the results of a port checkup requests.
 *	It is called by the srr transport protocol.
 *
 * Parameters:
 *	client_id	: the id of the request which generated this reply
 *	reply		: pointer to the reply sbuf
 *	from		: host from which the reply was received
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Design:
 *	Extract the corresponding request record from the pc_request_queue.
 *	Examine the returned status bits to determine:
 *		whether we should call port_search
 *		whether we should change the blocked status of the port
 *		whether we should call ipc_retry
 *	The outstanding request counter is decremented and if it reaches zero
 *	then we restart the checkups timer.
 *
 */
/* ARGSUSED */
PUBLIC int pc_handle_checkup_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
	int			num_entries, index, size;
	unsigned char		status;
	portcheck_ptr_t 	pc_ptr;
	port_rec_ptr_t  	port_rec_ptr;
	pc_host_list_ptr_t	hp;

	INCSTAT(pc_replies_rcvd);
	LOGCHECK;

	if ((hp = (pc_host_list_ptr_t)lq_cond_delete_from_queue(&pc_request_queue,pc_clientequal,client_id))
		== (pc_host_list_ptr_t)0)
	{
		RETURN(DISP_FAILURE);
	}
	if (from != hp->pchl_destination) {
		/*
		 * Deallocate the memory used for this request.
		 */
		MEM_DEALLOCOBJ(hp->pchl_portcheck, MEM_TRBUFF);
		MEM_DEALLOCOBJ(hp, MEM_PCITEM);
		RETURN(DISP_FAILURE);
	}

	SBUF_GET_SEG(*reply, pc_ptr, portcheck_ptr_t);
	SBUF_GET_SIZE(*reply, size);
	if (DISP_CHECK_SWAP(pc_ptr->pc_disp_hdr.src_format)) {
		SWAP_DECLS;

		num_entries = SWAP_LONG(pc_ptr->pc_num_entries,
						pc_ptr->pc_num_entries);
	} else {
		num_entries = pc_ptr->pc_num_entries;
	}
	if (size < (num_entries * sizeof(pc_network_port_t))) {
		/*
		 * Deallocate the memory used for this request.
		 */
		MEM_DEALLOCOBJ(hp->pchl_portcheck, MEM_TRBUFF);
		MEM_DEALLOCOBJ(hp, MEM_PCITEM);
		RETURN(DISP_FAILURE);
	}


	for (index = 0; index < num_entries; index++) {
		network_port_t	nport;
		PC_EXTRACT_NPORT(nport, pc_ptr->pc_nports[index]);
		LOGCHECK;
		port_rec_ptr = pr_np_puid_lookup(pc_ptr->pc_nports[index].pc_np_puid);
		if (port_rec_ptr == PORT_REC_NULL) {
			continue;
		}
		/* port_rec_ptr LOCK RW/RW */

		status = pc_ptr->pc_status[index];
		if ((status & PORTCHECK_DEAD) || (status & PORTCHECK_NOTOK)
			|| (status & PORTCHECK_O_R_CHANGED))
		{
			ps_do_port_search(port_rec_ptr, FALSE, (network_port_ptr_t)0, (int(*)())0);
		}
		else {
			if ((status & PORTCHECK_BLOCK)
				!= (port_rec_ptr->portrec_info & PORT_INFO_BLOCKED))
			{
				if (port_rec_ptr->portrec_info & PORT_INFO_BLOCKED) {
					port_rec_ptr->portrec_info ^= PORT_INFO_BLOCKED;
					(void)ipc_retry(port_rec_ptr);
				}
			}
			else {
				port_rec_ptr->portrec_aliveness = PORT_ACTIVE;
			}
		}
		/* port_rec_ptr LOCK -/- */
		lk_unlock(&port_rec_ptr->portrec_lock);
	}

	/*
	 * Deallocate the memory used for this request.
	 */
	MEM_DEALLOCOBJ(hp->pchl_portcheck, MEM_TRBUFF);
	MEM_DEALLOCOBJ(hp, MEM_PCITEM);

	/*
	 * Update the outstanding request counter
	 */
	mutex_lock(&pc_request_counter.pc_lock);
	pc_request_counter.pc_counter --;
	if (pc_request_counter.pc_counter < 0) {
		ERROR((msg, "pc_handle_checkup_reply: pc_counter = %d.", pc_request_counter.pc_counter));
	}
	mutex_unlock(&pc_request_counter.pc_lock);

	RETURN(DISP_SUCCESS);
}
