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
 * Copyright (c) 1993-1987 Carnegie Mellon University
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
 *	File:	vm/vm_map.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Virtual memory mapping module.
 */

#import <mach/features.h>

#define	USE_VERSIONS	MACH_XP

#import <mach/vm_param.h>
#import <vm/vm_map.h>
#import <kern/zalloc.h>
#import <mach/kern_return.h>
#import <vm/vm_page.h>
#import <vm/vm_object.h>
#import <mach/port.h>
#import <mach/vm_attributes.h>


/*
 * Macros to copy a vm_map_entry. We must be careful to correctly
 * manage the wired page count. vm_map_entry_copy() creates a new
 * map entry to the same memory - the wired count in the new entry
 * must be set to zero. vm_map_entry_copy_full() creates a new
 * entry that is identical to the old entry.  This preserves the
 * wire count; it's used for map splitting and zone changing in
 * vm_map_copyout.
 */
#define vm_map_entry_copy(NEW,OLD) \
MACRO_BEGIN                                     \
                *(NEW) = *(OLD);                \
                (NEW)->is_shared = FALSE;	\
                (NEW)->needs_wakeup = FALSE;    \
                (NEW)->in_transition = FALSE;   \
                (NEW)->wired_count = 0;         \
                (NEW)->user_wired_count = 0;    \
MACRO_END

#define vm_map_entry_copy_full(NEW,OLD)        (*(NEW) = *(OLD))

/*
 *	Virtual memory maps provide for the mapping, protection,
 *	and sharing of virtual memory objects.  In addition,
 *	this module provides for an efficient virtual copy of
 *	memory from one map to another.
 *
 *	Synchronization is required prior to most operations.
 *
 *	Maps consist of an ordered doubly-linked list of simple
 *	entries; a single hint is used to speed up lookups.
 *
 *	In order to properly represent the sharing of virtual
 *	memory regions among maps, the map structure is bi-level.
 *	Top-level ("address") maps refer to regions of sharable
 *	virtual memory.  These regions are implemented as
 *	("sharing") maps, which then refer to the actual virtual
 *	memory objects.  When two address maps "share" memory,
 *	their top-level maps both have references to the same
 *	sharing map.  When memory is virtual-copied from one
 *	address map to another, the references in the sharing
 *	maps are actually copied -- no copying occurs at the
 *	virtual memory object level.
 *
 *	Since portions of maps are specified by start/end addreses,
 *	which may not align with existing map entries, all
 *	routines merely "clip" entries to these start/end values.
 *	[That is, an entry is split into two, bordering at a
 *	start or end value.]  Note that these clippings may not
 *	always be necessary (as the two resulting entries are then
 *	not changed); however, the clipping is done for convenience.
 *	No attempt is currently made to "glue back together" two
 *	abutting entries.
 *
 *	As mentioned above, virtual copy operations are performed
 *	by copying VM object references from one sharing map to
 *	another, and then marking both regions as copy-on-write.
 *	It is important to note that only one writeable reference
 *	to a VM object region exists in any map -- this means that
 *	shadow object creation can be delayed until a write operation
 *	occurs.
 */

zone_t		vm_map_zone;		/* zone for vm_map structures */
zone_t		vm_map_entry_zone;	/* zone for vm_map_entry structures */
zone_t		vm_map_kentry_zone;	/* zone for kernel entry structures */

vm_object_t	vm_submap_object;

/*
 *	vm_map_init:
 *
 *	Initialize the vm_map module.  Must be called before
 *	any other vm_map routines.
 *
 *	Map and entry structures are allocated from zones -- we must
 *	initialize those zones.
 *
 *	There are three zones of interest:
 *
 *	vm_map_zone:		used to allocate maps.
 *	vm_map_entry_zone:	used to allocate map entries.
 *	vm_map_kentry_zone:	used to allocate map entries for the kernel.
 *
 *	The kernel allocates map entries from a special zone that is initially
 *	"crammed" with memory.  It would be difficult (perhaps impossible) for
 *	the kernel to allocate more memory to a entry zone when it became
 *	empty since the very act of allocating memory implies the creatio
 *	of a new entry.  Further, since the kernel map is created from the
 *	map zone, the map zone is initially "crammed" with enough memory
 *	to fullfill that need.
 */

void vm_map_init()
{
	extern vm_offset_t	map_data, kentry_data;
	extern vm_size_t	map_data_size, kentry_data_size;

	vm_map_zone = zinit((vm_size_t) sizeof(struct vm_map), 100*1024,
					0, FALSE, "maps");
	vm_map_entry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					1024*1024, 0,
					FALSE, "non-kernel map entries");
	vm_map_kentry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					kentry_data_size, 0,
					FALSE, "kernel map entries");
	zchange(vm_map_kentry_zone, FALSE, FALSE, FALSE, FALSE);

	/*
	 *	Cram the map and kentry zones with initial data.
	 */
	zcram(vm_map_zone, map_data, map_data_size);
	zcram(vm_map_kentry_zone, kentry_data, kentry_data_size);
}

/*
 *	vm_map_create:
 *
 *	Creates and returns a new empty VM map with
 *	the given physical map structure, and having
 *	the given lower and upper address bounds.
 */
vm_map_t vm_map_create(pmap, min, max, pageable)
	pmap_t		pmap;
	vm_offset_t	min, max;
	boolean_t	pageable;
{
	register vm_map_t	result;

	result = (vm_map_t) zalloc(vm_map_zone);
	if (result == VM_MAP_NULL)
		panic("vm_map_create");

	vm_map_first_entry(result) = vm_map_to_entry(result);
	vm_map_last_entry(result)  = vm_map_to_entry(result);
	result->hdr.nentries = 0;
	result->hdr.entries_pageable = pageable;

	result->size = 0;
	result->ref_count = 1;
	result->pmap = pmap;
#if	OLD_VM_CODE
	result->is_main_map = TRUE;
#endif
	result->min_offset = min;
	result->max_offset = max;
	result->wiring_required = FALSE;
	result->wait_for_space = FALSE;
	result->first_free = vm_map_to_entry(result);
	result->hint = vm_map_to_entry(result);
#if	OLD_VM_CODE
	result->timestamp = 0;
#endif
	vm_map_lock_init(result);
	simple_lock_init(&result->ref_lock);
	simple_lock_init(&result->hint_lock);

	return(result);
}

/*
 *	vm_map_entry_create:	[ internal use only ]
 *
 *	Allocates a VM map entry for insertion in the
 *	given map (or map copy).  No fields are filled.
 */
#define	vm_map_entry_create(map) \
	    _vm_map_entry_create(&(map)->hdr)

#define	vm_map_copy_entry_create(copy) \
	    _vm_map_entry_create(&(copy)->cpy_hdr)

vm_map_entry_t _vm_map_entry_create(map_header)
	register struct vm_map_header *map_header;
{
	register zone_t	zone;
	register vm_map_entry_t	entry;

	if (map_header->entries_pageable)
	    zone = vm_map_entry_zone;
	else
	    zone = vm_map_kentry_zone;

	entry = (vm_map_entry_t) zalloc(zone);
	if (entry == VM_MAP_ENTRY_NULL)
		panic("vm_map_entry_create");

	return(entry);
}

/*
 *	vm_map_entry_dispose:	[ internal use only ]
 *
 *	Inverse of vm_map_entry_create.
 */
#define	vm_map_entry_dispose(map, entry) \
	_vm_map_entry_dispose(&(map)->hdr, (entry))

#define	vm_map_copy_entry_dispose(map, entry) \
	_vm_map_entry_dispose(&(copy)->cpy_hdr, (entry))

void _vm_map_entry_dispose(map_header, entry)
	register struct vm_map_header *map_header;
	register vm_map_entry_t	entry;
{
	register zone_t		zone;

	if (map_header->entries_pageable)
	    zone = vm_map_entry_zone;
	else
	    zone = vm_map_kentry_zone;

	zfree(zone, (vm_offset_t) entry);
}

/*
 *	vm_map_entry_{un,}link:
 *
 *	Insert/remove entries from maps (or map copies).
 */
#define vm_map_entry_link(map, after_where, entry)	\
	_vm_map_entry_link(&(map)->hdr, after_where, entry)

#define vm_map_copy_entry_link(copy, after_where, entry)	\
	_vm_map_entry_link(&(copy)->cpy_hdr, after_where, entry)

#define _vm_map_entry_link(hdr, after_where, entry)	\
	MACRO_BEGIN					\
	(hdr)->nentries++;				\
	(entry)->vme_prev = (after_where);		\
	(entry)->vme_next = (after_where)->vme_next;	\
	(entry)->vme_prev->vme_next =			\
	 (entry)->vme_next->vme_prev = (entry);		\
	MACRO_END

#define vm_map_entry_unlink(map, entry)			\
	_vm_map_entry_unlink(&(map)->hdr, entry)

#define vm_map_copy_entry_unlink(copy, entry)			\
	_vm_map_entry_unlink(&(copy)->cpy_hdr, entry)

#define _vm_map_entry_unlink(hdr, entry)		\
	MACRO_BEGIN					\
	(hdr)->nentries--;				\
	(entry)->vme_next->vme_prev = (entry)->vme_prev; \
	(entry)->vme_prev->vme_next = (entry)->vme_next; \
	MACRO_END

/*
 *	vm_map_reference:
 *
 *	Creates another valid reference to the given map.
 *
 */
void vm_map_reference(map)
	register vm_map_t	map;
{
	if (map == VM_MAP_NULL)
		return;

	simple_lock(&map->ref_lock);
	map->ref_count++;
	simple_unlock(&map->ref_lock);
}

/*
 *	vm_map_deallocate:
 *
 *	Removes a reference from the specified map,
 *	destroying it if no references remain.
 *	The map should not be locked.
 */
