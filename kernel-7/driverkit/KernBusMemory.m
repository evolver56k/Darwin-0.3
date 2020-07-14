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
 * Kernel Bus Generic Memory Resource Object(s).
 *
 * HISTORY
 *
 * 15 Mar 1994 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <vm/vm_kern.h>

#import <driverkit/KernBusMemory.h>

@implementation KernBusMemoryRange

- mapToAddress:		(vm_offset_t)destAddr
	inTarget:	(task_t)task
	cache:		(IOCache)cache
{
    Range		subRange = [self range];

    subRange.base = 0;

    return [[KernBusMemoryRangeMapping alloc]
				    initWithRange:self
				    subRange:subRange
				    atAddress:destAddr
				    inTarget:task
				    cache:cache];
}

- mapInTarget:		(task_t)task
	cache:		(IOCache)cache
{
    Range		subRange = [self range];

    subRange.base = 0;

    return [[KernBusMemoryRangeMapping alloc]
				    initWithRange:self
				    subRange:subRange
				    inTarget:task
				    cache:cache];
}

@end

#if ppc
#include <machdep/ppc/proc_reg.h>
#endif

@implementation KernBusMemoryRangeMapping

- initWithRange:	range
	subRange:	(Range)subRange
	atAddress:	(vm_offset_t)address
	inTarget:	(task_t)task
	cache:		(IOCache)cache
{
    Range		mappedRange;

    if (task == TASK_NULL)
    	return [self free];

    if ([super initWithRange:range subRange:subRange] == nil)
    	return nil;

    mappedRange = [self mappedRange];	// absolute physical range

#if ppc

    if( (task->map->pmap == kernel_pmap)
    &&  (IO_CacheOff	 == cache)
    &&  (address 	 == PEMapSegment( mappedRange.base, mappedRange.length)) ) {

	_address = address;
	_task = TASK_NULL;
	return self;

    } else {

	vm_offset_t	spa, epa;
    
	spa = mappedRange.base & 0xfffff000;
	epa = (mappedRange.base + mappedRange.length + 0xfff) & 0xfffff000;
	// non fatal - it may overlap another range
	pmap_add_physical_memory( spa, epa, FALSE, PTE_WIMG_IO);
    }

#endif

    if (_KernBusMemoryCreateMapping(
    				mappedRange.base,
				mappedRange.length, // will not work if == 0
				&address,
				task,
				NO,
				cache) != KERN_SUCCESS)
	return [self free];

    _task = task;
    _address = address;
    
    return self;
}

- initWithRange:	range
	subRange:	(Range)subRange
	inTarget:	(task_t)task
	cache:		(IOCache)cache
{
    Range		mappedRange;
    
    if (task == TASK_NULL)
    	return [self free];

    if ([super initWithRange:range subRange:subRange] == nil)
    	return nil;

    mappedRange = [self mappedRange];	// absolute physical range

#if ppc

    if( (task->map->pmap == kernel_pmap)
    &&  (IO_CacheOff	 == cache)
    &&  (_address 	 =  PEMapSegment( mappedRange.base, mappedRange.length)) ) {

	_task = TASK_NULL;
	return self;

    } else {

	vm_offset_t	spa, epa;
    
	spa = mappedRange.base & 0xfffff000;
	epa = (mappedRange.base + mappedRange.length + 0xfff) & 0xfffff000;
	// non fatal - it may overlap another range
	pmap_add_physical_memory( spa, epa, FALSE, PTE_WIMG_IO);
    }

#endif

    if (_KernBusMemoryCreateMapping(
    				mappedRange.base,
				mappedRange.length, // will not work if == 0
				&_address,
				task,
				YES,
				cache) != KERN_SUCCESS)
	return [self free];

    _task = task;

    return self;
}

- free
{
    Range		mappedRange = [self mappedRange];

    if (_task != TASK_NULL)
    	(void) vm_deallocate(_task->map, _address, mappedRange.length);

    return [super free];
}

- (task_t)task
{
    return _task;
}

- (vm_offset_t)address
{
    return _address;
}

@end

kern_return_t
_KernBusMemoryCreateMapping(
    vm_offset_t		physAddr,
    vm_size_t		length,
    vm_offset_t		*destAddr,
    task_t		task,
    BOOL		findSpace,
    IOCache		cache
)
{
    kern_return_t	result;
    vm_map_t		map = task->map;
    vm_offset_t		virtAddr;
    cache_spec_t	caching;
	
    vm_map_reference(map);
    
    if (findSpace)
    	*destAddr = vm_map_min(map);
    else
    	*destAddr = trunc_page(*destAddr);
    
    length = round_page(length);

    result = vm_map_find(
    			map,
			VM_OBJECT_NULL, (vm_offset_t) 0,
			destAddr, length,
			findSpace);
			
    if (result != KERN_SUCCESS) {
    	vm_map_deallocate(map);
	return result;
    }
    
    virtAddr = trunc_page(*destAddr);
    
    (void) vm_map_inherit(
			map,
			virtAddr, virtAddr + length,
			VM_INHERIT_NONE);
			
    physAddr = trunc_page(physAddr);
	
    switch (cache) {
    
    case IO_CacheOff:
    	caching = cache_disable;
	break;
	
    case IO_WriteThrough:
    	caching = cache_writethrough;
	break;
	
    default:
    	caching = cache_default;
	break;
    }
    
    while (length > 0) {
    	pmap_enter_cache_spec(
			    vm_map_pmap(map),
			    virtAddr,
			    physAddr,
			    VM_PROT_READ | VM_PROT_WRITE,
			    TRUE,
			    caching);
			    
	virtAddr += PAGE_SIZE; length -= PAGE_SIZE; physAddr += PAGE_SIZE;
    }
    
    vm_map_deallocate(map);
    
    return KERN_SUCCESS;
}
