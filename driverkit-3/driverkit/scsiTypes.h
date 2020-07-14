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
/*      Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *	Copyright 1997, 1997 Apple Computer Inc. All rights reserved.
 *
 * scsiTypes.h - Exported API of SCSIController class.
 *
 * HISTORY
 * 13-Nov-1997	Martin Minow at Apple
 *	Added IOSCSIControllerExportedWithIOMemoryDescriptor
 * 14-Jun-95	Doug Mitchell at NeXT
 *	Added SCSI-3 support.
 * 11-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <objc/objc.h>
#import <driverkit/driverTypes.h>
#import <driverkit/scsiRequest.h>
#import <driverkit/IOMemoryDescriptor.h>

/*
 * Buffers aligned to IO_SCSI_DMA_ALIGNMENT (both start and end addresses) are
 * guaranteed to be legal.
 */
#ifndef IO_SCSI_DMA_ALIGNMENT
#define IO_SCSI_DMA_ALIGNMENT      64
#endif

/*
 * Exported protocol for SCSIController object.
 */
@protocol IOSCSIControllerExported

- releaseAllUnitsForOwner: owner;

/*
 * Attempt to reserve specified target and lun for calling device. Returns
 * non-zero if device already reserved.
 */
- (int)reserveTarget: (unsigned char) target
                 lun: (unsigned char) lun
            forOwner: owner;

- (void)releaseTarget: (unsigned char) target
                  lun: (unsigned char) lun
             forOwner: owner;

/*
 * SCSI-3 versions of same.
 */
- (int)reserveSCSI3Target: (unsigned long long) target
                      lun: (unsigned long long) lun
                 forOwner: owner;

- (void)releaseSCSI3Target: (unsigned long long) target
                       lun: (unsigned long long) lun
                  forOwner: owner;

/*
 * Returns the number of SCSI targets supported.
 * FIXME - should this return an unsigned long long?
 */
- (int) numberOfTargets;

/*
 * Standard I/O methods.
 *
 * executeRequest requires buffers aligned to IO_SCSI_DMA_ALIGNMENT.
 */
- (sc_status_t) executeRequest: (IOSCSIRequest *) scsiReq
                        buffer: (void *) buffer 	/* data destination */
                        client: (vm_task_t) client;

/*
 * executeRequest (with an IOMemoryDescriptor) requires buffers aligned to
 * IO_SCSI_DMA_ALIGNMENT. The client identification is contained in the
 * IOMemoryDescriptor object
 */
- (sc_status_t) executeRequest: (IOSCSIRequest *) scsiReq
            ioMemoryDescriptor: (IOMemoryDescriptor *) ioMemoryDescriptor;

/*
 * SCSI-3 version, allows 64-bit target/lun and 16-byte CDB.
 */
- (sc_status_t) executeSCSI3Request: (IOSCSI3Request *) scsiReq
                             buffer: (void *) buffer 	/* data destination */
                             client: (vm_task_t) client;

/*
 * executeRequest (with an IOMemoryDescriptor) requires buffers aligned to
 * IO_SCSI_DMA_ALIGNMENT. The client identification is contained in the
 * IOMemoryDescriptor object
 */
- (sc_status_t) executeSCSI3Request: (IOSCSI3Request *) scsiReq
                 ioMemoryDescriptor: (IOMemoryDescriptor *) ioMemoryDescriptor;

- (sc_status_t) resetSCSIBus;			/* reset the bus */

/*
 * Convert an sc_status_t to an IOReturn.
 */
- (IOReturn) returnFromScStatus: (sc_status_t) sc_status;

/*
 * Determine maximum DMA which can be peformed in a single call to 
 * executeRequest:buffer:client:.
 */
- (unsigned) maxTransfer;

/*
 * Return required DMA alignment for current architecture.
 */
- (void) getDMAAlignment: (IODMAAlignment *)alignment;

/*
 * Allocate some well-aligned memory.
 * Usage:
 * 
 * void *freePtr;
 * void *alignedPtr;
 * unsigned freeCnt;
 * 
 * alignedPtr = [controllerId allocateBufferOfLength:someLength
 *			actualStart:&freePtr
 *			actualLength:&freeCnt];
 * ...
 * when done...
 *
 * IOFree(freePtr, freeCnt);
 */
- (void *) allocateBufferOfLength: (unsigned) length
                     actualStart : (void **) actualStart
                    actualLength : (unsigned *) actualLength;


@end

/*
 * Public IONamedValue arrays.
 */
extern IONamedValue IOScStatusStrings[];
extern IONamedValue IOSCSISenseStrings[];
extern IONamedValue IOSCSIOpcodeStrings[];

