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


#import <driverkit/ppc/IOTreeDevice.h>

#pragma options align=mac68k

struct NVRAMDescriptor {
    unsigned int format:4;
    unsigned int marker:1;
    unsigned int bridgeCount:3;
    unsigned int busNum:2;
    unsigned int bridgeDevices:6 * 5;
    unsigned int functionNum:3;
    unsigned int deviceNum:5;
};
typedef struct NVRAMDescriptor NVRAMDescriptor;

struct NVRAMProperty
{
    NVRAMDescriptor	header;
    UInt8		nameLen;
    UInt8		name[ kMaxNVNameLen ];
    UInt8		dataLen;
    UInt8		data[ kMaxNVDataLen ];
};
typedef struct NVRAMProperty NVRAMProperty;

#pragma options align=reset

@interface IOTreeDevice(Private)

+ findForIndex:(UInt32 )index;
- initAt:propTable parent:parent ref:ref;
- getRef;

- (ItemCount) sizeCells;
- (ItemCount) addressCells;
- (IOReturn) resolveAddressing;
- (IOReturn) findMemoryApertures:(UInt32 *)cells num:(UInt32)num;
- (IOReturn) resolveAddressCell:(UInt32 *)cell physicalAddress:(PhysicalAddress *)phys;
- (IOReturn) resolveInterrupts;

- getLocation:(UInt8 *)bus device:(UInt8 *)device function:(UInt8 *)function;
- (IOReturn) setNVRAMProperty:(const char *)propertyName;
- (BOOL) isNVRAMProperty:(const char *)propertyName;
- denyNVRAM:(BOOL)flag;
- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen;

@end

@interface IORootDevice : IOTreeDevice
{
}

- initAt:(IOPropertyTable *)_propTable ref:_ref;

@end

@interface IODeviceTreeBus : IODirectDevice
{
@private
    id				myEntry;
    IOPropertyTable 	*	propTable;
    IOTreeDevice	* 	myDevice;
    id				myParent;
    UInt32			myAddrCells;
    UInt32			mySizeCells;
    UInt32			childSizeCells;
    UInt32			childAddrCells;
    int				_IODeviceTreeBus_reserved[8];
}

+ probeTree;
- probeBus;
- createDevice:(IOPropertyTable *)newTable ref:ref;
- (BOOL) match:(IOTreeDevice *)device key:(const char *)key location:(const char *)location;
- (IOReturn) resolveAddressCell:(UInt32 *)cell physicalAddress:(PhysicalAddress *)phys;
- (ItemCount) addressCells;
- (ItemCount) sizeCells;
- makeNVRAMDescriptor:device descriptor:(NVRAMDescriptor *)propHdr;
- getDevicePath:(IOTreeDevice *)device path:(char *)path maxLength:(int)maxLen;
- getDeviceUnitStr:(IOTreeDevice *)device name:(char *)name maxLength:(int)len;
- getSlotName:(IOTreeDevice *) device index:(UInt32)deviceNum;
- (IOReturn) mapInterrupts:(IOTreeDevice *)device
	interrupts:(UInt32 *)interrupts num:(UInt32 *)num;

@end

