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


#import <mach/mach_types.h>
#import <machdep/ppc/DeviceTree.h>
#import <machdep/ppc/powermac.h>

#import <driverkit/KernDeviceDescription.h>
#import <driverkit/ppc/PPCKernBus.h>

#import <driverkit/ppc/IOPPCDeviceDescription.h>
#import <driverkit/ppc/IOPPCDeviceDescriptionPriv.h>
#import <driverkit/IODeviceDescriptionPrivate.h>
#import <driverkit/ppc/directDevice.h>

#import <driverkit/ppc/IODeviceTreeBus.h>

// platform expert
extern vm_offset_t
PEResidentAddress( vm_offset_t address, vm_size_t length );


#define INFO	if(0)	kprintf


struct tree_private {
    IOTreeDevice	* 	listNext;
    id   			parent;
    id				ref;
    IOPropertyTable   *		propTable;
    char			nodeName[ 40 ];
    BOOL			matched;
    BOOL			denyNVRAM;
    UInt32             		numMaps;
    UInt32			addressCells;
    UInt32			sizeCells;
    IOApertureInfo     	 	maps[8];         // FIXME
    char			nvramPropName[ 5 ];
};

@implementation IOTreeDevice


- initAt:(IOPropertyTable *)_propTable parent:_parent ref:_ref
{
    struct tree_private	*private;
    IOReturn		err;
    char 	    *	prop;
    ByteCount		propSize;

    [super _initWithDelegate:nil];

    _tree_private = (void *)IOMalloc(sizeof (struct tree_private));
    bzero(_tree_private, sizeof (struct tree_private));
    private = _tree_private;

    private->parent = _parent;
    private->ref = _ref;
    private->propTable = _propTable;

    propSize = 40;
    prop = private->nodeName;
    err = [_propTable getProperty:"name" flags:0
		value:(void **) &prop length:&propSize];
    private->nodeName[ propSize ] = 0;

    INFO("---- Adding device %s\n", private->nodeName );

    if( _parent) {
        private->addressCells = [_parent addressCells];
        private->sizeCells = [_parent sizeCells];
    } else	// must be device-tree root
        private->addressCells = private->sizeCells = 1;

    return( self);
}

- (IOReturn) resolveAddressing
{
    struct tree_private	*private = _tree_private;
    IOReturn		err;
    UInt32   	*  	cells;
    ByteCount		propSize;

    err = [private->propTable getProperty:"reg" flags:kReferenceProperty
                value:(void **) &cells length:&propSize];

    if( err == noErr) {

	err = [self findMemoryApertures:cells
	    num:(propSize / (4 * (private->addressCells + private->sizeCells)))];
    } else
	err = noErr;

    return( err);
}


- (IOReturn) findMemoryApertures:(UInt32 *)cells num:(UInt32)numCells
{
    struct tree_private	*private = _tree_private;
    IOReturn		err;
    UInt32		num;
    IOApertureInfo  *	map;
    IORange	    *	ranges = NULL;
    IOLogicalAddress *	aaplAddress = NULL;
    UInt32		myCells;

    do {
        ranges = (IORange *) IOMalloc( numCells * sizeof( IORange));
	if( ranges == NULL)
	    continue;
        aaplAddress = (IOLogicalAddress *) IOMalloc( numCells * sizeof( IOLogicalAddress));
        if( aaplAddress == NULL)
            continue;

        myCells = private->addressCells + private->sizeCells;
        map = private->maps;
        private->numMaps = 0;
	num = numCells;

        while( num--) {

            err = [private->parent resolveAddressCell:cells
                    physicalAddress:(PhysicalAddress) &map->physical];

            // resolvePhysicalAddresses should return only memory mapped apertures
            if( err == noErr) {

                map->length 	= cells[ myCells - 1 ];
#if 0
                map->length 	= 0xfffff000 & (0xfff + map->length);
#endif
                map->logical 	= (IOLogicalAddress)
                    PEResidentAddress( map->physical, map->length);
                map->cacheMode 	= 0;
                map->usage 	= 0;

                ranges[ private->numMaps ].start = map->physical;
                ranges[ private->numMaps ].size = map->length;

                INFO("Physical = %08x, %08x\n", map->physical, map->length );

                aaplAddress[ private->numMaps++ ] = map->logical;
                map++;
            }

            cells += myCells;
        }

        [private->propTable createProperty:"AAPL,dk_Share "MEM_MAPS_KEY flags:0
                value:"Y" length:1];
        [private->propTable createProperty:"AAPL,address" flags:0
                value:aaplAddress length:(private->numMaps * sizeof( IOLogicalAddress))];

        err = [self setMemoryRangeList:ranges num:private->numMaps];
        if( err)
            IOLog("%s: couldn't get physical range.\n", [self nodeName]);

    } while( false);

    if( ranges)
        IOFree( ranges, numCells * sizeof( IORange));
    if( aaplAddress)
        IOFree( aaplAddress, numCells * sizeof( IOLogicalAddress));

    return( err);
}


