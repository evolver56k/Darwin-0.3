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
 * Copyright (c) 1992 NeXT, Inc.
 *
 *  History:
 *
 *	14-Dec-92: Brian Pinkerton at NeXT
 *		Created
 */
#import <sys/types.h>
#import <sys/param.h>

#import <mach/mach_types.h>
#import <mach/vm_param.h>
#import <mach/boolean.h>
#import <mach/kern_return.h>
#import <mach/vm_policy.h>

#import <vm/vm_object.h>
#import <vm/vm_map.h>
#import <vm/vm_page.h>
#import <vm/vnode_pager.h>

#import <kern/task.h>
#import <kern/kalloc.h>


/*
 *	Returns the given page to the HEAD of the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
static void
vm_page_free_head(mem)
	register vm_page_t	mem;
{
	vm_page_remove(mem);
	if (mem->free)
		return;

	if (mem->active) {
		queue_remove(&vm_page_queue_active, mem, vm_page_t, pageq);
		mem->active = FALSE;
		vm_page_active_count--;
	}

	if (mem->inactive) {
		queue_remove(&vm_page_queue_inactive, mem, vm_page_t, pageq);
		mem->inactive = FALSE;
		vm_page_inactive_count--;
	}

	if (!mem->fictitious) {
		int	spl;

		spl = splimp();
		simple_lock(&vm_page_queue_free_lock);
		queue_enter_first(&vm_page_queue_free, mem, vm_page_t, pageq);
		mem->free = TRUE;
		vm_page_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
}


/*
 *  Do something with a particular page.  This function is the locus for all
 *  policy decisions.
 */
void
vm_policy_apply(vm_object_t object, vm_page_t m, int when)
{
	boolean_t	shared = FALSE;
	
	/*
	 *  Determine whether an object is "shared".  It is shared if it has
	 *  more than two references, and it's backed by something that's not
	 *  the swapfile.
	 *
	 *  Note: this is a pretty bogus definition of shared.  The ref count
	 *  on the object could easily be high even if only one task has this
	 *  object mapped.  This is OK, though, because we're making a
	 *  conservative decision.  Someone can always turn on the shared
	 *  flag if they want to be really aggressive.
	 */
	if (object->ref_count > 2) {
		if (object->pager && !object->pager->is_device) {
			vnode_pager_t pager = (vnode_pager_t) object->pager;
			shared = !pager->vs_swapfile;
		}
	}

	if (!(when & VM_DEACTIVATE_SHARED) && shared)
		return;

	vm_page_lock_queues();

	switch (when) {
		/*
		 *  Make this page as cold as possible.  That means that we
		 *  try to free the page.
		 */
		case VM_DEACTIVATE_NOW:
			if (m->clean && !pmap_is_modified(VM_PAGE_TO_PHYS(m)))
				vm_page_free_head(m);
			else {
				if (m->active)
					vm_page_deactivate(m);
			}
			pmap_remove_all(VM_PAGE_TO_PHYS(m));
			break;
		
		/*
		 *  a little warmer.
		 */
		case VM_DEACTIVATE_SOON:
			if (m->active)
				vm_page_deactivate(m);
			break;
		default:
			break;
	}

	vm_page_unlock_queues();
}


/*
 *  Deactivate active pages in this object and all shadow objects over the
 *  specified address range.
 *
 *  If we encounter an error, we'll attempt to push the rest of the pages,
 *  and return an error for the whole object.
 *
 *  If we encounter a locked page, then we'll skip it...
 */
static void
deactivate_object(vm_object_t object, vm_offset_t start, vm_offset_t end, int temperature)
{
	vm_object_t	shadow;
	vm_page_t	page;
	vm_size_t	size;
	int		flags;

	if (object == VM_OBJECT_NULL)
		return;

	vm_object_lock(object);

	for (page = (vm_page_t) queue_first(&object->memq);
	     !queue_end(&object->memq, (queue_entry_t) page);
	     page = (vm_page_t) queue_next(&page->listq))
	{
		/*
		 * If this page isn't actually referenced from this
		 * entry, then forget it.
		 */
		if (page->offset < start || page->offset >= end)
			continue;
		
		vm_policy_apply(object, page, temperature);
	}

	/*
	 * Recurse over shadow objects.
	 */
	size = end - start;
	if ((object->size > 0) && (end - start > object->size))
		size = object->size;
	start += object->shadow_offset;
	end = start + size;

	deactivate_object(object->shadow, start, end, temperature);

	vm_object_unlock(object);
	thread_wakeup(object);
}


