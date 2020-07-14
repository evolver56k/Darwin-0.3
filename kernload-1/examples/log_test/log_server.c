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
 * Copyright 1989, 1990 NeXT, Inc.  All rights reserved.
 *
 * Loadable kernel server example showing how to log events.
 *
 * This server accepts two messages:
 *	log_msg() Logs the specified message using the level provided.
 *	log_async() Logs the specified message asynchronusly as specified.
 */
#include "log.h"
#include <kernserv/kern_server_types.h>
#include <mach/mig_errors.h>

/*
 * Allocate an instance variable to be used by the kernel server interface
 * routines for initializing and accessing this service.
 */
kern_server_t instance;

/*
 * Stamp our arival.
 */
void log_init(void)
{
	printf("Log test kernel server initialized\n");
}

/*
 * Notify the world that we're going away.
 */
void log_signoff(void)
{
	printf("Log test kernel server unloaded\n");
}

/*
 * Print the passed string on the console.
 */
kern_return_t log_msg (
	port_t		log_port,
	int		level)
{
	kern_serv_log(&instance, level, "logged message", 0, 0, 0, 0, 0);
	return KERN_SUCCESS;
}

/*
 * Set this message up to be logged asynchronously.
 */
kern_return_t log_async (
	port_t		log_port,
	int		level,
	int		interval,
	int		iterations)
{
}