- (IOReturn) resolveAddressCell:(UInt32 *)cell physicalAddress:(PhysicalAddress *)phys
{
    struct tree_private	*private = _tree_private;

    // root has no regs or ranges so shouldn't get here
    return( [private->parent resolveAddressCell:cell physicalAddress:phys] );
}

- (IOReturn) resolveInterrupts
{
    struct tree_private	*private = _tree_private;
    id			propTable;
    IOReturn		err;
    UInt32		propSize;
    void	*	prop;
    UInt32		i;
    UInt32		intBuf[ 16 ];

    propTable = private->propTable;

    prop = intBuf;
    propSize = sizeof( intBuf);
    err = [propTable getProperty:"AAPL,interrupts" flags:0
                value:&prop length:&propSize];

    if (err == noErr)
	propSize = propSize / 4;

    else {
	// Try NewWorld mapping
	// NB: IsYosemite == IsNewWorld
        if ([self parent] && (IsYosemite())) {
	    propSize = 16;
	    err = [[self parent] mapInterrupts:self
			interrupts:intBuf num:&propSize];
            if( err == noErr) {
		// only necessary for ATY66
                [propTable createProperty:"AAPL,interrupts" flags:0
                            value:intBuf length:(propSize * 4)];
	    }
	}
    } 

    if( err == noErr) {

	for( i = 0; i < propSize; i++ ) {
	    INFO("{%x", intBuf[ i ]);
	    intBuf[ i ] ^= 0x18;		// !!remove : byte reverse bit number
	    INFO(" = %x}\n", intBuf[ i ]);
	}

	err = [self setInterruptList:(unsigned int *)intBuf num:propSize];
        if( err)
	    IOLog("%s: couldn't get interrupts.\n", [self nodeName]);
    }

    return( err);
}

- getResources
{
    struct tree_private	*private = _tree_private;
    IOReturn		err;
    UInt8		nvramProp[ 8 ];
    ByteCount		propSize;

    [self resolveInterrupts];
    [self resolveAddressing];

    // read the nvram property into the table

    err = [self readNVRAMProperty:private->nvramPropName value:(void *)nvramProp length:&propSize];
    if( err == noErr)
        err = [private->propTable createProperty:private->nvramPropName flags:0
			value:nvramProp length:propSize ];
    return( self);
}

- (IOReturn) getApertures:(IOApertureInfo *)info items:(UInt32 *)items
{
    struct tree_private	*private = _tree_private;
    UInt32	count = *items;

    *items = private->numMaps;
    if( info) {
        if( count > private->numMaps)
            count = private->numMaps;
        bcopy( private->maps, info, count * sizeof( IOApertureInfo));
    }
    return( noErr);
}

// This is used to generate the NVRAM node description.
// It will make "PCI" IDs for non-PCI devices, using the Expansion Mgr method.

