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



#import  <driverkit/ppc/IOTreeDevice.h>
#import "IOPEFLoader.h"
#import "IONDRVInterface.h"

#define LOG	if(1) kprintf

struct NDRVInstanceVars {
    PCodeInstance 	pcInst;
};
typedef struct NDRVInstanceVars NDRVInstanceVars;


OSStatus    NDRVLoad( LogicalAddress container, ByteCount containerSize, NDRVInstance * instance )
{
    NDRVInstanceVars *	ndrvInst;
    OSStatus            err;

    ndrvInst = (NDRVInstanceVars *) IOMalloc( sizeof( NDRVInstanceVars));

    err = PCodeOpen( container, containerSize, &ndrvInst->pcInst );

    if( err == noErr)
	err = PCodeInstantiate( ndrvInst->pcInst );

    *instance = ndrvInst;

    return( err);
}

OSStatus    NDRVGetSymbol( NDRVInstance instance, const char * symbolName, LogicalAddress * address )
{
    NDRVInstanceVars  * ndrvInst = (NDRVInstanceVars *) instance;
    OSStatus            err;

    err = PCodeFindExport( ndrvInst->pcInst, symbolName, address, NULL );
    return( err);
}

OSStatus    NDRVGetShimClass( id ioDevice, NDRVInstance instance, UInt32 serviceIndex, char * className )
{
    NDRVInstanceVars  * 	ndrvInst = (NDRVInstanceVars *) instance;
    OSStatus            	err;
    static const char *		driverDescProperty = "TheDriverDescription";
    static const char *		frameBufferShim = "IONDRVFramebuffer";
    DriverDescription * 	desc;
    UInt32			serviceType;

    className[ 0 ] = 0;
    do {
	err = PCodeFindExport( ndrvInst->pcInst, driverDescProperty, (LogicalAddress *)&desc, NULL );
        if( err) continue;

	if( desc->driverDescSignature != kTheDescriptionSignature) {
	    err = -1;
	    continue;
	}
	if( serviceIndex >= desc->driverServices.nServices) {
	    err = -1;
	    continue;
	}

	serviceType = desc->driverServices.service[ serviceIndex ].serviceType;
	switch( desc->driverServices.service[ serviceIndex ].serviceCategory) {

	    case kServiceCategoryNdrvDriver:
		if( serviceType == kNdrvTypeIsVideo) {
                    strcpy( className, frameBufferShim);
		    break;
		}
	    default:
		err = -1;
	}
    } while( false);

    return( err);
}

OSStatus    NDRVUnload( NDRVInstance * instance )
{
    return( noErr);
}


OSStatus    NDRVDoDriverIO( LogicalAddress entry,
		UInt32 commandID, void * contents, UInt32 commandCode, UInt32 commandKind )
{
    OSStatus            	err;

#if 1

    fpu_save_thread();
    err = CallTVector( /*AddressSpaceID*/ 0, (void *)commandID, contents,
		(void *)commandCode, (void *)commandKind, /*p6*/ 0, entry );
    fpu_disable_thread();

#else

static	LogicalAddress	stack = NULL;		// FIXME

    if( stack == NULL)
	stack = (LogicalAddress) ( IOMalloc( 32768) + 32768 );

    fpu_save();
    err = CallTVectorWithStack( /*AddressSpaceID*/ 0, (void *)commandID, contents,
		(void *)commandCode, (void *)commandKind, /*p6*/ 0, entry, stack);
    fpu_disable();

#endif

#if 0
    if( err) {
	UInt32 i;
	static const char * commands[] = 
		{ "kOpenCommand", "kCloseCommand", "kReadCommand", "kWriteCommand",
		"kControlCommand", "kStatusCommand", "kKillIOCommand", "kInitializeCommand",
		"kFinalizeCommand", "kReplaceCommand", "kSupersededCommand" };

	LOG("Driver failed (%d) on %s : ", err, commands[ commandCode ] );

	switch( commandCode) {
	    case kControlCommand:
	    case kStatusCommand:
		LOG("%d : ", ((UInt16 *)contents)[ 0x1a / 2 ]);
		contents = ((void **)contents)[ 0x1c / 4 ];
		for( i = 0; i<5; i++ )
		    LOG("%08x, ", ((UInt32 *)contents)[i] );
		break;
	}
	LOG("\n");
    }
#endif

    return( err);
}

