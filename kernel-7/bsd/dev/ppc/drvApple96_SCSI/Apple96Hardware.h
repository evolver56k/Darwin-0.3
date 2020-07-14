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
 * Apple96PCISCSI.h - Architecture-specific methods for Apple96 SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"

/*
 * Return values from -hardwareStart.
 */
typedef enum {
	kHardwareStartOK,			/* command started successfully			*/
	kHardwareStartRejected,		/* command rejected, try another		*/
	kHardwareStartBusy			/* hardware not ready for command		*/
} HardwareStartResult;

@interface Apple96_SCSI(Hardware)

/**
 * Perform one-time-only architecture-specific init.
 * Return self on success, nil on failure.
 */
- hardwareInitialization
						: deviceDescription;



/**
 * Reusable hardware initization function. This includes a SCSI reset.
 * Handling of ioComplete of active and disconnected commands must be done
 * elsewhere. Returns IO_R_SUCCESS if successful. This is called
 * from a Task thread. It will disable and re-enable interrupts.
 * Reason is for error logging.
 */
- (IOReturn)	hardwareReset
						: (Boolean) resetSCSIBus
			reason		: (const char *) reason;

/**
 * Initialize volatile target options at start and
 * after bus reset. This initializes synchronous transfer
 * It does not reset disable command queue or disable select
 * with ATN states.
 */
- (void)	initializePerTargetData;
/**
 * Initialize the synchronous data transfer state for a target.
 */
- (void)	renegotiateSyncMode
						: (UInt8) target;

/*
 * Start a SCSI transaction for the command in cmdBuf. gActiveCommand must be 
 * NULL. A return of kHardwareStartRejected indicates that caller may try
 * again with another command; kHardwareStartBusy indicates a condition
 * other than (gActiveCommand != NULL) which prevents the processing of
 * the command. The command will have been enqueued on gPendingCommandQueue
 * in the latter case. The command will have been ioComplete'd in the
 * kHardwareStartRejected case.
 */
- (HardwareStartResult) hardwareStart
						: (CommandBuffer *) cmdBuf;

@end /* Apple96_SCSI(Hardware) */

