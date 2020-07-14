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
 * @revision	1997.02.18  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Apple96SCSI.h - "Main" public methods for the 53C96 PCI SCSI interface.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Hmm, these methods are probably common to all SCSI implementations.
 *
 * Edit History
 * 1998.07.29   MM	Disable logRegisters on command timeout.
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.11.12	MM	Added support for scatter-gather lists.
 */
#import "Apple96SCSI.h"
#import "Apple96SCSIPrivate.h"
#import "Apple96CurioPrivate.h"
#import "Apple96CurioPublic.h"
#import "Apple96CurioDBDMA.h"
#import "Apple96Hardware.h"
#import "Apple96HWPrivate.h"
#import "Apple96ISR.h"
#import "bringup.h"
#import <mach/message.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/align.h>
#import <driverkit/kernelDriver.h>
#import <kernserv/prototypes.h>
#import <driverkit/IOSimpleMemoryDescriptor.h>
/* string.h defines bzero */
#import <string.h>


@implementation Apple96_SCSI

/*
 * Create and initialize one instance of Apple96_SCSI. The work is done by
 * architecture- and chip-specific modules. 
 */
+ (Boolean)		probe	: deviceDescription
{
		Apple96_SCSI		*inst = [self alloc];
		Boolean				result;

		ENTRY("Spr probe");
		MakeTimestampRecord(512);		/* TBS: contitionally compile */
		/*
		 * Perform device-specific initialization -- free the instance
		 * on failure.
		 */
		if ([inst hardwareInitialization : deviceDescription] == nil) {
			result = NO;
		}
		else {
			result = YES;
		}
		RESULT(result);
		return (result);
}

- free
{
		CommandBuffer		cmdBuf;
		
		ENTRY("Sfr free");
		/*
		 * First kill the I/O thread if running. 
		 */
		if (gFlagIOThreadRunning) {
			cmdBuf.op = kCommandAbortRequest;
			cmdBuf.scsiReq = NULL;
			[self executeCmdBuf:&cmdBuf];
		}	
		if (gIncomingCommandLock != NULL) {
			[gIncomingCommandLock free];
		}
		[self dbdmaTerminate];	/* Free up DBDMA memory	*/
		EXIT();
		return [super free];
}


/*
 * Return required DMA alignment for current architecture.
 */
- (void)getDMAAlignment : (IODMAAlignment *)alignment;
{
    alignment->readStart   = DBDMA_ReadStartAlignment;
    alignment->writeStart  = DBDMA_WriteStartAlignment;
    alignment->readLength  = DBDMA_ReadLengthAlignment;
    alignment->writeLength = DBDMA_WriteLengthAlignment;
}

/*
 * Statistics support.
 */
- (unsigned int) numQueueSamples
{
    return (gTotalCommands);
}


- (unsigned int) sumQueueLengths
{
    return (gQueueLenTotal);
}


- (unsigned int) maxQueueLength
{
    return (gMaxQueueLen);
}


- (void)resetStats
{
    gTotalCommands = 0;
    gQueueLenTotal = 0;
    gMaxQueueLen   = 0;
}

/**
 * Do a SCSI command, as specified by an IOSCSIRequest. All the 
 * work is done by the I/O thread.
 * @param   scsiReq	The request to execute
 * @param   buffer	The data buffer to transfer to/from, if any
 * @param   client	The data buffer owner task (for VM munging) 
 *
 * This method is called from IOSCSIDevice
 */
- (sc_status_t) executeRequest
					: (IOSCSIRequest *) scsiReq
	    buffer  : (void *) buffer 
	    client  : (vm_task_t) client
{
	IOMemoryDescriptor  *mem = NULL;
	sc_status_t	scsiStatus = SR_IOST_GOOD;  /* Fool compiler */

	ENTRY("Ser executeRequest");
	/*
	 * Create a simple I/O memory descriptor for this client,
	 * then toss it to the common method.
	 */
	if (buffer != NULL) {
            mem = [[IOSimpleMemoryDescriptor alloc]
                        initWithAddress : (void *) buffer
                        length          : scsiReq->maxTransfer
                    ];
            [mem setClient : client];
	}
	scsiStatus = [self executeRequest
			: scsiReq
		ioMemoryDescriptor : mem
	    ];
	if (mem != NULL) {
	    [mem release];
	}
	RESULT(scsiReq->bytesTransferred);
	return (scsiStatus);
}

