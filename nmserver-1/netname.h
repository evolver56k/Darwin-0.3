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
#ifndef	_netname
#define	_netname

/* Module netname */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#ifndef	mig_external
#define mig_external extern
#endif

#include "netname_defs.h"

/* Routine netname_check_in */
mig_external kern_return_t netname_check_in (
	port_t server_port,
	netname_name_t port_name,
	port_t signature,
	port_t port_id);

/* Routine netname_look_up */
mig_external kern_return_t netname_look_up (
	port_t server_port,
	netname_name_t host_name,
	netname_name_t port_name,
	port_t *port_id);

/* Routine netname_check_out */
mig_external kern_return_t netname_check_out (
	port_t server_port,
	netname_name_t port_name,
	port_t signature);

/* Routine netname_version */
mig_external kern_return_t netname_version (
	port_t server_port,
	netname_name_t version);

#endif	_netname
