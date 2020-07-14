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
 * @revision	1997.02.17	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96CurioPublic.m - Chip-specific methods for Apple96 SCSI driver.
 *
 * Edit History
 * 1997.02.18	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */
#import "Apple96SCSI.h"
#import "Apple96CurioPublic.h"
#import "Apple96CurioPrivate.h"
#import "Apple96HWPrivate.h"
#import "Apple96CurioDBDMA.h"
#import "Apple96ISR.h"
#import <driverkit/generalFuncs.h>
#import <kernserv/prototypes.h>

@implementation Apple96_SCSI(CurioPublic)

/*
 * This should be extended to perform a real chip self-test.
 * (See the code in Copland DR1)
 */
- (IOReturn) hardwareChipSelfTest
{
		IOReturn			ioReturn = IO_R_SUCCESS;
		UInt8				chipID;
		
		ENTRY("Cst hardwareChipSelfTest");
		[self disableAllInterrupts];
	CURIOresetChip();
	CURIOwriteRegister(rCF2, CF2_ENFEATURES);
	CURIOsetCommandRegister(cNOP | bDMDEnBit);  /* DMA Nop	*/
	chipID = CURIOreadRegister(rTCH);
	CURIOresetChip();
		/*
		 * Allow the user to *disable* fast and sync if they
		 * were enabled by hardware probe, but do not allow the
		 * user to enable them if the hardware does not support
		 * the feature.
		 */
		switch (chipID) {
		case 0xA2:						/* NCR 53cf96		*/
		case 0x12:						/* AMD 53cf96		*/
			gOptionSyncModeSupportedByHardware = 1;
			gOptionFastModeSupportedByHardware = 1;
			break;
		default:						/* NCR 53c96		*/
			gOptionSyncModeSupportedByHardware = 0;
			gOptionFastModeSupportedByHardware = 0;
			break;
		}
		/*
		 * To do: write a test pattern into the fifo and check
		 * for correct count and bits.
		 */
		RESULT(ioReturn);
		return (ioReturn);
}

/*
 * Reusable hardware initializer function. if resetSCSIBus is TRUE, this
 * includes a SCSI reset. Handling of ioComplete of active and disconnected
 * commands must be done elsewhere. Returns IO_R_SUCCESS.
 * This is called with interrupts disabled.
 */