/*
 * Execute a SCSI request using an IOMemoryDescriptor. This allows callers to
 * provide (kernel-resident) logical scatter-gather lists. For compatibility
 * with existing implementations, the low-level SCSI device driver must first
 * ensure that executeRequest:ioMemoryDescriptor is supported by executing:
 *  [controller respondsToSelector : executeRequest:ioMemoryDescriptor]
 */
- (sc_status_t) executeRequest
		    : (IOSCSIRequest *) scsiReq 
	ioMemoryDescriptor  : (IOMemoryDescriptor *) ioMemoryDescriptor 
{
		CommandBuffer		commandBuffer;

	ENTRY("Sem executeRequest (IOMemoryDescriptor)");
		TAG("Ser", scsiReq->maxTransfer);
		ddmExported("executeRequest: cmdBuf 0x%x maxTransfer 0x%x\n", 
			&commandBuffer,
			scsiReq->maxTransfer,
			3,4,5
		);
	if (ioMemoryDescriptor != NULL) {
	    [ioMemoryDescriptor setMaxSegmentCount : kMaxDMATransfer];
	    [ioMemoryDescriptor state : &commandBuffer.savedDataState];
	}
		bzero(&commandBuffer, sizeof(CommandBuffer));
		commandBuffer.op			= kCommandExecute;
		commandBuffer.scsiReq		= scsiReq;
	commandBuffer.mem	= ioMemoryDescriptor;
	scsiReq->driverStatus	    = SR_IOST_INVALID;	/* "In progress"    */
		[self executeCmdBuf : &commandBuffer];
		ddmExported("executeRequest: cmdBuf 0x%x complete; driverStatus %s\n", 
			&commandBuffer,
			IOFindNameForValue(scsiReq->driverStatus, IOScStatusStrings),
			3,4,5
		);
		RESULT(commandBuffer.scsiReq->bytesTransferred);
#if TIMESTAMP_AT_IOCOMPLETE
		[self logTimestamp : "I/O complete"];	/* After RETURN macro */
#endif
		return (commandBuffer.scsiReq->driverStatus);
}

/**
 *  Reset the SCSI bus. All the work is done by the I/O thread.
 */
- (sc_status_t) resetSCSIBus
{
		CommandBuffer		commandBuffer;

		ENTRY("Sre resetSCSIBus");
		ddmExported("resetSCSIBus: cmdBuf 0x%x\n",
			&commandBuffer,
			2, 3,4,5
		);

		commandBuffer.op			= kCommandResetBus;
		commandBuffer.scsiReq		= NULL;

		[self executeCmdBuf:&commandBuffer];
		ddmExported("resetSCSIBus: cmdBuf 0x%x DONE\n",
			&commandBuffer,
			2, 3,4,5
		);
		RESULT(SR_IOST_GOOD);
	return (SR_IOST_GOOD);		/* can not fail */
}

/*
 * The following 6 methods are all called from the I/O thread in IODirectDevice. 
 */
 
/*
 * Called from the I/O thread when it receives an interrupt message.
 * Currently all work is done by chip-specific module; maybe we should 
 * put this method there....
 */
- (void) interruptOccurred
{
		ENTRY("Sin interruptOccurred");
		/*
		 * Note: the following compiles because ENTRY ends with "do {"
		 */
#if DDM_DEBUG
    {
		/*
		 * calculate interrupt service time if enabled.
		 */
		ns_time_t			startTime;
		unsigned			elapsedNSec = 0;

		if (IODDMMasks[APPLE96_DDM_INDEX] & DDM_INTR) {
			IOGetTimestamp(&startTime);
		}
		ddmInterrupt("interruptOccurred: TOP\n", 1,2,3,4,5);
#endif	DDM_DEBUG
		[self hardwareInterrupt];
#if DDM_DEBUG
		if (IODDMMasks[APPLE96_DDM_INDEX] & DDM_INTR) {
			ns_time_t	endTime;
			
			IOGetTimestamp(&endTime);
			elapsedNSec = (unsigned) (endTime - startTime);
		}
		ddmInterrupt("interruptOccurred: DONE; elapsed time %d.%03d us\n", 
			elapsedNSec / 1000U,
			elapsedNSec - ((elapsedNSec / 1000U) * 1000U),
			3,4,5
		);
    }
#endif	DDM_DEBUG
//		[self enableInterrupt:0];
//		[self enableAllInterrupts ];
		EXIT();
}

