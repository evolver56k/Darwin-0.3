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
 * Copyright (c) 1994-1996 NeXT Software, Inc.	All rights reserved. 
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author  Martin Minow    mailto:minow@apple.com
 * @revision	1997.02.18  Initial conversion from AMDPCSCSIDriver sources.
 *
 * Apple96SCSIPrivate.m - "Main" private methods for the 53C96 PCI SCSI interface.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Hmm, these methods are probably common to all Apple SCSI implementations.
 *
 * Edit History
 * 1997.02.13	MM	Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96SCSI.h"
#import "Apple96SCSIPrivate.h"
#import "Apple96CurioPublic.h"
#import "Apple96Hardware.h"
#import "bringup.h"
#import "MacSCSICommand.h"
#import <mach/message.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/align.h>
#import <driverkit/kernelDriver.h>
#import <kernserv/prototypes.h>

/* 
 * Template for command message sent to the I/O thread.
 */
static const msg_header_t cmdMessageTemplate = {
    0,		    		/* msg_unused		*/
    1,		    		/* msg_simple		*/
    sizeof(msg_header_t),	/* msg_size		*/ 
    MSG_TYPE_NORMAL,	 	/* msg_type		*/
    PORT_NULL,			/* msg_local_port	*/
    PORT_NULL,			/* msg_remote_port - filled in	*/
    IO_COMMAND_MSG		/* msg_id		*/
};

/*
 * Template for timeout message.
 */
static const msg_header_t timeoutMsgTemplate = {
    0,		   		 /* msg_unused		*/
    1,		    		/* msg_simple		*/
    sizeof(msg_header_t),	/* msg_size		*/ 
    MSG_TYPE_NORMAL,		/* msg_type		*/
    PORT_NULL,			/* msg_local_port	*/
    PORT_NULL,			/* msg_remote_port - filled in	*/
    IO_TIMEOUT_MSG		/* msg_id		*/
};

static void		    serviceTimeoutInterrupt(
	void			*arg
    );

@implementation Apple96_SCSI(Private)

/*
 * Private chip- and architecture-independent methods.
 */
/*
 * Pass one CommandBuffer to the I/O thread; wait for completion.
 * (We are called on the client's execution thread.)
 * Normal completion status is in cmdBuf->scsiReq->driverStatus; 
 * a non-zero return from this function indicates a Mach IPC error.
 *
 * This method allocates and frees cmdBuf->cmdLock.
 */
- (IOReturn)	executeCmdBuf
			: (CommandBuffer *) cmdBuf
{
	msg_header_t	    msg		    = cmdMessageTemplate;
	kern_return_t	    kernelReturn;
	IOReturn	    ioReturn	    = IO_R_SUCCESS;

	ENTRY("Sex executeCmdBuf");
	cmdBuf->flagActive = 0;
	cmdBuf->cmdLock = [[NXConditionLock alloc] initWith:CMD_PENDING];
	[gIncomingCommandLock lock];
	queue_enter(&gIncomingCommandQueue, cmdBuf, CommandBuffer *, link);
	[gIncomingCommandLock unlock];

	/*
	 * Create a Mach message and send it in order to wake up the 
	 * I/O thread.
	 */
	msg.msg_remote_port = gKernelInterruptPort;
	kernelReturn = msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
	if (kernelReturn != KERN_SUCCESS) {
	    IOLog("%s: msg_send_from_kernel() error status %d\n", 
		[self name],
		kernelReturn
	    );
	    ioReturn = IO_R_IPC_FAILURE;
	    goto exit;
	}

	/*
	 * Wait for I/O complete.
	 */
	ddmExported("executeCmdBuf: waiting for completion on cmdBuf 0x%x\n",
	    cmdBuf, 2,3,4,5);
	[cmdBuf->cmdLock lockWhen:CMD_COMPLETE];
exit:	RESULT(ioReturn);	/* Log the completion before clearing the lock */
	[cmdBuf->cmdLock free];
	return (ioReturn);
}

/*
 * Abort all active and disconnected commands with specified status. No 
 * hardware action. Currently used by threadResetBus and during processing
 * of a kCommandAbortRequest command. 
 */
