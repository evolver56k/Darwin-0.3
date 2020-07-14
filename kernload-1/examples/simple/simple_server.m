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
#import "simple_handler.h"
#import <kernserv/kern_server_types.h>
#import <mach/mig_errors.h>
#import <objc/Object.h>

kern_return_t s_puts (
	void		*arg,
	simple_msg_t	string);

kern_return_t s_vers (
	void		*arg,
	simple_msg_t	string);

simple_t simple = {
	0,
	100,
	s_puts,
	s_vers
};

death_pill_t *simple_outmsg;

@interface Test : Object
{
	int	var1;
}
- init;
- sayhello;
@end

@implementation Test

- init
{
	[super init];
	var1 = 5;
	return self;
}

- sayhello
{
	printf("Hello from an instance of Test, var1 = %d\n", var1);
	return self;
}
@end

/*
 * Allocate an instance variable to be used by the kernel server interface
 * routines for initializing and accessing this service.
 */
kern_server_t instance;
id myobj;

/*
 * Stamp our arival.
 */
void simple_init(void)
{
	simple_outmsg = (death_pill_t *)kalloc(simpleMaxReplySize);
	printf("Simple kernel server initialized\n");
	myobj = [[Test alloc] init];
	[myobj sayhello];
}

/*
 * Notify the world that we're going away.
 */
void simple_signoff(void)
{
	kfree(simple_outmsg, simpleMaxReplySize);
	simple_outmsg = 0;
	[myobj free];
	printf("Simple kernel server unloaded\n");
}

/*
 * Print the passed string on the console.
 */
kern_return_t s_puts (
	void		*arg,
	simple_msg_t	string)
{
	printf(string);
	return KERN_SUCCESS;
}

/*
 * Return the kernel version string to the caller.
 */
kern_return_t s_vers (
	void		*arg,
	simple_msg_t	string)
{
	extern char version[];
	strcpy(string, version);
	return KERN_SUCCESS;
}
