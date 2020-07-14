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
 * Simon Douglas  26 Jan 98
 * - support PCI-PCI bridges
 */

#import <machdep/ppc/proc_reg.h>
#import "IOMacRiscPCI.h"

#define INFO	if(0)	kprintf

enum {
        kPCIConfigSpace		= 0,
        kPCIIOSpace		= 1,
        kPCI32BitMemorySpace	= 2,
        kPCI64BitMemorySpace	= 3
};

struct AddressCell {
    unsigned int	reloc:1;
    unsigned int	prefetch:1;
    unsigned int	t:1;
    unsigned int	resv:3;
    unsigned int	space:2;
    unsigned int	busNum:8;
    unsigned int	deviceNum:5;
    unsigned int	functionNum:3;
    unsigned int	registerNum:8;
    unsigned int	physMid;
    unsigned int	physLo;
    unsigned int	lengthHi;
    unsigned int	lengthLo;
};
typedef struct AddressCell AddressCell;

#define SWAPLONG(value) (   ((value >> 24) & 0xff) | \
			    ((value >> 8) & 0xff00) | \
			    ((value << 8) & 0xff0000) | \
			    ((value << 24) & 0xff000000) )


@implementation IOPCIBridge

- initFromDeviceDescription:deviceDescription
{
    IOReturn		err;
    UInt32	*	prop;
    ByteCount		propSize;

    do {
	if ([super initFromDeviceDescription:deviceDescription] == nil)
	    continue;

	// Redo the default ranges so less may be pmap'ed
	err = [self mapRegisters:deviceDescription];
	if( err)
	    continue;

        err = [[deviceDescription propertyTable] getProperty:"bus-range"
                    flags:kReferenceProperty value:(void **) &prop length:&propSize];

	if( (err == noErr) && (propSize >= sizeof( int)))
	    primaryBus = *prop;

        if( [deviceDescription isKindOf:[IOPCIDevice class]] )
	    parentBridge = [deviceDescription parent];

	return( self);

    } while( false);

    return( [super free]);
}


- createDevice:(IOPropertyTable *)newTable ref:ref
{
    IOReturn			err;
    IOPCIDevice 	*	newDev;
    AddressCell	    	*	cell;
    ByteCount			propSize;

    newDev = [IOPCIDevice alloc];

    // If we were really probing PCI we would know the device & function
    // numbers but for now just rely on the tree probe to create our children
    // and get the info necessary from the OpenFirmware probe.

    err = [newTable getProperty:"reg" flags:kReferenceProperty
                value:(void **) &cell length:&propSize];
    if( err || (propSize < sizeof(AddressCell)))
        return( nil);

    newDev->busNum = cell->busNum;
    newDev->deviceNum = cell->deviceNum;
    newDev->functionNum = cell->functionNum;

    [newDev initAt:newTable parent:self ref:ref];

    return( [newDev publish] );
}

#if 0
- (BOOL) match:(IOPCIDevice *)device key:(const char *)key location:(const char *)location
{

    // check for vendor, product ID keys and so on...

    // otherwise the default device name matching from IODeviceTreeBus
    return( [super match:device key:key location:location]);
}
#endif

- (IOReturn) configReadLong:(IOPCIDevice *)device
	offset:(UInt32)offset value:(UInt32 *)value
{
    volatile UInt32 *	access;

    access = [self setConfigAddress:device offset:offset];
    *value = SWAPLONG( *access);
    eieio();
    return( noErr);
}

- (IOReturn) configWriteLong:(IOPCIDevice *)device
	offset:(UInt32)offset value:(UInt32)value
{
    volatile UInt32 *	access;

    access = [self setConfigAddress:device offset:offset];
    *access = SWAPLONG( value);
    eieio();
    return( noErr);
}

- getDeviceUnitStr:(IOPCIDevice *)device name:(char *)name maxLength:(int)len
{
    UInt32  *	regProp;
    char	unitStr[ 20 ];

    if( device->functionNum)
        sprintf( unitStr, "%x,%x", device->deviceNum, device->functionNum );
    else
        sprintf( unitStr, "%x", device->deviceNum );
    if( strlen( unitStr) < len) {
	strcpy( name, unitStr);
        return( self);
    }
    // failed
    return( nil);
}

- (IOReturn) mapRegisters:device
{
    return( IO_R_SUCCESS);
}

- (volatile UInt32 *) setConfigAddress:(IOPCIDevice *)device offset:(UInt32)offset;
{
    if( parentBridge)
        return( [parentBridge setConfigAddress:device offset:offset]);

    return( NULL);
}

- (LogicalAddress) getIOAperture;
{
    if( parentBridge)
        return( [parentBridge getIOAperture]);

    return( NULL);
}

@end

@implementation IOMacRiscPCIBridge

- registerLoudly
{
    return( self);
}

