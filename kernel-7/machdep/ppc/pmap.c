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
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * Copyright (c) 1991,1987 Carnegie Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that all advertising materials mentioning features or use of
 * this software display the following acknowledgement: ``This product
 * includes software developed by the Center for Software Science at
 * the University of Utah.''
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * Carnegie Mellon requests users of this software to return to
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * 	Utah $Hdr: pmap.c 1.28 92/06/23$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSS, 10/90
 */
 
/*
 *	Manages physical address maps for powerpc.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information to
 *	when physical maps must be made correct.
 *	
 */

/*
 * CAVAETS:
 *
 *	Needs more work for MP support
 */

#include <debug.h>
#include <mach_vm_debug.h>

#include <kern/thread.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <kernserv/machine/spl.h>

#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/mem.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>
#include <machdep/ppc/powermac.h>


#define NULL 0
#define DPRINTF(x) if(0) {kprintf("%s : ", __FUNCTION__);kprintf x;}

/* forward */
void pmap_activate(pmap_t pmap, thread_t th, int which_cpu);
void pmap_deactivate(pmap_t pmap, thread_t th, int which_cpu);
void copy_to_phys(vm_offset_t sva, vm_offset_t dpa, int bytecount);

static struct mapping *pmap_find_mapping(space_t space, vm_offset_t offset);

static void pmap_free_mapping(register struct mapping *mp);

static void pmap_reap_mappings(void);

static struct mapping *pmap_enter_mapping(pmap_t pmap,
					  space_t space,
					  vm_offset_t va,
					  vm_offset_t pa,
					  pte_t *pte,
					  unsigned prot,
					  struct phys_entry *pp);

#if DEBUG
#define PDB_USER	0x01	/* exported functions */
#define PDB_MAPPING	0x02	/* low-level mapping routines */
#define PDB_ENTER	0x04	/* pmap_enter specifics */
#define PDB_COPY	0x08	/* copy page debugging */
#define PDB_ZERO	0x10	/* zero page debugging */
#define PDB_WIRED	0x20	/* things concerning wired entries */
#define PDB_PTEG	0x40	/* PTEG overflows */
#define PDB_MASSIVE	0x80	/* Massive costly assert checks */
#define PDB_IO		0x100	/* Improper use of WIMG_IO checks - PCI machines */
int pmdebug = 0;

#define PCI_BASE	0x80000000
#endif

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
struct zone	*pmap_zone;		/* zone of pmap structures */
boolean_t	pmap_initialized = FALSE;

#define		HASH_TABLE_FACTLOG2	0
int		hash_table_factlog2 = HASH_TABLE_FACTLOG2;

/*
 * Physical-to-virtual translations are handled by inverted page table
 * structures, phys_tables.  Multiple mappings of a single page are handled
 * by linking the affected mapping structures. We initialise one region
 * for phys_tables of the physical memory we know about, but more may be
 * added as it is discovered (eg. by drivers).
 */
struct phys_entry *phys_table;	/* For debugging */

/*
 * XXX use mpqueue_head_t's??
 * (no one else does, are they coming or going?)
 */
queue_head_t		free_mapping;	   /* list of free mapping structs */
decl_simple_lock_data(,free_mapping_lock) /* and lock */

pmap_t			free_pmap;	   /* list of free pmaps */
decl_simple_lock_data(,free_pmap_lock)   /* and lock */

decl_simple_lock_data(,pmap_lock)	   /* XXX this is all broken */

unsigned	prot_bits[8];

/*
 * This is the master space ID counter, initially set to the kernel's
 * SID. The counter is incremented (though not necessarily by one)
 * whenever a new space ID is required. For PPC, the SID ranges from
 * to 2**20 - 1.
 */
int		sid_counter=PPC_SID_KERNEL;	/* seed for SID generator */



/*
 * pmap_find_physentry(pa)
 *
 * Function to get index into phys_table for a given physical address
 */
struct phys_entry *
pmap_find_physentry(
	vm_offset_t pa)
{
	int i;
	struct phys_entry *entry;

	for (i = pmap_mem_regions_count-1; i >= 0; i--) {
		if (pa < pmap_mem_regions[i].start)
			continue;
		if (pa >= pmap_mem_regions[i].end)
		{
			return PHYS_NULL;
		}
		
		entry = &pmap_mem_regions[i].
		    phys_table[(pa -
				pmap_mem_regions[i].start) >> PPC_PGSHIFT];
		return entry;
	}
	DPRINTF(("NMGS DEBUG : pmap_find_physentry 0x%08x out of range\n",pa));
	return (struct phys_entry *)PHYS_NULL;
}



/*
 * kern_return_t
 * pmap_add_physical_memory(vm_offset_t spa, vm_offset_t epa,
 *                          boolean_t available, unsigned int attr)
 *	Allocate some extra physentries for the physical addresses given,
 *	specifying some default attribute that on the powerpc specifies
 *      the default cachability for any mappings using these addresses
 *	If the memory is marked as available, it is added to the general
 *	VM pool, otherwise it is not (it is reserved for card IO etc).
 */
kern_return_t
pmap_add_physical_memory(
	vm_offset_t spa,
	vm_offset_t epa,
	boolean_t available,
	unsigned int attr)
{
	int i,j;
	spl_t s;
	struct phys_entry *phys_table;

	/* Only map whole pages */

	spa = trunc_page(spa);
	epa = round_page(epa);

	/* First check that the region doesn't already exist */

	assert (epa >= spa);
	for (i = 0; i < pmap_mem_regions_count; i++) {
		/* If we're below the next region, then no conflict */
		if (epa <= pmap_mem_regions[i].start)
			break;
		if (spa < pmap_mem_regions[i].end) {
#if DEBUG
			DPRINTF(("(0x%08x,0x%08x,0x%08x) - memory already present\n",spa,epa,attr));
#endif /* DEBUG */
			return KERN_NO_SPACE;
		}
	}

	/* Check that we've got enough space for another region */
	if (pmap_mem_regions_count == MEM_REGION_MAX)
		return KERN_RESOURCE_SHORTAGE;

	/* Once here, i points to the mem_region above ours in physical mem */

	/* allocate a new phys_table for this new region */

	phys_table =  (struct phys_entry *)
		kalloc(sizeof(struct phys_entry) * atop(epa-spa));

	/* Initialise the new phys_table entries */
	for (j = 0; j < atop(epa-spa); j++) {
		queue_init(&phys_table[j].phys_link);
		/* We currently only support these two attributes */
		assert((attr == PTE_WIMG_DEFAULT) ||
		       (attr == PTE_WIMG_IO));
		phys_table[j].pte1.bits.wimg = attr;
	}
	s = splhigh();
	
	/* Move all the phys_table entries up some to make room in
	 * the ordered list.
	 */
	for (j = pmap_mem_regions_count; j > i ; j--)
		pmap_mem_regions[j] = pmap_mem_regions[j-1];

	/* Insert a new entry with some memory to back it */

	pmap_mem_regions[i].start 	     = spa;
	pmap_mem_regions[i].end           = epa;
	pmap_mem_regions[i].phys_table    = phys_table;

	pmap_mem_regions_count++;
	splx(s);

	if (available) {
		DPRINTF(("warning : pmap_add_physical_mem() "
		       "available not yet supported\n"));
	}

	return KERN_SUCCESS;
}



/*
 * ppc_protection_init()
 *	Initialise the user/kern_prot_codes[] arrays which are 
 *	used to translate machine independent protection codes
 *	to powerpc protection codes. The PowerPc can only provide
 *      [no rights, read-only, read-write]. Read implies execute.
 * See PowerPC 601 User's Manual Table 6.9
 */
void
ppc_protection_init(void)
{
	prot_bits[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_NONE]     = 0;
	prot_bits[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_NONE]     = 3;
	prot_bits[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_EXECUTE]  = 3;
	prot_bits[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_EXECUTE]  = 3;
	prot_bits[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE]     = 2;
	prot_bits[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE]     = 2;
	prot_bits[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE]  = 2;
	prot_bits[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE]  = 2;
}



/*
 * pmap_map(va, spa, epa, prot)
 *	is called during boot to map memory in the kernel's address map.
 *	A virtual address range starting at "va" is mapped to the physical
 *	address range "spa" to "epa" with machine independent protection
 *	"prot".
 *
 *	"va", "spa", and "epa" are byte addresses and must be on machine
 *	independent page boundaries.
 */
vm_offset_t
pmap_map(
	vm_offset_t va,
	vm_offset_t spa,
	vm_offset_t epa,
	vm_prot_t prot)
{

	if (spa == epa)
		return(va);

	assert(epa > spa);

	while (spa < epa) {
		pmap_enter(kernel_pmap, va, spa, prot, TRUE);

		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}
	return(va);
}



