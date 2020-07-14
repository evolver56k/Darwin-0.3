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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	vm/vm_user.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	User-exported virtual memory functions.
 */

#import <mach_xp.h>

#import <sys/types.h>

#import <mach/vm_param.h>
#import <vm/vm_object.h>
#import <vm/vm_map.h>
#import <vm/vm_page.h>
#import <mach/vm_statistics.h>
#import <mach/vm_attributes.h>

#import <mach/boolean.h>
#import <mach/kern_return.h>
#import <kern/task.h>

#import <mach/mach_types.h>	/* to get vm_address_t */

#import <vm/vm_kern.h>

vm_statistics_data_t	vm_stat;

#if	MACH_XP
#else	MACH_XP
#import <kern/lock.h>
lock_data_t		vm_alloc_lock;

/*
 *	vm_user_init initializes the vm_alloc lock.  XXX
 */
void
vm_user_init()
{
	lock_init(&vm_alloc_lock, TRUE);
}
#endif	MACH_XP

/*
 *	vm_allocate allocates virtual memory in the specified address
 *	map.  Newly allocated memory is not yet validated, but will be
 *	when page faults occur.
 */
kern_return_t vm_allocate_with_pager(map, addr, size, find_space, pager,
		pager_offset)
	register vm_map_t	map;
	register vm_offset_t	*addr;
	register vm_size_t	size;
	boolean_t		find_space;
	vm_pager_t		pager;
	vm_offset_t		pager_offset;
{
	register vm_object_t	object;
	register kern_return_t	result;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	*addr = trunc_page(*addr);
	size = round_page(size);

#if	MACH_XP
	if ((object = (vm_object_t) vm_object_enter(pager, size, FALSE))
			== VM_OBJECT_NULL)
		return(KERN_INVALID_ARGUMENT);
#else	MACH_XP
	/*
	 *	Lookup the pager/paging-space in the object cache.
	 *	If it's not there, then create a new object and cache
	 *	it.
	 */
	lock_write(&vm_alloc_lock);
	object = vm_object_lookup(pager);
	vm_stat.lookups++;
	if (object == VM_OBJECT_NULL) {
		/*
		 *	Need a new object.
		 */
		object = vm_object_allocate(size);
		/*
		 *	Associate the object with the pager and
		 *	put it in the cache.
		 */
		if (pager != vm_pager_null) {
			vm_object_setpager(object, pager,
				(vm_offset_t) 0, TRUE);
			vm_object_enter(object, pager);
		}
	}
	else {
		vm_stat.hits++;
	}
	lock_write_done(&vm_alloc_lock);
	object->internal = FALSE;
#endif	MACH_XP

	if ((result = vm_map_find(map, object, pager_offset, addr, size,
				find_space)) != KERN_SUCCESS)
		vm_object_deallocate(object);
	return(result);
}

/*
 *	vm_allocate allocates "zero fill" memory in the specfied
 *	map.
 */
kern_return_t vm_allocate(map, addr, size, anywhere)
	register vm_map_t	map;
	register vm_offset_t	*addr;
	register vm_size_t	size;
	boolean_t		anywhere;
{
	kern_return_t	result;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);
	if (size == 0) {
		*addr = 0;
		return(KERN_SUCCESS);
	}

	if (anywhere)
		*addr = vm_map_min(map);
	else
		*addr = trunc_page(*addr);
	size = round_page(size);

	result = vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t) 0, addr,
			size, anywhere);

	return(result);
}

/*
 *	vm_deallocate deallocates the specified range of addresses in the
 *	specified address map.
 */
kern_return_t vm_deallocate(map, start, size)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (size == (vm_offset_t) 0)
		return(KERN_SUCCESS);

	return(vm_map_remove(map, trunc_page(start), round_page(start+size)));
}

kern_return_t vm_reallocate(map, start, size)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (size == (vm_offset_t) 0)
		return(KERN_SUCCESS);

	return(vm_map_reallocate(
			map, trunc_page(start), round_page(start+size)));
}

/*
 *	vm_inherit sets the inheritence of the specified range in the
 *	specified map.
 */