- getLocation:(UInt8 *)bus device:(UInt8 *)device function:(UInt8 *)function
{
    struct tree_private	*private = _tree_private;
    UInt32		regHi;
    IOReturn		err;
    UInt32   	*  	cells;
    ByteCount		propSize;

    err = [private->propTable getProperty:"reg" flags:kReferenceProperty
                value:(void **) &cells length:&propSize];

    if( err) {
	*bus 		= 0;
	*function 	= 0;
	*device 	= 0;
	return( nil);
    }

    regHi = *cells;
    if( (regHi & 0xf0000000) != 0xf0000000) {
	// ExpMgr considers this a PCI device!
	// Wacky, but works out well for gc's children.
	*bus 		= 0x03 & (regHi >> 16);
	*function 	= 0x07 & (regHi >> 8);
	*device 	= 0x1f & (regHi >> 11);

    } else {
	*bus 		= 3;
	*function 	= 0;
	*device 	= 0x1f & (regHi >> 24);
    }
    return( self);
}

- getRef
{
    struct tree_private	*private = _tree_private;

    return( private->ref);
}

- setDelegate:delegate
{
    id		ret;

    ret = [super _initWithDelegate:delegate];

    if( ret && delegate) {
	[self getResources];
//	[[delegate bus] allocateResourcesForDeviceDescription:delegate];
    }
    return( ret);
}

- (IOConfigTable *) configTable
{
    struct tree_private	*  private = _tree_private;

    return( (IOConfigTable *) private->propTable);
}

- propertyTable
{
    struct tree_private	*private = _tree_private;

    return( private->propTable);
}

- parent
{
    struct tree_private	*private = _tree_private;

    return( private->parent);
}

- (char *)nodeName
{
    struct tree_private	*private = _tree_private;

    return( private->nodeName);
}

- (ItemCount) addressCells
{
    struct tree_private	*private = _tree_private;

    return( private->addressCells);
}

- (ItemCount) sizeCells
{
    struct tree_private	*private = _tree_private;

    return( private->sizeCells);
}


- (BOOL) match:(const char *)key location:(const char *)location
{
    struct tree_private	*private = _tree_private;

    return( [private->parent match:self key:key location:location]);
}

- taken:(BOOL)taken
{
    struct tree_private	*private = _tree_private;

    private->matched = taken;
    return( self);
}

static IOTreeDevice *   firstDevice;
static IOTreeDevice *   lastDevice;

- publish
{
    struct tree_private	*private;

    if( firstDevice == nil)
	firstDevice = self;
    if( lastDevice) {
	private = lastDevice->_tree_private;
        private->listNext = self;
    }
    lastDevice = self;
    PublishDevice( self);
    return( self);
}

+ findMatchingDevice:(const char *)key location:(const char *)location
{
    IOTreeDevice	*   	list = firstDevice;
    struct tree_private	*	private;

    while( list) {
	private = list->_tree_private;

	if( (NO == private->matched) && ([list match:key location:location]) )
	    return( list);
	list = private->listNext;
    }
    return( nil);
}

+ findForIndex:(UInt32 )index
{
    IOTreeDevice	*   	list = firstDevice;
    struct tree_private	*	private;

    while( list && (index--)) {
	private = list->_tree_private;
	list = private->listNext;
    }
    return( list);
}


////////
//////// NVRAM device property access.
////////

// move these to platform expert:
// Can't be used anymore...
#if 0
enum {
    kXPRAMNVPartition		= 0x1300,
    kNameRegistryNVPartition	= 0x1400,
    kOpenFirmwareNVPartition	= 0x1800,
};
#endif
extern IOReturn ReadNVRAM( unsigned int offset, unsigned int length, unsigned char * buffer );
extern IOReturn WriteNVRAM( unsigned int offset, unsigned int length, unsigned char * buffer );
////


#define READINC( size,dest )	ReadNVRAM( nvpc, size, (unsigned char *) dest); \
				nvpc += size;

#define MIN(a,b) ((a > b) ? b : a)


