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
 * Copyright (c) 1993,1991,1990,1989,1988,1987 Carnegie Mellon University
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
 * Copyright (c) 1995, 1997 Apple Computer, Inc.
 *
 * HISTORY
 *
 * 23 June 1995 ? at NeXT
 *	Pulled over from CMU (MK83), added local
 * 	mods: freespace suballocator and sorted
 *	zone free lists to reduce fragmentation.
 */
/*
 *	File:	kern/zalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Zone-based memory allocator.  A zone is a collection of fixed size
 *	data blocks for which quick allocation/deallocation is possible.
 */
 
#import <mach/features.h>

#include <kern/macro_help.h>
#include <kern/sched.h>
#include <kern/time_out.h>
#include <kern/zalloc.h>
#include <mach/vm_param.h>
#include <vm/vm_kern.h>

#if	MACH_DEBUG
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach_debug/zone_info.h>
#include <kern/host.h>
#include <vm/vm_map.h>
#include <vm/vm_user.h>
#include <vm/vm_kern.h>
#endif

#if DIAGNOSTIC
vm_offset_t	zlowest = (vm_offset_t)(-1);
vm_offset_t	zhighest = (vm_offset_t)0;
#define WATERMARK_ZONE(zone, element)					\
MACRO_BEGIN								\
	if ((element) < (zone)->lowest)					\
		(zone)->lowest = (element);				\
		if ((element) < zlowest)				\
			zlowest = (element);				\
	if ((element) > (zone)->highest)				\
		(zone)->highest = (element);				\
		if ((element) > zhighest)				\
			zhighest = (element);				\
MACRO_END
#else
#define WATERMARK_ZONE(zone, element)
#endif

/*
 * 1 - fill freed memory with 0xdeadface
 * 2 - check_zone at zfree
 * 4 - check_zone at zget
 * 8 - check_zone at zalloc
 * 10 - duplicate the free list pointer chain
 */
unsigned zone_check = 0;

void *memset __P((void *, int , size_t));

#define ADD_TO_ZONE(zone, element)					\
MACRO_BEGIN								\
	vm_offset_t	cur, *last;					\
		if (zone_check & 1)					\
                	(void) memset((void *)(element), 0xdeadface,	\
                		      (size_t) (zone)->elem_size);	\
		WATERMARK_ZONE((zone), (element));			\
		if (		(zone)->last_insert != 0 	&&	\
				(element) > (zone)->last_insert	)	\
			(vm_offset_t)last = (zone)->last_insert;	\
		else							\
			last = &(zone)->free_elements;			\
		while (		(cur = *last) != 0	&&		\
				(element) > cur		)		\
			(vm_offset_t)last = cur;			\
		*(vm_offset_t *)(element) = cur;			\
		if (zone_check & 0x10)					\
			*((vm_offset_t *)(element) + 1) = cur;		\
		*last = (element);					\
		(zone)->last_insert = (element);			\
		(zone)->count--;					\
MACRO_END

#define REMOVE_FROM_ZONE(zone, ret, type)				\
MACRO_BEGIN								\
	(ret) = (type) (zone)->free_elements;				\
	if ((ret) != (type) 0) {					\
		(zone)->count++;					\
		(zone)->free_elements = *(vm_offset_t *)(ret);		\
		if ((zone)->last_insert == (vm_offset_t)(ret))		\
			(zone)->last_insert = 0;			\
	}								\
MACRO_END

zone_t		zone_zone;	/* this is the zone containing other zones */

boolean_t	zone_ignore_overflow = TRUE;

vm_map_t	zone_map = VM_MAP_NULL;
vm_size_t	zone_map_size;
vm_size_t	zone_map_size_min = 12 * 1024 * 1024;
vm_size_t	zone_map_size_max = 128 * 1024 * 1024;
vm_offset_t	zone_min, zone_max;

/*
 *	The VM system gives us an initial chunk of memory.
 *	It has to be big enough to allocate the zone_zone
 *	and some initial kernel data structures, like kernel maps.
 *	It is advantageous to make it bigger than really necessary,
 *	because this memory is more efficient than normal kernel
 *	virtual memory.  (It doesn't have vm_page structures backing it
 *	and it may have other machine-dependent advantages.)
 *	So for best performance, zdata_size should approximate
 *	the amount of memory you expect the zone system to consume.
 */

vm_offset_t	zdata;
vm_size_t	zdata_size = 512 * 1024;

#define lock_zone(zone)					\
MACRO_BEGIN						\
	if ((zone)->pageable) { 			\
		lock_write(&(zone)->complex_lock);	\
	} else {					\
		spl_t s = splhigh();			\
		simple_lock(&(zone)->lock);		\
		(zone)->lock_ipl = s;			\
	}						\
MACRO_END

#define unlock_zone(zone)				\
MACRO_BEGIN						\
	if ((zone)->pageable) { 			\
		lock_done(&(zone)->complex_lock);	\
	} else {					\
		spl_t s = (zone)->lock_ipl;		\
		simple_unlock(&(zone)->lock);		\
		splx(s);				\
	}						\
MACRO_END

#define lock_zone_init(zone)				\
MACRO_BEGIN						\
	if ((zone)->pageable) { 			\
		lock_init(&(zone)->complex_lock, TRUE);	\
	} else {					\
		simple_lock_init(&(zone)->lock);	\
	}						\
MACRO_END

vm_offset_t zget_space(
	struct zone_free_space	*free_space,
	vm_size_t		size,
	boolean_t		canblock);

decl_simple_lock_data(,zget_space_lock)

/*
 * A free list entry, which is created
 * at the front of an available region
 * of memory.  The 'pred' field points
 * at the 'next' pointer of the previous
 * entry (or the head pointer).
 */
struct zone_free_space_entry {
	struct zone_free_space_entry	*next;
	vm_size_t			length;
	struct zone_free_space_entry	**pred;
	int				_pad[1];
};
#define ZONE_MIN_ALLOC		16	/*
					 * *Must* be:
					 *  a power of two
					 *	and
					 *  at least sizeof (struct
					 * zone_free_space_entry)
					 */

/*
 * An entry in the free list hint table,
 * which allows for quickly locating a
 * suitably sized region needed to satisfy
 * a request.
 */
struct zone_free_space_hint {
	struct zone_free_space_entry	*entry;
	int				_pad[3];
};

/*
 * Structure used to manage memory which
 * has been obtained from the system, but
 * which is not currently owned by any zone.
 * The main two parameters are 'alloc_unit'
 * and 'alloc_max'.  The former specifies the
 * allocation granularity.  Allocation requests
 * will be rounded up to this size, which must
 * be a power of two.  The size of the largest
 * (suggested) allocation request is specified
 * by 'alloc_max'.  The free list is headed by
 * 'entries', and is a doubly linked list of
 * free regions, sorted by ascending address.
 * There also is a table of hints, indexed by
 * region size, which is used to speed up the
 * allocations when the free list becomes
 * especially large.  It contains one entry
 * per size value between alloc_unit and alloc_max.
 */ 
