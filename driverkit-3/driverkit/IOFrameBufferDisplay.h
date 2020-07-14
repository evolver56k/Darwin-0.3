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
/* 	Copyright (c) 1992-96 NeXT Software, Inc.  All rights reserved. 
 *
 * IOFrameBufferDisplay.h - Standard frame buffer display driver class.
 *
 *
 * HISTORY
 * 24 Oct 95	Rakesh Dubey
 *      Added mode change feature. 
 * 01 Sep 92	Joe Pasqua
 *      Created. 
 */

/* Notes:
 * This module defines an abstract superclass for "standard" (linear)
 * framebuffers.
 */

#ifndef __IOFRAMEBUFFERDISPLAY_H__
#define __IOFRAMEBUFFERDISPLAY_H__

#import <driverkit/IODisplay.h>

@interface IOFrameBufferDisplay:IODisplay
{
@private
    void *priv;
    /* Mapping tables used in cursor drawing to 5-5-5 displays. */
    unsigned char *_bm34To35SampleTable;
    unsigned char *_bm35To34SampleTable;

    /* Mapping tables used in cursor drawing to 8-bit RGB displays. */
    unsigned int *_bm256To38SampleTable;
    unsigned char *_bm38To256SampleTable;

    /* Used for dynamic mode changes */
    int _currentDisplayMode;
    int	_pendingDisplayMode;
    
    /* Modes supported by hardware */
    int _displayModeCount;
    IODisplayInfo *_displayModes;
    
    /* Reserved for future expansion. */
    int _IOFrameBufferDisplay_reserved[2];
}

/*
 * Put the display into linear framebuffer mode. This typically happens when
 * the window server starts running or the display mode is switched. To
 * simplify mode switching, revertToVGAMode is called first. This method is
 * implemented by subclasses in a device specific way. 
 */
- (void)enterLinearMode;

/*
 * Get the device out of whatever advanced linear mode it was using and back
 * into a state where it can be used as a standard VGA device. This method is
 * implemented by subclasses in a device specific way. 
 */
- (void)revertToVGAMode;

/*
 * Look up the physical memory location for this device instance and map it
 * into VM for use by the device driver. If problems occur, the method
 * returns (vm_address_t)0. If `addr' is not 0, then it is used as the
 * physical memory address and `length' is used as the length. 
 */
- (vm_address_t)mapFrameBufferAtPhysicalAddress:(unsigned int)addr
 	length:(int)length;

/*
 * Choose a mode from the list `modeList' (containing `count' modes) based on
 * the value of the `DisplayMode' key in the device's config table.  If
 * `isValid' is nonzero, each element specifies whether or not the
 * corresponding mode is valid. If you pass a non-NULL string as displayMode
 * it will use that instead of picking that from from the config table. 
 */
- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count
	valid:(const BOOL *)isValid modeString:(const char *)displayMode;

- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count
	valid:(const BOOL *)isValid;

/*
 * Equivalent to the above with `isValid' set to zero. 
 */
- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count;

/* IODevice methods reimplemented by this class. */

+ (BOOL)probe:deviceDescription;

- initFromDeviceDescription:deviceDescription;

- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

- (IOReturn)setCharValues		: (unsigned char *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

- setTransferTable:(const unsigned int *)table count:(int)count;

/* 'IOScreenEvents' protocol methods reimplemented by this class. */

- hideCursor: (int)token;

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

/* NOTE: Subclasses must override setBrightness and implement appropriately. */
- setBrightness:(int)level token:(int)t;

/*
 * Superclass will choose the next display mode by invoking this method. It
 * will actually enter the new mode by calling enterLinearMode at a later
 * time. 
 */
- (BOOL)setPendingDisplayMode:(int)displayMode;
- (int)pendingDisplayMode;

/*
 * The superclass will use the modeList passed in selectMode:.. call. If
 * desired, the subclass can override this behavior by implementing these two
 * methods. 
 */
- (unsigned int)displayModeCount;		/* total number of modes */
- (IODisplayInfo *)displayModes;		/* array of possible modes */

/*
 * Subclass should return appropriate values. 
 */
- (unsigned int)displayMemorySize;		/* display memory in bytes */
- (unsigned int)ramdacSpeed;			/* RAMDAC speed in Hz */

@end

#define IO_R_UNDEFINED_MODE		(-750)		// no such mode
#define IO_R_FAILED_TO_SET_MODE		(-751)		// mode change failed

#endif	/* __IOFRAMEBUFFERDISPLAY_H__ */