kern_return_t vm_inherit(map, start, size, new_inheritance)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
	vm_inherit_t		new_inheritance;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_inherit(map, trunc_page(start), round_page(start+size), new_inheritance));
}

/*
 *	vm_protect sets the protection of the specified range in the
 *	specified map.
 */

kern_return_t vm_protect(map, start, size, set_maximum, new_protection)
	register vm_map_t	map;
	vm_offset_t		start;
	vm_size_t		size;
	boolean_t		set_maximum;
	vm_prot_t		new_protection;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_protect(map, trunc_page(start), round_page(start+size), new_protection, set_maximum));
}

kern_return_t vm_statistics(map, stat)
	vm_map_t	map;
	vm_statistics_data_t	*stat;
{
	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	vm_stat.pagesize = PAGE_SIZE;
	vm_stat.free_count = vm_page_free_count;
	vm_stat.active_count = vm_page_active_count;
	vm_stat.inactive_count = vm_page_inactive_count;
	vm_stat.wire_count = vm_page_wire_count;
	
	*stat = vm_stat;

	return(KERN_SUCCESS);
}

/*
 * Handle machine-specific attributes for a mapping, such
 * as cachability, migrability, etc.
 */
kern_return_t vm_machine_attribute(map, address, size, attribute, value)
	vm_map_t	map;
	vm_address_t	address;
	vm_size_t	size;
	vm_machine_attribute_t	attribute;
	vm_machine_attribute_val_t* value;		/* IN/OUT */
{
	extern kern_return_t	vm_map_machine_attribute();

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	return vm_map_machine_attribute(map, address, size, attribute, value);
}

kern_return_t vm_read(map, address, size, data, data_size)
	vm_map_t	map;
	vm_address_t	address;
	vm_size_t	size;
	pointer_t	*data;
	vm_size_t	*data_size;
{
	kern_return_t	error;
	vm_offset_t	ipc_address;

	if ((round_page(address) != address) || (round_page(size) != size))
		return(KERN_INVALID_ARGUMENT);

	if ((error = vm_allocate(ipc_soft_map, &ipc_address, size, TRUE)) != KERN_SUCCESS) {
		printf("vm_read: kernel error %d\n", error);
		return(KERN_RESOURCE_SHORTAGE);
	}

	if ((error = vm_map_copy(ipc_soft_map, map, ipc_address, size, address, FALSE, FALSE)) == KERN_SUCCESS) {
		*data = ipc_address;
		*data_size = size;
	}
	else
		(void) vm_deallocate(ipc_soft_map, ipc_address, size);
	return(error);
}

kern_return_t vm_write(map, address, data, size)
	vm_map_t	map;
	vm_address_t	address;
	pointer_t	data;
	vm_size_t	size;
{
	if ((round_page(address) != address) || (round_page(size) != size))
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_copy(map, ipc_soft_map, address, size, data, FALSE, TRUE));
}

kern_return_t vm_copy(map, source_address, size, dest_address)
	vm_map_t	map;
	vm_address_t	source_address;
	vm_size_t	size;
	vm_address_t	dest_address;
{
	if ( (round_page(source_address) != source_address) || (round_page(dest_address) != dest_address)
	     || (round_page(size) != size) )
		return(KERN_INVALID_ARGUMENT);

	return(vm_map_copy(map, map, dest_address, size, source_address, FALSE, FALSE));
}



/*
 *	Specify that the range of the virtual address space
 *	of the target task must not cause page faults for
 *	the indicated accesses.
 *
 *	[ To unwire the pages, specify VM_PROT_NONE. ]
 */
kern_return_t
vm_wire(
	host_t			host,
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_prot_t		access)
{
	kern_return_t		rc;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	if (map == VM_MAP_NULL)
		return KERN_INVALID_TASK;

	if (access & ~VM_PROT_ALL)
		return KERN_INVALID_ARGUMENT;

	if (access != VM_PROT_NONE) {
		rc = vm_map_pageable(map, trunc_page(start),
				 round_page(start+size), FALSE);
	} else {
		rc = vm_map_pageable(map, trunc_page(start),
				   round_page(start+size), TRUE);
	}
	return rc;
}