struct zone_free_space {
	vm_size_t			alloc_unit;
	vm_size_t			alloc_max;
	struct zone_free_space_entry	*entries;
	integer_t			num_entries;
	integer_t			hash_shift;
	struct zone_free_space_hint	*hints;
	integer_t			num_hints;
};

struct zone_free_space		*zone_free_space[8];
integer_t			zone_free_space_count;

/*
 * There is a default allocation space (zone_free_space[0]),
 * which is special.  It is statically allocated, contains
 * a single hint, and space is not reclaimed from it.  It
 * is used to allocate space for non collectable zones,
 * as well as resources needed for bootstrapping purposes.
 */
struct zone_free_space_hint	_zone_default_space_hint;
struct zone_free_space		_zone_default_space;
#define	zone_default_space	(&_zone_default_space)

static void zone_free_space_select(zone_t	zone);

#define zone_collectable(z)	((z)->free_space != 0 && \
					(z)->free_space != zone_default_space)

/*
 * Compute the hash address for an element
 * of a particular size.  Oversized requests
 * all land in the last bucket.
 */
static __inline__
integer_t
zone_free_space_hash(
	struct zone_free_space		*freespace,
	vm_size_t			size
)
{
	integer_t		hash;
	
	if ((hash = size >> freespace->hash_shift) >
					freespace->num_hints)
		return (freespace->num_hints);
	else
		return (hash);
}
#define	zone_free_space_hint_from_hash(freespace, hash)	\
					((freespace)->hints + (hash) - 1)

/*
 * Return the hint entry for an element of
 * a particular size.
 */
static __inline__
struct zone_free_space_hint *
zone_free_space_hint(
	struct zone_free_space		*freespace,
	vm_size_t			size
)
{
	return zone_free_space_hint_from_hash(freespace,
					zone_free_space_hash(freespace, size));
}

/*
 * Conditionally insert the entry into the
 * hint table such that the hint indicates
 * the lowest addressed entry of the particular
 * size.
 */
static __inline__
void
zone_free_space_hint_insert(
	struct zone_free_space		*freespace,
	struct zone_free_space_entry	*entry
)
{
	struct zone_free_space_hint	*hint;

	hint = zone_free_space_hint(freespace, entry->length);
	if (hint->entry == 0 || entry < hint->entry)
		hint->entry = entry;
}

/*
 * Delete the entry from the hint table (if
 * present), and locate the next entry of the
 * particular size by searching forwards in
 * the free list.  For the 'alloc_max' hint,
 * we choose the next entry of size >= alloc_max.
 */
static
void
zone_free_space_hint_delete(
	struct zone_free_space		*freespace,
	struct zone_free_space_entry	*entry
)
{
	integer_t			hash;
	struct zone_free_space_hint	*hint;

	hash = zone_free_space_hash(freespace, entry->length);
	hint = zone_free_space_hint_from_hash(freespace, hash);

	if (entry == hint->entry) {
		if (hash < freespace->num_hints) {
			struct zone_free_space_entry
					*cur, **last = &entry->next;

			while ((cur = *last) != 0 &&
					cur->length != entry->length)
				last = &cur->next;

			hint->entry = cur;
		}
		else {
			struct zone_free_space_entry
					*cur, **last = &entry->next;

			while ((cur = *last) != 0 &&
					cur->length < freespace->alloc_max)
				last = &cur->next;

			hint->entry = cur;
		}
	}
}

/*
 * Update the hint for an entry which has been
 * expanded from the front.  In this case, the
 * header has moved (and is no longer valid),
 * so we need both the length and the address
 * of the old entry.  This could be accomplished
 * with separate delete/insert operations, but
 * this is much more efficient for several
 * important cases.
 */
static
void
zone_free_space_hint_prepend(
	struct zone_free_space		*freespace,
	struct zone_free_space_entry	*entry,
	vm_size_t			old_length,
	void				*old_entry
)
{
	integer_t			old_hash, new_hash;
	struct zone_free_space_hint	*hint;

	/*
	 * Calculate both the new and the old hash
	 * addresses, as well as the old hint structure,
	 * which we are likely to need.
	 */
	new_hash = zone_free_space_hash(freespace, entry->length);
	old_hash = zone_free_space_hash(freespace, old_length);
	hint = zone_free_space_hint_from_hash(freespace, old_hash);

	/*
	 * Hash address has changed.
	 */
	if (old_hash != new_hash) {
		/*
		 * Old hint was valid, update it.
		 */
		if (old_entry == hint->entry) {
			if (old_hash < freespace->num_hints) {
				struct zone_free_space_entry
						*cur, **last = &entry->next;

				while ((cur = *last) != 0 &&
						cur->length != old_length)
					last = &cur->next;

				hint->entry = cur;
			}
			else {
				struct zone_free_space_entry
						*cur, **last = &entry->next;

				while ((cur = *last) != 0 &&
						cur->length < 
							freespace->alloc_max)
					last = &cur->next;

				hint->entry = cur;
			}
		}

		/*
		 * Insert a hint for the new entry.
		 */
		hint = zone_free_space_hint_from_hash(freespace, new_hash);
		if (hint->entry == 0 || entry < hint->entry)
			hint->entry = entry;
	}
	/*
	 * If the hash address has not
	 * changed, always replace a valid
	 * old hint with the new one.
	 */
	else if (old_entry == hint->entry)
		hint->entry = entry;
}

/*
 * Update the hint for an entry which has been
 * expanded on the end.  This could be accomplished
 * with separate delete/insert operations, but
 * this is much more efficient for several
 * important cases.
 */
static
void
zone_free_space_hint_append(
	struct zone_free_space		*freespace,
	struct zone_free_space_entry	*entry,
	vm_size_t			old_length
)
{
	integer_t			old_hash, new_hash;
	struct zone_free_space_hint	*hint;

	/*
	 * Calculate both the new and the old hash
	 * addresses.
	 */
	new_hash = zone_free_space_hash(freespace, entry->length);
	old_hash = zone_free_space_hash(freespace, old_length);

	if (old_hash != new_hash) {
		hint = zone_free_space_hint_from_hash(freespace, old_hash);
		/*
		 * Old hint was valid, update it.
		 */
		if (entry == hint->entry) {
			if (old_hash < freespace->num_hints) {
				struct zone_free_space_entry
						*cur, **last = &entry->next;

				while ((cur = *last) != 0 &&
						cur->length != old_length)
					last = &cur->next;

				hint->entry = cur;
			}
			else {
				struct zone_free_space_entry
						*cur, **last = &entry->next;

				while ((cur = *last) != 0 &&
						cur->length < 
							freespace->alloc_max)
					last = &cur->next;

				hint->entry = cur;
			}
		}

		/*
		 * Insert a hint for the new entry.
		 */
		hint = zone_free_space_hint_from_hash(freespace, new_hash);
		if (hint->entry == 0 || entry < hint->entry)
			hint->entry = entry;
	}
}