/*
 * pmap_map_bd(va, spa, epa, prot)
 *	Back-door routine for mapping kernel VM at initialisation.
 *	Used for mapping memory outside the known physical memory
 *      space, with caching disabled. Designed for use by device probes.
 * 
 *	A virtual address range starting at "va" is mapped to the physical
 *	address range "spa" to "epa" with machine independent protection
 *	"prot".
 *
 *	"va", "spa", and "epa" are byte addresses and must be on machine
 *	independent page boundaries.
 *
 * WARNING: The current version of memcpy() can use the dcbz instruction
 * on the destination addresses.  This will cause an alignment exception
 * and consequent overhead if the destination is caching-disabled.  So
 * avoid memcpy()ing into the memory mapped by this function.
 *
 * also, many other pmap_ routines will misbehave if you try and change
 * protections or remove these mappings, they are designed to be permanent.
 */
vm_offset_t
pmap_map_bd(
	vm_offset_t va,
	vm_offset_t spa,
	vm_offset_t epa,
	vm_prot_t prot)
{
	pte_t  *pte;
	struct phys_entry* pp;
	struct mapping *mp;
	spl_t s;

	if (spa == epa)
		return(va);

	assert(epa > spa);

#if DEBUG
	DPRINTF(("va=0x%08X, spa=0x%08X, epa=0x%08X, prot=0x%x\n\n",va,spa,epa,prot));
#endif
	s = splhigh();
	while (spa < epa) {
		
		pte = find_or_allocate_pte(PPC_SID_KERNEL, va, TRUE);

		assert(pte != PTE_NULL);

		/* remapping of already-mapped addresses is OK, we just
		 * trample on the PTE, but make sure we're mapped to same
		 * address
		 */
		if (pte->pte0.bits.valid)
			assert(trunc_page(pte->pte1.word) == spa);

		/*	Perform a general case PTE update designed to work
			in SMP configurations. TODO - locks and tlbsync
		*/
		assert(pte->pte0.bits.valid == TRUE);
		pte->pte0.bits.valid = FALSE;	/* Invalidate pte entry */
		sync();			/* Force updates to complete */
		tlbie(va);		/* Wipe out this virtual address */
		eieio();		/* Enforce ordering of v bit */
		tlbsync();
		sync();

		/*	Allowable early PTE updates: RPN,R,C,WIMG,PP
		 */
		pte->pte1.bits.protection = prot_bits[prot];
		pte->pte1.bits.phys_page = spa >> PPC_PGSHIFT;
		pte->pte1.bits.wimg	  = PTE_WIMG_IO;
		eieio();

		/*	Post tlbie updates: VSID,H,API,V
		*/
		pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
		sync();


		/* If memory is mappable (is in phys table) then set
		 * default cach attributes to non-cached and verify
		 * that there aren't any non-cached mappings which would
		 * break the processor spec (and hang the machine).
		 */
		pp = pmap_find_physentry(spa);
		if (pp != PHYS_NULL) {
			/* Set the default mapping type */
			pp->pte1.bits.wimg = PTE_WIMG_IO;
#if DEBUG
			/* Belt and braces check on mappings */
			queue_iterate(&pp->phys_link, mp,
				      struct mapping *, phys_link) {
				assert(mp->pte->pte1.bits.wimg ==
				       PTE_WIMG_IO);
			}
#endif /* DEBUG */
		}
		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}
	splx(s);
	return(va);
}



/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *	Called with mapping done by BATs. Page_size must already be set.
 *
 *	Parameters:
 *	first_avail	PA of address where we can allocate structures.
 */
void
pmap_bootstrap(
	unsigned int mem_size,
	vm_offset_t *first_avail)
{
	struct mapping		*mp;
	vm_offset_t		struct_addr;
	vm_size_t		struct_size, hash_table_align, phys_table_size,
	    				mapping_table_size;
	unsigned int		mapping_table_num, phys_pages_num;
	int			i;

	*first_avail = round_page(*first_avail);

	assert(PAGE_SIZE == PPC_PGBYTES);

	ppc_protection_init();

	/*
	 * Initialize kernel pmap
	 */
	kernel_pmap = &kernel_pmap_store;
#if	NCPUS > 1
	lock_init(&pmap_lock, FALSE, ETAP_VM_PMAP_SYS, ETAP_VM_PMAP_SYS_I);
#endif	/* NCPUS > 1 */
	simple_lock_init(&kernel_pmap->lock);
	simple_lock_init(&free_pmap_lock);

	kernel_pmap->ref_count = 1;
	kernel_pmap->space = PPC_SID_KERNEL;
	kernel_pmap->next = PMAP_NULL;

	/*
	 * Allocate: (from first_avail up)
	 *      aligned to its own size:
         *        hash table (for mem size 2**x, allocate 2**(x-10) entries)
	 *	physical page entries (1 per physical page)
	 *	physical -> virtual mappings (for multi phys->virt mappings)
	 */

	/*
	 * Table 7-21 on page 7-52 of the PowerPC Programming
	 * Environments book (32-bit) doesn't tell you how to
	 * size the hashed page table for strange memory sizes
	 * (i.e. not a power of 2).  It has been empirically
	 * determined that splitting the difference and rounding
	 * can be used effectively in these circumstances.
	 */
	{
	    vm_size_t	rounded_mem_size;
	    int		factor;

	    /*
	     * The minimum size for the hash table is 64K
	     * and is bounded by a memory size of 8MB.
	     */
	    hash_table_size = 64*1024; rounded_mem_size = 8*1024*1024;
	    while (hash_table_size < 32*1024*104 &&
		   rounded_mem_size < mem_size) {
		hash_table_size *= 2; rounded_mem_size *= 2;
	    }

	    /*
	     * If we need to allocate more than a minimum
	     * sized hash table and our memory size is not
	     * a power of 2
	     *
	     * AND
	     *
	     * if we are less than half way to next higher
	     * power of 2, then use the next lower value.
	     */
	    if (hash_table_size > 64*1024 && rounded_mem_size != mem_size) {
		if (
		    /*
		     * Special case for mem_size
		     * greater than 2G since
		     * rounded_mem_size is zero
		     * in this case.
		     */
		    (mem_size > 2*1024*1024*1024UL &&
		     mem_size < 3*1024*1024*1024UL)
		    ||
		    (mem_size < ((rounded_mem_size / 2) +
				 (rounded_mem_size / 4)))
		    ) hash_table_size /= 2;
	    }

#ifdef notdef
	    printf("rounded_mem_size %x\n", rounded_mem_size);
#endif

	    /*
	     * Determine the value of any additional
	     * factor to apply to the size of the
	     * hash table.
	     */
	    if (hash_table_factlog2 < 0)
		factor = -1;
	    else
		factor = 1;
	    
	    hash_table_factlog2 *= factor;

	    while (hash_table_factlog2 > 0) {
		factor *= 2; hash_table_factlog2--;
	    }

	    if (factor > 0)
		printf("hash table factor is %d\n", factor);
	    else
		printf("hash table factor is 1/%d\n", -factor);

	    /*
	     * Apply the factor, and catch out
	     * of bound sizes.
	     */
	    if (factor > 1) {
		hash_table_size *= factor;

		if (hash_table_size < 64*1024)
		    hash_table_size = 64*1024;
	    }
	    else if (factor < -1) {
		hash_table_size /= -factor;

		if (hash_table_size > 32*1024*1024 || hash_table_size == 0)
		    hash_table_size = 32*1024*1024;
	    }

	    /*
	     * Notice a bit of handwaving here: since we
	     * use a BAT to statically map the kernel,
	     * the hashed page table and the ancillary
	     * mapping structures, using mem_size is a
	     * bit generous.  However, we do need to allow
	     * for some extra mappings for I/O regions, so
	     * we'll just call it even.
	     */
	}

	/* HASH TABLE MUST BE aligned to its size */

	struct_addr = (*first_avail + (hash_table_size - 1)) &~
	    					(hash_table_size - 1);

	hash_table_align = struct_addr - *first_avail;

	if (round_page(*first_avail) + PPC_PGBYTES < round_page(struct_addr)) {
		free_regions[free_regions_count].start = 
			round_page(*first_avail) + PPC_PGBYTES;
		free_regions[free_regions_count].end = round_page(struct_addr);
		free_regions_count++;
	}

	phys_pages_num = atop(mem_size);

	phys_table_size = sizeof(struct phys_entry) * phys_pages_num;
	mapping_table_size =
	    sizeof(struct mapping) * (hash_table_size/sizeof(pte_t));

	/* size of all structures that we're going to allocate */

	struct_size = (vm_size_t) (hash_table_size +
				   phys_table_size +
				   mapping_table_size);

#ifdef notdef
	printf("pmap static allocation:\n");
	printf("hash table: %x (alignment) + %x (size) +\n",
	       hash_table_align, hash_table_size);
	printf("ptov table: %x (phys entries) + %x (mappings) =\n",
	       phys_table_size, mapping_table_size);
	printf("total: %x\n", hash_table_align + struct_size);
#endif

	struct_size = round_page(struct_size);

	/* Zero everything - this also invalidates the hash table entries */
	bzero((char *)struct_addr, struct_size);

	/* Set up some pointers to our new structures */
	
	/*
	 * Set up hash table address, keeping alignment. These
	 * mappings are 1-1.
	 */
	hash_table_base = struct_addr;
	struct_addr += hash_table_size;

	/*
	 * phys_table is static to help debugging,
	 * this variable is no longer actually used
	 * outside of this scope
	 */
	phys_table = (struct phys_entry *) struct_addr;

	for (i = 0; i < pmap_mem_regions_count; i++) {
		pmap_mem_regions[i].phys_table = phys_table;
		phys_table = phys_table +
			atop(pmap_mem_regions[i].end -
			     pmap_mem_regions[i].start);
	}

	/* restore phys_table for debug */
	phys_table = (struct phys_entry *) struct_addr;
	struct_addr += phys_table_size;

	/* Initialise the registers necessary for supporting the hashtable */
	hash_table_init(hash_table_base, hash_table_size);

	/* Initialise the physical table mappings */ 
	for (i = 0; i < phys_pages_num; i++) {
		queue_init(&phys_table[i].phys_link);
		phys_table[i].pte1.bits.wimg = PTE_WIMG_DEFAULT;
	}

	/*
	 * Remaining space is for mapping entries.  Chain them
	 * together (XXX can't use a zone
	 * since zone package hasn't been initialized yet).
	 */
	mapping_table_num = mapping_table_size /
		sizeof(struct mapping);

	mp = (struct mapping *) struct_addr;

	queue_init(&free_mapping);
	simple_lock_init(&free_mapping_lock);

	while (mapping_table_num-- > 0) {
		queue_enter(&free_mapping, mp, struct mapping *, phys_link);
		mp++;
	}

	*first_avail = round_page(mp);

	/* All the rest of memory is free - add it to the free
	 * regions so that it can be allocated by pmap_steal
	 */
	free_regions[free_regions_count].start = *first_avail;
	free_regions[free_regions_count].end = pmap_mem_regions[0].end;
	free_regions_count++;
}



