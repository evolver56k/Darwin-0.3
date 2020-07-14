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

/*
 * The class must provide the following instance or local variable:
 * DBDMAInfoRecord		gDBDMAInfoRecord
 */
 /*
 This structure defines the DB-DMA channel command descriptor.
 *** WARNING:	Endian-ness issues must be considered when performing load/store! ***
 ***			DB-DMA specifies memory organization as quadlets so it is not correct
 ***			to think of either the operation or result field as two 16-bit fields.
 ***			This would have undesirable effects on the byte ordering within their
 ***			respective quadlets. Use the accessor macros provided below.
*/
struct DBDMADescriptor {
	unsigned long 		operation;		/* cmd || key || i || b || w || reqCount*/
	unsigned long 		address;
	unsigned long 		cmdDep;
	unsigned long 		result;			/* xferStatus || resCount*/
};
typedef struct DBDMADescriptor DBDMADescriptor;

typedef DBDMADescriptor *DBDMADescriptorPtr;


/* This structure defines the standard set of DB-DMA channel registers.*/
/** ** ** ** Shouldn't these be marked volatile ** ** ** **/
struct DBDMAChannelRegisters {
	unsigned long 		channelControl;
	unsigned long 		channelStatus;
	unsigned long 		commandPtrHi;		/* implementation optional*/
	unsigned long 		commandPtrLo;
	unsigned long 		interruptSelect;	/* implementation optional*/
	unsigned long 		branchSelect;		/* implementation optional*/
	unsigned long 		waitSelect;			/* implementation optional*/
	unsigned long 		transferModes;		/* implementation optional*/
	unsigned long 		data2PtrHi;			/* implementation optional*/
	unsigned long 		data2PtrLo;			/* implementation optional*/

	unsigned long 		reserved1;
	unsigned long 		addressHi;			/* implementation optional*/
	unsigned long 		reserved2[4];
	unsigned long 		unimplemented[16];

				/* This structure must remain fully padded to 256 bytes.*/
	unsigned long 		undefined[32];
};
typedef struct DBDMAChannelRegisters DBDMAChannelRegisters;

/* These constants define the DB-DMA channel control words and status flags.*/

enum {
	kdbdmaSetRun				= 0x80008000,
	kdbdmaClrRun				= 0x80000000,
	kdbdmaSetPause				= 0x40004000,
	kdbdmaClrPause				= 0x40000000,
	kdbdmaSetFlush				= 0x20002000,
	kdbdmaSetWake				= 0x10001000,
	kdbdmaClrDead				= 0x08000000,
	kdbdmaSetS7					= 0x00800080,
	kdbdmaClrS7					= 0x00800000,
	kdbdmaSetS6					= 0x00400040,
	kdbdmaClrS6					= 0x00400000,
	kdbdmaSetS5					= 0x00200020,
	kdbdmaClrS5					= 0x00200000,
	kdbdmaSetS4					= 0x00100010,
	kdbdmaClrS4					= 0x00100000,
	kdbdmaSetS3					= 0x00080008,
	kdbdmaClrS3					= 0x00080000,
	kdbdmaSetS2					= 0x00040004,
	kdbdmaClrS2					= 0x00040000,
	kdbdmaSetS1					= 0x00020002,
	kdbdmaClrS1					= 0x00020000,
	kdbdmaSetS0					= 0x00010001,
	kdbdmaClrS0					= 0x00010000,
	kdbdmaClrAll				= 0xFFFF0000
};


enum {
	kdbdmaRun					= 0x00008000,
	kdbdmaPause					= 0x00004000,
	kdbdmaFlush					= 0x00002000,
	kdbdmaWake					= 0x00001000,
	kdbdmaDead					= 0x00000800,
	kdbdmaActive				= 0x00000400,
	kdbdmaBt					= 0x00000100,
	kdbdmaS7					= 0x00000080,
	kdbdmaS6					= 0x00000040,
	kdbdmaS5					= 0x00000020,
	kdbdmaS4					= 0x00000010,
	kdbdmaS3					= 0x00000008,
	kdbdmaS2					= 0x00000004,
	kdbdmaS1					= 0x00000002,
	kdbdmaS0					= 0x00000001
};

/* These constants define the DB-DMA channel command operations and modifiers.*/

enum {
												/* Command.cmd operations*/
	OUTPUT_MORE					= 0x00000000,
	OUTPUT_LAST					= 0x10000000,
	INPUT_MORE					= 0x20000000,
	INPUT_LAST					= 0x30000000,
	STORE_QUAD					= 0x40000000,
	LOAD_QUAD					= 0x50000000,
	NOP_CMD						= 0x60000000,
	STOP_CMD					= 0x70000000,
	kdbdmaCmdMask				= 0xF0000000
};


enum {
	/* Command.key modifiers (choose one for INPUT, OUTPUT, LOAD, and STORE)*/
	KEY_STREAM0					= 0x00000000,		/* default modifier*/
	KEY_STREAM1					= 0x01000000,
	KEY_STREAM2					= 0x02000000,
	KEY_STREAM3					= 0x03000000,
	KEY_REGS					= 0x05000000,
	KEY_SYSTEM					= 0x06000000,
	KEY_DEVICE					= 0x07000000,
	kdbdmaKeyMask				= 0x07000000,	/* Command.i modifiers (choose one for INPUT, OUTPUT, LOAD, STORE, and NOP)*/
	kIntNever					= 0x00000000,	/* default modifier*/
	kIntIfTrue					= 0x00100000,
	kIntIfFalse					= 0x00200000,
	kIntAlways					= 0x00300000,
	kdbdmaIMask					= 0x00300000,	/* Command.b modifiers (choose one for INPUT, OUTPUT, and NOP)*/
	kBranchNever				= 0x00000000,	/* default modifier*/
	kBranchIfTrue				= 0x00040000,
	kBranchIfFalse				= 0x00080000,
	kBranchAlways				= 0x000C0000,
	kdbdmaBMask					= 0x000C0000,	/* Command.w modifiers (choose one for INPUT, OUTPUT, LOAD, STORE, and NOP)*/
	kWaitNever					= 0x00000000,	/* default modifier*/
	kWaitIfTrue					= 0x00010000,
	kWaitIfFalse				= 0x00020000,
	kWaitAlways					= 0x00030000,
	kdbdmaWMask					= 0x00030000,	/* operation masks*/
	kdbdmaCommandMask			= 0xFFFF0000,
	kdbdmaReqCountMask			= 0x0000FFFF
};

/* These constants define the DB-DMA channel command results.*/

enum {
	kXferStatusRun				= kdbdmaRun << 16,
	kXferStatusPause			= kdbdmaPause << 16,
	kXferStatusFlush			= kdbdmaFlush << 16,
	kXferStatusWake				= kdbdmaWake << 16,
	kXferStatusDead				= kdbdmaDead << 16,
	kXferStatusActive			= kdbdmaActive << 16,
	kXferStatusBt				= kdbdmaBt << 16,
	kXferStatusS7				= kdbdmaS7 << 16,
	kXferStatusS6				= kdbdmaS6 << 16,
	kXferStatusS5				= kdbdmaS5 << 16,
	kXferStatusS4				= kdbdmaS4 << 16,
	kXferStatusS3				= kdbdmaS3 << 16,
	kXferStatusS2				= kdbdmaS2 << 16,
	kXferStatusS1				= kdbdmaS1 << 16,
	kXferStatusS0				= kdbdmaS0 << 16,				/* result masks*/
	kdbdmaResCountMask			= 0x0000FFFF,
	kdbdmaXferStatusMask		= kdbdmaResCountMask << 16
};