// autoconf_ppc comes here for matching
BOOL
NDRVForDevice( IOTreeDevice * ioDevice )
{
    OSStatus		err;
    NDRVInstance	instance;
    LogicalAddress	pef;
    IOByteCount		propSize;
    static const char * pefPropertyName = "driver,AAPL,MacOS,PowerPC";
    IOPropertyTable   *	propTable = [ioDevice propertyTable];
    char		classNames[ 80 ];


    if( NO == [ioDevice isKindOf:[IOTreeDevice class]] )
        return( NO);

#if 0
    // to test code aligning
    err = [propTable getProperty:pefPropertyName flags:kReferenceProperty
                value:nil length:&propSize];
    if( err == noErr) {
	pef = IOMalloc( propSize);
	err = [propTable getProperty:pefPropertyName flags:0
		    value:&pef length:&propSize];
    }
#else
    err = [propTable getProperty:pefPropertyName flags:kReferenceProperty
                value:&pef length:&propSize];
#endif

    // God awful hack:
    // Some onboard devices don't have the ndrv in the tree. The new booter
    // can load & match PEF's but only from disk, not network boots.

    if( err && (strcmp( [ioDevice nodeName], "ATY,mach64_3DU") == 0) ) {

        unsigned int * patch;

        patch = (unsigned int *) 0xffe88140;
        propSize = 0x10a80;

	// Check ati PEF exists there
        if( patch[ 0x1f0 / 4 ] == 'ATIU') {

            pef = (LogicalAddress) IOMalloc( propSize );
            bcopy( (void *) patch, pef, propSize );
            err = noErr;
        }
    }

    if( err && (strcmp( [ioDevice nodeName], "ATY,mach64_3DUPro") == 0) ) {

        unsigned int * patch;

        patch = (unsigned int *) 0xffe99510;
        propSize = 0x12008;

	// Check ati PEF exists there
        if( patch[ 0x1fc / 4 ] == 'ATIU') {

            pef = (LogicalAddress) IOMalloc( propSize );
            bcopy( (void *) patch, pef, propSize );
            err = noErr;
        }
    }

    if( err && (strcmp( [ioDevice nodeName], "control") == 0) ) {

#define ins(i,d,a,simm) ((i<<26)+(d<<21)+(a<<16)+simm)
        unsigned int * patch;

        patch = (unsigned int *) 0xffe6bd50;
        propSize = 0xac10;

	// Check control PEF exists there
        if( patch[ 0x41ac / 4 ] == ins( 32, 3, 0, 0x544)) {	// lwz r3,0x544(0)

            pef = (LogicalAddress) IOMalloc( propSize );
            bcopy( (void *) patch, pef, propSize );
            patch = (unsigned int *) pef;
	    // patch out low mem accesses
            patch[ 0x8680 / 4 ] = ins( 14, 12, 0, 0);		// addi r12,0,0x0
            patch[ 0x41ac / 4 ] = ins( 14, 3, 0, 0x544);	// addi r3,0,0x544;
            patch[ 0x8fa0 / 4 ] = ins( 14, 3, 0, 0x648);	// addi r3,0,0x648;
            err = noErr;
        }
    }

    if( err == noErr) {
        do {
            if(	(err = NDRVLoad( pef, propSize, &instance ))
            ) continue;
            [propTable createProperty:"AAPL,ndrvInst" flags:0
                value:&instance length:sizeof( NDRVInstance)];
            if(	(err = NDRVGetShimClass( ioDevice, instance, 0, classNames ))
            ) continue;

            err = [propTable createProperty:"AAPL,dk_Driver Name" flags:0
                        value:classNames length:strlen( classNames) ];
            err = [propTable createProperty:"AAPL,dk_Server Name" flags:0
                        value:classNames length:strlen( classNames) ];

            return( YES);

        } while( false);
    }

    return( NO);
}


