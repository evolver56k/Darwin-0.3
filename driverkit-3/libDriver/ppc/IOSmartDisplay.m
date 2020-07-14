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



#import  <driverkit/ppc/IOMacOSTypes.h>
#import "IOMacOSVideo.h"
#import	<driverkit/ppc/IOSmartDisplay.h>
#import <mach/mach_types.h>
#import <mach/message.h>
#import <bsd/dev/ppc/busses.h>
#import <bsd/dev/ppc/adb.h>

@interface IOSmartDisplay:Object <IOSmartDisplayExported>
{
    // used to query the framebuffer controller
    id			attachedFramebuffer;
    UInt32		attachedRefCon;

@private
    void *		priv;
    
    /* Reserved for future expansion. */
    int _IOSmartDisplay_reserved[2];
}

@end

struct AVDeviceInfo
{
    UInt8		wiggleLADAddr;
    UInt32		numTimings;
    const UInt32    *   timings;
};
typedef struct AVDeviceInfo AVDeviceInfo;

@interface IOSmartADBDisplay:IOSmartDisplay
{
@private
    UInt8		adbAddr;
    UInt8		waitAckValue;
    SInt16		avDisplayID;
    const AVDeviceInfo * deviceInfo;
}

+ probeADBForDisplays;		// strictly temporary!

@end

struct EDID {
    UInt8	header[8];
    UInt8	vendorProduct[10];
    UInt8	version;
    UInt8	revision;
    UInt8	displayParams[5];
    UInt8	colorCharacteristics[10];
    UInt8	establishedTimings[3];
    UInt16	standardTimings[8];
    UInt8	detailedTimings[72];
    UInt8	extension;
    UInt8	checksum;
};
typedef struct EDID EDID;

@interface IOSmartDDCDisplay:IOSmartDisplay
{
@private
    EDID		edid1;

}

@end


static UInt32	smInited = 0;			// why does +initialize get called twice?
static IOSmartADBDisplay * ADB2SmartDisplay[ ADB_DEVICE_COUNT ];

@implementation IOSmartDisplay

+ findForConnection:framebuffer refCon:(UInt32)refCon
{
    IOSmartADBDisplay *	try;
    IOSmartDisplay *	found = nil;
    UInt32		i;

    if( !smInited) {
        smInited = YES;
        [IOSmartADBDisplay probeADBForDisplays];
    }

    if( [framebuffer conformsTo:@protocol( IOFBAppleSense)])
    {
        for (i = 0; (found == nil) && (i < ADB_DEVICE_COUNT); i++ ) {
            try = ADB2SmartDisplay[ i ];
            found = [try attach:framebuffer refCon:refCon];
        }
    }

    if( (found == nil) && [framebuffer conformsTo:@protocol( IOFBHighLevelDDCSense)])
    {
	found = [[[IOSmartDDCDisplay alloc] init] attach:framebuffer refCon:refCon];
    }

    if( found == nil)
	found = [[[IOSmartDisplay alloc] init] attach:framebuffer refCon:refCon];

    return( found);
}

- attach:framebuffer refCon:(UInt32)refCon;
{
    attachedFramebuffer = framebuffer;
    attachedRefCon = refCon;
    return( self);
}

- detach
{
    attachedFramebuffer = nil;
    return( self);
}

- (BOOL) attached
{
    return( attachedFramebuffer != nil);
}

- (IOReturn) getDisplayInfoForMode:(IOFBTimingInformation *)mode flags:(UInt32 *)flags
{
    // Pass the existing flags (from framebuffer) thru
    return( noErr);
}

- (IOReturn)
    getGammaTableByIndex:
	(UInt32 *)channelCount dataCount:(UInt32 *)dataCount
    	dataWidth:(UInt32 *)dataWidth data:(void **)data
{
    return( IO_R_UNSUPPORTED);
}
@end

@implementation IOSmartDDCDisplay

- attach:framebuffer refCon:(UInt32)refCon
{
    IOReturn		err;
    IOByteCount		length;

    do {
	if( NO == [framebuffer hasDDCConnect:refCon])
	    continue;

	length = sizeof( EDID);
	err = [framebuffer getDDCBlock:refCon blockNumber:1 blockType:kIOFBDDCBlockTypeEDID
		options:0 data:(UInt8 *)&edid1 length:&length];
	if( err || (length != sizeof( EDID)))
	    continue;

	kprintf("%s EDID Version %d, Revision %d\n", [framebuffer name],
	    edid1.version, edid1.revision );
	if( edid1.version != 1)
	    continue;
#if 1
    {
	int i;

	kprintf("Est: ");
	for( i=0; i<3; i++)
	    kprintf(" 0x%02x,", edid1.establishedTimings[i] );
	kprintf("\nStd: " );
	for( i=0; i<8; i++)
	    kprintf(" 0x%04x,", edid1.standardTimings[i] );
	kprintf("\n");
    }
#endif

	kprintf("IOSmartDDCDisplay attach on %s\n", [framebuffer name]);
	return( [super attach:framebuffer refCon:refCon]);

    } while( false);

    return([self free]);
}


