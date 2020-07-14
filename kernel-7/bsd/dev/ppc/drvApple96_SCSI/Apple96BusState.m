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
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @revision	1997.02.17	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96Chip.m - Chip-specific methods for Apple96 SCSI driver.
 *
 * Edit History
 * 1997.02.18	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.04.17	MM		Removed SCS_PHASECHANGE (not needed)
 * 1997.06.24	MM		Radar 1669736 - correctly store autosense status
 * 1997.09.10	MM		Radar 1678545 - Don't clear the gBusBusy flag until we
 *				get a "disconnected" interrupt.
 */
#import "Apple96SCSI.h"
#import "Apple96BusState.h"
#import "Apple96SCSIPrivate.h"
#import "Apple96CurioPublic.h"
#import "Apple96CurioPrivate.h"
#import "Apple96CurioDBDMA.h"
#import "Apple96HWPrivate.h"
#import "Apple96ISR.h"
#import "MacSCSICommand.h"
#import "bringup.h"
#import <driverkit/generalFuncs.h>
#import <kernserv/prototypes.h>

#define IO_R_BV	0xDEADBEEF	/* TEMP */

extern IONamedValue scsiMsgValues[];	/* In Apple96ISR.m */
	
@implementation Apple96_SCSI(BusState)
/*
 * Disconnected - only legal event here is reselection.
 * [self fsmHandleReselectionInterrupt] may set gBusState to SCS_INITIATOR
 */
- (void) fsmDisconnected
{
		UInt8				selectByte;
		UInt8				target;
		
		ENTRY("Bdi fsmDisconnected");
		ddmChip("fsmDisconnected\n", 1,2,3,4,5);
		if ((gSaveInterrupt & iReselected) != 0) {
			[self fsmHandleReselectionInterrupt];
		}
		else if ((gSaveInterrupt & (iSelected | iSelectWAtn)) != 0) {
			/*
			 * To do: allow selection by an external target and support
			 * a few mandatory commands:
			 *	(PowerUp:		gPendingTargetError = Unit Attention)
			 *	Inquiry			We're a CPU
			 *	Test Unit Ready:
			 *					if (gPendingTargetError == Unit Attention)
			 *						Check Condition
			 *					else {
			 *						Success
			 *					}
			 *	All others		gPendingTargetError := Illegal Command.
			 *					Check Condition
			 *	Request Sense:	Send current error, clear current error.
			 */
	    SynchronizeIO();
	    selectByte = CURIOgetFifoByte();
			selectByte &= ~gInitiatorIDMask;
			for (target = 0; target < 8; target++) {
				if ((selectByte & (1 << target)) != 0) {
					break;
				}
			}
			IOLog("%s: Selected by external target %d -- not supported\n",
				[self name],
				target
			);
	    CURIOdisconnect();
		}
		else if (gFlagBusBusy == FALSE) {
			/*
			 * disconnect interrupted while disconnected
			 * This always happens when we finish an I/O request.
			 * gFlagBusBusy is cleared when the "bus disconnected" bit
			 * is set in the NCR 53C96 interrupt register.
			 */
		}
		else {
			/*
			 * Since we're disconnected, just ignore the interrupt
			 */
			if (gActiveCommand != NULL
				&& gActiveCommand->scsiReq != NULL
				&& gActiveCommand->scsiReq->driverStatus == SR_IOST_INVALID) {
#if 0 /* Radar 1678545 */
				IOLog("fsmDisconnected: timeout, target %d, int %02x, status %02x, step %02x\n",
					gActiveCommand->scsiReq->target,
					gSaveInterrupt & 0xFF,
					gSaveStatus & 0xFF,
					gSaveSeqStep & 0xFF
				);
#endif 
				gActiveCommand->scsiReq->driverStatus = SR_IOST_SELTO;
			}
			if (gActiveCommand != NULL) {
		[self ioComplete
			    : gActiveCommand
		    finalStatus : SR_IOST_INVALID
		];
			}
			gCurrentBusPhase = kBusPhaseBusFree;
		}
		EXIT();
}

/*
 * This state is called when we expect to disconnect from a target after
 * receiving a Command Complete, Disconnect, or Abort message and the bus
 * is still busy on exit from the finite state automaton (because
 * curioQuickCheckForBusFree returned false).
 */ 
- (void) fsmWaitForBusFree
{
		if ((gSaveInterrupt & (iReselected | iSelected | iSelectWAtn)) != 0) {
			/*
			 * Hmm, we went straight from command completion to (presumably)
			 * reselection. Treat it as a normal selection/reselection
			 * interrupt from "disconnected" state. (This is an abuse of
			 * the finite-state automaton design.)
			 */
			[self fsmDisconnected];
		}
		else if (gFlagBusBusy == FALSE) {
			/*
			 * This is expected: the bus has just gone free after a command
			 * completed. We can now safely try to start another command.
			 */
			gBusState = SCS_DISCONNECTED;
		}
		else {
			/*
			 * This is strange. It may not be an error, but I don't know
			 * if there are other legal bus states after disconnect
			 * (the one counter example would be after a linked command
			 * when the target goes to Command phase, but we should have
			 * rejected this at command complete.
			 */
			[self fsmStartErrorRecovery
							: SR_IOST_HW
					reason	: "Unexpected interrupt when disconnecting"
			];
		}
}
	
/*
 * This is called from fsmDisconnected when we receive a reselected interrupt.
 */