static UInt16
SearchNVRAMProperty( NVRAMDescriptor * propHdr, UInt16 * theEnd )
{
UInt16		nvpc, nvEnd;
NVRAMDescriptor	nvDescrip;

    nvpc = NVRAM_NameRegistry_Offset;
    READINC( sizeof( short), &nvEnd );

    if( IsYosemite())
	nvEnd += NVRAM_NameRegistry_Offset - 0x100;

    if( (nvEnd < NVRAM_NameRegistry_Offset) || (nvEnd >= (NVRAM_NameRegistry_Offset + 0x400)))
	nvEnd = NVRAM_NameRegistry_Offset + 2;
    *theEnd = nvEnd;

    while( (nvpc + sizeof( NVRAMProperty)) <= nvEnd ) {

	READINC( sizeof( NVRAMDescriptor), &nvDescrip )
	if( 0 == bcmp( &nvDescrip, propHdr, sizeof( NVRAMDescriptor)))
	    return( nvpc );
	else
	    nvpc += (sizeof( NVRAMProperty) - sizeof( NVRAMDescriptor));
    }

    return( 0);
}

- (IOReturn) readNVRAMProperty:(char *)name value:(void *)buffer length:(ByteCount *)length
{
    IOReturn		err = nrNotFoundErr;
    UInt16		nvpc, nvEnd;
    unsigned char	nameLen, dataLen;
    NVRAMDescriptor	propHdr;

    bzero( &propHdr, sizeof( NVRAMDescriptor));
    [[self parent] makeNVRAMDescriptor:self descriptor:&propHdr];

#if 0
    kprintf("makeNVRAMDescriptor:bridgeCount %x, busNum %x, bridgeDevices %x, functionNum %x, deviceNum %x\n", 
	propHdr.bridgeCount, propHdr.busNum, propHdr.bridgeDevices, propHdr.functionNum, propHdr.deviceNum );
#endif

    nvpc = SearchNVRAMProperty( &propHdr, &nvEnd);

    if( nvpc) {
	READINC( sizeof( UInt8), &nameLen )
	nameLen = MIN( nameLen, 4);
	ReadNVRAM( nvpc, nameLen, (unsigned char *) name);
	name[ nameLen ] = 0;
	nvpc += kMaxNVNameLen;
	READINC( sizeof( UInt8), &dataLen )
	dataLen = MIN( dataLen, 8);
	*length = dataLen;
	ReadNVRAM( nvpc, dataLen, (unsigned char *) buffer);
	err = noErr;
    }
    return( err);
}

- (IOReturn) writeNVRAMProperty:(const char *)name value:(void *)value length:(ByteCount)length
{
    IOReturn		err = noErr;
    UInt16		nvpc, nvEnd;
    UInt8		nameLen;
    NVRAMProperty	nvram;

    nameLen = strlen( name);
    if( (nameLen > kMaxNVNameLen) || (length > kMaxNVDataLen))
	return( nrOverrunErr);

    nvram.nameLen = nameLen;
    bcopy( name, nvram.name, nameLen);
    nvram.dataLen = length;
    bcopy( value, nvram.data, length);

    bzero( &nvram.header, sizeof( NVRAMDescriptor));
    [[self parent] makeNVRAMDescriptor:self descriptor:&nvram.header];
    nvpc = SearchNVRAMProperty( &nvram.header, &nvEnd);

    if( nvpc == 0) {

	// Not found - append it.
	nvpc = nvEnd;
	nvEnd += sizeof( NVRAMProperty);
	if( nvEnd > (NVRAM_NameRegistry_Offset + 0x400))
	    err = nrNotEnoughMemoryErr;
	else {
	    WriteNVRAM( nvpc, sizeof( NVRAMProperty), (UInt8 *) &nvram );
            if( IsYosemite())
                nvEnd -= NVRAM_NameRegistry_Offset - 0x100;
	    WriteNVRAM( NVRAM_NameRegistry_Offset, 2, (UInt8 *) &nvEnd );
	}

    } else {

	// Overwrite the current one. They're all maximum length.
	WriteNVRAM( nvpc, sizeof( NVRAMProperty) - sizeof( NVRAMDescriptor), (UInt8 *) &nvram.nameLen );
    }

#if 0
    {
	int i;
	ReadNVRAM( NVRAM_NameRegistry_Offset, 0x50, buffer);
	for( i=0; i<0x50; i++) {
	    kprintf(" 0x%02x,", buffer[i]);
	    if( (i%16) == 15) kprintf("\n");
	}
    }
#endif

    return( err);
}

