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
/* -*-mode:C; tab-width: 4 -*- */

#ifndef MACPARTITIONS_H
#define MACPARTITIONS_H 1

#include <Types.h>

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

// pmParType field value for Secondary Loader Extension partitions
#define kSecondaryLoaderExtensionPartitionType	"Apple_MacOSPrepExtension"


/* Signatures */
enum {
	sbSIGWord	= 0x4552,				/* signature word for Block 0 ('ER') */
	sbMac		= 1,					/* system type for Mac */
	pMapSIG		= 0x504D				/* partition map signature ('PM') */
};

/* Partition Map Entry */
struct Partition {
	UInt16			pmSig;				/* unique value for map entry blk */
	UInt16			pmSigPad;			/* currently unused */
	UInt32			pmMapBlkCnt;		/* # of blks in partition map */
	UInt32			pmPyPartStart;		/* physical start blk of partition */
	UInt32			pmPartBlkCnt;		/* # of blks in this partition */
	UInt8			pmPartName[32];		/* ASCII partition name */
	char			pmParType[32];		/* ASCII partition type */
	UInt32			pmLgDataStart;		/* log. # of partition's 1st data blk */
	UInt32			pmDataCnt;			/* # of blks in partition's data area */
	UInt32			pmPartStatus;		/* bit field for partition status */
	UInt32			pmLgBootStart;		/* log. blk of partition's boot code */
	UInt32			pmBootSize;			/* number of bytes in boot code */
	UInt32			pmBootAddr;			/* memory load address of boot code */
	UInt32			pmBootAddr2;		/* currently unused */
	UInt32			pmBootEntry;		/* entry point of boot code */
	UInt32			pmBootEntry2;		/* currently unused */
	UInt32			pmBootCksum;		/* checksum of boot code */
	char			pmProcessor[16];	/* ASCII for the processor type */
	UInt16			pmPad[188];			/* ARRAY[0..187] OF INTEGER; not used */
} GCC_PACKED;
typedef struct Partition Partition;

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#endif
