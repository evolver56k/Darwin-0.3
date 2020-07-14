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


#define KERNOBJC 1			// remove
#define KERNEL_PRIVATE 1
#define DRIVER_PRIVATE 1

#import <driverkit/generalFuncs.h>
#import <driverkit/IODisplay.h>
#import <driverkit/IODisplayPrivate.h>
#import	<driverkit/IOFrameBufferShared.h>
#import	<bsd/dev/kmreg_com.h>
#import	<bsd/dev/ppc/kmDevice.h>

#import <mach/vm_param.h>       /* for round_page() */
#import <machdep/ppc/proc_reg.h>
#import <string.h>

#import	<driverkit/ppc/IOFramebuffer.h>
#import	"IONDRVFramebuffer.h"

extern IODisplayInfo	bootDisplayInfo;

@implementation IONDRVFramebuffer

//=======================================================================

- (IOReturn)doControl:(UInt32)code params:(void *)params
{
    IOReturn	err;
    CntrlParam	pb;

    pb.qLink = 0;
    pb.csCode = code;
    pb.csParams = params;

    err = NDRVDoDriverIO( doDriverIO, /*ID*/ (UInt32) &pb, &pb,
	kControlCommand, kImmediateIOCommandKind );

    return( err);
}

- (IOReturn)doStatus:(UInt32)code params:(void *)params
{
    IOReturn	err;
    CntrlParam	pb;

    pb.qLink = 0;
    pb.csCode = code;
    pb.csParams = params;

    err = NDRVDoDriverIO( doDriverIO, /*ID*/ (UInt32) &pb, &pb,
	kStatusCommand, kImmediateIOCommandKind );

    return( err);
}

//=======================================================================

+ (BOOL)probe:device
{
    id		inst;
    Class       newClass = self;
    char *	name;

    if( NDRVForDevice( device)) {
        // temporary for in-kernel acceleration
        name = [device nodeName];
        if( 0 == strncmp("ATY,", name, strlen("ATY,")))
            newClass = [IOATINDRV class];
    } else
	newClass = [IOOFFramebuffer class];

    // Create an instance and initialize
    inst = [[newClass alloc] initFromDeviceDescription:device];
    if (inst == nil)
        return NO;

    [inst setDeviceKind:"Linear Framebuffer"];

    [inst registerDevice];

    return YES;
}

- initFromDeviceDescription:(IOPCIDevice *) deviceDescription
{
    id			me = nil;
    kern_return_t	err = 0;
    UInt32		i, propSize;
    UInt32		numMaps = 8;
    IOApertureInfo	maps[ numMaps ];	// FIX
    IOLogicalAddress	aaplAddress[ 8 ];	// FIX
    IOApertureInfo   *	map;
    IOPropertyTable  *	propTable;
    void *		prop;
    char *		logname;
    InterruptSetMember	intsProperty[ 3 ];

    do {
   	ioDevice = deviceDescription;
	logname = [ioDevice nodeName];
	propTable = [ioDevice propertyTable];

	prop = &ndrvInst;
	propSize = sizeof( NDRVInstance);
	err = [propTable getProperty:"AAPL,ndrvInst" flags:0 value:&prop length:&propSize];
	if( err)
	    continue;
	err = NDRVGetSymbol( ndrvInst, "DoDriverIO", &doDriverIO );
	if( err)
	    continue;
	err = NDRVGetSymbol( ndrvInst, "TheDriverDescription", (void **)&theDriverDesc );
	if( err)
	    continue;
    
	transferTable = IOMalloc(sizeof( ColorSpec) * 256);    // Initialize transfer table variables.  
	brightnessLevel = EV_SCREEN_MAX_BRIGHTNESS;
	scaledTable = 0;
	cachedModeIndex = 0x7fffffff;

	numMaps = 8;
	err = [ioDevice getApertures:maps items:&numMaps];
	if( err)
	    continue;
    
	for( i = 0, map = maps; i < numMaps; i++, map++) {

	    if( (((UInt32)bootDisplayInfo.frameBuffer) >= ((UInt32)map->physical))
	    &&  (((UInt32)bootDisplayInfo.frameBuffer) < (map->length + (UInt32)map->physical)) )
		consoleDevice = YES;

	    // this means any graphics card grabs a BAT for the segment.
	    aaplAddress[ i ] = PEMapSegment( map->physical, 0x10000000);
	    if( aaplAddress[ i ] != map->physical) {
		IOLog("%s: NDRV needs 1-1 mapping\n", logname);
		err = IO_R_VM_FAILURE;
		break;
	    }

#if 0
           // Declare range for pmap'ing into user space
           // Shouldn't be here but it's too expensive to do it for all devices
           if( map->length > 0x01000000)
               map->length = 0x01000000;               // greed hack
           err = pmap_add_physical_memory( map->physical,
                                           map->physical + map->length, FALSE, PTE_WIMG_IO);
           if(err)
               kprintf("%s: pmap_add_physical_memory: %d for 0x%08x - 0x%08x\n",
                       logname, err, map->physical, map->physical + map->length);
#endif
	}
	if( err)
	    continue;

	// NDRV aperture vectors
        [propTable deleteProperty:"AAPL,address"];
        err = [propTable createProperty:"AAPL,address" flags:0
		    value:aaplAddress length:(numMaps * sizeof( IOLogicalAddress))];
	if( err)
	    continue;

	// interrupt tree for NDRVs - not really
	err = [propTable createProperty:"driver-ist" flags:0
		    value:intsProperty length:sizeof( intsProperty)];

	// tell kmDevice if it's on the boot console, it's going away
	if( consoleDevice)
	    [kmId unregisterDisplay:nil];

	err = [self checkDriver];
	if( err) {
	    kprintf("%s: Not usable\n", logname );
            if( err == -999)
                IOLog("%s: Driver is incompatible.\n", logname );
	    continue;
	}

	if (nil == [super initFromDeviceDescription:deviceDescription])
	    continue;
	err = [self open];
	if( err)
	    continue;

#if 0
	if ([self startIOThread] != IO_R_SUCCESS)
	    kprintf("startIOThread FAIL\n");
	[self enableAllInterrupts];
#endif

	me = self;			// Success

    } while( false);

    if( me == nil)
	[super free];

    return( me);
}