struct TimingToEDID {
    UInt32	timingID;
    UInt16	standardTiming;
    UInt8	establishedBit;
    UInt8	spare;
};
typedef struct TimingToEDID TimingToEDID;

#define MAKESTD(h,a,r)		( (((h/8)-31)<<8) | (a<<6) | (r-60) )

static const TimingToEDID timingToEDID[] = {
    { timingApple_512x384_60hz,		MAKESTD(  512,1,60), 0xff, 0 },
    { timingApple_640x480_67hz,		MAKESTD(  640,1,67), 0x04, 0 },
    { timingVESA_640x480_60hz,		MAKESTD(  640,1,60), 0x05, 0 },
    { timingVESA_640x480_72hz ,		MAKESTD(  640,1,72), 0x03, 0 },
    { timingVESA_640x480_75hz,		MAKESTD(  640,1,75), 0x02, 0 },
    { timingVESA_640x480_85hz,		MAKESTD(  640,1,85), 0xff, 0 },
    { timingApple_832x624_75hz,		MAKESTD(  832,1,75), 0x0d, 0 },
    { timingVESA_800x600_56hz,		MAKESTD(  800,1,56), 0x01, 0 },
    { timingVESA_800x600_60hz,		MAKESTD(  800,1,60), 0x00, 0 },
    { timingVESA_800x600_72hz,		MAKESTD(  800,1,72), 0x0f, 0 },
    { timingVESA_800x600_75hz,		MAKESTD(  800,1,75), 0x0e, 0 },
    { timingVESA_800x600_85hz,		MAKESTD(  800,1,85), 0xff, 0 },
    { timingVESA_1024x768_60hz,		MAKESTD( 1024,1,60), 0x0b, 0 },
    { timingVESA_1024x768_70hz,		MAKESTD( 1024,1,70), 0x0a, 0 },
    { timingVESA_1024x768_75hz,		MAKESTD( 1024,1,75), 0x09, 0 },
    { timingVESA_1024x768_85hz,		MAKESTD( 1024,1,85), 0xff, 0 },
    { timingApple_1024x768_75hz,	MAKESTD( 1024,1,75), 0x09, 0 },
    { timingApple_1152x870_75hz,	MAKESTD( 0000,0,00), 0x17, 0 },
    { timingVESA_1280x960_75hz,		MAKESTD( 1280,1,75), 0xff, 0 },
    { timingVESA_1280x1024_60hz,	MAKESTD( 1280,2,60), 0xff, 0 },
    { timingVESA_1280x1024_75hz,	MAKESTD( 1280,2,75), 0x08, 0 },
    { timingVESA_1280x1024_85hz,	MAKESTD( 1280,2,85), 0xff, 0 },
    { timingVESA_1600x1200_60hz,	MAKESTD( 1600,1,60), 0xff, 0 },
    { timingVESA_1600x1200_65hz,	MAKESTD( 1600,1,65), 0xff, 0 },
    { timingVESA_1600x1200_70hz,	MAKESTD( 1600,1,70), 0xff, 0 },
    { timingVESA_1600x1200_75hz,	MAKESTD( 1600,1,75), 0xff, 0 },
    { timingVESA_1600x1200_80hz,	MAKESTD( 1600,1,80), 0xff, 0 }
};

- (IOReturn) getDisplayInfoForMode:(IOFBTimingInformation *)mode flags:(UInt32 *)flags
{
    const TimingToEDID	*	lookTiming;
    UInt32			estBit, i;
    enum {			kSetFlags = (kDisplayModeValidFlag | kDisplayModeSafeFlag) };

    lookTiming = timingToEDID;
    while( lookTiming < (timingToEDID + sizeof( timingToEDID) / sizeof( TimingToEDID))) {

	if( lookTiming->timingID == mode->standardTimingID) {
	    estBit = lookTiming->establishedBit;
	    if( estBit != 0xff) {
		if( edid1.establishedTimings[ estBit / 8 ] & (1 << (estBit % 8)))
		    *flags = kSetFlags;
	    }
	    for( i = 0; i < 8; i++ ) {
		if( lookTiming->standardTiming == edid1.standardTimings[ i ]) {
		    *flags = kSetFlags;
		    break;
		}
	    }
	    break;
	}
	lookTiming++;
    }

    // Pass the existing flags (from framebuffer) thru
    return( noErr);
}