/*
 * pmap_init()
 *	finishes the initialization of the pmap module.
 *	This procedure is called from vm_mem_init() in vm/vm_init.c
 *	to initialize any remaining data structures that the pmap module
 *	needs to map virtual memory (VM is already ON).
 */
/*ARGSUSED*/
void
pmap_init(void)
{
	vm_size_t s;

	s = sizeof(struct pmap);
	pmap_zone = zinit(s, 400*s, 4096, FALSE, "pmap");	/* XXX */

	pmap_initialized = TRUE;
}


/*
 * pmap_create
 *
 * Create and return a physical map.
 *
 * If the size specified for the map is zero, the map is an actual physical
 * map, and may be referenced by the hardware.
 *
 * If the size specified is non-zero, the map will be used in software 
 * only, and is bounded by that size.
 */
pmap_t
pmap_create(
	vm_size_t size)
{
	pmap_t pmap;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(size=%x)%c", size, size ? '\n' : ' '));
#endif

	/*
	 * A software use-only map doesn't even need a pmap structure.
	 */
	if (size)
		return(PMAP_NULL);

	/* 
	 * If there is a pmap in the pmap free list, reuse it. 
	 */
	s = splhigh();
	simple_lock(&free_pmap_lock);
	if (free_pmap != PMAP_NULL) {
		pmap = free_pmap;
		free_pmap = pmap->next;
	} else
		pmap = PMAP_NULL;
	simple_unlock(&free_pmap_lock);

	/*
	 * Couldn't find a pmap on the free list, try to allocate a new one.
	 */
	if (pmap == PMAP_NULL) {
		pmap = (pmap_t) zalloc(pmap_zone);
		if (pmap == PMAP_NULL)
		{
			splx(s);
			return(PMAP_NULL);
		}

		/*
		 * Allocate space IDs for the pmap.
		 * If all are allocated, there is nothing we can do.
		 */

		/* If sid_counter == MAX_SID, we've allocated
		 * all the possible SIDs and they're all live,
		 * we can't carry on, but this is HUGE, so don't
		 * expect it in normal operation.
		 */
		assert(sid_counter != SID_MAX);

		/* Try to spread out the sid's through the possible
		 * name space to improve the hashing heuristics
		 */
#warning Get a better VSID allocation algorithm from the AIX folks.
/*		sid_counter = (sid_counter + PPC_SID_PRIME) & PPC_SID_MASK; */
		sid_counter ++;	/* Replaced by a simpler algorithm */


		/* 
		 * Initialize the sids
		 */
		pmap->space = sid_counter;
		simple_lock_init(&pmap->lock);
	}
	pmap->ref_count = 1;
	pmap->next = PMAP_NULL;
	pmap->stats.resident_count = 0;
	pmap->stats.wired_count = 0;
#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("-> %x, space id = %d\n", pmap, pmap->space));
#endif
	splx(s);
	return(pmap);
}



/* 
 * pmap_destroy
 * 
 * Gives up a reference to the specified pmap.  When the reference count 
 * reaches zero the pmap structure is added to the pmap free list.
 *
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(
	pmap_t pmap)
{
	int ref_count;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x)\n", pmap));
#endif

	if (pmap == PMAP_NULL)
		return;

	s = splhigh();
	simple_lock(&pmap->lock);
	ref_count = --pmap->ref_count;
	simple_unlock(&pmap->lock);

	if (ref_count < 0)
		panic("pmap_destroy(): ref_count < 0");
	if (ref_count > 0)
	{
		splx(s);
		return;
	}

#if DEBUG
	if (pmap->stats.resident_count != 0 || pmap->stats.wired_count != 0) {
		kprintf("pmap_destroy: non_empty pmap\n");
	}
#endif	/* DEBUG */

	/* 
	 * Add the pmap to the pmap free list. 
	 */
	simple_lock(&free_pmap_lock);
	pmap->next = free_pmap;
	free_pmap = pmap;
	simple_unlock(&free_pmap_lock);
	splx(s);
}



/*
 * pmap_reference(pmap)
 *
 *	gains a reference to the specified pmap.
 *
 *	This is used to indicate that the pmap is in use by multiple
 *	machine-independent maps and will prevent early deallocation
 *	of the pmap.
 */
void
pmap_reference(
	pmap_t pmap)
{
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x)\n", pmap));
#endif

	if (pmap != PMAP_NULL) {
		s = splhigh();
		simple_lock(&pmap->lock);
		pmap->ref_count++;
		simple_unlock(&pmap->lock);
		splx(s);
	}
}

/*
 * pmap_remove(pmap, s, e)
 *	unmaps all virtual addresses v in the virtual address
 *	range determined by [s, e) and pmap.
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_remove(
	pmap_t pmap,
	vm_offset_t sva,
	vm_offset_t eva)
{
	space_t space;
	struct mapping *mp;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x, sva=%x, eva=%x)\n",
		       pmap, sva, eva));
#endif

	if (pmap == PMAP_NULL)
		return;

	s = splhigh();
	space = pmap->space;

	/* It is just possible that eva might have wrapped around to zero,
	 * and sometimes we get asked to liberate something of size zero
	 * even though it's dumb (eg. after zero length read_overwrites)
	 */
	assert(eva >= sva);

	/* We liberate addresses from high to low, since the stack grows
	 * down. This means that we won't need to test addresses below
	 * the limit of stack growth
	 */
	while ((pmap->stats.resident_count > 0) && (eva > sva)) {
		eva -= PAGE_SIZE;
		if ((mp = pmap_find_mapping(space, eva)) != MAPPING_NULL) {
			pmap->stats.resident_count--;
			pmap_free_mapping(mp);
		}
	}
	splx(s);
}



/*
 *	Routine:
 *		pmap_page_protect
 *
 *	Function:
 *		Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(
	vm_offset_t pa,
	vm_prot_t prot)
{
	register struct phys_entry *pp;
	register struct mapping *mp;
	register struct mapping *mp2;
	unsigned pteprot;
	boolean_t remove;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pa=%x, prot=%x)\n", pa, prot));
#endif

	switch (prot) 
	{
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		remove = FALSE;
		break;
	case VM_PROT_ALL:
		return;
	default:
		remove = TRUE;
		break;
	}

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	s = splhigh();
	if (remove) 
	{
		mp2 = MAPPING_NULL;
		while (!queue_empty(&pp->phys_link)) 
		{
			mp = (struct mapping *) queue_first(&pp->phys_link);
			if (mp->vm_info.bits.phys) 
			{
				// Don't zap the phys page
				mp2 = mp;
				queue_remove(&pp->phys_link, mp2, struct mapping *, phys_link);
			} else 
			{
				mp->pmap->stats.resident_count--;
#if	DEBUG
				if (mp->vm_info.bits.wired) 
				{
					DPRINTF((": removing WIRED page!!\n"));
				}
#endif	/* DEBUG */
				pmap_free_mapping(mp);
			}
		}
#if	DEBUG
		if (queue_empty(&pp->phys_link) && (mp2 == MAPPING_NULL))
		{
			DPRINTF(("WHOOPS! removed and entire map\n"));
		}