- free
{
    if (transferTable != 0) {
        IOFree(transferTable, 256 * sizeof( ColorSpec));
    }
    return [super free];
}


- (IOReturn)
    getDisplayModeTiming:(IOFBIndex)connectIndex mode:(IOFBDisplayModeID)modeID
		timingInfo:(IOFBTimingInformation *)info connectFlags:(UInt32 *)flags
{
    VDTimingInfoRec		timingInfo;
    OSStatus			err;

    timingInfo.csTimingMode = modeID;
    timingInfo.csTimingFormat = kDeclROMtables;			// in case the driver doesn't do it
    err = [self doStatus:cscGetModeTiming params:&timingInfo];
    if( err == noErr) {
	if( timingInfo.csTimingFormat == kDeclROMtables)
	    info->standardTimingID = timingInfo.csTimingData;
	else
	    info->standardTimingID = timingInvalid;
	*flags = timingInfo.csTimingFlags;
	return( [super getDisplayModeTiming:connectIndex mode:modeID timingInfo:info connectFlags:flags]);
    }
    *flags = 0;
    return( IO_R_UNDEFINED_MODE);
}

- (IOReturn)
    getDisplayModeByIndex:(IOFBIndex)modeIndex displayMode:(IOFBDisplayModeID *)displayModeID
{

    // unfortunately, there is no "kDisplayModeIDFindSpecific"
    if( modeIndex <= cachedModeIndex) {
	cachedModeID = kDisplayModeIDFindFirstResolution;
	cachedModeIndex = -1;
    }

    cachedVDResolution.csPreviousDisplayModeID = cachedModeID;

    while( 
 	   (noErr == [self doStatus:cscGetNextResolution params:&cachedVDResolution])
	&& ((SInt32) cachedVDResolution.csDisplayModeID > 0) ) {

	    cachedVDResolution.csPreviousDisplayModeID = cachedVDResolution.csDisplayModeID;

	    if( modeIndex == ++cachedModeIndex) {
		cachedModeID 		= cachedVDResolution.csDisplayModeID;
		*displayModeID		= cachedModeID;
		return( noErr);
	    }
    }
    cachedModeIndex = 0x7fffffff;
    return( IO_R_UNDEFINED_MODE);
}


- (IOReturn)
    getResInfoForMode:(IOFBDisplayModeID)modeID info:(VDResolutionInfoRec **)theInfo
{

    *theInfo = &cachedVDResolution;

    if( cachedVDResolution.csDisplayModeID == modeID)
	return( noErr);

    cachedVDResolution.csPreviousDisplayModeID = kDisplayModeIDFindFirstResolution;

    while(
	(noErr == [self doStatus:cscGetNextResolution params:&cachedVDResolution])
    && ((SInt32) cachedVDResolution.csDisplayModeID > 0) )
    {
	cachedVDResolution.csPreviousDisplayModeID = cachedVDResolution.csDisplayModeID;
	if( cachedVDResolution.csDisplayModeID == modeID)
	    return( noErr);
    }
    cachedVDResolution.csDisplayModeID = -1;
    return( IO_R_UNDEFINED_MODE);
}