/*
 * These three should not occur; they are here as error traps. All three are 
 * called out from the I/O thread upon receipt of messages which it should
 * not be seeing.
 */
- (void)interruptOccurredAt:(int)localNum
{
		IOLog("%s: interruptOccurredAt:%d\n", [self name], localNum);
}

- (void)otherOccurred:(int)id
{
		IOLog("%s: otherOccurred:%d\n", [self name], id);
}

- (void)receiveMsg
{
		IOLog("%s: receiveMsg\n", [self name]);
		
		/*
		 * We have to let IODirectDevice take care of this (i.e., dequeue the
		 * bogus message).
		 */
		[super receiveMsg];
}

/*
 * Used in -timeoutOccurred to determine if specified cmdBuf has timed out.
 * Returns YES if timeout, else NO.
 */
static inline Boolean
isCmdTimedOut(
		CommandBuffer		*cmdBuf,
		ns_time_t			now
    )
{
		IOSCSIRequest		*scsiReq;
		ns_time_t			expire;
		Boolean				result;

		ENTRY("Sit isCmdTimedOut");
		scsiReq = cmdBuf->scsiReq;
	expire	= cmdBuf->startTime + 
			(1000000000ULL * (unsigned long long) scsiReq->timeoutLength);
		result = (now > expire);
#if 0
		if (result) {
			IOLog("%s: command timeout, target %d exceeded %d\n"
		"drvApple96SCSI",   /* Can't use [self name] here */
				(int) scsiReq->target,
				(int) scsiReq->timeoutLength
			);
		}
#endif
		RESULT(result);
		return (result);
}

/*
 * Called from the I/O thread when it receives a timeout
 * message. We send these messages ourself from serviceTimeoutInterrupt().
 */
- (void) timeoutOccurred
{
		ns_time_t			now;
		Boolean				cmdTimedOut = NO;
		CommandBuffer		*cmdBuf = gActiveCommand;
		CommandBuffer		*nextCmdBuf;

		ENTRY("Sto timeoutOccurred");
		ddmThread("timeoutOccurred: TOP\n", 1,2,3,4,5);
		IOGetTimestamp(&now);
#if 0 /* Disable. Part of Radar 2261237 */
		[self logRegisters : TRUE  reason : "TimeoutOccurred"];
#endif /* Disable. Part of Radar 2261237 */
// ** ** **
// ** ** ** Umm, is there a race-condition here? Can gActiveCommand be changed
// ** ** ** out from under us by an interrupt or does the O.S. ensure that
// ** ** ** these methods are single-threaded? Also, note that the bus
// ** ** ** is likely to be stuck if there is a very slow device
// ** ** ** such as a Format or Tape Rewind ongoing. This needs to be
// ** ** ** updated to use the Copland (and/or SCSI Manager 4.3) algorithms.
// ** ** **
		/*
		 *  Scan gActiveCommand and disconnectQ looking for tardy I/Os.
		 */
		if (cmdBuf != NULL) {
			if (isCmdTimedOut(cmdBuf, now)) {
				ddmThread("gActiveCommand TIMEOUT, cmd 0x%x\n", 
					cmdBuf, 2,3,4,5);
				gActiveCommand = NULL;
				/*
				 * ** ** ** Umm, what about abort commands?
				 */
				ASSERT(cmdBuf->scsiReq != NULL);
		[self ioComplete
			    : cmdBuf
		    finalStatus : SR_IOST_IOTO
		];
				cmdTimedOut = YES;
			}
		}
		/*
		 * ** ** **
		 * ** ** ** Umm, disconnected commands should timeout only
		 * ** ** ** when the bus is idle. Otherwise, we can timeout
		 * ** ** ** a command whose only fault is that other commands
		 * ** ** ** saturated the bus.
		 */
		cmdBuf = (CommandBuffer *) queue_first(&gDisconnectedCommandQueue);
		while (queue_end(&gDisconnectedCommandQueue, (queue_entry_t) cmdBuf) == 0) {
			if (isCmdTimedOut(cmdBuf, now)) {
				ddmThread("disconnected cmd timeout, cmd 0x%x\n",  cmdBuf, 2,3,4,5);
			
				/*
				 *  Remove cmdBuf from disconnectQ and complete it.
				 */
				nextCmdBuf = (CommandBuffer *) queue_next(&cmdBuf->link);
				queue_remove(&gDisconnectedCommandQueue, cmdBuf, CommandBuffer *, link);
				ASSERT(cmdBuf->scsiReq != NULL);
		[self ioComplete
			    : cmdBuf
		    finalStatus : SR_IOST_IOTO
		];
				cmdBuf = nextCmdBuf;
				cmdTimedOut = YES;
			}
			else {
				cmdBuf = (CommandBuffer *) queue_next(&cmdBuf->link);
			}
		}
		/*
		 * ** ** **
		 * ** ** ** This is a really big hammer -- and will cause problems
		 * ** ** ** with tape drives. It may also cause data loss.
		 * ** ** **
		 * Reset bus. This also completes all I/Os in disconnectQ with
		 * status CS_Reset.
		 */
		if (cmdTimedOut) {
			[self logRegisters : TRUE reason : "Command timeout"];
			[self threadResetBus:NULL];
		}
		ddmThread("timeoutOccurred: DONE\n", 1,2,3,4,5);
		EXIT();
}

