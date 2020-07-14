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
/* -*- mode:C; tab-width: 4 -*- */
/*
 	File:		BootBlocksPriv.h
 
 	Version:	
 
 	DRI:		Alan Mimms
 
 	Copyright:	© 1984-1996 by Apple Computer, Inc.
 				All rights reserved.
 
 	Warning:	*** APPLE INTERNAL USE ONLY ***
 				This file contains unreleased SPI's
 
 	BuildInfo:	Built by:			Alan Mimms
 				With Interfacer:	2.0d13   (PowerPC native)
 				From:				BootBlocksPriv.i
 					Revision:		3
 					Dated:			1/11/96
 					Last change by:	ABM
 					Last comment:	And it gets even PICKIER in Interfacer 2.0.
 
 	Bugs:		Report bugs to Radar component “System Interfaces”, “Latest”
 				List the version information (from above) in the Problem Description.
 
*/
#ifndef __BOOTBLOCKSPRIV__
#define __BOOTBLOCKSPRIV__

#ifndef __TYPES__
#include <Types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

/*
 This is doubtless defined somewhere else, but without the addition of the Copland Loader thingie
 on the end.
*/
struct BootBlocks {
	unsigned short signature;			/* Always contains 'LK'*/
	unsigned long branch;				/* Branch instruction to boot blocks code*/
	unsigned char machineFlags;			/* Kind(s) of machines on which these boot blocks can run*/
	unsigned char version;				/* Version number of these boot blocks*/
	unsigned short page2Flags;			/* Flags for old Classic Mac second sound/screen page memory*/
	Str15 systemFileName;				/* Name of System file to use*/
	Str15 finderFileName;				/* Name of Finder to relaunch*/
	Str15 debuggerFileName;				/* Name of debugger to load*/
	Str15 disassemblerFileName;			/* Name of disassembler to load*/
	Str15 startupScreenFileName;		/* Name of startup screen to display*/
	Str15 helloFileName;				/* Name of Finder to launch	*/
	Str15 clipboardFileName;			/* Name of file to store scrap*/
	unsigned short numFCBs;				/* Number of FCBs to allocate at boot*/
	unsigned short numEvents;			/* Size of event queue*/
	unsigned long systemHeapSize128K;	/* Size of system heap on 128K machines*/
	unsigned long systemHeapSize256K;	/* Size of system heap on 256K machines*/
	unsigned long systemHeapSizePre7_0;	/* Size of system heap on System < 7.0*/
	Str15 coplandLoaderFileName;		/* Name of file containing Copland Loader pieces*/
	char filler[358];					/* Fill out to size of a disk block*/
} GCC_PACKED;
typedef struct BootBlocks BootBlocks;


#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BOOTBLOCKSPRIV__ */