#endif	/* DEBUG */

		if (mp2 != MAPPING_NULL) 
		{
		    // Restore the phys page
		    queue_enter_first(&pp->phys_link, mp2, struct mapping *, phys_link);
		}
		splx(s);
		return;
	}
	
	/*
	 * Modify mappings if necessary.
	 */
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) 
	{
		/*	Compare the new protection bits with the old
			one to see if anything needs to be changed.
			Note: In mach 2.5, we must not reduce the 
			protection status of the static logical mapping
			(the phys page), but we *do* want to record RC
			bits.
		*/
		if (mp->vm_info.bits.phys) 
		{
			// Phys page mapping - don't alter protection
			// Do not merge RC bits into pp.
			continue;
		}

		pteprot = prot_bits[prot];
		if ((mp->pte->pte1.bits.protection) != pteprot)
		{
			/*
			 * Perform a sync to force saving
			 * of change/reference bits, followed by a
			 * fault and reload with the new protection.
			 */
			assert(mp->pte->pte0.bits.valid == TRUE);
			mp->pte->pte0.bits.valid	= FALSE;
			sync();
			tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
			eieio();
			tlbsync();
			sync();

			mp->pte->pte1.bits.protection = pteprot;
			eieio();

			mp->pte->pte0.bits.valid	= TRUE;
			sync();
		}
		//pp->pte1.word |= mp->pte->pte1.word;
		pp->pte1.bits.protection |= mp->pte->pte1.bits.protection;
	}
	splx(s);
}



/*
 * pmap_protect(pmap, s, e, prot)
 *	changes the protection on all virtual addresses v in the 
 *	virtual address range determined by [s, e] and pmap to prot.
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_protect(
	pmap_t pmap,
	vm_offset_t sva, 
	vm_offset_t eva,
	vm_prot_t prot)
{
	register struct mapping *mp;
	pte_t  *pte;
	unsigned pteprot;
	space_t space;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x, sva=%x, eva=%x, prot=%x)\n",
		       pmap, sva, eva, prot));
#endif

	if (pmap == PMAP_NULL)
		return;

	assert(sva <= eva);

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
#if MACH_30
	if (prot & VM_PROT_WRITE)
		return;
#endif

	s = splhigh();
	space = pmap->space;
	for ( ; sva < eva; sva += PAGE_SIZE) {
		mp = pmap_find_mapping(space, sva);
		if (mp == MAPPING_NULL)
			continue;
#if DEBUG
		if (pmap != mp->pmap)
			panic("protect: pmap mismatch");
#endif
		/*
		 * Determine if mapping is changing.
		 * If not, nothing to do.
		 */
		pte 	= mp->pte;
		pteprot = prot_bits[prot];
		if (pte->pte1.bits.protection == pteprot)
			continue;
		
		/*
		 * Purge the current TLB entry (if any) to force
		 * any modifications to changed/referenced bits, and so
		 * that future references will fault and reload with the
		 * new protection.
		 */

		/*	Perform a general case PTE update designed to work
			in SMP configurations. TODO - locks
		*/
		assert(pte->pte0.bits.valid == TRUE);
		pte->pte0.bits.valid = FALSE;	/* Invalidate pte entry */
		sync();				/* Force updates to complete */
		tlbie(sva);
		eieio();
		tlbsync();
		sync();

		/*	Allowable early PTE updates: RPN,R,C,WIMG,PP
		*/
		pte->pte1.bits.protection = pteprot;	/* Change just the protection */
		eieio();

		/*	Post tlbie updates: VSID,H,API,V
		*/
		pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
		sync();

	}
	splx(s);
}



void
pmap_enter_phys_page(
	vm_offset_t pa,
	vm_offset_t va)
{
    struct mapping *mp;

    pmap_enter(kernel_pmap, va, pa, VM_PROT_READ|VM_PROT_WRITE, TRUE);

    if (mp = pmap_find_mapping(kernel_pmap->space, va)) {
	mp->vm_info.bits.phys = TRUE;
    }
#if	DEBUG
    else
    {
	DPRINTF(("pmap_enter_phys_page: mp == NULL\n"));
	DPRINTF(("pmap_enter_phys_page: mp == NULL\n"));
	panic("pmap_enter_phys_page: can't find mapping entry");
    }
#endif	/* DEBUG */
}



int pmap_enter_cnt = 0;
int pmap_enter_valid_cnt = 0;
int pmap_enter_invalid_cnt = 0;
/*
 * pmap_enter
 *
 * Create a translation for the virtual address (virt) to the physical
 * address (phys) in the pmap with the protection requested. If the
 * translation is wired then we can not allow a page fault to occur
 * for this mapping.
 *
 * NB: This is the only routine which MAY NOT lazy-evaluate
 *     or lose information.  That is, this routine must actually
 *     insert this page into the given map NOW.
 */
void
pmap_enter(
	pmap_t pmap,
	vm_offset_t va,
	vm_offset_t pa,
	vm_prot_t prot,
	boolean_t wired)
{
	register struct mapping *mp;
	struct phys_entry *pp,*old_pp;
	pte_t *pte;
	space_t space;
	boolean_t was_wired;
	spl_t s;

#if DEBUG
	if ((wired && (pmdebug & (PDB_WIRED))) ||
	     ((pmdebug & (PDB_USER|PDB_ENTER)) == (PDB_USER | PDB_ENTER)))
		DPRINTF(("(pmap=%x, va=%x, pa=%x, prot=%x, wire=%x)\n", pmap, va, pa, prot, wired));
	pmap_enter_cnt++;
#endif
        if (pmap == PMAP_NULL)
                return;
#if DEBUG
	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, va, va + PAGE_SIZE);
		return;
	}
#endif	/* DEBUG */

	/* TODO NMGS - take lock on physentry? */
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	/* Find the pte associated with the virtual address/space ID */
	space = pmap->space;

	/* TODO NMGS - take lock on pte?! */
	pte = find_or_allocate_pte(space,va,TRUE);

	assert(pte != PTE_NULL);
	/* Even if the pte is new, pte0 should be set up for the new mapping,
	 * but with the valid bit set to FALSE
	 */
	assert((pte->pte0.word & 0x7fffffff) != PTE_EMPTY);

	s = splhigh();
	if (pte->pte0.bits.valid == TRUE) {

		/* mapping is already valid, thus virtual address was
		 * already mapped somewhere...
		 */

#if	DEBUG
		pmap_enter_valid_cnt++;
#endif	/* DEBUG */
		/*
		**  invalidate tlb for previous mapping 
		*/
		pte->pte0.bits.valid = FALSE;/* Invalidate pte entry */
		sync();		/* Force updates to complete */
		tlbie(va);	/* Wipe out this virtual address */
		eieio();	/* Enforce ordering of v bit */
		tlbsync();
		sync();

		if (((physical_addr_t *) &pa)->bits.page_no !=
	    	    pte->pte1.bits.phys_page) {

			/* The current mapping is to a different
			 * physical addr - release the mapping
			 * before mapping the new address. 
			 */

#if DEBUG
			if ((pmdebug & (PDB_USER|PDB_ENTER)) == (PDB_USER|PDB_ENTER))
				DPRINTF(("Virt 0x%08x was mapped to different address 0x%08x, changing mapping\n",va,((physical_addr_t *) &pa)->bits.page_no * PPC_PGBYTES));
#endif

			/* Take advantage of the fact that pte1 can be
			 * considered as a pointer to an address in
			 * the physical page when looking for physentry.
			 */

			old_pp = pmap_find_physentry(
					     (vm_offset_t)(pte->pte1.word));

			queue_iterate(&old_pp->phys_link, mp,
				      struct mapping *, phys_link) {
				if (mp->pte == pte)
					break;
			}

			/* pte entry must have good reverse mapping */
			assert(mp != NULL);
			assert(mp->pte == pte);
			assert(((mp->vm_info.bits.page >> 10) & 0x3f) ==
			       mp->pte->pte0.bits.page_index);

			/* Wired/not wired is taken care of later on
			 * in this function. We can remove wired mappings.
			 */

			/* Do the equivalent of pmap_free_mapping followed
			 * by a pmap_enter_mapping, recycling the pte and
			 * the mapping
			 */

			/* keep track of changed and referenced at old
			 * address
			 */
			old_pp->pte1.word |= mp->pte->pte1.word;

			queue_remove(&old_pp->phys_link, mp,
				     struct mapping *, phys_link);

			mp->vm_info.bits.page  = (va >> PPC_PGSHIFT);

			assert(mp->pte == pte);

			queue_enter_first(&pp->phys_link, mp,
					  struct mapping *, phys_link);


			assert(mp->pmap == pmap);

			/* Set up pte as it would have been done by
			 * find_or_allocate_pte() + pmap_enter_mapping()
			 */

			/*	Allowable early PTE updates: RPN,R,C,WIMG,PP
			*/
			pte->pte1.word = 0;
			pte->pte1.bits.phys_page  = pa >> PPC_PGSHIFT;
			pte->pte1.bits.wimg 	  = pp->pte1.bits.wimg; /* default */

	
			/*	Post tlbie updates: VSID,H,API,V
			*/
			pte->pte0.bits.segment_id = (space << 4) | (va >> 28);
			pte->pte0.bits.page_index = ((va_abbrev_t *)&va)->page_index;
			eieio();


		} else {
			/* The current mapping is the same as the one
			 * we're being asked to make, find the mapping
			 * structure that goes along with it.
			 */
			queue_iterate(&pp->phys_link, mp,
				      struct mapping *, phys_link) {
				if (mp->pte == pte)
					break;
			}
			/* mapping struct must exist */
			assert(mp != NULL);
			assert(mp->pte == pte);


#if DEBUG


			if ((wired && (pmdebug & (PDB_WIRED))) ||
			    ((pmdebug & (PDB_USER|PDB_ENTER)) == (PDB_USER|PDB_ENTER))) {
				DPRINTF(("address already mapped, changing prots or wiring\n"));
				if (pte->pte1.bits.protection !=
				    prot_bits[prot])
					DPRINTF(("Changing PROTS\n"));
				if (wired != mp->vm_info.bits.wired)
				       DPRINTF(("changing WIRING\n"));
			}
#endif
		}
	} else {

#if DEBUG
		pmap_enter_invalid_cnt++;
#endif	/* DEBUG */

		/* There was no pte match, a new entry was created
                 * (marked as invalid). We make sure that a new mapping
		 * structure is allocated as it's partner.
		 */
		mp = MAPPING_NULL;
		assert(pte->pte0.word != PTE_EMPTY);
		sync();		/* Force updates to complete */
		tlbie(va);	/* Wipe out this virtual address */
		eieio();	/* Enforce ordering of v bit */
		tlbsync();
		sync();

		pte->pte1.bits.protection = prot_bits[prot];
		eieio();
	}

	/* Once here, pte contains a valid pointer to a structure
	 * that we may use to map the virtual address (which may
	 * already map this (and be valid) or something else (invalid)
	 * and mp contains either MAPPING_NULL (new mapping) or
	 * a pointer to the old mapping which may need it's
	 * privileges modified.
	 */

	/* set up the protection bits on new mapping
	 */


	if (mp == MAPPING_NULL) {
		/*
		 * Mapping for this virtual address doesn't exist.
		 * Get a mapping structure and, and fill it in,
		 * updating the pte that we already have, and
		 * making it valid
		 */
		mp = pmap_enter_mapping(pmap, space, va, pa, pte, prot, pp);
		pmap->stats.resident_count++;
		was_wired = FALSE;/* indicate that page was never wired */
	} else {
		
		/*
		 * We are just changing the protection of a current mapping.
		 */
		was_wired = mp->vm_info.bits.wired;

		/*
		 * This is the pte that came in valid
		 */
		pte->pte1.bits.protection = prot_bits[prot];
		eieio();
		pte->pte0.bits.valid = TRUE;		/* !!! */
		sync();
	}

	assert(pte->pte0.bits.valid == TRUE);

	/* if wired status has changed, update stats and change bit */
	if (was_wired != wired) {
		pmap->stats.wired_count += wired ? 1 : -1;
		if (wired && pmap->stats.wired_count == 0)
			panic("pmap_enter: wired_count");
		mp->vm_info.bits.wired = wired;
#if DEBUG
		if (pmdebug & (PDB_WIRED))
			DPRINTF(("changing wired status to : %s\n",
			       wired ? "TRUE" : "FALSE"));
#endif /* DEBUG */
	}


	/* Belt and braces check to make sure we didn't give a bogus map,
	 * this map is given once when mapping the kernel
	 */
	assert((pte->pte1.bits.phys_page != 0) ||
	       (kgdb_kernel_in_pmap == FALSE));
	