void vm_map_deallocate(map)
	register vm_map_t	map;
{
	register int		c;

	if (map == VM_MAP_NULL)
		return;

	simple_lock(&map->ref_lock);
	c = --map->ref_count;
	simple_unlock(&map->ref_lock);

	if (c > 0) {
		return;
	}

	/*
	 *	Lock the map, to wait out all other references
	 *	to it.
	 */

	vm_map_lock(map);

	(void) vm_map_delete(map, map->min_offset, map->max_offset);

	pmap_destroy(map->pmap);

	zfree(vm_map_zone, (vm_offset_t) map);
}

/*
 *	vm_map_insert:	[ internal use only ]
 *
 *	Inserts the given whole VM object into the target
 *	map at the specified address range.  The object's
 *	size should match that of the address range.
 *
 *	Requires that the map be locked, and leaves it so.
 */
kern_return_t vm_map_insert(map, object, offset, start, end)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	start;
	vm_offset_t	end;
{
	vm_map_entry_t		new_entry;
	vm_map_entry_t		prev_entry;
	boolean_t vm_map_lookup_entry();

	/*
	 *	Check that the start and end points are not bogus.
	 */

	if ((start < map->min_offset) || (end > map->max_offset) ||
			(start >= end))
		return(KERN_INVALID_ADDRESS);

	/*
	 *	Find the entry prior to the proposed
	 *	starting address; if it's part of an
	 *	existing entry, this range is bogus.
	 */

	if (vm_map_lookup_entry(map, start, &prev_entry))
		return(KERN_NO_SPACE);

	/*
	 *	Assert that the next entry doesn't overlap the
	 *	end point.
	 */

	if ((prev_entry->vme_next != vm_map_to_entry(map)) &&
			(prev_entry->vme_next->vme_start < end))
		return(KERN_NO_SPACE);

	/*
	 *	See if we can avoid creating a new entry by
	 *	extending one of our neighbors.
	 */

	if (object == VM_OBJECT_NULL) {
		if ((prev_entry != vm_map_to_entry(map)) &&
		    (prev_entry->vme_end == start) &&
		    (prev_entry->is_a_map == FALSE) &&
		    (prev_entry->is_sub_map == FALSE) &&
		    (prev_entry->inheritance == VM_INHERIT_DEFAULT) &&
		    (prev_entry->protection == VM_PROT_DEFAULT) &&
		    (prev_entry->max_protection == VM_PROT_ALL) &&
		    (prev_entry->wired_count == 0)) {

			if (vm_object_coalesce(prev_entry->object.vm_object,
					VM_OBJECT_NULL,
					prev_entry->offset,
					(vm_offset_t) 0,
					(vm_size_t)(prev_entry->vme_end
						     - prev_entry->vme_start),
					(vm_size_t)(end -
						prev_entry->vme_end))) {
				/*
				 *	Coalesced the two objects - can extend
				 *	the previous map entry to include the
				 *	new range.
				 */
				map->size += (end - prev_entry->vme_end);
				prev_entry->vme_end = end;
				return(KERN_SUCCESS);
			}
		}
	}

	/*
	 *	Create a new entry
	 */

	new_entry = vm_map_entry_create(map);
	new_entry->vme_start = start;
	new_entry->vme_end = end;

	new_entry->is_a_map = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->object.vm_object = object;
	new_entry->offset = offset;

	new_entry->copy_on_write = FALSE;
	new_entry->needs_copy = FALSE;

	if (map->is_main_map) {
		new_entry->inheritance = VM_INHERIT_DEFAULT;
		new_entry->protection = VM_PROT_DEFAULT;
		new_entry->max_protection = VM_PROT_ALL;
		new_entry->wired_count = 0;
	}

	/*
	 *	Insert the new entry into the list
	 */

	vm_map_entry_link(map, prev_entry, new_entry);
	map->size += new_entry->vme_end - new_entry->vme_start;

	/*
	 *	Update the free space hint
	 */

	if ((map->first_free == prev_entry) && (prev_entry->vme_end >= new_entry->vme_start))
		map->first_free = new_entry;

	return(KERN_SUCCESS);
}

/*
 *	SAVE_HINT:
 *
 *	Saves the specified entry as the hint for
 *	future lookups.  Performs necessary interlocks.
 */
#define	SAVE_HINT(map,value) \
		simple_lock(&(map)->hint_lock); \
		(map)->hint = (value); \
		simple_unlock(&(map)->hint_lock);

/*
 *	vm_map_lookup_entry:	[ internal use only ]
 *
 *	Finds the map entry containing (or
 *	immediately preceding) the specified address
 *	in the given map; the entry is returned
 *	in the "entry" parameter.  The boolean
 *	result indicates whether the address is
 *	actually contained in the map.
 */
boolean_t vm_map_lookup_entry(map, address, entry)
	register vm_map_t	map;
	register vm_offset_t	address;
	vm_map_entry_t		*entry;		/* OUT */
{
	register vm_map_entry_t		cur;
	register vm_map_entry_t		last;

	/*
	 *	Start looking either from the head of the
	 *	list, or from the hint.
	 */

	simple_lock(&map->hint_lock);
	cur = map->hint;
	simple_unlock(&map->hint_lock);

	if (cur == vm_map_to_entry(map))
		cur = cur->vme_next;

	if (address >= cur->vme_start) {
	    	/*
		 *	Go from hint to end of list.
		 *
		 *	But first, make a quick check to see if
		 *	we are already looking at the entry we
		 *	want (which is usually the case).
		 *	Note also that we don't need to save the hint
		 *	here... it is the same hint (unless we are
		 *	at the header, in which case the hint didn't
		 *	buy us anything anyway).
		 */
		last = vm_map_to_entry(map);
		if ((cur != last) && (cur->vme_end > address)) {
			*entry = cur;
			return(TRUE);
		}
	}
	else {
	    	/*
		 *	Go from start to hint, *inclusively*
		 */
		last = cur->vme_next;
		cur = vm_map_first_entry(map);
	}

	/*
	 *	Search linearly
	 */

	while (cur != last) {
		if (cur->vme_end > address) {
			if (address >= cur->vme_start) {
			    	/*
				 *	Save this lookup for future
				 *	hints, and return
				 */

				*entry = cur;
				SAVE_HINT(map, cur);
				return(TRUE);
			}
			break;
		}
		cur = cur->vme_next;
	}
	*entry = cur->vme_prev;
	SAVE_HINT(map, *entry);
	return(FALSE);
}

/*
 *	Routine:	vm_map_find_entry
 *	Purpose:
 *		Allocate a range in the specified virtual address map,
 *		returning the entry allocated for that range.
 *		Used by kmem_alloc, etc.  Returns wired entries.
 *
 *		The map must be locked.
 *
 *		If an entry is allocated, the object/offset fields
 *		are initialized to zero.  If an object is supplied,
 *		then an existing entry may be extended.
 */
kern_return_t vm_map_find_entry(map, address, size, mask, object, o_entry)
	register vm_map_t	map;
	vm_offset_t		*address;	/* OUT */
	vm_size_t		size;
	vm_offset_t		mask;
	vm_object_t		object;
	vm_map_entry_t		*o_entry;	/* OUT */
{
	register vm_map_entry_t	entry, new_entry;
	register vm_offset_t	start;
	register vm_offset_t	end;

	/*
	 *	Look for the first possible address;
	 *	if there's already something at this
	 *	address, we have to start after it.
	 */

	if ((entry = map->first_free) == vm_map_to_entry(map))
		start = map->min_offset;
	else
		start = entry->vme_end;

	/*
	 *	In any case, the "entry" always precedes
	 *	the proposed new region throughout the loop:
	 */

	while (TRUE) {
		register vm_map_entry_t	next;

		/*
		 *	Find the end of the proposed new region.
		 *	Be sure we didn't go beyond the end, or
		 *	wrap around the address.
		 */

		start = ((start + mask) & ~mask);
		end = start + size;

		if ((end > map->max_offset) || (end < start))
			return(KERN_NO_SPACE);

		/*
		 *	If there are no more entries, we must win.
		 */

		next = entry->vme_next;
		if (next == vm_map_to_entry(map))
			break;

		/*
		 *	If there is another entry, it must be
		 *	after the end of the potential new region.
		 */

		if (next->vme_start >= end)
			break;

		/*
		 *	Didn't fit -- move to the next entry.
		 */

		entry = next;
		start = entry->vme_end;
	}

	/*
	 *	At this point,
	 *		"start" and "end" should define the endpoints of the
	 *			available new range, and
	 *		"entry" should refer to the region before the new
	 *			range, and
	 *
	 *		the map should be locked.
	 */

	*address = start;

	/*
	 *	See whether we can avoid creating a new entry by
	 *	extending one of our neighbors.  [So far, we only attempt to
	 *	extend from below.]
	 */

	if ((object != VM_OBJECT_NULL) &&
	    (entry != vm_map_to_entry(map)) &&
	    (entry->vme_end == start) &&
	    (!entry->is_shared) &&
	    (!entry->is_sub_map) &&
	    (entry->object.vm_object == object) &&
	    (entry->needs_copy == FALSE) &&
	    (entry->inheritance == VM_INHERIT_DEFAULT) &&
	    (entry->protection == VM_PROT_DEFAULT) &&
	    (entry->max_protection == VM_PROT_ALL) &&
	    (entry->wired_count == 1) &&
	    (entry->user_wired_count == 0)) {
		/*
		 *	Because this is a special case,
		 *	we don't need to use vm_object_coalesce.
		 */

		entry->vme_end = end;
		new_entry = entry;
	} else {
		new_entry = vm_map_entry_create(map);

		new_entry->vme_start = start;
		new_entry->vme_end = end;

		new_entry->is_shared = FALSE;
		new_entry->is_sub_map = FALSE;
		new_entry->object.vm_object = VM_OBJECT_NULL;
		new_entry->offset = (vm_offset_t) 0;

		new_entry->needs_copy = FALSE;

		new_entry->inheritance = VM_INHERIT_DEFAULT;
		new_entry->protection = VM_PROT_DEFAULT;
		new_entry->max_protection = VM_PROT_ALL;
		new_entry->wired_count = 1;
		new_entry->user_wired_count = 0;

		new_entry->in_transition = FALSE;
		new_entry->needs_wakeup = FALSE;

		/*
		 *	Insert the new entry into the list
		 */

		vm_map_entry_link(map, entry, new_entry);
    	}

	map->size += size;

	/*
	 *	Update the free space hint and the lookup hint
	 */

	map->first_free = new_entry;
	SAVE_HINT(map, new_entry);

	*o_entry = new_entry;
	return(KERN_SUCCESS);
}

