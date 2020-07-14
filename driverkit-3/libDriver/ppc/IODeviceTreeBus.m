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
 * - first checked in. Some stuff from ExpMgr in MacOS.
 */


#import <mach/mach_types.h>
#import <machdep/ppc/DeviceTree.h>

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/ppc/PPCKernBus.h>

#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/ppc/IOPPCDeviceDescriptionPriv.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/ppc/directDevice.h>
#import <string.h>

#import <driverkit/ppc/IODeviceTreeBus.h>

/* DK calls this for PExpert to change DT entries */
extern int PEEditDTEntry( DTEntry dtEntry, char * nodeName, int index,
                    char ** propName , void ** propData, int * propSize);

static const char * device_type = "device_type";

@implementation IODeviceTreeBus

static IOPropertyTable *
MakeReferenceTable( DTEntry dtEntry )
{
    IOReturn		err;
    IOPropertyTable *	propTable;
    DTPropertyIterator	dtIter;
    void *		prop;
    int			propSize;
    char *		name;
    char *		nodeName = NULL;
    int			index;

    propTable = [[IOPropertyTable alloc] init];
    err = DTCreatePropertyIterator( dtEntry, &dtIter);

    if( err == kSuccess) {
	while( kSuccess == DTIterateProperties( dtIter, &name)) {
	    err = DTGetProperty( dtEntry, name, &prop, &propSize );
	    if( err != kSuccess)
		continue;

	    if( 0 == strcmp( name, "name"))
		nodeName = (char *) prop;

	    err = [propTable createProperty:name flags:kReferenceProperty value:prop length:propSize];
	}
	DTDisposePropertyIterator( dtIter);
    }

    index = 0;
    do {
        err = PEEditDTEntry( dtEntry, nodeName, index++, &name, &prop, &propSize);
	if( err)
	    continue;

        [propTable deleteProperty:name];
	if( prop)
            err = [propTable createProperty:name flags:kReferenceProperty value:prop length:propSize];

    } while( err == noErr);

    return( propTable);
}


+ (BOOL)probe:deviceDescription
{
    id	inst;

    // Create an instance and initialize some basic instance variables.
    inst = [[self alloc] initFromDeviceDescription:deviceDescription];
    if (inst == nil)
        return NO;

    [inst setDeviceKind:"Bus"];

    [inst registerDevice];

    [inst probeBus];

    return( YES);
}

- initFromDeviceDescription:(IOTreeDevice *)deviceDescription
{
    void *		prop;
    UInt32		propSize;
    IOReturn		err;
    char		nameBuf[ 64 ];

    if ([super initFromDeviceDescription:deviceDescription] == nil)
	return [super free];

    myDevice = deviceDescription;
    myParent = [deviceDescription parent];
    propTable = [deviceDescription propertyTable];
    myEntry = [deviceDescription getRef];

    [self setUnit:0];
    [myDevice getDevicePath:nameBuf maxLength:sizeof( nameBuf) useAlias:NO];
    [self setName:nameBuf];

    prop = &childSizeCells;
    propSize = 4;
    err = [propTable getProperty:"#size-cells" flags:0
                value:(void **) &prop length:&propSize];
    if( err || (propSize < sizeof(int)))
	childSizeCells = 1;

    prop = &childAddrCells;
    propSize = 4;
    err = [propTable getProperty:"#address-cells" flags:0
                value:(void **) &prop length:&propSize];
    if( err || (propSize < sizeof(int)))
	childAddrCells = 2;

    myAddrCells = [deviceDescription addressCells];
    mySizeCells = [deviceDescription sizeCells];

    return( self);
}


- probeBus
{
    IOReturn		err;
    IOPropertyTable   *	table;
    DTEntryIterator 	iter;
    DTEntry		child;
    void *		prop;
    UInt32     		propSize;
    id			dev;

    DTCreateEntryIterator( (DTEntry) myEntry, &iter );

    while( kSuccess == DTIterateEntries( iter, &child) ) {

	table = MakeReferenceTable( child);

        // Look for existence of a debug property to skip
        if( [table getProperty:"AAPL,ignore" flags:kReferenceProperty
                value:&prop length:&propSize])
            dev = [self createDevice:table ref:(id)child];
	else
	    dev = nil;

	if( nil == dev)
	    [table free];

    }
    DTDisposeEntryIterator( iter);

    return( self);
}


- createDevice:(IOPropertyTable *)newTable ref:ref
{
    id		newDev;

    newDev = [[IOTreeDevice alloc] initAt:newTable parent:self ref:ref];

    return( [newDev publish] );
}

