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
 *	File:	vm/vm_page.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Resident memory management module.
 */

#import <mach/features.h>

#include <mach/vm_prot.h>
#include <kern/counters.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <mach/vm_statistics.h>
#include <kern/xpr.h>
#include <kern/zalloc.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>

#include <mach/vm_policy.h>

/*
 *	Associated with eacn page of user-allocatable memory is a
 *	page structure.
 */

/*
 *	These variables record the values returned by vm_page_bootstrap,
 *	for debugging purposes.  The implementation of pmap_steal_memory
 *	and pmap_startup here also uses them internally.
 */

vm_offset_t virtual_space_start;
vm_offset_t virtual_space_end;

/*
 *	The vm_page_lookup() routine, which provides for fast
 *	(virtual memory object, offset) to page lookup, employs
 *	the following hash table.  The vm_page_{insert,remove}
 *	routines install and remove associations in the table.
 *	[This table is often called the virtual-to-physical,
 *	or VP, table.]
 */
typedef struct {
	decl_simple_lock_data(,lock)
	vm_page_t pages;
} vm_page_bucket_t;

vm_page_bucket_t *vm_page_buckets;		/* Array of buckets */
unsigned int	vm_page_bucket_count = 0;	/* How big is array? */
unsigned int	vm_page_hash_mask;		/* Mask for hash function */

/*
 *	The virtual page size is currently implemented as a runtime
 *	variable, but is constant once initialized using vm_set_page_size.
 *	This initialization must be done in the machine-dependent
 *	bootstrap sequence, before calling other machine-independent
 *	initializations.
 *
 *	All references to the virtual page size outside this
 *	module must use the PAGE_SIZE constant.
 */
vm_size_t	page_size  = 0;
vm_size_t	page_mask;
int		page_shift;

vm_offset_t	map_data;
vm_size_t	map_data_size;

vm_offset_t	kentry_data;
vm_size_t	kentry_data_size;

extern vm_offset_t	zdata;
extern vm_size_t	zdata_size;

queue_head_t	vm_page_queue_free;
queue_head_t	vm_page_queue_active;
queue_head_t	vm_page_queue_inactive;
simple_lock_data_t	vm_page_queue_lock;
simple_lock_data_t	vm_page_queue_free_lock;

vm_page_t	vm_page_array;
long		first_page;
long		last_page;
vm_offset_t	first_phys_addr;
vm_offset_t	last_phys_addr;

int	vm_page_free_count;
int	vm_page_active_count;
int	vm_page_inactive_count;
int	vm_page_wire_count;

/*
 *	Several page replacement parameters are also
 *	shared with this module, so that page allocation
 *	(done here in vm_page_alloc) can trigger the
 *	pageout daemon.
 */
int	vm_page_free_target = 0;
int	vm_page_free_min = 0;
int	vm_page_inactive_target = 0;
int	vm_page_free_reserved = 0;
int	vm_page_laundry_count = 0;

struct vm_page	vm_page_template;

#if	SHOW_SPACE
extern int	show_space;
#endif	SHOW_SPACE

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from page_size.
 */
void vm_set_page_size(void)
{
	page_mask = page_size - 1;

	if ((page_mask & page_size) != 0)
		panic("vm_set_page_size: page size not a power of two");

	for (page_shift = 0; ; page_shift++)
		if ((1 << page_shift) == page_size)
			break;
}

/*
 *	vm_page_startup:
 *
 *	Initializes the resident memory module.
 *
 *	Allocates memory for the page cells, and
 *	for the object/offset-to-page hash table headers.
 *	Each page cell is initialized and placed on the free list.
 */

#if	SHOW_SPACE
#define	PALLOC(name, type, num)					\
MACRO_BEGIN							\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((num) * sizeof(type),	\
				       __alignof__(type));	\
	bzero((name), (num) * sizeof(type));			\
	if (show_space) {					\
		printf(#name " = %d (0x%x) bytes @%x,"		\
		  " %d cells of %d bytes\n",			\
		  (num)*sizeof(type), (num)*sizeof(type), name,	\
		  num, sizeof(type));				\
	}							\
