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
 * Apple96PCISCSI.m - Architecture-specific methods for Apple96 SCSI driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.07.31	MM		Radar 1670762: Correct transfer length for > 64K transfers.
 *						rewrote hardwareGetNextContiguousPhysicalTransferRange.
 *						This no longer combines adjacent physical pages, which
 *						greatly simplifies hardwareInitializeRequestCCL (which was
 *						also rewritten). Remove hard-wired logical page size values.
 */
#import "Apple96SCSI.h"
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import	<driverkit/align.h>
#import <kernserv/prototypes.h>
#import <mach/kern_return.h>
#import <mach/mach_interface.h>
#import "Apple96SCSIPrivate.h"
#import "Apple96HWPrivate.h"
#import "Apple96CurioDBDMA.h"
#import "Apple96CurioPrivate.h"
#import "bringup.h"
#import "Apple96Curio.h"
#import "MacSCSICommand.h"
/* bzero is defined in string.h */
#import <string.h>

enum {
	kCURIORegisterBase	= 0,
	kDBDMARegisterBase	= 1
};

/**
 * These are the hardware-specific methods.
 */
@implementation Apple96_SCSI(HardwarePrivate)

/**
 * Fetch the device's bus address and allocate one page
 * of memory for the channel command. (Strictly speaking, we don't
 * need an entire page, but we can use the rest of the page for
 * a permanent status log).
 *
 * @param	deviceDescription	Specify the device to initialize.
 * @return	IO_R_SUCCESS if successful, else an error status.
 */
