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
 * Loadable kernel server example showing how to panic.
 */

#include "panic.h"

#import <mach.h>
#import <stdlib.h>
#import <libc.h>
#import <stdio.h>
#import <strings.h>
#import <mach_error.h>
#import <mig_errors.h>
#import <servers/netname.h>

/*
 * Communication with the kernel server loader.
 */
main(int ac, char **av)
{
	kern_return_t r;
	port_name_t panic_port;
	char buf[80];

	/*
	 * Look up the advertized port of the loadable server.
	 */
	r = netname_look_up(name_server_port, "",
			    "panic", &panic_port);
	if (r != KERN_SUCCESS) {
		mach_error("panic: can't find panic server", r);
		exit(1);
	}

	r = panic_panic(panic_port);
	if (r != KERN_SUCCESS) {
		mach_error("panic", r);
		exit(1);
	}
	exit(0);
}
