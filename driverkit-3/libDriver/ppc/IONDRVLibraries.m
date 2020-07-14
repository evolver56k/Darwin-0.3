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



#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#import <machdep/ppc/proc_reg.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>

#import <driverkit/generalFuncs.h>
#import <driverkit/ppc/IOPCIDevice.h>
#import <driverkit/ppc/IODeviceTreeBus.h>
#import "IOPEFLibraries.h"
#import "IOPEFLoader.h"
#import "IONDRVInterface.h"

#import <stdio.h>
#import <string.h>

#define LOG		if(1) kprintf

#define LOGNAMEREG	0

enum {
    kNVRAMProperty        = 0x00000020L,            // matches NR
};

//=========================================================================

#define SWAPLONG(value) (   ((value >> 24) & 0xff) | \
			    ((value >> 8) & 0xff00) | \
			    ((value << 8) & 0xff0000) | \
			    ((value << 24) & 0xff000000) )


OSStatus _eExpMgrConfigReadLong( void * entryID, UInt32 offset, UInt32 * value )
{
    id	ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    return( [ioDevice configReadLong:offset value:value]);
}

OSStatus _eExpMgrConfigWriteLong( void * entryID, UInt32 offset, UInt32 value )
{
    id	ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    return( [ioDevice configWriteLong:offset value:value]);
}


OSStatus _eExpMgrConfigReadWord( void * entryID, UInt32 offset, UInt16 * value )
{
    OSStatus	err;
    UInt32	lvalue;

    err = _eExpMgrConfigReadLong( entryID, offset & (-4), &lvalue);
    if( offset & 2)
	*value = lvalue >> 16;
    else
	*value = lvalue;
    return( err);
}

OSStatus _eExpMgrConfigWriteWord( void * entryID, UInt32 offset, UInt16 value )
{
    OSStatus	err;
    UInt32	lvalue;

    err = _eExpMgrConfigReadLong( entryID, offset & (-4), &lvalue);

    if( offset & 2)
	lvalue = (lvalue & 0xffff) | (value << 16);
    else
	lvalue = (lvalue & 0xffff0000) | value;

    err = _eExpMgrConfigWriteLong( entryID, offset & (-4), lvalue);
    return( err);
}

OSStatus _eExpMgrConfigReadByte( void * entryID, UInt32 offset, UInt8 * value )
{
    OSStatus	err;
    UInt32	lvalue;

    err = _eExpMgrConfigReadLong( entryID, offset & (-4), &lvalue);
    *value = lvalue >> ((offset & 3) * 8);
    return( err);
}

OSStatus _eExpMgrConfigWriteByte( void * entryID, UInt32 offset, UInt8 value )
{
    OSStatus	err;
    UInt32	lvalue;

    err = _eExpMgrConfigReadLong( entryID, offset & (-4), &lvalue);
    lvalue = (lvalue & ~(0xff << ((offset & 3) * 8))) | (value << ((offset & 3) * 8));
    err = _eExpMgrConfigWriteLong( entryID, offset & (-4), lvalue);
    return( err);
}

OSStatus _eExpMgrIOReadLong( void * entryID, UInt32 offset, UInt32 * value )
{
    id		ioDevice;
    UInt32 *	io;
    UInt32	result;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt32 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    __asm__ ("lwbrx %0,%1,%2" : "=r" (result) : "r" (io), "r" (offset) );
    eieio();
    *value = result;

    return( noErr);
}

OSStatus _eExpMgrIOWriteLong( void * entryID, UInt32 offset, UInt32 value )
{
    id		ioDevice;
    UInt32 *	io;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt32 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    __asm__ ("stwbrx %0,%1,%2" : : "r" (value), "r" (io), "r" (offset) );
    eieio();

    return( noErr);
}

OSStatus _eExpMgrIOReadWord( void * entryID, UInt32 offset, UInt16 * value )
{
    id		ioDevice;
    UInt16 *	io;
    UInt16	result;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt16 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    __asm__ ("lhbrx %0,%1,%2" : "=r" (result) : "r" (io), "r" (offset) );
    eieio();
    *value = result;

    return( noErr);
}