- (IOReturn) hardwareAllocateHardwareAndChannelMemory
						: deviceDescription
{
		IOReturn			ioReturn = IO_R_SUCCESS;
		enum {
			kCurioRegisterBase	= 0,
			kDBDMARegisterBase	= 1,
			kNumberRegisters	= 2
		};
				
		ENTRY("Hal hardwareAllocateHardwareAndChannelMemory");
		gSCSIPhysicalAddress	= 0;
		gSCSIRegisterLength	 	= 0;
		gSCSILogicalAddress		= NULL;
		gDBDMAPhysicalAddress	= 0;
		gDBDMARegisterLength	= 0;
		gDBDMALogicalAddress	= NULL;
		/*
		 * Set the default selection timeout to the MESH value (10 msec units).
		 */
	//	gSelectionTimeout		= 250 / 10;
		/*
		 * Retrieve the system-wide logical page size and define a mask that
		 * contains only the low-order bits. Part of Radar 1670762.
		 */
		gPageSize				= page_size;
		gPageMask				= gPageSize - 1;
		/*
		 * Allocate a page of wired-down memory in the kernel. Although
		 * Driver Kit provides a memory allocator, IOMalloc, it does
		 * not guarantee page alignment. Thus, we call the Mach kernel
		 * routine. According to the description of kalloc(), 4096 is
		 * the smallest amount of memory we can allocate. The channel
		 * command area will fit into the start of this area and the
		 * autosense area will be placed at the end.
		 */
		gChannelCommandAreaSize = gPageSize;
		gChannelCommandArea = (DBDMADescriptor *) kalloc(gChannelCommandAreaSize);
		if (gChannelCommandArea == NULL) {
	    IOLog("%s: Cannot allocate channel command area %d bytes, fatal.\n",
				[self name],
				gChannelCommandAreaSize
			);
			ioReturn = IO_R_NO_MEMORY;
		}
		if (ioReturn == IO_R_SUCCESS) {
			if (IOIsAligned(gChannelCommandArea, gPageSize) == 0) {
		IOLog("%s: Command area at %08x is not page-aligned.\n",
					[self name],
					(UInt32) gChannelCommandArea
				);
				ioReturn = IO_R_NO_MEMORY;
			}
		}
		if (ioReturn == IO_R_SUCCESS) {

			/*
			 * Curio-specific: use the high-end of the descriptor area for
			 * the bit bucket and autosense buffer.
			 */
			UInt32			autosenseSize;
			
			autosenseSize = kMaxAutosenseByteCount;
			gAutosenseArea = (esense_reply_t *)
				(((UInt32) gChannelCommandArea)
				+ gChannelCommandAreaSize
				- autosenseSize);
			/*
			 * Determine the number of DBDMA descriptors that fit in
			 * the channel command area.
			 */
			gDBDMADescriptorMax = (gChannelCommandAreaSize - autosenseSize)
								/ sizeof (DBDMADescriptor);
			/*
			 * Fetch the logical and physical addresses for the
			 * Curio and DBDMA chips.
			 */
#if 0
	    do {
		IORange	    *memoryRangeList;
		UInt32	    numMemoryRanges;
		int	i;

			memoryRangeList		= [deviceDescription memoryRangeList];
			numMemoryRanges		= [deviceDescription numMemoryRanges];
			for (i = 0; i < numMemoryRanges; i++) {
		    ddmDMA("%s: memory range[%d] %08x (%d)\n",
					[self name],
					i,
					memoryRangeList[i].start,
					memoryRangeList[i].size
				);
			}
			if (numMemoryRanges != kNumberRegisters) {
		    ddmDMA("%s: Expect %d memory ranges, got %d. Fatal.\n",
					[self name],
					kNumberRegisters,
					numMemoryRanges
				);
				ioReturn = IO_R_INVALID;	/* This "can't happen"	*/
			}
	    } while (0);
#endif
		}
#if 0
		if (ioReturn == IO_R_SUCCESS) {		
			/*
			 * We know that the first range describes the SCSI chip,
			 * and the second range describes the DBDMA chip.
			 */
	    gSCSIPhysicalAddress    = (PhysicalAddress) memoryRangeList[kCURIORegisterBase].start;
			gSCSIRegisterLength	 	= memoryRangeList[kCURIORegisterBase].size;
	    gDBDMAPhysicalAddress   = (PhysicalAddress) memoryRangeList[kDBDMARegisterBase].start;
	    gDBDMARegisterLength    = memoryRangeList[kDBDMARegisterBase].size;
			/*
			 * Weave together the logical and physical addresses.
			 * First, map the SCSI and DBDMA chips into our address space.
			 */
			ioReturn = IOMapPhysicalIntoIOTask(
						(UInt32) gSCSIPhysicalAddress,
						gSCSIRegisterLength,
						(vm_offset_t *) &gSCSILogicalAddress
					);
			if (ioReturn != IO_R_SUCCESS) {
		ddmDMA("%s: Mapping SCSI chip at %08x [%d] to virtual address:"
					" error %d (%s), fatal.\n",
					[self name],
					gSCSIPhysicalAddress,
					gSCSIRegisterLength,
					ioReturn,
					[self stringFromReturn:ioReturn]
				);
			}
		}
		if (ioReturn == IO_R_SUCCESS) {		
			ioReturn = IOMapPhysicalIntoIOTask(
						(UInt32) gDBDMAPhysicalAddress,
						gDBDMARegisterLength,
						(vm_offset_t *) &gDBDMALogicalAddress
					);
			if (ioReturn != IO_R_SUCCESS) {
		ddmDMA("%s: Mapping DBDMA at %08x [%d] to virtual address:"
					" error %d (%s), fatal.\n",
					[self name],
					gDBDMAPhysicalAddress,
					gDBDMARegisterLength,
					ioReturn,
					[self stringFromReturn:ioReturn]
				);
			}
		}
#else
		/*
		 * Temp (?) Use hard-wired addresses until the registry is complete.
		 */
		if (ioReturn == IO_R_SUCCESS) {		
	    gSCSILogicalAddress     = (UInt8 *) PCI_ASC_BASE_PHYS;
	    gSCSIPhysicalAddress    = (PhysicalAddress) PCI_ASC_BASE_PHYS;
	    gSCSIRegisterLength = 0x100;
	    gDBDMALogicalAddress    = (DBDMAChannelRegisters *) PCI_DMA_BASE_PHYS;
	    gDBDMAPhysicalAddress   = (PhysicalAddress) PCI_DMA_BASE_PHYS;
	    gDBDMARegisterLength    = 0x20;
		}
#endif
		if (ioReturn == IO_R_SUCCESS) {		
			/*
			 * Ensure that the addresses are valid.
			 */
#if 0 /* probe_rb is not present (yet?) */
			ASSERT(probe_rb((UInt8 *) gSCSILogicalAddress) == 0);
			ASSERT(probe_rb((UInt8 *) gDBDMALogicalAddress) == 0);
#endif
			/*
			 * Get the physical address corresponding the DBDMA channel area.
			 */
			ioReturn = IOPhysicalFromVirtual(
						IOVmTaskSelf(),
						(vm_offset_t) gChannelCommandArea,
						(vm_offset_t *) &gDBDMAChannelAddress
					);
			if (ioReturn != IO_R_SUCCESS) {
		IOLog("%s: Mapping channel at %08x to physical address: error %d (%s), fatal.\n",
					[self name],
					(UInt32) gChannelCommandArea,
					ioReturn,
					[self stringFromReturn:ioReturn]
				);
			}
		}
		if (ioReturn == IO_R_SUCCESS) {
			gAutosenseAddress = gDBDMAChannelAddress
				+ (((UInt32) gAutosenseArea) - ((UInt32) gChannelCommandArea));
			[self initializeChannelProgram];
		}
		/*
		 * What do we do on failure? Should we try to deallocate the stuff
		 * we created, or will the system do this for us?
		 */
		RESULT(ioReturn);
		return (ioReturn);
}

