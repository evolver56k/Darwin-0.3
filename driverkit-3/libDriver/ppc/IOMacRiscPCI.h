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


#import <driverkit/ppc/IODeviceTreeBus.h>
#import <driverkit/ppc/IOPCIDevice.h>


@interface IOPCIBridge : IODeviceTreeBus
{
    id			parentBridge;		// or nil
    UInt8		primaryBus;
    int			__reserved[ 4 ];
}
- (IOReturn) configWriteLong:(IOPCIDevice *)device
	offset:(UInt32)offset value:(UInt32)value;
- (IOReturn) configReadLong:(IOPCIDevice *)device
	offset:(UInt32)offset value:(UInt32 *)value;
- (LogicalAddress) getIOAperture;
- getDeviceUnitStr:(IOPCIDevice *)device name:(char *)name maxLength:(int)len;

// implemented by subclasses
- (IOReturn) mapRegisters:device;
- (volatile UInt32 *) setConfigAddress:(IOPCIDevice *)device offset:(UInt32)offset;
- (LogicalAddress) getIOAperture;

@end

@interface IOMacRiscPCIBridge : IOPCIBridge
{
@private
	volatile UInt32	*	configAddr;
	volatile UInt32	*	configData;
	volatile UInt32	*	ioAperture;
        int			___reserved[ 4 ];
}

- (BOOL) isVCI;

@end

@interface IOMacRiscVCIBridge : IOMacRiscPCIBridge
{
}
@end

@interface IOGracklePCIBridge : IOPCIBridge
{
@private
	volatile UInt32	*	configAddr;
	volatile UInt32	*	configData;
	volatile UInt32	*	ioAperture;
        int			___reserved[ 4 ];
}
@end