OSStatus _eExpMgrIOWriteWord( void * entryID, UInt32 offset, UInt16 value )
{
    id		ioDevice;
    UInt16 *	io;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt16 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    __asm__ ("sthbrx %0,%1,%2" : : "r" (value), "r" (io), "r" (offset) );
    eieio();

    return( noErr);
}

OSStatus _eExpMgrIOReadByte( void * entryID, UInt32 offset, UInt8 * value )
{
    id		ioDevice;
    UInt8 *	io;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt8 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    *value = io[ offset ];
    eieio();
    return( noErr);
}

OSStatus _eExpMgrIOWriteByte( void * entryID, UInt32 offset, UInt8 value )
{
    id		ioDevice;
    UInt8 *	io;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    io = (UInt8 *) [ioDevice getIOAperture];
    if( NULL == io)
	return( IO_R_UNSUPPORTED);

    io[ offset ] = value;
    eieio();

    return( noErr);
}


UInt32 _eEndianSwap32Bit( UInt32 data )
{
    return( SWAPLONG( data ));
}

UInt32 _eEndianSwap16Bit( UInt16 data )
{
    return( 	((data >> 8) & 0x00ff) |
		((data << 8) & 0xff00) );
}

//=========================================================================

OSStatus _eRegistryEntryIDCopy( void *entryID, void *to )
{
    bcopy( entryID, to, 16 );
    return( noErr);
}


OSStatus _eRegistryEntryIDInit( void *entryID )
{
    MAKE_REG_ENTRY( entryID, nil);
    return( noErr);
}


/*
 * Compare EntryID's for equality or if invalid
 *
 * If a NULL value is given for either id1 or id2, the other id 
 * is compared with an invalid ID.  If both are NULL, the id's 
 * are consided equal (result = true). 
 *   note: invalid != uninitialized
 */
Boolean _eRegistryEntryIDCompare( void * entryID1, void * entryID2 )
{
    id	ioDevice1 = nil, ioDevice2 = nil;

    if( entryID1) {
 	REG_ENTRY_TO_ID( entryID1, ioDevice1)
    }
    if( entryID2) {
	REG_ENTRY_TO_ID( entryID2, ioDevice2)
    }
    return( ioDevice1 == ioDevice2 );
}

OSStatus _eRegistryPropertyGetSize( void *entryID, const char *propertyName, UInt32 *propertySize )
{
    OSStatus   err;
    id	ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] getProperty:propertyName flags:kReferenceProperty value:NULL length:propertySize];
#if LOGNAMEREG
    LOG("RegistryPropertyGetSize: %s : %d\n", propertyName, err);
#endif
    return( err);

}

OSStatus _eRegistryPropertyGet(void *entryID, const char *propertyName, UInt32 *propertyValue, UInt32 *propertySize)
{
    OSStatus   err;
    id	ioDevice;
    void * value = propertyValue;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] getProperty:propertyName flags:0 value:&value length:propertySize];
#if LOGNAMEREG
    LOG("RegistryPropertyGet: %s : %d\n", propertyName, err);
#endif
    return( err);
}

OSStatus _eRegistryPropertyCreate( void *entryID, const char *propertyName, void * propertyValue, UInt32 propertySize )
{
    id	ioDevice;
    OSStatus	err;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] createProperty:propertyName flags:0 value:propertyValue length:propertySize];

#if LOGNAMEREG
    LOG("RegistryPropertyCreate: %s : %d\n", propertyName, err);
#endif
    return( err);
}

OSStatus _eRegistryPropertyDelete( void *entryID, const char *propertyName )
{
    id	ioDevice;
    OSStatus	err;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] deleteProperty:propertyName];

#if LOGNAMEREG
    LOG("RegistryPropertyDelete: %s : %d\n", propertyName, err);
#endif
    return( err);
}