/*
 * Process all commands in gIncomingCommandQueue. At most one of these will become
 * gActiveCommand. The remainder of kCommandExecute commands go to gPendingCommandQueue. Other
 * types of commands (such as bus reset) are executed immediately.
 *
 * This method is called from IODirectDevice.
 * ** ** **
 * ** ** ** Note that we don't have a concept of frozen queue.
 * ** ** **
 */
- (void) commandRequestOccurred
{
		CommandBuffer		*cmdBuf;
		CommandBuffer		*pendCmd;

		ENTRY("Sco commandRequestOccurred");
		ddmThread("commandRequestOccurred: top\n", 1,2,3,4,5);
		[gIncomingCommandLock lock];
		while (queue_empty(&gIncomingCommandQueue) == FALSE) {
			cmdBuf = (CommandBuffer *) queue_first(&gIncomingCommandQueue);
			queue_remove(&gIncomingCommandQueue, cmdBuf, CommandBuffer *, link);
			[gIncomingCommandLock unlock];
			switch (cmdBuf->op) {
			case kCommandResetBus:
			   	/* 
				 * Note all active and disconnected commands will
				 * be terminted.
				 */
				[self threadResetBus : "Reset Command Received"];
		[self ioComplete
			    : cmdBuf
		    finalStatus : SR_IOST_RESET
		];
				break;
			case kCommandAbortRequest:
				/*
				 * 1. Abort all active, pending, and disconnected
		 *  commands.
				 * 2. Notify caller of completion.
				 * 3. Self-terminate.
				 */
		[self abortAllCommands : SR_IOST_INT];
				pendCmd = (CommandBuffer *) queue_first(&gPendingCommandQueue);
				while (queue_end(&gPendingCommandQueue, (queue_entry_t) pendCmd) == FALSE) {
		    [self ioComplete
				: pendCmd
			finalStatus : SR_IOST_INT
		    ];
					pendCmd = (CommandBuffer *) queue_next(&pendCmd->link);
				}
				[cmdBuf->cmdLock lock];
				[cmdBuf->cmdLock unlockWith:CMD_COMPLETE];
				ddmEntry("-Method: commandRequestOccurred", 1, 2, 3, 4, 5);
				IOExitThread();
				/* not reached */
			case kCommandExecute:
				[self threadExecuteRequest:cmdBuf];
				break;
				
			}
			[gIncomingCommandLock lock];
		}
		[gIncomingCommandLock unlock];
		ddmThread("commandRequestOccurred: DONE\n", 1,2,3,4,5);
		EXIT();
}


/*
 * Power management methods. All we care about is power off, when we must 
 * reset the SCSI bus due to the Compaq BIOS's lack of a SCSI reset, which
 * causes a hang if we have set up targets for sync data transfer mode.
 */
- (IOReturn) getPowerState : (PMPowerState *) state_p
{
	 	ddmExported("getPowerState called\n", 1,2,3,4,5);
	   	return IO_R_UNSUPPORTED;
}

