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
 */

#include "simple.h"

#import <mach/mach.h>
#import <stdlib.h>
#import <libc.h>
#import <stdio.h>
#import <strings.h>
#import <mach/mach_error.h>
#import <mach/mig_errors.h>
#import <servers/netname.h>

/*
 * Communication with the kernel server loader.
 */
main(int ac, char **av)
{
	kern_return_t r;
	port_name_t simple_port;
	char buf[256];
	char *msg;

	if (ac == 2) {	/* message on command line */
		av++;
		msg = *av;
	} else 
		msg = "Hello World\n";

	/*
	 * Look up the advertized port of the loadable server.
	 */
	r = netname_look_up(name_server_port, "",
			    "simple", &simple_port);
	if (r != KERN_SUCCESS) {
		mach_error("simple: can't find simple server", r);
		exit(1);
	}

/*	r = simple_puts(simple_port, "Hello, World\n"); */
	r = simple_puts(simple_port, msg);
	if (r != KERN_SUCCESS) {
		mach_error("simple: simple_puts", r);
		exit(1);
	}
	r = simple_vers(simple_port, buf);
	if (r != KERN_SUCCESS) {
		mach_error("simple: simple_vers", r);
		exit(1);
	}
	printf("kernel returns version: %s\n", buf);
	exit(0);
}