/*
 * Locate a suitably sized entry, using the
 * allocation hints.  The entry is removed
 * from the hint table, which is also updated
 * accordingly.  The entry's position in the
 * free list is not affected.
 */
static
struct zone_free_space_entry *
zone_free_space_lookup(
	struct zone_free_space		*freespace,
	vm_size_t			size
)
{
	integer_t			hash;
	struct zone_free_space_hint	*hint;
	struct zone_free_space_entry	*entry;

	/*
	 * Perform a couple of quick checks:
	 * 1) an empty free list or
	 * 2) a suitable first entry
	 */
	if ((entry = freespace->entries) == 0)
		return (entry);
	else if (entry->length >= size) {
		zone_free_space_hint_delete(freespace, entry);
		
		return (entry);
	}

	/*
	 * Calculate the hash address and hint
	 * entry corresponding to the request
	 * size.  We start there and move on to
	 * the larger hints if needed.
	 */
	hash = zone_free_space_hash(freespace, size); 
	hint = zone_free_space_hint_from_hash(freespace, hash);

	/*
	 * This loop checks the exact sized hints
	 * (hash to [num_hints - 1]).  If we encounter
	 * a valid hint, we return that entry, after
	 * first searching ahead in the free list to
	 * replace it.  If we come to the end of
	 * the free list while searching, we end up
	 * invalidating this hint.
	 */
	while (hash < freespace->num_hints) {
		if ((entry = hint->entry) != 0) {
			struct zone_free_space_entry
					*cur, **last = &entry->next;

			while ((cur = *last) != 0 &&
					cur->length != entry->length)
				last = &cur->next;

			hint->entry = cur;
			
			return (entry);
		}
		
		hash++; hint++;
	}

	/*
	 * Now check the last bucket.
	 */
	if ((entry = hint->entry) != 0) {
		struct zone_free_space_entry
				*cur, **last = &entry->next;

		/*
		 * The last bucket contains the lowest
		 * addressed entry >= alloc_max, which
		 * isn't necessarily big enough for this
		 * request.  If it isn't big enough, then
		 * search ahead in the free list for a
		 * suitable entry to return.  In this case
		 * we also leave the current hint alone
		 * since we aren't going to use it.
		 */
		if (entry->length < size) {
			while ((cur = *last) != 0 &&
					cur->length < size)
				last = &cur->next;
				
			return (cur);
		}

		while ((cur = *last) != 0 &&
				cur->length < freespace->alloc_max)
			last = &cur->next;

		hint->entry = cur;
	}
	
	return (entry);
}

/*
 *	Protects first_zone, last_zone, num_zones,
 *	and the next_zone field of zones.
 */
decl_simple_lock_data(,all_zones_lock)
zone_t			first_zone;
zone_t			*last_zone;
int			num_zones;

/*
 *	zinit initializes a new zone.  The zone data structures themselves
 *	are stored in a zone, which is initially a static structure that
 *	is initialized by zone_init.
 */
zone_t zinit(size, max, alloc, pageable, name)
	vm_size_t	size;		/* the size of an element */
	vm_size_t	max;		/* maximum memory to use */
	vm_size_t	alloc;		/* allocation size */
	boolean_t	pageable;	/* is this zone pageable? */
	char		*name;		/* a name for the zone */
{
	register zone_t		z;

	if (zone_zone == ZONE_NULL)
		z = (zone_t) zget_space(
				zone_default_space,
				sizeof(struct zone),
				FALSE);
	else
		z = (zone_t) zalloc(zone_zone);
	if (z == ZONE_NULL)
		panic("zinit");

 	if (alloc == 0)
		alloc = PAGE_SIZE;

	if (size == 0)
		size = sizeof(z->free_elements);

	size = ((size + (ZONE_MIN_ALLOC - 1)) & ~(ZONE_MIN_ALLOC - 1));

	/*
	 *	Round off all the parameters appropriately.
	 */

	if ((max = round_page(max)) < (alloc = round_page(alloc)))
		max = alloc;

	z->last_insert = z->free_elements = 0;
	z->cur_size = 0;
	z->max_size = max;
	z->elem_size = size;

#if DIAGNOSTIC
	z->lowest = (vm_offset_t)(-1);
	z->highest = (vm_offset_t)0;
#endif
	z->alloc_size = alloc;
	z->pageable = pageable;
	z->zone_name = name;
	z->count = 0;
	z->doing_alloc = FALSE;
	z->exhaustible = z->sleepable = FALSE;
	z->expandable  = TRUE;
	lock_zone_init(z);
	zone_free_space_select(z);

	/*
	 *	Add the zone to the all-zones list.
	 */

	z->next_zone = ZONE_NULL;
	simple_lock(simple_lock_addr(all_zones_lock));
	*last_zone = z;
	last_zone = &z->next_zone;
	num_zones++;
	simple_unlock(simple_lock_addr(all_zones_lock));

	return(z);
}

/*
 *	Cram the given memory into the specified zone.
 */
void zcram(zone, newmem, size)
	register zone_t		zone;
	vm_offset_t		newmem;
	vm_size_t		size;
{
	register vm_size_t	elem_size;

	if (newmem == (vm_offset_t) 0) {
		panic("zcram - memory at zero");
	}
	elem_size = zone->elem_size;

	lock_zone(zone);
	while (size >= elem_size) {
		ADD_TO_ZONE(zone, newmem);
		zone->count++;	/* compensate for ADD_TO_ZONE */
		size -= elem_size;
		newmem += elem_size;
		zone->cur_size += elem_size;
	}
	unlock_zone(zone);
}

/*
 * Allocate (return) a new zone element from a new memory
 * region.  Remaining memory from the new region is added
 * to the specified freelist.  Before generating the new 
 * element, an attempt is made to combine the new region
 * with an existing entry.  New elements are always taken
 * from the front of a free region.
 */
