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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in.
 */



#define KERNEL_PRIVATE	1

#import <objc/List.h>

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/ppc/PPCKernBus.h>

#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/ppc/IOPPCDeviceDescriptionPriv.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/ppc/directDevice.h>

#import <machdep/ppc/DeviceTree.h>
#import <machdep/ppc/proc_reg.h>
#import "IOMacRiscPCI.h"


struct _pciprivate {

    IOMacRiscPCIBridge *parent;
};


@implementation IOPCIDevice

- initAt:(IOPropertyTable *)propTable parent:_parent ref:ref
{
    struct _pciprivate *private;

    _dtpciprivate = (void *)IOMalloc(sizeof (struct _pciprivate));
    bzero(_dtpciprivate, sizeof (struct _pciprivate));

    private = _dtpciprivate;
    private->parent = (IOMacRiscPCIBridge *) _parent;

    return( [super initAt:propTable parent:_parent ref:ref]);
}

- (IOReturn) resolveAddressing
{
    IOReturn	err;
    UInt32   *  cells;
    UInt32	myCells;
    UInt32	propSize;

    myCells = [self addressCells] + [self sizeCells];		// or, 5

    // Same as IODeviceTree, except "reg" is "assigned-addresses"
    err = [[self propertyTable] getProperty:"assigned-addresses" flags:kReferenceProperty
                value:(void **) &cells length:&propSize];
    if( err == noErr) {
	err = [self findMemoryApertures:cells num:(propSize / (4 * myCells))];
    } else
	err = noErr;

    return( err);
}

- (IOReturn) resolveInterrupts
{
    IOReturn		err;
    IOPropertyTable  *	propTable = [self propertyTable];
    ByteCount		propSize;

    [propTable createProperty:"AAPL,dk_Share IRQ Levels" flags:0
                value:"YES" length:4];

    err = [super resolveInterrupts];

    if( err && (noErr == [propTable getProperty:"interrupts" flags:kReferenceProperty
		value:NULL length:&propSize])) {
	// inherit & share parent's
        IODeviceDescription  *	bridge;

        bridge = [[self parent] deviceDescription];
	if( bridge) {
            err = [self setInterruptList:[bridge interruptList] num:[bridge numInterrupts]];
	}

    } else
	err = noErr;

    return( err);
}

- getResources
{
    [super getResources];

    [[self parent] getSlotName:self index:deviceNum];

    return( self);
}

- (IOReturn) configReadLong:(UInt32)offset value:(UInt32 *)value
{
    return( [[self parent] configReadLong:self offset:offset value:value] );
}

- (IOReturn) configWriteLong:(UInt32)offset value:(UInt32)value
{
    return( [[self parent] configWriteLong:self offset:offset value:value] );
}

- (LogicalAddress) getIOAperture
{
    return( [[self parent] getIOAperture] );
}

- getLocation:(UInt8 *)bus device:(UInt8 *)device function:(UInt8 *)function
{
    *bus 	= busNum;
    *device 	= deviceNum;
    *function 	= functionNum;
    return( self);
}


- property_IODeviceType:(char *)types length:(unsigned int *)maxLen
{
    [super property_IODeviceType:types length:maxLen];
    strcat( types, " "IOTypePCI);
    return( self);
}

@end