/**
 * Perform one-time-only channel command program initialization.
 */
- (void) initializeChannelProgram
{
		ENTRY("Hic initializeChanelProgram");
		/*
		 * Set the interrupt, branch, and wait DBDMA registers.
		 * Caution: the following MESH interrupt register bits are
		 * reverse polarity and are in a different position.
		 * The pattern is: 0x00MM00VV, where MM is a mask byte
		 * and VV is a value byte to match.
		 *	0x80	means NO errors			(kMeshIntrError)
		 *	0x40	means NO exceptions		(kMeshIntrException)
		 *	0x20	means NO command done	(kMeshIntrCmdDone)
		 *	Branch Select is used with BRANCH_FALSE
		 */
		DBDMASetInterruptSelect(0x00000000);	/* Never let DBDMA interrupt	*/
		DBDMASetWaitSelect(0x00200020);			/* Wait until command done		*/
		DBDMASetBranchSelect(0x00000000);		/* Never branch on error		*/
		EXIT();
}

/**
 * When a (legitimate) data phase starts, this method is called to configure
 * the DBDMA Channel Command list. Autosense is simple (as we "cannot" be
 * called more than once), while ordinary data transfers are arbitrarily complex.
 */
- (IOReturn) hardwareInitializeCCL
{
		IOReturn			ioReturn;
		
		ENTRY("Hcc hardwareInitializeCCL");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		/**
		 * First, make sure that the DBDMA chip is idle.
		 */
	DBDMAreset();
	DBDMAspinUntilIdle();
		if (gActiveCommand->flagIsAutosense) {
			UInt32				actualAutosenseTransferLength;
			/*
			 * This should be provided by the client (and checked that it is
			 * less than or equal to kMaxAutosenseByteCount). Note that this
			 * sequence is duplicated in Apple96Curio.m.
			 */
			actualAutosenseTransferLength = sizeof (esense_reply_t);
			ASSERT(actualAutosenseTransferLength <= kMaxAutosenseByteCount);
			ASSERT(actualAutosenseTransferLength <= 255);
			bzero(gAutosenseArea, actualAutosenseTransferLength);
			MakeCCDescriptor(
				&gChannelCommandArea[0],
				INPUT_LAST | actualAutosenseTransferLength,
				(UInt32) gAutosenseAddress
			);
			MakeCCDescriptor(&gChannelCommandArea[1], STOP_CMD, 0);
			flush_cache_v((vm_offset_t) gChannelCommandArea, sizeof (DBDMADescriptor) * 2);
			gActiveCommand->thisTransferLength = actualAutosenseTransferLength;
			ioReturn = IO_R_SUCCESS;
		}
		else {
			ioReturn = [self hardwareInitializeRequestCCL];
		}
		RESULT(ioReturn);
		return (ioReturn);
}

