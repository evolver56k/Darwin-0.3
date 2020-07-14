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
 * @author  Martin Minow    mailto:minow@apple.com
 * @revision	1997.02.13  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Private structures and definitions for Apple 53C96 SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"

@interface Apple96_SCSI(Private)

/*
 * Send a command to the controller thread, and wait for its completion.
 * Only invoked by publicly exported methods in SCSIController.m.
 */
- (IOReturn) executeCmdBuf
						: (CommandBuffer *) cmdBuf;

/*
 * Abort all active and disconnected commands with specified status. No 
 * hardware action. Used by threadResetBus and during processing
 * of a kCommandAbortRequest command.
 */
- (void)    abortAllCommands	: (sc_status_t) status;

/*
 * I/O thread version of resetSCSIBus and executeRequest.
 */
- (void)    threadResetBus
						: (const char *) reason;
						

- (void)    threadExecuteRequest
						: (CommandBuffer *) cmdBuf;

/*
 * Methods called by other modules in this driver. 
 */
 
/*
 * Called when a transaction associated with cmdBuf is complete. Notify 
 * waiting thread. If cmdBuf->scsiReq exists (i.e., this is not a reset
 * or an abort), scsiReq->driverStatus must be valid. If cmdBuf is activeCmd,
 * caller must remove from activeCmd.
 *
 * If the request status is SR_IOST_INVALID (the initial value), it will
 * be set to finalStatus.
 */
- (void)    ioComplete
	    : (CommandBuffer *) cmdBuf
	finalStatus : (sc_status_t) finalStatus;

/**
 * A target reported a full queue. Push this command back on the pending
 * queue and try it again, later. (Return SR_IOST_GOOD if successful,
 * SR_IOST_BADST on failure.
 */
- (sc_status_t) pushbackFullTargetQueue
						: (CommandBuffer *) cmdBuf;

/**
 * A command couldn't be issued (because a target is trying to reselect
 * us or we lost arbitration for some other reason). Push this request
 * onto the front of the pending request queue.
 */
- (void) pushbackCurrentRequest
						: (CommandBuffer *) cmdBuf;

/**
 * A command can't be continued. Perhaps there is no target.
 */
// - (void) killCurrentRequest;

/*
 * I/O associated with activeCmd has disconnected. Place it on disconnectQ
 * and enable another transaction.
 */ 
- (void)    disconnect;

/*
 * The current target,lun is trying to reselect. If we have a CommandBuffer
 * for this TL nexus and it is not tagged, remove it, make it the current
 * gActiveCommand, and return IO_R_SUCCESS. If the first disconnected
 * command is tagged, we need to read the queue tag message, return
 * IO_R_INTERNAL to signal this (it is not an error). If there is no
 * disconnected command, return IO_R_HW to initiate error recovery.
 */
- (IOReturn)	reselectNexusWithoutTag;

/*
 * The current target, lun, and specified queueTag is trying to reselect.
 * If we have  a CommandBuffer for this TLQ nexus on disconnectQ, remove it,
 * make it the current activeCmd, and return IO_R_SUCCESS. Else return IO_R_HW.
 */
- (IOReturn)	reselectNexusWithTag
					: (unsigned char) queueTag;

/*
 * Determine if gActiveArray[][], maxQueue[][], cmdQueueEnable, and a 
 * command's target and lun show that it's OK to start processing cmdBuf.
 * Returns YES if the command can be started.
 */
- (Boolean) commandCanBeStarted
						: (CommandBuffer *) cmdBuf;
    
/*
 * The bus has gone free. Start up commands in pendingQ, if any.
 */
- (void)    busFree;

/**
 * Choose the next request that can be started.
 */
- (CommandBuffer *) selectNextRequest;

/*
 * Abort activeCmd (if any) and any disconnected I/Os (if any) and reset 
 * the bus due to gross hardware failure.
 * If activeCmd is valid, its scsiReq->driverStatus will be set to 'status'.
 */
- (void)    killActiveCommandAndResetBus
						: (sc_status_t) status
    reason	: (const char *) reason;

- (void)    killActiveCommand
						: (sc_status_t) status;
/*
 * Called by chip level to indicate that a command has gone out to the 
 * hardware.
 */
- (void)    activateCommand
						: (CommandBuffer *) cmdBuf;

/*
 * Remove the current command from "active" status. Update activeArray,
 * activeCount, and unschedule pending timer.
 */
- (void)    deactivateCmd;

/**
 * Kill everything in the indicated queue. Called after bus reset.
 */
- (void) killQueue  : (queue_head_t *)	queuePtr
	finalStatus : (sc_status_t)	scsiStatus;


@end