static BOOL CompareName( const char * keys, const char * nameList, IOPropertyTable * table)
{
    BOOL		matched;
    UInt32		keyLen, nameLen;
    const char *	nextKey;
    const char *	names;
    const char *	nextName;
    BOOL		wild;

    do {
	// for each key

	nextKey = strchr( keys, ' ');
	if( nextKey)
	    keyLen = nextKey - keys;
	else
	    keyLen = strlen( keys);
	wild = (keys[ keyLen - 1 ] == '*');

	names = nameList;
	do {
	    // for each name

	    nextName = strchr( names, ' ');
	    if( nextName)
		nameLen = nextName - names;
	    else
		nameLen = strlen( names);

	    if( wild)
		matched = (0 == strncmp( keys, names, keyLen - 1 ));
	    else
		matched =  ((nameLen == keyLen) 
			&& (0 == strncmp( keys, names, keyLen )));

	    if( matched)
		[table createProperty:"AAPL,matched-on" flags:0
		    value:names length:nameLen];

	    names = nextName + 1;

	} while( nextName && (NO == matched));

	keys = nextKey + 1;

    } while( nextKey && (NO == matched));

    return( matched);
}

- (BOOL) match:(IOTreeDevice *)device key:(const char *)keys location:(const char *)location
{
    UInt32		propSize, i;
    IOPropertyTable  *  table;
    const char *	nodeName;
    char       *	compat;
    char       *	tail;
    const char *	devType;
    const char *	model;
    BOOL		matched;

    table = [device propertyTable];

    if( 0 == strcmp( keys, "device-tree")) {

	// match ourselves to anything looking like a bus
	return(
            (noErr == [table getProperty:"ranges" flags:kReferenceProperty
		value:NULL length:&propSize])	);
    }

    nodeName = [device nodeName];

    if( [table getProperty:device_type flags:kReferenceProperty
	    value:(void **)&devType length:&propSize])
	devType = NULL;

    if( [table getProperty:"model" flags:kReferenceProperty
	    value:(void **)&model length:&propSize])
	model = NULL;

    if( [table getProperty:"compatible" flags:kReferenceProperty
	    value:(void **)&compat length:&propSize])
	compat = NULL;
    else {
	// convert separators from null to blanks. Blech.
	compat = (char *)IOMalloc( propSize);
	[table getProperty:"compatible" flags:0
	    value:(void **)&compat length:&propSize];
	for( i = 0; i < propSize - 1; i++)
	    if( compat[ i ] == 0)
		compat[ i ] = ' ';
    }

    // Is this enough ordering?
    matched =  (model && CompareName( keys, model, table))
	    || (compat && CompareName( keys, compat, table))
	    || (nodeName && CompareName( keys, nodeName, table))
	    || (devType && CompareName( keys, devType, table));

    if( compat)
	IOFree( compat, propSize);

    if( matched && location)
	matched = ((tail = [device matchDevicePath:location]) && (tail[0] == 0));

    return( matched );
}

+ probeTree
{
    IOPropertyTable *   table;
    id			root;
    DTEntry		rootEntry;

    DTLookupEntry( (DTEntry) 0, "/", &rootEntry );
    table = MakeReferenceTable( rootEntry);

    root = [[IORootDevice alloc] initAt:table ref:(id)rootEntry];
    if( nil == root)
        return( [table free]);
    else
        return( [root publish]);
}

void DeviceTreeProbe( void )
{
    [IODeviceTreeBus probeTree];
}

// Cells in child nodes
- (ItemCount) sizeCells
{
    return( childSizeCells );
}
- (ItemCount) addressCells
{
    return( childAddrCells );
}

// Given an addr-len cell from our child, find it in our ranges property, then
// recurse to our parent to resolve the base of the range for us.

// Range[]: child-addr  our-addr  child-len
// #cells:    child       ours     child