#if DEBUG

	/*	Assert a PTE of type WIMG_IO is not mapped to general RAM.
	*/
	if (pmdebug & PDB_IO 	&&
	    pte->pte1.bits.wimg == PTE_WIMG_IO)
		assert (((unsigned) pa) >= PCI_BASE)

	/* Assert that there are no other mappings with different
	 * cachability information, as this can freeze the machine
	 */
	{
		struct mapping *mp2;
		queue_iterate(&pp->phys_link, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->pte->pte1.bits.wimg ==
			       pte->pte1.bits.wimg);
		}
	}

	if (pmdebug & PDB_MASSIVE) {
		register struct mapping *mp2;
		queue_iterate(&free_mapping, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->phys_link.next != NULL);
		}
	}
#endif /* DEBUG */	
	splx(s);
#if DEBUG
	if ((pmdebug & (PDB_USER|PDB_ENTER)) == (PDB_USER | PDB_ENTER))
		DPRINTF(("leaving pmap_enter\n"));
#endif
}



/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 *
 * Change the wiring for a given virtual page. This routine currently is 
 * only used to unwire pages and hence the mapping entry will exist.
 */
void
pmap_change_wiring(
	register pmap_t	pmap,
	vm_offset_t	va,
	boolean_t	wired)
{
	register struct mapping *mp;
	boolean_t waswired;
	spl_t s;

#if DEBUG
	if ((pmdebug & (PDB_USER | PDB_WIRED)) == (PDB_USER|PDB_WIRED))
		DPRINTF(("(pmap=%x, va=%x, wire=%x)\n",
		       pmap, va, wired));
#endif
	if (pmap == PMAP_NULL)
		return;

	s = splhigh();
	if ((mp = pmap_find_mapping(pmap->space, va)) == MAPPING_NULL)
		panic("pmap_change_wiring: can't find mapping entry");

	waswired = mp->vm_info.bits.wired;
	if (wired && !waswired) {
		mp->vm_info.bits.wired = TRUE;
		if (++pmap->stats.wired_count == 0)
			panic("pmap_change_wiring: wired_count");
	} else if (!wired && waswired) {
		mp->vm_info.bits.wired = FALSE;
		pmap->stats.wired_count--;
	}
	splx(s);
}



/*
 * pmap_extract(pmap, va)
 *	returns the physical address corrsponding to the 
 *	virtual address specified by pmap and va if the
 *	virtual address is mapped and 0 if it is not.
 */
vm_offset_t
pmap_extract(
	pmap_t pmap,
	vm_offset_t va)
{
	pte_t *pte;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x, va=%x)\n", pmap, va));
#endif

	if (pmap == PMAP_NULL)
	{
		DPRINTF((": pmap == PMAP_NULL\n"));
		return 0;
	}

	if (pmap == kernel_pmap) {
	    extern vm_offset_t virtual_avail;

	    if (va < virtual_avail)
		return va;
	}

	s = splhigh();
	pte = find_or_allocate_pte(pmap->space, trunc_page(va), FALSE);
	if (pte == NULL)
	{
		splx(s);
		return(0);
	}
	splx(s);
	return trunc_page(pte->pte1.word) | (va & page_mask);
}



/*
 *	pmap_attributes:
 *
 *	Set/Get special memory attributes
 *
 */
kern_return_t
pmap_attribute(
	pmap_t		pmap,
	vm_offset_t	address,
	vm_size_t	size,
	vm_machine_attribute_t	attribute,
	vm_machine_attribute_val_t* value)	
{
	vm_offset_t 	sva, eva;
	vm_offset_t	addr;
	kern_return_t	ret;

	if (attribute != MATTR_CACHE)
		return KERN_INVALID_ARGUMENT;

	/* We can't get the caching attribute for more than one page
	 * at a time
	 */
	if ((*value == MATTR_VAL_GET) &&
	    (trunc_page(address) != trunc_page(address+size-1)))
		return KERN_INVALID_ARGUMENT;

	if (pmap == PMAP_NULL)
		return KERN_SUCCESS;

	sva = trunc_page(address);
	eva = round_page(address + size);
	ret = KERN_SUCCESS;

	simple_lock(&pmap->lock);

	switch (*value) {
	case MATTR_VAL_CACHE_FLUSH:	/* flush from all caches */
	case MATTR_VAL_DCACHE_FLUSH:	/* flush from data cache(s) */
	case MATTR_VAL_ICACHE_FLUSH:	/* flush from instr cache(s) */
		sva = trunc_page(sva);
		for (; sva < eva; sva += PAGE_SIZE) {
			if ((addr = pmap_extract(pmap, sva)) == 0)
				continue;
			flush_cache(addr, PAGE_SIZE);
		}
		break;

	case MATTR_VAL_GET:		/* return current value */
	case MATTR_VAL_OFF:		/* turn attribute off */
	case MATTR_VAL_ON:		/* turn attribute on */
	default:
		ret = KERN_INVALID_ARGUMENT;
		break;
	}
	simple_unlock(&pmap->lock);

	return ret;
}



/*
 * pmap_collect
 * 
 * Garbage collects the physical map system for pages that are no longer used.
 */
/* ARGSUSED */
void
pmap_collect(
	pmap_t pmap)
{
#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x)\n", pmap));
#endif
}



/*
 *	Routine:	pmap_activate
 *	Function:
 *		Binds the given physical map to the given
 *		processor, and returns a hardware map description.
 */
/* ARGSUSED */
void
pmap_activate(
	pmap_t pmap,
	thread_t th,
	int which_cpu)
{
#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x, th=%x, cpu=%x)\n",
		       pmap, th, which_cpu));
#endif
}



/* ARGSUSED */
void
pmap_deactivate(
	pmap_t pmap,
	thread_t th,
	int which_cpu)
{
#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pmap=%x, th=%x, cpu=%x)\n",
		       pmap, th, which_cpu));