@end


#define	kOrgDisplayAddr 		0x7		// Original display ADB address

#define kTelecasterADBHandlerID		0x03
#define kSmartDisplayADBHandlerID	0xc0

#define	kADBReg0			0x0		// Device register zero
#define	kADBReg1			0x1		// Device register one
#define	kADBReg2			0x2		// Device register two
#define	kADBReg3			0x3		// Device register three

#define	kReg2DataRdy			0xFD	// data (to be read) ready
#define	kReg2DataAck			0xFE	// data (just written) OK

#define	kNoDevice		-1
#define	kTelecaster		0
#define	kSousaSoundUnit		1
#define	kHammerhead		2
#define	kOrca			3
#define	kWhaler			4
#define kWarriorEZ		5
#define kManta			6
#define kLastDeviceType		kManta


#define kWiggleLADAddr			0x04	// 0x0f on Telecaster & Sousa?
#define kDisplayLocalRemoteLADAddr	0x02	// lad address used in SetDisplayRemoteMode
#define kAudioKeypadEnableLADAddr	0x7D

#define kUnknown	-1
#define kLocal		0
#define kRemote		1

static const UInt32		orcaTimings[] = {
	timingApple_640x480_67hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_640x480_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_640x480_85hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_832x624_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_1024x768_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1024x768_85hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_1152x870_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeDefaultFlag,
	timingVESA_1280x960_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1280x1024_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1280x1024_85hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1600x1200_60hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1600x1200_65hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1600x1200_70hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1600x1200_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag
};

static const UInt32		hammerheadTimings[] = {
	timingApple_640x480_67hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_832x624_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_1024x768_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_1152x870_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeDefaultFlag,
	timingVESA_1280x960_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1280x1024_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingVESA_1600x1200_60hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
};

static const UInt32		mantaTimings[] = {
	timingApple_640x480_67hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_832x624_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag,
	timingApple_1024x768_75hz,	kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeDefaultFlag,
};


static const AVDeviceInfo	orcaInfo = 
{
	kWiggleLADAddr,
	sizeof( orcaTimings) / sizeof( UInt32) / 2, orcaTimings
};

static const AVDeviceInfo	hammerheadInfo = 
{
	kWiggleLADAddr,
	sizeof( hammerheadTimings) / sizeof( UInt32) / 2, hammerheadTimings
};

static const AVDeviceInfo	mantaInfo = 
{
	kWiggleLADAddr,
	sizeof( mantaTimings) / sizeof( UInt32) / 2, mantaTimings
};

static const AVDeviceInfo *	deviceInfoTable[ kLastDeviceType  + 1 ] = 
{ 
	0, 0, &hammerheadInfo, &orcaInfo, &orcaInfo, &hammerheadInfo, &mantaInfo
};


@implementation IOSmartADBDisplay

- (IOReturn) getDisplayInfoForMode:(IOFBTimingInformation *)mode flags:(UInt32 *)flags
{
    IOReturn		err = noErr;
    UInt32		numTimings;
    const UInt32    *	timings = deviceInfo->timings;

    *flags = 0;
    numTimings = deviceInfo->numTimings;
    while( numTimings--) {
	if( mode->standardTimingID == *(timings++)) {
	    *flags = *timings;
	    break;
	}
	timings++;
    }
    return( err);
}

- (IOReturn) doConnect
{
    IOReturn   	err;
    UInt16 	value;
    UInt32	retries = 9;

    while( retries--) {
	value = 0x6000 | (adbAddr << 8);
	err = adb_writereg( adbAddr, kADBReg3, value);
	if (err != ADB_RET_OK)
            continue;
	IOSleep(10);  
	/* IODelay(1000); */
	err = adb_readreg( adbAddr, kADBReg3, &value);
    	if (err != ADB_RET_OK)
	    continue;

	if( (value & 0xF000) == 0x6000)
	    break;
	else
	    err = ADB_RET_UNEXPECTED_RESULT;
    }

    if( err)
	kprintf( "SMDoConnect %d\n", err);

    return( err);
}


void SMADBHandler( int number, unsigned char *buffer, int count, void * ssp)
{
IOSmartADBDisplay * sm;

    sm = ADB2SmartDisplay[ number ];
    if( sm && count)
	if( *buffer == sm->waitAckValue)
	    sm->waitAckValue = 0;
}