/**
 * Initialize the data transfer channel command list for a normal SCSI command.
 * This is not an optimal implementation, but it simplifies maintaining a
 * common code base with the NuBus Curio/AMIC hardware interface.
 * Note that the last DBDMA command must be INPUT_LAST or OUTPUT_LAST to handle
 * synchronous transfer odd-byte disconnect. 
 *
 * Significantly revised for Radar 1670762.
 */
- (IOReturn) hardwareInitializeRequestCCL
{
		CommandBuffer		*cmdBuf;
		IOSCSIRequest		*scsiReq;
	DBDMADescriptor	    *descriptorPtr;	/* -> the current data descriptor   */
	DBDMADescriptor	    *descriptorMax;	/* -> beyond the last data descr.   */
		IOReturn			ioReturn = IO_R_SUCCESS;
		UInt32				dbdmaOp;			/* Opcode for DBDMA request			*/
	unsigned int    rangeByteCount;
	unsigned int    actualRanges;
	unsigned int    i;
#define kMaxPhysicalRange   16
	PhysicalRange	    range[kMaxPhysicalRange];
		
		ENTRY("Hir hardwareInitializeRequestCCL");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		cmdBuf = gActiveCommand;
		scsiReq = cmdBuf->scsiReq;
		ASSERT(scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN);
		/*
		 * Select the correct DBDMA command for this command.
		 */
		if (scsiReq->read) {
			dbdmaOp				= INPUT_MORE;
		}
		else {
			dbdmaOp				= OUTPUT_MORE;
		}
		/*
		 * How many descriptors can we store (need some slop for the terminator commands).
		 * Get a pointer to the first free descriptor and the total number of bytes left
		 * to transfer in this I/O request.
		 */
		descriptorPtr			= gChannelCommandArea;
		descriptorMax			= &descriptorPtr[gDBDMADescriptorMax];
		/*
		 * cmdBuf->thisTransferLength will contain the actual number of bytes we
		 * intend to transfer in this DMA request, which is needed when DMA completes
		 * to recover the residual transfer length.
		 */
	cmdBuf->thisTransferLength  = 0;
	ddmDMA("Req CCL, start %8d, max size %8d\n",
				cmdBuf->currentDataIndex,
				scsiReq->maxTransfer - cmdBuf->currentDataIndex,
				3, 4, 5
			);
/* + Radar xxxxxx -- Use IOMemoryDescriptors */
	while (descriptorPtr < descriptorMax
	    && cmdBuf->thisTransferLength < kMaxDMATransfer) {
	    rangeByteCount = [cmdBuf->mem getPhysicalRanges
				    : kMaxPhysicalRange
		    maxByteCount    : (kMaxDMATransfer - cmdBuf->thisTransferLength)
		    newPosition	    : NULL
		    actualRanges    : &actualRanges
		    physicalRanges  : range
		];
#if 0
	    IOLog("rangeByteCount %d, actualRanges %d, range[0] = %08x, %d\n",
		rangeByteCount, actualRanges,
		(UInt32) range[0].address,
		range[0].length
					);
#endif
	    if (rangeByteCount == 0) {
		break;
				}
	    for (i = 0; i < actualRanges; i++) {
#if 0
	    IOLog("%d: %08x for %d\n",
		i, (UInt32) range[i].address, range[i].length);
#endif
#if 1
	    if (range[i].length == 0) {
		IOLog("%d: %08x for %d\n",
		i, (UInt32) range[i].address, range[i].length);
		[self logIOMemoryDescriptor : "Bogus length"];
		MakeCCDescriptor(descriptorPtr + 1, STOP_CMD, 0);
		[self logChannelCommandArea : "Bogus length"];
		IOPanic("Bogus length\n");
			}
#endif
		MakeCCDescriptor(descriptorPtr,
		    (dbdmaOp | range[i].length),
		    (unsigned int) range[i].address
				);
	    descriptorPtr++;
			}
	    cmdBuf->thisTransferLength += rangeByteCount;
			}
/* - Radar xxxxxx -- Use IOMemoryDescriptors */
		if (descriptorPtr > gChannelCommandArea) {
			/*
			 * We stored at least one descriptor. Change the last one so it
			 * is an INPUT_LAST or OUTPUT_LAST.
			 */
			SetCCOperation(
				&descriptorPtr[-1],
				GetCCOperation(&descriptorPtr[-1]) ^ OUTPUT_LAST
			);
		} /* Set "last operation" bit in the command */
		MakeCCDescriptor(descriptorPtr, STOP_CMD, 0);
		descriptorPtr++;
		ASSERT(descriptorPtr < &gChannelCommandArea[gDBDMADescriptorMax]);
		flush_cache_v(
			(vm_offset_t) gChannelCommandArea,
			((UInt32) descriptorPtr) - ((UInt32) gChannelCommandArea)
		);
#if 0	// ddmDMA
	IOLog("DMA setup, %d descriptors: thisTransferLength %d\n",
			descriptorPtr - gChannelCommandArea,
			cmdBuf->thisTransferLength,
			3, 4, 5
		);
#endif
		RESULT(ioReturn);
		return (ioReturn);
}