#endif
}



#if DEBUG

/*
 * pmap_zero_page
 * pmap_copy_page
 * 
 * are implemented in movc.s, these
 * are just wrappers to help debugging
 */

extern void pmap_zero_page_assembler(vm_offset_t p);
extern void pmap_copy_page_assembler(vm_offset_t src, vm_offset_t dst);



/*
 * pmap_zero_page(pa)
 *
 * pmap_zero_page zeros the specified (machine independent) page pa.
 */
void
pmap_zero_page(
	vm_offset_t p)
{

	if ((pmdebug & (PDB_USER|PDB_ZERO)) == (PDB_USER|PDB_ZERO))
		DPRINTF(("(pa=%x)\n", p));

	/*
	 * XXX can these happen?
	 */
	if (pmap_find_physentry(p) == PHYS_NULL)
		panic("zero_page: physaddr out of range");

	/* TODO NMGS optimisation - if page has no mappings, set bit in
	 * physentry to indicate 'needs zeroing', then zero page in
	 * pmap_enter or pmap_copy_page if bit is set. This keeps the
	 * cache 'hot'.
	 */

	assert((p & 0xFFF) == 0);
	pmap_zero_page_assembler(p);
}



/*
 * pmap_copy_page(src, dst)
 *
 * pmap_copy_page copies the specified (machine independent)
 * page from physical address src to physical address dst.
 *
 * We need to invalidate the cache for address dst before
 * we do the copy. Apparently there won't be any mappings
 * to the dst address normally.
 */
void
pmap_copy_page(
	vm_offset_t src,
	vm_offset_t dst)
{

	if ((pmdebug & (PDB_USER|PDB_COPY)) == (PDB_USER|PDB_COPY))
		DPRINTF(("(spa=%x, dpa=%x)\n", src, dst));
	if (pmdebug & PDB_COPY)
		DPRINTF((": phys_copy(%x, %x, %x)\n",
		       src, dst, PAGE_SIZE));

	assert((src & 0xFFF) == 0);
	assert((dst & 0xFFF) == 0);
	pmap_copy_page_assembler(src, dst);
}
#endif /* DEBUG */



/*
 * pmap_pageable(pmap, s, e, pageable)
 *	Make the specified pages (by pmap, offset)
 *	pageable (or not) as requested.
 *
 *	A page which is not pageable may not take
 *	a fault; therefore, its page table entry
 *	must remain valid for the duration.
 *
 *	This routine is merely advisory; pmap_enter()
 *	will specify that these pages are to be wired
 *	down (or not) as appropriate.
 *
 *	(called from vm/vm_fault.c).
 */
/* ARGSUSED */
void
pmap_pageable(
	pmap_t		pmap,
	vm_offset_t	start,
	vm_offset_t	end,
	boolean_t	pageable)
{
#if DEBUG
	if ((pmdebug & (PDB_USER | PDB_WIRED)) == (PDB_USER|PDB_WIRED))
		DPRINTF(("(pmap=%x, sva=%x, eva=%x, pageable=%s)\n",
		       pmap, start, end, pageable ? "TRUE" : "FALSE"));
#endif
}



/*
 * pmap_clear_modify(phys)
 *	clears the hardware modified ("dirty") bit for one
 *	machine independant page starting at the given
 *	physical address.  phys must be aligned on a machine
 *	independant page boundary.
 */
void
pmap_clear_modify(
	vm_offset_t pa)
{
	register struct phys_entry *pp;
	register struct mapping *mp;
	pte_t	*pte;
	register vm_offset_t offset;
	spl_t s;
	boolean_t was_valid;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pa=%x)\n", pa));
#endif

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	s = splhigh();
	/*
	 * invalidate any pte entries and remove them from tlb
	 * TODO might it be faster to wipe all tlb entries outside loop? NO, rav.
	 */

	/* Remove any reference to modified to the physical page */
	pp->pte1.bits.changed = FALSE;

	/* mark PTE entries as having no modifies, and flush tlbs */
	queue_iterate(&pp->phys_link, mp,
		      struct mapping *, phys_link) {
		pte	= mp->pte;
		offset	= mp->vm_info.bits.page << PPC_PGSHIFT;

		was_valid = FALSE;
		if (pte->pte0.bits.valid == TRUE)	{
			/* Perform a general case PTE update designed to work
			** in SMP configurations. TODO - locks and tlbsync
			*/
			pte->pte0.bits.valid = FALSE;	/* Invalidate PTE */
			was_valid = TRUE;
		}
		sync();		/* Force updates to complete */
		tlbie(offset);	/* Wipe out this virtual address */
		eieio();	/* Enforce ordering of v bit */
		tlbsync();
		sync();
	
		/*	Allowable early PTE updates: RPN,R,C,WIMG,PP
		*/
		pte->pte1.bits.changed = FALSE;
		eieio();

		/*	Post tlbie updates: VSID,H,API,V
		*/
		if (was_valid == TRUE)
		{
			pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
		}
		sync();
	}

#if	DEBUG
	/* Assert that there are no other mappings with different
	 * cachability information, as this can freeze the machine
	 */
	{
		struct mapping *mp2;
		queue_iterate(&pp->phys_link, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->pte->pte1.bits.wimg ==
			       pte->pte1.bits.wimg);
		}
	}
#endif	/* DEBUG */

	splx(s);
}



/*
 * pmap_is_modified(phys)
 *	returns TRUE if the given physical page has been modified 
 *	since the last call to pmap_clear_modify().
 */
boolean_t
pmap_is_modified(
	vm_offset_t pa)
{
	struct phys_entry *pp;
	struct mapping *mp;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pa=%x)\n", pa));
#endif
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);

	/* Check to see if we've already noted this as modified */
	if (pp->pte1.bits.changed)
		return(TRUE);

	s = splhigh();
	/*	Make sure all TLB -> PTE updates have completed.
	*/
	sync();

	queue_iterate(&pp->phys_link, mp,
		      struct mapping *, phys_link) {

		/*	Find the first changed page mapping but
			do not include the phys page mapping.
		*/
		if (!mp->vm_info.bits.phys	&& 
		     mp->pte->pte1.bits.changed) 
		{
			pp->pte1.bits.changed = TRUE;
			splx(s);
			return TRUE;
		}
	}
	
	splx(s);
	return(FALSE);
}



/*
 * pmap_clear_reference(phys)
 *	clears the hardware referenced bit in the given machine
 *	independant physical page.  
 *
 */
void
pmap_clear_reference(
	vm_offset_t pa)
{
	register struct phys_entry *pp;
	register struct mapping *mp;
	spl_t s;
	boolean_t was_valid;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pa=%x)\n", pa));
#endif

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	s = splhigh();
	/*	Run through all the virtual mappings to clear the reference
		bit in each PTE. Because the reference bit does not have to be
		maintained exactly, we can avoid issuing syncs but once.	
	 */
	queue_iterate(&pp->phys_link, mp,
		      struct mapping *, phys_link) {

		was_valid = FALSE;
		if (mp->pte->pte0.bits.valid == TRUE) {
			mp->pte->pte0.bits.valid = FALSE;
			was_valid = TRUE;
		}
		/*
		** Make sure there is no entry in the TBL, it is assumed
		** that if there is an entry that the page has been referenced
		*/
		sync();
		tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
		eieio();
		tlbsync();
		sync();

		/*
		** Gotta drop both bits or the chip gets hosed!
		**	See section 7.5.3
		**
		** Thanks Rene!
		*/
		pp->pte1.bits.changed |=
			mp->pte->pte1.bits.changed;
		mp->pte->pte1.bits.changed = FALSE;
		mp->pte->pte1.bits.referenced = FALSE;
		eieio();
		if (was_valid == TRUE)
		{
			mp->pte->pte0.bits.valid = TRUE;
		}
		sync();
	}

	/*	Mark the physical entry shadow copy as non-referenced too.
	*/
	pp->pte1.bits.referenced = FALSE;

	splx(s);
}



/*
 * pmap_is_referenced(phys)
 *	returns TRUE if the given physical page has been referenced 
 *	since the last call to pmap_clear_reference().
 */
boolean_t
pmap_is_referenced(
	vm_offset_t pa)
{
	struct phys_entry *pp;
	register struct mapping *mp;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("(pa=%x)\n", pa));
#endif
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);

	if (pp->pte1.bits.referenced)
		return (TRUE);


	s = splhigh();
	/*	Make sure all TLB -> PTE updates have completed.
	*/
	sync();

	/*	Check all the mappings to see if any one of them
		have recorded a reference to this physical page.
	*/
	queue_iterate(&pp->phys_link, mp,
		      struct mapping *, phys_link) {

		/*	Find the first referenced page mapping but
			do not include the phys page mapping.
		*/
		if (!mp->vm_info.bits.phys	&& 
		     mp->pte->pte1.bits.referenced) 
		{
			pp->pte1.bits.referenced = TRUE;
			splx(s);
			return (TRUE);
		}
	}
	splx(s);
	return(FALSE);
}



