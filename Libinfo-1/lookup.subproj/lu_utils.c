/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Port and memory management for doing lookups to the lookup server
 * Copyright (C) 1989 by NeXT, Inc.
 */
/*
 * HISTORY
 * 27-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed to use bootstrap port instead of service port.
 *
 */
#include <stdlib.h> 			/* imports NULL */
#include <mach/mach.h>
#include "lu_utils.h"

extern port_t _lookupd_port(port_t);

/* called as child starts up.  mach ports aren't inherited: trash cache */
void
_lu_fork_child()
{
	_lu_port = PORT_NULL;
}

void
_lu_setport(port_t desired)
{
	if (_lu_port != PORT_NULL) {
		port_deallocate(task_self(), _lu_port);
	}
	_lu_port = desired;
}

static int
port_valid(port_t port)
{
	port_set_name_t enabled;
	int num_msgs;
	int backlog;
	boolean_t owner;
	boolean_t receiver;

	return (port_status(task_self(), port, &enabled, &num_msgs, &backlog,
			    &owner, &receiver) == KERN_SUCCESS);
}

int
_lu_running(void)
{
	if (_lu_port != PORT_NULL) {
		if (port_valid(_lu_port)) {
			return (1);
		}
		_lu_port = PORT_NULL;
	}
	_lu_port = _lookupd_port(0);
	if (_lu_port != PORT_NULL && _lu_port != 4)	// not defined
		return 1;
	else
		return 0;
}