vm_offset_t zone_free_space_add(freespace, size, new_space, space_to_add)
	struct zone_free_space *freespace;
	vm_size_t size;
	vm_offset_t new_space;
	vm_size_t space_to_add;
{
	struct zone_free_space_entry	*cur, **last;

	if (freespace == 0)
		freespace = zone_default_space;

	/*
	 * Search the free list for an existing
	 * abutting entry.
	 */
	last = &freespace->entries;
	while ((cur = *last) != 0 &&
			(vm_offset_t)cur < new_space &&
			((vm_offset_t)cur + cur->length) != new_space)
		last = &cur->next;
			
	if (cur == 0 || ((vm_offset_t)cur + cur->length) < new_space) {
		/*
		 * No entry was found to combine with.
		 * Take the new element from the front
		 * of the new region, and insert the
		 * remainder as a new entry.
		 */
		if ((space_to_add - size) >= ZONE_MIN_ALLOC) {
			/*
			 * If we are not at the end of
			 * the free list, then insert the
			 * new entry after the current entry.
			 */
			if (cur != 0)
				last = &cur->next;
			(vm_offset_t)cur = new_space + size;
			cur->length = space_to_add - size;
			if (cur->next = *last)
				cur->next->pred = &cur->next;
			cur->pred = last;
			*last = cur;
			freespace->num_entries++;

			/*
			 * Insert this entry into the hint
			 * table.
			 */
			zone_free_space_hint_insert(freespace, cur);
		}
	}
	else
	if (((vm_offset_t)cur + cur->length) == new_space) {
		struct zone_free_space_entry	*new;

		/*
		 * Delete the existing entry from the
		 * hint table.  It is very likely that
		 * the hash address will be changing
		 * anyways.
		 */
		zone_free_space_hint_delete(freespace, cur);

		/*
		 * Combine the new region with an existing
		 * entry, and take the new element from the
		 * front of the aggregate region.  Create a
		 * new entry for the remainder, and insert it
		 * in place of the existing entry.
		 */
		new_space = (vm_offset_t)cur;
		(vm_offset_t)new = (vm_offset_t)cur + size;
		new->length = cur->length + space_to_add - size;
		if (new->next = cur->next)
			new->next->pred = &new->next;
		new->pred = last;
		*last = new;

		/*
		 * Insert this entry into the hint
		 * table.
		 */
		zone_free_space_hint_insert(freespace, new);
	}
	
	return (new_space);
}

#if	0
/* NOT USED and OUTDATED */
/*
 * Ensure that no portion of the specified region
 * is represented on the free list (either wholly
 * or in part).
 */
void zone_free_space_remove(freespace, address, size)
	struct zone_free_space *freespace;
	vm_offset_t address;
	vm_size_t size;
{
	struct zone_free_space_entry
			*cur, **last;

	if (freespace == 0)
		freespace = zone_default_space;

	/*
	 * Search the free list, looking for
	 * the first suitable entry.
	 */
	last = &freespace->entries;
	while ((cur = *last) != 0) {
		if ((vm_offset_t)cur >= address) {
			/*
			 * Entry is above (and does not
			 * overlap) the region. (skip)
			 */
			if ((vm_offset_t)cur >= (address + size))
				last = &cur->next;
			else {
				/*
				 * Entry is entirely contained
				 * within the region. (remove)
				 */
				if (((vm_offset_t)cur + cur->length) <=
							(address + size)) {
					*last = cur->next;
					freespace->num_entries--;
				}
				else {
					struct zone_free_space_entry	*new;

					/*
					 * Entry overlaps the end
					 * of the region. (clip)
					 */
					(vm_offset_t)new = address + size;
					new->length = cur->length - 
							((vm_offset_t)new -
							 (vm_offset_t)cur);
					new->next = cur->next;
					*last = new;
				}

				break;
			}
		}
		else {
			/*
			 * Entry is entirely below the
			 * region. (skip)
			 */
			if (((vm_offset_t)cur + cur->length) <= address)
				last = &cur->next;
			else {
				/*
				 * Entry overlaps the front
				 * of the region. (clip)
				 */
				cur->length = address - (vm_offset_t)cur;
				break;
			}
		}
	}
}
#endif

/*
 * Return the elements on the zone free list back
 * to the free space pool, coalescing with existing
 * space where possible.  Caller must hold the
 * zget_space_lock, as well as the lock on the zone.
 */
void zone_collect(zone)
	struct zone	*zone;
{
	struct zone_free_space_entry	*cur, **last, *new;
	vm_offset_t			*free, *elem, next;
	vm_size_t			elem_size = zone->elem_size;
	struct zone_free_space		*freespace = zone->free_space;

	if (!zone_collectable(zone))
		return;

	last = &freespace->entries;

	free = &zone->free_elements;
	while (((vm_offset_t)elem = *free) != 0) {
		zone->cur_size -= elem_size;
		next = *elem;
		
		while ((cur = *last) != 0 &&
				((vm_offset_t)cur +
					cur->length) < (vm_offset_t)elem)
			last = &cur->next;
		/*
		 * Either at end of (maybe empty)
		 * list, or new entry before current
		 * entry.
		 */
		if (cur == 0 || ((vm_offset_t)elem + elem_size) <
							(vm_offset_t)cur) {
			(vm_offset_t)new = (vm_offset_t)elem;
			new->length = elem_size;
			if (new->next = cur)
				cur->pred = &new->next;
			new->pred = last;
			*last = new;
			freespace->num_entries++;

			/*
			 * Insert this entry into the hint
			 * table.
			 */
			zone_free_space_hint_insert(freespace, new);
		}
		else
		/*
		 * Prepend element to current entry
		 */
		if (((vm_offset_t)elem + elem_size) == (vm_offset_t)cur) {
			vm_size_t		old_length = cur->length;

			(vm_offset_t)new = (vm_offset_t)elem;
			new->length = cur->length + elem_size;
			if (new->next = cur->next)
				new->next->pred = &new->next;
			new->pred = last;
			*last = new;

			/*
			 * Update the hint table.
			 */
			zone_free_space_hint_prepend(freespace,
							new, old_length, cur);
		}
		else
		/*
		 * Append element to current entry.
		 */
		if (((vm_offset_t)cur + cur->length) == (vm_offset_t)elem) {
			vm_size_t		old_length = cur->length;

			cur->length += elem_size;
			/*
			 * Coalesce the current entry with the
			 * following one if we are filling the
			 * gap between them.
			 */
			if (((vm_offset_t)cur + cur->length) ==
						(vm_offset_t)cur->next) {

				/*
				 * Delete the obsolete entry
				 * from the hint table.
				 */
				zone_free_space_hint_delete(
							freespace, cur->next);

				cur->length += cur->next->length;
				if (cur->next = cur->next->next)
					cur->next->pred = &cur->next;
				freespace->num_entries--;
			}

			/*
			 * Update the hint table.
			 */
			zone_free_space_hint_append(freespace,
							cur, old_length);
		}
		else
		/* WTF?? */;
		
		*free = next;
	}
	
	zone->last_insert = 0;
}

/*
 * Scan the collectable free space free lists
 * and gather up pages which can be returned to
 * the system.  Caller must hold the zget_space_lock.
 */