- (void) fsmHandleReselectionInterrupt
{
		UInt8					selectByte;
		UInt32	 				fifoDepth;
		UInt8					msgByte;
		IOReturn				ioReturn;
		const char				*reason = NULL;

		ENTRY("Bre fsmHandleReselectionInterrupt");
		/* 
		 * Make sure there's a selection byte and an identify message in the fifo.
		 */
		ioReturn = IO_R_SUCCESS;
		gFlagBusBusy = ((gSaveInterrupt & iDisconnect) == 0);	/* Radar 1678545 */
	SynchronizeIO();
	fifoDepth = CURIOgetFifoCount();
		ddmChip("reselect: fifoDepth %d\n", fifoDepth, 2,3,4,5);
		if (fifoDepth != 2) {
			ddmError("reselection, fifoDepth %d\n", fifoDepth, 
				2,3,4,5);
#if LOG_ERROR_RECOVERY
			IOLog("%s: Bad FIFO count (%d) (expect 2) on Reselect\n",
				[self name],
				fifoDepth
			);
#endif /* LOG_ERROR_RECOVERY */
			ioReturn = SR_IOST_HW;
			reason =  "Bad FIFO count on reselect";
		}
		if (ioReturn == IO_R_SUCCESS) {
	    SynchronizeIO();
	    selectByte = CURIOgetFifoByte();
			ddmChip("reselection: select byte %02x\n", selectByte, 2, 3, 4, 5);
			ioReturn = [self getReselectionTargetID : selectByte];
			if (ioReturn != IO_R_SUCCESS) {
				reason = "Bad selection value on reselect";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			/*
			 * gCurrentTarget has the bus ID of the target that is reselecting.
			 * The first message byte must be an Identify with the LUN.
			 */
	    SynchronizeIO();
	    msgByte = CURIOgetFifoByte();
			if ((gSaveStatus & sParityErr) != 0) {
				ioReturn = SR_IOST_PARITY;
				reason = "Parity error on reselect";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			if ((msgByte & kScsiMsgIdentify) == 0) {
				ioReturn = SR_IOST_BV;
				reason = "Bad ID Message (no identify) on reselect";
			}
			gCurrentLUN = msgByte & ~kScsiMsgIdentify;
	    /* gFlagBusBusy = TRUE; -- Radar 1678545: move to method start  */
			gBusState = SCS_RESELECTING;
			/*
			 * At this point, the chip is waiting for us to validate 
			 * the identify message. While we have a target and LUN,
			 * we don't have a command to reconnect to (that's the
			 * job of fsmReselectionAction).
			 *
			 * In case of sync mode, we need to load target context right
			 * now, before dropping ACK, because the target might go
			 * straight to a data in or data out as soon as ACK drops.
			 */
	    CURIOmessageAccept();   /* Accept the identify message  */
			ddmChip("Successful reselect from %d.%d\n",
					gCurrentTarget,
					gCurrentLUN,
					3, 4, 5
				);
		}
		if (ioReturn != IO_R_SUCCESS) {
			[self fsmStartErrorRecovery
							: ioReturn
					reason	: reason
			];
		}
		EXIT();
}

/*
 * One of three things can happen here - the selection could succeed (though
 * with possible incomplete message out), it could time out, or we can be 
 * reselected.
 */
- (void) fsmSelecting
{
		IOReturn				ioReturn;
		UInt8					fifoDepth;
		CommandBuffer			*cmdBuf;
		IOSCSIRequest			*scsiReq = NULL;
		const char				*reason = NULL;

		ENTRY("Bse fsmSelecting");
		ddmChip("fsmSelecting\n", 1,2,3,4,5);
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		scsiReq = gActiveCommand->scsiReq;
		ioReturn = IO_R_SUCCESS;
		gFlagBusBusy = ((gSaveInterrupt & iDisconnect) == 0);
		if (gFlagBusBusy == FALSE) {
			/*
			 * selection timed-out. Abort this request.
			 */
			ddmChip("***SELECTION TIMEOUT for target %d.%d\n",
				scsiReq->target,
				scsiReq->lun,
				3,4,5
			);
	    CURIOflushFifo();
			gBusState = SCS_DISCONNECTED;
#if 0 /* Radar 1678545 */
			IOLog("fsmSelecting: timeout, target %d, int %02x, status %02x, step %02x\n",
				gActiveCommand->scsiReq->target,
				gSaveInterrupt & 0xFF,
				gSaveStatus & 0xFF,
				gSaveSeqStep & 0xFF
			);
#endif 
	    [self ioComplete
			: gActiveCommand
		finalStatus : SR_IOST_SELTO
	    ];
			gActiveCommand = NULL;
		}
		else if (gSaveInterrupt == (iFuncComp | iBusService)) {
			ddmChip("selection seqstep=%d\n", 
				gSaveSeqStep & INS_STATE_MASK, 2,3,4,5);
			switch (gSaveSeqStep & INS_STATE_MASK) {
	    case 0: 
				/*
				 * No message phase. If we really wanted one,
				 * this could be significant...
				 */
				if (gActiveCommand->queueTag != QUEUE_TAG_NONTAGGED) {
					/*
					 * This target can't do command queueing.
					 */
					[self disableMode : kTargetModeCmdQueue];
					/*
					 * Clear the queue tag so we don't get here on
					 * an eventual autosense.
					 */
					gActiveCommand->queueTag = QUEUE_TAG_NONTAGGED;
				}
				/*
				 * OK, let's try to continue following phase changes.
				 */
				gBusState = SCS_INITIATOR;
				break;
	    case 3: /* didn't complete cmd phase, parity? */
	    case 4: /* everything worked */
	    case 1: /* everything worked, SCMD_SELECT_ATN_STOP case */
				/* 
				 * We're connected. Start following the target's phase
				 * changes.
				 *
				 * If we're trying to do sync negotiation,
				 * this is the place to do it. In that case, we
				 * sent a SCMD_SELECT_ATN_STOP command, and
				 * ATN is now asserted (and we're hopefully in
				 * msg out phase). We want to send 5 bytes. 
				 * Drop them into currMsgOut[] and prime the  
				 * msgOutState machine.
				 */
				gBusState = SCS_INITIATOR;
				break;
	    case 2: 
				/*
				 * Either no command phase, or incomplete message
				 * transfer.
				 */
		SynchronizeIO();
		fifoDepth = CURIOgetFifoCount();
#if LOG_ERROR_RECOVERY
				IOLog("%s: Incomplete select (step 2); fifoDepth %d phase %s\n",
					[self name],
					fifoDepth, 
		    IOFindNameForValue(gSaveStatus & mPhase, scsiPhaseValues)
				);
#endif /* LOG_ERROR_RECOVERY */
				if (gActiveCommand->queueTag != QUEUE_TAG_NONTAGGED) {
					/*
					 * This target can't do command queueing.
					 */
					[self disableMode : kTargetModeCmdQueue];
				}
				/*
				 * Spec says ATN is asserted if all message bytes
				 * were not sent.
				 */
				if (fifoDepth > gActiveCommand->cdbLength) {
		    CURIOclearATN();
				}
				/*
				 * 970611: clear the fifo. If we don't do this and the
				 * target is entering MSGI phase (trying to send us a
				 * Message Reject), it will sit on top of the fifo, and
				 * we'll read garbage from the fifo.
				 */
		CURIOflushFifo();   /* The command is still in the fifo */
				/*
				 * OK, let's try to continue following phase changes.
				 */
				gBusState = SCS_INITIATOR;
				break;
			default:
				/* gFlagBusBusy = TRUE;		-- Radar 1678545 */
				ioReturn = SR_IOST_HW;
				reason = "Selection sequence Error (strange state)";
				break;
			}
		}
		else if ((gSaveInterrupt & iReselected) != 0) {
			/*
			 * We were reselected while trying to do a selection. 
			 * Enqueue this cmdBuf on the HEAD of pendingQ, then deal
			 * with the reselect. 
			 * Tricky case, we have to "deactivate" this command
	     * since this hardwareStart attempt failed.	 
			 */
			cmdBuf = gActiveCommand;
		//	[ self deactivateCmd ];		// mlj - done by pushbackCurrentRequest
			[self pushbackCurrentRequest : cmdBuf];
			ddmChip("reselect while trying to select target %d.%d\n",
					scsiReq->target,
					scsiReq->lun,
					3,4,5
				);
			gBusState = SCS_DISCONNECTED;
			/*
			 * Go deal with reselect.
			 */
			[self fsmDisconnected];
		}
		else if (gSaveInterrupt == iFuncComp
			  && (gSaveSeqStep & INS_STATE_MASK) == 2
			  && (gSaveStatus & mPhase) == kBusPhaseMSGI
			  && (gLastMsgOut[0] & kScsiMsgIdentify) == kScsiMsgIdentify
			  && (gLastMsgOut[0] & 0x07) != 0) {
				/*
				 * This shouldn't happen according to the NCR documentation,
				 * but we saw it if the bus scanner executes INQUIRY on
				 * a non-zero LUN on an Apple CD-300. The CD is sending a
				 * Message Reject to our Identify message byte. We'll
				 * handle this in the Message In code.
				 */
				gBusState = SCS_INITIATOR;
		}
		else {
			ioReturn = SR_IOST_HW;
			reason = "Bogus select/reselect interrupt";
		}
		if (ioReturn != IO_R_SUCCESS) {
			[self fsmStartErrorRecovery
							: ioReturn
					reason	: "Error selecting target"
			];
		}
		EXIT();
}

/*
 * This is a dummy interrupt service state that we enter after selection
 * when the previous Curio operation has no interrupt-service completion
 * requirement. For example, we can get here after a Message Out or
 * reselection. Nothing happens here; we continue at fsmPhaseChange.
 */
- (void) fsmInitiator
{
		ENTRY("Bin fsmInitiator");
		ddmChip("fsmInitiator\n", 1,2,3,4,5);
		gBusState = SCS_INITIATOR;
		EXIT();
}

/*
 * We just did a SCMD_INIT_CMD_CMPLT command, hopefully all that's left is
 * to drop ACK. Command Complete message is handled in fscAcceptingMsg.
 * We can't go to SCS_DISCONNECTED until the target disconnects. If we
 * go to "disconnected" state too soon, we'll encounter a load-dependent
 * race condition that causes us to start another command before we've
 * cleaned up from the last command. The actual state change is in
 * fsmProcessMessage.
 */
- (void) fsmCompleting
{
		unsigned			fifoDepth;
		IOReturn			ioReturn;
	UInt8		    statusByte;
		const char			*reason = NULL;
		
		ENTRY("Bco fsmCompleting");
		ddmChip("fsmCompleting\n", 1,2,3,4,5);
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		ioReturn = IO_R_SUCCESS;
		if ((gSaveInterrupt & iDisconnect) != 0) {
			ddmError("unexpected disconnect\n", 1,2,3,4,5);
			ioReturn = SR_IOST_HW;
			reason = "I/O complete, bus free before target completion complete";
		}
		if (ioReturn == IO_R_SUCCESS) {
	    SynchronizeIO();
	    fifoDepth = CURIOgetFifoCount();
			if ((gSaveInterrupt & iFuncComp) != 0) {
				/*
				 * Got both status and msg in fifo; ACK is still asserted.
				 */
				if (fifoDepth != 2) {
					/*
					 * This is pretty bogus - we expect a status and 
					 * msg in the fifo.
				   	 */
				   	ioReturn = SR_IOST_HW;
				   	reason = "I/O complete, incorrect fifo count (expecting two bytes)";
				}
				if (ioReturn == IO_R_SUCCESS) {
		    SynchronizeIO();
		    statusByte = CURIOgetFifoByte();
		    if (gActiveCommand->flagIsAutosense) {		/* Radar 1669736    */
			gActiveCommand->autosenseStatus = statusByte;
					}
					else {
			gActiveCommand->scsiReq->scsiStatus = statusByte;
					}
		    SynchronizeIO();
		    gMsgInBuffer[0] = CURIOgetFifoByte();
					gMsgInIndex		= 1;
					ioReturn		= [self fsmProcessMessage];
					reason			= "Message in failed";
				}
			}
			else {
				/*
				 * We only received a status byte. This can occur
				 * if we responded to the interrupt before the device
				 * successfully transmitted the Command Complete message.
				 * This is kind of weird, but let's try to handle it.
				 */
				ddmError("fsmCompleting: status only on complete\n", 
					1,2,3,4,5);
				if (fifoDepth != 1) {
					ioReturn = SR_IOST_HW;
					reason = "I/O complete: incorrect fifo count (expecting one byte)";
				}
				if (gActiveCommand->flagIsAutosense) {				/* Radar 1669736	*/
		    SynchronizeIO();
		    gActiveCommand->autosenseStatus = CURIOgetFifoByte();
				}
				else {
		    SynchronizeIO();
		    gActiveCommand->scsiReq->scsiStatus = CURIOgetFifoByte();
				}
				/*
				 * Back to watching phase changes. Presumably, the target
				 * will switch to MSGI phase and complete the command
				 * at its leasure.
				 */
				gBusState = SCS_INITIATOR;
			}
		}
		if (ioReturn != IO_R_SUCCESS) {
			[self fsmStartErrorRecovery
							: ioReturn
					reason	: reason
			];
		}
		EXIT();
}

/*
 * DMA Complete.
 */
- (void) fsmDMAComplete
{
		unsigned		bytesMoved;

		ENTRY("Bdc fsmDMAComplete");
		ddmChip("fsmDMAComplete\n", 1,2,3,4,5);
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		/*
		 * Stop the dma engine and retrieve the total number of
		 * bytes transferred. This will be
		 *	gActiveCommand->thisTransferLength	The number of bytes requested
		 *		- the current DMA residual count
		 *		- the current FIFO residual count
		 * Note that there may still be bytes in the fifo.
		 */
		bytesMoved = [self curioDMAComplete];
		if ((gSaveStatus & sParityErr) != 0) {
			[self fsmStartErrorRecovery
							: SR_IOST_PARITY
					reason	: "SCSI Data Parity Error"
			];
		}
		else {
			/*
			 * Continue following phase changes.
			 */
			gBusState = SCS_INITIATOR;
		}
		EXIT();
}

/*
 * Just completed the SCMD_TRANSFER_INFO operation for message in. ACK is
 * not asserted (we have not ACK'ed this byte). There is no parity error.
 * We will not have a command if we're reselecting.
 */
- (void) fsmGettingMsg
{
		IOReturn			ioReturn;
		const char			*reason		= NULL;
		UInt32				fifoCount;
		UInt8				msgInByte;
		
		ENTRY("Bmi fsmGettingMsg");
		ioReturn = IO_R_SUCCESS;
		if (gFlagBusBusy == FALSE) {
			ddmChip("fsmGettingMsg: message In Disconnect\n", 1,2,3,4,5);
			/*
			 * This (non-fatal) error is handled on return...
			 */
			ioReturn = IO_R_INTERNAL;	/* Any non-zero non-fatal error status	*/
		}
		else {
	    SynchronizeIO();
	    fifoCount = CURIOgetFifoCount();
			if (fifoCount != 1) {
				ddmChip("Message in fifo count error, count = %d\n",
					fifoCount,
					2, 3, 4, 5
				);
				ioReturn = SR_IOST_HW;
				reason = "Message in fifo count error";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
	    SynchronizeIO();
	    msgInByte = CURIOgetFifoByte();
			if ((gSaveStatus & sParityErr) != 0) {
				ioReturn = SR_IOST_PARITY;
				reason = "Parity error getting Command Complete message";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			if (gMsgInState == kMsgInIdle) {
				gMsgInCount = 0;
				gMsgInIndex = 0;
			}
			if (gMsgInIndex >= kMessageInBufferLength) {
				ioReturn = SR_IOST_HW;
				reason = "Message in too many bytes";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			gMsgInBuffer[gMsgInIndex++] = msgInByte;
			switch (gMsgInState) {
			case kMsgInIdle:
				/*
				 * This is the first message byte. Check for 1-byte codes.
				 */
				ddmChip("fsmGettingMsg: msgByte = 0x%x (%s)\n",
					msgInByte,
					IOFindNameForValue(msgInByte, scsiMsgValues),
					3, 4, 5
				);
				if ((/* msgInByte >= kScsiMsgOneByteMin && */ msgInByte <= kScsiMsgOneByteMax)
				 || msgInByte >= kScsiMsgIdentify) {
					gMsgInState = kMsgInReady;
				}
				else if (msgInByte >= kScsiMsgTwoByteMin
					  && msgInByte <= kScsiMsgTwoByteMax) {
					/*
					 * This is a two-byte message. Set the count and
					 * read the next byte.
					 */
		    gMsgInState = kMsgInReading;	/* Need one more    */
					gMsgInCount = 1;
				}
				else {
					/*
					 * This is an extended message. The next byte has the count.
					 */
					gMsgInState = kMsgInCounting;
				}
				break;
			case kMsgInCounting:
				/*
				 * Read the count byte of an extended message.
				 */
				gMsgInCount = msgInByte;
				gMsgInState = kMsgInReading;
				break;
			case kMsgInReading:
				if (--gMsgInCount <= 0) {
					gMsgInState = kMsgInReady;
				}
				break;
			default:
				ASSERT(gMsgInState != 0xDEADBEEF /* Bogus state */);
				ioReturn = SR_IOST_BV;
				reason = "Message in bogus state";
			} /* Switch on message state */
		}
		if (ioReturn == IO_R_SUCCESS) {
			switch (gMsgInState) {
			case kMsgInReading:
			case kMsgInCounting:
				/*
				 * We have more message bytes to read. Accept this byte
				 * (this sets ACK) and setup to transfer the next byte.
				 */
		CURIOmessageAccept();
				/*
				 * Since the message accept command sets "interrupt", eat
				 * it here so we don't get a second interrupt.
				 */
		CURIOquickCheckForChipInterrupt();
		CURIOstartMSGIAction();
				gBusState = SCS_GETTINGMSG;
				/*
				 * This would be a good place to spin-wait for
				 * completion, continuing at the start of this
				 * method if there is another byte (and we're still
				 * in message in phase). Perhaps this method should
				 * return a "spinwait" status to the mainline FSM.
				 */
				break;
			case kMsgInReady:
				gMsgInState = kMsgInIdle;
				ioReturn = [self fsmProcessMessage];
				if (ioReturn != IO_R_SUCCESS) {
					reason = "Can't process received message";
				}
				break;
			case kMsgInIdle:
			default:
				/*
				 * Hmm, that's strange: we should never be in idle state
				 * *after* successfully reading a message byte.
				 */
				ioReturn = SR_IOST_BV;
				reason = "Getting message: bogus idle state";
				break;
			}
		}
		if (ioReturn != IO_R_SUCCESS) {
			[self fsmStartErrorRecovery
							: ioReturn
					reason	: reason
			];
		}
		EXIT();
}

/*
 * Just finished a message in; ACK is false and the message has been read
 * into gMsgInBuffer[0..gMsgInIndex]. We have not ACK'ed the last byte yet.
 * If we fail here, the caller will start error recovery.
 */
- (IOReturn) fsmProcessMessage
{
		IOReturn			ioReturn			= IO_R_SUCCESS;
		const char			*reason				= "(Unknown)";
		UInt8				queueTag			= QUEUE_TAG_NONTAGGED;
		Boolean				messageAckNeeded	= TRUE;

		ENTRY("Bmp fsmProcessMessage");
		RAW_TAG(OSTag('*', "msg"), gMsgInBuffer[0]);
		/*
		 * Message in complete. Handle message(s) in currMsgIn[].
		 */
		if (gBusState == SCS_RESELECTING) {
			/*
			 * The only interesting message here is queue tag.
			 * (Hmm, what about target SDTR or Abort?)
			 */
			ASSERT(gActiveCommand == NULL);
			ASSERT(gCurrentTarget != kInvalidTarget && gCurrentLUN != kInvalidLUN);
			ASSERT(gMsgInIndex > 0);
			switch (gMsgInBuffer[0]) {
			case kScsiMsgHeadOfQueueTag:
			case kScsiMsgOrderedQueueTag:
			case kScsiMsgSimpleQueueTag:
				if (gMsgInIndex != 2) {
					ioReturn = SR_IOST_HW;
					reason = "Queue tag message: queue tag without tag value";
				}
				else {
					ioReturn = [self reselectNexusWithTag : gMsgInBuffer[1]];
					if (ioReturn != IO_R_SUCCESS) {
						reason = "Tagged queue reselection failed to locate command";
					}
				}
				break;
			default:
				ddmError("Strange message byte %02x from %d.%d while reselecting (expect tag)\n",
					gMsgInBuffer[0],
					gCurrentTarget,
					gCurrentLUN,
					4, 5
				);
				ioReturn = SR_IOST_HW;
				reason = "Reselection failed (expecting queue tag message)";
				break;				
			}
			if (ioReturn == IO_R_SUCCESS) {
				ASSERT(gActiveCommand != NULL);
				gActiveCommand->currentDataIndex = gActiveCommand->savedDataIndex;
		if (gActiveCommand->mem != NULL) {
		    [gActiveCommand->mem setState : &gActiveCommand->savedDataState];
		}
			}
			/*
			 * Since we process commands one by one, whack this command
			 * so we fall through the regular message handler.
			 */
			gMsgInBuffer[0] = kScsiMsgNop;
			gMsgInIndex = 1;
		}
		gBusState					= SCS_INITIATOR;
		gFlagNeedAnotherInterrupt	= TRUE;

		switch (gMsgInBuffer[0]) {
		case kScsiMsgNop:
			break;
		case kScsiMsgCmdComplete:
			/*
			 * Normally, we get here from fsmCommandComplete. All we need
			 * to do is to ack the message and complete the command. Exit
			 * in a transitional bus state that becomes SCS_DISCONNECTED
			 * when the bus is no longer busy. 
			 */
			gBusState = SCS_WAIT_FOR_BUS_FREE;
			/* gFlagBusBusy = FALSE;	-- Radar 1678545: wait for interrupt */
			if (gActiveCommand != NULL) {
		[self ioComplete
			    : gActiveCommand
		    finalStatus : SR_IOST_GOOD
		];
			}
			ASSERT(gActiveCommand == NULL);
			break;

		case kScsiMsgDisconnect:
			ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
			if (gActiveCommand->scsiReq->disconnect == FALSE) {
				/*
				 * This is bogus; we could do a message reject, but
				 * ignoring it is simpler and, probably better.
				 */
				ddmChip("Illegal disconnect for %d.%d\n",
					gActiveCommand->scsiReq->target,
					gActiveCommand->scsiReq->lun,
					3, 4, 5
				);
#if LOG_ERROR_RECOVERY
				IOLog("%s: Unexpected disconnect attempt from target %d.%d\n",
						[self name],
						gActiveCommand->scsiReq->target,
						gActiveCommand->scsiReq->lun
				);
#endif /* LOG_ERROR_RECOVERY */
			}
			else {
				/*
				 * Some targets fail to do a restore pointers before
				 * disconnect if all requested data has been transferred.
				 * Do an implied save pointers if this is the case.
				 */
				if ( gActiveCommand->currentDataIndex >= gActiveCommand->scsiReq->maxTransfer )
					 gActiveCommand->savedDataIndex = gActiveCommand->currentDataIndex;

				if ( gActiveCommand->mem != NULL )
					   [ gActiveCommand->mem state : &gActiveCommand->savedDataState ];
				[ self disconnect ];
			}
			/*
			 * Exit the interrupt service routine so we don't miss a reselection interrupt.
			 */
			/* gFlagBusBusy = FALSE;	-- Radar 1678545: wait for interrupt */
			gBusState = SCS_WAIT_FOR_BUS_FREE;
			gFlagCheckForAnotherInterrupt = FALSE;
			break;
		case kScsiMsgSaveDataPointers:
			ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
			gActiveCommand->savedDataIndex = gActiveCommand->currentDataIndex;
	    if (gActiveCommand->mem != NULL) {
		[gActiveCommand->mem state : &gActiveCommand->savedDataState];
	    }
			break;
		case kScsiMsgRestorePointers:
			ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
			gActiveCommand->currentDataIndex = gActiveCommand->savedDataIndex;
	    if (gActiveCommand->mem != NULL) {
		[gActiveCommand->mem setState : &gActiveCommand->savedDataState];
	    }
			break;
		case kScsiMsgRejectMsg:
			/*
			 * Hmm, the only message reject we should see would have to be
			 * a queue tag (to a device that doesn't do queuing).
			 */
			switch (gLastMsgOut[0]) {
			case kScsiMsgHeadOfQueueTag:
			case kScsiMsgOrderedQueueTag:
			case kScsiMsgSimpleQueueTag:
				[self disableMode : kTargetModeCmdQueue];
				break;
			default:
				if (gLastMsgOut[0] >= kScsiMsgIdentify
				 && (gLastMsgOut[0] & 0x07) != 0) {
				 	ddmError("Target %d.%d rejected Identify message %02x\n",
				 		gCurrentTarget,
				 		gCurrentLUN,
				 		gLastMsgOut[0],
				 		4, 5
				 	);
#if LOG_ERROR_RECOVERY
					/*
					 * The target rejected an identify message for a non-zero
					 * LUN. We treat this as a selection failure and tell the
					 * target to get off the bus.
					 */
					IOLog("%s: Target %d.%d rejected Identify message %02x\n",
						[self name],
						gCurrentTarget,
						gCurrentLUN,
						gLastMsgOut[0]
					);
#endif /* LOG_ERROR_RECOVERY */
					/*
					 * When we send an Abort to a target, we expect that it
					 * will immediately go to bus free. Unfortunately, some
					 * targets go to MSGI and send us an Abort, too.
					 */
					if (gActiveCommand != NULL) {
						if (gActiveCommand->scsiReq != NULL
			 			 && gActiveCommand->scsiReq->driverStatus == SR_IOST_INVALID) {
#if 0
			IOLog("fsmProcessMsg abort: timeout, target %d, int %02x, status %02x, step %02x\n",
				gActiveCommand->scsiReq->target,
				gSaveInterrupt & 0xFF,
				gSaveStatus & 0xFF,
				gSaveSeqStep & 0xFF
			);
#endif 
							gActiveCommand->scsiReq->driverStatus = SR_IOST_SELTO;
						}
						/* [self ioComplete : gActiveCommand];	970610	*/
					}
				}
				else {
					ddmError("%s Message rejected from target %d.%d\n",
						IOFindNameForValue(gLastMsgOut[0], scsiMsgValues),
						gCurrentTarget,
						gCurrentLUN,
						4, 5
					);
#if LOG_ERROR_RECOVERY
					IOLog("%s: %s Message Rejected, that's strange.\n",
						[self name],
						IOFindNameForValue(gLastMsgOut[0], scsiMsgValues)
					);
#endif /* LOG_ERROR_RECOVERY */
					ioReturn = SR_IOST_HW;				/* 970610 */
				}
				/* ioReturn = SR_IOST_HW;	970610 */
				break;
			}
		case kScsiMsgAbort:
		case kScsiMsgAbortTag:
			if (gActiveCommand != NULL && gActiveCommand->scsiReq != NULL) {
				/*
					* Oops: something is terribly wrong with this command.
					* This can happen if we get a parity error, set ATN,
					* and send an inititor detected error to the target.
					*/
				if (gCurrentTarget != kInvalidTarget) {
					ddmError("Target %d.%d aborted request\n",
						gCurrentTarget,
						gCurrentLUN,
						3, 4, 5
					);
#if LOG_ERROR_RECOVERY
					IOLog("%s: Target %d.%d aborted request\n",
						[self name],
						gCurrentTarget,
						gCurrentLUN
					);
#endif /* LOG_ERROR_RECOVERY */
				}
		[self ioComplete
			    : gActiveCommand
		    finalStatus : SR_IOST_HW
		];
				gFlagCheckForAnotherInterrupt = FALSE;
				ASSERT(gActiveCommand == NULL);
			}
			/*
			 * After receiving an Abort, the target will go free
			 */
			/* gFlagBusBusy = FALSE;	-- Radar 1678545: wait for interrupt */
			gBusState = SCS_WAIT_FOR_BUS_FREE;
			break;
		case kScsiMsgLinkedCmdComplete:
		case kScsiMsgLinkedCmdCompleteFlag:
			ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
			/*
			 * These are impossible: we reject commands with the link bit set.
			 * About all we can do is to fail the client's command and
			 * stagger onwards.
			 */
			IOLog("%s: Target %d.%d terminated with unexpected linked command complete\n",
				[self name],
				gActiveCommand->scsiReq->target,
				gActiveCommand->scsiReq->lun
			);
	    [self ioComplete
			: gActiveCommand
		finalStatus : SR_IOST_HW
	    ];
			ASSERT(gActiveCommand == NULL);
			/* gFlagBusBusy = FALSE;	-- Radar 1678545: wait for interrupt */
			gBusState = SCS_WAIT_FOR_BUS_FREE;
			break;
		case kScsiMsgExtended:
			ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
			/*
			 * The only expected extended message is synchronous negotiation.
			 * We don't support this yet.
			 */
			switch (gMsgInBuffer[2]) {
			case kScsiMsgSyncXferReq:
				/*
				 * We don't support SDTR (yet), so send a message reject
				 * (Perhaps it would be better to send an "async only" response,
				 * but message reject is legal.)
				 */
		CURIOmessageReject();
				messageAckNeeded = FALSE;
				break;
			default:
				ddmError("Unexpected Extended Message (0x%x) received"
						"from target %d.%d\n",
					gMsgInBuffer[2],
					gActiveCommand->scsiReq->target,
					gActiveCommand->scsiReq->lun,
					4, 5
				);
		CURIOmessageReject();
				messageAckNeeded = FALSE;
				break;
			}
			break;
		default:
			/*
			 * all others are unacceptable. 
			 */
			ddmError("Unsupported message (0x%x) received"
					" from target %d.%d \n", 
				gMsgInBuffer[0],
				gCurrentTarget,
				gCurrentLUN,
				4, 5
			);
	    CURIOmessageReject();
			messageAckNeeded = FALSE;
		} /* Message byte switch */
		if (messageAckNeeded) {
	    CURIOmessageAccept();
		}
		if (ioReturn != IO_R_SUCCESS && gCurrentTarget != kInvalidTarget) {
			ddmError("Message from target %d.%d failed: %s\n",
				gCurrentTarget,
				gCurrentLUN,
				reason,
				4, 5
			);
		}
		RESULT(ioReturn);
		return (ioReturn);
}

/*
 * This writes the message byte into the staging buffer. When the target
 * cycles to MSGO phase, we will send the message from the state automaton.
 */
- (void) putMessageOutByte
						: (UInt8)	messageByte
			setATN		: (Boolean) setATN
{
		ENTRY("Bmo putMessageOutByte");
		ASSERT(gMsgOutPtr < &gMsgOutBuffer[kMessageOutBufferLength]);
		ASSERT(gMsgPutPtr <= gMsgOutPtr);
		*gMsgOutPtr++ = messageByte;
		if (setATN) {
	    CURIOsetATN();
		}
		EXIT();
}

/*
 * Just completed the SCMD_TRANSFER_INFO operation for message out. 
 */
- (void) fsmSendingMsg
{
		ENTRY("Bms fsmSendingMsg");
		ddmChip("fsmSendingMsg\n", 1,2,3,4,5);
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		EXIT();
}

/*
 * Just completed the SCMD_TRANSFER_INFO operation for command.
 */
- (void) fsmSendingCmd
{
		ENTRY("Bmc fsmSendingCmd");
		ddmChip("fsmSendingCmd\n", 1,2,3,4,5);
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		gBusState = SCS_INITIATOR;
		EXIT();
}

/*
 * Continue to process reselection interrupts. We expect to be in
 * message in phase in order to get the tagged queue message. Anything
 * else is an error.
 */
- (void) fsmReselecting
{
	 	ENTRY("Bre fsmReselecting");

		[self fsmGettingMsg];
		if (gBusState == SCS_GETTINGMSG) {
			gBusState = SCS_RESELECTING;	/* continue at fsmReselectinAction */
		}
		EXIT();
}

/*
 * Complete the processing of a reselection interrupt. We have just ack'ed
 * the identify message. Try to find the command that corresponds to this
 * target.lun (without a queue tag). If we're successful, complete the
 * reselection and start following phases by calling fsmPhaseChange directly.
 * If the first disconnected command has a non-zero tag queue, we expect to
 * be in messsage in phase, and will receive a tagged queue message in the
 * fullness of time.
 */
- (void) fsmReselectionAction
{
		IOReturn			ioReturn;

		ENTRY("Bra fsmReselectionAction");
		ioReturn = [self reselectNexusWithoutTag];
		if (ioReturn == IO_R_SUCCESS) {
			/*
			 * We have successfully reselected this target.
			 * Vamp until the target tells us what to do next.
			 */
		//	[self fsmPhaseChange];	// mlj ???
			gBusState = SCS_INITIATOR;
		}
		else {
			/*
			 * We don't have a nexus yet. If we're in MSGI phase,
			 * continue grabbing message bytes until we receive
			 * a queue tag message (we might receive an SDTR),
			 * continuing at fsmReselecting to process interrupts.
			 */
			gCurrentBusPhase = gSaveStatus & mPhase;
			// [curioQuickCheckForChipInterrupt];
			if (gCurrentBusPhase == kBusPhaseMSGI) {
		CURIOstartMSGIAction();
			}
			else {
				/*
				 * We're in the wrong phase. Bail out.
				 */
				[self fsmStartErrorRecovery
								: ioReturn
						reason	: "Unexpected SCSI bus phase while reselecting"
				];
			}
		}
		EXIT();
}


/*
 * Follow SCSI Phase change. Called while SCS_INITIATOR. 
 */
- (void) fsmPhaseChange
{
		int 				i;

		ENTRY("Bph fsmPhaseChange");
		ddmChip("fsmPhaseChange, target %d.%d\n", 
			gCurrentTarget,
			gCurrentLUN,
			3, 4, 5
		);
		gCurrentBusPhase = gSaveStatus & mPhase;
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		ASSERT(gCurrentTarget	== gActiveCommand->scsiReq->target);
		ASSERT(gCurrentLUN		== gActiveCommand->scsiReq->lun);
		ddmChip("fsmPhaseChange:  phase = %s\n", 
			IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues),
			2,3,4,5
		);
		switch (gCurrentBusPhase) {
		case kBusPhaseCMD:
			/*
			 * The normal case here is after a host-initiated SDTR sequence. 
			 */
	    CURIOflushFifo();
	    CURIOputCommandIntoFifo();
	    CURIOstartNonDMATransfer();
			gMsgInState = kMsgInIdle;
			gBusState = SCS_SENDINGCMD;
			break;
		case kBusPhaseDATI:	/* From target to Initiator (read)	*/
		case kBusPhaseDATO:	/* To Target from Initiator (write) */
			gMsgInState = kMsgInIdle;
			[self fsmStartDataPhase];
			gFlagCheckForAnotherInterrupt = FALSE;
			break;
		case kBusPhaseSTS:	/* Status from Target to Initiator */
			/*
			 * fsmCompleting will collect the STATUS byte (and hopefully
			 * a MSG) from the fifo when this completes.
			 */
			gMsgInState = kMsgInIdle;
			gBusState = SCS_COMPLETING;
	    CURIOflushFifo();
	    CURIOinitiatorCommandComplete();
			break;
		case kBusPhaseMSGI:	/* Message from Target to Initiator */
			gBusState = SCS_GETTINGMSG;
	    CURIOstartMSGIAction();
			break;
		case kBusPhaseMSGO:	/* Message from Initiator to Target */
			gMsgInState = kMsgInIdle;
	    CURIOflushFifo();
			if (gMsgPutPtr == gMsgOutPtr) {
				/*
					* Hmm, there is no message waiting to be sent.
					*/
				*gMsgOutPtr++ = kScsiMsgNop;
				ddmChip("msg out (%d.%d): forced nop\n",
					gCurrentTarget,
					gCurrentLUN,
					3, 4, 5
				);
			}
			for (i = 0; i < 16 && gMsgPutPtr < gMsgOutPtr; i++) {
				ddmChip("msg out[%d] = %02x\n", i, *gMsgPutPtr, 3, 4, 5);
		CURIOputByteIntoFifo(*gMsgPutPtr);
		gMsgPutPtr++;
			}
			if (gMsgPutPtr == gMsgOutPtr) {
				gMsgPutPtr = gMsgOutPtr = gMsgOutBuffer;
			}
			gBusState = SCS_SENDINGMSG;
			/* 
			 * ATN is automatically cleared when transfer info completes.
			 */
	    CURIOstartNonDMATransfer();
			break;
		default:
			[self fsmStartErrorRecovery
							: SR_IOST_HW
					reason	: "Strange bus phase"
			];
			break;
		}
		EXIT();
}

- (void) fsmStartDataPhase
{
		UInt32 				phase;
		IOReturn	 		ioReturn	= IO_R_SUCCESS;
		const char			*reason		= NULL;
		Boolean				isReadOK;


		ENTRY("Bdp startDataPhase");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		phase = gSaveStatus & mPhase;
		/*
		 * Data in phase is legal if this is a read command or we're
		 * doing autosense.
		 */
		isReadOK = (gActiveCommand->scsiReq->read || gActiveCommand->flagIsAutosense);
		if ((phase == kBusPhaseDATI) != isReadOK) {
			ioReturn = SR_IOST_BV;
			reason = "bad I/O direction";
		}
		if (ioReturn == IO_R_SUCCESS) {
			ioReturn = [self hardwareInitializeCCL];
			if (ioReturn != IO_R_SUCCESS) {
				reason = "Cannot setup DBDMA Channel Command area";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			if (gActiveCommand->thisTransferLength == 0) {
				ioReturn = IO_R_BV;
				reason = "No data for transfer";
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			DBDMASetCommandPtr((UInt32) gDBDMAChannelAddress);
	    DBDMAstartTransfer();
	    CURIOstartDMATransfer(gActiveCommand->thisTransferLength);
			gBusState = SCS_DMACOMPLETE;
		}
		if (ioReturn != IO_R_SUCCESS) {
			[self fsmStartErrorRecovery
							: ioReturn
					reason	: "Illegal Data Phase (wrong direction or length error)"
			];
		}
		EXIT();
}

/*
 * Start error recovery by doing something reasonable for the current bus
 * phase. This is called when we enter error recovery (SCS_DEATH_MARCH)
 * from an interrupt or other bus phase action handler.
 */
- (void)			fsmStartErrorRecovery
						: (sc_status_t) status
			reason		: (const char *) reason
{
		ENTRY("Bes fsmStartErrorRecovery");
		TAG(__tag__, status);
		ddmChip("fsmStartErrorRecovery for %d.%d, phase \"%s\", status \"%s\": %s\n",
			gCurrentTarget,
			gCurrentLUN,
			IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues),
			IOFindNameForValue(status, IOScStatusStrings),
			(reason == NULL) ? "unspecified" : reason
		);
#if LOG_ERROR_RECOVERY
		if (gActiveCommand != NULL
		 && gActiveCommand->scsiReq != NULL
		 && gActiveCommand->scsiReq->target == gCurrentTarget
		 && gActiveCommand->scsiReq->lun == gCurrentLUN) {
			IOLog("%s: Error recovery: active target %d.%d, error \"%s\" (\"%s\"), bus phase %s\n",
				[self name],
				gCurrentTarget,
				gCurrentLUN,
				IOFindNameForValue(status, IOScStatusStrings),
				(reason == NULL) ? "no further explanation" : reason,
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues)
			);
		}
		else if (gActiveCommand != NULL && gActiveCommand->scsiReq != NULL) {
			IOLog("%s: Error recovery: command target %d.%d, error \"%s\" (\"%s\"), bus phase %s\n",
				[self name],
				gActiveCommand->scsiReq->target,
				gActiveCommand->scsiReq->lun,
				IOFindNameForValue(status, IOScStatusStrings),
				(reason == NULL) ? "no further explanation" : reason,
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues)
			);
		}
		else if (gCurrentTarget != kInvalidTarget && gCurrentLUN != kInvalidLUN) {
			IOLog("%s: Error Recovery: expected target %d.%d, error \"%s\" (\"%s\"), bus phase %s\n",
				[self name],
				gCurrentTarget,
				gCurrentLUN,
				IOFindNameForValue(status, IOScStatusStrings),
				(reason == NULL) ? "no further explanation" : reason,
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues)
			);
		}
		else if (gCurrentTarget != kInvalidTarget && gCurrentLUN == kInvalidLUN) {
			IOLog("%s: Error recovery: command target %d.[no lun], error \"%s\" (\"%s\"), bus phase %s\n",
				[self name],
				gCurrentTarget,
				IOFindNameForValue(status, IOScStatusStrings),
				(reason == NULL) ? "no further explanation" : reason,
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues)
			);
		}
		else {
			IOLog("%s: Error recovery: no current target, error \"%s\" (\"%s\"), bus phase %s\n",
				[self name],
				IOFindNameForValue(status, IOScStatusStrings),
				(reason == NULL) ? "no further explanation" : reason,
				IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues)
			);
		}
		[self logRegisters : FALSE reason : "At error recovery start"];
#endif /* LOG_ERROR_RECOVERY */
		[self killActiveCommand : status];
		/*
		 * Clean out the scsi and dbdma chips
		 */
	CURIOflushFifo();
	CURIOclearTransferCountZeroBit();
	DBDMAreset();
		if (gFlagBusBusy == FALSE) {
			gBusState = SCS_DISCONNECTED;
		}
		else {
			/*
			 * We will continue at fsmContinueErrorRecovery
			 */
			gBusState = SCS_DEATH_MARCH;
		}
		gFlagCheckForAnotherInterrupt = FALSE;
		EXIT();
}

