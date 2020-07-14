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
 * Copyright 1993-1995 by Apple Computer, Inc., all rights reserved.
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author  Martin Minow    mailto:minow@apple.com
 * @revision	1997.02.13  Initial conversion from AMDPCSCSIDriver sources.
 * Accessor functions for the 53C96 registers and commands.
 * As noted above, the class provides an instance variable defined as:
 * 	volatile UInt8 *gSCSILogicalAddress		The chip's LogicalAddress
 *
 * Set tabs every 4 characters.
 * Edit History
 * 1997.02.13	MM	Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.07.18	MM	Added USE_CURIO_METHODS and wrote macros for the
 *			one-liner methods.
 */

#import "Apple96CurioPrivate.h"
#import "Apple96CurioPublic.h"
#import "Apple96CurioDBDMA.h"
#import "MacSCSICommand.h"
#import <driverkit/generalFuncs.h>
#import <kernserv/prototypes.h>

extern IONamedValue gAutomatonStateValues[];

@implementation Apple96_SCSI(Curio)

/*
 * This spin-loop looks for another chip operation - it prevents
 * exiting the interrupt service routine if the chip presents
 * another operation within ten microseconds. Note that this
 * must be called from an IOThread, not from a "real" primary
 * interrupt service routine.
 */
- (Boolean) curioQuickCheckForChipInterrupt
{
		ns_time_t			startTime;
		ns_time_t			endTime;
		unsigned			elapsedNSec;
		Boolean				result;

		ENTRY("Cin curioQuickCheckForChipInterrupt");
		IOGetTimestamp(&startTime);
		result = FALSE;
		do {
	    if (CURIOinterruptPending()) {
				result = TRUE;
				break;
			}
			IOGetTimestamp(&endTime);
			elapsedNSec = (unsigned) (endTime - startTime);
		} while (elapsedNSec < 10000U);
		RESULT(result);
		return (result);
}

/*
 * Determine if SCSI interrupt is pending. If so, store the current
 * chip state. Note that reading the interrupt register clears the
 * interrupt source, so this should be done once for each interrupt.
 */
- (Boolean) curioInterruptPending
{
	UInt8		chipStatus;
	Boolean		result = FALSE;


	ENTRY( "Cip curioInterruptPending" );

		/* mlj - use WHILE to check for double interrupts	*/
		/* and get get most recent conditions.				*/
	while (((chipStatus = CURIOreadStatusRegister()) & sIntPend) != 0)
	{
		result = TRUE;
		gSaveStatus		= chipStatus;
		gSaveSeqStep	= CURIOreadSequenceStateRegister();
		gSaveInterrupt	= CURIOreadInterruptRegister();
	}
	RESULT( result );
	return  result;
}/* end curioInterruptPending */


/*
 * Store the current command into the fifo. If this is an autosense,
 * store a REQUEST SENSE command.
 */
- (void)		curioPutCommandIntoFifo
{
		IOSCSIRequest			*scsiReq;
		UInt32					i;
		
		ENTRY("Cpc curioPutCommandIntoFifo");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		scsiReq = gActiveCommand->scsiReq;
		if (gActiveCommand->flagIsAutosense) {
			UInt32				actualAutosenseTransferLength;
			/*
			 * This should be provided by the client (and checked that it is
			 * less than or equal to kMaxAutosenseByteCount). Note that this
			 * sequence is duplicated in Apple96HWPrivate.m.
			 */
			actualAutosenseTransferLength = (sizeof (esense_reply_t) < kMaxAutosenseByteCount)
					? (sizeof (esense_reply_t)) : kMaxAutosenseByteCount;
			ASSERT(actualAutosenseTransferLength <= 255);
			/*
			 * Put the autosense command into the fifo.
			 */
	    CURIOputByteIntoFifo(kScsiCmdRequestSense);
	    CURIOputByteIntoFifo(scsiReq->lun << 5);
	    CURIOputByteIntoFifo(0);
	    CURIOputByteIntoFifo(0);
	    CURIOputByteIntoFifo(actualAutosenseTransferLength);
	    CURIOputByteIntoFifo(0);
		}
		else {
			/*
			 * Put the original command into the fifo.
			 */
			for (i = 0; i < gActiveCommand->cdbLength; i++) {
		CURIOputByteIntoFifo(((UInt8 *) &scsiReq->cdb)[i]);
			}
		}
		EXIT();
}

/*
 * Reset the chip to hardware power on state.
 */
- (void)		curioResetChip
{
		ENTRY("Crc curioResetChip");
	CURIOsetCommandRegister(cRstSChp);	    /* Reset chip	*/
		IODelay(50);									/* Chip settle		*/
	CURIOnop();				    /* Re-enable chip	*/
	CURIOflushFifo();			    /* In a clean state */
	EXIT();
}