- (IOReturn)
    getDisplayModeInformation:(IOFBDisplayModeID)modeID info:(IOFBDisplayModeInformation *)info 
{
    IOReturn			err;
    VDResolutionInfoRec	*	resInfo;

    do {
	err = [self getResInfoForMode:modeID info:&resInfo];
	if( err)
	    continue;
	info->displayModeID	= resInfo->csDisplayModeID;
	info->maxDepthIndex	= resInfo->csMaxDepthMode - kDepthMode1;
	info->nominalWidth	= resInfo->csHorizontalPixels;
	info->nominalHeight	= resInfo->csVerticalLines;
	info->refreshRate	= resInfo->csRefreshRate;
	return( noErr);
    } while( false);

    return( IO_R_UNDEFINED_MODE);
}


- (IOReturn)
    getPixelInformationForDisplayMode:(IOFBDisplayModeID)modeID andDepthIndex:(IOFBIndex)depthIndex
    	pixelInfo:(IOFBPixelInformation *)info
{
    SInt32		err;

    VDVideoParametersInfoRec	pixelParams;
    VPBlock			pixelInfo;
    VDResolutionInfoRec	*	resInfo;

    do {
	err = [self getResInfoForMode:modeID info:&resInfo];
	if( err)
	    continue;
    	pixelParams.csDisplayModeID = modeID;
	pixelParams.csDepthMode = depthIndex + kDepthMode1;
	pixelParams.csVPBlockPtr = &pixelInfo;
	err = [self doStatus:cscGetVideoParameters params:&pixelParams];
	if( err)
	    continue;
    
	info->flags 			= 0;
	info->rowBytes             	= pixelInfo.vpRowBytes & 0x7fff;
	info->width                	= pixelInfo.vpBounds.right;
	info->height               	= pixelInfo.vpBounds.bottom;
	info->refreshRate          	= resInfo->csRefreshRate;
	info->pageCount 		= 1;
	info->pixelType 		= (pixelInfo.vpPixelSize <= 8) ?
					kIOFBRGBCLUTPixelType : kIOFBDirectRGBPixelType;
	info->bitsPerPixel 		= pixelInfo.vpPixelSize;
//	info->channelMasks 		=    

    } while( false);

    return( err);
}

- (IOReturn) open
{
    IOReturn	err;

    do {
	err = [self checkDriver];
	if( err)
	    continue;
	err = [super open];

    } while( false);

    return( err);
}

- (IOReturn) checkDriver
{
    OSStatus			err = noErr;
    struct  DriverInitInfo	initInfo;
    CntrlParam          	pb;
    IOFBConfiguration		config;
    VDClutBehavior		clutSetting = kSetClutAtSetEntries;

    if( ndrvState == 0) {
	do {
	    initInfo.refNum = 0xffcd;			// ...sure.
	    MAKE_REG_ENTRY(initInfo.deviceEntry, ioDevice)
    
	    err = NDRVDoDriverIO( doDriverIO, 0, &initInfo, kInitializeCommand, kImmediateIOCommandKind );
	    if( err) continue;
	
	    err = NDRVDoDriverIO( doDriverIO, 0, &pb, kOpenCommand, kImmediateIOCommandKind );
	    if( err) continue;
    
	} while( false);
	if( err)
	    return( err);

	{
	    VDVideoParametersInfoRec	pixelParams;
	    VPBlock			pixelInfo;
	    VDResolutionInfoRec		vdRes;
	    UInt32			size;

	    vramLength = 0;
	    vdRes.csPreviousDisplayModeID = kDisplayModeIDFindFirstResolution;
	    while(
		(noErr == [self doStatus:cscGetNextResolution params:&vdRes])
	    && ((SInt32) vdRes.csDisplayModeID > 0) )
	    {
    
		pixelParams.csDisplayModeID = vdRes.csDisplayModeID;
		pixelParams.csDepthMode = vdRes.csMaxDepthMode;
		pixelParams.csVPBlockPtr = &pixelInfo;
		err = [self doStatus:cscGetVideoParameters params:&pixelParams];
		if( err)
		    continue;

                // Control hangs its framebuffer off the end of the aperture to support
                // 832 x 624 @ 32bpp. The commented out version will correctly calculate
		// the vram length, but DPS needs the full extent to be mapped, so we'll
                // end up mapping an extra page that will address vram through the little
                // endian aperture. No other drivers like this known.
#if 1
		size = 0x40 + pixelInfo.vpBounds.bottom * (pixelInfo.vpRowBytes & 0x7fff);
#else
		size = ( (pixelInfo.vpBounds.right * pixelInfo.vpPixelSize) / 8)	// last line
			+ (pixelInfo.vpBounds.bottom - 1) * (pixelInfo.vpRowBytes & 0x7fff);
#endif
		if( size > vramLength)
		    vramLength = size;
    
		vdRes.csPreviousDisplayModeID = vdRes.csDisplayModeID;
	    }
   
	    err = [self getConfiguration:&config];
	    vramBase = config.physicalFramebuffer;
	    vramLength = (vramLength + (vramBase & 0xffff) + 0xffff) & 0xffff0000;
	    vramBase &= 0xffff0000;

#if 1
	    // Declare range for pmap'ing into user space
	    // Shouldn't be here but it's too expensive to do it for all devices
	    err = pmap_add_physical_memory( vramBase,
					    vramBase + vramLength, FALSE, PTE_WIMG_IO);
	    if(err)
		kprintf("%s: pmap_add_physical_memory: %d for 0x%08x - 0x%08x\n",
			[self name], err, vramBase, vramBase + vramLength );
#endif
	}

	// Set CLUT immediately since there's no VBL.
	[self doControl:cscSetClutBehavior params:&clutSetting];

	ndrvState = 1;
    }
    return( noErr);
}