OSStatus _eRegistryPropertySet( void *entryID, const char *propertyName, void * propertyValue, UInt32 propertySize )
{
    id	ioDevice;
    OSStatus	err;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] setProperty:propertyName flags:0 value:propertyValue length:propertySize];

    if( (err == noErr) && ([ioDevice isNVRAMProperty:propertyName]))
	err = [ioDevice setNVRAMProperty:propertyName];

#if LOGNAMEREG
    LOG("RegistryPropertySet: %s : %d\n", propertyName, err);
#endif
    return( err);
}

OSStatus _eRegistryPropertyGetMod(void *entryID, const char *propertyName, UInt32 *mod)
{
    id	ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    if( [ioDevice isNVRAMProperty:propertyName])
	*mod = kNVRAMProperty;
    else
	*mod = 0;

    return( noErr);
}

OSStatus _eRegistryPropertySetMod(void *entryID, const char *propertyName, UInt32 mod)
{
    OSStatus	err = noErr;
    id		ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    if( mod & kNVRAMProperty)
	err = [ioDevice setNVRAMProperty:propertyName];

    return( err);
}


struct RegIterator
{
    RegEntryID	regEntry;
    UInt32	index;
};
typedef struct RegIterator RegIterator;

OSStatus _eRegistryPropertyIterateCreate (RegEntryID * entryID, RegIterator ** cookie)
{
    id			ioDevice;
    RegIterator * 	iterator;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    iterator = IOMalloc( sizeof( RegIterator));
    *cookie = iterator;
    if( iterator == NULL)
	return( nrNotEnoughMemoryErr);

    bcopy( entryID, iterator->regEntry, sizeof( RegEntryID));
    iterator->index = 0;

    return( noErr);
}

OSStatus _eRegistryPropertyIterateDispose( RegIterator ** cookie)
{
    IOFree( *cookie, sizeof( RegIterator));
    *cookie = NULL;
    return( noErr);
}


OSStatus _eRegistryPropertyIterate( RegIterator ** cookie, char * name, Boolean * done )
{
    OSStatus		err;
    id			ioDevice;
    RegIterator * 	iterator = *cookie;
    RegEntryID * 	entryID = &iterator->regEntry;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    err = [[ioDevice propertyTable] getPropertyWithIndex:(iterator->index++) name:name];

    // Seems to be differences in handling "done". ATI assumes done = true when getting the last
    // property. Book says done is true after last property. ATI does check err, so this will work.
    // Control ignores err and checks done.

    *done = (err != noErr);

    return( err);
}

typedef UInt32 RegEntryIter;

OSStatus
_eRegistryEntryIterateCreate( RegEntryIter * cookie)
{
    *cookie = 0;
    return( noErr);
}

OSStatus
_eRegistryEntryIterateDispose( RegEntryIter * cookie)
{
    return( noErr);
}

OSStatus
_eRegistryEntryIterate( RegEntryIter *	cookie,
			UInt32		relationship,
			RegEntryID *	foundEntry,
			Boolean *	done)
{
    id			ioDevice;

    ioDevice = [IOTreeDevice findForIndex:(*cookie)++];

    MAKE_REG_ENTRY( foundEntry, ioDevice);
    *done = (ioDevice == nil);

    if( ioDevice)
	return( noErr);
    else
	return( nrNotFoundErr);
}

OSStatus
_eRegistryCStrEntryToName( const RegEntryID *	entryID,
			RegEntryID *		parentEntry,
			char *			nameComponent,
			Boolean *		done)
{
    id			ioDevice;

    REG_ENTRY_TO_ID( entryID, ioDevice)

    strncpy( nameComponent, [ioDevice nodeName], 31 );
    nameComponent[ 31 ] = 0;
    return( noErr);
}

