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

#include	"netmsg.h"
#include	<servers/nm_defs.h>

#include	<mach/cthreads.h>
#include	<mach/mach.h>
#include	<stdio.h>
#include	<mach/boolean.h>

#ifndef WIN32
#include	<sys/time.h>
#include	<sys/types.h>
#include	<netinet/in.h>
#endif

#include	"debug.h"
#include	"dispatcher.h"
#include	"lock_queue.h"
#include	"ls_defs.h"
#include	"mem.h"
#include	"network.h"
#include	"nm_extra.h"
#include	"pc_defs.h"
#include	"portcheck.h"
#include	"portops.h"
#include	"portrec.h"
#include	"sbuf.h"
#include	"srr.h"
#include	"timer.h"
#include	"ipc.h"

extern int running_local_ports;

PRIVATE int		pc_last_client_id = 0;

struct lock_queue	pc_request_queue;

struct lock_queue	pc_throttle_queue;

struct timer		pc_timer;

pc_count_t		pc_request_counter;

PRIVATE int pc_do_checkups();

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_PCITEM;



/*
 * pc_init
 *	Initialises the port checkups module.
 *
 */
PUBLIC boolean_t pc_init()
{

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_PCITEM,"PC item",sizeof(pc_host_list_t),
								FALSE,250,50);

	/*
	 * Set up the queues which will hold outstanding requests.
	 */
	lq_init(&pc_request_queue);
	lq_init(&pc_throttle_queue);

	/*
	 * Set up the timer which will call pc_do_checkups.
 	 */		
	pc_timer.interval.tv_sec = param.pc_checkup_interval;
	pc_timer.interval.tv_usec = 0;
	pc_timer.action = (void (*)()) pc_do_checkups;
	pc_timer.info = (char *)0;
	timer_start(&pc_timer);

	/*
	 * Set up the counter which will count outstanding requests.
	 */
	mutex_init(&pc_request_counter.pc_lock);
	pc_request_counter.pc_counter = 0;

	/*
	 * Fill in our entries in the dispatcher switch.
 	 */
	dispatcher_switch[DISPE_PORTCHECK].disp_indata_simple = pc_handle_checkup_reply;
	dispatcher_switch[DISPE_PORTCHECK].disp_rr_simple = pc_handle_checkup_request;
	dispatcher_switch[DISPE_STARTUP].disp_indata_simple = pc_handle_startup_hint;

	/*
	 * Send out a startup hint.
	 */
	if (param.conf_network) {
		pc_send_startup_hint();
	}

	RETURN(TRUE);
}


/*
 * pc_hostequal
 *	Checks to see whether the host in a queued record equals the second host.
 *
 */
PRIVATE int pc_hostequal(hp, netaddr)
pc_host_list_ptr_t	hp;
netaddr_t		netaddr;
{
	RETURN(hp->pchl_destination == netaddr);
}


/*
 * pc_add_host
 *	Adds a host which is a new destination for checkup requests
 *	to the queue of host requests.
 *
 * Parameters:
 *	destination	: the new host
 *
 * Returns:
 *	Pointer to the queue entry for the host.
 *
 * Notes:
 *	The new record is queued at the head of the host queue
 *	so that if it is a duplicate request (needed because the
 *	first request has reached maximum size) it will be found
 *	first by another call to pc_gen_request.
 *
 *
 */
PRIVATE pc_host_list_ptr_t pc_add_host(destination)
netaddr_t	destination;
{
	pc_host_list_ptr_t	newp;

	/*
	 * Update the number of requests outstanding.
	 */
	mutex_lock(&pc_request_counter.pc_lock);
	++pc_request_counter.pc_counter;
	mutex_unlock(&pc_request_counter.pc_lock);

	/*
	 * Create and fill in a record for this destination.
	 */
	MEM_ALLOCOBJ(newp,pc_host_list_ptr_t,MEM_PCITEM);
	newp->pchl_destination = destination;
	newp->pchl_client_id = pc_last_client_id++;
	MEM_ALLOCOBJ(newp->pchl_portcheck,portcheck_ptr_t,MEM_TRBUFF);
	newp->pchl_portcheck->pc_num_entries = 0;


	lq_prequeue(&pc_throttle_queue, (cthread_queue_item_t)newp);

	RETURN(newp);
}