- (IOReturn) writeWithAcknowledge:(UInt8)regNum data:(UInt16)data ackValue:(UInt8)ackValue
{
    IOReturn	err;
    enum { kTimeoutMS = 400 };
    UInt32	timeout;

    waitAckValue = ackValue;

    err = adb_writereg(adbAddr, regNum, data);

    if( !err) {
	timeout = kTimeoutMS / 10;
	while( waitAckValue && timeout--)
	    IOSleep( 10 );

	if( waitAckValue)
	    err = -3;
    }
    waitAckValue = 0;
    return( err);
}


- (IOReturn) setLogicalRegister:(UInt16)address data:(UInt16) data
{
IOReturn	err = -1;
UInt32		reconnects = 3;

    while( err && reconnects--)
    {
	err = [self writeWithAcknowledge:kADBReg1 data:address ackValue:kReg2DataRdy];
	if( err == noErr) {
	    err = [self writeWithAcknowledge:kADBReg2 data:data ackValue:kReg2DataAck];
	}
	if( err)
	    if( [self doConnect])
		break;
    }

    if( err)
	kprintf( "SMSetLogicalRegister %x, %d\n", address, err);

    return( err);
}


- (IOReturn) getLogicalRegister:(UInt16)address data:(UInt16 *) data
{
IOReturn	err = -1;
UInt32		reconnects = 3;
UInt16		value;

    while( err && reconnects--)
    {
	err = [self writeWithAcknowledge:kADBReg1 data:address ackValue:kReg2DataRdy];
	if( err == noErr) {
	    err = adb_readreg( adbAddr, kADBReg2, &value);
	    *data = value & 0xff;			// actually only 8 bit
	}
	if( err)
	    if( [self doConnect])
		break;
    }

    if( err)
	kprintf( "SMGetLogicalRegister %x=%x, %d\n", address, *data, err);

    return( err);
}

- setWiggle:(BOOL) active
{
    IOReturn	err;

    err = [self setLogicalRegister:deviceInfo->wiggleLADAddr data:(active ? 1 : 0)];
    return( self);
}

- initForADB:(UInt8)adbBusAddr
{
    IOReturn	err;
    UInt16	data, deviceType;

    [super init];
    adbAddr = adbBusAddr;

    ADB2SmartDisplay[ adbAddr ] = self;

    do {

	err = [self doConnect];
	if( err)
	    continue;
	adb_register_dev( adbAddr, SMADBHandler);
	err = [self setLogicalRegister:0xff data:0xff];
	if( err)
	    continue;
	err = [self getLogicalRegister:0xff data:&data];
	if( err)
	    continue;
	err = [self getLogicalRegister:0xff data:&deviceType];
	if( err)
	    continue;

	kprintf("Found ADBDisplay@%d, AVType %d\n", adbAddr, deviceType );

	if( (deviceType > kLastDeviceType)
	    || (deviceInfoTable[ deviceType ] == (const AVDeviceInfo *)0) ) {
	    err = -49;
	    continue;
	}

	avDisplayID = deviceType;
	deviceInfo = deviceInfoTable[ deviceType ];
	[self setWiggle:NO];

    } while( false);

    if( err) {
	return( [self free]);
    }
    return( self);
}

- free
{

//  adb_register_dev( adbAddr, 0);	// can't unregister!
    ADB2SmartDisplay[ adbAddr ] = 0;
    return( [super free]);
}

//kprintf( "%s: sense %d, ext sense %d\n", [framebuffer name], info.primarySense, info.extendedSense);

- attach:framebuffer refCon:(UInt32)refCon
{
    IOReturn		err;
    IOFBAppleSenseInfo	info;
    id			attached = nil;

    if( [self attached])
	return( nil);

    do {
	err = [framebuffer getAppleSense:refCon info:&info];
	if( err)
	    continue;
	if( (info.primarySense != kRSCSix) || (info.extendedSense != kESCSixStandard))		// straight-6
	    continue;

	[self setWiggle:YES];
	err = [framebuffer getAppleSense:refCon info:&info];
	[self setWiggle:NO];
	if( err)
	    continue;
	if( (info.primarySense != kRSCFour) || (info.extendedSense != kESCFourNTSC))		// straight-4
	    continue;

 	kprintf("ADBDisplay@%d attached to %s\n", adbAddr, [framebuffer name]);
	attached = [super attach:framebuffer refCon:refCon];

    } while( false);

    return( attached);
}

+ probeADBForDisplays
{
    struct adb_device   *devp;
    int 	i;

    devp = adb_devices;
    for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {

        if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
            continue;
	// no sense looking for telecaster since its fixed freq
	// && (devp->a_dev_handler != kTelecasterADBHandlerID) )
	if( (devp->a_dev_handler != kSmartDisplayADBHandlerID) )
            continue;

	[[IOSmartADBDisplay alloc] initForADB:i];
    }
    return( self);
}

@end