MACRO_END

#define	PALLOC_SIZE(name, size, type, num)			\
MACRO_BEGIN							\
	(size) = (num) * sizeof(type);				\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((size),			\
				      __alignof__(type));	\
	bzero((name), (size));					\
	if (show_space) {					\
		printf(#name " = %d (0x%x) bytes @%x,"		\
		  " %d cells of %d bytes\n",			\
		  (size), (size), name,				\
		  num, sizeof(type));				\
	}							\
MACRO_END

#define	PALLOC_PAGES(name, type, size)				\
MACRO_BEGIN							\
	(size) = round_page(size);				\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((size), PAGE_SIZE);	\
	bzero((name), (size));					\
	if (show_space) {					\
		printf(#name " = %d (0x%x) bytes @%x,"		\
		  " %d pages of %d bytes\n",			\
		  (size), (size), name,				\
		  (size) / PAGE_SIZE, PAGE_SIZE);		\
	}							\
MACRO_END
#else	SHOW_SPACE
#define	PALLOC(name, type, num)					\
MACRO_BEGIN							\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((num) * sizeof(type),	\
				       __alignof__(type));	\
	bzero((name), (num) * sizeof(type));			\
MACRO_END

#define	PALLOC_SIZE(name, size, type, num)			\
MACRO_BEGIN							\
	(size) = (num) * sizeof(type);				\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((size),			\
				       __alignof__(type));	\
	bzero((name), (size));					\
MACRO_END

#define	PALLOC_PAGES(name, type, size)				\
MACRO_BEGIN							\
	(size) = round_page(size);				\
	(type *)(name) = (type *)				\
		vm_alloc_from_regions((size), PAGE_SIZE);	\
	bzero((name), (size));					\
MACRO_END
#endif	SHOW_SPACE