/*
 *	vm_map_find finds an unallocated region in the target address
 *	map with the given length.  The search is defined to be
 *	first-fit from the specified address; the region found is
 *	returned in the same parameter.
 *
 */
kern_return_t vm_map_find(map, object, offset, addr, length, find_space)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	*addr;		/* IN/OUT */
	vm_size_t	length;
	boolean_t	find_space;
{
	register vm_map_entry_t	entry;
	register vm_offset_t	start;
	register vm_offset_t	end;
	kern_return_t		result;

	start = *addr;

	vm_map_lock(map);

	if (find_space) {
		/*
		 *	Calculate the first possible address.
		 */

		if (start < map->min_offset)
			start = map->min_offset;
		if (start > map->max_offset) {
			vm_map_unlock(map);
			return (KERN_NO_SPACE);
		}

		/*
		 *	Look for the first possible address;
		 *	if there's already something at this
		 *	address, we have to start after it.
		 */

		if (start == map->min_offset) {
			if ((entry = map->first_free) != vm_map_to_entry(map))
				start = entry->vme_end;
		} else {
			vm_map_entry_t	tmp_entry;
			if (vm_map_lookup_entry(map, start, &tmp_entry))
				start = tmp_entry->vme_end;
			entry = tmp_entry;
		}

		/*
		 *	In any case, the "entry" always precedes
		 *	the proposed new region throughout the
		 *	loop:
		 */

		while (TRUE) {
			register vm_map_entry_t	next;

		    	/*
			 *	Find the end of the proposed new region.
			 *	Be sure we didn't go beyond the end, or
			 *	wrap around the address.
			 */

			end = start + length;

			if ((end > map->max_offset) || (end < start)) {
				vm_map_unlock(map);
				return (KERN_NO_SPACE);
			}

			/*
			 *	If there are no more entries, we must win.
			 */

			next = entry->vme_next;
			if (next == vm_map_to_entry(map))
				break;

			/*
			 *	If there is another entry, it must be
			 *	after the end of the potential new region.
			 */

			if (next->vme_start >= end)
				break;

			/*
			 *	Didn't fit -- move to the next entry.
			 */

			entry = next;
			start = entry->vme_end;
		}
		*addr = start;
		
		SAVE_HINT(map, entry);
	}

	result = vm_map_insert(map, object, offset, start, start + length);
	
	vm_map_unlock(map);
	return(result);
}

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void _vm_map_clip_start();
#define vm_map_clip_start(map, entry, startaddr) \
	MACRO_BEGIN \
	if ((startaddr) > (entry)->vme_start) \
		_vm_map_clip_start(&(map)->hdr,(entry),(startaddr)); \
	MACRO_END

void _vm_map_copy_clip_start();
#define vm_map_copy_clip_start(copy, entry, startaddr) \
	MACRO_BEGIN \
	if ((startaddr) > (entry)->vme_start) \
		_vm_map_clip_start(&(copy)->cpy_hdr,(entry),(startaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
void _vm_map_clip_start(map_header, entry, start)
	register struct vm_map_header *map_header;
	register vm_map_entry_t	entry;
	register vm_offset_t	start;
{
	register vm_map_entry_t	new_entry;

	/*
	 *	Split off the front portion --
	 *	note that we must insert the new
	 *	entry BEFORE this one, so that
	 *	this entry has the specified starting
	 *	address.
	 */

	new_entry = _vm_map_entry_create(map_header);
	vm_map_entry_copy_full(new_entry, entry);

	new_entry->vme_end = start;
	entry->offset += (start - entry->vme_start);
	entry->vme_start = start;

	_vm_map_entry_link(map_header, entry->vme_prev, new_entry);

	if (entry->is_a_map || entry->is_sub_map)
	 	vm_map_reference(new_entry->object.share_map);
	else
		vm_object_reference(new_entry->object.vm_object);
}

/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void _vm_map_clip_end();
#define vm_map_clip_end(map, entry, endaddr) \
	MACRO_BEGIN \
	if ((endaddr) < (entry)->vme_end) \
		_vm_map_clip_end(&(map)->hdr,(entry),(endaddr)); \
	MACRO_END

void _vm_map_copy_clip_end();
#define vm_map_copy_clip_end(copy, entry, endaddr) \
	MACRO_BEGIN \
	if ((endaddr) < (entry)->vme_end) \
		_vm_map_clip_end(&(copy)->cpy_hdr,(entry),(endaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
void _vm_map_clip_end(map_header, entry, end)
	register struct vm_map_header *map_header;
	register vm_map_entry_t	entry;
	register vm_offset_t	end;
{
	register vm_map_entry_t	new_entry;

	/*
	 *	Create a new entry and insert it
	 *	AFTER the specified entry
	 */

	new_entry = _vm_map_entry_create(map_header);
	vm_map_entry_copy_full(new_entry, entry);

	new_entry->vme_start = entry->vme_end = end;
	new_entry->offset += (end - entry->vme_start);

	_vm_map_entry_link(map_header, entry, new_entry);

	if (entry->is_a_map || entry->is_sub_map)
	 	vm_map_reference(new_entry->object.share_map);
	else
		vm_object_reference(new_entry->object.vm_object);
}

/*
 *	VM_MAP_RANGE_CHECK:	[ internal use only ]
 *
 *	Asserts that the starting and ending region
 *	addresses fall within the valid range of the map.
 */
#define	VM_MAP_RANGE_CHECK(map, start, end)		\
		{					\
		if (start < vm_map_min(map))		\
			start = vm_map_min(map);	\
		if (end > vm_map_max(map))		\
			end = vm_map_max(map);		\
		if (start > end)			\
			start = end;			\
		}

/*
 *	vm_map_submap:		[ kernel use only ]
 *
 *	Mark the given range as handled by a subordinate map.
 *
 *	This range must have been created with vm_map_find,
 *	and no other operations may have been performed on this
 *	range prior to calling vm_map_submap.
 *
 *	Only a limited number of operations can be performed
 *	within this rage after calling vm_map_submap:
 *		vm_fault
 *	[Don't try vm_map_copy!]
 *
 *	To remove a submapping, one must first remove the
 *	range from the superior map, and then destroy the
 *	submap (if desired).  [Better yet, don't try it.]
 */
kern_return_t vm_map_submap(map, start, end, submap)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	vm_map_t		submap;
{
	vm_map_entry_t		entry;
	register kern_return_t	result = KERN_INVALID_ARGUMENT;
	register vm_object_t	object;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &entry)) {
		vm_map_clip_start(map, entry, start);
	}
	 else
		entry = entry->vme_next;

	vm_map_clip_end(map, entry, end);

	if ((entry->vme_start == start) && (entry->vme_end == end) &&
	    (!entry->is_a_map) &&
	    ((object = entry->object.vm_object) == vm_submap_object) &&
	    (!entry->copy_on_write)) {
		entry->object.vm_object = VM_OBJECT_NULL;
		vm_object_deallocate(object);
		entry->is_sub_map = TRUE;
		vm_map_reference(entry->object.sub_map = submap);
		result = KERN_SUCCESS;
	}
	vm_map_unlock(map);

	return(result);
}

/*
 *	vm_map_protect:
 *
 *	Sets the protection of the specified address
 *	region in the target map.  If "set_max" is
 *	specified, the maximum protection is to be set;
 *	otherwise, only the current protection is affected.
 */
kern_return_t vm_map_protect(map, start, end, new_prot, set_max)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_prot_t	new_prot;
	register boolean_t	set_max;
{
	register vm_map_entry_t		current;
	vm_map_entry_t			entry;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &entry)) {
		vm_map_clip_start(map, entry, start);
	}
	 else
		entry = entry->vme_next;

	/*
	 *	Make a first pass to check for protection
	 *	violations.
	 */

	current = entry;
	while ((current != vm_map_to_entry(map)) &&
	       (current->vme_start < end)) {
		if (current->is_sub_map) {
			vm_map_unlock(map);
			return(KERN_INVALID_ARGUMENT);
		}
		if ((new_prot & current->max_protection) != new_prot) {
			vm_map_unlock(map);
			return(KERN_PROTECTION_FAILURE);
		}

		current = current->vme_next;
	}

	/*
	 *	Go back and fix up protections.
	 *	[Note that clipping is not necessary the second time.]
	 */

	current = entry;

	while ((current != vm_map_to_entry(map)) &&
	       (current->vme_start < end)) {
		vm_prot_t	old_prot;

		vm_map_clip_end(map, current, end);

		old_prot = current->protection;
		if (set_max)
			current->protection =
				(current->max_protection = new_prot) &
					old_prot;
		else
			current->protection = new_prot;

		/*
		 *	Update physical map if necessary.
		 *	Worry about copy-on-write here -- CHECK THIS XXX
		 */

		if (current->protection != old_prot) {

#define MASK(entry)	((entry)->copy_on_write ? ~VM_PROT_WRITE : \
							VM_PROT_ALL)
#define	max(a,b)	((a) > (b) ? (a) : (b))

			if (current->is_a_map) {
				vm_map_entry_t	share_entry;
				vm_offset_t	share_end;

				vm_map_lock(current->object.share_map);
				(void) vm_map_lookup_entry(
						current->object.share_map,
						current->offset,
						&share_entry);
				share_end = current->offset +
					(current->vme_end -
						current->vme_start);
				while ((share_entry !=
					vm_map_to_entry(
						current->object.share_map)) &&
					(share_entry->vme_start < share_end)) {

					pmap_protect(map->pmap,
						(max(share_entry->vme_start,
							current->offset) -
							current->offset +
							current->vme_start),
						max(share_entry->vme_end,
							share_end) -
						current->offset +
						current->vme_start,
						current->protection &
							MASK(share_entry));

					share_entry = share_entry->vme_next;
				}
				vm_map_unlock(current->object.share_map);
			}
			else
			 	pmap_protect(map->pmap, current->vme_start,
					current->vme_end,
					current->protection & MASK(entry));
#undef	max
#undef	MASK
		}
		current = current->vme_next;
	}

	vm_map_unlock(map);
	return(KERN_SUCCESS);
}