/**
 * Start a request after [self hardwareStart] has figured out what to do.
 */
- (void)    hardwareStartSCSIRequest
{
		register CommandBuffer	*cmdBuf;
		IOSCSIRequest			*scsiReq;
		UInt8					selectCmd;

		ENTRY("Hsr hardwareStartSCSIRequest");
		ASSERT(gActiveCommand != NULL && gActiveCommand->scsiReq != NULL);
		cmdBuf					= gActiveCommand;
		scsiReq					= cmdBuf->scsiReq;
		ASSERT(scsiReq->target == gCurrentTarget && scsiReq->lun == gCurrentLUN);
		/*
		 * Flush the fifo, then load synchronous registers and the target bus ID
		 */
	CURIOflushFifo();
	CURIOsetDestinationID(scsiReq->target);
		/*
		 * Put the contents of the message buffer into the fifo. The Curio makes this
		 * slightly messy. The command depends on the number of message bytes. The
		 * "select ATN and stop" case only occurs when we negotiate synchronous transfers.
		 */
		switch (gMsgOutPtr - gMsgPutPtr) {
		case 0:				/* Select, no ATN, send CDB		*/
			gLastMsgOut[0] = gLastMsgOut[1] = kScsiMsgNop;
			selectCmd = cSlctNoAtn;
			break;							
	case 1:			/* Select, ATN, 1 Msg, send CDB */
			gLastMsgOut[0] = gMsgPutPtr[0];
			gLastMsgOut[1] = kScsiMsgNop;
			selectCmd = cSlctAtn;
			break;
	case 3:			/* Select, ATN, 3 Msg, send CDB */
			gLastMsgOut[0] = gMsgPutPtr[0];
			gLastMsgOut[1] = gMsgPutPtr[1];
			selectCmd = cSlctAtn3;
			break;
		default:				/* Select, ATN, 1 Msg, stop		*/
			gLastMsgOut[0] = gMsgPutPtr[0];
			gLastMsgOut[1] = kScsiMsgNop;
			selectCmd = cSlctAtnStp;
			break;
		}
		if ((gMsgOutPtr - gMsgPutPtr) > 3
		 || (cmdBuf->flagIsAutosense == FALSE
		  && (gMsgOutPtr - gMsgPutPtr) + gActiveCommand->cdbLength >= 16)) {
	    CURIOputByteIntoFifo(*gMsgPutPtr++);
			selectCmd = cSlctAtnStp;
		}
		else {
			while (gMsgPutPtr < gMsgOutPtr) {
		CURIOputByteIntoFifo(*gMsgPutPtr++);
			}
		}
		if (selectCmd != cSlctAtnStp) {
	    CURIOputCommandIntoFifo();
	}   
		/* ** ** **
		 * ** ** ** Can a caller override the default timeout?
		 * ** ** **
		 */
//	CURIOsetSelectionTimeout(gSelectionTimeout);
		/*
		 * Initialize the finite state automaton and start the Curio.
		 */
		gBusState = SCS_SELECTING;
		gCurrentBusPhase = kBusPhaseBusFree;
	CURIOsetCommandRegister(selectCmd);
		EXIT();
}