vm_offset_t vm_page_startup(mem_region, num_regions, vavail)
	mem_region_t		mem_region;
	int			num_regions;
	vm_offset_t		vavail;
{
	int			mem_size;
	mem_region_t		rp;
	vm_page_t		m;
	queue_t			bucket;
	int			i;
	vm_offset_t		pa;
	extern vm_offset_t	virtual_avail;

	/*
	 *	Initialize the vm_page template.
	 */

	m = &vm_page_template;
	m->object = VM_OBJECT_NULL;	/* reset later */
	m->offset = 0;			/* reset later */
	m->wire_count = 0;

#if	OLD_VM_CODE
	m->clean = TRUE;
	m->nfspagereq = FALSE;
	m->copy_on_write = FALSE;
	m->asyncrw = FALSE;
#endif
	m->inactive = FALSE;
	m->active = FALSE;
	m->laundry = FALSE;
	m->free = FALSE;

	m->busy = TRUE;
	m->wanted = FALSE;
	m->tabled = FALSE;
	m->fictitious = FALSE;
	m->private = FALSE;
	m->absent = FALSE;
	m->error = FALSE;
	m->dirty = FALSE;
	m->precious = FALSE;
	m->reference = FALSE;

	m->phys_addr = 0;		/* reset later */

	m->page_lock = VM_PROT_NONE;
	m->unlock_request = VM_PROT_NONE;

	/*
	 *	Initialize the locks
	 */

	simple_lock_init(&vm_page_queue_free_lock);
	simple_lock_init(&vm_page_queue_lock);

	/*
	 *	Initialize the queue headers for the free queue,
	 *	the active queue and the inactive queue.
	 */

	queue_init(&vm_page_queue_free);
	queue_init(&vm_page_queue_active);
	queue_init(&vm_page_queue_inactive);

	/*
	 *	Allocate (and initialize) the virtual-to-physical
	 *	table hash buckets.
	 *
	 *	The number of buckets should be a power of two to
	 *	get a good hash function.  The following computation
	 *	chooses the first power of two that is greater
	 *	than the number of physical pages in the system.
	 */
	mem_size = 0;
	for (rp = mem_region; rp < &mem_region[num_regions]; rp += 1) {
		mem_size += trunc_page(rp->last_phys_addr)
		  - round_page(rp->first_phys_addr);
	}

	if (vm_page_bucket_count == 0) {
		vm_page_bucket_count = 1;
		while (vm_page_bucket_count < atop(mem_size))
			vm_page_bucket_count <<= 1;
	}

	vm_page_hash_mask = vm_page_bucket_count - 1;

	if (vm_page_hash_mask & vm_page_bucket_count)
		printf("vm_page_bootstrap: WARNING -- strange page hash\n");

	PALLOC(vm_page_buckets, vm_page_bucket_t, vm_page_bucket_count);

	for (i = 0; i < vm_page_bucket_count; i++) {
		register vm_page_bucket_t *bucket = &vm_page_buckets[i];

		bucket->pages = VM_PAGE_NULL;
		simple_lock_init(&bucket->lock);
	}

	/*
	 *	Steal pages for some zones that cannot be
	 *	dynamically allocated.
	 */
#if	1
	zdata_size = 8*PAGE_SIZE;
	PALLOC_PAGES(zdata, void, zdata_size);
#else
	PALLOC_SIZE(zdata, zdata_size, struct zone, 40);
#endif
	PALLOC_SIZE(map_data, map_data_size, struct vm_map, 10);

	/*
	 *	Allow 2048 kernel map entries... this should be plenty
	 *	since people shouldn't be cluttering up the kernel
	 *	map (they should use their own maps).
	 */
	PALLOC_SIZE(kentry_data, kentry_data_size, struct vm_map_entry, 2048);

	/*
	 * First make a pass over each region and allocate
	 * vm_page_arrays.  Later go back and size the pages in the
	 * region (since some of the region may have been taken
	 * by the vm_page_array).
	 */
	for (rp = mem_region; rp < &mem_region[num_regions]; rp += 1) {
		ASSERT(rp->first_phys_addr <= rp->last_phys_addr);
		PALLOC(rp->vm_page_array, struct vm_page,
		       atop(trunc_page(rp->last_phys_addr) -
		            round_page(rp->first_phys_addr)));
	}
	vm_page_free_count = 0;
	for (rp = mem_region; rp < &mem_region[num_regions]; rp += 1) {
		rp->first_phys_addr = round_page(rp->first_phys_addr);
		rp->last_phys_addr = trunc_page(rp->last_phys_addr);
		rp->first_page = atop(rp->first_phys_addr);
		rp->last_page = atop(rp->last_phys_addr);
		rp->num_pages = rp->last_page - rp->first_page;
		ASSERT((int)rp->num_pages >= 0);
		vm_page_free_count += rp->num_pages;

		m = rp->vm_page_array;
		pa = rp->first_phys_addr;

		for (i = 0; i < rp->num_pages; i += 1) {
			m->phys_addr = pa;
			queue_enter(&vm_page_queue_free, m, vm_page_t, pageq);
			m->free = TRUE;
			m++;
			pa += page_size;
		}
	}

	/*
	 *	Initialize vm_pages_needed lock here - don't wait for pageout
	 *	daemon	XXX
	 */
	simple_lock_init(&vm_pages_needed_lock);

	return(virtual_avail);
}

/*
 *	vm_page_hash:
 *
 *	Distributes the object/offset key pair among hash buckets.
 *
 *	NOTE:	To get a good hash function, the bucket count should
 *		be a power of two.
 */
#define vm_page_hash(object, offset) \
	(((unsigned int)(vm_offset_t)object + (unsigned int)atop(offset)) \
		& vm_page_hash_mask)

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the object/object-page
 *	table and object list.
 *
 *	The object and page must be locked.
 */

