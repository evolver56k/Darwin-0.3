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


#import <driverkit/IODeviceDescription.h>
#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/driverTypes.h>
#import <machdep/ppc/DeviceTree.h>

#import <driverkit/ppc/IOTreeDevice.h>

#ifdef	KERNEL

@interface IOPCIDevice : IOTreeDevice
{
@public
	UInt8           busNum;
	UInt8           deviceNum;
	UInt8           functionNum;
@private
	void	*	_dtpciprivate;
        int		_IOPCIDevice_reserved[8];
}

- (IOReturn) configReadLong:(UInt32)offset value:(UInt32 *)value;
- (IOReturn) configWriteLong:(UInt32)offset value:(UInt32)value;
- (LogicalAddress) getIOAperture;

@end


#endif	KERNEL

