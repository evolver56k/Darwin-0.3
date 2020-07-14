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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOSVGADisplay.h - SVGA display driver class.
 *
 *
 * HISTORY
 * 07 July 93	Scott Forstall
 *      Created.
 */

/*
 * Notes:
 * This module defines an abstract superclass for SVGA display
 * drivers.
 */

#import <driverkit/IODisplay.h>

@interface IOSVGADisplay:IODisplay
{
@private
    void *_priv;
}

/*
 * Select which 64K segment we intend to read from. This method is
 * implemented by subclasses in a device specific way.
 */
- (void)setReadSegment: (unsigned char)segmentNum;

/*
 * Select which 64K segment we intend to write to.  This method is
 * implemented by subclasses in a device specific way.
 */
- (void)setWriteSegment: (unsigned char)segmentNum;

/*
 * Select which of 4 bit planes to read from in planar modes.  This method
 * is implemented by subclasses in a device specific way.
 */
- (void)setReadPlane: (unsigned char)planeNum;

/*
 * Select which of 4 bit planes to write to in planar modes.  This method
 * is implemented by subclasses in a device specific way.
 */
- (void)setWritePlane: (unsigned char)planeNum;

/*
 * Save write plane, read plane, write segment, and read segment.
 * This method is implemented by subclasses in a device specific way.
 */
- (void)savePlaneAndSegmentSettings;

/*
 * Restore write plane, read plane, write segment, and read segment.
 * This method is implemented by subclasses in a device specific way.
 */
- (void)restorePlaneAndSegmentSettings;

/*
 * Put the display into SVGA mode. This typically happens
 * when the window server starts running. This method is implemented by
 * subclasses in a device specific way.
 */
- (void)enterSVGAMode;

/*
 * Get the device out of whatever advanced mode it was using and back
 * into a state where it can be used as a standard VGA device. This method
 * is implemented by subclasses in a device specific way.
 */
- (void)revertToVGAMode;

/*
 * Look up the physical memory location for this device instance and map
 * it into VM for use by the device driver. If problems occur, the method
 * returns (vm_address_t)0. If `addr' is not 0, then it is used as the
 * physical memory address and `length' is used as the length.
 */
- (vm_address_t)mapFrameBufferAtPhysicalAddress:(unsigned int)addr
 	length:(int)length;

/*
 * Choose a mode from the list `modeList' (containing `count' modes) based
 * on the value of the `DisplayMode' key in the device's config table.  If
 * `isValid' is nonzero, each element specifies whether or not the
 * corresponding mode is valid.
 */
- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count
	valid:(const BOOL *)isValid;

/*
 * Equivalent to the above with `isValid' set to zero.
 */
- (int)selectMode:(const IODisplayInfo *)modeList count:(int)count;

/*
 * IODevice methods reimplemented by this class.
 */

+ (BOOL)probe:deviceDescription;

- initFromDeviceDescription:deviceDescription;

- (IOReturn)getIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned *)count;	// in/out

- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count;

/*
 * `evScreen' protocol methods reimplemented by this class.
 */

- hideCursor: (int)token;

- moveCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

- showCursor:(Point*)cursorLoc frame:(int)frame token:(int)t;

/*
 * NOTE: Subclasses must override setBrightness and implement appropriately.
 */
- setBrightness:(int)level token:(int)t;

@end

