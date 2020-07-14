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
 * Copyright (c) 1990 NeXT, Inc.
 *
 *  History:
 *
 *	6-Aug-90: Brian Pinkerton at NeXT
 *		created
 */
 

#import <mach_xp.h>

#import <sys/types.h>
#import <sys/param.h>

#import <mach/vm_param.h>
#import <vm/vm_object.h>
#import <vm/vm_map.h>
#import <vm/vm_page.h>
#import <mach/vm_statistics.h>

#import <mach/boolean.h>
#import <mach/kern_return.h>
#import <kern/task.h>
#import <kern/kalloc.h>

#import <mach/mach_types.h>	/* to get vm_address_t */


static kern_return_t map_push_range(vm_map_t map, vm_offset_t start, vm_offset_t end);
	
static kern_return_t object_push(vm_object_t object, vm_offset_t object_start, vm_offset_t object_end);
	
enum pageout_outcome { PAGEOUT_SUCCESS, PAGEOUT_ERROR, PAGEOUT_RERUN };

/*
 * Push all pages to disk.
 */
kern_return_t
vm_synchronize(
	vm_map_t	task_map,
	vm_address_t	vmaddr,
	vm_size_t	vmsize)
{
	kern_return_t	result;

	if (!task_map)
		return KERN_FAILURE;

	if (vmsize == 0)
		vmsize = task_map->max_offset - task_map->min_offset;

	if (vmaddr == 0)
		vmaddr = task_map->min_offset;

	result = map_push_range(task_map, vmaddr, vmaddr+vmsize);
	
	return result;
}


/*
 * Push a range of pages in a map.
 */
static kern_return_t
map_push_range(
	vm_map_t	map,
	vm_offset_t	start,
	vm_offset_t	end)
{
	vm_map_entry_t	entry;
	vm_object_t	object;
	vm_offset_t	tend;
	int		offset;
	kern_return_t	rtn, realRtn = KERN_SUCCESS;

	vm_map_lock_read(map);
	/*
	 * Push all the pages in this map.
	 */
	for (  entry = vm_map_first_entry(map)
	     ; entry != vm_map_to_entry(map)
	     ; entry = entry->vme_next)
	{
		if (entry->is_a_map || entry->is_sub_map) {
			vm_map_t sub_map;
			/*
			 * Recurse over this sub-map.
			 */
			sub_map = entry->object.sub_map;
			rtn = map_push_range(sub_map, start, end);
			if (rtn == KERN_FAILURE)
				realRtn = KERN_FAILURE;
			continue;
		}
		if (entry->vme_start > end || entry->vme_end <= start)
			continue;

		/*
		 * Push the pages we can see in this object chain.
		 */
		object = entry->object.vm_object;
		if (entry->vme_start > start)
			start = entry->vme_start;
		tend = (entry->vme_end > end) ? end : entry->vme_end;
		offset = entry->offset + start - entry->vme_start;
		rtn = object_push(object, offset, offset + tend - start);
		if (rtn)
			realRtn = KERN_FAILURE;
	}
	vm_map_unlock(map);

	return realRtn;
}


/*
 *  Page out a page belonging to the given object.  The object must be locked.
 */
static enum pageout_outcome
vm_pageout_page(vm_object_t object, vm_page_t m)
{
	enum pageout_outcome pageout_succeeded;
	vm_pager_t pager = object->pager;
	
	vm_page_lock_queues();
	if (m->clean && !pmap_is_modified(VM_PAGE_TO_PHYS(m))) {
		vm_page_unlock_queues();
		return PAGEOUT_SUCCESS;
	}

	/*
	 *  If the page is already busy, sleep on it...
	 */
	while (m->busy) {
		vm_page_unlock_queues();
		PAGE_ASSERT_WAIT(m, FALSE);
		vm_object_unlock(object);
		thread_block();
		vm_object_lock(object);
		return PAGEOUT_RERUN;
	}
	
	object->paging_in_progress++;
	
	m->busy = TRUE;
	if (m->inactive)
		vm_page_activate(m);
	vm_page_deactivate(m);
	
	pmap_remove_all(VM_PAGE_TO_PHYS(m));
	vm_stat.pageouts++;

	vm_page_unlock_queues();
	if (pager == vm_pager_null) {
		object->paging_in_progress--;
		return PAGEOUT_ERROR;
	}
	vm_object_unlock(object);
	
	pageout_succeeded = PAGEOUT_ERROR;
	if (pager != vm_pager_null) {
		if (vm_pager_put(pager, m) == PAGER_SUCCESS) {
			pageout_succeeded = PAGEOUT_SUCCESS;
		}
	}

	vm_object_lock(object);
	vm_page_lock_queues();
	m->busy = FALSE;
	PAGE_WAKEUP(m);
	
	object->paging_in_progress--;
	vm_page_unlock_queues();
	
	return pageout_succeeded;
}


/*
 *  Count resident pages in this object and all shadow objects over the
 *  specified address range.
 *
 *  If we encounter an error, we'll attempt to push the rest of the pages,
 *  and return an error for the whole object.
 *
 *  If we encounter a locked page, then we'll restart
 */
static kern_return_t
object_push(vm_object_t object, vm_offset_t object_start, vm_offset_t object_end)
{
	vm_object_t	shadow;
	vm_page_t	page;
	vm_size_t	object_size;
	int		flags;
	int		allPageoutsSucceeded = TRUE;
	kern_return_t	rtn = FALSE;

	if (object == VM_OBJECT_NULL)
		return KERN_SUCCESS;

	vm_object_lock(object);

retry:
	for (  page = (vm_page_t) queue_first(&object->memq)
	     ; !queue_end(&object->memq, (queue_entry_t) page)
	     ; page = (vm_page_t) queue_next(&page->listq))
	{
		/*
		 * If this page isn't actually referenced from this
		 * entry, then forget it.
		 */
		if (page->offset < object_start || page->offset >= object_end)
			continue;

		/*
		 *  Push this page.  If we fail, retry on the whole object.
		 */
		switch (vm_pageout_page(object, page)) {
			case PAGEOUT_RERUN:
				goto retry;
			case PAGEOUT_ERROR:
				allPageoutsSucceeded = FALSE;
			case PAGEOUT_SUCCESS:
			default:
				break;
		}
	}

	/*
	 * Recurse over shadow objects.
	 */
	object_size = object_end - object_start;
	if (object->size && object_end - object_start > object->size)
		object_size = object->size;
	object_start += object->shadow_offset;
	object_end = object_start + object_size;

	if (object_push(object->shadow, object_start, object_end) != KERN_SUCCESS)
		rtn = KERN_FAILURE;
	
	vm_object_unlock(object);
	thread_wakeup(object);
	
	if (rtn == KERN_FAILURE || !allPageoutsSucceeded)
	    	return KERN_FAILURE;
	else
		return KERN_SUCCESS;
}