/*
 *	vm_map_inherit:
 *
 *	Sets the inheritance of the specified address
 *	range in the target map.  Inheritance
 *	affects how the map will be shared with
 *	child maps at the time of vm_map_fork.
 */
kern_return_t vm_map_inherit(map, start, end, new_inheritance)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_inherit_t	new_inheritance;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t	temp_entry;

	switch (new_inheritance) {
	case VM_INHERIT_NONE:
	case VM_INHERIT_COPY:
	case VM_INHERIT_SHARE:
		break;
	default:
		return(KERN_INVALID_ARGUMENT);
	}

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else
		entry = temp_entry->vme_next;

	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->inheritance = new_inheritance;

		entry = entry->vme_next;
	}

	vm_map_unlock(map);
	return(KERN_SUCCESS);
}

#if NeXT
extern vm_map_t kernel_map;
#endif

/*
 *	vm_map_pageable:
 *
 *	Sets the pageability of the specified address
 *	range in the target map.  Regions specified
 *	as not pageable require locked-down physical
 *	memory and physical page maps.
 *
 *	The map must not be locked, but a reference
 *	must remain to the map throughout the call.
 */
kern_return_t vm_map_pageable(map, start, end, new_pageable)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register boolean_t	new_pageable;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		temp_entry;
#if NeXT
	int map_locked = 1;
#endif

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	/*
	 *	Only one pageability change may take place at one
	 *	time, since vm_fault assumes it will be called
	 *	only once for each wiring/unwiring.  Therefore, we
	 *	have to make sure we're actually changing the pageability
	 *	for the entire region.  We do so before making any changes.
	 */

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else
		entry = temp_entry->vme_next;
	temp_entry = entry;

	/*
	 *	Actions are rather different for wiring and unwiring,
	 *	so we have two separate cases.
	 */

	if (new_pageable) {

		/*
		 *	Unwiring.  First ensure that the range to be
		 *	unwired is really wired down.
		 */
		while ((entry != vm_map_to_entry(map)) &&
		       (entry->vme_start < end)) {

		    if (entry->wired_count == 0) {
			vm_map_unlock(map);
			return(KERN_INVALID_ARGUMENT);
		    }
		    entry = entry->vme_next;
		}

		/*
		 *	Now decrement the wiring count for each region.
		 *	If a region becomes completely unwired,
		 *	unwire its physical pages and mappings.
		 */
		entry = temp_entry;
		while ((entry != vm_map_to_entry(map)) && 
		       (entry->vme_start < end)) {
		    vm_map_clip_end(map, entry, end);

		    entry->wired_count--;
		    if (entry->wired_count == 0)
			vm_fault_unwire(map, entry);

		    entry = entry->vme_next;
		}
	}

	else {
		/*
		 *	Wiring.  We must do this in two passes:
		 *
		 *	1.  Holding the write lock, we increment the
		 *	    wiring count.  For any area that is not already
		 *	    wired, we create any shadow objects that need
		 *	    to be created.
		 *
		 *	2.  We downgrade to a read lock, and call
		 *	    vm_fault_wire to fault in the pages for any
		 *	    newly wired area (wired_count is 1).
		 *
		 *	Downgrading to a read lock for vm_fault_wire avoids
		 *	a possible deadlock with another thread that may have
		 *	faulted on one of the pages to be wired (it would mark
		 *	the page busy, blocking us, then in turn block on the
		 *	map lock that we hold).  Because of problems in the
		 *	recursive lock package, we cannot upgrade to a write
		 *	lock in vm_map_lookup.  Thus, any actions that require
		 *	the write lock must be done beforehand.  Because we
		 *	keep the read lock on the map, the copy-on-write status
		 *	of the entries we modify here cannot change.
		 */

		/*
		 *	Pass 1.
		 */
		entry = temp_entry;
		while ((entry != vm_map_to_entry(map)) &&
		       (entry->vme_start < end)) {
		    vm_map_clip_end(map, entry, end);

		    if (++entry->wired_count == 0)
			panic("vm_map_pageable: wired_count");
		    if (entry->wired_count == 1) {

			/*
			 *	Perform actions of vm_map_lookup that need
			 *	the write lock on the map: create a shadow
			 *	object for a copy-on-write region, or an
			 *	object for a zero-fill region.
			 *
			 *	We don't have to do this for entries that
			 *	point to sharing maps, because we won't hold
			 *	the lock on the sharing map.
			 */
			if (!entry->is_a_map) {
			    if (entry->needs_copy &&
				((entry->protection & VM_PROT_WRITE) != 0)) {

				vm_object_shadow(&entry->object.vm_object,
						&entry->offset,
						(vm_size_t)(entry->vme_end
							- entry->vme_start));
				entry->needs_copy = FALSE;
			    }
			    else if (entry->object.vm_object == VM_OBJECT_NULL) {
				entry->object.vm_object =
				    vm_object_allocate(
				    	(vm_size_t)(entry->vme_end
				    			- entry->vme_start));
				entry->offset = (vm_offset_t)0;
			    }
			}
		    }

		    entry = entry->vme_next;
		}

		/*
		 *	Pass 2.
		 */

		/* If we are wiring pages in the kernel map 
		 * we know that the map will not be modified in a
		 * destructive way. Drivers call us to wire in
		 * pages after they do a vm_map_copy into the
		 * kernel. Enhancing submaps to allow vm_map_copies
		 * is a better long term solution. For now we will 
		 * release the lock to prevent deadlock.
		 */
		if (map == kernel_map){
			map_locked = 0;
			vm_map_unlock(map);
		} else {
			lock_set_recursive(&map->lock);
			lock_write_to_read(&map->lock);
		}

		entry = temp_entry;
		while ((entry != vm_map_to_entry(map)) &&
		       (entry->vme_start < end)) {
		    if (entry->wired_count == 1) {
			vm_fault_wire(map, entry);
		    }
		    entry = entry->vme_next;
		}

		if (map_locked){
			lock_clear_recursive(&map->lock);
		}
	}

	if (map_locked){
		vm_map_unlock(map);
	}

	return(KERN_SUCCESS);
}

/*
 *	vm_map_entry_unwire:	[ internal use only ]
 *
 *	Make the region specified by this entry pageable.
 *
 *	The map in question should be locked.
 *	[This is the reason for this routine's existence.]
 */
void vm_map_entry_unwire(map, entry)
	vm_map_t		map;
	register vm_map_entry_t	entry;
{
	vm_fault_unwire(map, entry);
	entry->wired_count = 0;
}

/*
 *	vm_map_entry_delete:	[ internal use only ]
 *
 *	Deallocate the given entry from the target map.
 */		
void vm_map_entry_delete(map, entry)
	register vm_map_t	map;
	register vm_map_entry_t	entry;
{
	if (entry->wired_count != 0)
		vm_map_entry_unwire(map, entry);
		
	vm_map_entry_unlink(map, entry);
	map->size -= entry->vme_end - entry->vme_start;

	if (entry->is_a_map || entry->is_sub_map)
		vm_map_deallocate(entry->object.share_map);
	else
	 	vm_object_deallocate(entry->object.vm_object);

	vm_map_entry_dispose(map, entry);
}

/*
 *	vm_map_delete:	[ internal use only ]
 *
 *	Deallocates the given address range from the target
 *	map.
 *
 *	When called with a sharing map, removes pages from
 *	that region from all physical maps.
 */
kern_return_t vm_map_delete(map, start, end)
	register vm_map_t	map;
	vm_offset_t		start;
	register vm_offset_t	end;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		first_entry;

	/*
	 *	Find the start of the region, and clip it
	 */

	if (!vm_map_lookup_entry(map, start, &first_entry))
		entry = first_entry->vme_next;
	else {
		entry = first_entry;
		vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		SAVE_HINT(map, entry->vme_prev);
	}

	/*
	 *	Save the free space hint
	 */

	if (map->first_free->vme_start >= start)
		map->first_free = entry->vme_prev;

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != vm_map_to_entry(map)) &&
	       (entry->vme_start < end)) {
		vm_map_entry_t		next;
		register vm_offset_t	s, e;
		register vm_object_t	object;
		extern vm_object_t	kernel_object;

		vm_map_clip_end(map, entry, end);

		next = entry->vme_next;
		s = entry->vme_start;
		e = entry->vme_end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		object = entry->object.vm_object;
		if (entry->wired_count != 0)
			vm_map_entry_unwire(map, entry);

		if (object == kernel_object)
			vm_object_page_remove(object, entry->offset,
					entry->offset + (e - s));
		
		/*
		 *	If this is a sharing map, we must remove
		 *	*all* references to this data, since we can't
		 *	find all of the physical maps which are sharing
		 *	it.
		 */

		if (!map->is_main_map)
			vm_object_pmap_remove(object,
					 entry->offset,
					 entry->offset + (e - s));

		pmap_remove(map->pmap, s, e);

		/*
		 *	Delete the entry (which may delete the object)
		 *	only after removing all pmap entries pointing
		 *	to its pages.  (Otherwise, its page frames may
		 *	be reallocated, and any modify bits will be
		 *	set in the wrong object!)
		 */

		vm_map_entry_delete(map, entry);
		entry = next;
	}
	return(KERN_SUCCESS);
}