/*
 * pc_add_request
 *	Adds a checkup request to the requests destined for a particular host.
 *	If there are no requests for this host, or the number of requests has
 *	grown too large, then a new request packet is made for this host.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to the port record
 *	destination	: host to be queried
 *
 */
PRIVATE void pc_add_request(port_rec_ptr, destination)
port_rec_ptr_t	port_rec_ptr;
netaddr_t	destination;
{
	pc_host_list_ptr_t	hp;

	/*
	 * See if we already have requests queued for the receiver.
	 */
	hp = (pc_host_list_ptr_t)lq_find_in_queue(&pc_throttle_queue, pc_hostequal, (int)destination);
	if ((hp == (pc_host_list_ptr_t)NULL)
		|| (hp->pchl_portcheck->pc_num_entries == PC_MAX_ENTRIES))
	{
		hp = pc_add_host(destination);
	}

	/*
	 * Add our information to the request.
	 */
	PC_ASSIGN_NPORT(hp->pchl_portcheck->pc_nports[hp->pchl_portcheck->pc_num_entries],
				port_rec_ptr->portrec_network_port);
	
	hp->pchl_portcheck->pc_status[hp->pchl_portcheck->pc_num_entries] = 0;
	if (port_rec_ptr->portrec_info & PORT_INFO_BLOCKED) {
		hp->pchl_portcheck->pc_status[hp->pchl_portcheck->pc_num_entries] = PORTCHECK_BLOCK;
	}
	hp->pchl_portcheck->pc_num_entries++;


	RET;
}


/*
 * pc_gen_request
 *	Generate the requests for a particular port.
 *
 * Parameters:
 *	pi_ptr		: pointer to a record generated by pr_list.
 *
 * Design:
 *	Look up the port.
 *	Queue a request for the receiver if it is not this network server.
 *	Queue a request for the owner if it is different from the receiver and not this network server.
 *	Deallocate the record created by pr_list.
 *
 */
PRIVATE void pc_gen_request(pi_ptr)
port_item_ptr_t		pi_ptr;
{
	register port_rec_ptr_t		port_rec_ptr;

	/*
	 * Find the port record corresponding to this port item.
	 */
	if ((port_rec_ptr = pr_lportlookup(pi_ptr->pi_port)) == PORT_REC_NULL) {
		MEM_DEALLOCOBJ(pi_ptr, MEM_PORTITEM);
		RET;
	}
	/* port_rec_ptr LOCK RW/R */

	/*
	 * Check the portrec_aliveness field.
	 */
	if (NPORT_HAVE_ALL_RIGHTS(port_rec_ptr->portrec_network_port)
		|| (port_rec_ptr->portrec_info & (PORT_INFO_SUSPENDED | PORT_INFO_DEAD | PORT_INFO_ACTIVE))
		|| ((--port_rec_ptr->portrec_aliveness) > PORT_INACTIVE))
	{
		lk_unlock(&port_rec_ptr->portrec_lock);
		MEM_DEALLOCOBJ(pi_ptr, MEM_PORTITEM);
		RET;
	}

	LOGCHECK;

	/*
	 * As a precaution, we kick the port in the RPC module to cause
	 * an abort of a pending RPC.
	 */
	ipc_port_moved(port_rec_ptr);

	if (PORT_REC_RECEIVER(port_rec_ptr) != my_host_id) {
		pc_add_request(port_rec_ptr, PORT_REC_RECEIVER(port_rec_ptr));
	}

	/*
	 * If the owner is not the same as the receiver do the same for the owner.
	 */
	if ((PORT_REC_OWNER(port_rec_ptr) != my_host_id)
		&& (PORT_REC_RECEIVER(port_rec_ptr) != PORT_REC_OWNER(port_rec_ptr)))
	{
		pc_add_request(port_rec_ptr, PORT_REC_OWNER(port_rec_ptr));
	}

	lk_unlock(&port_rec_ptr->portrec_lock);
	MEM_DEALLOCOBJ(pi_ptr, MEM_PORTITEM);
	RET;

}