/*
 * Auxilliary routines for manipulating mappings
 */

static struct mapping *
pmap_enter_mapping(
	pmap_t	pmap,
	space_t	space,
	vm_offset_t	va,
	vm_offset_t	pa,
	pte_t	*pte,
	unsigned	prot,
	struct phys_entry *pp)
{
	register struct mapping *mp;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF(("(pmap=%x, sp=%x, off=%x, &pte=%x, prot=%x)",
		       pmap, space, va, pte, prot));
#endif /* DEBUG */

	s = splhigh();
	simple_lock(&free_mapping_lock);
	if (queue_empty(&free_mapping))
		pmap_reap_mappings();
	queue_remove_first(&free_mapping, mp, struct mapping *, phys_link);

	simple_unlock(&free_mapping_lock);
	assert(mp != MAPPING_NULL);

	mp->pmap = pmap;
	mp->vm_info.bits.phys	= FALSE;
	mp->vm_info.bits.wired	= FALSE;
	mp->vm_info.bits.page  = (va >> PPC_PGSHIFT);
	mp->pte			= pte;

	/* Set up pte -
	 *   pte0 already contains everything except the valid bit.
	 *   pte1 already contains the protection information.
	 */

	assert(pte->pte0.word != PTE_EMPTY);

	assert(pte->pte0.bits.valid == FALSE);

	sync();
	tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
	eieio();
	tlbsync();
	sync();

	pte->pte1.bits.phys_page = pa >> PPC_PGSHIFT;
	pte->pte1.bits.wimg = pp->pte1.bits.wimg; /* Set default cachability*/

	/*	Make sure those pte1 writes are ordered ahead of
		of the v bit.
	*/
	eieio();


	pte->pte0.bits.valid = TRUE;		/* Validate pte entry */
	sync();

	queue_enter_first(&pp->phys_link, mp, struct mapping *, phys_link);

#if	DEBUG
	/*	Assert a PTE of type WIMG_IO is not mapped to general RAM.
	*/
	if (pmdebug & PDB_IO 	&&
	    pte->pte1.bits.wimg == PTE_WIMG_IO)
		assert (((unsigned) pa) >= PCI_BASE)

	/* Assert that there are no other mappings with different
	 * cachability information, as this can freeze the machine
	 */
	{
		struct mapping *mp2;
		queue_iterate(&pp->phys_link, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->pte->pte1.bits.wimg ==
			       pte->pte1.bits.wimg);
		}
	}
#endif	/* DEBUG */
#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF((" -> %x\n"));
#endif
#if DEBUG
	if (pmdebug & PDB_MASSIVE) {
		register struct mapping *mp2;
		queue_iterate(&free_mapping, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->phys_link.next != NULL);
		}
	}
#endif /* DEBUG */	
	splx(s);
	return(mp);
}



/*
 * pmap_find_mapping(space, offset)
 *	Lookup the virtual address <space,offset> in the mapping "table".
 *	Returns a pointer to the mapping or NULL if none.
 *
 *	XXX what about MP locking?
 */
static struct mapping *
pmap_find_mapping(
	register space_t space,
	vm_offset_t offset)
{
	register struct mapping *mp;
	register struct phys_entry *pp;
	register pte_t *pte;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF(("(sp=%x, off=%x) -> ", space, offset));
#endif
	offset = trunc_page(offset);

	/* Locate the pte but don't allocate a new one if not found */
	pte = find_or_allocate_pte(space, offset, FALSE);

	if (pte == NULL) {
#if DEBUG
		if (pmdebug & PDB_MAPPING)
			DPRINTF(("MAPPING_NULL\n"));
#endif /* DEBUG */
		return MAPPING_NULL;
	}

	s = splhigh();
	/* Take advantage of the fact that pte1 can be
	 * considered as a pointer to an address in
	 * the physical page when looking for physentry.
	 */

	pp = pmap_find_physentry((vm_offset_t)(pte->pte1.word));

	queue_iterate(&pp->phys_link, mp,
		      struct mapping *, phys_link) {
		if (mp->pte == pte)
			break;
	}
	assert(mp != MAPPING_NULL);	/* There must be mapping for a pte */
	assert(mp->pte == pte);
	assert(((mp->vm_info.bits.page >> 10) & 0x3f) ==
	       mp->pte->pte0.bits.page_index);

#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF(("%x\n", mp));
#endif
	splx(s);
	return(mp);
}



static void
pmap_free_mapping(
	register struct mapping *mp)
{
	register vm_offset_t offset;
	struct phys_entry *pp;
	spl_t s;

#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF(("(mp=%x), pte at 0x%x (0x%08x,0x%08x)\n",
		       mp, mp->pte, mp->pte->pte0.word,mp->pte->pte1.word));
#endif

	s = splhigh();

	offset = mp->vm_info.bits.page << PPC_PGSHIFT;

	pp = pmap_find_physentry((vm_offset_t)mp->pte->pte1.word);
	assert(pp != PHYS_NULL);

	/*	Perform a general case PTE update designed to work
		in SMP configurations. TODO - locks and tlbsync
	*/
	mp->pte->pte0.bits.valid = FALSE;		/* Invalidate PTE */
	sync();				/* Force updates to complete */
	tlbie(offset);			/* Wipe out this virtual address */
	eieio();
	tlbsync();
	sync();				/* force completion */

	/* Reflect back the modify and reference bits for the pager */
	if (!mp->vm_info.bits.phys) 
		pp->pte1.word |= mp->pte->pte1.word;

	/*
	 * Now remove from the PTOV/VTOP lists and return to the free list.
	 * Note that we must block interrupts since they might cause TLB
	 * misses which would search (potentially inconsistant) lists.
	 */


	/* for now, just remove the mapping */
	mp->pte->pte0.word = PTE_EMPTY; /* Remove pte (v->p) lookup */
#if DEBUG
	mp->pte->pte1.word = 0;
#endif	

	queue_remove(&pp->phys_link, mp, struct mapping *, phys_link);

	simple_lock(&free_mapping_lock);
	queue_enter(&free_mapping, mp, struct mapping *, phys_link);
	simple_unlock(&free_mapping_lock);
#if DEBUG
	if (pmdebug & PDB_MASSIVE) {
		register struct mapping *mp2;
		queue_iterate(&free_mapping, mp2,
			      struct mapping *, phys_link) {
			assert(mp2->phys_link.next != NULL);
		}
	}
#endif /* DEBUG */	
	splx(s);
}



/*
 * Deal with a hash-table pteg overflow. pmap_pteg_overflow is upcalled from
 * find_or_allocate_pte(), and assumes knowledge of the pteg hashing
 * structure
 */

static int pteg_overflow_counter=0;

static pte_t *pteg_pte_exchange(pte_t *pteg, pte0_t match);
pte_t *
pmap_pteg_overflow(
	pte_t *primary_hash,
	pte0_t primary_match,
	pte_t *secondary_hash,
	pte0_t secondary_match)
{
	pte_t *pte;
#if DEBUG
	int i;
	if (pmdebug & PDB_PTEG) {
		kprintf("pteg overflow for ptegs 0x%08x and 0x%08x\n",
		       primary_hash,secondary_hash);
		kprintf("PRIMARY\t\t\t\t\tSECONDARY\n");
		for (i=0; i<8; i++) {
			kprintf("0x%08x : (0x%08x,0x%08x)\t"
			       "0x%08x : (0x%08x,0x%08x)\n",
			       &primary_hash[i],
			       primary_hash[i].pte0.word,
			       primary_hash[i].pte1.word,
			       &secondary_hash[i],
			       secondary_hash[i].pte0.word,
			       secondary_hash[i].pte1.word);
			assert(primary_hash[i].pte0.word != PTE_EMPTY);
			assert(secondary_hash[i].pte0.word != PTE_EMPTY);
		}
	}
#endif
	/* First try to replace an entry in the primary PTEG */
	pte = pteg_pte_exchange(primary_hash, primary_match);
	if (pte != PTE_NULL)
		return pte;
	pte = pteg_pte_exchange(secondary_hash, secondary_match);
	if (pte != PTE_NULL)
		return pte;

	panic("both ptegs are completely wired down\n");
	return PTE_NULL;
}


			
/*
 * Used by the pmap_pteg_overflow function to scan through a pteg
*/
static pte_t *
pteg_pte_exchange(
	pte_t *pteg,
	pte0_t match)
{
	/* Try and replace an entry in a pteg */

	int i;
	register struct phys_entry *pp;
	register struct mapping *mp;
	pte_t *pte;

	/* TODO NMGS pteg overflow algorithm needs working on!
	 * currently we just loop around the PTEGs rejecting
	 * the first available entry.
	 */

	for (i=0; i < 8; i++) {

		pte = &pteg[pteg_overflow_counter];
		pteg_overflow_counter = (pteg_overflow_counter+1) % 8;

		/* all mappings must be valid otherwise we wouldn't be here */
		assert(pte->pte0.bits.valid == TRUE);

		/* Take advantage of the fact that pte1 can be
		 * considered as a pointer to an address in
		 * the physical page when looking for physentry.
		 */
		pp = pmap_find_physentry(pte->pte1.word);

		/* pte entry must have reverse mapping, otherwise we
		 * cannot remove it (it was entered by pmap_map_bd)
		 */
		if (pp == PHYS_NULL)
			continue;

		queue_iterate(&pp->phys_link, mp,
			      struct mapping *, phys_link) {
			if (mp->pte == pte)
				break;
		}
		assert(mp != NULL);
		assert(mp->pte == pte);

		/* if this entry isn't wired, isn't in the kernel space,
		 * and it doesn't have any special cachability attributes
		 * we can remove mapping without losing any information.
		 * XXX kernel space mustn't be touched as we may be doing
		 * XXX an I/O on a non-wired page, and we must not take
		 * XXX a page fault in an interrupt handler.
		 */
		/* TODO better implementation for cache info? */
		if (!mp->vm_info.bits.wired &&
		    mp->pmap != kernel_pmap &&
		    mp->pte->pte1.bits.wimg == PTE_WIMG_DEFAULT) {
#if DEBUG
			if (pmdebug & PDB_PTEG) {
				kprintf("entry chosen at 0x%08x in %s PTEG\n",
				       pte,
				       pte->pte0.bits.hash_id ?
				       	"SECONDARY" : "PRIMARY");
				kprintf("throwing away mapping from sp 0x%x, "
				       "virtual 0x%08x to physical 0x%08x\n",
				       mp->pmap->space,
				       mp->vm_info.bits.page << PPC_PGSHIFT,
				       trunc_page(mp->pte->pte1.word));
				kprintf("pteg_overflow_counter=%d\n",
				       pteg_overflow_counter);
				kprintf("pte0.vsid=0x%08x\n",
				       pte->pte0.bits.segment_id);
			}
#endif /* DEBUG */
			mp->pmap->stats.resident_count--;

			/* release mapping structure and PTE, keeping
			 * track of changed and referenced bits
			 */
			pmap_free_mapping(mp);

			/* Replace the pte entry by the new one */

			match.bits.valid = FALSE;
			sync();
			pte->pte0 = match;
			sync();

			return pte;
		}
	}
	return PTE_NULL;
}