struct zone_free_space_entry *
zone_free_space_reclaim(void)
{
	struct zone_free_space_entry	**last, *cur, *pages = 0;
	struct zone_free_space		**f = &zone_free_space[0];
	int				i;

	for (i = 1; i < zone_free_space_count; i++) {
		last = &(*++f)->entries;
		while ((cur = *last) != 0) {
			if (cur->length >= PAGE_SIZE) {
			    vm_offset_t		start, end;
		    
			    start = round_page((vm_offset_t)cur);
			    end = trunc_page(
				    (vm_offset_t)cur + cur->length);
			    if (start < end &&
					start >= zone_min &&
					end <= zone_max) {
				struct zone_free_space_entry	*tmp;

				/*
				 * Delete this entry from the hint
				 * table.  Space which is leftover
				 * after clipping will be added back
				 * normally after the entries are
				 * created.
				 */ 
				zone_free_space_hint_delete(*f, cur);

				/*
				 * If the region does not end on a
				 * page boundary, create a new
				 * trailing entry.
				 */
				if (((vm_offset_t)cur + cur->length) != end) {
					(vm_offset_t)tmp = end;
					tmp->length = (vm_offset_t)cur +
						    	cur->length - end;
					if (tmp->next = cur->next)
						tmp->next->pred = &tmp->next;

					/*
					 * Now, if the region does begin
					 * on a page boundary, just remove
					 * the current entry.
					 */
					if ((vm_offset_t)cur == start) {
						*last = tmp;
						tmp->pred = last;
					}
					/*
					 * Otherwise, adjust the current
					 * entry, and link it to the new
					 * one.  Do not forget to account
					 * for the new entry.
					 */
					else {
						cur->length = start -
					    		(vm_offset_t)cur;
					    	cur->next = tmp;
					    	tmp->pred = &cur->next;
					    	(*f)->num_entries++;

						/*
						 * Reinsert the leading
						 * entry into the hint
						 * table.
						 */
					    	zone_free_space_hint_insert(
								*f, cur);
					}

					/*
					 * Insert the new trailing entry
					 * into the hint table.
					 */
					zone_free_space_hint_insert(*f, tmp);
				}
				/*
				 * If the region does not begin on a
				 * page boundary (but the end does),
				 * adjust the current entry.
				 */
				else if ((vm_offset_t)cur != start) {
					cur->length = start - (vm_offset_t)cur;

					/*
					 * Reinsert the entry into
					 * the hint table.
					 */
					zone_free_space_hint_insert(*f, cur);
				}
				/*
				 * If no clipping is required, just
				 * remove the current entry.
				 */
				else {
					if (*last = cur->next)
						cur->next->pred = last;
					(*f)->num_entries--;
				}

				/*
				 * Add the new page aligned region
				 * to the list of pages to be freed.
				 * Continue on with the next entry.
				 */
				(vm_offset_t)tmp = start;
				tmp->length = end - start;
				tmp->next = pages;
				pages = tmp;
				continue;
			    }
			}

			/*
			 * Skip this entry.
			 */
			last = &cur->next;
		}
	}

	return (pages);
}

/*
 * Contiguous space allocator for non-paged zones. Allocates "size" amount
 * of memory from zone_map.
 */

vm_offset_t zget_space(freespace, size, canblock)
	struct zone_free_space *freespace;
	vm_size_t size;
	boolean_t canblock;
{
	vm_offset_t	new_space = 0;
	vm_offset_t	result;
	vm_size_t	space_to_add;
	struct zone_free_space_entry
			*cur, **last;

	if (freespace == 0)
		freespace = zone_default_space;

	/*
	 * Round up all requests (even 0) to
	 * our minimum allocation unit.
	 */
	if (size > ZONE_MIN_ALLOC)
		size = ((size + (ZONE_MIN_ALLOC - 1)) & ~(ZONE_MIN_ALLOC - 1));
	else
		size = ZONE_MIN_ALLOC;

	simple_lock(simple_lock_addr(zget_space_lock));
	for (;;) {
		if ((cur = zone_free_space_lookup(freespace, size)) != 0) {
			last = cur->pred;

			/*
			 * The entry which was found has been
			 * removed from the hint table, but
			 * remains in the free list.  Trim off
			 * the space to be returned.
			 */
			if ((cur->length - size) < ZONE_MIN_ALLOC) {
				/*
				 * This is a real lose if it
				 * happens in a collectable
				 * space, since the memory
				 * will be lost, making it
				 * impossible to reclaim the
				 * page later.
				 */
				if (*last = cur->next)
					cur->next->pred = last;
				freespace->num_entries--;
			}
			else {
				struct zone_free_space_entry	*new;

				/*
				 * Create a new entry for the
				 * remaining space and position
				 * on the free list.
				 */
				(vm_offset_t)new = (vm_offset_t)cur + size;
				new->length = cur->length - size;
				if (new->next = cur->next)
					new->next->pred = &new->next;
				new->pred = last;
				*last = new;

				/*
				 * After trimming the entry, reinsert
				 * it back into the hint table.
				 */
				zone_free_space_hint_insert(freespace, new);
			}
			result = (vm_offset_t)cur;
			break;
		}
		else if (new_space == 0) {
			/*
			 *	Add at least one page to allocation area.
			 */
	
			space_to_add = round_page(size);

			if (zdata_size >= space_to_add) {
				zdata_size -= space_to_add;
				result = zone_free_space_add(
							freespace,
							size,
							zdata + zdata_size,
							space_to_add);
				break;
			}

			/*
			 *	Memory cannot be wired down while holding
			 *	any locks that the pageout daemon might
			 *	need to free up pages.  [Making the zget_space
			 *	lock a complex lock does not help in this
			 *	regard.]
			 *
			 *	Unlock and allocate memory.  Because several
			 *	threads might try to do this at once, don't
			 *	use the memory before checking for available
			 *	space again.
			 */

			simple_unlock(simple_lock_addr(zget_space_lock));
			{
				kern_return_t	kr;

				kr = kmem_alloc_zone(zone_map,
						     &new_space, space_to_add,
						     	canblock);
				if (kr != KERN_SUCCESS) {
					if (kr == KERN_NO_SPACE)
						panic("zget_space");

					return(0);
				}
			}

			simple_lock(simple_lock_addr(zget_space_lock));
			continue;
		}
		else {
			/*
			 *	Memory was allocated in a previous iteration.
			 */

			result = zone_free_space_add(
						freespace,
						size,
						new_space,
						space_to_add);
			new_space = 0;
			break;
		}
	}
	simple_unlock(simple_lock_addr(zget_space_lock));

	if (new_space != 0)
		kmem_free(zone_map, new_space, space_to_add);

	return(result);
}

static
struct zone_free_space *
zone_free_space_alloc(alloc_unit, alloc_max)
	vm_size_t	alloc_unit;
	vm_size_t	alloc_max;
{
	struct zone_free_space	*freespace, **f;
	
	if (zone_free_space_count >=
			(sizeof (zone_free_space) / sizeof (freespace)))
		return (0);
	
	f = &zone_free_space[zone_free_space_count++];

	(vm_offset_t)freespace = zget_space(
					zone_default_space,
					sizeof (struct zone_free_space), 
					FALSE);
	freespace->alloc_unit = alloc_unit;
	freespace->alloc_max = alloc_max;

	freespace->entries = 0;
	freespace->num_entries = 0;
	
	freespace->hash_shift = 0;
	while (!(alloc_unit & 01)) {
		freespace->hash_shift++; alloc_unit >>= 1;
	}

	freespace->num_hints = freespace->alloc_max >> freespace->hash_shift;
	(vm_offset_t)freespace->hints =
			zget_space(
				zone_default_space,
				freespace->num_hints *
					sizeof (struct zone_free_space_hint),
				FALSE);
	bzero(freespace->hints, freespace->num_hints *
					sizeof (struct zone_free_space_hint));

	*f = freespace;
	
	return (freespace);
}