/*
 *	vm_map_remove:
 *
 *	Remove the given address range from the target map.
 *	This is the exported form of vm_map_delete.
 */
kern_return_t vm_map_remove(map, start, end)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register kern_return_t	result;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	result = vm_map_delete(map, start, end);
	vm_map_unlock(map);

	return(result);
}

kern_return_t vm_map_reallocate(map, start, end)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		first_entry;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);

	/*
	 *	Find the start of the region, and clip it
	 */

	if (!vm_map_lookup_entry(map, start, &first_entry))
		entry = first_entry->vme_next;
	else {
		entry = first_entry;
		vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		SAVE_HINT(map, entry->vme_prev);
	}

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != vm_map_to_entry(map)) &&
	       (entry->vme_start < end)) {
		vm_map_entry_t		next;
		register vm_offset_t	s, e;
		register vm_object_t	object;
		extern vm_object_t	kernel_object;

		vm_map_clip_end(map, entry, end);

		next = entry->vme_next;
		s = entry->vme_start;
		e = entry->vme_end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		object = entry->object.vm_object;
		if (entry->wired_count != 0)
			vm_map_entry_unwire(map, entry);

		if (object == kernel_object)
			vm_object_page_remove(object, entry->offset,
					entry->offset + (e - s));
		
		/*
		 *	If this is a sharing map, we must remove
		 *	*all* references to this data, since we can't
		 *	find all of the physical maps which are sharing
		 *	it.
		 */

		if (!map->is_main_map)
			vm_object_pmap_remove(object,
					 entry->offset,
					 entry->offset + (e - s));

		pmap_remove(map->pmap, s, e);

		/*
		 *	Release the reference to the underlying object
		 *	only after removing all pmap entries pointing
		 *	to its pages.  (Otherwise, its page frames may
		 *	be reallocated, and any modify bits will be
		 *	set in the wrong object!)
		 */

		if (entry->is_a_map || entry->is_sub_map)
			vm_map_deallocate(entry->object.share_map);
		else
			vm_object_deallocate(entry->object.vm_object);

		entry->is_a_map = FALSE;
		entry->is_sub_map = FALSE;
		entry->object.vm_object = VM_OBJECT_NULL;
		entry->offset = 0;

		entry->copy_on_write = FALSE;
		entry->needs_copy = FALSE;

		if (map->is_main_map) {
			entry->protection =
				(entry->max_protection & VM_PROT_DEFAULT);
			entry->wired_count = 0;
		}

		entry = next;
	}

	vm_map_unlock(map);

	return(KERN_SUCCESS);
}

/*
 *	vm_map_check_protection:
 *
 *	Assert that the target map allows the specified
 *	privilege on the entire address region given.
 *	The entire region must be allocated.
 */
boolean_t vm_map_check_protection(map, start, end, protection)
	register vm_map_t	map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_prot_t	protection;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		tmp_entry;

	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		return(FALSE);
	}

	entry = tmp_entry;

	while (start < end) {
		if (entry == vm_map_to_entry(map)) {
			return(FALSE);
		}

		/*
		 *	No holes allowed!
		 */

		if (start < entry->vme_start) {
			return(FALSE);
		}

		/*
		 * Check protection associated with entry.
		 */

		if ((entry->protection & protection) != protection) {
			return(FALSE);
		}

		/* go to next entry */

		start = entry->vme_end;
		entry = entry->vme_next;
	}
	return(TRUE);
}

/*
 *	vm_map_copy_entry:
 *
 *	Copies the contents of the source entry to the destination
 *	entry.  The entries *must* be aligned properly.
 */
void vm_map_copy_entry(src_map, dst_map, src_entry, dst_entry)
	vm_map_t		src_map, dst_map;
	register vm_map_entry_t	src_entry, dst_entry;
{
	vm_object_t	temp_object;

	if (src_entry->is_sub_map || dst_entry->is_sub_map)
		return;

	/*
	 *	If our destination map was wired down,
	 *	unwire it now.
	 */

	if (dst_entry->wired_count != 0)
		vm_map_entry_unwire(dst_map, dst_entry);

	/*
	 *	If we're dealing with a sharing map, we
	 *	must remove the destination pages from
	 *	all maps (since we cannot know which maps
	 *	this sharing map belongs in).
	 */

	if (!dst_map->is_main_map)
		vm_object_pmap_remove(dst_entry->object.vm_object,
			dst_entry->offset,
			dst_entry->offset +
				(dst_entry->vme_end - dst_entry->vme_start));

	pmap_remove(dst_map->pmap, dst_entry->vme_start, dst_entry->vme_end);

	if (src_entry->wired_count == 0) {

		boolean_t	src_needs_copy;

		/*
		 *	If the source entry is marked needs_copy,
		 *	it is already write-protected.
		 */
		if (!src_entry->needs_copy) {

			boolean_t	su;

			/*
			 *	If the source entry has only one mapping,
			 *	we can just protect the virtual address
			 *	range.
			 */
			if (!(su = src_map->is_main_map)) {
				simple_lock(&src_map->ref_lock);
				su = (src_map->ref_count == 1);
				simple_unlock(&src_map->ref_lock);
			}

			if (su) {
				pmap_protect(src_map->pmap,
					src_entry->vme_start,
					src_entry->vme_end,
					src_entry->protection & ~VM_PROT_WRITE);
			}
			else {
				vm_object_pmap_copy(src_entry->object.vm_object,
					src_entry->offset,
					src_entry->offset + (src_entry->vme_end
							    -src_entry->vme_start));
			}
		}

		/*
		 *	Make a copy of the object.
		 */
		temp_object = dst_entry->object.vm_object;
		vm_object_copy(src_entry->object.vm_object,
				src_entry->offset,
				(vm_size_t)(src_entry->vme_end -
					    src_entry->vme_start),
				&dst_entry->object.vm_object,
				&dst_entry->offset,
				&src_needs_copy);
		/*
		 *	If we didn't get a copy-object now, mark the
		 *	source map entry so that a shadow will be created
		 *	to hold its changed pages.
		 */
		if (src_needs_copy)
			src_entry->needs_copy = TRUE;

		/*
		 *	The destination always needs to have a shadow
		 *	created.
		 */
		dst_entry->needs_copy = TRUE;

		/*
		 *	Mark the entries copy-on-write, so that write-enabling
		 *	the entry won't make copy-on-write pages writable.
		 */
		src_entry->copy_on_write = TRUE;
		dst_entry->copy_on_write = TRUE;
/* XXX */
		if (src_entry->protection & VM_PROT_EXECUTE)
			dst_entry->protection |= (VM_PROT_EXECUTE & dst_entry->max_protection);
/* XXX */
		/*
		 *	Get rid of the old object.
		 */
		vm_object_deallocate(temp_object);

		pmap_copy(dst_map->pmap, src_map->pmap, dst_entry->vme_start,
			dst_entry->vme_end - dst_entry->vme_start, src_entry->vme_start);
	}
	else {
		/*
		 *	Of course, wired down pages can't be set copy-on-write.
		 *	Cause wired pages to be copied into the new
		 *	map by simulating faults (the new pages are
		 *	pageable)
		 */
		vm_fault_copy_entry(dst_map, src_map, dst_entry, src_entry);
	}
}

/*
 *	vm_map_copy:
 *
 *	Perform a virtual memory copy from the source
 *	address map/range to the destination map/range.
 *
 *	If src_destroy or dst_alloc is requested,
 *	the source and destination regions should be
 *	disjoint, not only in the top-level map, but
 *	in the sharing maps as well.  [The best way
 *	to guarantee this is to use a new intermediate
 *	map to make copies.  This also reduces map
 *	fragmentation.]
 */
