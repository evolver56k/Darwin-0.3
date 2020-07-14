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
 * Exported interface for Kernel Bus Generic Memory Resource Object(s).
 *
 * HISTORY
 *
 * 15 Mar 1994 ? at NeXT
 *	Created.
 */

#ifdef	DRIVER_PRIVATE

#import <driverkit/KernBus.h>
#import <driverkit/driverTypes.h>

#import <mach/mach_types.h>

@interface KernBusMemoryRange : KernBusRange
{
@private
}

- mapToAddress: (vm_offset_t)destAddr
	inTarget: (task_t)task
	cache: (IOCache)cache;
- mapInTarget: (task_t)task
	cache: (IOCache)cache;

@end

@interface KernBusMemoryRangeMapping : KernBusRangeMapping
{
@private
    task_t		_task;
    vm_offset_t		_address;
}

- initWithRange: range
	subRange: (Range)subRange
	atAddress: (vm_offset_t)destAddr
	inTarget: (task_t)task
	cache: (IOCache)cache;
- initWithRange: range
	subRange: (Range)subRange
	inTarget: (task_t)task
	cache: (IOCache)cache;
	
- (task_t)task;
- (vm_offset_t)address;

@end

/*
 * This primative is used to
 * create wired down mappings
 * in a task pmap.  It bypasses
 * the MACH VM system.
 * It should be totally private,
 * but may be needed by some
 * poorly behaved clients.
 */

kern_return_t	_KernBusMemoryCreateMapping(
		    vm_offset_t		physAddr,
		    vm_size_t		length,
		    vm_offset_t		*destAddr,
		    task_t		task,
		    BOOL		findSpace,
		    IOCache		cache);

#endif	/* DRIVER_PRIVATE */