- (IOReturn)
    setDisplayMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex page:(IOFBIndex)pageIndex;
{
    SInt32		err;
    VDSwitchInfoRec	switchInfo;
    VDPageInfo		pageInfo;

    switchInfo.csData = modeID;
    switchInfo.csMode = depthIndex + kDepthMode1;
    switchInfo.csPage = pageIndex;
    err = [self doControl:cscSwitchMode params:&switchInfo];
    if(err)
	IOLog("%s: cscSwitchMode:%d\n",[self name],(int)err);

    // duplicate QD InitGDevice
    pageInfo.csMode = switchInfo.csMode;
    pageInfo.csData = 0;
    pageInfo.csPage = pageIndex;
    [self doControl:cscSetMode params:&pageInfo];
    [self doControl:cscGrayPage params:&pageInfo];

    return( err);
}

- (IOReturn)
    setStartupMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex;
{
    SInt32		err;
    VDSwitchInfoRec	switchInfo;

    switchInfo.csData = modeID;
    switchInfo.csMode = depthIndex + kDepthMode1;
    err = [self doControl:cscSavePreferredConfiguration params:&switchInfo];
    return( err);
}

- (IOReturn)
    getStartupMode:(IOFBDisplayModeID *)modeID depth:(IOFBIndex *)depthIndex
{
    SInt32		err;
    VDSwitchInfoRec	switchInfo;

    err = [self doStatus:cscGetPreferredConfiguration params:&switchInfo];
    if( err == noErr) {
	*modeID		= switchInfo.csData;
	*depthIndex	= switchInfo.csMode - kDepthMode1;
    }
    return( err);
}


- (IOReturn)
    getConfiguration:(IOFBConfiguration *)config;
{
    IOReturn		err;
    VDSwitchInfoRec	switchInfo;
    VDGrayRecord	grayRec;

    bzero( config, sizeof( IOFBConfiguration));

    grayRec.csMode = 0;			// turn off luminance map
    err = [self doControl:cscSetGray params:&grayRec];
    if( (noErr == err) && (0 != grayRec.csMode))
        // driver refused => mono display
	config->flags |= kFBLuminanceMapped;

    err = [self doStatus:cscGetCurMode params:&switchInfo];
    config->displayMode		= switchInfo.csData;
    config->depth		= switchInfo.csMode - kDepthMode1;
    config->page		= switchInfo.csPage;
    config->physicalFramebuffer	= ((UInt32) switchInfo.csBaseAddr);
    config->mappedFramebuffer	= config->physicalFramebuffer;		// assuming 1-1

    return( err);
}

- (IOReturn)
    getApertureInformationByIndex:(IOFBIndex)apertureIndex
    	apertureInfo:(IOFBApertureInformation *)apertureInfo
{
    IOReturn		err;

    if( apertureIndex == 0) {
	apertureInfo->physicalAddress 	=	vramBase;
	apertureInfo->mappedAddress 	=	vramBase;
	apertureInfo->length 		=	vramLength;
	apertureInfo->cacheMode 	=	0;
	apertureInfo->type		=	kIOFBGraphicsMemory;
	err = noErr;
    } else
	err = IO_R_UNDEFINED_MODE;

    return( err);
}