kern_return_t vm_map_copy(dst_map, src_map,
			  dst_addr, len, src_addr,
			  dst_alloc, src_destroy)
	vm_map_t	dst_map;
	vm_map_t	src_map;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
	boolean_t	dst_alloc;
	boolean_t	src_destroy;
{
	register
	vm_map_entry_t	src_entry;
	register
	vm_map_entry_t	dst_entry;
	vm_map_entry_t	tmp_entry;
	vm_offset_t	src_start;
	vm_offset_t	src_end;
	vm_offset_t	dst_start;
	vm_offset_t	dst_end;
	vm_offset_t	src_clip;
	vm_offset_t	dst_clip;
	kern_return_t	result;
	boolean_t	old_src_destroy;

	/*
	 *	XXX While we figure out why src_destroy screws up,
	 *	we'll do it by explicitly vm_map_delete'ing at the end.
	 */

	old_src_destroy = src_destroy;
	src_destroy = FALSE;

	/*
	 *	Compute start and end of region in both maps
	 */

	src_start = src_addr;
	src_end = src_start + len;
	dst_start = dst_addr;
	dst_end = dst_start + len;

	/*
	 *	Check that the region can exist in both source
	 *	and destination.
	 */

	if ((dst_end < dst_start) || (src_end < src_start))
		return(KERN_NO_SPACE);

	/*
	 *	Lock the maps in question -- we avoid deadlock
	 *	by ordering lock acquisition by map value
	 */

	if (src_map == dst_map) {
		vm_map_lock(src_map);
	}
	else if ((int) src_map < (int) dst_map) {
	 	vm_map_lock(src_map);
		vm_map_lock(dst_map);
	} else {
		vm_map_lock(dst_map);
	 	vm_map_lock(src_map);
	}

	result = KERN_SUCCESS;

	/*
	 *	Check protections... source must be completely readable and
	 *	destination must be completely writable.  [Note that if we're
	 *	allocating the destination region, we don't have to worry
	 *	about protection, but instead about whether the region
	 *	exists.]
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (!vm_map_check_protection(src_map, src_start, src_end,
					VM_PROT_READ)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}

		if (dst_alloc) {
			/* XXX Consider making this a vm_map_find instead */
			if ((result = vm_map_insert(dst_map, VM_OBJECT_NULL,
					(vm_offset_t) 0, dst_start, dst_end)) != KERN_SUCCESS)
				goto Return;
		}
		else if (!vm_map_check_protection(dst_map, dst_start, dst_end,
					VM_PROT_WRITE)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}
	}

	/*
	 *	Find the start entries and clip.
	 *
	 *	Note that checking protection asserts that the
	 *	lookup cannot fail.
	 *
	 *	Also note that we wait to do the second lookup
	 *	until we have done the first clip, as the clip
	 *	may affect which entry we get!
	 */

	(void) vm_map_lookup_entry(src_map, src_addr, &tmp_entry);
	src_entry = tmp_entry;
	vm_map_clip_start(src_map, src_entry, src_start);

	(void) vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry);
	dst_entry = tmp_entry;
	vm_map_clip_start(dst_map, dst_entry, dst_start);

	/*
	 *	If both source and destination entries are the same,
	 *	retry the first lookup, as it may have changed.
	 */

	if (src_entry == dst_entry) {
		(void) vm_map_lookup_entry(src_map, src_addr, &tmp_entry);
		src_entry = tmp_entry;
	}

	/*
	 *	If source and destination entries are still the same,
	 *	a null copy is being performed.
	 */

	if (src_entry == dst_entry)
		goto Return;

	/*
	 *	Go through entries until we get to the end of the
	 *	region.
	 */

	while (src_start < src_end) {
		/*
		 *	Clip the entries to the endpoint of the entire region.
		 */

		vm_map_clip_end(src_map, src_entry, src_end);
		vm_map_clip_end(dst_map, dst_entry, dst_end);

		/*
		 *	Clip each entry to the endpoint of the other entry.
		 */

		src_clip = src_entry->vme_start + (dst_entry->vme_end - dst_entry->vme_start);
		vm_map_clip_end(src_map, src_entry, src_clip);

		dst_clip = dst_entry->vme_start + (src_entry->vme_end - src_entry->vme_start);
		vm_map_clip_end(dst_map, dst_entry, dst_clip);

		/*
		 *	Both entries now match in size and relative endpoints.
		 *
		 *	If both entries refer to a VM object, we can
		 *	deal with them now.
		 */

		if (!src_entry->is_a_map && !dst_entry->is_a_map) {
			vm_map_copy_entry(src_map, dst_map, src_entry,
						dst_entry);
		}
		else {
			register vm_map_t	new_dst_map;
			vm_offset_t		new_dst_start;
			vm_size_t		new_size;
			vm_map_t		new_src_map;
			vm_offset_t		new_src_start;

			/*
			 *	We have to follow at least one sharing map.
			 */

			new_size = (dst_entry->vme_end - dst_entry->vme_start);

			if (src_entry->is_a_map) {
				new_src_map = src_entry->object.share_map;
				new_src_start = src_entry->offset;
			}
			else {
			 	new_src_map = src_map;
				new_src_start = src_entry->vme_start;
				lock_set_recursive(&src_map->lock);
			}

			if (dst_entry->is_a_map) {
			    	vm_offset_t	new_dst_end;

				new_dst_map = dst_entry->object.share_map;
				new_dst_start = dst_entry->offset;

				/*
				 *	Since the destination sharing entries
				 *	will be merely deallocated, we can
				 *	do that now, and replace the region
				 *	with a null object.  [This prevents
				 *	splitting the source map to match
				 *	the form of the destination map.]
				 *	Note that we can only do so if the
				 *	source and destination do not overlap.
				 */

				new_dst_end = new_dst_start + new_size;

				if (new_dst_map != new_src_map) {
					vm_map_lock(new_dst_map);
					(void) vm_map_delete(new_dst_map,
							new_dst_start,
							new_dst_end);
					(void) vm_map_insert(new_dst_map,
							VM_OBJECT_NULL,
							(vm_offset_t) 0,
							new_dst_start,
							new_dst_end);
					vm_map_unlock(new_dst_map);
				}
			}
			else {
			 	new_dst_map = dst_map;
				new_dst_start = dst_entry->vme_start;
				lock_set_recursive(&dst_map->lock);
			}

			/*
			 *	Recursively copy the sharing map.
			 */

			(void) vm_map_copy(new_dst_map, new_src_map,
				new_dst_start, new_size, new_src_start,
				FALSE, FALSE);

			if (dst_map == new_dst_map)
				lock_clear_recursive(&dst_map->lock);
			if (src_map == new_src_map)
				lock_clear_recursive(&src_map->lock);
		}

		/*
		 *	Update variables for next pass through the loop.
		 */

		src_start = src_entry->vme_end;
		src_entry = src_entry->vme_next;
		dst_start = dst_entry->vme_end;
		dst_entry = dst_entry->vme_next;

		/*
		 *	If the source is to be destroyed, here is the
		 *	place to do it.
		 */

		if (src_destroy && src_map->is_main_map &&
						dst_map->is_main_map)
			vm_map_entry_delete(src_map, src_entry->vme_prev);
	}

	/*
	 *	Update the physical maps as appropriate
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (src_destroy)
			pmap_remove(src_map->pmap, src_addr, src_addr + len);
	}

	/*
	 *	Unlock the maps
	 */

	Return: ;

	if (old_src_destroy)
		vm_map_delete(src_map, src_addr, src_addr + len);

	vm_map_unlock(src_map);
	if (src_map != dst_map)
		vm_map_unlock(dst_map);

	return(result);
}

/*
 *	vm_map_fork:
 *
 *	Create and return a new map based on the old
 *	map, according to the inheritance values on the
 *	regions in that map.
 *
 *	The source map must not be locked.
 */
vm_map_t vm_map_fork(old_map)
	vm_map_t	old_map;
{
	vm_map_t	new_map;
	vm_map_entry_t	old_entry;
	vm_map_entry_t	new_entry;
	pmap_t		new_pmap;

	vm_map_lock(old_map);

	new_pmap = pmap_create((vm_size_t) 0);
	new_map = vm_map_create(new_pmap,
			old_map->min_offset,
			old_map->max_offset,
			old_map->hdr.entries_pageable);

	old_entry = vm_map_first_entry(old_map);

	while (old_entry != vm_map_to_entry(old_map)) {
		if (old_entry->is_sub_map)
			panic("vm_map_fork: encountered a submap");

		switch (old_entry->inheritance) {
		case VM_INHERIT_NONE:
			break;

		case VM_INHERIT_SHARE:
			/*
			 *	If we don't already have a sharing map:
			 */

			if (!old_entry->is_a_map) {
			 	vm_map_t	new_share_map;
				vm_map_entry_t	new_share_entry;
				
				/*
				 *	Create a new sharing map
				 */
				 
				new_share_map = vm_map_create(PMAP_NULL,
							old_entry->vme_start,
							old_entry->vme_end,
							TRUE);
				new_share_map->is_main_map = FALSE;

				/*
				 *	Create the only sharing entry from the
				 *	old task map entry.
				 */

				new_share_entry =
					vm_map_entry_create(new_share_map);
				*new_share_entry = *old_entry;

				/*
				 *	Insert the entry into the new sharing
				 *	map
				 */

				vm_map_entry_link(new_share_map,
					vm_map_last_entry(new_share_map),
						new_share_entry);

				/*
				 *	Fix up the task map entry to refer
				 *	to the sharing map now.
				 */

				old_entry->is_a_map = TRUE;
				old_entry->object.share_map = new_share_map;
				old_entry->offset = old_entry->vme_start;
			}

			/*
			 *	Clone the entry, referencing the sharing map.
			 */

			new_entry = vm_map_entry_create(new_map);
			*new_entry = *old_entry;
			vm_map_reference(new_entry->object.share_map);

			/*
			 *	Insert the entry into the new map -- we
			 *	know we're inserting at the end of the new
			 *	map.
			 */

			vm_map_entry_link(new_map,
				vm_map_last_entry(new_map),
						new_entry);

			/*
			 *	Update the physical map
			 */

			pmap_copy(new_map->pmap, old_map->pmap,
				new_entry->vme_start,
				(old_entry->vme_end - old_entry->vme_start),
				old_entry->vme_start);
			break;

		case VM_INHERIT_COPY:
			/*
			 *	Clone the entry and link into the map.
			 */

			new_entry = vm_map_entry_create(new_map);
			*new_entry = *old_entry;
			new_entry->wired_count = 0;
			new_entry->object.vm_object = VM_OBJECT_NULL;
			new_entry->is_a_map = FALSE;
			vm_map_entry_link(new_map,
				vm_map_last_entry(new_map),
							new_entry);
			if (old_entry->is_a_map) {
				kern_return_t	check;

				check = vm_map_copy(new_map,
						old_entry->object.share_map,
						new_entry->vme_start,
						(vm_size_t)
							(new_entry->vme_end -
							new_entry->vme_start),
						old_entry->offset,
						FALSE, FALSE);
				if (check != KERN_SUCCESS)
					kprintf("vm_map_fork: copy in share_map region failed\n");
			}
			else {
				vm_map_copy_entry(old_map, new_map, old_entry,
						new_entry);
			}
			break;
		}
		old_entry = old_entry->vme_next;
	}

	new_map->size = old_map->size;
	vm_map_unlock(old_map);

	return(new_map);
}