- (void) clearChannelCommandResults
{
		ENTRY("Hcc clearChannelCommandResults");
		EXIT();
}

/**
 * Debug log channel command area
 */
- (void) logChannelCommandArea
			: (const char *) reason
{
#if DEBUG
	DBDMADescriptor	    *descriptorPtr;	/* -> the current data descriptor   */
	DBDMADescriptor	    *descriptorMax;	/* -> beyond the last data descr.   */
		UInt32				operation;
		UInt32				address;
		UInt32				cmdDep;
		UInt32				result;
		UInt32				status;
		const char			*opName;
		const char			*keyName;
	const char	    *intName	= "";
	const char	    *branchName = "";
	const char	    *waitName	= "";
		Boolean				first;
		
		ENTRY("Dlo logChannelCommandArea");
		status = DBDMAGetChannelStatus();
		if (reason != NULL) {
			IOLog("%s: *** %s\n", [self name], reason);
		}
		IOLog("%s: *** DBDMA sts %08x, cmdPtr %08x,"
			" branchSel %08x, byteCount %08x, ccl @ %08x\n",
			[self name],
			status,
			DBDMAGetCommandPtr(),
			(UInt32) DBDMAGetBranchSelect(),
			(UInt32) DBDMAGetWaitSelect(),
			(UInt32) gChannelCommandArea
		);
		if ((status & 0x00008000) != 0) {
			IOLog("%s: *** status: run\n", [self name]);
		}
		if ((status & 0x00004000) != 0) {
			IOLog("%s: *** status: pause\n", [self name]);
		}
		if ((status & 0x00002000) != 0) {
			IOLog("%s: *** status: flush\n", [self name]);
		}
		if ((status & 0x00001000) != 0) {
			IOLog("%s: *** status: wake\n", [self name]);
		}
		if ((status & 0x00000800) != 0) {
			IOLog("%s: *** status: dead\n", [self name]);
		}
		if ((status & 0x00000400) != 0) {
			IOLog("%s: *** status: active\n", [self name]);
		}
		if ((status & 0x000000100) != 0) {
			IOLog("%s: *** status: bt\n", [self name]);
		}
		if ((status & 0xFFFF02FF) != 0) {
			IOLog("%s: *** other status set\n", [self name]);
		}
		descriptorPtr			= gChannelCommandArea;
		descriptorMax			= &descriptorPtr[gDBDMADescriptorMax];
		for (; descriptorPtr < descriptorMax; descriptorPtr++) {
			operation	= GetCCOperation(descriptorPtr);
			address		= GetCCAddress(descriptorPtr);
			cmdDep		= GetCCCmdDep(descriptorPtr);
			result		= GetCCResult(descriptorPtr);
	    IOLog("%s: *** dma[%2d]: op %08x, addr %08x, cmddep %08x, res %08x\n",
				[self name],
				descriptorPtr - gChannelCommandArea,
				operation,
				address,
				cmdDep,
				result
			);
			switch (operation & kdbdmaCmdMask) {
			case OUTPUT_MORE:		opName = "Output More";		break;
			case OUTPUT_LAST:		opName = "Output Last";		break;
			case INPUT_MORE:		opName = "Input More";		break;
			case INPUT_LAST:		opName = "Input Last";		break;
			case STORE_QUAD:		opName = "Store Quad";		break;
			case LOAD_QUAD:			opName = "Load Quad";		break;
			case NOP_CMD:			opName = "NOP";				break;
			case STOP_CMD:			opName = "Stop";			break;
			default:				opName = "Op Unknown";		break;
			}
			switch (operation & kdbdmaKeyMask) {
			case KEY_STREAM0:		keyName = "Stream 0";		break;
			case KEY_STREAM1:		keyName = "Stream 1";		break;
			case KEY_STREAM2:		keyName = "Stream 2";		break;
			case KEY_STREAM3:		keyName = "Stream 3";		break;
			case KEY_REGS:			keyName = "Regs";			break;
			case KEY_SYSTEM:		keyName = "System";			break;
			case KEY_DEVICE:		keyName = "System";			break;
			default:				keyName = "Unknown";		break;
			}
			switch (operation & kdbdmaIMask) {
			case kIntNever:			intName = "never";			break;
			case kIntIfTrue:		intName = "if True";		break;
			case kIntIfFalse:		intName = "if False";		break;
	    case kIntAlways:	    intName = "always";		break;
			}
			switch (operation & kdbdmaBMask) {
			case kBranchNever:		branchName = "never";		break;
			case kBranchIfTrue:		branchName = "if True";		break;
	    case kBranchIfFalse:    branchName = "if False";	break;
			case kBranchAlways:		branchName	= "always";		break;
			}
			switch (operation & kdbdmaWMask) {
			case kWaitNever:		waitName = "never";			break;
			case kWaitIfTrue:		waitName = "if True";		break;
			case kWaitIfFalse:		waitName = "if False";		break;
			case kWaitAlways:		waitName = "always";		break;
			}
			IOLog("%s: ***    [%2d]: op %s, key %s, int %s, branch %s, wait %s, count %d,"
				" residual %d, status %04x",
				[self name],
				descriptorPtr - gChannelCommandArea,
				opName,
				keyName,
				intName,
				branchName,
				waitName,
				(operation & kdbdmaReqCountMask),
				result & kdbdmaResCountMask,
				(result & kdbdmaXferStatusMask) >> 16
			);
			first = TRUE;
			if ((result & kdbdmaXferStatusMask) == 0) {
				IOLog(" (no status)\n");
			}
			else {
				if ((result & kXferStatusRun) != 0) {
					IOLog(": Run");
					result &= ~kXferStatusRun;
					first = FALSE;
				}
				if ((result & kXferStatusPause) != 0) {
					IOLog("%s Pause", (first) ? ":" : ",");
					result &= kXferStatusPause;
					first = FALSE;
				}
				if ((result & kXferStatusFlush) != 0) {
					IOLog("%s Flush", (first) ? ":" : ",");
					result &= kXferStatusFlush;
					first = FALSE;
				}
				if ((result & kXferStatusWake) != 0) {
					IOLog("%s Wake", (first) ? ":" : ",");
					result &= kXferStatusWake;
					first = FALSE;
				}
				if ((result & kXferStatusDead) != 0) {
					IOLog("%s Dead", (first) ? ":" : ",");
					result &= kXferStatusDead;
					first = FALSE;
				}
				if ((result & kXferStatusActive) != 0) {
					IOLog("%s Active", (first) ? ":" : ",");
					result &= kXferStatusActive;
					first = FALSE;
				}
				if ((result & kXferStatusBt) != 0) {
					IOLog("%s Branch True", (first) ? ":" : ",");
					result &= kXferStatusBt;
					first = FALSE;
				}
				if (result != 0) {
					IOLog("%s Other", (first) ? ":" : ",");
				}
				IOLog("\n");
			}
			if ((operation & kdbdmaCmdMask) == STOP_CMD || operation == 0) {
				break;
			}
		}
		EXIT();
#endif	/* DEBUG */
}

- (void)	    logIOMemoryDescriptor
	    : (const char *) info
{
#if DEBUG
    const char	*intro	= " \"";
    const char	*outro	= "\"";
    IOMemoryDescriptorState    state;

    if (gActiveCommand != NULL && gActiveCommand->mem != NULL) {
	[gActiveCommand->mem state : &state];
	if (info == NULL || info[0] == '\0') {
	info = intro = outro = "";
	}
	IOLog("IOMemoryDescriptor%s%s%s: %d offset in %d, %d maxSeg\n",
	    intro, info, outro,
	    [gActiveCommand->mem currentOffset],
	    [gActiveCommand->mem totalByteCount],
	    [gActiveCommand->mem maxSegmentCount]
	);
	IOLog("IOMemoryDescriptor%s%s%s: range %d, logical %d in %d, physical %d in %d\n",
	    intro, info, outro,
	    state.rangeIndex,
	    state.logicalOffset,
	    state.ioRange.size,
	    state.physicalOffset,
	    state.physical.length
	);
    }
    else {
	IOLog("Null IOMemoryDescriptor\n");
    }
#endif	/* DEBUG */
}


@end /* Apple96_SCSI(HardwarePrivate) */

