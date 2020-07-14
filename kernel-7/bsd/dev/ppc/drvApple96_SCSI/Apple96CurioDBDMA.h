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
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.07.18	MM	Added USE_CURIO_METHODS and wrote macros for the
 *			one-liner methods.
 */

#import "Apple96SCSI.h"

/**
 * Copyright 1984-1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from Copland D12 DBDMA.Rev 9 sources.
 *
 * Set tabs every 4 characters.
 *
 * AppleDBDMADefinitions.h - registers and inline functions for the DBDMA memory controller.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 */

#import "Apple96DBDMADefs.h"
#import <machdep/ppc/proc_reg.h>

#if 1
/**
 * This is a temporary implementation of EndianSwap32Bit until
 * the correct library/method is made available.
 * @param	value		The value to change
 * @result	The value endian-swapped.
 */
static inline unsigned EndianSwap32Bit(
		unsigned			value
	)
{

		register unsigned	temp;
	
		temp = ((value & 0xFF000000) >> 24);
		temp |= ((value & 0x00FF0000) >> 8);
		temp |= ((value & 0x0000FF00) << 8);
		temp |= ((value & 0x000000FF) << 24);
		return (temp);
}
#endif

/*
 * Don't Endian-swap register read/write operations.
 */
#define __DBDMA_READ(reg)	EndianSwap32Bit(*((volatile UInt32 *) &gDBDMALogicalAddress->reg))
#define __DBDMA_WRITE(reg, value) do {									\
		*((volatile UInt32 *) &gDBDMALogicalAddress->reg) = EndianSwap32Bit(value);	\
		SynchronizeIO();												\
	} while (0)

#define DBDMASetChannelControl(ctlValue) do {		\
		SynchronizeIO();							\
		__DBDMA_WRITE(channelControl, ctlValue);	\
		SynchronizeIO();							\
	} while (0)
#define DBDMAGetChannelStatus()					__DBDMA_READ(channelStatus)
#define DBDMAGetCommandPtr()					__DBDMA_READ(commandPtrLo)
#define DBDMASetCommandPtr(cclPtr) 				__DBDMA_WRITE(commandPtrLo, cclPtr)
#define DBDMAGetInterruptSelect()				__DBDMA_READ(interruptSelect)
#define DBDMASetInterruptSelect(intSelValue)	__DBDMA_WRITE(interruptSelect, intSelValue)
#define DBDMAGetBranchSelect()					__DBDMA_READ(branchSelect)
#define DBDMASetBranchSelect(braSelValue)		__DBDMA_WRITE(branchSelect, braSelValue)
#define DBDMAGetWaitSelect()					__DBDMA_READ(waitSelect)
#define DBDMASetWaitSelect(waitSelValue)		__DBDMA_WRITE(waitSelect, waitSelValue)


/*
 * Construct a DBDMA Channel Command descriptor with no command dependencies.
 * Opcode and address may not have side-effects.
 *  void MakeCCDescriptor(
 *			DBDMADescriptor		descriptorPtr,
 *			unsigned long		opcode,
 *			unsigned long		addr		-- physical address
 *		);
 */
#define MakeCCDescriptor(descriptorPtr, opcode, addr) do {			\
		(descriptorPtr)->address = EndianSwap32Bit(addr);			\
		(descriptorPtr)->cmdDep = 0;								\
		(descriptorPtr)->result = EndianSwap32Bit(0xDEADBEEF);		\
		sync();														\
		(descriptorPtr)->operation = EndianSwap32Bit(opcode);		\
		sync();														\
	} while (0)
/*
 * Construct a DBDMA Channel Command descriptor with command dependencies.
 * Opcode, address, and dependency may not have side-effects.
 *  void MakeCmdDepCCDescriptor(
 *			DBDMADescriptor		descriptorPtr,
 *			unsigned long		opcode,
 *			unsigned long		addr,	-- physical address
 *	    unsigned long	dependency
 *		);
 */
#define MakeCmdDepCCDescriptor(descriptorPtr, opcode, addr, dep) do {	\
		(descriptorPtr)->address = EndianSwap32Bit(addr);			\
		(descriptorPtr)->cmdDep = EndianSwap32Bit(dep);				\
		(descriptorPtr)->result = EndianSwap32Bit(0xDEADBEEF);		\
		sync();														\
		(descriptorPtr)->operation = EndianSwap32Bit(opcode);		\
		sync();														\
	} while (0)
#define GetCCOperation(descriptorPtr)								\
		(EndianSwap32Bit((descriptorPtr)->operation))
#define SetCCOperation(descriptorPtr, opcode)						\
		((descriptorPtr)->operation = EndianSwap32Bit(opcode))
#define GetCCAddress(descriptorPtr)									\
		(EndianSwap32Bit((descriptorPtr)->address))
#define SetCCAddress(descriptorPtr, addr)							\
		((descriptorPtr)->address = EndianSwap32Bit(addr))
#define GetCCCmdDep(descriptorPtr)									\
		(EndianSwap32Bit((descriptorPtr)->cmdDep))
#define SetCCCmdDep(descriptorPtr, cmdDepValue)						\
		((descriptorPtr)->cmdDep = EndianSwap32Bit(cmdDepValue))
#define GetCCResult(descriptorPtr)									\
		(EndianSwap32Bit((descriptorPtr)->result))
#define SetCCResult(descriptorPtr, resultValue)						\
		((descriptorPtr)->result = EndianSwap32Bit(resultValue))

/*
 * These macros implement the DBDMA interface. 
 */
#define __DBDMAreset() do {			\
		DBDMASetChannelControl(			\
			kdbdmaClrRun			\
			| kdbdmaClrPause		\
			| kdbdmaClrDead			\
		);			    \
    } while (0)

#define __DBDMAspinUntilIdle() do {		    \
		SynchronizeIO();		    \
	} while ((DBDMAGetChannelStatus() & kdbdmaActive) != 0);    \

#define __DBDMAstartTransfer() do {		    \
		DBDMASetChannelControl(kdbdmaSetRun | kdbdmaSetWake);	\
    } while (0)

#define __DBDMAstopTransfer() __DBDMAreset()

#if USE_CURIO_METHODS
#define DBDMAreset()	    [self dbdmaReset]
#define DBDMAstartTransfer()	[self dbdmaStartTransfer]
#define DBDMAstopTransfer() [self dbdmaStopTransfer]
#define DBDMAspinUntilIdle()	[self dbdmaSpinUntilIdle]
#else
#define DBDMAreset()	    __DBDMAreset()
#define DBDMAstartTransfer()	__DBDMAstartTransfer()
#define DBDMAstopTransfer() __DBDMAstopTransfer()
#define DBDMAspinUntilIdle()	__DBDMAspinUntilIdle()
#endif /* USE_CURIO_METHODS */

@interface Apple96_SCSI(Curio_DBDMA)

/**
 * Quit: stop the DBDMA controller and free all memory.
 * This is always a method.
 */
- (void) dbdmaTerminate;

#if USE_CURIO_METHODS
/**
 * Stop the DBDMA controller
 */
- (void) dbdmaReset;

/**
 * Start a DMA operation.
 */
- (void) dbdmaStartTransfer;

/**
 * Stop a DMA operation.
 */
- (void) dbdmaStopTransfer;

/**
 * Spin-wait until the DBDMA channel is inactive. This should be
 * timed to prevent deadlock.
 */
- (void) dbdmaSpinUntilIdle;

#endif /* USE_CURIO_METHODS */
@end