#if 0

- (IOReturn) getInterruptFunctionsTV:(UInt32)member
			refCon:(void **)refCon,
			handler:(TVector *) handler,
			enabler:(TVector *) enabler,
			disabler:(TVector *) disabler

{
    OSStatus	err;

    err = [interrupt getInterruptFunctionsTV:member refCon:refCon handler:handler
			enabler:enabler disabler:disabler];
    return( err);
}


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


- (IOReturn) setInterruptFunctionsTV:(UInt32)member
			refCon:(void *)refCon,
			handler:(TVector *) handler,
			enabler:(TVector *) enabler,
			disabler:(TVector *) disabler
{

   if( handler != NULL)
	currentIntHandler = handler;
   if( enabler != NULL)
	currentIntEnabler = enabler;
   if( disabler != NULL)
	currentIntDisabler = disabler;

    err = [interrupt setInterruptFunctions:member refCon:refCon handler:currentIntHandler
			enabler:enabler disabler:disabler];

}

- (IOReturn) setInterruptFunctions:(UInt32)member
			refCon:(void *)refCon,
			handler:(InterruptHandler) handler,
			enabler:(InterruptEnabler) enabler,
			disabler:(InterruptDisabler) disabler



- (IOReturn) getInterruptFunctions:(UInt32)member
			refCon:(void **)refCon,
			handler:(InterruptHandler *) handler,
			enabler:(InterruptEnabler *) enabler,
			disabler:(InterruptDisabler *) disabler

#endif

//////////////////////////////////////////////////////////////////////////////////////////

- (IOReturn) getAppleSense:(IOFBIndex)connectIndex info:(IOFBAppleSenseInfo *)info;
{
    OSStatus			err;
    VDDisplayConnectInfoRec	displayConnect;

    err = [self doStatus:cscGetConnection params:&displayConnect];
    if( err)
	return( err);

    if( displayConnect.csConnectFlags & ((1<<kReportsTagging) | (1<<kTaggingInfoNonStandard))
	!= ((1<<kReportsTagging)) )

	err = IO_R_UNSUPPORTED;

    else {
//	info->standardDisplayType 	= displayConnect.csDisplayType;
	info->primarySense 	= displayConnect.csConnectTaggedType;
	info->extendedSense = displayConnect.csConnectTaggedData;
	if( (info->primarySense == 0) && (info->extendedSense == 6)) {
	    info->primarySense          = kRSCSix;
	    info->extendedSense         = kESCSixStandard;
	}
	if( (info->primarySense == 0) && (info->extendedSense == 4)) {
	    info->primarySense          = kRSCFour;
	    info->extendedSense         = kESCFourNTSC;
	}
    }
    return( err);
}

- (BOOL) hasDDCConnect:(IOFBIndex)connectIndex
{
    OSStatus			err;
    VDDisplayConnectInfoRec	displayConnect;
    enum		{	kNeedFlags = (1<<kReportsDDCConnection) | (1<<kHasDDCConnection) };

    err = [self doStatus:cscGetConnection params:&displayConnect];
    if( err)
        return( NO);

    return( (displayConnect.csConnectFlags & kNeedFlags) == kNeedFlags );
}

- (IOReturn) getDDCBlock:(IOFBIndex)connectIndex blockNumber:(UInt32)num blockType:(OSType)type
	options:(UInt32)options data:(UInt8 *)data length:(ByteCount *)length
{
    OSStatus		err = 0;
    VDDDCBlockRec	ddcRec;
    ByteCount		actualLength = *length;

    ddcRec.ddcBlockNumber 	= num;
    ddcRec.ddcBlockType 	= type;
    ddcRec.ddcFlags 		= options;

    err = [self doStatus:cscGetDDCBlock params:&ddcRec];

    if( err == noErr) {

	actualLength = (actualLength < kDDCBlockSize) ? actualLength : kDDCBlockSize;
        bcopy( ddcRec.ddcBlockData, data, actualLength);
	*length = actualLength;
    }
    return( err);
}

//////////////////////////////////////////////////////////////////////////////////////////

- (IOReturn)getIntValues:(unsigned *)parameterArray
		forParameter:(IOParameterName)parameterName
    		count:(unsigned int *)count
{
       
    if (strcmp(parameterName, "IOGetTransferTable") == 0) {
	return( [self getTransferTable:&parameterArray[0] count:count] );

    } else
	return [super getIntValues:parameterArray
	    forParameter:parameterName
	    count:count];
}