static
void
zone_free_space_select(zone)
	zone_t		zone;
{
	struct zone_free_space	**f = &zone_free_space[1];
	vm_size_t	elem_size;
	int		i;
	
	if (zone->cur_size > 0)
		return;
	
	for (i = 1; i < zone_free_space_count; i++) {
		elem_size = ((zone->elem_size + ((*f)->alloc_unit - 1))
				& ~((*f)->alloc_unit - 1));
		if (elem_size <= (*f)->alloc_max) {
			zone->elem_size = elem_size;				
			zone->free_space = *f;
			break;
		}
		
		f++;
	}
}

/*
 *	Initialize the "zone of zones" which uses fixed memory allocated
 *	earlier in memory initialization.  zone_bootstrap is called
 *	before zone_init.
 */
void zone_bootstrap(void)
{
	simple_lock_init(simple_lock_addr(all_zones_lock));
	first_zone = ZONE_NULL;
	last_zone = &first_zone;
	num_zones = 0;

	if (sizeof (struct zone_free_space_entry) > ZONE_MIN_ALLOC)
		panic("zone_bootstrap");

	simple_lock_init(simple_lock_addr(zget_space_lock));

	_zone_default_space.hints = &_zone_default_space_hint;
	_zone_default_space.num_hints = 1;
	
	zone_free_space[0] = &_zone_default_space;
	zone_free_space_count = 1;

	zone_zone = ZONE_NULL;
	zone_zone = zinit(sizeof(struct zone), 128 * sizeof(struct zone),
			  sizeof(struct zone), FALSE, "zones");

	zone_free_space_alloc(16,	96);
	zone_free_space_alloc(128,	768);
	zone_free_space_alloc(1024,	PAGE_SIZE);
}

vm_size_t
zone_map_sizer(void)
{
#if defined(__ppc__)
	vm_size_t	map_size = mem_size / 4;
#else
	vm_size_t	map_size = mem_size / 8;
#endif

	if (map_size < zone_map_size_min)
		map_size = zone_map_size_min;
	else
	if (map_size > zone_map_size_max)
		map_size = zone_map_size_max;

	return (map_size);
}

void zone_init(void)
{
	zone_map_size = zone_map_sizer();

	zone_map = kmem_suballoc(kernel_map, &zone_min, &zone_max,
				 zone_map_size, FALSE);
}

void
check_zone(zone, elem)
	zone_t	zone;
	vm_offset_t	elem;
{
	vm_offset_t this;
	/*
	 * check the zone's consistency
	 */
#if !DIAGNOSTIC
	if (!elem)
		return;
#endif
	for (this = zone->free_elements;
	     this != 0;
	     this = * (vm_offset_t *) this)
		if (this == elem)
			panic("check_zone: argument already free");
#if DIAGNOSTIC
		else if (this < zone->lowest || this > zone->highest)
			panic("check_zone: item out of range");
#endif
}

/*
 *	zalloc returns an element from the specified zone.
 */
static
vm_offset_t zalloc_canblock(zone, canblock)
	register zone_t	zone;
	boolean_t canblock;
{
	vm_offset_t	addr;

	if (zone == ZONE_NULL)
		panic ("zalloc: null zone");

	lock_zone(zone);
	if (zone_check & 8)
		check_zone(zone, 0);
	REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
	while (addr == 0) {
		/*
 		 *	If nothing was there, try to get more
		 */
		if (zone->doing_alloc) {
			/*
			 *	Someone is allocating memory for this zone.
			 *	Wait for it to show up, then try again.
			 */
			if (!canblock) {
				unlock_zone(zone);
				return(0);
			}
			assert_wait((event_t)&zone->doing_alloc, TRUE);
			/* XXX say wakeup needed */
			unlock_zone(zone);
			thread_block_with_continuation((void (*)()) 0);
			lock_zone(zone);
		}
		else {
			if ((zone->cur_size + (zone->pageable ?
				zone->alloc_size : zone->elem_size)) >
			    zone->max_size) {
				if (zone->exhaustible)
					break;

				if (zone->expandable) {
					/*
					 * We're willing to overflow certain
					 * zones, but not without complaining.
					 *
					 * This is best used in conjunction
					 * with the collecatable flag. What we
					 * want is an assurance we can get the
					 * memory back, assuming there's no
					 * leak. 
					 */
					zone->max_size += (zone->max_size >> 1);
				} else if (!zone_ignore_overflow) {
					unlock_zone(zone);
					if (!canblock)
						return(0);
					printf("zone \"%s\" empty.\n",
						zone->zone_name);
					panic("zalloc");
				}
			}

			if (zone->pageable)
				zone->doing_alloc = TRUE;
			unlock_zone(zone);

			if (zone->pageable) {
				if (kmem_alloc_pageable(zone_map, &addr,
							zone->alloc_size)
							!= KERN_SUCCESS)
					panic("zalloc");
				zcram(zone, addr, zone->alloc_size);
				lock_zone(zone);
				zone->doing_alloc = FALSE; 
				/* XXX check before doing this */
				thread_wakeup((event_t)&zone->doing_alloc);

				if (zone_check & 8)
					check_zone(zone, 0);
				REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
			} else {
				addr = zget_space(
					zone->free_space,
					zone->elem_size,
					canblock);
				if (addr == 0) {
					if (!canblock)
						return(0);
					panic("zalloc");
				}

				lock_zone(zone);
				zone->count++;
				zone->cur_size += zone->elem_size;
				WATERMARK_ZONE(zone, addr);
				unlock_zone(zone);
				return(addr);
			}
		}
	}

	unlock_zone(zone);
	return(addr);
}

vm_offset_t zalloc(zone)
	register zone_t zone;
{
	return (zalloc_canblock(zone, TRUE));
}

vm_offset_t zalloc_noblock(zone)
	register zone_t zone;
{
	return (zalloc_canblock(zone, FALSE));
}


/*
 *	zget returns an element from the specified zone
 *	and immediately returns nothing if there is nothing there.
 *
 *	This form should be used when you can not block (like when
 *	processing an interrupt).
 */
vm_offset_t zget(zone)
	register zone_t	zone;
{
	register vm_offset_t	addr;

	if (zone == ZONE_NULL)
		panic ("zalloc: null zone");

	lock_zone(zone);
	if (zone_check & 4)
		check_zone(zone, 0);
	REMOVE_FROM_ZONE(zone, addr, vm_offset_t);
	unlock_zone(zone);

	return(addr);
}

void
zfree(zone, elem)
	register zone_t	zone;
	vm_offset_t	elem;
{
	lock_zone(zone);
#if DIAGNOSTIC
	if (elem < zone->lowest || elem > zone->highest)
		panic("zfree: argument out of range");
#endif
	if (zone_check & 2)
		check_zone(zone, elem);
	ADD_TO_ZONE(zone, elem);
	unlock_zone(zone);
}