- (void) abortAllCommands : (sc_status_t) status
{
	ENTRY("Sab abortAllCommands");
	ddmThread("abortAllCommands\n", 1,2,3,4,5);
	[gIncomingCommandLock lock];
	[self killActiveCommand : status];
	[self killQueue  : &gDisconnectedCommandQueue   finalStatus : status];
	[self killQueue  : &gPendingCommandQueue	finalStatus : status];
	[self killQueue  : &gIncomingCommandQueue	finalStatus : status];
#ifdef	DEBUG
	/*
	 * activeArray "should be" empty...if not, make sure it is for debug.
	 */
	do {
	    int		target;
	    int		lun;
	    int		active;
	    
	    for (target = 0; target < SCSI_NTARGETS; target++) {
		for (lun = 0; lun < SCSI_NLUNS; lun++) {
		    active = gActiveArray[target][lun];
		    if (active > 0) {
			IOLog("%s: (abortAllCommands) gActiveArray[%d][%d] = %d, should be zero\n",
			    [self name],
			    target, lun, active
			);
			gActiveCount -= active;
			gActiveArray[target][lun] = 0;
		    }
		}
	    }
	    if (gActiveCount != 0) {
		IOLog("%s: (abortAllCommands) activeCount = %d, should be zero\n",
		    [self name],
		    gActiveCount
		);
		gActiveCount = 0;
	    }
	} while (0);
#endif	DEBUG
	[gIncomingCommandLock unlock];
	EXIT();
}

/*
 * Abort all active and disconnected commands with status SR_IOST_RESET.
 * Reset hardware and SCSI bus. If there is a command in gPendingCommandQueue,
 * start it up.
 */
- (void)    threadResetBus
			: (const char *) reason
{
	ENTRY("Str threadResetBus");
	[self abortAllCommands : SR_IOST_RESET];
	[self hardwareReset : TRUE	reason : reason];   /* Reset SCSI and chip  */
	[self busFree];					/* Restart commands	*/
	EXIT();
}

/*
 * Commence processing of the specified command. This is called by
 * commandRequestOccurred when it receives a kCommandExecute message
 * from IODirectDevice. There is a new SCSI request. Either start
 * it now, or add it to the end of our pending request queue.
 */
