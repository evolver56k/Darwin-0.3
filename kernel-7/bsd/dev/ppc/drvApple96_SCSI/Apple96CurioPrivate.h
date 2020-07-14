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
 * Copyright 1993-1995 by Apple Computer, Inc., all rights reserved.
 * Copyright 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * Apple96Curio.h - Hardware (chip) definitions for Apple 53c96 SCSI interface.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver
 * using design concepts from Copland DR2 Curio and MESH SCSI plugins.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.07.18	MM	Added USE_CURIO_METHODS and wrote macros for the
 *			one-liner methods.
 */

#import "Apple96Curio.h"

/* ** ** **
 * ** ** ** Warning: all macros that take arguments must safely handle
 * ** ** ** side-effects. For example CURIOputByteIntoFifo(*ptr++)
 * ** ** ** must increment the pointer once only.
 * ** ** **
 */

/*
 * CurioReadRegister and CurioWriteRegister actually touch the hardware
 */
#if USE_CURIO_METHODS || WRITE_CHIP_OPERATION_INTO_TIMESTAMP_LOG
extern volatile UInt8 __CurioReadRegister__(
		volatile UInt8		*scsiLogicalAddress,
		UInt32				index
    );
extern void __CurioWriteRegister__(
		volatile UInt8		*scsiLogicalAddress,
		UInt32				index,
		UInt8				value
    );

#else
#define __CurioReadRegister__(scsiLogicalAddress, index)	    \
	(((volatile UInt8 *) scsiLogicalAddress)[index])
#define __CurioWriteRegister__(scsiLogicalAddress, index, value)    \
	((((volatile UInt8 *) scsiLogicalAddress)[index]) = value)  
#endif /* USE_CURIO_METHODS */
/*
 * These macros implement the one-line methods. They are included inline
 * if USE_CURIO_METHODS is FALSE, or provided by the method implementation
 * if USE_CURIO_METHODS is TRUE.
 */
#define CurioReadRegister(index)				\
	    __CurioReadRegister__(gSCSILogicalAddress, (index))
#define CurioWriteRegister(index, value)			\
	    __CurioWriteRegister__(gSCSILogicalAddress, (index), (value))
#define __CURIOreadStatusRegister()	(CurioReadRegister(rSTA))
#define __CURIOreadSequenceStateRegister() (CurioReadRegister(rSQS))
#define __CURIOreadInterruptRegister()	(CurioReadRegister(rINT))
#define __CURIOstartMSGIAction()	do {			\
	    __CURIOnop();					    \
	    __CURIOstartNonDMATransfer();			    \
	} while (0)
#define __CURIOnop()			__CURIOsetCommandRegister(cNOP)
#define __CURIOstartNonDMATransfer()	__CURIOsetCommandRegister(cIOXfer)
#define __CURIOstartDMATransfer(count)	do {			\
	    UInt32	    __count = (count);			\
	    __CURIOwriteRegister(rXCM, (__count >> 8) & 0xFF);	\
	    SynchronizeIO();					\
	    __CURIOwriteRegister(rXCL, __count & 0xFF);		\
	    SynchronizeIO();					\
	    __CURIOsetCommandRegister(cDMAXfer);		    \
	} while (0)
#define __CURIOtransferPad(forOutput)	do {			\
	    __CURIOwriteRegister(rXCM, 0xFF);			    \
	    SynchronizeIO();					\
	    __CURIOwriteRegister(rXCL, 0xFF);			    \
	    SynchronizeIO();					\
	    __CURIOsetCommandRegister((forOutput) ? cDMAXferPad : cXferPad);	\
	} while (0)
#define __CURIOclearTransferCountZeroBit() __CURIOsetCommandRegister(cNOP | bDMDEnBit)
#define __CURIOgetTransferCount() (				\
	    (				    \
	    (  ((__CURIOreadRegister(rTCH) & 0xFF) << 16)	    \
	     | ((__CURIOreadRegister(rXCM) & 0xFF) <<  8)	    \
	     | ((__CURIOreadRegister(rXCL) & 0xFF) <<  0)	    \
	    )))
#define __CURIOsetDestinationID(target) do {			\
	    __CURIOwriteRegister(rSTA, target);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOinitiatorCommandComplete()   __CURIOsetCommandRegister(cCmdComp)