- (IOReturn) curioHardwareReset
						: (Boolean) resetSCSIBus
			reason		: (const char *) reason
{
		IOReturn			ioReturn = IO_R_SUCCESS;
		UInt8			 	configValue;
		UInt8				clockConversionFactor;
		UInt8				defaultSelectionTimeout;

		ENTRY("Chr curioHardwareReset");
	/*
	 * Temp for debugging
	 */
	scsiClockRate	    = kChipDefaultBusClockMHz;
		/*
		 * First of all, reset interrupts, the SCSI chip, and the DMA engine.
		 */
	CURIOresetChip();		/* Clear out the chip	    */
	DBDMAreset();			/* Stop the DMA engine	    */
	CURIOreadInterruptRegister();	/* Clear pending interrupt  */
		/*
		 * Initialize the chip.
		 */
	CURIOwriteRegister(rCF2, CF2_ENFEATURES);
	CURIOsetCommandRegister(cNOP | bDMDEnBit);  /* DMA Nop	*/

		/*
		 * Init state variables.
		 */
		gFlagCheckForAnotherInterrupt	= 0;
		gBusState				= SCS_DISCONNECTED;
		gFlagBusBusy					= FALSE;
		gCurrentBusPhase		= kBusPhaseBusFree;
		/*
		 * Smash all active command state (just in case).
		 */
		gActiveCommand			= NULL;
		gCurrentTarget			= kInvalidTarget;
		gCurrentLUN				= kInvalidLUN;
		gMsgInState				= kMsgInIdle;
		gMsgOutPtr = gMsgPutPtr	= gMsgOutBuffer;

		/*
		 * Configuration Register 1
		 *	Disable interrupt on initiator-instantiated bus reset (is this correct?)
		 *	Enable parity.
		 *	Set default bus ID (7) (This should be overriden by the Inspector)
		 */
		gInitiatorID = kDefaultInitiatorID;
		configValue = CF1_SRD | CF1_ENABPAR | gInitiatorID;
		if (gOptionExtendTiming) {
			/*
			 * Per instance table. This slows down transfers on the bus.
			 */
			configValue |= CF1_SLOW;
		}
		ddmInit("Configuration 1 = 0x%x\n", configValue, 2,3,4,5);
	CURIOwriteRegister(rCF1, configValue);
		/*
		 * Clock factor and select timeout.
		 */
		ASSERT(scsiClockRate != 0);
		/*
		 * Use the clock frequency (in MHz) to select the clock conversion
		 * factor. According to the NCR53CF94/96 data manual, the conversion
		 * factor is defined by the following table: Currently, we don't allow
		 * the caller to change selection timeout from the ANSI standard 250 Msec
		 * to avoid having to support floating-point register manipulation
		 */
		if (scsiClockRate < 10) {
			IOLog("Apple96_SCSI: Clock %d MHZ too low; using 10 MHz\n",
				 scsiClockRate);
			scsiClockRate = 10;
		}
		if (scsiClockRate > 40) {
			IOLog("Apple96_SCSI: Clock %d MHZ too high; using 40 MHz\n",
				 scsiClockRate);
			scsiClockRate = 40;
		}
		if		(scsiClockRate <= 10) {
			clockConversionFactor = ccf10MHz;
			defaultSelectionTimeout = SelTO16Mhz;
		}
		else if (scsiClockRate <= 15) {
			clockConversionFactor = ccf11to15MHz;
			defaultSelectionTimeout = SelTO16Mhz;
		}
		else if (scsiClockRate <= 20) {
			clockConversionFactor = ccf16to20MHz;
			defaultSelectionTimeout = SelTO16Mhz;
		}
		else if (scsiClockRate <= 25) {
			clockConversionFactor = ccf21to25MHz;
			defaultSelectionTimeout = SelTO25Mhz;
		}
		else if (scsiClockRate <= 30) {
			clockConversionFactor = ccf26to30MHz;
			defaultSelectionTimeout = SelTO33Mhz;
		}
		else if (scsiClockRate <= 35) {
			clockConversionFactor = ccf31to35MHz;
			defaultSelectionTimeout = SelTO40Mhz;
		}
		else {
			clockConversionFactor = ccf31to35MHz;
			defaultSelectionTimeout = SelTO40Mhz;
		}
		ddmInit("clockFactor %d\n", clockConversionFactor, 2,3,4,5);
	CURIOwriteRegister(rCKF, clockConversionFactor);
		ddmInit("select timeout reg 0x%x\n", defaultSelectionTimeout, 2,3,4,5);
	CURIOsetSelectionTimeout(defaultSelectionTimeout);

		/*
		 * Configuration Register 2 - enable extended features
		 *		- mainly, 24-bit transfer count.
		 */
	CURIOwriteRegister(rCF2, CF2_ENFEATURES);
		/*
		 * Configuration Register 3
		 */
		configValue = 0;
		if ((gOptionFastModeEnable && gOptionFastModeSupportedByHardware)
		 || scsiClockRate > 25) {
			configValue |= CF3_FASTSCSI;
		}
		ddmInit("control3 = 0x%x\n", configValue, 2,3,4,5);
	CURIOwriteRegister(rCF3, configValue);
		/*
		 * Configuration Register 4 - glitch eater, active negation.
		 * Let's not worry about these whizzy features just yet.
		 */
	CURIOwriteRegister(rCF4, 0);
		/*
		 * Go to async xfer mode for now. Sync gets enabled on a per-target 
		 * basis in -targetContext.
		 */
	CURIOsetSynchronousOffset(0);
		/*
		 * Reset SCSI bus, wait, clear possible interrupt.
		 */
		if (resetSCSIBus) {
			if (reason != NULL) {
				IOLog("%s: Resetting SCSI bus (%s)\n",
					[self name],
					reason
				);
			}
			else {
				IOLog("%s: Resetting SCSI bus\n", [self name]);
			}
	    CURIOresetSCSIBus();
			IOSleep(APPLE_SCSI_RESET_DELAY);
	    CURIOreadInterruptRegister();
			ddmInit("hardwareReset: enabling interrupts\n", 1,2,3,4,5);
		}
		ddmInit("hardwareReset: DONE\n", 1,2,3,4,5);
		RESULT(ioReturn);
		return (ioReturn);
}

/*
 * Terminate a DMA, including FIFO flush if necessary. Returns number of 
 * bytes transferred.
 */