//////////////////////////////////////////////////////////////////////////////////////////
// IOCallDeviceMethods:

// Should be in NDRV class?
- (IOReturn) IONDRVGetDriverName:(char *)outputParams size:(unsigned *) outputCount
{
    char * 	name;
    UInt32	len, plen;

    name = theDriverDesc->driverOSRuntimeInfo.driverName;

    plen = *(name++);
    len = *outputCount - 1;
    *outputCount = plen + 1;
    if( plen < len)
	len = plen;
    strncpy( outputParams, name, len);
    outputParams[ len ] = 0;

    return( noErr);
}

- (IOReturn) IONDRVDoControl:(UInt32 *)inputParams inputSize:(unsigned) inputCount 
		params:(void *)outputParams outputSize:(unsigned *) outputCount
		privileged:(host_priv_t *)privileged
{
    IOReturn	err = noErr;
    UInt32	callSelect;

    if( privileged == NULL)
	err = IO_R_PRIVILEGE;
    else {
	callSelect = *inputParams;
	err = [self doControl:callSelect params:(inputParams + 1 )];
	bcopy( inputParams, outputParams, *outputCount);
    }
    return( err);
}

//////////////////////////////////////////////////////////////////////////////////////////

- (IOReturn) IONDRVDoStatus:(UInt32 *)inputParams inputSize:(unsigned) inputCount 
		params:(void *)outputParams outputSize:(unsigned *) outputCount
{
    IOReturn	err = noErr;
    UInt32	callSelect;

    callSelect = *inputParams;
    err = [self doStatus:callSelect params:(inputParams + 1 )];
    bcopy( inputParams, outputParams, *outputCount);
    return( err);

}

@end

//////////////////////////////////////////////////////////////////////////////////////////
// DACrap

@implementation IONDRVFramebuffer (ProgramDAC)

- setTheTable
{
    IOReturn	err;
    ColorSpec * table = transferTable;
    VDSetEntryRecord	setEntryRec;
    int		i, code;

    if( transferTableCount != 0) {
	if( brightnessLevel != EV_SCREEN_MAX_BRIGHTNESS) {
	    if( !scaledTable)
		scaledTable = IOMalloc( 256 * sizeof( ColorSpec));
	    if( scaledTable) {
		for( i = 0; i < transferTableCount; i++ ) {
		    scaledTable[ i ].rgb.red	= EV_SCALE_BRIGHTNESS( brightnessLevel, table[ i ].rgb.red);
		    scaledTable[ i ].rgb.green	= EV_SCALE_BRIGHTNESS( brightnessLevel, table[ i ].rgb.green);
		    scaledTable[ i ].rgb.blue	= EV_SCALE_BRIGHTNESS( brightnessLevel, table[ i ].rgb.blue);
		}
		table = scaledTable;
	    }
	} else {
	    if( scaledTable) {
		IOFree( scaledTable, transferTableCount * sizeof( ColorSpec));
		scaledTable = 0;
	    }
	}

	setEntryRec.csTable = table;
	setEntryRec.csStart = 0;
	setEntryRec.csCount = transferTableCount - 1;
	if( [self displayInfo]->bitsPerPixel == IO_8BitsPerPixel)
	    code = cscSetEntries;
	else
	    code = cscDirectSetEntries;
	err = [self doControl:code params:&setEntryRec];
    }
    return self;
}

- setBrightness:(int)level token:(int)t
{
    if ((level < EV_SCREEN_MIN_BRIGHTNESS) || 
        (level > EV_SCREEN_MAX_BRIGHTNESS)){
            IOLog("Display: Invalid arg to setBrightness: %d\n",
                level);
            return nil;
    }   
    brightnessLevel = level;    
    [self setTheTable];
    return self;
}

#define Expand8To16(x)  (((x) << 8) | (x))