/*
 * Manage error recovery by responding to a target interrupt while in
 * an error recovery state. On exit, the bus state will be as follows:
 *	SCS_DEATH_MARCH		Still in error recovery. Call fsmContinueErrorRecovery
 *						to continue processing target requests.
 *	SCS_DISCONNECTED	The target is finally off the bus. We can restart
 *						normal operation.
 * Each bus phase requires a different action:
 *	Data Out	Clean up after DMA.
 *	Data In		Clean up after DMA.
 *	Command		Flush the fifo
 *	Status		Drain the fifo, if we get two bytes, just ACK the message
 *	Message Out	Do nothing
 *	Message In	Read the message byte, ACK it.
 */
- (void) fsmErrorRecoveryInterruptService
{
		ENTRY("Bei fsmErrorRecoveryInterruptService");
#if LOG_ERROR_RECOVERY
		[self logRegisters : FALSE reason : "At error recovery interrupt"];
		IOLog("%s: Error recovery interrupt in bus phase %s, sts %02x, int %02x\n",
			[self name],
			IOFindNameForValue(gSaveStatus & mPhase, scsiPhaseValues),
			gSaveStatus,
			gSaveInterrupt
		);
 #endif
	CURIOflushFifo();
		switch (gCurrentBusPhase) {
		case kBusPhaseDATO:
		case kBusPhaseDATI:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt in data in/out phase\n", [self name]);
#endif
	    DBDMAreset();
	    CURIOclearTransferCountZeroBit();
			break;
		case kBusPhaseCMD:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt in command phase\n", [self name]);
#endif
			break;
		case kBusPhaseSTS:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt in status phase\n", [self name]);
#endif
			if ((gSaveInterrupt & iFuncComp) != 0) {
#if LOG_ERROR_RECOVERY
				IOLog("%s: Error recovery interrupt in status phase with complete msg\n",
					[self name]
				);
#endif
				/*
				 * We got both bytes: ACK the command complete
				 */
		CURIOmessageAccept();
			}
 			break;
		case kBusPhaseMSGO:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt in message out phase\n", [self name]);
#endif
			break;
		case kBusPhaseMSGI:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt in message in phase\n", [self name]);
#endif
	    CURIOmessageAccept();
			break;
		case kBusPhaseBusFree:	/* Selecting */
#if LOG_ERROR_RECOVERY
			IOLog("%s: Error recovery interrupt at bus free\n", [self name]);
#endif
			break;
		default:
#if LOG_ERROR_RECOVERY
			IOLog("%s: Strange bus phase %d in error recovery\n",
				[self name],
				gCurrentBusPhase
			);
#endif
			[self killActiveCommandAndResetBus
						: SR_IOST_HW
				reason	: "Strange SCSI phase in error recovery interrupt"
			];
			break;
		}
		EXIT();
}

