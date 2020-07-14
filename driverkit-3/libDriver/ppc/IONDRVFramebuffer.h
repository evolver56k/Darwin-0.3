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



#import <driverkit/displayDefs.h>
#import <driverkit/ppc/IOFramebuffer.h>
#import "IONDRVInterface.h"
#import <driverkit/ppc/IOPCIDevice.h>
#import "IOMacOSVideo.h"


@interface IONDRVFramebuffer:IOFramebuffer <IOFBAppleSense, IOFBHighLevelDDCSense>
{
    /* 
     * Transfer tables.
     */
    ColorSpec       *		transferTable;
    ColorSpec 	* 		scaledTable;
    int         		transferTableCount; // # of entries in transfer
                        			//    table
    int         		brightnessLevel;  
    BOOL			gammaKilled;  

    UInt32	       		 modeListCount;
    IODisplayInfo    *		modeList;
    IODisplayInfo    *		defaultDisplayMode;

    IOPCIDevice  	* 	ioDevice;
    NDRVInstance		ndrvInst;
    LogicalAddress		doDriverIO;
    DriverDescription *		theDriverDesc;

    int				consoleDevice;
    int				ndrvState;

    UInt32			vramBase;
    UInt32			vramLength;

    SInt32			cachedModeIndex;
    SInt32			cachedModeID;
    VDResolutionInfoRec		cachedVDResolution;
#if 0
    TVector			intHandler;
    TVector			intEnabler;
    TVector			intDisabler;

    TVector		*	currentIntHandler;
    TVector		*	currentIntEnabler;
    TVector		*	currentIntDisabler;
#endif
}

- (IOReturn) checkDriver;
- (IOReturn) doControl:(UInt32)code params:(void *)params;
- (IOReturn) doStatus:(UInt32)code params:(void *)params;

@end

@interface IONDRVFramebuffer (ProgramDAC)

- setTransferTable:(const unsigned int *)table count:(int)count;
- (IOReturn)getTransferTable:(unsigned int *)table count:(int *)count;
- setBrightness:(int)level token:(int)t;
- setTheTable;

@end

@interface IOOFFramebuffer:IOFramebuffer
{
}
@end

#define IO_DISPLAY_CAN_FILL		0x00000040
#define IO_DISPLAY_FILL_MASK		0x00000040
#define IO_DISPLAY_DO_FILL           	"IODisplayDoFill"
#define IO_DISPLAY_FILL_SIZE		5
#define IO_DISPLAY_GET_SYNCED		"IOGetDisplaySynced"
#define IO_DISPLAY_GET_SYNCED_SIZE	1

@interface IOATINDRV:IONDRVFramebuffer
{
}
@end

@interface IOATIMACH64NDRV:IOATINDRV
{
}
@end