#define __CURIOenableSelectionOrReselection() __CURIOsetCommandRegister(cEnSelResel)
#define __CURIOdisconnect()	    __CURIOsetCommandRegister(cDisconnect)
/*
 * Execute __CURIOgetFifoByte using the following sequence:
 *	SynchronizeIO();
 *	result = __CURIOgetFifoByte();
 */
#define __CURIOgetFifoByte() ( (UInt8) (			\
	    __CURIOreadRegister(rFFO)				    \
	))
#define __CURIOputByteIntoFifo(theByte) do {			\
	    __CURIOwriteRegister(rFFO, (theByte));		\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOflushFifo()		do {			\
	    __CURIOsetCommandRegister(cFlshFFO);		    \
	    __CURIOnop(); /* Mandatory 360 nsec delay */	    \
	    __CURIOnop();					    \
	} while (0)
/*
 * Execute __CURIOgetFifoCount using the following sequence:
 *	SynchronizeIO();
 *	result = __CURIOgetFifoCount();
 */
#define __CURIOgetFifoCount() ( (UInt8) (			\
	    (__CURIOreadRegister(rFOS) & kFIFOCountMask)	    \
	))
#define __CURIOsetSelectionTimeout(tmo) do {			\
	    UInt8   __selectionTimeout = (tmo);			\
	    gLastSelectionTimeout = __selectionTimeout;		\
	    __CURIOwriteRegister(rINT, __selectionTimeout);	\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOconfigForNonDMA()	do {			\
	    __CURIOwriteRegister(rCF3, CONFIG_FOR_NON_DMA);	\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOconfigForDMA()		do {			\
	    __CURIOwriteRegister(rCF3, CONFIG_FOR_DMA);		\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOmessageAccept()		do {			\
	    __CURIOsetCommandRegister(cMsgAcep);		    \
	    SynchronizeIO();					\
	} while (0)
#define __CURIOmessageReject()		do {			\
	    __CURIOputByteIntoFifo(kScsiMsgRejectMsg);		\
	    __CURIOsetATN();					    \
	} while (0)
#define __CURIOsetATN()			do {			\
	    __CURIOsetCommandRegister(cSetAtn);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOclearATN()		do {			\
	    __CURIOsetCommandRegister(cRstAtn);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOsetSynchronousOffset(off) do {			\
	    UInt8 __off = (off);				\
	    gLastSynchronousOffset = __off;			\
	    __CURIOwriteRegister(rFOS, __off);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOsetSynchronousPeriod(per) do {			\
	    UInt8 __per = (per);				\
	    gLastSynchronousPeriod = __per;			\
	    __CURIOwriteRegister(rSQS, __per);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOwriteRegister(reg, val)	do {			\
	    UInt8 __reg = (reg);				\
	    UInt8 __val = (val);				\
	    TAG("wri", (__reg << 8) | __val);			\
	    CurioWriteRegister(__reg, __val);			\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOreadRegister(reg)	CurioReadRegister(reg)
#define __CURIOsetCommandRegister(cmdByte) do {			\
	    UInt8 __cmdByte = (cmdByte);			\
	    ddmChip("curioSetCommandRegister byte = %x\n", __cmdByte, 2, 3, 4, 5); \
	    SynchronizeIO();	/* paranoia */			\
	    CurioWriteRegister(rCMD, __cmdByte);		\
	    SynchronizeIO();					\
	} while (0)
#define __CURIOreadCommandRegister()	__CURIOreadRegister(rCMD)
#define __CURIOsetSelectTimeout(tmo, rate, factor) (( (UInt32) (    \
	    (((UInt32) rate) * ((UInt32) tmo))	/ (7682 * ((UInt32) factor)) \
	)))


#if USE_CURIO_METHODS
/*
 * Expand the one-liner macros as methods. This is useful for
 * debugging as methods can be logged.
 */
#define CURIOreadStatusRegister()	[self curioReadStatusRegister]
#define CURIOreadSequenceStateRegister() [self curioReadSequenceStateRegister]
#define CURIOreadInterruptRegister()	[self curioReadInterruptRegister]
#define CURIOstartMSGIAction()		[self curioStartMSGIAction]
#define CURIOnop()			[self curioNop]
#define CURIOstartNonDMATransfer()	[self curioStartNonDMATransfer]
#define CURIOstartDMATransfer(count)	[self curioStartDMATransfer : (count)]
#define CURIOtransferPad(forOutput)	[self curioTransferPad : (forOutput)]
#define CURIOclearTransferCountZeroBit() [self curioClearTransferCountZeroBit]
#define CURIOgetTransferCount()		[self curioGetTransferCount]
#define CURIOsetDestinationID(target)	[self curioSetDestinationID : (target)]
#define CURIOinitiatorCommandComplete() [self curioInitiatorCommandComplete]
#define CURIOenableSelectionOrReselection() [self curioEnableSelectionOrReselection]
#define CURIOdisconnect()		[self curioDisconnect]
#define CURIOgetFifoByte()		[self curioGetFifoByte]
#define CURIOputByteIntoFifo(theByte)	[self curioPutByteIntoFifo : (theByte)]
#define CURIOflushFifo()		[self curioFlushFifo]
#define CURIOgetFifoCount()		[self curioGetFifoCount]
#define CURIOsetSelectionTimeout(tmo)	[self curioSetSelectionTimeout : (tmo)]
#define CURIOconfigForNonDMA()		[self curioConfigForNonDMA]
#define CURIOconfigForDMA()		[self curioConfigForDMA]
#define CURIOmessageAccept()		[self curioMessageAccept]
#define CURIOmessageReject()		[self curioMessageReject]
#define CURIOsetATN()			[self curioSetATN]
#define CURIOclearATN()			[self curioClearATN]
#define CURIOsetSynchronousOffset(off)	[self curioSetSynchronousOffset : (off)]
#define CURIOsetSynchronousPeriod(per)	[self curioSetSynchronousPeriod : (per)]
#define CURIOwriteRegister(reg, val)	[self curioWriteRegister : (reg) value : (val)]
#define CURIOreadRegister(reg)		[self curioReadRegister : (reg)]
#define CURIOsetCommandRegister(cmdByte) [self curioSetCommandRegister : (cmdByte)]
#define CURIOreadCommandRegister()	[self curioReadCommandRegister]
#define CURIOsetSelectTimeout(tmo, rate, factor)	\
	    [self curioSetSelectTimeout : (tmo)	    \
		    curioClockMHz	: (rate)    \
		    curioClockFactor	: (factor)  \
		]
#else
/*
 * Define the public CURIOxxx macros as private macros
 */
#define CURIOreadStatusRegister()	__CURIOreadStatusRegister()
#define CURIOreadSequenceStateRegister() __CURIOreadSequenceStateRegister()
#define CURIOreadInterruptRegister()	__CURIOreadInterruptRegister()
#define CURIOstartMSGIAction()		__CURIOstartMSGIAction()
#define CURIOnop()			__CURIOnop()
#define CURIOstartNonDMATransfer()	__CURIOstartNonDMATransfer()
#define CURIOstartDMATransfer(count)	__CURIOstartDMATransfer(count)
#define CURIOtransferPad(forOutput)	__CURIOtransferPad(forOutput)
#define CURIOclearTransferCountZeroBit() __CURIOclearTransferCountZeroBit()
#define CURIOgetTransferCount()		__CURIOgetTransferCount()
#define CURIOsetDestinationID(target)	__CURIOsetDestinationID(target)
#define CURIOinitiatorCommandComplete() __CURIOinitiatorCommandComplete()
#define CURIOenableSelectionOrReselection() __CURIOenableSelectionOrReselection()
#define CURIOdisconnect()		__CURIOdisconnect()
#define CURIOgetFifoByte()		__CURIOgetFifoByte()
#define CURIOputByteIntoFifo(theByte)	__CURIOputByteIntoFifo(theByte)
#define CURIOflushFifo()		__CURIOflushFifo()
#define CURIOgetFifoCount()		__CURIOgetFifoCount()
#define CURIOsetSelectionTimeout(tmo)	__CURIOsetSelectionTimeout(tmo)
#define CURIOconfigForNonDMA()		__CURIOconfigForNonDMA()
#define CURIOconfigForDMA()		__CURIOconfigForDMA()
#define CURIOmessageAccept()		__CURIOmessageAccept()
#define CURIOmessageReject()		__CURIOmessageReject()
#define CURIOsetATN()			__CURIOsetATN()
#define CURIOclearATN()			__CURIOclearATN()
#define CURIOsetSynchronousOffset(off)	__CURIOsetSynchronousOffset(off)
#define CURIOsetSynchronousPeriod(per)	__CURIOsetSynchronousPeriod(per)
#define CURIOwriteRegister(reg, val)	__CURIOwriteRegister(reg, val)
#define CURIOreadRegister(reg)		__CURIOreadRegister(reg)
#define CURIOsetCommandRegister(cmdByte) __CURIOsetCommandRegister(cmdByte)
#define CURIOreadCommandRegister()	__CURIOreadCommandRegister()
#define CURIOsetSelectTimeout(tmo, rate, factor) __CURIOsetSelectTimeout(tmo, rate, factor)
#endif /* USE_CURIO_METHODS */
/*
 * These "macros" always expand to method calls. This keeps the mainline
 * source code looking consistent. Methods were chosen either because
 * the code is messy or because it is seldom used (or both).
 */
#define CURIOquickCheckForChipInterrupt() [self curioQuickCheckForChipInterrupt]
#define CURIOinterruptPending()		[self curioInterruptPending]
#define CURIOputCommandIntoFifo()	[self curioPutCommandIntoFifo]
#define CURIOresetChip()		[self curioResetChip]
#define CURIOresetSCSIBus()		[self curioResetSCSIBus]

@interface Apple96_SCSI(CurioPrivate)

/*
 * Private methods
 */
- (Boolean)		curioQuickCheckForChipInterrupt;
- (Boolean)		curioInterruptPending;
- (void)		curioPutCommandIntoFifo;
- (void)		curioResetChip;
- (void)	curioResetSCSIBus;

#if USE_CURIO_METHODS
- (UInt8)		curioReadStatusRegister;
- (UInt8)		curioReadSequenceStateRegister;
- (UInt8)		curioReadInterruptRegister;
- (void)		curioStartMSGIAction;
- (void)		curioNop;
- (void)		curioStartNonDMATransfer;
- (void)		curioStartDMATransfer
						: (UInt32) transferCount;
- (void)		curioTransferPad
						: (Boolean) forOutput;
- (void)		curioClearTransferCountZeroBit;
- (UInt32)		curioGetTransferCount;
- (void)		curioSetDestinationID
						: (UInt8) targetID;
- (void)		curioInitiatorCommandComplete;
- (void)		curioEnableSelectionOrReselection;
- (void)		curioDisconnect;
- (UInt8)		curioGetFifoByte;
- (void)		curioPutByteIntoFifo
						: (UInt8)	value;
- (void)		curioFlushFifo;
- (UInt32)		curioGetFifoCount;
- (void)		curioSetSelectionTimeout
						: (UInt8)	selectionTimeout;
- (void)		curioConfigForNonDMA;
- (void)		curioConfigForDMA;
- (void)		curioMessageAccept;
- (void)		curioMessageReject;
- (void)		curioSetATN;
- (void)		curioClearATN;
- (void)		curioSetSynchronousOffset
						: (UInt8) synchronousOffset;
- (void)		curioSetSynchronousPeriod
						: (UInt8) synchronousPeriod;
- (void)		curioWriteRegister
						: (UInt8)	curioRegister
		value			: (UInt8)	value;
- (UInt8)		curioReadRegister
						: (UInt8)	curioRegister;
- (void)		curioSetCommandRegister
						: (UInt8) commandByte;
- (UInt8)		curioReadCommandRegister;
- (UInt32)  curioNanosecondToSyncPeriod
			: (UInt8)   nsecPeriod	/* Desired period	    */
	fastSCSI	: (UInt32)  fastSCSI	/* Control3.CR3_FAST_SCSI   */
	clockRate	: (UInt32)  clockRate;	/* in MHz		    */
- (UInt32)  curioSelectTimeout
			: (UInt32)  selectTimeoutMSec
    curioClockMHz	: (UInt32)  chipClockRateMHz
    curioClockFactor	: (UInt32)  chipClockFactor;
#endif /* USE_CURIO_METHODS */
@end /* Apple96_SCSI(CurioPrivate) */




		