/*
 * Continue to manage error recovery. We are here because the target
 * is still on the bus and doing strange bus phase things. Follow
 * the target bus phase (one phase at a time) until the target
 * disconnects. We just run bit-bucket commands until the target
 * gives up. (Note that this means that we might send a Command Complete
 * message to the target. We'll look for this case and send an Abort
 * or Abort Tag instead.)
 */
- (void)			fsmContinueErrorRecovery
{
		UInt8			msgByte;

		ENTRY("Bec fsmContinueErrorRecovery");
	CURIOquickCheckForChipInterrupt();
#if LOG_ERROR_RECOVERY
		IOLog("%s: Error recovery continues in bus phase %s, sts %02x, int %02x\n",
			[self name],
			IOFindNameForValue(gSaveStatus & mPhase, scsiPhaseValues),
			gSaveStatus,
			gSaveInterrupt
		);
#endif
		if (gFlagBusBusy) {
	    (void) CURIOinterruptPending();
			if ((gSaveInterrupt & iDisconnect) != 0) {
				gFlagBusBusy = FALSE;
			}
		}
		if (gFlagBusBusy == FALSE) {
			gBusState = SCS_DISCONNECTED;
		}
		else {
			gCurrentBusPhase = gSaveStatus & mPhase;
			switch (gCurrentBusPhase) {
			case kBusPhaseMSGO:
				msgByte = kScsiMsgAbort;
				if (gActiveCommand != NULL
				 && gActiveCommand->queueTag != QUEUE_TAG_NONTAGGED) {
					msgByte = kScsiMsgAbortTag;
				}
		CURIOputByteIntoFifo(msgByte);
		CURIOstartNonDMATransfer();
				break;
			case kBusPhaseDATO:
			case kBusPhaseCMD:
		CURIOtransferPad(TRUE);		    /* Output pad   */
				break;
			case kBusPhaseMSGI:
			case kBusPhaseDATI:
			case kBusPhaseSTS:
		CURIOtransferPad(FALSE);	    /* Input pad    */
				break;
			default:
				[self killActiveCommandAndResetBus
						: SR_IOST_HW
					reason	: "Strange SCSI phase in error recovery"
				];
				break;
			}
		}
#if LOG_ERROR_RECOVERY
		[self logRegisters : FALSE reason : "Registers at error recovery action exit"];
#endif /* -Debug */
		EXIT();
}