/*
 *	vm_map_lookup:
 *
 *	Finds the VM object, offset, and
 *	protection for a given virtual address in the
 *	specified map, assuming a page fault of the
 *	type specified.
 *
#if	USE_VERSIONS
 *	Returns the (object, offset, protection) for
 *	this address, whether it is wired down, and whether
 *	this map has the only reference to the data in question.
 *	In order to later verify this lookup, a "version"
 *	is returned.
 *
 *	The map should not be locked; it will not be
 *	locked on exit.  In order to guarantee the
 *	existence of the returned object, it is returned
 *	locked.
#else	USE_VERSIONS
 *	Leaves the map in question locked for read; return
 *	values are guaranteed until a vm_map_lookup_done
 *	call is performed.  Note that the map argument
 *	is in/out; the returned map must be used in
 *	the call to vm_map_lookup_done.
 *
 *	A handle (out_entry) is returned for use in
 *	vm_map_lookup_done, to make that fast.
#endif	USE_VERSIONS
 *
 *	If a lookup is requested with "write protection"
 *	specified, the map may be changed to perform virtual
 *	copying operations, although the data referenced will
 *	remain the same.
 */
#if	USE_VERSIONS
kern_return_t vm_map_lookup(var_map, vaddr, fault_type, out_version,
				object, offset, out_prot, wired, single_use)
#else	USE_VERSIONS
kern_return_t vm_map_lookup(var_map, vaddr, fault_type, out_entry,
				object, offset, out_prot, wired, single_use)
#endif	USE_VERSIONS
	vm_map_t		*var_map;	/* IN/OUT */
	register vm_offset_t	vaddr;
	register vm_prot_t	fault_type;

#if	USE_VERSIONS
	vm_map_version_t	*out_version;	/* OUT */
#else	USE_VERSIONS
	vm_map_entry_t		*out_entry;	/* OUT */
#endif	USE_VERSIONS
	vm_object_t		*object;	/* OUT */
	vm_offset_t		*offset;	/* OUT */
	vm_prot_t		*out_prot;	/* OUT */
	boolean_t		*wired;		/* OUT */
	boolean_t		*single_use;	/* OUT */
{
	vm_map_t			share_map;
	vm_offset_t			share_offset;
	register vm_map_entry_t		entry;
	register vm_map_t		map = *var_map;
	register vm_prot_t		prot;
	register boolean_t		su;

	RetryLookup: ;

	/*
	 *	Lookup the faulting address.
	 */

	vm_map_lock_read(map);

#define	L_RETURN(why) \
		{ \
		vm_map_unlock_read(map); \
		return(why); \
		}

	/*
	 *	If the map has an interesting hint, try it before calling
	 *	full blown lookup routine.
	 */

	simple_lock(&map->hint_lock);
	entry = map->hint;
	simple_unlock(&map->hint_lock);

#if	!USE_VERSIONS
	*out_entry = entry;
#endif	!USE_VERSIONS

	if ((entry == vm_map_to_entry(map)) ||
	    (vaddr < entry->vme_start) || (vaddr >= entry->vme_end)) {
		vm_map_entry_t	tmp_entry;

		/*
		 *	Entry was either not a valid hint, or the vaddr
		 *	was not contained in the entry, so do a full lookup.
		 */
		if (!vm_map_lookup_entry(map, vaddr, &tmp_entry))
			L_RETURN(KERN_INVALID_ADDRESS);

		entry = tmp_entry;
#if	!USE_VERSIONS
		*out_entry = entry;
#endif	!USE_VERSIONS
	}

	/*
	 *	Handle submaps.
	 */

	if (entry->is_sub_map) {
		vm_map_t	old_map = map;

		*var_map = map = entry->object.sub_map;
		vm_map_unlock_read(old_map);
		goto RetryLookup;
	}
		
	/*
	 *	Check whether this task is allowed to have
	 *	this page.
	 */

	prot = entry->protection;
	if ((fault_type & (prot)) != fault_type)
		L_RETURN(KERN_PROTECTION_FAILURE);

	/*
	 *	If this page is not pageable, we have to get
	 *	it for all possible accesses.
	 */

	if (*wired = (entry->wired_count != 0))
		prot = fault_type = entry->protection;

	/*
	 *	If we don't already have a VM object, track
	 *	it down.
	 */

	if (su = !entry->is_a_map) {
	 	share_map = map;
		share_offset = vaddr;
	}
	else {
		vm_map_entry_t	share_entry;

		/*
		 *	Compute the sharing map, and offset into it.
		 */

		share_map = entry->object.share_map;
		share_offset = (vaddr - entry->vme_start) + entry->offset;

		/*
		 *	Look for the backing store object and offset
		 */

		vm_map_lock_read(share_map);

		if (!vm_map_lookup_entry(share_map, share_offset,
					&share_entry)) {
			vm_map_unlock_read(share_map);
			L_RETURN(KERN_INVALID_ADDRESS);
		}
		entry = share_entry;
	}

	/*
	 *	If the entry was copy-on-write, we either ...
	 */

	if (entry->needs_copy) {
	    	/*
		 *	If we want to write the page, we may as well
		 *	handle that now since we've got the sharing
		 *	map locked.
		 *
		 *	If we don't need to write the page, we just
		 *	demote the permissions allowed.
		 */

		if (fault_type & VM_PROT_WRITE) {
			/*
			 *	Make a new object, and place it in the
			 *	object chain.  Note that no new references
			 *	have appeared -- one just moved from the
			 *	share map to the new object.
			 */

			if (lock_read_to_write(&share_map->lock)) {
				if (share_map != map)
					vm_map_unlock_read(map);
				goto RetryLookup;
			}

			vm_object_shadow(
				&entry->object.vm_object,
				&entry->offset,
				(vm_size_t) (entry->vme_end -
							entry->vme_start));
				
			entry->needs_copy = FALSE;
			
			lock_write_to_read(&share_map->lock);
		}
		else {
			/*
			 *	We're attempting to read a copy-on-write
			 *	page -- don't allow writes.
			 */

			prot &= (~VM_PROT_WRITE);
		}
	}

	/*
	 *	Create an object if necessary.
	 */
	if (entry->object.vm_object == VM_OBJECT_NULL) {

		if (lock_read_to_write(&share_map->lock)) {
			if (share_map != map)
				vm_map_unlock_read(map);
			goto RetryLookup;
		}

		entry->object.vm_object = vm_object_allocate(
					(vm_size_t)(entry->vme_end -
							entry->vme_start));
		entry->offset = 0;
		lock_write_to_read(&share_map->lock);
	}

	/*
	 *	Return the object/offset from this entry.  If the entry
	 *	was copy-on-write or empty, it has been fixed up.
	 */

	*offset = (share_offset - entry->vme_start) + entry->offset;
	*object = entry->object.vm_object;

	/*
	 *	Return whether this is the only map sharing this data.
	 */

	if (!su) {
		simple_lock(&share_map->ref_lock);
		su = (share_map->ref_count == 1);
		simple_unlock(&share_map->ref_lock);
	}

	*out_prot = prot;
	*single_use = su;

#if	USE_VERSIONS
	/*
	 *	Lock the object to prevent it from disappearing
	 */

	vm_object_lock(*object);

	/*
	 *	Save the version numbers and unlock the map(s).
	 */

	if (share_map != map) {
		out_version->share_timestamp = share_map->timestamp;
		vm_map_unlock_read(share_map);
	}
	out_version->share_map = share_map;
	out_version->main_timestamp = map->timestamp;

	vm_map_unlock_read(map);
#endif	USE_VERSIONS

	return(KERN_SUCCESS);
	
#undef	L_RETURN
}

#if	USE_VERSIONS
/*
 *	vm_map_verify:
 *
 *	Verifies that the map in question has not changed
 *	since the given version.  If successful, the map
 *	will not change until vm_map_verify_done() is called.
 */
boolean_t	vm_map_verify(map, version)
	register
	vm_map_t	map;
	register
	vm_map_version_t *version;	/* REF */
{
	boolean_t	result;

	vm_map_lock_read(map);
	if (result = (map->timestamp == version->main_timestamp)) {
		register
		vm_map_t	share_map = version->share_map;

		if (share_map != map) {
			vm_map_lock_read(version->share_map);
			if (!(result = (share_map->timestamp == version->share_timestamp))) {
				vm_map_unlock_read(share_map);
			}
		}
	}

	if (!result)
		vm_map_unlock_read(map);

	return(result);
}

/*
 *	vm_map_verify_done:
 *
 *	Releases locks acquired by a vm_map_verify.
 */
void		vm_map_verify_done(map, version)
	register vm_map_t	map;
	vm_map_version_t	*version;	/* REF */
{
	if (version->share_map != map)
		vm_map_unlock_read(version->share_map);
	vm_map_unlock_read(map);
}

#else	USE_VERSIONS

/*
 *	vm_map_lookup_done:
 *
 *	Releases locks acquired by a vm_map_lookup
 *	(according to the handle returned by that lookup).
 */

void vm_map_lookup_done(map, entry)
	register vm_map_t	map;
	vm_map_entry_t		entry;
{
	/*
	 *	If this entry references a map, unlock it first.
	 */

	if (entry->is_a_map)
		vm_map_unlock_read(entry->object.share_map);

	/*
	 *	Unlock the main-level map
	 */

	vm_map_unlock_read(map);
}
#endif	USE_VERSIONS