- setTransferTable:(const unsigned int *)table count:(int)count
{

// no checking table count vs. depth ??

    int         k;
    IOBitsPerPixel  bpp;
    IOColorSpace    cspace;
    VDGammaRecord		gammaRec;

    bpp = [self displayInfo]->bitsPerPixel;
    cspace = [self displayInfo]->colorSpace;

    if( table) {
        transferTableCount = count;
        if (bpp == IO_8BitsPerPixel && cspace == IO_OneIsWhiteColorSpace) {
            for (k = 0; k < count; k++) {
                transferTable[k].rgb.red = transferTable[k].rgb.green = transferTable[k].rgb.blue
                    = Expand8To16(table[k] & 0xFF);
            }
        } else if (cspace == IO_RGBColorSpace &&
            ((bpp == IO_8BitsPerPixel) ||
            (bpp == IO_15BitsPerPixel) ||
            (bpp == IO_24BitsPerPixel))) {
            for (k = 0; k < count; k++) {
                transferTable[k].rgb.red    = Expand8To16((table[k] >> 24) & 0xFF);
                transferTable[k].rgb.green  = Expand8To16((table[k] >> 16) & 0xFF);
                transferTable[k].rgb.blue   = Expand8To16((table[k] >>  8) & 0xFF);
            }
        } else {
            transferTableCount = 0;
        }
    }

    if( NO == gammaKilled) {
	gammaRec.csGTable = 0;
	[self doControl:cscSetGamma params:&gammaRec];
	gammaKilled = YES;
    }

    [self setTheTable];
    return self;
}

- (IOReturn)getTransferTable:(unsigned int *)table count:(int *)count
{
    int	k;

    if( *count != transferTableCount)
	return( IO_R_INVALID_ARG);

    for (k = 0; k < transferTableCount; k++) {
	table[k] = ((transferTable[k].rgb.red << 16) & 0xff000000)
		|  ((transferTable[k].rgb.green << 8) & 0xff0000)
		|  ((transferTable[k].rgb.blue) & 0xff00)
		|  ((transferTable[k].rgb.blue >> 8) & 0xff);
    }
    return( IO_R_SUCCESS);
}

// Apple style CLUT - pass thru gamma table

- (IOReturn)
    setCLUT:(IOFBColorEntry *) colors index:(UInt32)index numEntries:(UInt32)numEntries 
    	brightness:(IOFixed)brightness options:(IOOptionBits)options
{
    IOReturn		err;
    VDSetEntryRecord	setEntryRec;
    int			code;

    setEntryRec.csTable = (ColorSpec *)colors;
    setEntryRec.csStart = index;
    setEntryRec.csCount = numEntries - 1;
    if( [self displayInfo]->bitsPerPixel == IO_8BitsPerPixel)
	code = cscSetEntries;
    else
	code = cscDirectSetEntries;
    err = [self doControl:code params:&setEntryRec];

    return( err);

}

#if 0
- (void) interruptOccurred
{
    kprintf("\n[%s]\n", [self name]);
    [self enableAllInterrupts];
}
#endif

@end

//////////////////////////////////////////////////////////////////////////////////////////

// OpenFirmware shim

enum { kTheDisplayMode	= 10 };

@implementation IOOFFramebuffer

//=======================================================================

- initFromDeviceDescription:(IOPCIDevice *) ioDevice
{
    id			me = nil;
    kern_return_t	err = 0;
    UInt32		i;
    UInt32		numMaps = 8;
    IOApertureInfo	maps[ numMaps ];	// FIX
    IOApertureInfo   *	map;

    do {
	numMaps = 8;
	err = [ioDevice getApertures:maps items:&numMaps];
	if( err)
	    continue;
    
	err = -1;
	for( i = 0, map = maps; i < numMaps; i++, map++) {

	    if( (((UInt32)bootDisplayInfo.frameBuffer) >= ((UInt32)map->physical))
	    &&  (((UInt32)bootDisplayInfo.frameBuffer) < (map->length + (UInt32)map->physical)) ) {

		// Declare range for pmap'ing into user space
		// Shouldn't be here but it's too expensive to do it for all devices
		err = pmap_add_physical_memory( map->physical,
						map->physical + map->length, FALSE, PTE_WIMG_IO);
		if(err)
		    kprintf("%s: pmap_add_physical_memory:%d for 0x%08x - 0x%08x\n",
			    [self name], err, map->physical, map->physical + map->length);
	    }
	}
	if( err)
	    continue;

	if (nil == [super initFromDeviceDescription:ioDevice])
	    continue;
	err = [self open];
	if( err)
	    continue;

	me = self;			// Success

    } while( false);

    if( me == nil)
	[super free];

    return( me);
}

- (IOReturn)
    getDisplayModeByIndex:(IOFBIndex)modeIndex displayMode:(IOFBDisplayModeID *)displayModeID
{
    if( modeIndex)
	return( IO_R_UNDEFINED_MODE);
    *displayModeID = kTheDisplayMode;
    return( noErr);
}