- (UInt32) curioDMAComplete
{
	UInt32		fifoResidualCount;
		UInt32			actualTransferCount;
		UInt32			dmaResidualCount;
		UInt8			residualByte;
		
		ENTRY("Cdc curioDMAComplete");
	DBDMAstopTransfer();
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		/*
		 * Get residual count from DMA transfer.
		 */
	SynchronizeIO();
	dmaResidualCount = CURIOgetTransferCount();
		actualTransferCount = gActiveCommand->thisTransferLength
						- dmaResidualCount;
		ddmChip("curioDMAComplete (dma): thisTransferLength %d, actualTransferCount %d\n",
			gActiveCommand->thisTransferLength, actualTransferCount, 3,4,5);
		if (actualTransferCount > gActiveCommand->thisTransferLength) {
			ddmError("fsmDMAComplete: DMA count exceeeded, %d expected, %d transferred\n",
				gActiveCommand->thisTransferLength,
				actualTransferCount,
				3, 4, 5
			);
		}
		/*
		 * Advance the current data index. Note that gActiveCommand->currentDataIndex
		 * will exceed scsiReq->maxTransfer if the target tries to transfer too much
		 * data. We need to add a bit-bucket handler here.
		 */
		gActiveCommand->currentDataIndex += actualTransferCount;
		[ gActiveCommand->mem setPosition : gActiveCommand->currentDataIndex ];
		/*
		 * Now, see if there are any bytes lurking in the fifo.
		 */
	SynchronizeIO();
	fifoResidualCount = CURIOgetFifoCount();
		if (fifoResidualCount != 0) {
			ddmChip("curioDMAComplete: fifoResidualCount %d\n",
				fifoResidualCount, 2, 3,4,5);
			if (gActiveCommand->scsiReq->read) {
				/*
				 * Try to move residual data from the fifo into the
				 * user's buffer.
				 */
/* ++ Radar xxxxxx use IOMemoryDescriptor */
		[ gActiveCommand->mem setPosition : gActiveCommand->currentDataIndex ];
		for (;;) {
		    SynchronizeIO();
		    if (CURIOgetFifoCount() == 0) {
			break;
				}
		    else {
			SynchronizeIO();
			residualByte = CURIOgetFifoByte();
			[gActiveCommand->mem writeToClient : &residualByte count : 1];
				gActiveCommand->currentDataIndex++;
			}
		}
			actualTransferCount += fifoResidualCount;						/* Radar 2207320	*/
/* -- Radar xxxxxx use IOMemoryDescriptor */
	    }
			else {
				CURIOflushFifo();
				CURIOclearTransferCountZeroBit();
				actualTransferCount					-= fifoResidualCount;	/* Radar 2207320	*/
				gActiveCommand->currentDataIndex	-= fifoResidualCount;
			}
			if (  gActiveCommand->mem )										/* Radar 2211821	*/
				[ gActiveCommand->mem setPosition : gActiveCommand->currentDataIndex ];
		}
		RESULT(actualTransferCount);
		return (actualTransferCount);
}

/*
 * Prepare for power down. 
 *  -- reset SCSI bus to get targets back to known state
 *  -- reset chip
 * Note: this presumes that a higher-power has cleaned out all
 * pending and in-progress commands.
 */
- (void) powerDown
{
		ENTRY("Cpd powerDown");
	DBDMAreset();			/* Stop the DMA engine	    */
	CURIOresetSCSIBus();		/* Reset the SCSI bus	    */
	IOSleep(APPLE_SCSI_RESET_DELAY);    /* Stall after reset	*/
	CURIOreadInterruptRegister();	/* Clear the interrupt	    */
	CURIOresetChip();		/* Clear out the chip	    */
	CURIOreadInterruptRegister();	/* Clear pending interrupt  */
		EXIT();
}

