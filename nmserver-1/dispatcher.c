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
/*
 * Functions implementing message dispatching.
 * This module is the interface between the transport-level
 * modules and the higher-level modules.
 * It has explicit knowledge of the higher-level modules.
 */


#ifdef WIN32
#include <winnt-pdo.h>
#include <sys/types.h>
#include <winsock.h>
#else
#include <sys/types.h>
#include <netinet/in.h>
#endif WIN32

#include "config.h"
#include "debug.h"
#include "dispatcher.h"
#include "disp_hdr.h"
#include "netmsg.h"
#include <servers/nm_defs.h>
#include "sbuf.h"

boolean_t disp_swap_table[DISP_FMT_MAX][DISP_FMT_MAX];

dispatcher_switch_t dispatcher_switch[DISP_TYPE_MAX];

short conf_own_format = 0;


/*
 * disp_no_function --
 *
 * Default function to dispatch for inexistant handlers.
 *
 * Parameters:
 *
 * Results:
 *
 * DISP_FAILURE
 *
 * Side effects:
 *
 * Prints an error message.
 *
 * Design:
 *
 * Note:
 *
 */
PRIVATE int disp_no_function()
{

	ERROR((msg,"** Dispatcher: no handler function **"));
	RETURN(DISP_FAILURE);
}


/*
 * disp_init --
 *	Initialise the dispatcher module.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Design:
 *	Set up the disp_swap_table.
 *
 */
EXPORT boolean_t disp_init()
{
    int	i;

    disp_swap_table[DISP_FMT_NETWORK][DISP_FMT_NETWORK] = FALSE;
    disp_swap_table[DISP_FMT_NETWORK][DISP_FMT_VL_1] = TRUE;
    disp_swap_table[DISP_FMT_NETWORK][DISP_FMT_NL_1] = FALSE;

    disp_swap_table[DISP_FMT_VL_1][DISP_FMT_NETWORK] = TRUE;
    disp_swap_table[DISP_FMT_VL_1][DISP_FMT_VL_1] = FALSE;
    disp_swap_table[DISP_FMT_VL_1][DISP_FMT_NL_1] = TRUE;

    disp_swap_table[DISP_FMT_NL_1][DISP_FMT_NETWORK] = FALSE;
    disp_swap_table[DISP_FMT_NL_1][DISP_FMT_VL_1] = TRUE;
    disp_swap_table[DISP_FMT_NL_1][DISP_FMT_NL_1] = FALSE;

    conf_own_format = htons(CONF_OWN_FORMAT);

    for (i = 0; i < DISP_TYPE_MAX; i++) {
    	dispatcher_switch[i].disp_indata = disp_no_function;
    	dispatcher_switch[i].disp_inprobe = disp_no_function;
    	dispatcher_switch[i].disp_indata_simple = disp_no_function;
    	dispatcher_switch[i].disp_rr_simple = disp_no_function;
    	dispatcher_switch[i].disp_in_request = disp_no_function;
    }

    RETURN(TRUE);

}



/*
 * disp_indata --
 *	Dispatch an incoming data buffer.
 *
 * Parameters:
 *	trid		: an ID assigned by the transport module to this data
 *	data		: the data being delivered
 *	from		: the source of the data
 *	tr_cleanup	: a function to be called by the higher-level module 
 *				when it has processed the data
 *	trmod		: the index of the transport module delivering this
 *				data
 *	client_id	: client ID returned by a previous probe, or 0
 *	crypt_level	: the encryption level of the data received
 *	broadcast	: was this data received on the broadcast address
 *
 * Results:
 *	DISP_FAILURE if the dispatch failed, or the return code from the
 *	higher-level routine that has been called.
 *
 * Design:
 *	Dispatch the data according to the disp_type.
 *
 * Note:
 *
 */
EXPORT int disp_indata(trid, data, from, tr_cleanup, trmod,
			client_id, crypt_level, broadcast)