/*
 *  Push a range of pages in a map.
 */
static void
deactivate_range(vm_map_t map, vm_offset_t start, vm_offset_t end, int temp)
{
	vm_map_entry_t	entry;
	vm_object_t	object;
	vm_offset_t	tend;
	int		offset;

	vm_map_lock_read(map);

	/*
	 * Push all the pages in this map.
	 */
	for (entry = vm_map_first_entry(map);
	     entry != vm_map_to_entry(map);
	     entry = entry->vme_next)
	{
		/*
		 * Recurse over sub-maps.
		 */
		if (entry->is_a_map || entry->is_sub_map) {
			vm_map_t sub_map;
			sub_map = entry->object.sub_map;
			deactivate_range(sub_map, start, end, temp);
			continue;
		}
		if (entry->vme_start > end || entry->vme_end <= start)
			continue;

		/*
		 * Push the pages we can see in this object chain.
		 */
		if (entry->vme_start > start)
			start = entry->vme_start;

		if (entry->vme_end < end)
			tend = entry->vme_end;
		else
			tend = end;

		object = entry->object.vm_object;
		offset = entry->offset + start - entry->vme_start;
		deactivate_object(object, offset, offset + tend - start, temp);
	}
	vm_map_unlock(map);
}


/*
 *  Push a range of pages in a map.
 */
static void
set_policy(vm_map_t map, vm_offset_t start, vm_offset_t end, int policy)
{
	vm_map_entry_t	entry;
	vm_object_t	object;
	vm_offset_t	tend;
	int		offset;

	vm_map_lock_read(map);

	/*
	 * Push all the pages in this map.
	 */
	for (entry = vm_map_first_entry(map);
	     entry != vm_map_to_entry(map);
	     entry = entry->vme_next)
	{
		/*
		 * Recurse over sub-maps.
		 */
		if (entry->is_a_map || entry->is_sub_map) {
			vm_map_t sub_map;
			sub_map = entry->object.sub_map;
			set_policy(sub_map, start, end, policy);
			continue;
		}
		if (entry->vme_start > end || entry->vme_end <= start)
			continue;

		/*
		 *  Set the policy on the object
		 */
		object = entry->object.vm_object;
		while (object != VM_OBJECT_NULL) {
			object->policy = policy;
			object = object->shadow;
		}
	}
	vm_map_unlock(map);
}


/*
 *  Set the policy on a range of pages.  We'd really like to do this on a
 *  per-object basis... maybe this should only be applied to pagers?
 */
kern_return_t
vm_set_policy(vm_map_t map, vm_address_t addr, vm_size_t size, int policy)
{
	if (!map)
		return KERN_FAILURE;

	if (size == 0)
		size = map->max_offset - map->min_offset;

	if (addr == 0)
		addr = map->min_offset;

	set_policy(map, addr, addr + size, policy);
	return KERN_SUCCESS;
}


/*
 *  Fault a range of pages with a single disk transfer.  Bypass all the normal
 *  vm_fault crap.
 */
kern_return_t
vm_fault_range(vm_map_t map, vm_address_t addr, vm_size_t size)
{
	return KERN_INVALID_ARGUMENT;
}


/*
 *  Deactivate a range of pages.
 */
kern_return_t
vm_deactivate(vm_map_t map, vm_address_t addr, vm_size_t size, int temperature)
{
	if (!map)
		return KERN_FAILURE;

	if (size == 0)
		size = map->max_offset - map->min_offset;

	if (addr == 0)
		addr = map->min_offset;

	deactivate_range(map, addr, size, temperature);
	return KERN_SUCCESS;
}