/*
 * pc_send_request
 *	Actually send out a request using srr_send.
 *
 * Parameters:
 *	hp	: pointer to an entry in the host queue.
 *
 * Notes:
 *	The value returned is ignored.
 *
 */
PRIVATE int pc_send_request(hp)
pc_host_list_ptr_t	hp;
{
	sbuf_t		msgbuf;
	sbuf_seg_t	msgbuf_seg;
	int		rc;

	LOGCHECK;

	hp->pchl_portcheck->pc_disp_hdr.disp_type = htons(DISP_PORTCHECK);
	hp->pchl_portcheck->pc_disp_hdr.src_format = conf_own_format;
	SBUF_SEG_INIT(msgbuf, &msgbuf_seg);
	SBUF_APPEND(msgbuf, hp->pchl_portcheck, srr_max_data_size);
	if ((rc = transport_switch[TR_SRR_ENTRY].send(hp->pchl_client_id,
				(sbuf_ptr_t)&msgbuf, hp->pchl_destination,
				TRSERV_NORMAL, CRYPT_DONT_ENCRYPT, pc_cleanup)) != TR_SUCCESS)
	{
		ERROR((msg, "pc_send_request.transport_send fails, rc = %d.", rc));
	}
	else INCSTAT(pc_requests_sent);
	RETURN(1);
}



/*
 * pc_send_some
 *	Called initially from pc_do_checkups (which is called from the timer 
 *	module) and then subsequently from the timer module.
 *
 *	Takes a few (param.pc_burst_size) requests off of 
 *	pc_throttle_queue, sends them, and sets a timer to 
 *	either send the next burst (queue not empty) or 
 *	restart pc_do_checkups (queue empty)
 *
 *
 */
PRIVATE int pc_send_some()

{

	pc_host_list_ptr_t	request_ptr;
	int			i;

	for (i = 1; i <= param.pc_burst_size; i++){
		request_ptr = (pc_host_list_ptr_t) lq_dequeue(&pc_throttle_queue);
		if (request_ptr == (pc_host_list_ptr_t) 0) break;
		
		lq_prequeue(&pc_request_queue,(cthread_queue_item_t) request_ptr);
		pc_send_request(request_ptr);
	}
	if (((lock_queue_t)(&pc_throttle_queue))->head ==0){
		pc_timer.interval.tv_sec = param.pc_checkup_interval;
		pc_timer.interval.tv_usec = 0;
		pc_timer.action = (void (*)()) pc_do_checkups;
		timer_start(&pc_timer);
	}
	else {
		pc_timer.interval.tv_sec = 0;
		pc_timer.interval.tv_usec = param.pc_burst_interval_usec;
		pc_timer.action = (void (*)()) pc_send_some;
		timer_start(&pc_timer);
	}
		
	RETURN(0);
}



/*
 * pc_do_checkups
 *	Called from the timer module.
 *	Generates checkup requests and then calls pc_send_some()
 *	to start sending them out.
 *
 */
PRIVATE int pc_do_checkups()
{

	if (running_local_ports != 0)
	{
	lock_queue_t	port_queue;
		port_item_ptr_t	pi_ptr;
		
	
		port_queue = pr_list();
		while ((pi_ptr = (port_item_ptr_t)
			lq_dequeue(port_queue)) != (port_item_ptr_t)0) {
			pc_gen_request(pi_ptr);
		}
		MEM_DEALLOCOBJ(port_queue, MEM_LQUEUE);
		
		pc_send_some();		/* ignore returned value */
	
		LOGCHECK;
	}

	RETURN(0);
}