- (IOReturn) setPowerState : (PMPowerState) state
{
#ifdef DEBUG
		IOLog("%s: received setPowerState: with %x\n", [self name],
			(unsigned)state);
#endif DEBUG
		if (state == PM_OFF) {
			// [self scsiReset];
			// ** ** ** TBS: [self powerDown];
			return IO_R_SUCCESS;
		}
		return IO_R_UNSUPPORTED;
}

- (IOReturn) getPowerManagement : (PMPowerManagementState *) state_p
{
		ddmExported("getPowerManagement called\n", 1,2,3,4,5);
		return IO_R_UNSUPPORTED;
}

- (IOReturn) setPowerManagement : (PMPowerManagementState) state
{
		ddmExported("setPowerManagement called\n", 1,2,3,4,5);
		return IO_R_UNSUPPORTED;
}

#if APPLE96_ENABLE_GET_SET

/**
 * Called by clients (and user-interaction) to configure the device.
 */
- (IOReturn) setIntValues
						: (unsigned *)		parameterArray
	forParameter	: (IOParameterName) parameterName
	count		: (unsigned int)    count
{
		IOReturn		ioReturn = IO_R_INVALID_ARG;
		
		/* Radar 1670762 ENTRY("Ssi setIntValues"); */
		ddmExported("setIntValues %s, count %u\n",
				parameterName,
				count,
				3, 4, 5
			);
		if (strcmp(parameterName, APPLE96_AUTOSENSE) == 0) {
			if (count == 1) {
				gOptionAutoSenseEnable = (parameterArray[0] ? 1 : 0);
				IOLog("%s: autoSense %s\n", [self name], 
					(gOptionAutoSenseEnable ? "Enabled" : "Disabled"));
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_ENABLE_DISCONNECT) == 0) { /* Radar 2005639 */
			if (count == 1) {
				gOptionDisconnectEnable = (parameterArray[0] ? 1 : 0);
				IOLog("%s: disconnect %s\n", [self name], 
					(gOptionDisconnectEnable ? "Enabled" : "Disabled"));
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_CMD_QUEUE) == 0) {
			if (count == 1) {
				gOptionCmdQueueEnable = (parameterArray[0] ? 1 : 0);
				IOLog("%s: cmdQueue %s\n", [self name], 
					(gOptionCmdQueueEnable ? "Enabled" : "Disabled"));
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_SYNC) == 0) {
			if (count == 1) {
				gOptionSyncModeEnable = (parameterArray[0] ? 1 : 0);
				IOLog("%s: syncMode %s\n", [self name], 
					(gOptionSyncModeEnable ? "Enabled" : "Disabled"));
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_FAST_SCSI) == 0) {
			if (count == 1) {
				gOptionFastModeEnable = (parameterArray[0] ? 1 : 0);
				IOLog("%s: fastMode %s\n", [self name], 
					(gOptionFastModeEnable ? "Enabled" : "Disabled"));
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_RESET_TARGETS) == 0) {
			if (count == 0) {
				int					target;
			
				/*
				 * Re-enable sync and command queueing. The
				 * disable bits persist after a reset.
				 */
				for (target = 0; target < SCSI_NTARGETS; target++) {
					PerTargetData		*perTargetPtr;
		
					perTargetPtr = &gPerTargetData[target];
		    perTargetPtr->cmdQueueDisable   = FALSE;
		    perTargetPtr->selectATNDisable  = FALSE;
					perTargetPtr->maxQueue			= 0;
				}
				IOLog("%s: Per Target disable flags cleared\n", [self name]);
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_RESET_TIMESTAMP) == 0) {
			ResetTimestampIndex();
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_ENABLE_TIMESTAMP) == 0) {
			EnableTimestamp(TRUE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_DISABLE_TIMESTAMP) == 0) {
			EnableTimestamp(FALSE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_PRESERVE_FIRST_TIMESTAMP) == 0) {
			PreserveTimestamp(TRUE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_PRESERVE_LAST_TIMESTAMP) == 0) {
			PreserveTimestamp(FALSE);
			ioReturn = IO_R_SUCCESS;
		}
		else {
			ioReturn = [super setIntValues
									: parameterArray
		    forParameter    : parameterName
					count			: count
				];
		}
		/* Radar 1670762 RESULT(ioReturn); */
		return (ioReturn);
}

/**
 * Called by clients and user-interaction to retrieve information from
 * the device. Note that non-privileged clients can call getIntValues
 * and it may be used to *set* timestamp parameters.
 */
