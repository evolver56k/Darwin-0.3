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
 * Copyright (c) 1997 NeXT Computer, Inc.
 *
 * private ppc-specific driverkit types.
 *
 * HISTORY
 *
 * Dieter Siegmund (dieter@next.com) Thu May 29 17:54:49 PDT 1997
 * - increased the size of the interrupts table and removed DMA stuff
 */
 
#import <driverkit/driverTypes.h>

// This defines the PPC specific Memory range structure & max # per device
typedef IORange		IOPPCMemoryRange;
#define PPC_MEMORY_RANGE_BOTH		0
#define IO_NUM_PPC_MEMORY_RANGES	1
typedef IOPPCMemoryRange	IOPPCMemoryMap[IO_NUM_PPC_MEMORY_RANGES];

// This defines the interupt type and the max number per device
typedef int	IOPPCInterrupt;
#define IO_NUM_PPC_INTERRUPTS		256
typedef IOPPCInterrupt	IOPPCInterruptList[IO_NUM_PPC_INTERRUPTS];
