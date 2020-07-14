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
#include "transport.h"

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_TRBUFF;


EXPORT transport_sw_entry_t	transport_switch[TR_MAX_ENTRY];


/*
 * transport_no_function --
 *
 * Default function for inexistant transport calls.
 *
 * Parameters:
 *
 * Results:
 *
 * TR_FAILURE
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
EXPORT int transport_no_function()
{

	ERROR((msg,"** Transport: invalid function **"));
	RETURN(TR_FAILURE);
}



/*
 * transport_noop_send --
 *
 * Function for dummy send operation.
 *
 * Parameters:
 *
 * Results:
 *
 * TR_SUCCESS
 *
 * Side effects:
 *
 * Design:
 *
 * Note:
 *
 */
EXPORT int transport_noop_send()
{

	RETURN(TR_SUCCESS);
}



/*
 * transport_init --
 *	Initialise the transport module.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Design:
 *	Set up the transport_switch.
 *
 */
EXPORT boolean_t transport_init()
{
    int	i;

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_TRBUFF,"Transport buffer",2000,FALSE,16,4);

    /*
     * Initialize the transport switch.
     */
    for (i = 0; i < TR_MAX_ENTRY; i++) {
	transport_switch[i].send = transport_no_function;
	transport_switch[i].sendrequest = transport_no_function;
	transport_switch[i].sendreply = transport_no_function;
    }

    transport_switch[TR_NOOP_ENTRY].send = transport_noop_send;

    RETURN(TRUE);

}


