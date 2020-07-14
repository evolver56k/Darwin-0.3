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

#define NM_EXTRA_DEBUG	0

#include <mach/mach.h>
#include <stdio.h>
#include <mach/message.h>

#include "config.h"
#include "debug.h"
#include "netmsg.h"
#include <servers/nm_defs.h>
#include <mach/cthreads.h>


/*
 * Tracing values.
 */
int		trace_recursion_level = 0;



/*
 * ipaddr_to_string
 *	Place an IP address into a string.
 *
 * Parameters:
 *	output_string	: the string to use
 *	input_address	: the address to used
 *
 */
EXPORT void ipaddr_to_string(output_string, input_address)
char		*output_string;
netaddr_t	input_address;
{
    ip_addr_t	ip_address;

    ip_address.ia_netaddr = input_address;
    (void)sprintf(output_string, "%d.%d.%d.%d",
		ip_address.ia_bytes.ia_net_owner,
		ip_address.ia_bytes.ia_net_node_type,
		ip_address.ia_bytes.ia_host_high,
		ip_address.ia_bytes.ia_host_low);
    RET;

}