// The following are property table based for NDRV support

- (IOReturn) setNVRAMProperty:(const char *)propertyName
{
    struct tree_private	*private = _tree_private;
    IOReturn		err;
    void * 		value;
    ByteCount		propSize;

    err = [[self propertyTable] getProperty:propertyName flags:kReferenceProperty
                value:&value length:&propSize];
    if( err)
        return( err);

    if( private->denyNVRAM)
        return( noErr);

    err = [self writeNVRAMProperty:propertyName value:value length:propSize];
    if( err)
        return( err);

    strcpy( private->nvramPropName, propertyName);		// already checked length
    return( noErr);
}

- (BOOL) isNVRAMProperty:(const char *)propertyName
{
    struct tree_private	*private = _tree_private;

    return( 0 == strcmp( propertyName, private->nvramPropName));
}

- denyNVRAM:(BOOL)flag
{
    struct tree_private	*private = _tree_private;

    private->denyNVRAM = flag;
    return( self);
}

/* ---------------------------------------------------------- */

const char * MatchPaths( const char * matchPath, const char * fullPath)
{
    const char *	ourPath;
    char 		c1, c2;

    ourPath = fullPath;
    do {
        c2 = *ourPath++;
        if( c2 == 0)
            return( matchPath);		// return tail of match path
        if( (c2 >= 'A') && (c2 <= 'Z'))
            c2 += 'a' - 'A';

        c1 = *matchPath++;
        if( (c1 >= 'A') && (c1 <= 'Z'))
            c1 += 'a' - 'A';

        if( c1 != c2) {
            // our path always has unit numbers, match may not
            if( c2 == '@') {
                while( (c2 = *ourPath++)
                    && (c2 != '/') )	{}
                ourPath--;
                matchPath--;

            // our path always has device names, match may not
            } else if( c1 == '@') {
                    while( (c2 = *ourPath++)
                        && (c2 != '@') )	{}

            } else
                break;
	}
    } while( YES );

    return( NULL);			// no match
}

// Remove aliases from the head of the path
// returns difference in length

int
IODealiasPath( char * outputPath, const char * inputPath)
{
    IOReturn		err;
    DTEntry 		dtEntry;
    DTPropertyIterator	dtIter;
    int			nameSize, aliasSize;
    char	*	name;
    char		c;
    int			diff = 0;

    outputPath[ 0 ] = 0;

    if( (inputPath[ 0 ] != '/')
    && (kSuccess == DTLookupEntry(0, "/aliases", &dtEntry) )) {

        err = DTCreatePropertyIterator( dtEntry, &dtIter);
        if( err == kSuccess) {
            while( kSuccess == DTIterateProperties( dtIter, &name)) {

                nameSize = strlen( name);
                c = inputPath[ nameSize ];
                if( c && (c != '/') && (c != ':') && (c != '@') && (c != ','))
                    continue;

                if( 0 == strncmp( inputPath, name, nameSize)) {
                    if( kSuccess != DTGetProperty( dtEntry, name, (void **) &name, &aliasSize ))
                        continue;
                    aliasSize--;				// ditch the zero in property
                    strncpy( outputPath, name, aliasSize);
                    strcpy( outputPath + aliasSize, inputPath + nameSize);
		    diff = nameSize - aliasSize;
                    break;
                }
            }
            DTDisposePropertyIterator( dtIter);
        }
    }
    if( outputPath[ 0 ] == 0)
        strcpy( outputPath, inputPath);

    return( diff);
}


// Necessary for bogus G3 requirement for ide alias
// Only does complete matches,
// eg. /bandit@f2000000/ATY,mach64@e will NOT become pci1/ATY,mach64@e