void vm_page_insert(
	register vm_page_t	mem,
	register vm_object_t	object,
	register vm_offset_t	offset)
{
	register vm_page_bucket_t *bucket;

	VM_PAGE_CHECK(mem);

	if (mem->tabled)
		panic("vm_page_insert");

	/*
	 *	Record the object/offset pair in this page
	 */

	mem->object = object;
	mem->offset = offset;

	/*
	 *	Insert it into the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];
{
	int spl = splimp();

	simple_lock(&bucket->lock);
	mem->next = bucket->pages;
	bucket->pages = mem;
	simple_unlock(&bucket->lock);
	(void) splx(spl);
}

	/*
	 *	Now link into the object's list of backed pages.
	 */

	queue_enter(&object->memq, mem, vm_page_t, listq);
	mem->tabled = TRUE;

	/*
	 *	Show that the object has one more resident page.
	 */

	object->resident_page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 *
 *	Removes the given mem entry from the object/offset-page
 *	table and the object page list.
 *
 *	The object and page must be locked.
 */

void vm_page_remove(
	register vm_page_t	mem)
{
	register vm_page_bucket_t	*bucket;
	register vm_page_t	this;

	assert(mem->tabled);
	VM_PAGE_CHECK(mem);

	if (!mem->tabled)
		return;

	/*
	 *	Remove from the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(mem->object, mem->offset)];
{
	int spl = splimp();

	simple_lock(&bucket->lock);
	if ((this = bucket->pages) == mem) {
		/* optimize for common case */

		bucket->pages = mem->next;
	} else {
		register vm_page_t	*prev;

		for (prev = &this->next;
		     (this = *prev) != mem;
		     prev = &this->next)
			continue;
		*prev = this->next;
	}
	simple_unlock(&bucket->lock);
	splx(spl);
}

	/*
	 *	Now remove from the object's list of backed pages.
	 */

	queue_remove(&mem->object->memq, mem, vm_page_t, listq);

	/*
	 *	And show that the object has one fewer resident
	 *	page.
	 */

	mem->object->resident_page_count--;

	mem->tabled = FALSE;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the object/offset
 *	pair specified; if none is found, VM_PAGE_NULL is returned.
 *
 *	The object must be locked.  No side effects.
 */

vm_page_t vm_page_lookup(
	register vm_object_t	object,
	register vm_offset_t	offset)
{
	register vm_page_t	mem;
	register vm_page_bucket_t *bucket;

	/*
	 *	Search the hash table for this object/offset pair
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];

{
	int spl = splimp();

	simple_lock(&bucket->lock);
	for (mem = bucket->pages; mem != VM_PAGE_NULL; mem = mem->next) {
		VM_PAGE_CHECK(mem);
		if ((mem->object == object) && (mem->offset == offset))
			break;
	}
	simple_unlock(&bucket->lock);
	splx(spl);
}
	return mem;
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current object to the specified target object/offset.
 *
 *	The object must be locked.
 */
void vm_page_rename(
	register vm_page_t	mem,
	register vm_object_t	new_object,
	vm_offset_t		new_offset)
{
	/*
	 *	Changes to mem->object require the page lock because
	 *	the pageout daemon uses that lock to get the object.
	 */

	vm_page_lock_queues();
    	vm_page_remove(mem);
	vm_page_insert(mem, new_object, new_offset);
	vm_page_unlock_queues();
}

/*
 *	vm_page_init:
 *
 *	Initialize the given vm_page, entering it into
 *	the VP table at the given (object, offset),
 *	and noting its physical address.
 *
 *	Implemented using a template set up in vm_page_startup.
 *	All fields except those passed as arguments are static.
 */
void		vm_page_init(mem, object, offset, phys_addr)
	vm_page_t	mem;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	phys_addr;
{
#define	vm_page_init(page, object, offset, pa)  { \
		register \
		vm_offset_t	a = (pa); \
		*(page) = vm_page_template; \
		(page)->phys_addr = a; \
		vm_page_insert((page), (object), (offset)); \
	}

	vm_page_init(mem, object, offset, phys_addr);
}