OSStatus
_eRegistryCStrEntryCreate(  const RegEntryID *	parentEntry,
			    const char 	*	name,
			    RegEntryID	*	newEntry)
{
    IOPropertyTable  *	propTable;
    IOTreeDevice  *	newDev;

    // NOT published

    propTable = [[IOPropertyTable alloc] init];

    [propTable createProperty:"name" flags:0
	value:name length:(strlen( name) + 1)];

    newDev = [[IOTreeDevice alloc] initAt:propTable parent:nil ref:nil];

    MAKE_REG_ENTRY( newEntry, newDev);

    return( noErr);
}

//=========================================================================

// in NDRVLibrariesAsm.s
extern void _eCompareAndSwap( void );
extern void _eSynchronizeIO( void );

// platform expert
extern vm_offset_t
PEResidentAddress( vm_offset_t address, vm_size_t length );

enum {
    kProcessorCacheModeDefault		= 0,
    kProcessorCacheModeInhibited 	= 1,
    kProcessorCacheModeWriteThrough 	= 2,
    kProcessorCacheModeCopyBack		= 3
};

OSStatus _eSetProcessorCacheMode( UInt32 space, void * addr, UInt32 len, UInt32 mode )
{
    struct phys_entry*	pp;
    vm_offset_t 	spa;
    vm_offset_t 	epa;
    int			wimg;

    // This doesn't change any existing kernel mapping eg. BAT changes etc.
    // but this is enough to change user level mappings for DPS etc.
    // Should use a kernel service when one is available.

    spa = kvtophys( (vm_offset_t)addr);
    if( spa == 0) {
	spa = PEResidentAddress( (vm_offset_t)addr, len);
	if( spa == 0)
	    return( IO_R_VM_FAILURE);
    }
    epa = (len + spa + 0xfff) & 0xfffff000;
    spa &=  0xfffff000;

    switch( mode) {
	case kProcessorCacheModeWriteThrough:
	    wimg = PTE_WIMG_WT_CACHED_COHERENT_GUARDED;
	    break;
	case kProcessorCacheModeCopyBack:
	    wimg = PTE_WIMG_CB_CACHED_COHERENT_GUARDED;
	    break;
	default:
	    wimg = PTE_WIMG_UNCACHED_COHERENT_GUARDED;
	    break;
    }

    while( spa < epa) {
	pp = pmap_find_physentry(spa);
	if (pp != PHYS_NULL)
	    pp->pte1.bits.wimg = wimg;
	spa += PAGE_SIZE;
    }
    _eSynchronizeIO();
    return( noErr);
}

char * _ePStrCopy( char *to, const char *from )
{
    UInt32	len;
    char   *	copy;

    copy = to;
    len = *(from++);
    *(copy++) = len;
    bcopy( from, copy, len);
    return( to);
}

LogicalAddress _ePoolAllocateResident(ByteCount byteSize, Boolean clear)
{
    LogicalAddress  mem;

    mem = IOMalloc( byteSize );
    if( clear && mem)
        memset( mem, 0, byteSize);

    return( mem);
}

OSStatus _ePoolDeallocate( void )
{
    LOG("_ePoolDeallocate\n");
    return( noErr);
}

UInt32	_eCurrentExecutionLevel(void)
{
	return(0);	// kTaskLevel, HWInt = 6
}

// don't expect any callers of this
OSErr _eIOCommandIsComplete( UInt32 commandID, OSErr result )
{
    LOG("_eIOCommandIsComplete\n");
    return( result);
}

//=========================================================================

#include <mach/clock_types.h>
#include <kern/clock.h>


tvalspec_t _eUpTime( void )
{
    return( clock_get_counter( System));
}

tvalspec_t _eAddAbsoluteToAbsolute(tvalspec_t left, tvalspec_t right)
{
    tvalspec_t    result = left;

    ADD_TVALSPEC( &left, &right)
    return( result);
}


tvalspec_t _eSubAbsoluteFromAbsolute(tvalspec_t left, tvalspec_t right)
{
    tvalspec_t    result = left;

    // !! ATI bug fix here:
    // They expect the 64-bit result to be signed. The spec says < 0 => 0
    // To workaround, make sure this routine takes 10 us to execute.
    IODelay( 10);

    SUB_TVALSPEC( &result, &right)
    if( ((int)result.tv_sec) < 0) {
	result.tv_sec = 0;
	result.tv_nsec = 0;
    }
    return( result);
}


