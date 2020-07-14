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
 * Simple loadable kernel server example.
 *
 * This server accepts two messages:
 *	simple_puts() prints the (inline string) argument on the console.
 *	simple_vers() returns the running kernel's version string.
 */
#import "simple_types.h"
#import <kernserv/kern_server_types.h>
#import <mach/mig_errors.h>

/*
 * Allocate an instance variable to be used by the kernel server interface
 * routines for initializing and accessing this service.
 */
kern_server_t instance;

/*
 * Stamp our arival.
 */
void simple_init(void)
{
	printf("Simple kernel server initialized\n");
}

/*
 * Notify the world that we're going away.
 */
void simple_signoff(void)
{
	printf("Simple kernel server unloaded\n");
}

/*
 * Print the passed string on the console.
 */
kern_return_t simple_puts (
	void		*arg,
	simple_msg_t	string)
{
	printf(string);
	return KERN_SUCCESS;
}

/*
 * Return the kernel version string to the caller.
 */
kern_return_t simple_vers (
	void		*arg,
	simple_msg_t	string)
{
	extern char version[];
	strcpy(string, version);
	return KERN_SUCCESS;
}