/**
 * The caller (hardwareReset and the bus reset interrupt handler) must delay for 250 msec
 * after resetting the bus or detecting a bus reset. This serves two purposes: "The 53C96
 * needs a vacation of about 100 msec after a reset otherwise it gets cranky" -- according
 * to notes in HALc96.c. Also, some devices don't respond if we issue another command too
 * quickly after bus reset.
 */
- (void)	curioResetSCSIBus
{
	ENTRY("Crb curioResetSCSIBus");
	SynchronizeIO();
	CURIOsetCommandRegister(cRstSBus);
		EXIT();
}

#if USE_CURIO_METHODS
/*
 * These are stub methods that implement the exact same code as
 * the inline macros.
 */

/**
 * Retrieve the status register. This can be done without disturbing
 * the chip operation state.
 */
- (UInt8)		curioReadStatusRegister
{
	return (__CURIOreadStatusRegister());
}

/**
 * Retrieve the internal state register. This is done after an interrupt.
 */
- (UInt8)		curioReadSequenceStateRegister
{
	return (__CURIOreadSequenceStateRegister());
}

/**
 * Retrieve the interrupt register. This is done after an interrupt.
 * Note: reading this register "releases" the chip. Registers must
 * be read in the following order:
 *	1.	Status (skip the rest if the interrupt bit is clear)
 *	2.	SequenceStep
 *	3.	Interrupt
 */
- (UInt8)		curioReadInterruptRegister
{
	return (__CURIOreadInterruptRegister());
}

/**
 * The target is in MSGI phase. Start the transfer.
 */
- (void)		curioStartMSGIAction
{
	__CURIOstartMSGIAction();
}

/**
 * CurioNop is a temporizing command to give the chip time to stabalize
 * its internal registers.
 */
- (void)		curioNop
{
	__CURIOnop();				    /* Restart the chip	    */
}

/**
 * Start a byte-by-byte transfer that does not involve the DMA subsystem.
 */
- (void)		curioStartNonDMATransfer
{
	__CURIOstartNonDMATransfer();		/* Start a non-DMA transfer	*/
}

/**
 * Start a DMA transfer.
 */
- (void)		curioStartDMATransfer
						: (UInt32) transferCount
{
		ENTRY("Cds curioStartDMATransfer");
	__CURIOstartDMATransfer(transferCount);
		EXIT();
}

- (void)		curioTransferPad
						: (Boolean) forOutput
{
	__CURIOtransferPad(forOutput);
}

/**
 * CurioClearTransferCountZeroBit
 * This is called after DMA transfers to stabalize the DMA and clear its fifo.
 * It is needed to keep the process alive in the following sequence:
 *	DATO		Start DMA transfer to the target
 *	MSGI		Target changes to message in in order to disconnect.
 */
- (void)		curioClearTransferCountZeroBit
{
	__CURIOclearTransferCountZeroBit();	/* Clear TC zero bit	    */
}

/**
 * Return the current transfer count. This is generally zero after DMA, but will be
 * non-zero if some data was not transferred.
 */
- (UInt32)		curioGetTransferCount
{
		UInt32					result;

		ENTRY("Cgt curioGetTransferCount");
	result = __CURIOgetTransferCount();
		RESULT(result);
		return (result);
}

/** 
 * Load the target destination id register.
 */
- (void)		curioSetDestinationID
						: (UInt8) targetID
{
	__CURIOsetDestinationID(targetID);
}

/**
 * Start an initiator-initiated command-complete sequence.
 */
- (void)		curioInitiatorCommandComplete
{
	__CURIOinitiatorCommandComplete();
}

/**
 * The bus is (presumably) free: allow us to be reselected.
 */
- (void)		curioEnableSelectionOrReselection
{
	__CURIOenableSelectionOrReselection();
}

/**
 * Disconnect from the SCSI bus.
 */
- (void)		curioDisconnect
{
	__CURIOdisconnect();
}


/**
 * Manipulate the FIFO
 * CurioGetFifoByte			Return the current fifo byte.
 * CurioPutByteIntoFifo		Store a byte into the fifo
 * CurioFlushFifo			Clear the fifo
 * CurioGetFifoCount		Get the number of bytes in the fifo.
 *							Note: the fifo only holds 16 bytes. For some reason,
 *							we ignore the 0x10 bit (follows Copland DR1 debugging).
 */
- (UInt8)		curioGetFifoByte
{
	return (__CURIOgetFifoByte());
}

- (void)		curioPutByteIntoFifo
					: (UInt8) theByte
{
	__CURIOputByteIntoFifo(theByte);
}

- (void)		curioFlushFifo
{
	__CURIOflushFifo();
}

- (UInt32)		curioGetFifoCount
{
	return (__CURIOgetFifoCount());
}
		
/**
 * Configure the 53C96 chip.
 */
- (void)		curioSetSelectionTimeout
						: (UInt8) selectionTimeout
{
	__CURIOsetSelectionTimeout(selectionTimeout);
}

