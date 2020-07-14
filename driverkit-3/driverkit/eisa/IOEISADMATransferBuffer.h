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
 * Copyright 1994 NeXT Computer, Inc.
 * All rights reserved.
 *
 * HISTORY
 * 23-Feb-94    Curtis Galloway at NeXT
 *      Created. 
 */

#import <driverkit/IOBuffer.h>
#import <driverkit/i386/directDevice.h>

@interface IOEISADMATransferBuffer : IOBuffer
{
@private
	id				directDevice;
	id				deviceDescription;
	unsigned int			localChannel;
	unsigned int			realChannel;
	IOPhysicalRange			currentTransfer;
	void				*dma_xfer_buf;
	IODMADirection			transferDirection;
}

-setChannel:(unsigned int)localChannel
 forDevice:device;

-(IODMADirection)transferDirection;
-(void)setTransferDirection:(IODMADirection)dir;

-(IOReturn)startTransfer;
-(IOReturn)startTransferAt:(IORange)transferRange;
-(IOReturn)abortTransfer;

-(BOOL)transferIsComplete;

-(unsigned)currentTransferLocation;
-(unsigned)currentTransferCount;

-(IOReturn)enableChannel;
-(void)disableChannel;

/*
 * Reserve and release exclusive DMA lock. Use is optional; provides
 * exclusion between mutually incompatible DMA devices.
 */
- (void)reserveDMALock;
- (void)releaseDMALock;

-(IODMATransferMode)transferMode;
-(IOReturn)setTransferMode:(IODMATransferMode)mode;

-(IOEISADMATransferWidth)transferWidth;
-(IOReturn)setTransferWidth:(IOEISADMATransferWidth)width;

-(IOEISADMATiming)transferTiming;
-(IOReturn)setTransferTiming:(IOEISADMATiming)timing;

-(IOEISAStopRegisterMode)stopRegisterMode;
-(IOReturn)setStopRegisterMode:(IOEISAStopRegisterMode)mode;

-(IOIncrementMode)transferIncrementMode;
-(IOReturn)setTransferIncrementMode:(IOIncrementMode)mode;

-(BOOL)autoinitializeMode;
-(IOReturn)setAutoinitializeMode:(BOOL)mode;

-(BOOL)isValidForDMA;
-(unsigned int)maximumTransferLengthAtOffset:(unsigned int)offset;


@end