/*
 *	Routine:	vm_map_machine_attribute
 *	Purpose:
 *		Provide machine-specific attributes to mappings,
 *		such as cachability etc. for machines that provide
 *		them.  NUMA architectures and machines with big/strange
 *		caches will use this.
 *	Note:
 *		Responsibilities for locking and checking are handled here,
 *		everything else in the pmap module. If any non-volatile
 *		information must be kept, the pmap module should handle
 *		it itself. [This assumes that attributes do not
 *		need to be inherited, which seems ok to me]
 */
kern_return_t vm_map_machine_attribute(map, address, size, attribute, value)
	vm_map_t	map;
	vm_offset_t	address;
	vm_size_t	size;
	vm_machine_attribute_t	attribute;
	vm_machine_attribute_val_t* value;		/* IN/OUT */
{
	kern_return_t	ret;

	if (address < vm_map_min(map) ||
	    (address + size) > vm_map_max(map))
		return KERN_INVALID_ARGUMENT;

	vm_map_lock(map);

	ret = pmap_attribute(map->pmap, address, size, attribute, value);

	vm_map_unlock(map);

	return ret;
}

#if	DEBUG
/*
 *	vm_map_print:	[ debug ]
 */
#if	1
/*
 * vm_map_print() is way out of date...
 */
void vm_map_print(vm_map_t map)
{

}
#else	1
void vm_map_print(map)
	register vm_map_t	map;
{
	register vm_map_entry_t	entry;
	extern int indent;

	iprintf("%s map 0x%x: pmap=0x%x,ref=%d,nentries=%d,version=%d\n",
		(map->is_main_map ? "Task" : "Share"),
 		(int) map, (int) (map->pmap), map->ref_count, map->nentries,
		map->timestamp);
	indent += 2;
	for (entry = map->header.next; entry != &map->header;
				entry = entry->next) {
		iprintf("map entry 0x%x: start=0x%x, end=0x%x, ",
			(int) entry, (int) entry->start, (int) entry->end);
		if (map->is_main_map) {
		     	static char *inheritance_name[4] =
				{ "share", "copy", "none", "donate_copy"};
			printf("prot=%x/%x/%s, ",
				entry->protection,
				entry->max_protection,
				inheritance_name[entry->inheritance]);
			if (entry->wired_count != 0)
				printf("wired, ");
		}

		if (entry->is_a_map) {
		 	printf("share=0x%x, offset=0x%x\n",
				(int) entry->object.share_map,
				(int) entry->offset);
			if ((entry->prev == &map->header) || (!entry->prev->is_a_map) ||
			    (entry->prev->object.share_map != entry->object.share_map)) {
				indent += 2;
				vm_map_print(entry->object.share_map);
				indent -= 2;
			}
				
		}
		else {
			printf("object=0x%x, offset=0x%x",
				(int) entry->object.vm_object,
				(int) entry->offset);
			if (entry->copy_on_write)
				printf(", copy (%s)", entry->needs_copy ? "needed" : "done");
			printf("\n");

			if ((entry->prev == &map->header) || (entry->prev->is_a_map) ||
			    (entry->prev->object.vm_object != entry->object.vm_object)) {
				indent += 2;
				vm_object_print(entry->object.vm_object);
				indent -= 2;
			}
		}
	}
	indent -= 2;
}
#endif	1
#endif	DEBUG

kern_return_t	vm_region(map, address, size,
				protection, max_protection,
				inheritance, is_shared,
				object_name, offset_in_object)
	vm_map_t	map;
	vm_offset_t	*address;		/* IN/OUT */
	vm_size_t	*size;			/* OUT */
	vm_prot_t	*protection;		/* OUT */
	vm_prot_t	*max_protection;	/* OUT */
	vm_inherit_t	*inheritance;		/* OUT */
	boolean_t	*is_shared;		/* OUT */
	port_t		*object_name;		/* OUT */
	vm_offset_t	*offset_in_object;	/* OUT */
{
	vm_map_entry_t	tmp_entry;
	register
	vm_map_entry_t	entry;
	register
	vm_offset_t	tmp_offset;
	vm_offset_t	start, inaddress = *address;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

again_after_submap:	
	start = *address;

	vm_map_lock_read(map);
	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
			vm_map_unlock_read(map);
		   	return(KERN_NO_SPACE);
		}
	} else {
		entry = tmp_entry;
	}

	start = entry->vme_start;
	*protection = entry->protection;
	*max_protection = entry->max_protection;
	*inheritance = entry->inheritance;
	*address = start;
	*size = (entry->vme_end - start);

	tmp_offset = entry->offset;
#define VM_OBJECT_NAME(object)		\
	vm_object_name((map == kernel_map) ? object : VM_OBJECT_NULL)

	if (entry->is_a_map) {
		register
		vm_map_t	share_map;
		vm_size_t	share_size;

		share_map = entry->object.share_map;

		vm_map_lock_read(share_map);
		(void) vm_map_lookup_entry(share_map, tmp_offset, &tmp_entry);

		if ((share_size = (tmp_entry->vme_end - tmp_offset)) < *size)
			*size = share_size;

		*object_name = VM_OBJECT_NAME(tmp_entry->object.vm_object);
		*offset_in_object = tmp_entry->offset +
					(tmp_offset - tmp_entry->vme_start);

		*is_shared = (share_map->ref_count != 1);
		vm_map_unlock_read(share_map);
	} else if (entry->is_sub_map) {
	    	vm_map_t	sub_map;
		vm_offset_t	oldstart = start;

		sub_map = entry->object.sub_map;
		vm_map_reference(sub_map);
		vm_map_unlock_read(map);

		vm_map_lock_read(sub_map);

		if (!vm_map_lookup_entry(sub_map, inaddress, &tmp_entry)) {
			if ((entry = tmp_entry->vme_next) ==
					vm_map_to_entry(sub_map)) {
				vm_map_unlock_read(sub_map);
				vm_map_deallocate(sub_map);
				inaddress = *address = oldstart + *size;
				goto again_after_submap;
			}
		}
		else
			entry = tmp_entry;

		start = entry->vme_start;
		*protection = entry->protection;
		*max_protection = entry->max_protection;
		*inheritance = entry->inheritance;
		*address = start;
		*size = (entry->vme_end - start);

		*is_shared = sub_map->hdr.entries_pageable;
		*object_name = VM_OBJECT_NAME(entry->object.vm_object);
		*offset_in_object = entry->offset;

		vm_map_unlock_read(sub_map);

		vm_map_deallocate(sub_map);

		return(KERN_SUCCESS);
	} else {
		*is_shared = FALSE;
		*object_name = VM_OBJECT_NAME(entry->object.vm_object);
		*offset_in_object = tmp_offset;
	}
#undef VM_OBJECT_NAME

	vm_map_unlock_read(map);

	return(KERN_SUCCESS);
}

#if	MACH_DEBUG
#include <kern/host.h>

kern_return_t	host_vm_region(
	host_t		host,
	vm_offset_t	*address,		/* IN/OUT */
	vm_size_t	*size,			/* OUT */
	vm_prot_t	*protection,		/* OUT */
	vm_prot_t	*max_protection,       	/* OUT */
	vm_inherit_t	*inheritance,		/* OUT */
	boolean_t	*is_pageable,		/* OUT */
	port_t		*object_name,		/* OUT */
	vm_offset_t	*offset_in_object	/* OUT */
)
{
	if (host == HOST_NULL)
		return(KERN_INVALID_ARGUMENT);

	return vm_region(kernel_map, address, size,
			 protection, max_protection,
			 inheritance, is_pageable,
			 object_name, offset_in_object);
}
#endif	/* MACH_DEBUG */

/*
 *	vm_move:
 *
 *	Move memory from source to destination map, possibly deallocating
 *	the source map reference to the memory.
 *
 *	Parameters are as follows:
 *
 *	src_map		Source address map
 *	src_addr	Address within source map
 *	dst_map		Destination address map
 *	num_bytes	Amount of data (in bytes) to copy/move
 *	src_dealloc	Should source be removed after copy?
 *
 *	Assumes the src and dst maps are not already locked.
 *
 *	If successful, returns destination address in dst_addr.
 */
kern_return_t vm_move(src_map,src_addr,dst_map,num_bytes,src_dealloc,dst_addr)
	vm_map_t		src_map;
	register vm_offset_t	src_addr;
	register vm_map_t	dst_map;
	vm_offset_t		num_bytes;
	boolean_t		src_dealloc;
	vm_offset_t		*dst_addr;
{
	register vm_offset_t	src_start;	/* Beginning of region */
	register vm_size_t	src_size;	/* Size of rounded region */
	vm_offset_t		dst_start;	/* destination address */
	register kern_return_t	result;

	if (num_bytes == 0) {
		*dst_addr = 0;
		return KERN_SUCCESS;
	}

	/*
	 *	Page-align the source region
	 */

	src_start = trunc_page(src_addr);
	src_size = round_page(src_addr + num_bytes) - src_start;

	/*
	 *	Allocate a place to put the copy
	 */

	dst_start = (vm_offset_t) 0;
	result = vm_allocate(dst_map, &dst_start, src_size, TRUE);
	if (result == KERN_SUCCESS) {
		/*
		 *	Perform the copy, asking for deallocation if desired
		 */
		result = vm_map_copy(dst_map, src_map, dst_start, src_size,
				     src_start, FALSE, src_dealloc);

		/*
		 *	Return the destination address corresponding to
		 *	the source address given (rather than the front
		 *	of the newly-allocated page).
		 */

		if (result == KERN_SUCCESS)
			*dst_addr = dst_start + (src_addr - src_start);
		else
			(void) vm_deallocate(dst_map, dst_start, src_size);
	}

	return(result);
}

pmap_t
vm_map_pmap_EXTERNAL(
	vm_map_t		map
)
{
	return (vm_map_pmap(map));
}
