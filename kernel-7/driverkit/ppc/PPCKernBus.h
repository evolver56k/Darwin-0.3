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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * Exported interface for Kernel PPC Bus Resource Object(s).
 *
 * HISTORY
 *
 * 28 June 1994 Curtis Galloway at NeXT
 *	Derived from EISA files.
 */

#ifdef	DRIVER_PRIVATE

#import <driverkit/KernBus.h>
#import <driverkit/KernBusMemory.h>
#import <driverkit/KernBusInterrupt.h>


@interface PPCKernBusInterrupt : KernBusInterrupt <KernBusInterrupt>
{
@private
    id		_PPCLock;
    int		_priorityLevel;
    int		_irq;
    BOOL	_irqAttached;
    BOOL	_irqEnabled;
}

@end


#define IO_PORTS_KEY 		"I/O Ports"
#define MEM_MAPS_KEY 		"Memory Maps"
#define IRQ_LEVELS_KEY		"IRQ Levels"
#define DMA_CHANNELS_KEY	"DMA Channels"

@interface PPCKernBus : KernBus
{
@private
}

- init;
- free;

@end

#endif	/* DRIVER_PRIVATE */