/*
 * Collect un-wired mappings. How best to do this??
 */
static void
pmap_reap_mappings(void)
{
#if DEBUG
	if (pmdebug & PDB_MAPPING)
		DPRINTF(("pmap_reap_mappings()\n"));
#endif

	panic("out of mapping structs");
}


/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
vm_offset_t
kvtophys(
	vm_offset_t va)
{
	pte_t *pte;
	extern vm_offset_t virtual_avail;

	if (va < virtual_avail)
	    return va;
	
	pte = find_or_allocate_pte(PPC_SID_KERNEL, trunc_page(va), FALSE);
	if (pte == NULL)
		return(0);
	return trunc_page(pte->pte1.word) | (va & page_mask);
}



/*
 * pmap_remove_all - Remove translations by physical address.
 *
 * This routine walks the pv_entry chain for the given va, and 
 * removes all translations.  It records modify bits.
 *
 * for 2.5vm <--> 3.0pmap this mapped to pmap_page_protect()
 */
void
pmap_remove_all(
	vm_offset_t phys)
{
	pmap_page_protect(phys, VM_PROT_NONE);
}



/*
 * pmap_copy_on_write - Ready region for copy-on-write handling.
 *
 * This routine removes write privleges from all physical maps
 * for a given physical page.
 *
 * for 2.5vm <--> 3.0pmap this mapped to pmap_page_protect()
 *
 */
void
pmap_copy_on_write(
	vm_offset_t phys)
{
	pmap_page_protect(phys, VM_PROT_READ);
}



/*
 * pmap_move_page - Move pages from one kernel vaddr to another.
 *
 * Parameters:
 *      from    - Original kernel va    (Mach. indep. page boundary)
 *      to      - New kernel va         (Mach. indep. page boundary)
 *      size    - Bytes to move.        (Mach. indep. pages)
 *
 * Returns:
 *      Nothing.
 */
static void
pmap_move_page(
	unsigned long from,
	unsigned long to,
	vm_size_t size)
{
        spl_t s;
        pte_t *pte;
		struct phys_entry *pp;
		struct mapping *mp;
        vm_prot_t prot;
		boolean_t wired;
		int ppn;

        s = splhigh();
        if ((size & page_mask) != 0) {
                panic("pmap_move_page: partial mach indep page");
        }
        for (; size != (unsigned) 0; size -= PAGE_SIZE, from += PAGE_SIZE, to += PAGE_SIZE) {
		if ((pte = find_or_allocate_pte(kernel_pmap->space, trunc_page(from), FALSE)) == PTE_NULL) {
                        panic("pmap_move_page: from not mapped");
                }
		/*
		 * Make sure the damn thing is valid.
		 */
		if (pte->pte0.bits.valid == FALSE) {
			panic("pmap_move_page: invalid pte!");
		}
                switch (pte->pte1.bits.protection) {
                case 3:
                        prot = VM_PROT_READ;
                        break;
                default:
                        prot = VM_PROT_READ|VM_PROT_WRITE;
                        break;
                }

		ppn = pte->pte1.bits.phys_page;
		pp = pmap_find_physentry((vm_offset_t)(pte->pte1.word));
		queue_iterate(&pp->phys_link, mp,
				struct mapping *, phys_link) {
			if (mp->pte == pte)
				break;
		}
		assert(mp != MAPPING_NULL);
		assert(mp->pte == pte);
		assert(((mp->vm_info.bits.page >> 10) & 0x3f) ==
			mp->pte->pte0.bits.page_index);
		wired = mp->vm_info.bits.wired;

		/*
		 * Remove old mapping first (avoid aliasing)
		 */
                pmap_remove(kernel_pmap, from, from + PAGE_SIZE);
                pmap_enter(kernel_pmap, to, PMAP_PTOB(ppn), prot, wired);
        }
        splx(s);
}



/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of the page size.
 */
void
pagemove(
	register caddr_t from,
	register caddr_t to,
	int size)
{
	pmap_move_page((unsigned long)from, (unsigned long)to, (vm_size_t)size);
}



#if DEBUG
/* This function is included to test compiler functionality, it does
 * a simple slide through the bitfields of a pte to check
 * minimally correct bitfield behaviour. We need bitfields to work!
 */
#define TEST(PTE0,PTE1)							\
	if ((pte.pte0.word != (unsigned int)PTE0) ||			\
	    (pte.pte1.word != (unsigned int)PTE1)) {			\
		DPRINTF(("BITFIELD TEST FAILED AT LINE %d\n",__LINE__));	\
		DPRINTF(("EXPECTING 0x%08x and 0x%08x\n",			\
		       (unsigned int)PTE0,(unsigned int)PTE1));		\
		DPRINTF(("GOT       0x%08x and 0x%08x\n",			\
		       pte.pte0.word,pte.pte1.word));			\
		return 1;						\
	}

extern int pmap_test_bitfields(void); /* Prototyped also in go.c */
int pmap_test_bitfields(void)
{
	pte_t	pte;
	pte.pte0.word = 0;
	pte.pte1.word = 0;

	pte.pte0.bits.segment_id = 0x103; TEST(0x103 << 7, 0);
	pte.pte0.bits.valid = 1;
	TEST((unsigned int)(0x80000000U| 0x103<<7), 0);
	pte.pte0.bits.hash_id = 1;
	TEST((unsigned int)(0x80000000U | 0x103 << 7 | 1<<6), 0);
	pte.pte0.bits.page_index = 3;
	TEST((unsigned int)(0x80000000U|0x103<<7|1<<6|3), 0);

	pte.pte0.bits.segment_id = 0;
	TEST((unsigned int)(0x80000000U|1<<6|3), 0);
	pte.pte0.bits.page_index = 0;
	TEST((unsigned int)(0x80000000U|1<<6), 0);
	pte.pte0.bits.valid = 0;
	TEST(1<<6, 0);

	pte.pte1.bits.referenced = 1; TEST(1<<6, 1<<8);
	pte.pte1.bits.protection = 3; TEST(1<<6, (unsigned int)(1<<8 | 3));
	pte.pte1.bits.changed = 1; TEST(1<<6,(unsigned int)(1<<8|1<<7|3));
	pte.pte1.bits.phys_page = 0xfffff;
	TEST(1<<6,(unsigned int)(0xfffff000U|1<<8|1<<7|3));

	pte.pte1.bits.changed = 0;
	TEST(1<<6,(unsigned int)(0xfffff000U|1<<8|3));

	return 0;
}
#endif /* DEBUG */



void 
pmap_enter_cache_spec(
    pmap_t		pmap,
    vm_offset_t		va,
    vm_offset_t		pa,
    vm_prot_t		prot,
    boolean_t		wired,
    cache_spec_t	caching
)
{
    if (pmap == PMAP_NULL)
	return;

	pmap_enter(pmap, va, pa, prot, wired);
	
}



void
pmap_update( void )
{
#if DEBUG
	if (pmdebug & PDB_USER)
		DPRINTF(("()\n"));
#endif
}