void
IOAliasPath( char * fullPath)
{
    IOReturn		err;
    DTEntry 		dtEntry;
    DTPropertyIterator	dtIter;
    int			aliasSize;
    char	*	name;
    char	*	alias;
    const char	*	tail;

    if( (fullPath[ 0 ] == '/')
    &&  (kSuccess == DTLookupEntry(0, "/aliases", &dtEntry) )) {

        err = DTCreatePropertyIterator( dtEntry, &dtIter);
        if( err == kSuccess) {
            while( kSuccess == DTIterateProperties( dtIter, &name)) {

                if( kSuccess != DTGetProperty( dtEntry, name, (void **) &alias, &aliasSize ))
                    continue;
		aliasSize--;
		if( aliasSize <= strlen( name))		// no replace if no gain
		    continue;

		if( NULL == (tail = MatchPaths( alias, fullPath)))
		    continue;
		if( *tail)				// exact matches only
		    continue;

		strcpy( fullPath, name);
		break;
            }
            DTDisposePropertyIterator( dtIter);
        }
    }
}

- getDevicePath:(char *)path maxLength:(int)maxLen useAlias:(BOOL)doAlias
{
    id		ok;

    *path = '\0';

    if( [self parent]) {
	ok = [[self parent] getDevicePath:self path:path maxLength:maxLen];

    } else if( maxLen > 1) {
	strcpy( path, "/");
        ok = self;
    } else
	ok = nil;

    if( ok && doAlias)
	IOAliasPath( path);

    return( ok);
}

/*
 * Returns the tail of the matchPath parameter if the head matches the
 * device's path, else returns nil. matchPath can contain aliases.
 */

- (char *) matchDevicePath:(char *)matchPath
{
    char *	pathBuf;
    char *	expandBuf;
    char *	tail = NULL;
    int		diff;
    enum { 	kPathSize = 256 };

    pathBuf = (char *) IOMalloc( kPathSize * 2);
    if( !pathBuf)
	return( NULL);
    expandBuf = pathBuf + kPathSize;

    diff = IODealiasPath( expandBuf, matchPath);

    if( [self getDevicePath:pathBuf maxLength:kPathSize useAlias:NO]) {

        tail = MatchPaths( expandBuf, pathBuf);
        if( tail) {
            diff += tail - expandBuf;
            if( diff > 0)
                tail = matchPath + diff;
            else
                tail = NULL;
        }
    }

    IOFree( pathBuf, kPathSize * 2);
    return( tail);
}

/*
 */

- lookUpProperty:(const char *)propertyName
	value:(unsigned char *)value
	length:(unsigned int *)length
	selector:(SEL)selector
	isString:(BOOL)isString
{
    struct tree_private	*private = _tree_private;
    id			rtn, prtn = nil;
    unsigned int	tempLength = *length;

    if( [self parent])
	prtn = [[[self parent] deviceDescription]
                    lookUpProperty:propertyName value:value length:&tempLength
                        selector:selector isString:isString];

    rtn = [super lookUpProperty:propertyName value:value length:length
                            selector:selector isString:isString];
    if( nil == rtn) {
        if( noErr == [private->propTable getProperty:propertyName flags:0
                    value:(void **)&value length:(UInt32 *)length])
            rtn = self;
	else
	    *length = tempLength;
    }

    return( prtn ? prtn : rtn);
}

- property_IODeviceType:(char *)classes length:(unsigned int *)maxLen
{
    struct tree_private	*private = _tree_private;
    char *		type;
    unsigned int	len;

    [super property_IODeviceType:classes length:maxLen];
    if( noErr == [private->propTable getProperty:"device_type" flags:kReferenceProperty
                value:(void **)&type length:(UInt32 *)&len]) {
        strcat( classes, " ");
        strncat( classes, type, len);
    }
    return( self);
}

- property_IOSlotName:(char *)name length:(unsigned int *)maxLen
{
    if( [[self propertyTable] getProperty:"AAPL,slot-name" flags:0
                value:(void **)&name length:(UInt32 *)maxLen])
	return( nil);

    if( (0 == *maxLen) || (0 == name[0]))
	return( nil);

    return( self);
}

@end

@implementation IORootDevice

- (BOOL) match:(const char *)key location:(const char *)location
{
    return( 0 == strcmp( key, [self nodeName] ));
}

- initAt:(IOPropertyTable *)_propTable ref:_ref
{
    id		rtn;

    rtn = [super initAt:_propTable parent:nil ref:_ref];
    return( rtn);
}


@end
