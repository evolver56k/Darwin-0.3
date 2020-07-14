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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Private definitions for pmap.
 *
 * HISTORY
 *
 * 10 April 1992 ? at NeXT
 *	Created.
 */
 
/*
 * For each managed physical page, there is a list of all currently
 * valid virtual mappings of that page.
 */
typedef struct pv_entry {
    struct pv_entry *	next;		/* next pv_entry */
    pmap_t		pmap;		/* pmap where mapping lies */
    vm_offset_t		va;		/* virtual address for mapping */
} pv_head_t, *pv_entry_t;

#define	PV_ENTRY_NULL	((pv_entry_t) 0)

zone_t		pv_entry_zone;		/* zone of pv_entry structures */

/*
 * First and last physical addresses that we maintain any information
 * for.  Initialized to zero so that pmap operations done before
 * pmap_init won't touch any non-existent structures.
 */
vm_offset_t	vm_first_phys = (vm_offset_t) 0;
vm_offset_t	vm_last_phys  = (vm_offset_t) 0;
#define managed_page(pa)	\
    ((pa) >= vm_first_phys && (pa) < vm_last_phys)
boolean_t	pmap_initialized = FALSE;/* Has pmap_init completed? */

/*
 * Page descriptor -
 * Maintained for each managed page.
 */
typedef struct pg_desc {
    pv_head_t		pv_list;		/* head of pv list for page  */
    struct pg_exten *	pg_exten;		/* page extension  for page  */
    unsigned int	page_attrib	:8,	/* page attributes:	     */
#define	PG_DIRTY    0x01			/* page has been modified    */
#define	PG_REFER    0x02			/* page has been referenced  */
					:0;
} pg_desc_t;

#define PG_DESC_NULL	((pg_desc_t *) 0)

pg_desc_t	*pg_desc_tbl;
unsigned int	pg_first_phys;

#define pg_desc(pa) \
     (&pg_desc_tbl[phys_to_pfn((unsigned int)(pa) -	\
				   pg_first_phys) >> (ptes_per_vm_page - 1)])

#define pg_desc_pvh(pd)		(&(pd)->pv_list)

/*
 * Page extension -
 * Contains information for pages used
 * as page tables or page directories.
 * NB: the 'links' field *must* be first.
 */
typedef struct pg_exten {
    queue_chain_t	links;			/* active/free queue links   */
    struct pg_desc *	pg_desc;		/* page descriptor for page  */
    unsigned int	phys;			/* physical address of page  */
    struct pmap *	pmap;			/* PT: which address map     */
    vm_offset_t		offset;			/* PT: what offset in map    */
    unsigned int	alloc_count	:16,	/* PT: mappings, PD: pg dirs */
    			wired_count	:16;	/* PT: wired mappings	     */
    unsigned int	alloc_bmap	:8,	/* PD: alloc map	     */
    			unrefd_age	:8,	/* PT: # aging ticks	     */
    					:0;
} pg_exten_t;

#define PG_EXTEN_NULL	((pg_exten_t *) 0)

zone_t		pg_exten_zone;

/*
 * A queue of free page tables is
 * maintained which is emptied by
 * pmap_update().  This strategy is
 * used for two reasons.  First, it is
 * inconvenient to actually free the pages
 * at the point in the code where it would
 * be necessary.  Secondly, keeping easily
 * available spare page tables around makes
 * pmap expansions quicker.
 */
queue_head_t	pt_active_queue, pt_free_queue;
int		pt_active_count, pt_free_count;
int		pt_alloc_count;

static __inline__
void
pt_active_add(
    pg_exten_t	*pe
)
{
    enqueue_tail(&pt_active_queue, (queue_entry_t)pe);
    pt_active_count++;
}

static __inline__
void
pt_active_remove(
    pg_exten_t	*pe
)
{
    remqueue(&pt_active_queue, (queue_entry_t)pe);
    pt_active_count--;
}

static __inline__
pg_exten_t *
pt_free_obtain(
    void
)
{
    pg_exten_t	*pe;

    if (((queue_entry_t)pe = dequeue_head(&pt_free_queue)))
	pt_free_count--;
    
    return (pe);
}

static __inline__
void
pt_free_add(
    pg_exten_t	*pe
)
{
    enqueue_tail(&pt_free_queue, (queue_entry_t)pe);
    pt_free_count++;
}

/*
 * A queue of pages that contain free
 * page directories is maintained.  This
 * is also emptied by pmap_update() (if
 * possible).  This added complexity is
 * needed to avoid wasting memory when
 * running with a large MI page size.
 */
queue_head_t	pd_free_queue;
int		pd_free_count;
int		pd_alloc_count;

static __inline__
pg_exten_t *
pd_free_obtain(
    void
)
{
    if (queue_empty(&pd_free_queue))
    	return (0);
    
    return ((pg_exten_t *) queue_first(&pd_free_queue));
}