- (IOReturn)
    getDisplayModeInformation:(IOFBDisplayModeID)modeID info:(IOFBDisplayModeInformation *)info 
{
    if( modeID == kTheDisplayMode) {
	info->displayModeID	= modeID;
	info->maxDepthIndex	= 0;
	info->nominalWidth	= bootDisplayInfo.width;
	info->nominalHeight	= bootDisplayInfo.height;
	info->refreshRate	= bootDisplayInfo.refreshRate << 16;
	return( noErr);
    }
    return( IO_R_UNDEFINED_MODE);
}


- (IOReturn)
    getPixelInformationForDisplayMode:(IOFBDisplayModeID)modeID andDepthIndex:(IOFBIndex)depthIndex
    	pixelInfo:(IOFBPixelInformation *)info
{

    if( (modeID == kTheDisplayMode) && (depthIndex == 0)) {
    
	info->flags 			= 0;
	info->rowBytes             	= bootDisplayInfo.rowBytes;
	info->width                	= bootDisplayInfo.width;
	info->height               	= bootDisplayInfo.height;
	info->refreshRate          	= bootDisplayInfo.refreshRate << 16;
	info->pageCount 		= 1;
	info->pixelType 		= kIOFBRGBCLUTPixelType;
	info->bitsPerPixel 		= 8;
//	info->channelMasks 		=    
	return( noErr);
    }
    return( IO_R_UNDEFINED_MODE);
}

- (IOReturn)
    setDisplayMode:(IOFBDisplayModeID)modeID depth:(IOFBIndex)depthIndex page:(IOFBIndex)pageIndex
{
    if( (modeID == kTheDisplayMode) && (depthIndex == 0) && (pageIndex == 0) )
	return( noErr);
    else
	return( IO_R_UNDEFINED_MODE);
}


- (IOReturn)
    getStartupMode:(IOFBDisplayModeID *)modeID depth:(IOFBIndex *)depthIndex
{
    *modeID		= kTheDisplayMode;
    *depthIndex		= 0;

    return( noErr);
}


- (IOReturn)
    getConfiguration:(IOFBConfiguration *)config
{
    bzero( config, sizeof( IOFBConfiguration));

    config->displayMode		= kTheDisplayMode;
    config->depth		= 0;
    config->page		= 0;
    config->physicalFramebuffer	= (IOPhysicalAddress) bootDisplayInfo.frameBuffer;
    config->mappedFramebuffer	= (IOVirtualAddress) bootDisplayInfo.frameBuffer;		// assuming 1-1

    return( noErr);
}

- (IOReturn)
    getDisplayModeTiming:(IOFBIndex)connectIndex mode:(IOFBDisplayModeID)modeID
                timingInfo:(IOFBTimingInformation *)info connectFlags:(UInt32 *)flags
{

    info->standardTimingID = timingInvalid;
    if( modeID == kTheDisplayMode) {
        *flags = kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeDefaultFlag;
        return( noErr);
    } else {
        *flags = 0;
        return( IO_R_UNDEFINED_MODE);
    }
}


- setBrightness:(int)level token:(int)t
{
    return self;
}

- setTransferTable:(const unsigned int *)table count:(int)count
{
    return self;
}

- (IOReturn)getIntValues:(unsigned *)parameterArray
		forParameter:(IOParameterName)parameterName
    		count:(unsigned int *)count
{
    const UInt8 *	clut;
    int			i;
    extern const UInt8  appleClut8[ 256 * 3 ];

    if( (0 == strcmp(parameterName, "IOGetTransferTable"))
	&& (*count == 256)) {
	for( clut = appleClut8, i = 0; i < 256; i++, clut += 3)
	    *(parameterArray++) = (clut[0] << 24) | (clut[1] << 16) | (clut[3] << 8) | 0xff;
	return( noErr);

    } else
	return [super getIntValues:parameterArray
	    forParameter:parameterName
	    count:count];
}

@end

//////////////////////////////////////////////////////////////////////////////////////////

// ATI patches. Acceleration to be removed when user level blitting is in.
// Real problem : getStartupMode doesn't.

@implementation IOATINDRV

- (IOReturn)
    getStartupMode:(IOFBDisplayModeID *)modeID depth:(IOFBIndex *)depthIndex
{

    IOReturn		err;
    UInt16	*	nvram;
    ByteCount		propSize = 8;

    err = [[ioDevice propertyTable] getProperty:"Sime" flags:kReferenceProperty
		value:(void *)&nvram length:&propSize];
    if( err == noErr) {
	*modeID = nvram[ 0 ];	// 1 is physDisplayMode
	*depthIndex = nvram[ 2 ] - kDepthMode1;
    }
    return( err);
}

@end





