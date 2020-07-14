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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * Example of a loadable i386 driver. This driver doesn't do anything
 * other than show that it has been loaded, probed, and has a valid
 * IOEISADeviceDescription.
 *
 * HISTORY
 * 04-Feb-93    Doug Mitchell at NeXT
 *      Created. 
 */
#import <driverkit/i386/directDevice.h>
#import <driverkit/i386/IOEISADeviceDescription.h>
#import <driverkit/generalFuncs.h>
#import "NullDriver.h"
#import <kernserv/kern_server_types.h>

/*
 * Allocate an instance variable to be used by the kernel server interface
 * routines for initializing and accessing this service.
 */
kern_server_t NullDriverInstance;

static int nullDriverUnit = 0;

@implementation NullDriver

/*
 *  Probe, configure board and init new instance.
 */
+ (BOOL)probe:deviceDescription
{
	NullDriver *driver = [self alloc];

	[driver setDeviceDescription:deviceDescription];
	[driver initDriver];
	return YES;
}

- (void)initDriver
{
	id deviceDescription = [self deviceDescription];
	char name[80];
	
	IOLog("NullDriver: interrupt %d channel %d\n",
		[deviceDescription interrupt], 
		[deviceDescription channel]);
	[self setUnit:nullDriverUnit];
	sprintf(name, "NullDriver%d", nullDriverUnit++);
	[self setName:name];
	[self setLocation:NULL];
	[self registerDevice];
	return; 
}