- (void)    threadExecuteRequest
			: (CommandBuffer *) cmdBuf
{
	ENTRY("Stx threadExecuteRequest");
	if (gActiveCommand != NULL) {
	    /*
	     * We are currently executing a request.
	     */
	    ddmThread("threadExecuteRequest: ACTIVE; deferring 0x%x\n",
		cmdBuf,
		2,3,4,5
	    );
	    queue_enter(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
	}	
	else if ([self commandCanBeStarted:cmdBuf] == NO) {
	    /*
	     * This request can't be started right now (perhaps the
	     * target's tagged command limit has been reached).
	     */
	    ddmThread("threadExecuteRequest: !commandCanBeStarted; deferring 0x%x\n",
		cmdBuf,
		2,3,4,5
	    );
	    queue_enter(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
	}
	else {
	    /*
	     * Apparently, we can start this request. Call the hardware layer.
	     */
	    ddmThread("calling hardwareStart: cmdBuf 0x%x activeArray[%d][%d] = %d\n",
		cmdBuf,
		cmdBuf->scsiReq->target,
		cmdBuf->scsiReq->lun,
		gActiveArray[cmdBuf->scsiReq->target][cmdBuf->scsiReq->lun],
		5
	    );	
	    switch ([self hardwareStart : cmdBuf]) {
	    case kHardwareStartBusy:	    	/* Hardware can't start now	*/
		ddmThread("threadExecuteRequest: hardware busy\n", 1, 2, 3, 4, 5);
		break;
	    case kHardwareStartOK:	    	/* Command started correctly	*/
		break;
	    case kHardwareStartRejected:	/* Command rejected, try another */
		ddmThread("threadExecuteRequest: calling busFree\n", 1,2,3,4,5);
		[self busFree];		    	/* Try another command		*/
	    }
	    ddmThread("threadExecuteRequest(0x%x): DONE\n", cmdBuf, 2,3,4,5);
	}
	EXIT();
}

/*
 * Methods called by hardware-dependent modules.
 */

#if TEST_QUEUE_FULL
int testQueueFull; /* Set in the debugger to return queue full status */
#endif	TEST_QUEUE_FULL

/*
 * Called when a transaction associated with cmdBuf is complete. Notify 
 * waiting thread. If cmdBuf->scsiReq exists (i.e., this is not a reset
 * or an abort), scsiReq->driverStatus must be valid. If cmdBuf is active,
 * caller must remove from gActiveCommand. We decrement activeArray[][] counter
 * if appropriate.
 */
- (void) ioComplete
	    : (CommandBuffer *) cmdBuf
	finalStatus : (sc_status_t) finalStatus
{
	ns_time_t		currentTime;
	IOSCSIRequest		*scsiReq;
	
	ENTRY("Sic ioComplete");
	ASSERT(cmdBuf != NULL);
	if (cmdBuf == gActiveCommand) {
	    [self deactivateCmd];
	}
	scsiReq = cmdBuf->scsiReq;
	if (scsiReq != NULL) {
	    if (scsiReq->driverStatus == SR_IOST_INVALID) {
		scsiReq->driverStatus = finalStatus;
	    }
	    IOGetTimestamp(&currentTime);
	    scsiReq->totalTime = currentTime - cmdBuf->startTime;
	    scsiReq->bytesTransferred = cmdBuf->currentDataIndex;
#if SERIOUS_DEBUGGING || LOG_COMMAND_AND_RESULT
	    IOLog("%s: ioComplete %d.%d: Transferred %d bytes, status %d: %s\n",
		    [self name],
		    scsiReq->target,
		    scsiReq->lun,
		    scsiReq->bytesTransferred,
		    scsiReq->driverStatus,
		    IOFindNameForValue(scsiReq->driverStatus, IOScStatusStrings)
		);
#endif /* SERIOUS_DEBUGGING || LOG_COMMAND_AND_RESULT */
#if (SERIOUS_DEBUGGING || DUMP_USER_BUFFER) && __IO_MEMORY_DESCRIPTOR_DEBUG__
	    if (cmdBuf->mem != NULL) {
		IOMemoryDescriptorState         state;
		LogicalRange                    range;
		unsigned int                    newPosition = 0;
    
		[cmdbuf->mem state : &state];
		[cmdbuf->mem setPosition : 0];
		while (newPosition < scsiReq->bytesTransferred
		    && [cmdbuf->mem getLogicalRange : 1
			    maxByteCount	    : scsiReq->bytesTransferred - newPosition
			    newPosition		    : &newPosition
			    actualRanges	    : NULL
			    logicalRanges	    : &range] != 0) {
		    [self logMemory	    : range.address
			    length	    : range.length
			    reason	    : "User buffer at I/O Complete"
		    ];
		}
		[cmdbuf->mem setState : &state];
	    }
#endif /* (SERIOUS_DEBUGGING || DUMP_USER_BUFFER) && __IO_MEMORY_DESCRIPTOR_DEBUG__ */
	    /*
	     * Catch bad SCSI status now.
	     */
	    if (scsiReq->driverStatus == SR_IOST_GOOD) {
#if TEST_QUEUE_FULL
		if (testQueueFull
		 && (activeArray[scsiReq->target][scsiReq->lun] > 1)) {
		    scsiReq->scsiStatus = STAT_QUEUE_FULL;
		    testQueueFull = 0;
		}
#endif  TEST_QUEUE_FULL
		if (cmdBuf->flagIsAutosense) {
		    /*
		     * We are completing an autosense command. Don't touch
		     * the request status (it should still be Check Condition).
		     * Queue full is a real problem.
		     */
		    ASSERT(scsiReq->scsiStatus == kScsiStatusCheckCondition);
		    switch (cmdBuf->autosenseStatus) {
		    case kScsiStatusGood:
			bcopy(gAutosenseArea, &scsiReq->senseData, sizeof (esense_reply_t));
#if SERIOUS_DEBUGGING || DUMP_USER_BUFFER
			[self logMemory		: &scsiReq->senseData
				length		: sizeof (esense_reply_t)
				reason		: "Autosense at I/O completion"
			];
#endif /* SERIOUS_DEBUGGING || DUMP_USER_BUFFER */
			scsiReq->driverStatus = SR_IOST_CHKSV;
			break;
		    case kScsiStatusQueueFull:
			if ([self pushbackFullTargetQueue : cmdBuf] == SR_IOST_GOOD) {
			    ddmThread("ioComplete: queue full after autosense\n", 1, 2, 3, 4, 5);
			    goto exit;
			}
			/* Fall through to failure */
		    default:
			scsiReq->driverStatus = SR_IOST_CHKSNV;
			break;
		    }
		}
		else { /* Normal command: not autosense */
		    switch (scsiReq->scsiStatus) {
		    case kScsiStatusGood:
	
			/* If this was an Inquiry, peek at the data	*/
			/* for Synchronous and Tagged Queuing support:	*/
	
			if ( (*(UInt8*)&scsiReq->cdb == kScsiCmdInquiry)
			 &&  (cmdBuf->currentDataIndex > 7)
			 &&  (cmdBuf->mem != NULL) ) {
			    IOMemoryDescriptorState             state;
			    UInt8				byte;
			    unsigned int                        count;
			    
			    [cmdBuf->mem state : &state];
			    [cmdBuf->mem setPosition : 7];
			    count = [cmdBuf->mem readFromClient : &byte count : 1];
			    [cmdBuf->mem setState : &state];
			    if (count == 1) {
				gPerTargetData[ scsiReq->target ].inquiry_7 = byte;
				if ( (byte & 0x02) == 0) {
				    gPerTargetData[ scsiReq->target ].cmdQueueDisable = TRUE;
				}
			    }
			} /* end if successful inquiry completion */
			break;
		    case kScsiStatusCheckCondition:
			/*
			 * ** ** ** The 386 SIM supresses autosense for Test
			 * ** ** ** Unit Ready to avoid request sense when polling
			 * ** ** ** for removable devices. This should be the caller's
			 * ** ** ** decision.
			 */
			if ( gOptionAutoSenseEnable			/* Supress for debugging	*/
			 && (scsiReq->ignoreChkcond == FALSE) ) {
			    cmdBuf->flagIsAutosense = 1;    /* We're doing autosense	*/
			    queue_enter_first(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
			    ddmThread("ioComplete: queuing for autosense\n", 1, 2, 3, 4, 5);
			    goto exit;
			}
			else {
			    /*
			     * This command failed and we aren't doing autosense.
			     */			
			    scsiReq->driverStatus = SR_IOST_CHKSNV;
			}
			break;
		    case kScsiStatusQueueFull:
			if ([self pushbackFullTargetQueue : cmdBuf] == SR_IOST_GOOD) {
			    /*
			     * ioComplete target tagged, queue full
			     */
			    ddmThread("ioComplete: target tagged, queue full\n", 1, 2, 3, 4, 5);
			    goto exit;
			}
			/* Huh? we weren't doing tagged queuing, fall through */
		    default:
			scsiReq->driverStatus = SR_IOST_BADST;
			break;
		    } /* switch */
		} /* If not autosense */
	    } /* if scsiReq->driverStatus == SR_IOST_GOOD on entrance */
	} /* if (scsiReq != NULL) */
#if DDM_DEBUG
	do {
	    const char		*status;
	    unsigned		moved;
	    
	    if (scsiReq != NULL) {
		status = IOFindNameForValue(scsiReq->driverStatus, 
		    IOScStatusStrings);
		moved = scsiReq->bytesTransferred;
	    }
	    else {
		status = "Complete";
		moved = 0;
	    }
	    ddmThread("ioComplete: cmdBuf 0x%x status %s bytesXfr 0x%x\n", 
		cmdBuf, status, moved,4,5);
	} while (0);
#if LOG_RESULT_ON_ERROR
	if (scsiReq != NULL && scsiReq->driverStatus != SR_IOST_GOOD) {
	    [self logCommand
			    	: cmdBuf
		logMemory	: TRUE
		reason		: "Unsuccessful result"
	    ];
	}
#endif /* LOG_RESULT_ON_ERROR */
#endif	DDM_DEBUG
	[cmdBuf->cmdLock lock];
	[cmdBuf->cmdLock unlockWith:YES];
exit:	; /* This semicolon is needed to prevent errors in the EXIT macro */
	EXIT();
}

/**
 * A target reported a full queue. Push this command back on the pending
 * queue and try it again, later. (Return SR_IOST_GOOD if successful,
 * SR_IOST_BADST on failure.
 */
- (sc_status_t) pushbackFullTargetQueue
			    : (CommandBuffer *) cmdBuf
{
	IOSCSIRequest		*scsiReq;
	int			target;
	int			lun;
	IOReturn		ioReturn;

	ENTRY("Spf pushbackFullTargetQueue");
	ASSERT(cmdBuf != NULL && cmdBuf->scsiReq != NULL);
	ASSERT(cmdBuf != gActiveCommand);
	/*
	 * Avoid notifying client of this condition; update
	 * perTarget.maxQueue and place this request on gPendingCommandQueue.
	 * We'll try this again when we ioComplete at least one
	 * command in this target's queue.
	 ** ** ** Note that this can execute commands out of order.
	 ** ** ** This can be disasterous for directory commands.
	 ** ** ** In the long run, the client (disk/tape/whatever)
	 ** ** ** needs to tell us how to execute the command
	 ** ** ** (in-order, out-of-order, etc.) For example,
	 ** ** ** virtual-memory page faults can be executed
	 ** ** ** out of order, but directory and volume bitmap
	 ** ** ** updates must be executed in-order to preserve
	 ** ** ** volume integrity.
	 */
	if (cmdBuf->queueTag == QUEUE_TAG_NONTAGGED) {
	    /*
	     * Huh? We're not doing command queueing...
	     */
	    ioReturn = SR_IOST_BADST;
	}
	else {
	    scsiReq	= cmdBuf->scsiReq;
	    target	= scsiReq->target;
	    lun		= scsiReq->lun;
	    gPerTargetData[target].maxQueue = gActiveArray[target][lun];
	    ddmThread("Target %d queue full, maxQueue set to %d\n",
		target,
		gPerTargetData[target].maxQueue,
		3,4,5
	    );
	    [self pushbackCurrentRequest : cmdBuf];
	    ioReturn = SR_IOST_GOOD;
	}
	RESULT(ioReturn);
	return (ioReturn);
}

/**
 * Push this request back on the pending queue.
 */
- (void) pushbackCurrentRequest
		: (CommandBuffer *) cmdBuf
{
	ENTRY("Spc pushbackCurrentRequest");
	if (gActiveCommand != NULL) {
	    [self deactivateCmd];
	}
	queue_enter_first(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
	EXIT();
}

#if 0
/**
 * Kill a request that can't be continued.
 */
- (void) killCurrentRequest
{
	ENTRY("Skc killCurrentRequest");
	if (gActiveCommand != NULL) {
	    if (gActiveCommand->scsiReq != NULL
	     && gActiveCommand->scsiReq->driverStatus == SR_IOST_INVALID) {
		if (gCurrentBusPhase == kBusPhaseBusFree) {
	    gActiveCommand->scsiReq->driverStatus = SR_IOST_SELTO;  /* No such device	*/
		}
		else {
	    gActiveCommand->scsiReq->driverStatus = SR_IOST_HW;	    /* Target went away */
		}
	    }
	    [self ioComplete
				: gActiveCommand
		finalStatus	: SR_IOST_INVALID
	    ];
	}
	EXIT();
}
#endif

/*
 * I/O associated with gActiveCommand has disconnected. Place it on the
 * disconnected command queue and enable another transaction.
 */ 
- (void) disconnect
{
	ENTRY("Sdi disconnect");
	ASSERT(gActiveCommand != NULL);
	ddmThread("DISCONNECT: cmdBuf 0x%x target %d lun %d tag %d\n",
	    gActiveCommand, gActiveCommand->scsiReq->target,
	    gActiveCommand->scsiReq->lun, gActiveCommand->queueTag, 5);
	queue_enter(
	    &gDisconnectedCommandQueue,
	    gActiveCommand,
	    CommandBuffer *,
	    link
	);
#if DDM_DEBUG
	if ((gActiveCommand->currentDataIndex != gActiveCommand->scsiReq->maxTransfer)
	 && (gActiveCommand->currentDataIndex != 0)) {
	    ddmThread("disconnect after partial DMA (max 0x%d curr 0x%x)\n",
		gActiveCommand->scsiReq->maxTransfer, 
		gActiveCommand->currentDataIndex, 3,4,5);
	}
#endif	DDM_DEBUG
	/*
	 * Record this time so that gActiveCommand can be billed for
	 * disconnect latency at reselect time.
	 */
	IOGetTimestamp(&gActiveCommand->disconnectTime);
	gActiveCommand	    = NULL;
	gCurrentTarget	    = kInvalidTarget;
	gCurrentLUN	    = kInvalidLUN;
	/*
	 * Since there is no active command, the caller must configure the
	 * bus interface to wait for bus free, then allow reselection.
	 */
	EXIT();
}

/*
 * Specified target, lun is trying to reselect. If we have a CommandBuffer
 * for this TL nexus and it is not tagged, remove it, make it the current
 * gActiveCommand, and return IO_R_SUCCESS. If the first disconnected
 * command is tagged, we need to read the queue tag message, return
 * IO_R_INTERNAL to signal this (it is not an error).
 */
- (IOReturn)	reselectNexusWithoutTag
{
	CommandBuffer	    *cmdBuf;
	IOSCSIRequest	    *scsiReq;
	ns_time_t	    currentTime;
	IOReturn	    ioReturn	    = SR_IOST_HW;   /* Presume failure	*/

	ENTRY("Srn reselectNexusWithoutTag");
	ASSERT(gActiveCommand == NULL);
	cmdBuf = (CommandBuffer *) queue_first(&gDisconnectedCommandQueue);
	while (queue_end(&gDisconnectedCommandQueue, (queue_t) cmdBuf) == FALSE) {
	    scsiReq = cmdBuf->scsiReq;
	    if (scsiReq->target == gCurrentTarget
	     && scsiReq->lun == gCurrentLUN
	     && cmdBuf->queueTag == QUEUE_TAG_NONTAGGED) {
		/*
		 * We found the correct command.
		 */
		ddmThread("RESELECT: target %d.%d no tag %FOUND; cmdBuf 0x%x\n",
		    gCurrentTarget,
		    gCurrentLUN,
		    cmdBuf,
		    4, 5
		);
		queue_remove(
		    &gDisconnectedCommandQueue,
		    cmdBuf,
		    CommandBuffer *,
		    link
		);
		gActiveCommand = cmdBuf;
		/*
		 * Bill this operation for latency time.
		 */
		IOGetTimestamp(&currentTime);
		scsiReq->latentTime += 
			    (currentTime - gActiveCommand->disconnectTime);
		ioReturn = IO_R_SUCCESS;
		break;
	    }
	    /*
	     * Try next element in queue.
	     */
	    cmdBuf = (CommandBuffer *) cmdBuf->link.next;
	}
	RESULT(ioReturn);
	return (ioReturn);
}
/*
 * Specified target, lun, and queueTag is trying to reselect. If we have 
 * a CommandBuffer for this TLQ nexus on disconnectQ, remove it, make it the
 * current activeCmd, and return IO_R_SUCCESS. Else return IO_R_HW.
 */
- (IOReturn) reselectNexusWithTag
		    : (UInt8) queueTag
{
	CommandBuffer	    *cmdBuf;
	IOSCSIRequest	    *scsiReq;
	ns_time_t	    currentTime;
	IOReturn	    ioReturn	    = SR_IOST_HW;   /* Presume failure	*/

	ENTRY("Srt reselectNexusWithTag");
	ASSERT(gActiveCommand == NULL);
	ASSERT(queueTag != QUEUE_TAG_NONTAGGED);
	/*
	 * Scan the disconnected queue looking for a command for this
	 * T/L/Q nexus.
	 */
	cmdBuf = (CommandBuffer *) queue_first(&gDisconnectedCommandQueue);
	while (queue_end(&gDisconnectedCommandQueue, (queue_t) cmdBuf) == FALSE) {
	    scsiReq = cmdBuf->scsiReq;
	    if (scsiReq->target == gCurrentTarget
	     && scsiReq->lun == gCurrentLUN
	     && cmdBuf->queueTag == queueTag) {
		/*
		 * We found the correct command.
		 */
		ddmThread("RESELECT: target %d.%d tag %d FOUND; cmdBuf 0x%x\n",
		    gCurrentTarget,
		    gCurrentLUN,
		    queueTag,
		    cmdBuf,
		    5
		);
		queue_remove(
		    &gDisconnectedCommandQueue,
		    cmdBuf,
		    CommandBuffer *,
		    link
		);
		gActiveCommand = cmdBuf;
		ASSERT(scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN);
		/*
		 * Bill this operation for latency time.
		 */
		IOGetTimestamp(&currentTime);
		scsiReq->latentTime += 
			    (currentTime - gActiveCommand->disconnectTime);
		ioReturn = IO_R_SUCCESS;
		break;
	    }
	    /*
	     * Try next element in queue.
	     */
	    cmdBuf = (CommandBuffer *) cmdBuf->link.next;
	}
	if (ioReturn != IO_R_SUCCESS) {
	    /*
	     * Hmm...this is not good! We don't want to talk to this target.
	     * Perhaps it's reselecting after we have already aborted the
	     * original command request.
	    */ 
	    IOLog("%s: Unexpected reselect from target %d lun %d tag %d\n",
		[self name],
		gCurrentTarget,
		gCurrentLUN,
		queueTag
	    );
	}
	RESULT(ioReturn);
	return (ioReturn);
}

/*
 * Determine if gActiveArray[][], maxQueue, cmdQueueEnable, and a 
 * command's target and lun show that it's OK to start processing cmdBuf.
 * Returns YES if this command can be started.
 * ** ** **
 * ** ** ** Here's where we can test for a frozen LUN queue.
 * ** ** **
 */
- (Boolean) commandCanBeStarted : (CommandBuffer *) cmdBuf
{
	IOSCSIRequest	    *scsiReq;
	unsigned	    target	= scsiReq->target;
	unsigned	    lun		= scsiReq->lun;
	Boolean		    result;

	ENTRY("Scs commandCanBeStarted");
	ASSERT(cmdBuf != NULL && cmdBuf->scsiReq != NULL);
	scsiReq	    = cmdBuf->scsiReq;
	target	    = scsiReq->target;
	lun	    = scsiReq->lun;
	if (gActiveArray[target][lun] == 0) {
	    /*
	     * No commands are active for this target, always ok.
	     */
	    result = TRUE;
	}
	else if (gOptionCmdQueueEnable == FALSE
	      || gPerTargetData[target].cmdQueueDisable) {
	    /*
	     * Command queueing is disabled for this target
	     * (or disabled globally); only one command at a time
	     * can be issued.
	     */
	    result = FALSE;
	}
	else {
	    if (gPerTargetData[target].maxQueue == 0
	     || gActiveArray[target][lun] < gPerTargetData[target].maxQueue) {
		/*
		 * If maxQueueDepth is zero, we haven't reached the
		 * target's limit. Otherwise, we're under the limit. In
		 * both cases, we can (presumably) start this command.
		 * 
		 */
		result = TRUE;
	    }
	    else {
		/*
		 * We're over the target limit. Wait on this one.
		 */
		result = FALSE;
	    }	
	}
	RESULT(result);
	return (result);
}

/*
 * The bus has gone free. Start up a command from gPendingCommandQueue, if any, and
 * if allowed by cmdQueueEnable and gActiveArray[][].
 *
 * This is called from the interrupt routine when it is about to exit (and the
 * bus is free and there is no active command). It may also be called from
 * threadExecuteRequest when the selected command couldn't be started.
 * 
 */
- (void) busFree
{
	CommandBuffer	    *cmdBuf;

	ENTRY("Sbf busFree");
	cmdBuf = [self selectNextRequest];
	if (cmdBuf != NULL) {
	    [self threadExecuteRequest:cmdBuf];
	}
	EXIT();
}

/*
 * Scan through the pending queue for the first command that can be started.
 * Return the command record, or NULL if there is no pending command.
 */
- (CommandBuffer *)	selectNextRequest
{
	CommandBuffer	    *cmdBuf = NULL;
	Boolean		    foundRequest;

	ENTRY("Ssn selectNextRequest");
	ASSERT(gActiveCommand == NULL);
	foundRequest = FALSE;
	if (queue_empty(&gPendingCommandQueue) == FALSE) {
	    /*
	     * Attempt to find a CommandBuffer in gPendingCommandQueue which we are in a position
	     * to process.
	     */
	    cmdBuf = (CommandBuffer *) queue_first(&gPendingCommandQueue);
	    while (queue_end(&gPendingCommandQueue, (queue_entry_t) cmdBuf) == FALSE) {
		if ([self commandCanBeStarted:cmdBuf]) {
		    queue_remove(&gPendingCommandQueue, cmdBuf, CommandBuffer *, link);
		    ddmThread("busFree: starting pending cmd 0x%x\n", cmdBuf,
			2,3,4,5);
		    foundRequest = TRUE;
		    break;
		    /*
		     * Note that threadExecuteRequest may call busFree if the command
		     * was rejected. If so, the rejected command will have been
		     * returned (with an error status) to its client, so there is
		     * no chance of an infinite loop here.
		     */
		}
		else {
		    cmdBuf = (CommandBuffer *) queue_next(&cmdBuf->link);
		}
	    }
	}
	if (foundRequest == FALSE) {
	    cmdBuf = NULL;
	}
	RESULT(cmdBuf);
	return (cmdBuf);
}

/*
 * Abort gActiveCommand (if any) and any disconnected I/Os (if any) and reset 
 * the bus due to gross hardware failure.
 *
 * If gActiveCommand is valid, its scsiReq->driverStatus will be set to 'status'.
 * This is called when the bus state machine detects an error that it can't
 * recover from, such as a "gross error" in the chip. It is a very big hammer.
 */
- (void) killActiveCommandAndResetBus
			: (sc_status_t) status
	     reason	: (const char *) reason
{
	ENTRY("Skr killActiveCommandAndResetBus");
	[self killActiveCommand : status];
	[self logRegisters : TRUE reason : "Killing Active Command and Resetting Bus"];
	[self threadResetBus : reason];
	EXIT();
}

- (void) killActiveCommand
			: (sc_status_t) status
{
    CommandBuffer	*activeCmd;	/* -> The currently executing command	*/

	ENTRY("Ski killActiveCommand");
	if (gActiveCommand != NULL) {
	    activeCmd = gActiveCommand;
	    [self deactivateCmd];
	    [self ioComplete
				: activeCmd
		finalStatus	: status
	     ];
	}
	EXIT();
}

/*
 * Called by chip level to indicate that a command has gone out to the 
 * hardware.
 */
- (void) activateCommand
		    : (CommandBuffer *) cmdBuf
{
	ENTRY("Sac activateCommand");
	ASSERT(gActiveCommand == NULL);
	/*
	 * Start timeout timer for this I/O. The timer request is cancelled
	 * in ioComplete.
	 */
	cmdBuf->timeoutPort = gKernelInterruptPort;
#if LONG_TIMEOUT
	cmdBuf->scsiReq->timeoutLength = OUR_TIMEOUT;
#endif	LONG_TIMEOUT
	IOScheduleFunc(serviceTimeoutInterrupt, cmdBuf, cmdBuf->scsiReq->timeoutLength);

	/*
	 * This is the only place where an gActiveArray[][] counter is 
	 * incremented (and, hence, the only place where cmdBuf->active is 
	 * set). The only other place gActiveCommand is set to non-NULL is
	 * in reselectNexux... (but those methods don't increment the
	 * active command counter)
	 */
	gActiveCommand		= cmdBuf;
	gCurrentTarget		= cmdBuf->scsiReq->target;
	gCurrentLUN		= cmdBuf->scsiReq->lun;
	TAG(__tag__, (gCurrentTarget << 4) | gCurrentLUN);
	gActiveArray[gCurrentTarget][gCurrentLUN]++;
	gActiveCount++;
	cmdBuf->flagActive	    = TRUE;
	/*
	 * Accumulate statistics.
	 */
	gMaxQueueLen = MAX(gMaxQueueLen, gActiveCount);
	gQueueLenTotal += gActiveCount;
	gTotalCommands++;
	ddmThread("activateCommand: cmdBuf 0x%x target %d lun %d\n",
	    cmdBuf, gCurrentTarget, gCurrentLUN,
	    4, 5
	);
	EXIT();
}

/*
 * Remove the current command from "active" status. Update activeArray,
 * activeCount, and unschedule pending timer.
 */
- (void) deactivateCmd
{
	CommandBuffer	    *cmdBuf;
	unsigned	    target, lun;

	ENTRY("Sdc deactivateCmd");
	/*
	 * If we are pushing back a full target queue, it's possible
	 * to be called twice on the same command: once from ioComplete
	 * and once from pushbackCurrentRequest. To prevent the second
	 * call from de-referencing NULL, we exit if gActiveCommand
	 * is already NULL. Radar 2206796.
	 */
	if (gActiveCommand != NULL) {
	    ASSERT(gActiveCommand->scsiReq != NULL);
	    TAG(__tag__,
		(gActiveCommand->scsiReq->target << 4)
		| gActiveCommand->scsiReq->lun
	    );
	    ddmThread("deactivate cmd 0x%x target %d lun %d activeArray %d\n",
		gActiveCommand,
		gActiveCommand->scsiReq->target,
		gActiveCommand->scsiReq->lun,
		gActiveArray[gActiveCommand->scsiReq->target]
			[gActiveCommand->scsiReq->lun],
		5
	    );
	    cmdBuf		= gActiveCommand;
	    gActiveCommand	= NULL;
	    gCurrentTarget	= kInvalidTarget;
	    gCurrentLUN	    	= kInvalidLUN;
	    target		= cmdBuf->scsiReq->target;
	    lun			= cmdBuf->scsiReq->lun;
	    ASSERT(gActiveArray[target][lun] > 0);
	    gActiveArray[target][lun]--;
	    ASSERT(gActiveCount > 0);
	    gActiveCount--;
	    /*
	     * Cancel pending timeout request. We don't have to do this
	     * for commands that have timed out as they won't have a
	     * timeout request in the schedular.
	     */
	    if (cmdBuf->scsiReq->driverStatus != SR_IOST_IOTO) {
		IOUnscheduleFunc(serviceTimeoutInterrupt, cmdBuf);
	    }
	    cmdBuf->flagActive = FALSE;
	}
	EXIT();
}

/**
 * Kill everything in the indicated queue. Called after bus reset.
 */
- (void) killQueue  : (queue_head_t *)	queuePtr
    finalStatus : (sc_status_t) scsiStatus
{
	CommandBuffer		*cmdBuf;

	ENTRY("Skq killQueue");
	while (queue_empty(queuePtr) == FALSE) {
	    cmdBuf = (CommandBuffer *) queue_first(queuePtr);
	    queue_remove(queuePtr, cmdBuf, CommandBuffer *, link);
	    cmdBuf->scsiReq->driverStatus = scsiStatus;
	    [self ioComplete
				: cmdBuf
		finalStatus	: scsiStatus
	    ];
	}
	EXIT();
}

@end	/* Apple96_SCSI(Private) */

/*
 *  Handle timeouts.  We just send a timeout message to the I/O thread
 *  so it wakes up.
 */
static void serviceTimeoutInterrupt(void *arg)
{
	CommandBuffer	    *cmdBuf = arg;
	msg_header_t	    msg = timeoutMsgTemplate;

	ENTRY("Sto serviceTimeoutInterrupt");
	ASSERT(cmdBuf != NULL && cmdBuf->scsiReq != NULL);
	ddmError("Apple96SCSI: timeout: cmdBuf 0x%x target %d\n",
	    cmdBuf,
	    cmdBuf->scsiReq->target,
	    3,4,5
	);
	if (cmdBuf->flagActive == FALSE) {
	    /*
	     * Should never happen...
	     */
	    IOLog("Apple96SCSI: Timeout on inactive command\n");
	}
	else {
	    msg.msg_remote_port = cmdBuf->timeoutPort;
	    IOLog("Apple96SCSI: SCSI Timeout on target %d, lun %d\n",
		cmdBuf->scsiReq->target,
		cmdBuf->scsiReq->lun
	    );
	    (void) msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
	}
	EXIT();
}

