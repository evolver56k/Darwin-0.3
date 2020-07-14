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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PPC direct device implementation.
 *
 */

#import <machkit/NXLock.h>
#import <objc/List.h>

#import <driverkit/IODirectDevice.h>
#import <driverkit/IODirectDevicePrivate.h>
#import <driverkit/driverTypes.h>
#import <driverkit/generalFuncs.h>

#import <driverkit/ppc/directDevice.h>
#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/ppc/driverTypes.h>
#import <driverkit/ppc/driverTypesPrivate.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/ppc/PPCKernBus.h>
#import <driverkit/KernDevice.h>
#import <driverkit/KernDeviceDescription.h>
#import <driverkit/interruptMsg.h>

#import <machdep/ppc/xpr.h>

#import <kernserv/lock.h>
#import <kernserv/prototypes.h>

#import <objc/HashTable.h>


@interface IODirectDevice(PPCPrivate)

- initPPC;
- freePPC;

@end

struct _ppc_private {
    Arch	type;
};

@implementation IODirectDevice(PPCPrivate)

- initPPC
{
    	struct _ppc_private	*private;

	private = _busPrivate = (void *)IOMalloc(sizeof (*private));
	private->type = ArchPPC;
	
	return self;
}

- freePPC
{
    	struct _ppc_private	*private = _busPrivate;
	
	IOFree((void *)private, sizeof (*private));
	
	return self;
}

@end

@implementation IODirectDevice(IOPPCDirectDevice)

@end