/*
 * Disable specified mode for gActiveCommand's target. If mode is currently 
 * enabled, we'll log a message to the console.
 */
- (void) disableMode : (TargetMode) mode
{
		int					target;
		PerTargetData		*perTargetPtr;
		const char			*modeStr = NULL;

		ENTRY("Bdm disableMode");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		target = gActiveCommand->scsiReq->target;
		perTargetPtr = &gPerTargetData[target];
		switch (mode) {
		case kTargetModeCmdQueue:
			if (perTargetPtr->cmdQueueDisable == 0) {
				perTargetPtr->cmdQueueDisable = 1;
				modeStr = "Command Queueing";
			}
			break;
		default:
			IOLog("%s: Bogus target mode %d for target %d\n",
				[self name],
				mode,
				target
			);
			break;
		}
		ddmChip("DISABLING %s for target %d\n", modeStr, target, 3,4,5);
		if (modeStr != NULL) {
			IOLog("%s: DISABLING %s for target %d\n",
				[self name],
				modeStr,
				target
			);
		}
		EXIT();
}

/**
 * Validate the target's reselection byte (put on the bus before
 * reselecting us). Erase the initiator ID and convert the other
 * bit into an index. The algorithm should be faster than a
 * sequential search, but it probably doesn't matter much.
 * @return	TRUE if successful (gCurrentTarget is now valid).
 *			This function does not check whether there actually
 *			is a command pending for this target.
 */