- (IOReturn) resolveAddressCell:(UInt32 *)cell physicalAddress:(PhysicalAddress *)phys
{
    IOReturn	err;
    UInt32  *	nextRange;
    UInt32  *	endRanges;
    UInt32	start, rangeStart;
    UInt32	rangeAddr[ 5 ];
    UInt32	childCells;
    UInt32	propSize;
    UInt32	result;

    childCells = childAddrCells + childSizeCells;

    err = [propTable getProperty:"ranges" flags:kReferenceProperty
                value:(void **) &nextRange length:&propSize];

    if( err) {
        // 1-1 map at root
        *phys = (PhysicalAddress) cell[ childAddrCells - 1 ];
        return( noErr);
    }

    if( propSize == 0)
        return( [myDevice resolveAddressCell:cell
                physicalAddress:(PhysicalAddress *)phys] );

    if( propSize < (sizeof(int) * (childCells + myAddrCells)) )
        return( IO_R_NO_MEMORY);

    err = IO_R_NO_MEMORY;
    endRanges = nextRange + (propSize / sizeof(int));

    for(    ;
	    nextRange < endRanges;
	    nextRange += (childCells + myAddrCells)) {

	// how is the address compare really supposed to be done?
	if( childAddrCells > 1) {
	    UInt32 space1, space2;
	    // treat 64-bit memory spaces as 32-bit
	    space1 = (nextRange[ 0 ] >> 24) & 0x03;
	    space2 = (cell[ 0 ] >> 24) & 0x03;
	    if( space1 == 3)
		space1 = 2;
	    if( space2 == 3)
		space2 = 2;
	    if( space1 != space2)
		continue;
	}

	rangeStart 	= nextRange[ childAddrCells - 1 ];
	start		= cell[ childAddrCells - 1 ];

	if( (rangeStart <= start)
	&&  ( (start + cell[ childCells - 1 ]) 
		<= (rangeStart + nextRange[ childCells + myAddrCells - 1 ]) )) {

	    // Get the physical start of the range from our parent
	    bcopy( nextRange + childAddrCells, rangeAddr, 4 * myAddrCells );
	    bzero( rangeAddr + myAddrCells, 4 * mySizeCells );
	    err = [myDevice resolveAddressCell:rangeAddr
		    physicalAddress:(PhysicalAddress *)&result];
	    if( err)
		continue;
	    *phys = (PhysicalAddress) (result + start - rangeStart);
	    break;
	}
    }
    return( err);
}

// Find interrupt addressing info

- (IOReturn) interruptSpecCells:(UInt32)handle count:(UInt32 *)count
{
    IOReturn	err;
    UInt32	cells;
    UInt32 *	prop;
    UInt32	propSize;

    err = [propTable getProperty:"AAPL,phandle"
                          flags:kReferenceProperty
                          value:(void **)&prop length:&propSize];
    if( err)
	return( err);

    if( handle && (handle != *prop)) {
	// This is a hack - a non-DT-parent interrupt parent
	// is assumed the root controller.
	*count = 1;
	return( noErr );
    }

    err = [propTable getProperty:"#interrupt-cells"
                          flags:kReferenceProperty
                          value:(void **)&prop length:&propSize];

    cells = childAddrCells;
    if( err == noErr)
	cells += *prop;
    *count = cells;

    return( noErr);
}

// Map one 'unit interrupt specifier' to platform interrupt

- (IOReturn) mapUnitInterrupt:(UInt32 *)interruptSpec
		toInterrupt:(UInt32 **)mapped
		parentHandle:(UInt32)handle
{
    IOReturn	err;
    UInt32 *	map;
    UInt32 *	mapEnd;
    UInt32	propSize;
    UInt32 *	mask;
    UInt32	myICells, parentICells;
    UInt32	iParent;
    int		i;

    err = [self interruptSpecCells:handle count:&myICells];
    if( err)
	return( err);

    if( 1 == myICells) {
	*mapped = interruptSpec;
	return( noErr);
    }

    err = [propTable getProperty:"interrupt-map-mask"
                          flags:kReferenceProperty
                          value:(void **)&mask length:&propSize];
    if( err)
	return( err);
    if( propSize != (4 * myICells))
	return( IO_R_INVALID_ARG);

    err = [propTable getProperty:"interrupt-map"
                          flags:kReferenceProperty
                          value:(void **)&map length:&propSize];
    if( err)
	return( err);

    mapEnd = map + (propSize / 4);

    while( map < mapEnd) {

	for( i = 0; i < myICells; i++) {
	    if( (interruptSpec[ i ] & mask[ i ]) != map[ i ])
		break;
	}

	iParent = map[ myICells ];

	if( i == myICells) {
	    // it matched
            err = [myParent mapUnitInterrupt:(map + myICells + 1)
                            toInterrupt:mapped
                            parentHandle:iParent];
	    break;
	}

	// skip this controller's stuff & loop
        err = [myParent interruptSpecCells:iParent count:&parentICells];
        if( err == noErr)
            map += myICells + parentICells + 1;
	else
	    break;
    }

    return( err);
}

// Create platform interrupts for a child device