void zcollectable(zone) 
	zone_t		zone;
{
	/* zones are collectable by default
	 * and cannot later be changed back to collectable */
}

void zchange(zone, pageable, sleepable, exhaustible, collectable)
	zone_t		zone;
	boolean_t	pageable;
	boolean_t	sleepable;
	boolean_t	exhaustible;
	boolean_t	collectable;
{
	zone->pageable = pageable;
	zone->sleepable = sleepable;
	zone->exhaustible = exhaustible;
	/* zones are collectable by default
	 * and later can only be changed to non-collectable */
	if (!collectable)
		zone->free_space = zone_default_space;
	lock_zone_init(zone);
}

#if	MACH_DEBUG
kern_return_t host_zone_info(host, namesp, namesCntp, infop, infoCntp)
	host_t		host;
	zone_name_array_t *namesp;
	unsigned int	*namesCntp;
	zone_info_array_t *infop;
	unsigned int	*infoCntp;
{
	zone_name_t	*names;
	vm_offset_t	names_addr;
	vm_size_t	names_size = 0; /*'=0' to quiet gcc warnings */
	zone_info_t	*info;
	vm_offset_t	info_addr;
	vm_size_t	info_size = 0; /*'=0' to quiet gcc warnings */
	unsigned int	max_zones, i;
	zone_t		z;
	kern_return_t	kr;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	/*
	 *	We assume that zones aren't freed once allocated.
	 *	We won't pick up any zones that are allocated later.
	 */

	simple_lock(simple_lock_addr(all_zones_lock));
	max_zones = num_zones;
	z = first_zone;
	simple_unlock(simple_lock_addr(all_zones_lock));

	if (max_zones <= *namesCntp) {
		/* use in-line memory */

		names = *namesp;
	} else {
		names_size = round_page(max_zones * sizeof *names);
		kr = kmem_alloc_pageable(ipc_kernel_map,
					 &names_addr, names_size);
		if (kr != KERN_SUCCESS)
			return kr;

		names = (zone_name_t *) names_addr;
	}

	if (max_zones <= *infoCntp) {
		/* use in-line memory */

		info = *infop;
	} else {
		info_size = round_page(max_zones * sizeof *info);
		kr = kmem_alloc_pageable(ipc_kernel_map,
					 &info_addr, info_size);
		if (kr != KERN_SUCCESS) {
			if (names != *namesp)
				kmem_free(ipc_kernel_map,
					  names_addr, names_size);
			return kr;
		}

		info = (zone_info_t *) info_addr;
	}

	for (i = 0; i < max_zones; i++) {
		zone_name_t *zn = &names[i];
		zone_info_t *zi = &info[i];
		struct zone zcopy;

		assert(z != ZONE_NULL);

		lock_zone(z);
		zcopy = *z;
		unlock_zone(z);

		simple_lock(simple_lock_addr(all_zones_lock));
		z = z->next_zone;
		simple_unlock(simple_lock_addr(all_zones_lock));

		/* assuming here the name data is static */
		(void) strncpy(zn->zn_name, zcopy.zone_name,
			       sizeof zn->zn_name);

		zi->zi_count = zcopy.count;
		zi->zi_cur_size = zcopy.cur_size;
		zi->zi_max_size = zcopy.max_size;
		zi->zi_elem_size = zcopy.elem_size;
		zi->zi_alloc_size = zcopy.alloc_size;
		zi->zi_pageable = zcopy.pageable;
		zi->zi_sleepable = zcopy.sleepable;
		zi->zi_exhaustible = zcopy.exhaustible;
		zi->zi_collectable = zone_collectable(&zcopy);
	}

	if (names != *namesp) {
		vm_size_t used;
#if	MACH_OLD_VM_COPY
#else
		vm_map_copy_t copy;
#endif

		used = max_zones * sizeof *names;

		if (used != names_size)
			bzero((char *) (names_addr + used), names_size - used);

#if	MACH_OLD_VM_COPY
		kr = vm_move(
			ipc_kernel_map, names_addr,
			ipc_soft_map, names_size,
			TRUE, &names_addr);
		assert(kr == KERN_SUCCESS);

		*namesp = (zone_name_t *) names_addr;
#else
		kr = vm_map_copyin(ipc_kernel_map, names_addr, names_size,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		*namesp = (zone_name_t *) copy;
#endif
	}
	*namesCntp = max_zones;

	if (info != *infop) {
		vm_size_t used;
#if	MACH_OLD_VM_COPY
#else
		vm_map_copy_t copy;
#endif

		used = max_zones * sizeof *info;

		if (used != info_size)
			bzero((char *) (info_addr + used), info_size - used);

#if	MACH_OLD_VM_COPY
		kr = vm_move(
			ipc_kernel_map, info_addr,
			ipc_soft_map, info_size,
			TRUE, &info_addr);
		assert(kr == KERN_SUCCESS);

		*infop = (zone_info_t *) info_addr;
#else
		kr = vm_map_copyin(ipc_kernel_map, info_addr, info_size,
				   TRUE, &copy);
		assert(kr == KERN_SUCCESS);

		*infop = (zone_info_t *) copy;
#endif
	}
	*infoCntp = max_zones;

	return KERN_SUCCESS;
}

kern_return_t host_zone_free_space_info(
				host,
				infop, infoCnt,
				chunksp, chunksCnt)
	host_t				host;
	zone_free_space_info_array_t	*infop;
	mach_msg_type_number_t		*infoCnt;
	zone_free_space_chunk_array_t	*chunksp;
	mach_msg_type_number_t		*chunksCnt;
{
	kern_return_t	kr;
	vm_size_t	size1, size2;
	vm_offset_t	addr1, addr2;
	vm_offset_t	memory1, memory2;
	mach_msg_type_number_t
			actual1, actual2;
	zone_free_space_info_t
			*info;
	zone_free_space_chunk_t
			*chunk;
	struct zone_free_space_entry
			**last, *cur;
	struct zone_free_space
			*freespace;
	int		i;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	size1 = size2 = 0;

	for (;;) {
		vm_size_t	size1_needed, size2_needed;

		size1_needed = size2_needed = 0;

		simple_lock(simple_lock_addr(zget_space_lock));

		actual1 = actual2 = 0;
		
		for (actual1 = 0; actual1 < zone_free_space_count; actual1++)
			actual2 += zone_free_space[actual1]->num_entries;
		
		if (actual1 > *infoCnt)
			size1_needed = round_page(
				actual1 * sizeof (**infop));
		
		if (actual2 > *chunksCnt)
			size2_needed = round_page(
				actual2 * sizeof (**chunksp));

		if (size1_needed <= size1 &&
				size2_needed <= size2)
			break;

		simple_unlock(simple_lock_addr(zget_space_lock));

		if (size1 < size1_needed) {
			if (size1 != 0)
				kmem_free(ipc_kernel_map, addr1, size1);
			size1 = size1_needed;

			kr = kmem_alloc_pageable(
						ipc_kernel_map, &addr1, size1);
			if (kr != KERN_SUCCESS) {
				if (size2 != 0)
					kmem_free(
						ipc_kernel_map, addr2, size2);
				return KERN_RESOURCE_SHORTAGE;
			}
#if	MACH_OLD_VM_COPY
			kr = vm_map_pageable(
				ipc_kernel_map, addr1, addr1 + size1, FALSE);
			assert(kr == KERN_SUCCESS);
#else
			kr = vm_map_pageable(
					ipc_kernel_map, addr1, addr1 + size1,
						VM_PROT_READ|VM_PROT_WRITE);
			assert(kr == KERN_SUCCESS);
#endif
		}

		if (size2 < size2_needed) {
			if (size2 != 0)
				kmem_free(ipc_kernel_map, addr2, size2);
			size2 = size2_needed;

			kr = kmem_alloc_pageable(
						ipc_kernel_map, &addr2, size2);
			if (kr != KERN_SUCCESS) {
				if (size1 != 0)
					kmem_free(
						ipc_kernel_map, addr1, size1);
				return KERN_RESOURCE_SHORTAGE;
			}
#if	MACH_OLD_VM_COPY
			kr = vm_map_pageable(
				ipc_kernel_map, addr2, addr2 + size2, FALSE);
			assert(kr == KERN_SUCCESS);
#else
			kr = vm_map_pageable(
					ipc_kernel_map, addr2, addr2 + size2,
						VM_PROT_READ|VM_PROT_WRITE);
			assert(kr == KERN_SUCCESS);
#endif
		}
	}

	if (size1 != 0)
		info = (zone_free_space_info_t *)addr1;
	else
		info = *infop;

	if (size2 != 0)
		chunk = (zone_free_space_chunk_t *)addr2;
	else
		chunk = *chunksp;

	for (i = 0; i < actual1; i++) {
		freespace = zone_free_space[i];

		info->zf_alloc_unit = freespace->alloc_unit;
		info->zf_alloc_max = freespace->alloc_max;
		info->zf_num_chunks = freespace->num_entries;
		info++;

		last = &freespace->entries;
		while ((cur = *last) != 0) {
			chunk->zf_address = (vm_offset_t)cur;
			chunk->zf_length = cur->length;
			chunk++;
			
			last = &cur->next;
		}
	}

	simple_unlock(simple_lock_addr(zget_space_lock));

	if (actual1 != 0 && size1 != 0) {
		vm_size_t	size_used;
		
		size_used = round_page(actual1 * sizeof (**infop));

#if	MACH_OLD_VM_COPY
		kr = vm_map_pageable(
			ipc_kernel_map, addr1, addr1 + size_used, TRUE);
		assert(kr == KERN_SUCCESS);

		kr = vm_move(
			ipc_kernel_map, addr1,
			ipc_soft_map, size_used,
			TRUE, &memory1);
		assert(kr == KERN_SUCCESS);
#else
		kr = vm_map_pageable(
				ipc_kernel_map,
				addr1, addr1 + size_used, VM_PROT_NONE);
		assert(kr == KERN_SUCCESS);
	
		kr = vm_map_copyin(
				ipc_kernel_map, addr1, size_used,
				TRUE, &memory1);
		assert(kr == KERN_SUCCESS);
#endif
	
		if (size_used != size1)
			kmem_free(
				ipc_kernel_map,
				addr1 + size_used, size1 - size_used);

		*infop = (zone_free_space_info_t *)memory1;
	}
	else if (actual1 == 0) {
#if	MACH_OLD_VM_COPY
		*infop = (zone_free_space_info_t *)0;
#else
		*infop = (zone_free_space_info_t *)VM_MAP_COPY_NULL;
#endif

		if (size1 != 0)
			kmem_free(ipc_kernel_map, addr1, size1);
	}
	
	*infoCnt = actual1;

	if (actual2 != 0 && size2 != 0) {
		vm_size_t	size_used;

		size_used = round_page(actual2 * sizeof (**chunksp));

#if	MACH_OLD_VM_COPY
		kr = vm_map_pageable(
			ipc_kernel_map, addr2, addr2 + size_used, TRUE);
		assert(kr == KERN_SUCCESS);

		kr = vm_move(
			ipc_kernel_map, addr2,
			ipc_soft_map, size_used,
			TRUE, &memory2);
		assert(kr == KERN_SUCCESS);
#else
		kr = vm_map_pageable(
				ipc_kernel_map,
				addr2, addr2 + size2, VM_PROT_NONE);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_copyin(
				ipc_kernel_map, addr2, size_used,
				TRUE, &memory2);
		assert(kr == KERN_SUCCESS);
#endif

		if (size_used != size2)
			kmem_free(
				ipc_kernel_map,
				addr2 + size_used, size2 - size_used);

		*chunksp = (zone_free_space_chunk_t *)memory2;
	}
	else if (actual2 == 0) {
#if	MACH_OLD_VM_COPY
		*chunksp = (zone_free_space_chunk_t *)0;
#else
		*chunksp = (zone_free_space_chunk_t *)VM_MAP_COPY_NULL;
#endif

		if (size2 != 0)
			kmem_free(ipc_kernel_map, addr2, size2);
	}

	*chunksCnt = actual2;

	return KERN_SUCCESS;
}

kern_return_t host_zone_collect(
	host_t		host,
	boolean_t	collect_zones,
	boolean_t	reclaim_pages)
{
    	struct zone_free_space_entry
			*cur, *pages = 0;
	zone_t		z;
	int		max_zones, i;

	if (host == HOST_NULL)
		return KERN_INVALID_HOST;

	if (!collect_zones)
	    return KERN_SUCCESS;
	
	simple_lock(simple_lock_addr(zget_space_lock));

	simple_lock(simple_lock_addr(all_zones_lock));
	max_zones = num_zones;
	z = first_zone;
	simple_unlock(simple_lock_addr(all_zones_lock));

	for (i = 0; i < max_zones; i++) {
		assert(z != ZONE_NULL);
	/* run this at splhigh so that interupt routines that use zones
	   can not interupt while their zone is locked */
		lock_zone(z);

		if (!z->pageable && zone_collectable(z))
		    zone_collect(z);

		unlock_zone(z);		
		simple_lock(simple_lock_addr(all_zones_lock));
		z = z->next_zone;
		simple_unlock(simple_lock_addr(all_zones_lock));
	}

	if (reclaim_pages)
		pages = zone_free_space_reclaim();
	
	simple_unlock(simple_lock_addr(zget_space_lock));

	/*
	 * Return any reclaimed pages to
	 * the system.
	 */
	while ((cur = pages) != 0) {
		pages = cur->next;
		kmem_free(zone_map, (vm_offset_t)cur, cur->length);
	}

	return KERN_SUCCESS;
}
#endif	MACH_DEBUG