- (void) logRegisters
			: (Boolean) examineInterruptRegister
		reason	: (const char *) reason
{
#if 1 || DEBUG
	UInt8			currentStatus;
	UInt8			currentInterrupt;
	UInt8 			fifoDepth;
	UInt32			transferCount;
		
	ENTRY("Clo logRegisters");
	if (reason != NULL) {
	    IOLog("%s: *** Log registers: %s\n", [self name], reason);
	}
	IOLog("%s: *** last chip status 0x%02x, interrupt 0x%02x, command 0x%02x\n", 
		[self name],
		gSaveStatus,
		gSaveInterrupt,
		CURIOreadCommandRegister()
	    );
	IOLog("%s: *** bus state %s, bus phase %s, bus %s\n",
		[self name],
		IOFindNameForValue(gBusState, gAutomatonStateValues),
		IOFindNameForValue(gCurrentBusPhase, scsiPhaseValues),
		(gFlagBusBusy) ? "busy" : "free"
	    );
	currentStatus = CURIOreadStatusRegister();
	if (examineInterruptRegister) {
	    currentInterrupt = CURIOreadInterruptRegister();	/* Possible problem */
	    IOLog("%s: *** current status %02x current intrStatus %02x\n",
		    [self name],
		    currentStatus,
		    currentInterrupt
		);
	}
	else {
	    IOLog("%s: *** current status %02x\n",
		    [self name],
		    currentStatus
		);
	}
	IOLog("%s: *** syncOffset %d  syncPeriod 0x%02x\n",
		[self name],
		gLastSynchronousOffset,
		gLastSynchronousPeriod
	    );
	SynchronizeIO();
	fifoDepth	    = CURIOgetFifoCount();
	SynchronizeIO();
	transferCount	    = CURIOgetTransferCount();
	IOLog("%s: *** fifoDepth %d  transfer count %d\n",
		[self name],
		fifoDepth,
		transferCount
	    );
	if (gActiveCommand != NULL) {
	    [self logCommand
			    : gActiveCommand
		logMemory   : TRUE
		    reason  : "Active Command"
		];
	}
	[self logChannelCommandArea : reason];
	[self logIOMemoryDescriptor : reason];
	[self logTimestamp : reason];
	EXIT();
#endif	/* DEBUG */
}

- (void) logCommand
			: (const CommandBuffer *) commandPtr
	logMemory	: (Boolean) isLogMemoryNeeded
	reason		: (const char *) reason;
{
#if DEBUG
	ENTRY("Clc logCommand");
	if (reason == NULL) {
	    reason = "";
	}
	if (commandPtr == NULL) {
	    IOLog("%s: *** no command to log: %s.\n",
		    [self name],
		    reason
		);
	}
	else {
	    IOLog("%s: *** %s\n",
		    [self name],
		    reason
		);
	    IOLog("%s: *** %scommand at %08x, tag %02x, cdbLength %d, scsiReq %08x\n",
		    [self name],
		    (commandPtr == gActiveCommand) ? "active " : "",
		    (UInt32) commandPtr,
		    commandPtr->queueTag,
		    commandPtr->cdbLength,
		    (UInt32) commandPtr->scsiReq
		);
	    IOLog("%s: *** currentDataIndex %d, saved %d, thisTransfer %d\n",
		    [self name],
		    commandPtr->currentDataIndex,
		    commandPtr->savedDataIndex,
		    commandPtr->thisTransferLength
		);
	    IOLog("%s: *** active %d, is autosense %d, sense status %02x\n",
		    [self name],
		    commandPtr->flagActive,
		    commandPtr->flagIsAutosense,
		    commandPtr->autosenseStatus
		);
#if __IO_MEMORY_DESCRIPTOR_DEBUG__
	    if (commandPtr->mem != NULL) {
		[commandPtr->mem debugLogWithContainer : [self name]];
	    }
#endif
	    if (commandPtr->scsiReq != NULL) {
		IOSCSIRequest		*scsiReq = commandPtr->scsiReq;
#if (SERIOUS_DEBUGGING || DUMP_USER_BUFFER) && __IO_MEMORY_DESCRIPTOR_DEBUG__
		if (isLogMemoryNeeded
		 && scsiReq != NULL
		 && commandPtr->mem != NULL) {
		    IOMemoryDescriptorContext	context;
		    LogicalRange	    range;
		    ByteCount		newPosition = 0;

		    [cmdbuf->mem context : &context];
		    [cmdbuf->mem setOffset : 0];
		    while (newPosition < scsiReq->bytesTransferred
			&& [cmdbuf->mem getLogicalRange
					: 1
			maxByteCount	: scsiReq->bytesTransferred - newPosition
			newPosition	: &newPosition
			actualRanges	: NULL
			logicalRanges	: &range] != 0) {
			[self logMemory : range.address
				length  	: range.length
				reason  	: "User buffer at I/O Complete"
			    ];
			}
		    [cmdbuf->mem setContext : &context];
		}
#endif /* (SERIOUS_DEBUGGING || DUMP_USER_BUFFER) && __IO_MEMORY_DESCRIPTOR_DEBUG__ */
		IOLog("%s: *** %d.%d, cdb: "
			"%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n",
			[self name],
			scsiReq->target,
			scsiReq->lun,
			((UInt8 *) &scsiReq->cdb)[0],
			((UInt8 *) &scsiReq->cdb)[1],
			((UInt8 *) &scsiReq->cdb)[2],
			((UInt8 *) &scsiReq->cdb)[3],
			((UInt8 *) &scsiReq->cdb)[4],
			((UInt8 *) &scsiReq->cdb)[5],
			((UInt8 *) &scsiReq->cdb)[6],
			((UInt8 *) &scsiReq->cdb)[7],
			((UInt8 *) &scsiReq->cdb)[8],
			((UInt8 *) &scsiReq->cdb)[9],
			((UInt8 *) &scsiReq->cdb)[10],
			((UInt8 *) &scsiReq->cdb)[11]
		    );
		IOLog("%s: *** read %d, disconnect %d, maxTransfer %d, bytesTransferred %d\n",
			[self name],
			scsiReq->read,
			scsiReq->disconnect,
			scsiReq->maxTransfer,
			scsiReq->bytesTransferred
		    );
		IOLog("%s: *** driverStatus %d, scsiStatus %02x, sense: "
			"%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n",
			[self name],
			scsiReq->driverStatus,
			scsiReq->scsiStatus,
			((UInt8 *) &scsiReq->senseData)[0],
			((UInt8 *) &scsiReq->senseData)[1],
			((UInt8 *) &scsiReq->senseData)[2],
			((UInt8 *) &scsiReq->senseData)[3],
			((UInt8 *) &scsiReq->senseData)[4],
			((UInt8 *) &scsiReq->senseData)[5],
			((UInt8 *) &scsiReq->senseData)[6],
			((UInt8 *) &scsiReq->senseData)[7],
			((UInt8 *) &scsiReq->senseData)[8],
			((UInt8 *) &scsiReq->senseData)[9],
			((UInt8 *) &scsiReq->senseData)[10],
			((UInt8 *) &scsiReq->senseData)[11]
		    );
	    }
	}
	EXIT();
#endif /* DEBUG */
}