tvalspec_t    _eDurationToAbsolute( Duration theDuration)
{
tvalspec_t    tval;

    if( theDuration > 0) {
	tval.tv_sec = theDuration / 1000;
        tval.tv_nsec = (theDuration % 1000) * 1000 * 1000;

    } else {
	tval.tv_sec = (-theDuration) / 1000000;
        tval.tv_nsec = (-theDuration % 1000000) * 1000;
    }
    return( tval);
}

tvalspec_t _eAddDurationToAbsolute( Duration duration, tvalspec_t absolute )
{
    return( _eAddAbsoluteToAbsolute(_eDurationToAbsolute( duration), absolute));
}

tvalspec_t    _eNanosecondsToAbsolute ( UnsignedWide theNanoseconds)
{
tvalspec_t    tval;

    if( theNanoseconds.hi == 0) {
	tval.tv_sec = theNanoseconds.lo / NSEC_PER_SEC;
	tval.tv_nsec = theNanoseconds.lo % NSEC_PER_SEC;
    } else {
	// overflows?
	tval.tv_sec = 4 * theNanoseconds.hi;
	tval.tv_sec += ((294967296 * theNanoseconds.hi) + theNanoseconds.lo) / NSEC_PER_SEC;
	tval.tv_nsec = ((294967296 * theNanoseconds.hi) + theNanoseconds.lo) % NSEC_PER_SEC;
    }
    return( tval);
}

UnsignedWide    _eAbsoluteToNanoseconds( tvalspec_t absolute )
{
UnsignedWide    nano;

    if( absolute.tv_sec == 0) {
	nano.hi = 0;
	nano.lo = absolute.tv_nsec;
    } else {
	nano.lo = absolute.tv_nsec + (absolute.tv_sec * NSEC_PER_SEC);
	nano.hi = (absolute.tv_sec / 4);
    }
    return( nano);
}

Duration    _eAbsoluteDeltaToDuration( tvalspec_t left, tvalspec_t right )
{
    Duration	dur;
    tvalspec_t  result;

    if( CMP_TVALSPEC( &left, &right) < 0)
	return( 0);

    result = left;
    SUB_TVALSPEC( &result, &right)

    if( result.tv_sec >= 2147) {
	if( result.tv_sec >= 2147483)
	    return( 0x7fffffff);
	dur = result.tv_sec * 1000 + result.tv_nsec / 1000000;		// ms
    } else {
	dur = -(result.tv_sec * 1000000 + result.tv_nsec / 1000);	// us
    }
    return( dur);
}


OSStatus    _eDelayForHardware( tvalspec_t time )
{
    tvalspec_t	deadline, now;

    deadline = time;
    now = _eUpTime();
    ADD_TVALSPEC( &deadline, &now);

    while( CMP_TVALSPEC( &deadline, &now) > 0) {
	now = _eUpTime();
    }

    return( noErr);
}

OSStatus    _eDelayFor( Duration theDuration )
{
#if 1

// In Marconi, DelayFor uses the old toolbox Delay routine
// which is based on the 60 Hz timer. Durations are not
// rounded up when converting to ticks. Yes, really.
// There is some 64-bit math there so we'd better reproduce
// the overhead of that calculation.

#define DELAY_FOR_TICK_NANO		16666666
#define DELAY_FOR_TICK_MILLI		17
#define NANO32_MILLI			4295

    UnsignedWide	nano;
    tvalspec_t		abs;
    unsigned int	ms;

    abs = _eDurationToAbsolute( theDuration);
    nano = _eAbsoluteToNanoseconds( abs);

    ms = (nano.lo / DELAY_FOR_TICK_NANO) * DELAY_FOR_TICK_MILLI;
    ms += nano.hi * NANO32_MILLI;
    if( ms)
        IOSleep( ms);

#else
    // Accurate, but incompatible, version

#define SLEEP_THRESHOLD		5000

    if( theDuration < 0) {

	// us duration
	theDuration -= theDuration;
	if( theDuration > SLEEP_THRESHOLD)
	    IOSleep( (theDuration + 999) / 1000);
	else
	    IODelay( theDuration);

    } else {

	// ms duration
        if( theDuration > (SLEEP_THRESHOLD / 1000))
            IOSleep( theDuration );                     	// ms
	else
            IODelay( theDuration * 1000);			// us
    }
#endif

    return( noErr);
}