int		trid;
sbuf_ptr_t	data;
netaddr_t	from;
int		(*tr_cleanup)();
int		trmod;
int		client_id;
int		crypt_level;
boolean_t	broadcast;
{
    register int		rc;
    register disp_hdr_ptr_t	disp_hdr_ptr;
    register short		disp_type;

    SBUF_GET_SEG(*data, disp_hdr_ptr, disp_hdr_ptr_t);

    disp_type = (short)ntohs(disp_hdr_ptr->disp_type) - DISPATCHER_VERSION;
    disp_hdr_ptr->src_format = ntohs(disp_hdr_ptr->src_format);

    if ((disp_type < 0) || (disp_type >= DISP_TYPE_MAX)) {
	    ERROR((msg,"Warning: wrong DISPATCHER_VERSION"));
	    RETURN(DISP_FAILURE);
    }

    rc = dispatcher_switch[disp_type].disp_indata(trid, data, from,
		tr_cleanup, trmod, client_id, crypt_level, broadcast);
    RETURN(rc);

}

/*
 * disp_inprobe --
 *	Dispatch the first packet of a message to a probe routine.
 *
 * Parameters:
 *	trid		: an ID assigned by the transport module to this data
 *	pkt		: the packet being probed
 *	from		: the source of the data
 *	OUT cancel	: a function to be called by the tranport module if
 *				the announced data cannot be delivered
 *	trmod		: the index of the transport module delivering this
 *				data
 *	OUT client_id	: client ID to be used in subsequent references to
 *				this message
 *	crypt_level	: the encryption level of the data received
 *	broadcast	: was this data received on the broadcast address
 *
 * Results:
 *	DISP_FAILURE if the dispatch failed, or the return code from the
 *	higher-level routine that has been called.
 *
 * Design:
 *	Dispatch the data according to the disp_type.
 *
 * Note:
 *
 * Have to do something about converting the representation format in the
 * disp_header twice, once in disp_inprobe and once in disp_indata. XXX
 *
 */
EXPORT int disp_inprobe(trid, pkt, from, cancel, trmod,
			client_id, crypt_level, broadcast)
int		trid;
sbuf_ptr_t	pkt;
netaddr_t	from;
int		*((*cancel)());
int		trmod;
int		*client_id;
int		crypt_level;
boolean_t	broadcast;
{
    register int		rc;
    register disp_hdr_ptr_t	disp_hdr_ptr;
    register short		disp_type;

    SBUF_GET_SEG(*pkt, disp_hdr_ptr, disp_hdr_ptr_t);

    disp_type = (short)ntohs(disp_hdr_ptr->disp_type) - DISPATCHER_VERSION;
    disp_hdr_ptr->src_format = ntohs(disp_hdr_ptr->src_format);

    if ((disp_type < 0) || (disp_type >= DISP_TYPE_MAX)) {
	    ERROR((msg,"Warning: wrong DISPATCHER_VERSION"));
	    RETURN(DISP_FAILURE);
    }

    rc = dispatcher_switch[disp_type].disp_inprobe(trid, pkt, from,
		cancel, trmod, client_id, crypt_level, broadcast);
    RETURN(rc);

}

/*
 * disp_indata_simple --
 *	Dispatch a simple incoming data buffer.
 *
 * Parameters:
 *	client_id	: if this is a response to a request, the ID assigned by client to the request
 *	data		: the data being delivered
 *	from		: the source of the data
 *	broadcast	: was this data received on the broadcast address
 *	crypt_level	: the encryption level of the data received
 *
 * Results:
 *	DISP_SUCCESS or DISP_FAILURE
 *
 * Design:
 *	Dispatch the data according to the disp_type.
 *
 * Note:
 *	disp_indata_simple can be used when the higher-level module can guarantee
 *	that after it returns to the dispatcher module, it has finished with the data
 *	thus allowing the transport module to reuse the buffer.
 *
 */
