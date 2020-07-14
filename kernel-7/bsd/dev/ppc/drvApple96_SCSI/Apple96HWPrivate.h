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

/**
 * Copyright (c) 1994-1996 NeXT Software, Inc.  All rights reserved. 
 * Copyright  1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * 
 * AppleMeshHardwarePrivate.h - Architecture-specific methods for Apple Mesh SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"

/*
 * These macros are used to access words (32 bit) and bytes (8 bit) in the channel
 * command area. They may be used as source or destination. CCLDescriptor is
 * aligned to a descriptor start, CCLAddress is just an address pointer.
 */
#define CCLAddress(offset)		(((UInt8 *) gChannelCommandArea) + (offset))
#define CCLDescriptor(offset)	((DBDMADescriptor *) CCLAddress(offset))
#define CCLWord(offset)			(*((UInt32 *) CCLAddress(offset)))
#define CCLByte(offset)			(*((UInt8  *) CCLAddress(offset)))

@interface Apple96_SCSI (HardwarePrivate)

/**
 * Perform one-time-only memory allocation.
 * @return  IO_R_SUCCESS    If successful.
 *          IO_R_NO_MEMORY  Can't allocate memory (fatal)
 */
- (IOReturn) hardwareAllocateHardwareAndChannelMemory
						: deviceDescription;

/**
 * When a (legitimate) data phase starts, this method is called to configure
 * the DBDMA Channel Command list. Autosense is simple (as we "cannot" be
 * called more than once), while ordinary data transfers are arbitrarily complex.
 */
- (IOReturn) hardwareInitializeCCL;

/**
 * Initialize the data transfer channel command list for a normal SCSI command.
 * This is not an optimal implementation, but it simplifies maintaining a
 * common code base with the NuBus Curio/AMIC hardware interface.
 * Note that the last DBDMA command must be INPUT_LAST or OUTPUT_LAST to handle
 * synchronous transfer odd-byte disconnect. 
 */
- (IOReturn) hardwareInitializeRequestCCL;
 
/**
 * Start a request after the CCL has been initialized
 * by either hardwareInitializeSCSIRequest or
 * hardwareInitializeAutosense.
 */
- (void)    hardwareStartSCSIRequest;

/**
 * Clear the volatile results field in the channel command list.
 * Don't touch the permanent data.
 */
- (void) clearChannelCommandResults;

/**
 * Perform one-time-only channel command program.
 */
- (void) initializeChannelProgram;

/**
 * Debug log channel command area
 */
- (void) logChannelCommandArea
            : (const char *) reason;

/**
 * Debug log the current IOMemoryDescriptor
 */
- (void)            logIOMemoryDescriptor
            : (const char *) info;

@end

