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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	vm/vm_kern.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Kernel memory management.
 */

#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/thread.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>

vm_map_t	kernel_map;

static boolean_t kmem_alloc_pages(
			vm_object_t	object,
			vm_offset_t	offset,
			vm_offset_t	start,
			vm_offset_t	end,
			boolean_t	canblock);
void kmem_remap_pages(
			vm_object_t	object,
			vm_offset_t	offset,
			vm_offset_t	start,
			vm_offset_t	end);

/*
 *	Allocate wired-down memory in the kernel's address map
 *	or a submap.
 */
static
kern_return_t
kmem_alloc_prim(
	vm_map_t		map,
	vm_offset_t		*addrp,
	vm_size_t		size,
	vm_object_t		object,
	boolean_t		canblock
)
{
	kern_return_t		result;
	vm_offset_t		addr, offset = 0;
	vm_map_entry_t		entry;

	size = round_page(size);
	vm_map_lock(map);
	if (result = vm_map_find_entry(map, &addr, size, (vm_offset_t) 0,
				(object != kernel_object)?
					VM_OBJECT_NULL : object, &entry)
						!= KERN_SUCCESS) {
		vm_map_unlock(map);
		if (object != kernel_object)
			vm_object_deallocate(object);
		return (result);
	}

	if (object == kernel_object)
		offset = addr - VM_MIN_KERNEL_ADDRESS;

	if (entry->object.vm_object == VM_OBJECT_NULL) {
		if (object == kernel_object)
			vm_object_reference(kernel_object);

		entry->object.vm_object = object;
		entry->offset = offset;
	}

	/*
	 *	Since we have not given out this address yet,
	 *	it is safe to unlock the map.
	 */
	vm_map_unlock(map);

	if (!kmem_alloc_pages(object, offset, addr, addr + size, canblock)) {
		vm_map_remove(map, addr, addr + size);

		return (KERN_RESOURCE_SHORTAGE);
	}

	*addrp = addr;

	return (KERN_SUCCESS);
}

kern_return_t
kmem_alloc(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	vm_object_t	object = vm_object_allocate(size);

	return kmem_alloc_prim(map, addrp, size, object, TRUE);
}

/*
 *	kmem_realloc:
 *
 *	Reallocate wired-down memory in the kernel's address map
 *	or a submap.
 *	This can only be used on regions allocated with kmem_alloc.
 *
 *	If successful, the pages in the old region are mapped twice.
 *	The old region is unchanged.  Use kmem_free to get rid of it.
 */
kern_return_t kmem_realloc(
	vm_map_t	map,
	vm_offset_t	oldaddr,
	vm_size_t	oldsize,
	vm_offset_t	*newaddrp,
	vm_size_t	newsize)
{
	vm_offset_t oldmin, oldmax;
	vm_offset_t newaddr;
	vm_object_t object;
	vm_map_entry_t oldentry, newentry;
	kern_return_t kr;

	oldmin = trunc_page(oldaddr);
	oldmax = round_page(oldaddr + oldsize);
	oldsize = oldmax - oldmin;
	newsize = round_page(newsize);

	/*
	 *	Find space for the new region.
	 */

	vm_map_lock(map);
	kr = vm_map_find_entry(map, &newaddr, newsize, (vm_offset_t) 0,
				VM_OBJECT_NULL, &newentry);
	if (kr != KERN_SUCCESS) {
		vm_map_unlock(map);
		return kr;
	}

	/*
	 *	Find the VM object backing the old region.
	 */

	if (!vm_map_lookup_entry(map, oldmin, &oldentry))
		panic("kmem_realloc");
	object = oldentry->object.vm_object;

	/*
	 *	Increase the size of the object and
	 *	fill in the new region.
	 */

	vm_object_reference(object);
	vm_object_lock(object);
	if (object->size != oldsize)
		panic("kmem_realloc");
	object->size = newsize;
	vm_object_unlock(object);

	newentry->object.vm_object = object;
	newentry->offset = 0;

	/*
	 *	Since we have not given out this address yet,
	 *	it is safe to unlock the map.  We are trusting
	 *	that nobody will play with either region.
	 */

	vm_map_unlock(map);

	/*
	 *	Remap the pages in the old region and
	 *	allocate more pages for the new region.
	 */

	kmem_remap_pages(object, 0,
			 newaddr, newaddr + oldsize);
	(void) kmem_alloc_pages(object, oldsize,
				newaddr + oldsize, newaddr + newsize, TRUE);

	*newaddrp = newaddr;
	return KERN_SUCCESS;
}

kern_return_t
kmem_alloc_wired(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	return kmem_alloc_prim(map, addrp, size, kernel_object, TRUE);
}