//=========================================================================

SInt32	StdIntHandler( InterruptSetMember setMember, void *refCon, UInt32 theIntCount)
{
    return( kIsrIsComplete);
}
void    StdIntEnabler( InterruptSetMember setMember, void *refCon)
{
    return;
}
Boolean StdIntDisabler( InterruptSetMember setMember, void *refCon)
{
    return( false);
}

static TVector _eStdIntHandler 	= { StdIntHandler,  0 };
static TVector _eStdIntEnabler 	= { StdIntEnabler,  0 };
static TVector _eStdIntDisabler = { StdIntDisabler, 0 };

OSStatus
_eGetInterruptFunctions(    id		 	     setID,
			    UInt32		     member,
			    void *		*    refCon,
			    TVector	 	**   handler,
			    TVector 		**   enabler,
			    TVector		**   disabler )
{
    OSStatus	err = noErr;

    if( handler)
	*handler = &_eStdIntHandler;
    if( enabler)
	*enabler = &_eStdIntEnabler;
    if( disabler)
	*disabler = &_eStdIntDisabler;

    return( err);
}

OSStatus
_eInstallInterruptFunctions(id		 	     setID,
			    UInt32		     member,
			    void *		     refCon,
			    TVector	 	*    handler,
			    TVector 		*    enabler,
			    TVector		*    disabler )
{
    OSStatus	err = noErr;

    return( err);
}

//=========================================================================

extern void PowerSurgeSendIIC( unsigned char iicAddr, unsigned char iicReg, unsigned char iicData);

OSStatus _eCallOSTrapUniversalProc( UInt32 theProc, UInt32 procInfo, UInt32 trap, UInt8 * pb )
{
OSStatus    err = -40;

    if( (procInfo == 0x133822)
    &&  (trap == 0xa092) ) {

	UInt8 addr, reg, data;

	addr = pb[ 2 ];
	reg = pb[ 3 ];
	pb = *( (UInt8 **) ((UInt32) pb + 8));
	data = pb[ 1 ];
	PowerSurgeSendIIC( addr, reg, data );
	err = 0;
    }
    return( err);
}

const UInt32 * _eGetKeys( void )
{
    static const UInt32 zeros[] = { 0, 0, 0, 0 };

    return( zeros);
}

UInt32 _eGetIndADB( void * adbInfo, UInt32 index )
{
    bzero( adbInfo, 10);
    return( 0);		// orig address
}

//=========================================================================

OSStatus _eNoErr( void )
{
    return( noErr);
}

OSStatus _eFail( void )
{
    return( -40);
}

//=========================================================================

// fix this!

#define 	heathrowID		((volatile UInt32 *)0xf3000034)
#define 	heathrowTermEna		(1 << 3)
#define 	heathrowTermDir		(1 << 0)

#define 	heathrowFeatureControl	((volatile UInt32 *)0xf3000038)
#define 	heathrowMBRES		(1 << 24)

#define 	heathrowBrightnessControl ((volatile UInt8 *)0xf3000032)
#define		defaultBrightness	144
#define 	heathrowContrastControl ((volatile UInt8 *)0xf3000033)
#define		defaultContrast		183

#define 	gossamerSystemReg1	((volatile UInt16 *)0xff000004)
#define		gossamerAllInOne	(1 << 4)