/*
 *	vm_page_alloc_sequential:
 *
 *	vm_page_alloc: is a macro  calling
 *		vm_page_alloc_sequential(object,offset,TRUE)
 *
 *	Allocate and return a memory cell associated
 *	with this VM object/offset pair. Don't perform sequential 
 *	access checks if called from ICS.
 *
 *	Object must be locked.
 */
vm_page_t vm_page_alloc_sequential(object, offset, sequential_unmap)
	vm_object_t	object;
	vm_offset_t	offset;
	boolean_t 	sequential_unmap;
{
	register vm_page_t	mem;
	int		spl;

	spl = splimp();
	simple_lock(&vm_page_queue_free_lock);

	if (queue_empty(&vm_page_queue_free)) {
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return(VM_PAGE_NULL);
	}

	if ((vm_page_free_count < vm_page_free_reserved) &&
			!current_thread()->vm_privilege) {
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return(VM_PAGE_NULL);
	}

	queue_remove_first(&vm_page_queue_free, mem, vm_page_t, pageq);
	mem->free = FALSE;

	vm_page_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

 	vm_page_remove(mem);	/* in case it is still in hash table */

	vm_page_init(mem, object, offset, mem->phys_addr);

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if ((vm_page_free_count < vm_page_free_min) ||
			((vm_page_free_count < vm_page_free_target) &&
			(vm_page_inactive_count < vm_page_inactive_target))){
		thread_wakeup(&vm_pages_needed);
	}

	/*
	 *	Detect sequential access and inactivate previous page
	 */
	if (object->policy & VM_POLICY_SEQUENTIAL && sequential_unmap &&
	    (abs(offset - object->last_alloc) == PAGE_SIZE)) {
		vm_page_t	last_mem;

		last_mem = vm_page_lookup(object, object->last_alloc);
		if (last_mem != VM_PAGE_NULL) {
			int	when;
			
			switch (object->policy) {
				case VM_POLICY_SEQ_DEACTIVATE:
					when = VM_DEACTIVATE_SOON;
					break;
				case VM_POLICY_SEQ_FREE:
				default:
					when = VM_DEACTIVATE_NOW;
					break;
			}
			vm_policy_apply(object, last_mem, when);
		}
	}
//	else if (object->policy & VM_SEQUENTIAL && sequential_unmap)
//		object->policy = VM_RANDOM;

	object->last_alloc = offset;
	return(mem);
}

/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
void vm_page_free(mem)
	register vm_page_t	mem;
{
	vm_page_remove(mem);
	if (!mem->free) {
		vm_page_addfree(mem);
	}
}

