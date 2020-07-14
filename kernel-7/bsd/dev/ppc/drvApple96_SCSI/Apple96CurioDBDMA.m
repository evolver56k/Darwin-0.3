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
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96PCIDBDMA.h - Minimal DBDMA Handler for the Apple 96 PCI driver. This is
 * a temporary file until "real" DBDMA support appears.
 *
 * Edit History
 * 1997.02.13	MM  Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.07.18	MM  Added USE_CURIO_METHODS and wrote macros for the
 *	    one-liner methods.
 */
#import "Apple96SCSI.h"
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import "Apple96CurioDBDMA.h"
#import <mach/mach_interface.h>

/**
 * These methods implement a minimal DBDMA support library for Curio.
 */
@implementation Apple96_SCSI(Curio_DBDMA)

/**
 * Quit: stop the DBDMA controller and free all memory.
 * Never done as a macro.
 */
- (void) dbdmaTerminate
{
		if (gDBDMALogicalAddress != NULL) {
	    DBDMAreset();
		}
		if (gChannelCommandArea != NULL) {
			IOFree(gChannelCommandArea, gChannelCommandAreaSize);
			gChannelCommandArea = NULL;
	}   
}

#if USE_CURIO_METHODS

/**
 * Stop the DBDMA controller
 */
- (void) dbdmaReset
{
	__DBDMAreset();
}

/**
 * Start a DMA operation.
 */
- (void) dbdmaStartTransfer
{
	__DBDMAstartTransfer(); 
}

/**
 * Stop a DMA operation.
 */
- (void) dbdmaStopTransfer
{
	__DBDMAstopTransfer();
}

/**
 * Spin-wait until the DBDMA channel is inactive. This should be
 * timed to prevent deadlock.
 */
- (void) dbdmaSpinUntilIdle
{
	ENTRY("dbd dbdmaSpinUntilIdle");
	__DBDMAspinUntilIdle();
	EXIT();
}
#endif /* USE_CURIO_METHODS */

@end	/* Apple96_SCSI(Curio_DBDMA) */
