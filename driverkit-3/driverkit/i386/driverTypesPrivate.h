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
/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * private i386-specific driverkit types.
 *
 * HISTORY
 *
 * 11-Mar-93	Doug Mitchell at NeXT
 *	Split off from public version.
 */
 
#import <driverkit/driverTypes.h>

typedef IORange		IOEISAPortRange;

#define IO_NUM_EISA_PORT_RANGES		20
typedef IOEISAPortRange	IOEISAPortMap[IO_NUM_EISA_PORT_RANGES];

typedef IORange		IOEISAMemoryRange;

#define IO_NUM_EISA_MEMORY_RANGES	9
typedef IOEISAMemoryRange	IOEISAMemoryMap[IO_NUM_EISA_MEMORY_RANGES];

typedef int	IOEISAInterrupt;

#define IO_NUM_EISA_INTERRUPTS		7
typedef IOEISAInterrupt	IOEISAInterruptList[IO_NUM_EISA_INTERRUPTS];

typedef int	IOEISADMAChannel;

#define IO_NUM_EISA_DMA_CHANNELS	4
typedef IOEISADMAChannel	IOEISADMAChannelList[IO_NUM_EISA_DMA_CHANNELS];
