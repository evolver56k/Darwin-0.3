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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/* 	Copyright (c) 1991-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * IdeCntDma.h - Interface for Dma category of IDE Controller device class.
 *
 * HISTORY
 */
 
#ifdef	DRIVER_PRIVATE

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODevice.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/generalFuncs.h>
#import "IdeCnt.h"
#import "IdeCntCmds.h"

@interface IdeController(Dma)

/*
 * Returns TRUE iff IDE DMA is allowed on this hardware. 
 */

/*
 * Called once during initialization to figure out if we can really do 
 * DMA transfers. Returns dma channel number.
 */
- (ide_return_t) initIdeDma:(unsigned int)unit; 
- (ide_return_t) allocDmaMemory;

/*
 * Called once during probe by IdeController Class to figure out  if the
 * hardware supports DMA (as opposed to the IDE drive supporting DMA). 
 */

- (ide_return_t) ideDmaRwCommon: (ideIoReq_t *)ideIoReq;

- (ide_return_t) ideReadWriteDma: (ideRegsVal_t *)ideRegs fRead:(BOOL)read;

-(ide_return_t) setupDMA:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead;
-(ide_return_t) setupDBDMA:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead;
-(ide_return_t) setupDMAList:(vm_offset_t) startAddr client:(vm_task_t) client length:(int)length fRead:(BOOL)fRead; 

- (uint) stopDBDMA;
- (uint) getDBDMATransferCount;
- (void) fixOHareDMACorruption_1;
- (void) fixOHareDMACorruption_2;

@end

#endif DRIVER_PRIVATE