void vm_page_addfree(mem)
	register vm_page_t	mem;
{
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
		queue_enter(&vm_page_queue_free, mem, vm_page_t, pageq);
		mem->free = TRUE;
		vm_page_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.
 *
 *	The page queues must be locked.
 */
void vm_page_wire(mem)
	register vm_page_t	mem;
{
	VM_PAGE_CHECK(mem);

	if (mem->wire_count == 0) {
		if (mem->active) {
			queue_remove(&vm_page_queue_active, mem, vm_page_t,
						pageq);
			vm_page_active_count--;
			mem->active = FALSE;
		}
		if (mem->inactive) {
			queue_remove(&vm_page_queue_inactive, mem, vm_page_t,
						pageq);
			vm_page_inactive_count--;
			mem->inactive = FALSE;
		}
		if (mem->free) {
			queue_remove(&vm_page_queue_free, mem, vm_page_t,
							pageq);
			vm_page_free_count--;
			mem->free = FALSE;
		}
		vm_page_wire_count++;
	}
	mem->wire_count++;
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 *
 *	The page queues must be locked.
 */
void vm_page_unwire(mem)
	register vm_page_t	mem;
{
	VM_PAGE_CHECK(mem);

	if (--mem->wire_count == 0) {
		queue_enter(&vm_page_queue_active, mem, vm_page_t, pageq);
		vm_page_active_count++;
		mem->active = TRUE;
		vm_page_wire_count--;
	}
}

/*
 *	_vm_page_deactivate:
 *
 *	Internal routine to Returns the given page to the inactive list,
 *	Page is put at the head of the inactive list if age is TRUE.
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
static void _vm_page_deactivate(m, age)
	register vm_page_t	m;
	register boolean_t age;
{
	VM_PAGE_CHECK(m);

	/*
	 *	Only move active pages -- ignore locked or already
	 *	inactive ones.
	 */

	if (m->active) {
		pmap_clear_reference(VM_PAGE_TO_PHYS(m));
		queue_remove(&vm_page_queue_active, m, vm_page_t, pageq);
		if (age)
			queue_enter_first(&vm_page_queue_inactive, m, vm_page_t, pageq);
		else
			queue_enter(&vm_page_queue_inactive, m, vm_page_t, pageq);
		m->active = FALSE;
		m->inactive = TRUE;
		vm_page_active_count--;
		vm_page_inactive_count++;
		if (m->clean && pmap_is_modified(VM_PAGE_TO_PHYS(m)))
			m->clean = FALSE;
		m->laundry = !m->clean;
	}
}

/*
 *	vm_page_deactivate:
 *
 *	Returns the given page to the inactive list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
void vm_page_deactivate(m)
	register vm_page_t	m;
{
	_vm_page_deactivate(m, FALSE);
}

/*
 *	vm_page_deactivate_first:
 *
 *	Returns the given page to the head of inactive list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
void vm_page_deactivate_first(m)
	register vm_page_t	m;
{
	_vm_page_deactivate(m, FALSE);
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	The page queues must be locked.
 */

void vm_page_activate(m)
	register vm_page_t	m;
{
	VM_PAGE_CHECK(m);


	if (m->inactive) {
		queue_remove(&vm_page_queue_inactive, m, vm_page_t,
						pageq);
		vm_page_inactive_count--;
		m->inactive = FALSE;
	}
	if (m->free) {
		queue_remove(&vm_page_queue_free, m, vm_page_t,
						pageq);
		vm_page_free_count--;
		m->free = FALSE;
	}
	if (m->wire_count == 0) {
		if (m->active)
			panic("vm_page_activate: already active");

		queue_enter(&vm_page_queue_active, m, vm_page_t, pageq);
		m->active = TRUE;
		vm_page_active_count++;
	}
}

/*
 *	vm_page_zero_fill:
 *
 *	Zero-fill the specified page.
 *	Written as a standard pagein routine, to
 *	be used by the zero-fill object.
 */

boolean_t vm_page_zero_fill(m)
	vm_page_t	m;
{
	VM_PAGE_CHECK(m);

	pmap_zero_page(VM_PAGE_TO_PHYS(m));
	return(TRUE);
}

/*
 *	vm_page_copy:
 *
 *	Copy one page to another
 */

void vm_page_copy(src_m, dest_m)
	vm_page_t	src_m;
	vm_page_t	dest_m;
{
	VM_PAGE_CHECK(src_m);
	VM_PAGE_CHECK(dest_m);

	pmap_copy_page(VM_PAGE_TO_PHYS(src_m), VM_PAGE_TO_PHYS(dest_m));
}


/*
 *	vm_page_to_phys:
 *
 *	return the physical page, this routine is here so that
 *	loadable drivers like macfs do not have to deref vm_page_t 
 * 	directly.
 */

vm_offset_t
vm_page_to_phys(m)
	vm_page_t	m;
{
	return(VM_PAGE_TO_PHYS(m));
}