void _eATISetMBRES( UInt32 state )
{
    UInt32	value;

    value = *heathrowFeatureControl;

    if( state == 0)
	value &= ~heathrowMBRES;
    else if( state == 1)
	value |= heathrowMBRES;

    *heathrowFeatureControl = value;
    eieio();
}

void _eATISetMonitorTermination( Boolean enable )
{

    UInt32	value;

    value = *heathrowID;

    value |= heathrowTermEna;
    if( enable)
	value |= heathrowTermDir;
    else
	value &= ~heathrowTermDir;

    *heathrowID = value;
    eieio();
}

Boolean _eATIIsAllInOne( void )
{
    Boolean	rtn;

    rtn = (0 == ((*gossamerSystemReg1) & gossamerAllInOne));
    if( rtn) {
	*heathrowBrightnessControl = defaultBrightness;
        eieio();
	*heathrowContrastControl = defaultContrast;
        eieio();
    }
    return( rtn);
}

//=========================================================================


static FunctionEntry PCILibFuncs[] =
{
    { "ExpMgrConfigReadLong", _eExpMgrConfigReadLong },
    { "ExpMgrConfigReadWord", _eExpMgrConfigReadWord },
    { "ExpMgrConfigReadByte", _eExpMgrConfigReadByte },
    { "ExpMgrConfigWriteLong", _eExpMgrConfigWriteLong },
    { "ExpMgrConfigWriteWord", _eExpMgrConfigWriteWord },
    { "ExpMgrConfigWriteByte", _eExpMgrConfigWriteByte },

    { "ExpMgrIOReadLong", _eExpMgrIOReadLong },
    { "ExpMgrIOReadWord", _eExpMgrIOReadWord },
    { "ExpMgrIOReadByte", _eExpMgrIOReadByte },
    { "ExpMgrIOWriteLong", _eExpMgrIOWriteLong },
    { "ExpMgrIOWriteWord", _eExpMgrIOWriteWord },
    { "ExpMgrIOWriteByte", _eExpMgrIOWriteByte },

    { "EndianSwap16Bit", _eEndianSwap16Bit },
    { "EndianSwap32Bit", _eEndianSwap32Bit }
};

static FunctionEntry VideoServicesLibFuncs[] =
{
    { "VSLPrepareCursorForHardwareCursor", _eFail },
    { "VSLNewInterruptService", _eNoErr },
    { "VSLDisposeInterruptService", _eNoErr },
    { "VSLDoInterruptService", _eNoErr }
};

static FunctionEntry NameRegistryLibFuncs[] =
{
    { "RegistryEntryIDCopy", _eRegistryEntryIDCopy },
    { "RegistryEntryIDInit", _eRegistryEntryIDInit },
    { "RegistryEntryIDDispose", _eNoErr },
    { "RegistryEntryIDCompare", _eRegistryEntryIDCompare },
    { "RegistryPropertyGetSize", _eRegistryPropertyGetSize },
    { "RegistryPropertyGet", _eRegistryPropertyGet },
    { "RegistryPropertyGetMod", _eRegistryPropertyGetMod },
    { "RegistryPropertySetMod", _eRegistryPropertySetMod },

    { "RegistryPropertyIterateCreate", _eRegistryPropertyIterateCreate },
    { "RegistryPropertyIterateDispose", _eRegistryPropertyIterateDispose },
    { "RegistryPropertyIterate", _eRegistryPropertyIterate },

    { "RegistryEntryIterateCreate", _eRegistryEntryIterateCreate },
    { "RegistryEntryIterateDispose", _eRegistryEntryIterateDispose },
    { "RegistryEntryIterate", _eRegistryEntryIterate },
    { "RegistryCStrEntryToName", _eRegistryCStrEntryToName },

    { "RegistryCStrEntryCreate", _eRegistryCStrEntryCreate },
    { "RegistryEntryDelete", _eNoErr },

    { "RegistryPropertyCreate", _eRegistryPropertyCreate },
    { "RegistryPropertyDelete", _eRegistryPropertyDelete },
    { "RegistryPropertySet", _eRegistryPropertySet }
};