- (void)		curioConfigForNonDMA
{
	__CURIOconfigForNonDMA();
}

- (void)		curioConfigForDMA
{
	__CURIOconfigForDMA();
}

/**
 * Manage Message Phase
 */
- (void)		curioMessageAccept
{
	__CURIOmessageAccept();
}

- (void)		curioMessageReject
{
	__CURIOmessageReject();
}

- (void)		curioSetATN
{
	__CURIOsetATN();
}

- (void)		curioClearATN
{
	__CURIOclearATN();
}


/*
 * Sync negotiation methods 
 * To do: don't load a register if its shadow has the same value..
 */
- (void)		curioSetSynchronousOffset
						: (UInt8) synchronousOffset
{
	__CURIOsetSynchronousOffset(synchronousOffset);
}

- (void)		curioSetSynchronousPeriod
						: (UInt8) synchronousPeriod
{
	__CURIOsetSynchronousPeriod(synchronousPeriod);
}

/**
 * Write a random register (used for configuration registers)
 */
- (void)		curioWriteRegister
						: (UInt8)	curioRegister
		value			: (UInt8)	value
{
	__CURIOwriteRegister(curioRegister, value);
}

/**
 * Read a random register (used for configuration registers)
 */
- (UInt8)		curioReadRegister
						: (UInt8)	curioRegister
{
	return (__CURIOreadRegister(curioRegister));
}

/**
 * This is the only place that the command register is written.
 */
- (void)		curioSetCommandRegister
		: (UInt8) commandByte
{
	__CURIOsetCommandRegister(commandByte);
}

/**
 * Error recovery needs to peek at the last command
 */
- (UInt8)		curioReadCommandRegister
{
	return (__CURIOreadCommandRegister());
}

/*
 * Convert the selection timeout to the value that must be stored in the
 * chip registers.
 */
- (UInt32)  curioSelectTimeout
			: (UInt32)  selectTimeoutMSec
    curioClockMHz	: (UInt32)  chipClockRateMHz
    curioClockFactor	: (UInt32)  chipClockFactor
{
	return (
		__CURIOsetSelectTimeout(
		    selectTimeoutMSec,
		    chipClockRateMHz,
		    chipClockFactor)
	    );
}
#endif /* USE_CURIO_METHODS */

@end /* Apple96_SCSI(Curio) */

#if USE_CURIO_METHODS || WRITE_CHIP_OPERATION_INTO_TIMESTAMP_LOG
volatile UInt8
__CurioReadRegister__(
		volatile UInt8		*scsiLogicalAddress,
		UInt32				index
    )
{
	volatile UInt8			result = scsiLogicalAddress[index];
	const char				*why;
	const char				*what;
	switch (index) {
	case rXCL:	what	= "XCL";	break;
	case rXCM:	what	= "XCM";	break;
	case rFFO:	what	= "FFO";	break;
	case rCMD:	what	= "CMD";	break;
	case rSTA:	what	= "STA";	break;
	case rINT:	what	= "INT";	break;
	case rSQS:	what	= "SQS";	break;
	case rFOS:	what	= "FOS";	break;
	case rCF1:	what	= "CF1";	break;
	case rCKF:	what	= "CKF";	break;
	case rTST:	what	= "TST";	break;
	case rCF2:	what	= "CF2";	break;
	case rCF3:	what	= "CF3";	break;
	case rCF4:	what	= "CF4";	break;
	case rTCH:	what	= "TCH";	break;
	case rDMA:	what	= "DMA";	break;
	default:	what	= "?\?\?";	break;
	}
	RAW_TAG(OSTag('<', what), (index << 8) | result);
	return (result);
}

void
__CurioWriteRegister__(
		volatile UInt8		*scsiLogicalAddress,
		UInt32				index,
		UInt8				value
    )
{
	const char				*what;
	switch (index) {
	case rXCL:	what	= "XCL";	break;
	case rXCM:	what	= "XCM";	break;
	case rFFO:	what	= "FFO";	break;
	case rCMD:	what	= "CMD";	break;
	case rSTA:	what	= "DST";	break;
	case rINT:	what	= "TMO";	break;
	case rSQS:	what	= "SQS";	break;
	case rFOS:	what	= "FOS";	break;
	case rCF1:	what	= "CF1";	break;
	case rCKF:	what	= "CKF";	break;
	case rTST:	what	= "TST";	break;
	case rCF2:	what	= "CF2";	break;
	case rCF3:	what	= "CF3";	break;
	case rCF4:	what	= "CF4";	break;
	case rTCH:	what	= "TCH";	break;
	case rDMA:	what	= "DMA";	break;
	default:	what	= "?\?\?";	break;
	}
	RAW_TAG(OSTag('>', what), (index << 8) | value);
	scsiLogicalAddress[index] = value;
}
#endif /* USE_CURIO_METHODS */