- (void) logTimestamp
		: (const char *) reason
{
#if DEBUG
/*
 * kMaxTimestamp should be greater than twice the expected method depth
 * since, if we dump the timestamp after it has wrapped around, we expect
 * to lose earlier entries and, hence, the shallower method starts.
 */
#ifndef kMaxTimestampStack
#define kMaxTimestampStack	64
#endif

	TimestampDataRecord	stack[kMaxTimestampStack + 1];	/* Allocate one extra */			
	UInt32			index = 0;
	int			start;
	UInt32			count;
	UInt32			maxDepth = 0;
	Boolean			wasEnabled;
	char			work[8];
	struct timeval		tv;
	ns_time_t		lastEventTime;
	UInt32			elapsed;
	UInt32			sinceMethodStart;
				
	/* ENTRY("Clt logTimestamp");	** No ENTRY so we don't corrupt timestamps */
	if (reason != NULL) {
	    IOLog("%s: *** Log timestamp: %s\n", [self name], reason);
	}
	/*
	 * In case something we call causes timestamping, we want to
	 * avoid getting into an infinite loop.
	 */
	wasEnabled = EnableTimestamp(FALSE);
	lastEventTime = stack[0].eventTime; /* Initialization hack */
	for (count = 1; ReadTimestamp(&stack[index]); count++) {
	    work[0] = (stack[index].timestampTag) >> 24 & 0xFF;
	    work[1] = (stack[index].timestampTag) >> 16 & 0xFF;
	    work[2] = (stack[index].timestampTag) >>  8 & 0xFF;
	    work[3] = (stack[index].timestampTag) >>  0 & 0xFF;
	    work[4] = '\0';
	    elapsed = (unsigned) stack[index].eventTime - lastEventTime;
	    lastEventTime = stack[index].eventTime;
	    ns_time_to_timeval(stack[index].eventTime, &tv);					    switch (work[0]) {
	    case '+':		/* Entering a method */
		IOLog("%s: '%s' %u.%06u %u.%03u 0.0 %d\n",
			[self name],
			work,
			tv.tv_sec,
			tv.tv_usec,
			elapsed / 1000,
			elapsed - ((elapsed / 1000) * 1000),
			(int) stack[index].timestampValue
		    );
		if (index < kMaxTimestampStack) {
		    ++index;
		    if (index > maxDepth) {
			maxDepth = index;
		    }
		}
		break;
	    case '=':	/* Intermediate tag: find the method start	*/
	    case '-':	/* End of method: find the method start		*/
		sinceMethodStart = 0;
		for (start = index - 1; start >= 0; --start) {
		    if ((stack[start].timestampTag & 0x00FFFFFF)
			    == (stack[index].timestampTag & 0x00FFFFFF)) {
			sinceMethodStart = (unsigned) stack[index].eventTime
					- stack[start].eventTime;
			break;
		    }
	        }
		IOLog("%s: '%s' %u.%06u %u.%03u %u.%03u %d\n",
			[self name],
			work,
			tv.tv_sec,
			tv.tv_usec,
			elapsed / 1000,
			elapsed - ((elapsed / 1000) * 1000),
			sinceMethodStart / 1000,
			sinceMethodStart - ((sinceMethodStart / 1000) * 1000),
			(int) stack[index].timestampValue
		    );
		if (start >= 0 && work[0] == '-') {
		    index = start;		/* Pop the stack */
		}
		break;
	    case '>':	/* Tag: log the value in hex			*/
	    case '<':	/* Tag: log the value in hex			*/
	    case '*':	/* Tag: log the value in hex			*/
		IOLog("%s: '%s' %u.%06u %u.%03u 0.0 %08x\n",
			[self name],
			work,
			tv.tv_sec,
			tv.tv_usec,
			elapsed / 1000,
			elapsed - ((elapsed / 1000) * 1000),
			stack[index].timestampValue
		    );
		/* The stack does not change */
		break;
	    default:
		IOLog("%s: '%s' %u.%06u %u.%03u 0.0 %08x\n",
			[self name],
			work,
			tv.tv_sec,
			tv.tv_usec,
			elapsed / 1000,
			elapsed - ((elapsed / 1000) * 1000),
			stack[index].timestampValue
		    );
		    break;
	    }
	}
	IOLog("%s: *** %d timestamps, %d max method depth\n",
		[self name],
		count,
		maxDepth
	    );
	EnableTimestamp(wasEnabled);
	/* EXIT(); ** No EXIT to avoid corrupting timestamps */
#endif /* DEBUG */
}