static FunctionEntry DriverServicesLibFuncs[] =
{
    { "SynchronizeIO", _eSynchronizeIO },
    { "SetProcessorCacheMode", _eSetProcessorCacheMode },
    { "BlockCopy", bcopy },
    { "BlockMove", bcopy },
    { "CStrCopy", strcpy },
    { "CStrCmp", strcmp },
    { "CStrLen", strlen },
    { "CStrCat", strcat },
    { "CStrNCopy", strncpy },
    { "CStrNCmp", strncmp },
    { "CStrNCat", strncat },
    { "PStrCopy", _ePStrCopy },

    { "PoolAllocateResident", _ePoolAllocateResident },
    { "MemAllocatePhysicallyContiguous", _ePoolAllocateResident },
    { "PoolDeallocate", _ePoolDeallocate },

    { "UpTime", _eUpTime },
    { "AbsoluteDeltaToDuration", _eAbsoluteDeltaToDuration },
    { "AddAbsoluteToAbsolute", _eAddAbsoluteToAbsolute },
    { "SubAbsoluteFromAbsolute", _eSubAbsoluteFromAbsolute },
    { "AddDurationToAbsolute", _eAddDurationToAbsolute },
    { "NanosecondsToAbsolute", _eNanosecondsToAbsolute },
    { "AbsoluteToNanoseconds", _eAbsoluteToNanoseconds },
    { "DurationToAbsolute", _eDurationToAbsolute },
    { "DelayForHardware", _eDelayForHardware },
    { "DelayFor", _eDelayFor },

    { "CurrentExecutionLevel", _eCurrentExecutionLevel },
    { "IOCommandIsComplete", _eIOCommandIsComplete },

    { "SysDebugStr", _eNoErr },
    { "SysDebug", _eNoErr },

    { "CompareAndSwap", _eCompareAndSwap },

    { "CreateInterruptSet", _eNoErr },
    { "DeleteInterruptSet", _eNoErr },
    { "GetInterruptFunctions", _eGetInterruptFunctions },
    { "InstallInterruptFunctions", _eNoErr }

};

static FunctionEntry ATIUtilsFuncs[] =
{
    // Gossamer onboard ATI
    { "ATISetMBRES", _eATISetMBRES },
    { "ATISetMonitorTermination", _eATISetMonitorTermination },
    { "ATIIsAllInOne", _eATIIsAllInOne }
};


// These are all out of spec

static FunctionEntry InterfaceLibFuncs[] =
{
    // Apple drivers : XPRam and EgretDispatch
    { "CallUniversalProc", _eFail },
    { "CallOSTrapUniversalProc", _eCallOSTrapUniversalProc },

    // Radius PrecisionColor 16

    { "CountADBs", _eNoErr },
    { "GetIndADB", _eGetIndADB },
    { "GetKeys", _eGetKeys }
};


#define NUMLIBRARIES	6
const ItemCount numLibraries = NUMLIBRARIES;
LibraryEntry Libraries[ NUMLIBRARIES ] =
{
    { "PCILib", sizeof(PCILibFuncs) / sizeof(FunctionEntry), PCILibFuncs },
    { "VideoServicesLib", sizeof(VideoServicesLibFuncs) / sizeof(FunctionEntry), VideoServicesLibFuncs },
    { "NameRegistryLib", sizeof(NameRegistryLibFuncs) / sizeof(FunctionEntry), NameRegistryLibFuncs },
    { "DriverServicesLib", sizeof(DriverServicesLibFuncs) / sizeof(FunctionEntry), DriverServicesLibFuncs },

    // G3
    { "ATIUtils", sizeof(ATIUtilsFuncs) / sizeof(FunctionEntry), ATIUtilsFuncs },

    // out of spec stuff
    { "InterfaceLib", sizeof(InterfaceLibFuncs) / sizeof(FunctionEntry), InterfaceLibFuncs }
};