/*
 *	kmem_alloc_pageable:
 *
 *	Allocate pageable memory to the kernel's address map.
 *	map must be "kernel_map" below.
 */

kern_return_t
kmem_alloc_pageable(
	vm_map_t	map,
	vm_offset_t	*addrp,
	vm_size_t	size)
{
	vm_offset_t addr;
	kern_return_t kr;

	addr = vm_map_min(map);
	kr = vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t) 0,
				&addr, round_page(size), TRUE);
	if (kr != KERN_SUCCESS)
		return kr;
		
	*addrp = addr;
	return KERN_SUCCESS;
}

/*
 *	kmem_free:
 *
 *	Release a region of kernel virtual memory allocated
 *	with kmem_alloc, and return the physical pages
 *	associated with that region.
 */
void
kmem_free(
	vm_map_t	map,
	vm_offset_t	addr,
	vm_size_t	size)
{
	(void) vm_map_remove(map, trunc_page(addr), round_page(addr + size));
}

/*
 *	Allocate new wired pages in an object.
 *	The object is assumed to be mapped into the kernel map or
 *	a submap.
 */
static
boolean_t
kmem_alloc_pages(
	vm_object_t		object,
	vm_offset_t		offset,
	vm_offset_t		start,
	vm_offset_t		end,
	boolean_t		canblock)
{
	/*
	 *	Mark the pmap region as not pageable.
	 */
	pmap_pageable(kernel_pmap, start, end, FALSE);

	while (start < end) {
	    register vm_page_t	mem;

	    vm_object_lock(object);

	    /*
	     *	Allocate a page
	     */
	    while ((mem = vm_page_alloc(object, offset)) == VM_PAGE_NULL) {
		vm_object_unlock(object);
		if (!canblock)
			return FALSE;
		VM_WAIT;
		vm_object_lock(object);
	    }

	    /*
	     *	Wire it down
	     */
	    vm_page_lock_queues();
	    vm_page_wire(mem);
	    vm_page_unlock_queues();
	    vm_object_unlock(object);

	    vm_page_zero_fill(mem);

	    /*
	     *	Enter it in the kernel pmap
	     */
	    PMAP_ENTER(kernel_pmap, start, mem,
		       VM_PROT_DEFAULT, TRUE);

	    vm_object_lock(object);
	    PAGE_WAKEUP(mem);
	    vm_object_unlock(object);

	    start += PAGE_SIZE;
	    offset += PAGE_SIZE;
	}

	return TRUE;
}

/*
 *	Remap wired pages in an object into a new region.
 *	The object is assumed to be mapped into the kernel map or
 *	a submap.
 */
void
kmem_remap_pages(
	vm_object_t	object,
	vm_offset_t	offset,
	vm_offset_t	start,
	vm_offset_t	end)
{
	/*
	 *	Mark the pmap region as not pageable.
	 */
	pmap_pageable(kernel_pmap, start, end, FALSE);

	while (start < end) {
	    register vm_page_t	mem;

	    vm_object_lock(object);

	    /*
	     *	Find a page
	     */
	    if ((mem = vm_page_lookup(object, offset)) == VM_PAGE_NULL)
		panic("kmem_remap_pages");

	    /*
	     *	Wire it down (again)
	     */
	    vm_page_lock_queues();
	    vm_page_wire(mem);
	    vm_page_unlock_queues();
	    vm_object_unlock(object);

	    /*
	     *	Enter it in the kernel pmap.  The page isn't busy,
	     *	but this shouldn't be a problem because it is wired.
	     */
	    PMAP_ENTER(kernel_pmap, start, mem,
		       VM_PROT_DEFAULT, TRUE);

	    start += PAGE_SIZE;
	    offset += PAGE_SIZE;
	}
}

/*
 *	kmem_suballoc:
 *
 *	Allocates a map to manage a subrange
 *	of the kernel virtual address space.
 *
 *	Arguments are as follows:
 *
 *	parent		Map to take range from
 *	size		Size of range to find
 *	min, max	Returned endpoints of map
 *	pageable	Can the region be paged
 */