- (IOReturn) mapInterrupts:(IOTreeDevice *)device
	interrupts:(UInt32 *)interrupts num:(UInt32 *)count
{
    IOReturn	err;
    UInt32 *	prop;
    UInt32	propSize;
    UInt32	iParent = 0;
    UInt32	intSpec[ 5 ];
    UInt32	myICells;
    UInt32 *	mapped;
    int		i, num;

    do {
        propSize = *count * 4;
        err = [[device propertyTable] getProperty:"interrupts" flags:0
                                value:(void **)&interrupts length:&propSize];
        if( err)
	    continue;
        num = propSize / 4;
	*count = num;

        err = [[device propertyTable] getProperty:"interrupt-parent"
                            flags:kReferenceProperty
                            value:(void **)&prop length:&propSize];
        if( err == noErr)
            iParent = *prop;

        err = [self interruptSpecCells:iParent count:&myICells];
        if( err)
	    continue;
        if( 1 == myICells)
	    continue;

        err = [[device propertyTable] getProperty:"reg"
                    flags:kReferenceProperty
		    value:(void **)&prop length:&propSize];
        if( err)
	    continue;
        bcopy( prop, intSpec, childAddrCells * 4);

	for( i = 0; (err == noErr) && (i < num); i++ ) {
	    // assuming #interrupt-cell == 1
	    intSpec[ childAddrCells ] = interrupts[ i ];
            err = [self mapUnitInterrupt:intSpec
                                toInterrupt:&mapped
                                parentHandle:iParent];
	    interrupts[ i ] = *mapped;
	}

    } while( NO);

    return( err);
}

// Create an AAPL,slot-name property from parent's slot-names

- getSlotName:(IOTreeDevice *) device index:(UInt32)deviceNum
{
    IOReturn		err;
    UInt32	*	prop;
    UInt32		propSize;
    int			i;
    char *		names;
    char *		lastName;
    UInt32		mask;

    err = [propTable getProperty:"slot-names"
                    flags:kReferenceProperty
                    value:(void **) &prop
                    length:&propSize];

    if( err == noErr) {
        mask = *prop;
        names = (char *)(prop + 1);
	lastName = names + (propSize - 4);

        for( 	i = 0;
	    	(i <= deviceNum) && (names < lastName);
		i++ ) {

            if( mask & (1 << i)) {
                if( i == deviceNum) {
                    err = [[device propertyTable]
                                createProperty:"AAPL,slot-name" flags:0
                                value:(void *)names
                                length:(1 + strlen( names))];
                } else
		    names += 1 + strlen( names);
            }
        }
    }

    return( self);
}


// Set the NVRAM description for the child device, which could be a bridge
// as this recurses up the tree.

- makeNVRAMDescriptor:(IOTreeDevice *)device
	descriptor:(NVRAMDescriptor *)propHdr
{
    UInt8	busNum, deviceNum, functionNum;

    [device getLocation:&busNum device:&deviceNum function:&functionNum];

    if( propHdr->format == 0) {
	propHdr->format		= 0x1;
	propHdr->busNum		= busNum;
	propHdr->functionNum	= functionNum;
	propHdr->deviceNum	= deviceNum;

    } else if( propHdr->bridgeCount < 6) {
	propHdr->bridgeDevices |= (deviceNum << (propHdr->bridgeCount * 5));
	propHdr->bridgeCount++;
    }

    if( myParent)
	return( [myParent makeNVRAMDescriptor:myDevice descriptor:propHdr] );

    return( self);
}

- getDevicePath:(IOTreeDevice *)device path:(char *)path maxLength:(int)maxLen
{
    IOReturn	err = noErr;
    int		len;
    ByteCount	propSize;
    char *	nextComp;

    if( myParent)
	if( nil == [myParent getDevicePath:myDevice path:path maxLength:maxLen])
            return( nil);

    len = strlen( path);
    maxLen--;
    nextComp = path + len;

    len++;
    if( len < maxLen) {
	*(nextComp++) = '/';
	len += strlen( [device nodeName]);
	if( len < maxLen) {
	    strcpy( nextComp, [device nodeName] );
	    nextComp = path + len;
	    len++;
	    if( len < maxLen) {
		*(nextComp++) = '@';
		if( [self getDeviceUnitStr:device name:nextComp maxLength:(maxLen - len)])
		    return( self);
	    }
	}
    }
    // failed
    *path = 0;
    return( nil);
}

- getDeviceUnitStr:(IOTreeDevice *)device name:(char *)name maxLength:(int)len
{
    IOReturn	err;
    UInt32  *	regProp;
    ByteCount	propSize;
    char	unitStr[ 10 ] = "0";		// lame default for no reg, just like OF

    err = [[device propertyTable] getProperty:"reg" flags:kReferenceProperty
                value:(void **) &regProp length:&propSize];
    if( err == noErr)
	sprintf( unitStr, "%lx", regProp[ childAddrCells - 1 ]);

    if( strlen( unitStr) < len) {
        strcpy( name, unitStr);
        return( self);
    }
    // failed
    return( nil);
}

- registerLoudly
{
    return( nil);
}

@end