static __inline__
void
pd_free_add(
    pg_exten_t	*pe
)
{
    enqueue_head(&pd_free_queue, (queue_entry_t)pe);
    pd_free_count++;
}

static __inline__
void
pd_free_remove(
    pg_exten_t	*pe
)
{
    remqueue(&pd_free_queue, (queue_entry_t)pe);
    pd_free_count--;
}

/*
 * A section is defined to be the amount
 * of address space mapped by a single
 * page table.
 */

/*
 * Machine dependent sections.
 */
#define I386_SECTBYTES		((I386_PGBYTES / sizeof (pt_entry_t)) * \
				    I386_PGBYTES)
#define i386_round_section(x)	((((unsigned int)(x)) + I386_SECTBYTES - 1) & \
				    ~(I386_SECTBYTES - 1)) 
#define i386_trunc_section(x)	(((unsigned int)(x)) & ~(I386_SECTBYTES - 1))

/*
 * Machine independent sections.
 */
#define SECTION_SIZE		((PAGE_SIZE / sizeof (pt_entry_t)) * \
				    I386_PGBYTES)
vm_offset_t			section_size;
#define round_section(x)	((((unsigned int)(x)) + section_size - 1) & \
				    ~(section_size - 1)) 
#define trunc_section(x)	(((unsigned int)(x)) & ~(section_size - 1))

/*
 *	Locking and TLB invalidation
 */

/*
 *	Locking Protocols:
 *
 *	There are two structures in the pmap module that need locking:
 *	the pmaps themselves, and the per-page pv_lists (which are locked
 *	by locking the pv_lock_table entry that corresponds to the pv_head
 *	for the list in question.)  Most routines want to lock a pmap and
 *	then do operations in it that require pv_list locking -- however
 *	pmap_remove_all and pmap_copy_on_write operate on a physical page
 *	basis and want to do the locking in the reverse order, i.e. lock
 *	a pv_list and then go through all the pmaps referenced by that list.
 *	To protect against deadlock between these two cases, the pmap_lock
 *	is used.  There are three different locking protocols as a result:
 *
 *  1.  pmap operations only (pmap_extract, pmap_access, ...)  Lock only
 *		the pmap.
 *
 *  2.  pmap-based operations (pmap_enter, pmap_remove, ...)  Get a read
 *		lock on the pmap_lock (shared read), then lock the pmap
 *		and finally the pv_lists as needed [i.e. pmap lock before
 *		pv_list lock.]
 *
 *  3.  pv_list-based operations (pmap_remove_all, pmap_copy_on_write, ...)
 *		Get a write lock on the pmap_lock (exclusive write); this
 *		also guaranteees exclusive access to the pv_lists.  Lock the
 *		pmaps as needed.
 *
 *	At no time may any routine hold more than one pmap lock or more than
 *	one pv_list lock.  Because interrupt level routines can allocate
 *	mbufs and cause pmap_enter's, the pmap_lock and the lock on the
 *	kernel_pmap can only be held at splvm.
 *	
 *	The dev_addr list is protected by using a write lock on the 
 *	pmap_lock.  Lookups are expected to be cheap.
 */

#define SPLVM(spl)	{ spl = splvm(); }
#define SPLX(spl)	{ splx(spl); }

#define PMAP_READ_LOCK(pmap, spl)	SPLVM(spl)
#define PMAP_WRITE_LOCK(spl)		SPLVM(spl)
#define PMAP_READ_UNLOCK(pmap, spl)	SPLX(spl)
#define PMAP_WRITE_UNLOCK(spl)		SPLX(spl)
#define PMAP_WRITE_TO_READ_LOCK(pmap)

#define LOCK_PVH(index)
#define UNLOCK_PVH(index)

#define PMAP_UPDATE_TLBS(pmap, s, e)			\
    pmap_update_tlbs((pmap), (s), (e))

struct {
    int		total;
    int		single;
} tlb_stat;

/*
 * Kernel pmap is statically
 * allocated.
 */
struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;

/*
 * Zone to allocate new
 * pmaps from.
 */
zone_t		pmap_zone;

int		ptes_per_vm_page;

static void		pmap_expand(
					pmap_t		pmap,
					vm_offset_t	va);
static void		pmap_allocate_mapping(
					pmap_t		pmap,
					vm_offset_t	va,
					boolean_t	wired);
static void		pmap_deallocate_mappings(
					pmap_t		pmap,
					vm_offset_t	va,
					int		count,
					int		unwire_count,
					boolean_t	free_table);
static void		pmap_wire_mapping(
					pmap_t		pmap,
					vm_offset_t	va);
static void		pmap_unwire_mapping(
					pmap_t		pmap,
					vm_offset_t	va);
static void		pmap_remove_range(
					pmap_t		pmap,
					vm_offset_t	start,
					vm_offset_t	end,
					boolean_t	free_table);
static void		pmap_clear_page_attrib(
					vm_offset_t	pa,
					int		attrib);
static boolean_t	pmap_check_page_attrib(
					vm_offset_t	pa,
					int		attrib);