vm_map_t
kmem_suballoc(
	vm_map_t	parent,
	vm_offset_t	*min,
	vm_offset_t	*max,
	vm_size_t	size,
	boolean_t	pageable)
{
	vm_map_t map;
	vm_offset_t addr;
	kern_return_t kr;

	size = round_page(size);

	/*
	 *	Need reference on submap object because it is internal
	 *	to the vm_system.  vm_object_enter will never be called
	 *	on it (usual source of reference for vm_map_enter).
	 */
	vm_object_reference(vm_submap_object);

	addr = (vm_offset_t) vm_map_min(parent);
	kr = vm_map_find(parent, vm_submap_object, (vm_offset_t) 0,
				&addr, size, TRUE);
	if (kr != KERN_SUCCESS)
		panic("kmem_suballoc 1");

	pmap_reference(vm_map_pmap(parent));
	map = vm_map_create(vm_map_pmap(parent), addr, addr + size, pageable);
	if (map == VM_MAP_NULL)
		panic("kmem_suballoc 2");

	kr = vm_map_submap(parent, addr, addr + size, map);
	if (kr != KERN_SUCCESS)
		panic("kmem_suballoc 3");

	*min = addr;
	*max = addr + size;
	return map;
}

/*
 *	kmem_init:
 *
 *	Initialize the kernel's virtual memory map, taking
 *	into account all memory allocated up to this time.
 */
void kmem_init(
	vm_offset_t	start,
	vm_offset_t	end)
{
	kernel_map = vm_map_create(pmap_kernel(),
				VM_MIN_KERNEL_ADDRESS, end,
				FALSE);

	/*
	 *	Reserve virtual memory allocated up to this time.
	 */

	if (start != VM_MIN_KERNEL_ADDRESS) {
		vm_offset_t addr = VM_MIN_KERNEL_ADDRESS;
		(void) vm_map_find(kernel_map, VM_OBJECT_NULL, (vm_offset_t) 0,
				   &addr, (start - VM_MIN_KERNEL_ADDRESS),
				   FALSE);
	}
}

/*
 *	Routine:	copyinmap
 *	Purpose:
 *		Like copyin, except that fromaddr is an address
 *		in the specified VM map.  This implementation
 *		is incomplete; it handles the current user map
 *		and the kernel map/submaps.
 */

int copyinmap(map, fromaddr, toaddr, length)
	vm_map_t map;
	char *fromaddr, *toaddr;
	int length;
{
	if (vm_map_pmap(map) == kernel_pmap) {
		/* assume a correct copy */
		bcopy(fromaddr, toaddr, length);
		return 0;
	}

	if (current_map() == map)
		return copyin( fromaddr, toaddr, length);

	return 1;
}

/*
 *	Routine:	copyoutmap
 *	Purpose:
 *		Like copyout, except that toaddr is an address
 *		in the specified VM map.  This implementation
 *		is incomplete; it handles the current user map
 *		and the kernel map/submaps.
 */

int copyoutmap(map, fromaddr, toaddr, length)
	vm_map_t map;
	char *fromaddr, *toaddr;
	int length;
{
	if (vm_map_pmap(map) == kernel_pmap) {
		/* assume a correct copy */
		bcopy(fromaddr, toaddr, length);
		return 0;
	}

	if (current_map() == map)
		return copyout(fromaddr, toaddr, length);

	return 1;
}

/*
 *	Allocate wired-down memory in the kernel's address map
 *	or a submap.
 */
kern_return_t
kmem_alloc_zone(map, addrp, size, canblock)
	vm_map_t		map;
	vm_offset_t		*addrp;
	register vm_size_t	size;
	boolean_t		canblock;
{
	return kmem_alloc_prim(map, addrp, size, kernel_object, canblock);
}

/*
 *	Special hack for allocation in mb_map.  Can never wait for pages
 *	(or anything else) in mb_map.
 */