- (void) logMemory
			: (const void *) buffer
	length		: (UInt32) length
	reason		: (const char *) reason
{
#ifndef kMaxLogMemoryLength
#define kMaxLogMemoryLength	512
#endif
#if DEBUG
	const UInt8		*ptr;
	UInt32			start;
	UInt32			i;
	char			work[32];

	ENTRY("Clm logMemory");
	if (reason == NULL) {
	    reason = "";
	}
	IOLog("%s: *** %s: log memory from %08x for %d bytes\n",
		[self name],
		reason,
		(UInt32) buffer,
		length
	    );
	if (buffer != NULL) {
	    if (length > kMaxLogMemoryLength) {
		length = kMaxLogMemoryLength;
		IOLog("%s: *** Logging first %d bytes\n", [self name], length);
	    }
	    ptr = (const UInt8 *) buffer;
	    for (start = 0; start < length; start += 16) {
		IOLog("%s: *** %08x %3d:",
			[self name],
			(UInt32) &ptr[start],
			start
		    );
		for (i = 0; i < 16; i++) {
		    if ((i % 4) == 0) {
			IOLog(" ");
		    }
		    if (start + i >= length) {
			IOLog("  ");
			work[i] = '\0';
		    }
		    else {
			IOLog("%02x", ptr[start + i]);
			work[i] = (ptr[start + i] >= ' ' && ptr[start + i] <= '~')
				? ptr[start + i] : '.';
		    }
		}
		work[i] = '\0';
		IOLog(" %s\n", work);
	    }
	}
	EXIT();
#endif /* DEBUG */
}

@end	/* Apple96_SCSI(CurioPublic) */

/* end of Apple96_Chip.m */

