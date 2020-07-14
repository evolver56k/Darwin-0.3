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
 * Copyright ) 1997 Apple Computer Inc. All Rights Reserved.
 * @author	Martin Minow	mailto:minow@apple.com
 * @revision	1997.02.13	Initial conversion from AMDPCSCSIDriver sources.
 *
 * Set tabs every 4 characters.
 *
 * DDM macros for Apple 53C96 SCSI driver.
 * Apple96SCSI is closely based on Doug Mitchell's AMD 53C974/79C974 driver.
 *
 * Edit History
 * 1997.02.13	MM		Initial conversion from AMDPCSCSIDriver sources. 
 * 1997.03.24	MM		Normalize Apple96DDM.h and AppleMeshDDM.h.
 */
 
#import <driverkit/debugging.h>
// #import <kernserv/ns_timer.h>
#import "Timestamp.h"

/*
 * The index into IODDMMasks[].
 */
#define APPLE96_DDM_INDEX	2

#define DDM_EXPORTED	0x00000001			// exported methods
#define DDM_IOTHREAD	0x00000002			// I/O thread methods
#define DDM_INIT		0x00000004			// Initialization
#define DDM_INTR 		0x00000008			// Interrupt
#define DDM_CHIP		0x00000010			// chip-level
#define DDM_ERROR		0x00000020			// error
#define DDM_DMA			0x00000040			// DMA
#define DDM_ENTRY		0x00000010			// method entry and exit
#
#define DDM_CONSOLE_LOG		0				/* really hosed...*/

#if APPLE96_ALWAYS_ASSERT
#undef ASSERT
#define ASSERT(what) do { if (!(what)) IOLog("Assert \"%s\" at %s, %d\n", #what, __FILE__, __LINE__);  } while (0)
#endif
#if	DDM_CONSOLE_LOG
#undef	IODEBUG
#define IODEBUG(index, mask, x, a, b, c, d, e) { 	\
	if (IODDMMasks[index] & mask) {					\
		IOLog(x, a, b, c,d, e); 					\
	}												\
}
#endif	/* DDM_CONSOLE_LOG */
#if VERY_SERIOUS_DEBUGGING /* TEMP for initial debug */
#undef	IODEBUG
#define IODEBUG(index, mask, x, a, b, c, d, e) printf(x, a, b, c, d, e)
#endif /* TEMP */

/*
 * Normal ddm calls..
 */
#define ddmExported(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_EXPORTED, x, a, b, c, d, e)
	
#define ddmThread(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_IOTHREAD, x, a, b, c, d, e)

#define ddmInit(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_INIT, x, a, b, c, d, e)

#define ddmEntry(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_ENTRY, x, a, b, c, d, e)

/*
 * catch both I/O thread events and interrupt events.
 */
#define ddmInterrupt(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, (DDM_IOTHREAD | DDM_INTR), x, a, b, c, d, e)

#define ddmChip(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_CHIP, x, a, b, c, d, e)

#define ddmError(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_ERROR, x, a, b, c, d, e)

#define ddmDMA(x, a, b, c, d, e) 					\
	IODEBUG(APPLE96_DDM_INDEX, DDM_DMA, x, a, b, c, d, e)

/*
 * Method entry/exit traces. Note that these have unbalanced
 * parentheses in order to enforce a single exit point and
 * provide for method timing. By convention, the first three
 * bytes of the "what" string will be used for timestamping,
 * so make them unique (at the expense of readability).
 */
#if 0 && DDM_DEBUG // TEMP for initial debugging
#define ENTRY(what) do {			\
	const char *__tag__ = (what);		\
	StoreTimestamp(OSTag('+', __tag__), 0);	\
	ddmEntry("+%s\n", __tag__, 2, 3, 4, 5);	\
	do {
#define EXIT() } while(0);			\
	StoreTimestamp(OSTag('-', __tag__), 0);	\
	ddmEntry("-%s\n", __tag__, 2, 3, 4, 5);	\
    } while (0)
#define TAG(name, value) RAW_TAG(OSTag('*', name), value)
#define RAW_TAG(tag, value) do {		\
	StoreTimestamp((tag), (UInt32) (value)); \
    } while (0)
#define RESULT(result) } while(0);		\
	StoreTimestamp(OSTag('-', __tag__), (UInt32) (result)); \
	ddmEntry("=%s %08x, %8u %8d\n", __tag__, \
		(UInt32) (result),		\
		(UInt32) (result),		\
		(int)	 (result),		\
		5);				\
	} while (0)
#else /* No DDM_DEBUG */
#define ENTRY(what) do {			\
	const char *__tag__ = (what);		\
	StoreTimestamp(OSTag('+', __tag__), 0);	\
	do {
#define EXIT() } while(0);			\
	StoreTimestamp(OSTag('-', __tag__), 0);	\
	} while (0)
#define TAG(name, value) RAW_TAG(OSTag('*', name), value)
#define RAW_TAG(tag, value) do {		\
	StoreTimestamp((tag), (UInt32) (value)); \
	} while (0)
#define RESULT(result) } while(0);		\
	StoreTimestamp(OSTag('-', __tag__), (UInt32) (result)); \
	} while (0)
#endif /* DDM_DEBUG */