- (BOOL) isVCI
{
    return( NO);
}

- (IOReturn) mapRegisters:device
{
    IOReturn		err;
    IORange		realRanges[ 3 ];
    IORange	*	range;
    UInt32		i;
    vm_address_t      *	logical;
    static AddressCell	ioAddrCell = { 0, 0, 0, 0, kPCIIOSpace,
                                        0, 0, 0, 0, 0, 0, 0, 0 };
    UInt32		ioPhys;

    // Look for our IO memory aperture
    err = [self resolveAddressCell:(UInt32 *)&ioAddrCell
            physicalAddress:(PhysicalAddress *)&ioPhys];
    if( err)
        return( err);

    range 		= realRanges;
    range->start 	= ioPhys + 0x00800000;			// config addr
    range->size  	= 0x1000;
    range++;
    range->start 	= ioPhys + 0x00c00000;			// config data
    range->size 	= 0x1000;
    range++;

    if( NO == [self isVCI]) {
	range->start 	= ioPhys;
	range->size 	= 0x10000;				// 64K of non-reloc I/O access !!
	range++;
    }

    [device setMemoryRangeList:0 num:0];
    err = [device setMemoryRangeList:realRanges num:(range - realRanges)];
    if( err)
	return( err);

    range = realRanges;
    logical = (vm_address_t *) &configAddr;
    for( i = 0; i < [device numMemoryRanges]; i++, logical++, range++ ) {

	err = [self mapMemoryRange:i to:logical findSpace:YES cache:IO_CacheOff];
	INFO( "Phys %x = Virt %x\n", range->start, *logical);

	if( err) {
	    IOLog( "%s: mapMemoryRange failed", [self name]);
	    break;
	}
    }
    return( err);
}

- (volatile UInt32 *) setConfigAddress:(IOPCIDevice *)device offset:(UInt32)offset
{
    UInt32		addrCycle;

    if( device->busNum == primaryBus) {
        // primary config cycle
        addrCycle = SWAPLONG(( (1 << device->deviceNum)
                                | (device->functionNum << 8)
                                | (offset & 0xfc)	));

    } else {
        // pass thru config cycle
        addrCycle = SWAPLONG(( (device->busNum << 16)
                             | (device->deviceNum << 11)
                             | (device->functionNum << 8)
                             | (offset & 0xfc)	
			     | 1 		));
    }

    while( *configAddr != addrCycle) {
        (*configAddr) = addrCycle;
        eieio();
    }

    return( configData);
}

- (LogicalAddress) getIOAperture
{
    return( (LogicalAddress) ioAperture);		// zero on VCI
}

@end

@implementation IOMacRiscVCIBridge

- (BOOL) isVCI
{
    return( YES);
}

@end

@implementation IOGracklePCIBridge

- (IOReturn) mapRegisters:device
{
    IOReturn		err;
    IORange		realRanges[ 3 ];
    IORange	*	range;
    UInt32		i;
    vm_address_t      *	logical;
    static AddressCell	ioAddrCell = { 0, 0, 0, 0, kPCIIOSpace,
                                        0, 0, 0, 0, 0, 0, 0, 0 };
    UInt32		ioPhys;

    // Look for our IO memory aperture
    err = [self resolveAddressCell:(UInt32 *)&ioAddrCell
            physicalAddress:(PhysicalAddress *)&ioPhys];
    if( err)
        return( err);

    range 		= realRanges;
    range->start 	= ioPhys + 0x00c00000;			// config addr
    range->size  	= 0x1000;
    range++;
    range->start 	= ioPhys + 0x00e00000;			// config data
    range->size 	= 0x1000;
    range++;
    range->start 	= ioPhys;
    range->size 	= 0x10000;				// 64K of non-reloc I/O access !!
    range++;

    [device setMemoryRangeList:0 num:0];
    err = [device setMemoryRangeList:realRanges num:3];
    if( err)
	return( err);

    range = realRanges;
    logical = (vm_address_t *) &configAddr;
    for( i = 0; i < 3; i++, logical++, range++ ) {

	err = [self mapMemoryRange:i to:logical findSpace:YES cache:IO_CacheOff];
	INFO( "Phys %x = Virt %x\n", range->start, *logical);

	if( err) {
	    IOLog( "%s: mapMemoryRange failed", [self name]);
	    break;
	}
    }
    return( err);
}

- (volatile UInt32 *) setConfigAddress:(IOPCIDevice *)device offset:(UInt32)offset
{
    *configAddr = (SWAPLONG((  (0x80000000)
                            | (device->busNum << 16)
                            | (device->deviceNum << 11)
			    | (device->functionNum << 8)
			    | (offset & 0xfc))));
    eieio();
    return( configData);
}

- (LogicalAddress) getIOAperture
{
    return( (LogicalAddress) ioAperture);
}

@end


