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
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 07-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#import <stdio.h>
#import <mach/mach_error.h>
#import <kernserv/kern_loader_error.h>
#import <kernserv/kern_loader_types.h>

void kern_loader_error(const char *s, kern_return_t r)
{
	if (r < KERN_LOADER_NO_PERMISSION || r > KERN_LOADER_SERVER_WONT_LOAD)
		mach_error(s, r);
	else
		fprintf(stderr, "%s : %s (%d)\n", s,
			kern_loader_error_string(r), r);
}

static const char *kern_loader_error_list[] = {
	"permission required",
	"unknown server",
	"server loaded",
	"server unloaded",
	"need a server name",
	"server already exists",
	"port already advertised",
	"server won't relocate",
	"server won't load",
	"can't open server relocatable",
	"relocatable malformed",
	"can't allocate server memory",
	"server deleted during operation",
	"server won't unload properly",
	"server detached",
};

const char *kern_loader_error_string(kern_return_t r)
{
	if (r < KERN_LOADER_NO_PERMISSION || r > KERN_LOADER_SERVER_WONT_LOAD)
		return mach_error_string(r);
	else
		return kern_loader_error_list[r - KERN_LOADER_NO_PERMISSION];
}