- (IOReturn) getIntValues
							: (unsigned *)		parameterArray
	forParameter	    : (IOParameterName) parameterName
		count				: (unsigned *)		count;		/* in/out	*/
{
		IOReturn			ioReturn = IO_R_INVALID_ARG;
		
		/* Radar 1670762 ENTRY("Sgi getIntValues"); */
		if (strcmp(parameterName, APPLE96_AUTOSENSE) == 0) {
			if (*count == 1) {
				parameterArray[0] = gOptionAutoSenseEnable;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_CMD_QUEUE) == 0) {
			if (*count == 1) {
				parameterArray[0] = gOptionCmdQueueEnable;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_SYNC) == 0) {
			if (*count == 1) {
				parameterArray[0] = gOptionSyncModeEnable;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_FAST_SCSI) == 0) {
			if (*count == 1) {
				parameterArray[0] = gOptionFastModeEnable;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_ENABLE_DISCONNECT) == 0) { /* Radar 2005639 */
			if (*count == 1) {
				parameterArray[0] = gOptionDisconnectEnable;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE_MAX_THREADS) == 0) { /* Radar 2005639 */
			if (*count == 1) {
				parameterArray[0] = MAX_CLIENT_THREADS;
				ioReturn = IO_R_SUCCESS;
			}
		}
		else if (strcmp(parameterName, APPLE96_RESET_TIMESTAMP) == 0) {
			ResetTimestampIndex();
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_ENABLE_TIMESTAMP) == 0) {
			EnableTimestamp(TRUE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_DISABLE_TIMESTAMP) == 0) {
			EnableTimestamp(FALSE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_PRESERVE_FIRST_TIMESTAMP) == 0) {
			PreserveTimestamp(TRUE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_PRESERVE_LAST_TIMESTAMP) == 0) {
			PreserveTimestamp(FALSE);
			ioReturn = IO_R_SUCCESS;
		}
		else if (strcmp(parameterName, APPLE96_STORE_TIMESTAMP) == 0) {
			/*
			 * Count
			 *		1: tag, <zero>, current time
			 *		2: tag, value, current time
			 *		4: tag, value, user's time
			 */
			switch (*count) {
			case 1:
				StoreTimestamp(parameterArray[0], 0);
				ioReturn = IO_R_SUCCESS;
				break;
			case 2:
				StoreTimestamp(parameterArray[0], parameterArray[1]);
				ioReturn = IO_R_SUCCESS;
				break;
			case 4:
				StoreNSecTimestamp(
					parameterArray[0],
					parameterArray[1],
					*((ns_time_t *) &parameterArray[2])
				);
				ioReturn = IO_R_SUCCESS;
				break;
			default:
				break;	/* Failure */
			}
		}
		else if (strcmp(parameterName, APPLE96_READ_TIMESTAMP) == 0) {
			/*
			 * Read the timestamp array:
			 *	parameterArray	-> result buffer (TimestampDataRecord vector)
			 *	count			-> maximum number of integers to return
			 *					<- set to the actual number of integers returned.
			 * Note that, because count is a number of integers, clients should
			 * call this as follows:
			 *		TimestampDataRecord		myTimestamps[123];
			 *		unsigned				count;
			 *		count = sizeof (myTimestamps) / sizeof (unsigned);
			 *		[scsiDevice getIntValues
			 *						: (unsigned int *) myTimestamps
			 *			forParameter	: APPLE96_READ_TIMESTAMP
			 *			count			: &count
			 *		];
			 *		count = (count * sizeof (unsigned)) / sizeof (TimestampDataRecord);
			 *		for (i = 0; i < count; i++) {
			 *			Process(myTimestamps[i]);
			 *		}
			 */
			unsigned	vectorSize = ((*count) * sizeof(unsigned))
								/ sizeof (TimestampDataRecord);
			ReadTimestampVector((TimestampDataPtr) parameterArray, &vectorSize);
			*count = (vectorSize * sizeof (TimestampDataRecord))
								/ sizeof (unsigned);
			ioReturn = IO_R_SUCCESS;
		}
		else {
			ioReturn = [super getIntValues : parameterArray
				forParameter : parameterName
				count : count];
		}
		/* RESULT(ioReturn); */
		return (ioReturn);
}					


#endif	/* APPLE96_ENABLE_GET_SET */

@end	/* Apple96_SCSI */