- (IOReturn) getReselectionTargetID
						: (UInt8) selectByte
{
		IOReturn			ioReturn	= SR_IOST_BV;
		register UInt8		targetBits	= selectByte;
		register UInt8		targetID	= 0;
		register UInt8		bitValue	= 0;	/* Supress warning	*/

		ENTRY("Brt getReselectionTargetID");
		if ((targetBits & gInitiatorIDMask) == 0) {
			IOLog("%s: Reselection failed: initiator ID bit not set, got %02x\n",
				[self name],
				selectByte
			);
		}
		else {
			targetBits &= ~gInitiatorIDMask;			/* Remove our bit			*/
			if (targetBits == 0) {
				IOLog("%s: Reselection failed: target ID bit not set, got %02x\n",
					[self name],
					selectByte
				);
			}
			else {
				bitValue		= targetBits;
				if ((bitValue > 0x0F) != 0) {
					targetID	+= 4;
					bitValue	>>= 4;
				}
				if ((bitValue > 0x03) != 0) {
					targetID	+= 2;
					bitValue	>>= 2;
				}
				if ((bitValue > 0x01) != 0) {
					targetID	+= 1;
				}
		targetBits	&= ~(1 << targetID);	/* Remove the target mask   */
				if (targetBits == 0) {					/* Was exactly one set?		*/
					ioReturn = IO_R_SUCCESS;			/* Yes: success!			*/
		    gCurrentTarget = targetID;		/* Save the current target  */
					ddmChip("Reselection from target %d\n",
						gCurrentTarget,
						2, 3, 4, 5
					);
				}
				else {
					IOLog("%s: Reselection failed, multiple targets, got %02x\n",
						[self name],
						selectByte
					);
				}
			}
		}
		RESULT(ioReturn);
		return (ioReturn);
}

@end	/* Apple96_SCSI(BusState) */