vm_offset_t kmem_mb_alloc(map, size)
	register vm_map_t	map;
	vm_size_t		size;
{
	vm_object_t		object;
	register vm_map_entry_t	entry;
	vm_offset_t		addr;
	register int		npgs;
	register vm_page_t	m;
	register vm_offset_t	vaddr, offset, cur_off;

	/*
	 *	Only do this on the mb_map.
	 */
	if (map != mb_map)
		panic("You fool!");

	size = round_page(size);

	vm_map_lock(map);
	entry = vm_map_first_entry(map);
	if (entry == vm_map_to_entry(map)) {
		/*
		 *	Map is empty.  Do things normally the first time...
		 *	this will allocate the entry and the object to use.
		 */
		vm_map_unlock(map);
		addr = vm_map_min(map);
		if (vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t) 0,
			&addr, size, TRUE) != KERN_SUCCESS)
			return (0);
		(void) vm_map_pageable(map, addr, addr + size, FALSE);

		return(addr);
	}
	/*
	 *	Map already has an entry.  We must be extending it.
	 */
	if (!(entry == vm_map_last_entry(map) &&
	      entry->is_a_map == FALSE &&
	      entry->vme_start == vm_map_min(map) &&
	      entry->max_protection == VM_PROT_ALL &&
	      entry->protection == VM_PROT_DEFAULT &&
	      entry->inheritance == VM_INHERIT_DEFAULT &&
	      entry->wired_count != 0)) {
		/*
		 *	Someone's not playing by the rules...
		 */
		panic("mb_map abused even more than usual");
	}

	/*
	 *	Make sure there's enough room in map to extend entry.
	 */

	if (vm_map_max(map) - size < entry->vme_end) {
		vm_map_unlock(map);
		return(0);
	}

	/*
	 *	extend the entry
	 */
	object = entry->object.vm_object;
	offset = (entry->vme_end - entry->vme_start) + entry->offset;
	addr   = entry->vme_end;
	entry->vme_end += size;

	/*
	 *	Since we may not have enough memory, and we may not
	 *	block, we first allocate all the memory up front, pulling
	 *	it off the active queue to prevent pageout.  We then can
	 *	either enter the pages, or free whatever we tried to get.
	 */

	vm_object_lock(object);
	cur_off = offset;
	npgs = atop(size);
	while (npgs) {
		m = vm_page_alloc_sequential(object, cur_off, FALSE);
		if (m == VM_PAGE_NULL) {
			/*
			 *	Not enough pages, and we can't
			 *	wait, so free everything up.
			 */
			while (cur_off > offset) {
				cur_off -= PAGE_SIZE;
				m = vm_page_lookup(object, cur_off);
				/*
				 *	Don't have to lock the queues here
				 *	because we know that the pages are
				 *	not on any queues.
				 */
				vm_page_free(m);
			}
			vm_object_unlock(object);

			/*
			 *	Shrink the map entry back to its old size.
			 */
			entry->vme_end -= size;
			vm_map_unlock(map);
			return(0);
		}

		/*
		 *	We want zero-filled memory
		 */

		vm_page_zero_fill(m);

		/*
		 *	Since no other process can see these pages, we don't
		 *	have to bother with the busy bit.
		 */

		m->busy = FALSE;

		npgs--;
		cur_off += PAGE_SIZE;
	}

	vm_object_unlock(object);

	/*
	 *	Map entry is already marked non-pageable.
	 *	Loop thru pages, entering them in the pmap.
	 *	(We can't add them to the wired count without
	 *	wrapping the vm_page_queue_lock in splimp...)
	 */
	vaddr = addr;
	cur_off = offset;
	while (vaddr < entry->vme_end) {
		vm_object_lock(object);
		m = vm_page_lookup(object, cur_off);
		vm_page_wire(m);
		vm_object_unlock(object);
		pmap_enter(map->pmap, vaddr, VM_PAGE_TO_PHYS(m),
			entry->protection, TRUE);
		vaddr += PAGE_SIZE;
		cur_off += PAGE_SIZE;
	}
	vm_map_unlock(map);

	return(addr);
}

/*
 *	kmem_alloc_wait
 *
 *	Allocates pageable memory from a sub-map of the kernel.  If the submap
 *	has no room, the caller sleeps waiting for more memory in the submap.
 *
 */
vm_offset_t kmem_alloc_wait(map, size)
	vm_map_t	map;
	vm_size_t	size;
{
	vm_offset_t	addr;
	kern_return_t	result;

	size = round_page(size);

	do {
		/*
		 *	To make this work for more than one map,
		 *	use the map's lock to lock out sleepers/wakers.
		 *	Unfortunately, vm_map_find also grabs the map lock.
		 */
		vm_map_lock(map);
		lock_set_recursive(&map->lock);

		addr = vm_map_min(map);
		result = vm_map_find(map, VM_OBJECT_NULL, (vm_offset_t) 0,
				&addr, size, TRUE);

		lock_clear_recursive(&map->lock);
		if (result != KERN_SUCCESS) {

			if ( (vm_map_max(map) - vm_map_min(map)) < size ) {
				vm_map_unlock(map);
				return(0);
			}

			assert_wait(map, TRUE);
			vm_map_unlock(map);
			thread_block();
		}
		else {
			vm_map_unlock(map);
		}

	} while (result != KERN_SUCCESS);

	return(addr);
}

/*
 *	kmem_free_wakeup
 *
 *	Returns memory to a submap of the kernel, and wakes up any threads
 *	waiting for memory in that map.
 */
void	kmem_free_wakeup(map, addr, size)
	vm_map_t	map;
	vm_offset_t	addr;
	vm_size_t	size;
{
	vm_map_lock(map);
	(void) vm_map_delete(map, trunc_page(addr), round_page(addr + size));
	thread_wakeup(map);
	vm_map_unlock(map);
}