EXPORT int disp_indata_simple(client_id, data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    register int		rc;
    register disp_hdr_ptr_t	disp_hdr_ptr;
    register short		disp_type;

    SBUF_GET_SEG(*data, disp_hdr_ptr, disp_hdr_ptr_t);

    disp_type = (short)ntohs(disp_hdr_ptr->disp_type) - DISPATCHER_VERSION;
    disp_hdr_ptr->src_format = ntohs(disp_hdr_ptr->src_format);

    if ((disp_type < 0) || (disp_type >= DISP_TYPE_MAX)) {
	    ERROR((msg,"Warning: wrong DISPATCHER_VERSION"));
	    RETURN(DISP_FAILURE);
    }

    rc = dispatcher_switch[disp_type].disp_indata_simple(client_id, data, 
						from, broadcast, crypt_level);
    RETURN(rc);

}



/*
 * disp_rr_simple --
 *	Dispatch a simple incoming request-response data buffer.
 *
 * Parameters:
 *	data		: the data being delivered
 *	from		: the source of the data
 *	broadcast	: was this data received on the broadcast address
 *	crypt_level	: the encryption level of the data received
 *
 * Results:
 *	DISP_SUCCESS or DISP_FAILURE
 *
 * Design:
 *	Dispatch the data according to the disp_type.
 *
 * Note:
 *	disp_rr_simple can be used when the higher-level module can guarantee
 *	that after it returns to the dispatcher module, it has finished with the data
 *	thus allowing the transport module to reuse the buffer.
 *	Moreover the buffer should now contain the reply to the request in situ.
 *
 */
EXPORT int disp_rr_simple(data, from, broadcast, crypt_level)
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
{
    register int		rc;
    register disp_hdr_ptr_t	disp_hdr_ptr;
    register short		disp_type;

    SBUF_GET_SEG(*data, disp_hdr_ptr, disp_hdr_ptr_t);

    disp_type = (short)ntohs(disp_hdr_ptr->disp_type) - DISPATCHER_VERSION;
    disp_hdr_ptr->src_format = ntohs(disp_hdr_ptr->src_format);

    if ((disp_type < 0) || (disp_type >= DISP_TYPE_MAX)) {
	    ERROR((msg,"Warning: wrong DISPATCHER_VERSION"));
	    RETURN(DISP_FAILURE);
    }

    rc = dispatcher_switch[disp_type].disp_rr_simple(data, from, 
						broadcast, crypt_level);
    RETURN(rc);

}


/*
 * disp_in_request --
 *	Dispatch an incoming request.
 *
 * Parameters:
 *
 * trmod: index of transport module delivering this request.
 * trid: ID used by the transport module for this request.
 * data_ptr: sbuf containing the request message.
 * from: the address of the network server where the message originated 
 * crypt_level: encryption level for this message.
 * broadcast: TRUE if the message was a broadcast.
 *
 * Results:
 *	DISP_FAILURE if the dispatch failed, or the return code from the
 *	higher-level routine that has been called.
 *
 * Design:
 *	Dispatch the data according to the disp_type.
 *
 * Note:
 *
 */
EXPORT int disp_in_request(trmod,trid,data_ptr,from,crypt_level,broadcast)
	int		trmod;
	int		trid;
	sbuf_ptr_t	data_ptr;
	netaddr_t	from;
	int		crypt_level;
	boolean_t	broadcast;
{
    register int		rc;
    register disp_hdr_ptr_t	disp_hdr_ptr;
    register short		disp_type;

    SBUF_GET_SEG(*data_ptr, disp_hdr_ptr, disp_hdr_ptr_t);

    disp_type = (short)ntohs(disp_hdr_ptr->disp_type) - DISPATCHER_VERSION;
    disp_hdr_ptr->src_format = ntohs(disp_hdr_ptr->src_format);

    if ((disp_type < 0) || (disp_type >= DISP_TYPE_MAX)) {
	    ERROR((msg,"Warning: wrong DISPATCHER_VERSION"));
	    RETURN(DISP_FAILURE);
    }


    rc = dispatcher_switch[disp_type].disp_in_request(trmod,trid,
					data_ptr,from,crypt_level,broadcast);

    RETURN(rc);

}



