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
 * Intel386 Family:	Hardware page mapping.
 *
 * HISTORY
 *
 * 9 April 1992 ? at NeXT
 *	Created.
 */
 
#import <cpus.h>

#import <mach/mach_types.h>

#import <vm/vm_kern.h>
#import <vm/vm_page.h>

#import <machdep/i386/pmap_private.h>
#import <machdep/i386/pmap_inline.h>
#import <machdep/i386/cpu_inline.h>

/*
 * Setup structures to map from mach vm_prot_t
 * to machine protections.
 */
unsigned int			user_prot_codes[8];
unsigned int			kernel_prot_codes[8];
#define kernel_pmap_prot(x)	(kernel_prot_codes[(x)])

static
void
pte_prot_init(
    void
)
{
    unsigned int	*kp, *up;
    int			prot;

    kp = kernel_prot_codes;
    up = user_prot_codes;
    for (prot = 0; prot < 8; prot++) {
	switch ((vm_prot_t)prot) {
	case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
	    *kp++ = 0;
	    *up++ = 0;
	    break;
	case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
	case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
	case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
	    *kp++ = PT_PROT_KR;
	    *up++ = PT_PROT_UR;
	    break;
	case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
	case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
	case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
	case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
	    *kp++ = PT_PROT_KRW;
	    *up++ = PT_PROT_URW;
	    break;
	}
    }
}

/*
 * Given a map and a machine independent protection code,
 * set the protection code in the given pte
 */
static inline
void
pte_prot(
    pmap_t		pmap,
    vm_prot_t		prot,
    int			valid,		/* valid bit for pte */
    pt_entry_t *	pte		/* IN/OUT */
)
{
    if (pmap == kernel_pmap)
	pte->prot = kernel_prot_codes[prot];
    else
	pte->prot = user_prot_codes[prot];

    pte->valid = valid;
}

/*
 * Return a ptr to the page
 * table entry at the indicated
 * offset in the pmap.  Return
 * PT_ENTRY_NULL if the page table
 * does not exist.
 */
inline
pt_entry_t *
pmap_pt_entry(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    pd_entry_t *	pde;
    
    pde = pd_to_pd_entry(pmap->root, va);
    
    if (!pde->valid)
	return (PT_ENTRY_NULL);
	
    return (pd_entry_to_pt_entry(pde, va));
}

/*
 * Return a ptr to the page
 * directory entry at the indicated
 * offset in the pmap.
 */
inline
pd_entry_t *
pmap_pd_entry(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    return (pd_to_pd_entry(pmap->root, va));
}

/*
 * Return the physical address
 * corresponding to the indicated
 * offset in the pmap.  Only used
 * internally since no locking is
 * done.
 */
static inline
unsigned int
pmap_phys(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    pt_entry_t		*pte;
    
    pte = pmap_pt_entry(pmap, va);
    if (pte == PT_ENTRY_NULL || !pte->valid)
	return (0);
	    
    return (pfn_to_phys(pte->pfn) + page_offset(va));
}

static inline
void
pmap_update_tlbs(
    pmap_t		pmap,
    vm_offset_t		start,
    vm_offset_t		end
)
{
    if (pmap == kernel_pmap || pmap->cpus_using) {
	tlb_stat.total++;
	if (end - start > PAGE_SIZE)
	    flush_tlb();
	else {
	    for (; start < end; start += I386_PGBYTES)
		invlpg(start, pmap == kernel_pmap);
	  
	    tlb_stat.single++;
	}
    }
}

/*
 * Allocate a new page table
 * in the kernel pmap at the
 * given offset when one does
 * not already exist.
 */
static
void
pmap_kernel_pt_alloc(
    vm_offset_t		addr
)
{
    pd_entry_t		template, *pde;
    vm_offset_t		pt;
    unsigned int	phys;
    int			i;
    
    pde = pmap_pd_entry(kernel_pmap, trunc_section(addr));
    if (pde->valid)
	panic("pmap_kernel_pt_alloc");
	
    if (!pmap_initialized) {
	pt = alloc_pages(PAGE_SIZE);
	if (!pt)
	    panic("pmap_kernel_pt_alloc 2");

	bzero(pt, PAGE_SIZE);

	phys = pt - VM_MIN_KERNEL_ADDRESS;	
    }
    else {
	if (kmem_alloc_wired(kernel_map, &pt, PAGE_SIZE) != KERN_SUCCESS)
	    panic("pmap_kernel_pt_alloc 3");
	    
	phys = pmap_phys(kernel_pmap, pt);
    }
    
    template = (pd_entry_t) { 0 };
    template.valid = 1;
    template.prot = PT_PROT_KRW;
    template.pfn = phys_to_pfn(phys);
    
    for (i = ptes_per_vm_page; i-- > 0; pde++) {
	*pde = template;
	template.pfn++;
    }
}

/*
 * Map memory at initialization.  The physical addresses being
 * mapped are not managed and are never unmapped.
 */
vm_offset_t
pmap_map(
    vm_offset_t		virt,
    vm_offset_t		start,
    vm_offset_t		end,
    vm_prot_t		prot
)
{
    pt_entry_t		template, *pte;
    
    template = (pt_entry_t) { 0 };

    if (prot != VM_PROT_NONE) {
	template.valid = 1;
	template.prot =  kernel_pmap_prot(prot);
	template.pfn = phys_to_pfn(start);
    }

    while (start < end) {
	pte = pmap_pt_entry(kernel_pmap, virt);
	if (pte == PT_ENTRY_NULL) {
	    pmap_kernel_pt_alloc(virt);
	    pte = pmap_pt_entry(kernel_pmap, virt);
	}

	/*
	 * Is this necessary ??
	 */	
	if (	start >= (640 * 1024) &&
		start < (1024 * 1024))
	    template.cachewrt = 1;
	else
	    template.cachewrt = 0;
	
	*pte = template;
	if (prot != VM_PROT_NONE)
	    template.pfn++;

	virt += I386_PGBYTES;
	start += I386_PGBYTES;
    }

    flush_tlb();

    return (virt);
}

static
void
pmap_enable_pg(
    vm_offset_t		kernel_pd
)
{
    pd_entry_t		*kpde, *end_kpde;
    pd_entry_t		*pde;
    cr0_t		_cr0 = cr0();

    /*
     * Double map the kernel memory
     * into the low end of the kernel
     * pmap linear space.  This is
     * necessary in order to enable
     * paging.
     */
    pde = (pd_entry_t *)kernel_pd;

    kpde =
    	pmap_pd_entry(kernel_pmap,
			VM_MIN_KERNEL_ADDRESS);
    end_kpde =
    	pmap_pd_entry(kernel_pmap,
			VM_MAX_KERNEL_ADDRESS);
			
    while (kpde < end_kpde)
	*pde++ = *kpde++;

    /*
     * Use the kernel pmap
     * as our initial translation
     * tree.
     */ 
    kernel_pmap->cr3 =
	    pmap_phys(kernel_pmap, kernel_pd);
    
    set_cr3(kernel_pmap->cr3);

    /*
     * Now, enable paging by
     * turning on the PG bit
     * in CR0.  Also turn on
     * the WP bit to allow
     * the write protecting of
     * memory with repect to
     * the kernel.
     */ 
    _cr0.pg = _cr0.wp = TRUE;
    set_cr0(_cr0);
}

/*
 */
void
pmap_bootstrap(
    mem_region_t	mem_region,
    int			num_regions,
    vm_offset_t *	virt_avail,	/* OUT */
    vm_offset_t *	virt_end	/* OUT */
)
{
    vm_offset_t		va;
    unsigned int	phys_end = mem_region[0].last_phys_addr;
    vm_offset_t		kernel_pd;
    
    /*
     * Setup section_size variable.
     */
    section_size = SECTION_SIZE;
    	
    /*
     *	Set ptes_per_vm_page for general use.
     */
    ptes_per_vm_page = PAGE_SIZE / I386_PGBYTES;

    /*
     *	Initialize pte protection arrays.
     */
    pte_prot_init();

    /*
     *	The kernel's pmap is statically allocated so we don't
     *	have to use pmap_create, which does not work
     *	correctly at this part of the boot sequence.
     */
    kernel_pmap = &kernel_pmap_store;

    simple_lock_init(&kernel_pmap->lock);

    /*
     * Allocate a page directory for the
     * kernel pmap.
     */
    (vm_offset_t)kernel_pmap->root =
	    kernel_pd = alloc_cnvmem(I386_PGBYTES, I386_PGBYTES);

    bzero((vm_offset_t)kernel_pmap->root, I386_PGBYTES);

    kernel_pmap->ref_count = 1;

    kernel_pmap->root +=
    	(KERNEL_LINEAR_BASE - VM_MIN_KERNEL_ADDRESS) / I386_SECTBYTES;
    
    /*
     * Map all physical memory V == P.
     */
    va = pmap_map(VM_MIN_KERNEL_ADDRESS,
		  0,
		  phys_end,
		  VM_PROT_READ | VM_PROT_WRITE);

    /*
     * Allocate additional kernel page tables
     * needed for allocating kernel virtual memory later.
     */
    *virt_avail = va;
     va = pmap_map(va,
     		   0,
		   64*1024*1024	+	/* base size */
		   zone_map_sizer() +	/* zone allocator */
		   buffer_map_sizer(),	/* buffer cache */
		   VM_PROT_NONE);
		   
    *virt_end = va;

    /*
     * Finish initialization
     * of the kernel pmap, and
     * then enable paging.
     */
    pmap_enable_pg(kernel_pd);
}

/*
 * Initialize the pmap module.
 * Called by vm_init, to initialize any structures that the pmap
 * system needs to map virtual memory.
 */
void
pmap_init(
    mem_region_t	mem_region,
    int			num_regions
)
{
    unsigned int	phys_start, phys_end;
    int			npages;
    vm_size_t		s;

    phys_start	= mem_region[0].first_phys_addr;
    phys_end	= mem_region[0].last_phys_addr;

    npages	= mem_region[0].num_pages;

    /*
     * Allocate memory for the page descriptor
     * table.
     */
    s = sizeof(struct pg_desc) * npages;
    (void) kmem_alloc_wired(kernel_map, (vm_offset_t *)&pg_desc_tbl, s);
    pg_first_phys = phys_start;

    /*
     * Create the zone of physical maps,
     * and of the physical-to-virtual entries.
     */
    s = sizeof (struct pmap);
    pmap_zone = zinit(s, 400*s, 0, FALSE, "pmap");		/* XXX */

    s = sizeof (struct pv_entry);
    pv_entry_zone = zinit(s, 10000*s, 0, FALSE, "pv_entry");	/* XXX */
    
    /*
     * Create the zone of page
     * extensions.
     */
    s = sizeof (struct pg_exten);
    pg_exten_zone = zinit(s, npages*s, 0, FALSE, "pg_exten");

    /*
     * Initialize the queues of active and
     * free page tables and free page directories.
     */	
    queue_init(&pt_active_queue);
    queue_init(&pt_free_queue);
    queue_init(&pd_free_queue);

    /*
     * Only now, when all of the data structures are allocated,
     * can we set vm_first_phys and vm_last_phys.  If we set them
     * too soon, the kmem_alloc above will try to use these
     * data structures and blow up.
     */

    vm_first_phys	= phys_start;
    vm_last_phys	= phys_end;

    pmap_initialized = TRUE;
}

static
pg_exten_t *
pg_exten_alloc(
    vm_offset_t		page
)
{
    pg_exten_t		*pe;
    pg_desc_t		*pd;
        
    (vm_offset_t)pe = zalloc(pg_exten_zone);
    
    pe->phys = pmap_phys(kernel_pmap, page);
    
    pd = pg_desc(pe->phys);

    pd->pg_exten = pe; pe->pg_desc = pd;
    
    pe->alloc_count = pe->wired_count = pe->unrefd_age = pe->alloc_bmap = 0;
    
    return (pe);
}

static
void
pg_exten_free(
    pg_exten_t		*pe
)
{
    pe->pg_desc->pg_exten = PG_EXTEN_NULL;
    
    zfree(pg_exten_zone, (vm_offset_t)pe);
}

static
void
pmap_alloc_pd(
    pmap_t		pmap
)
{
    pg_exten_t		*pe;
    int			i;
   
    if (pe = pd_free_obtain()) {
	if (++pe->alloc_count == ptes_per_vm_page)
	    pd_free_remove(pe);

	i = ffs(~pe->alloc_bmap) - 1;
	pe->alloc_bmap |= (1 << i);

	(vm_offset_t)pmap->root = pe->pg_desc->pv_list.va + (I386_PGBYTES * i);
    }
    else {
    	if (kmem_alloc_wired(kernel_map,
			(vm_offset_t *)&pmap->root, PAGE_SIZE) != KERN_SUCCESS)
	    panic("pmap_alloc_pd");

	pd_alloc_count++;
	
	pe = pg_exten_alloc((vm_offset_t)pmap->root);

	if (++pe->alloc_count < ptes_per_vm_page)
	    pd_free_add(pe);

	pe->alloc_bmap |= 1;
    }
}

static
void
pmap_free_pd(
    pmap_t		pmap
)
{
    pg_exten_t		*pe;
    pg_desc_t		*pd;
    int			i;
    
    pd = pg_desc(pmap_phys(kernel_pmap, (vm_offset_t)pmap->root));
    
    pe = pd->pg_exten;

    if (pe->alloc_count-- == ptes_per_vm_page)
    	pd_free_add(pe);

    i = ((vm_offset_t)pmap->root - pd->pv_list.va) / I386_PGBYTES;

    pe->alloc_bmap &= ~(1 << i);
}

/*
 * Allocate and setup the page directory
 * for a new pmap.  The pd_entries that
 * correspond to the kernel address space
 * are initialized by copying them from
 * the kernel pmap.  This works correctly
 * since we never expand the kernel pmap.
 */
static
void
pmap_create_pd(
    pmap_t		pmap
)
{
    pd_entry_t		*pde;
    pd_entry_t		*kpde, *end_kpde;
    
    pmap_alloc_pd(pmap);
    
    if (pmap->root == PD_ENTRY_NULL)
	panic("pmap_create_page_directory");
    
    if (i386_trunc_page(pmap->root) != (vm_offset_t)pmap->root)
    	panic("pmap_create_page_directory 1");
    
    pmap->cr3 = pmap_phys(kernel_pmap, (vm_offset_t)pmap->root);
	
    kpde =
    	pmap_pd_entry(kernel_pmap,
			VM_MIN_KERNEL_ADDRESS);
    end_kpde =
    	pmap_pd_entry(kernel_pmap,
			VM_MAX_KERNEL_ADDRESS);
		    
    pde = pmap_pd_entry(pmap, KERNEL_LINEAR_BASE);
    
    while (kpde < end_kpde)
	*pde++ = *kpde++;
}

/*
 * Create and return a physical map.
 *
 * If the size specified for the map
 * is zero, the map is an actual physical
 * map, and may be referenced by the
 * hardware.
 *
 * If the size specified is non-zero,
 * the map will be used in software only, and
 * is bounded by that size.
 */
pmap_t
pmap_create(
    vm_size_t		size
)
{
    pmap_t		pmap;

    /*
     *	A software use-only map doesn't even need a pmap.
     */
    if (size != 0)
	return (PMAP_NULL);

    /*
     * Allocate a pmap struct from the pmap_zone.
     */
    pmap = (pmap_t) zalloc(pmap_zone);
    if (pmap == PMAP_NULL)
	panic("pmap_create pmap");

    bzero((vm_offset_t)pmap, sizeof (struct pmap));

    /*
     * Create the page directory.
     */
    pmap_create_pd(pmap);

    pmap->ref_count = 1;

    simple_lock_init(&pmap->lock);
	
    return (pmap);
}

/*
 * Retire the given physical map from service.
 * Should only be called if the map contains
 * no valid mappings.
 */
void
pmap_destroy(
    pmap_t		pmap
)
{
    int			c, s;

    if (pmap == PMAP_NULL)
	return;

    SPLVM(s);
    simple_lock(&pmap->lock);

    c = --pmap->ref_count;

    simple_unlock(&pmap->lock);
    SPLX(s);

    if (c != 0)
	return;

    pmap_free_pd(pmap);

    zfree(pmap_zone, (vm_offset_t)pmap);
}

/*
 *	Add a reference to the specified pmap.
 */

void
pmap_reference(
    pmap_t		pmap
)
{
    int			s;

    if (pmap != PMAP_NULL) {
	SPLVM(s);
	simple_lock(&pmap->lock);

	pmap->ref_count++;

	simple_unlock(&pmap->lock);
	SPLX(s);
    }
}

/*
 * Remove a range of mappings from
 * a pmap.  The indicated range must
 * lie completely within one section,
 * i.e. the ptes must be within one
 * page table.
 */
static
void
pmap_remove_range(
    pmap_t		pmap,
    vm_offset_t		start,
    vm_offset_t		end,
    boolean_t		free_table
)
{
    pt_entry_t		*pte, *epte;
    pg_desc_t		*pd;
    pv_entry_t		pv_h;
    pv_entry_t		cur, prev;
    unsigned int	pa;
    vm_offset_t		va = start;
    int			i, num_removed = 0, num_unwired = 0;
    
    if ((pte = pmap_pt_entry(pmap, start)) == PT_ENTRY_NULL)
	return;
	
    epte = pmap_pt_entry(pmap, end);
    if (trunc_page(pte) != trunc_page(epte))
	epte = (pt_entry_t *)round_page(pte + ptes_per_vm_page);
    
    for (; pte < epte; va += PAGE_SIZE) {
	if (!pte->valid) {
	    pte += ptes_per_vm_page;
	    continue;
	}
	    
	num_removed++;
	if (pte->wired)
	    num_unwired++;
	    
	pa = pfn_to_phys(pte->pfn);
	if (!managed_page(pa)) {
	    for (i = ptes_per_vm_page; i-- > 0; pte++)
		*pte = (pt_entry_t) { 0 };
	    continue;
	}
	    
	pd = pg_desc(pa);
	LOCK_PVH(pd);
	
	/*
	 * Collect the referenced & dirty bits
	 * and clear the mapping.
	 */
	for (i = ptes_per_vm_page; i-- > 0; pte++) {
	    if (pte->dirty) {
		vm_page_t	m = PHYS_TO_VM_PAGE(pa);
		
		vm_page_set_modified(m);
		pd->page_attrib |= PG_DIRTY;
	    }
	    
	    if (pte->refer)
		pd->page_attrib |= PG_REFER;
		
	    *pte = (pt_entry_t) { 0 };
	}

	/*
	 * Remove the mapping from the pvlist for
	 * this physical page.
	 */
	pv_h = pg_desc_pvh(pd);
	if (pv_h->pmap == PMAP_NULL)
	    panic("pmap_remove_range");

	if (pv_h->va == va && pv_h->pmap == pmap) {
	    /*
	     * Header is the pv_entry.  Copy the next one
	     * to header and free the next one (we can't
	     * free the header)
	     */
	    cur = pv_h->next;
	    if (cur != PV_ENTRY_NULL) {
		*pv_h = *cur;
		zfree(pv_entry_zone, (vm_offset_t) cur);
	    }
	    else
		pv_h->pmap = PMAP_NULL;
	}
	else {
	    prev = pv_h;
	    while ((cur = prev->next) != PV_ENTRY_NULL) {
		if (cur->va == va && cur->pmap == pmap)
		    break;
		prev = cur;
	    }
	    if (cur == PV_ENTRY_NULL)
		panic("pmap_remove_range 2");

	    prev->next = cur->next;
	    zfree(pv_entry_zone, (vm_offset_t) cur);
	}
	
	UNLOCK_PVH(pd);
    }

    /*
     * Free the mappings from the page table.
     */	
    pmap_deallocate_mappings(
			    pmap, start,
			    num_removed, num_unwired, free_table);
}

/*
 * Remove the given range of addresses
 * from the specified pmap.
 *
 * It is assumed that the start and end are properly
 * rounded to the page size.
 */
void
pmap_remove(
    pmap_t		pmap,
    vm_offset_t		start,
    vm_offset_t		end
)
{
    vm_offset_t		sect_end;
    int			s;

    if (pmap == PMAP_NULL)
	return;

    PMAP_READ_LOCK(pmap, s);

    /*
     * Invalidate the translation buffer first
     */
    PMAP_UPDATE_TLBS(pmap, start, end);
    
    while (start < end) {
	sect_end = round_section(start + PAGE_SIZE);
	if (sect_end > end)
	    sect_end = end;

	pmap_remove_range(pmap, start, sect_end, TRUE);
	start = sect_end;
    }

    PMAP_READ_UNLOCK(pmap, s);
}

/*
 * Remove all references to 
 * the indicated page from
 * all pmaps.
 */
void
pmap_remove_all(
    unsigned int	pa
)
{
    pt_entry_t		*pte;
    pv_entry_t		pv_h, cur;
    pg_desc_t		*pd;
    vm_offset_t		va;
    pmap_t		pmap;
    int			s, i;
    
    if (!managed_page(pa))
	return;

    /*
     * Lock the pmap system first, since we will be changing
     * several pmaps.
     */
    PMAP_WRITE_LOCK(s);
    
    /*
     * Walk down PV list, removing all mappings.
     * We have to do the same work as in pmap_remove_range
     * since that routine locks the pv_head.  We don't have
     * to lock the pv_head, since we have the entire pmap system.
     */
    pd = pg_desc(pa);
    pv_h = pg_desc_pvh(pd);
    
    while ((pmap = pv_h->pmap) != PMAP_NULL) {
	va = pv_h->va;

	simple_lock(&pmap->lock);

	pte = pmap_pt_entry(pmap, va);
	if (pte == PT_ENTRY_NULL || !pte->valid)
	    panic("pmap_remove_all");
	    
	if (pfn_to_phys(pte->pfn) != pa)
	    panic("pmap_remove_all 2");
	    
	if (pte->wired)
	    panic("pmap_remove_all 3");
	
	/*
	 * Tell CPU using pmap to invalidate its TLB
	 */
	PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);

	if ((cur = pv_h->next) != PV_ENTRY_NULL) {
	    *pv_h = *cur;
	    zfree(pv_entry_zone, (vm_offset_t) cur);
	}
	else
	    pv_h->pmap = PMAP_NULL;	
	
	/*
	 * Collect the referenced & dirty bits
	 * and clear the mapping.
	 */
	for (i = ptes_per_vm_page; i-- > 0; pte++) {
	    if (pte->dirty) {
		vm_page_t	m = PHYS_TO_VM_PAGE(pa);
		
		vm_page_set_modified(m);
		pd->page_attrib |= PG_DIRTY;
	    }
	    
	    if (pte->refer)
		pd->page_attrib |= PG_REFER;
		
	    *pte = (pt_entry_t) { 0 };
	}
	
	pmap_deallocate_mappings(pmap, va, 1, 0, TRUE);
	
	simple_unlock(&pmap->lock);
    }
    
    PMAP_WRITE_UNLOCK(s);
}

/*
 * Remove write access to the
 * indicated page from all pmaps.
 */
void
pmap_copy_on_write(
    unsigned int	pa
)
{
    pt_entry_t		*pte;
    pv_entry_t		pv_e;
    int			i, s;

    if (!managed_page(pa))
	return;
	
    /*
     * Lock the entire pmap system, since we may be changing
     * several maps.
     */
    PMAP_WRITE_LOCK(s);
    
    pv_e = pg_desc_pvh(pg_desc(pa));
    if (pv_e->pmap == PMAP_NULL) {
	PMAP_WRITE_UNLOCK(s);
	return;
    }    

    /*
     * Run down the list of mappings to this physical page,
     * disabling write privileges on each one.
     */
    while (pv_e != PV_ENTRY_NULL) {
	pmap_t		pmap;
	vm_offset_t	va;

	pmap = pv_e->pmap;
	va = pv_e->va;

	simple_lock(&pmap->lock);
	
	pte = pmap_pt_entry(pmap, va);
	
	if (pte == PT_ENTRY_NULL || !pte->valid)
	    panic("pmap_copy_on_write");

	/*
	 * Ask cpus using pmap to invalidate their TLBs
	 */
	PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);
	
	if (pte->prot == PT_PROT_URW || pte->prot == PT_PROT_KRW)
	    for (i = ptes_per_vm_page; i-- > 0; pte++)
		pte_prot(pmap, VM_PROT_READ, pte->valid, pte);
	    
	simple_unlock(&pmap->lock);
	
	pv_e = pv_e->next;
    }
    
    PMAP_WRITE_UNLOCK(s);
}

/*
 * Change the page protection
 * on a range of addresses in
 * the indicated pmap.  If protect
 * is being changed to VM_PROT_NONE,
 * remove the mappings.
 */
void
pmap_protect(
    pmap_t		pmap,
    vm_offset_t		start,
    vm_offset_t		end,
    vm_prot_t		prot
)
{
    pt_entry_t		*pte, *epte;
    vm_offset_t		sect_end;
    int			i, s;
    
    if (pmap == PMAP_NULL)
	return;
	
    if (prot == VM_PROT_NONE) {
	pmap_remove(pmap, start, end);
	return;
    }
    
    SPLVM(s);
    simple_lock(&pmap->lock);

    /*
     * Invalidate the translation buffer first
     */
    PMAP_UPDATE_TLBS(pmap, start, end);    
    
    while (start < end) {
	sect_end = round_section(start + PAGE_SIZE);
	if (sect_end > end)
	    sect_end = end;
	    
	pte = pmap_pt_entry(pmap, start);
	if (pte != PT_ENTRY_NULL) {
	    epte = pmap_pt_entry(pmap, sect_end);
	    if (trunc_page(pte) != trunc_page(epte))
		epte = (pt_entry_t *)round_page(pte + ptes_per_vm_page);
		
	    while (pte < epte) {
		if (!pte->valid) {
		    pte += ptes_per_vm_page;
		    continue;
		}
		
		for (i = ptes_per_vm_page; i-- > 0; pte++)
		    pte_prot(pmap, prot, pte->valid, pte);
	    }
	}

	start = sect_end;
    }
    
    simple_unlock(&pmap->lock);
    SPLX(s);
}

/*
 * Insert the given physical page (p) at
 * the specified virtual address (v) in the
 * target physical map with the protection requested.
 *
 * If specified, the page will be wired down, meaning
 * that the related pte can not be reclaimed.
 *
 * NB:  This is the only routine which MAY NOT lazy-evaluate
 * or lose information.  That is, this routine must actually
 * insert this page into the given map NOW.
 */
void inline
pmap_enter_cache_spec(
    pmap_t		pmap,
    vm_offset_t		va,
    vm_offset_t		pa,
    vm_prot_t		prot,
    boolean_t		wired,
    cache_spec_t	caching
)
{
    pt_entry_t		*pte;
    pv_entry_t		pv_h;
    pg_desc_t		*pd;
    int			i, s;
    pv_entry_t		pv_e;
    pt_entry_t		template;
    vm_offset_t		old_pa;
    
    if (pmap == PMAP_NULL)
	return;
	
    if (prot == VM_PROT_NONE) {
	pmap_remove(pmap, va, va + PAGE_SIZE);
	return;
    }

    /*
     * Must allocate a new pvlist entry while we're unlocked;
     * zalloc may cause pageout (which will lock the pmap system).
     * If we determine we need a pvlist entry, we will unlock
     * and allocate one.  Then we will retry, throwing away
     * the allocated entry later (if we no longer need it).
     */
    pv_e = PV_ENTRY_NULL;
    template = (pt_entry_t) { 0 };

Retry:
    PMAP_READ_LOCK(pmap, s);

    /*
     * Expand pmap to include this pte.  Assume that
     * pmap is always expanded to include enough
     * pages to map one VM page.
     */
    while ((pte = pmap_pt_entry(pmap, va)) == PT_ENTRY_NULL) {
	/*
	 * Must unlock to expand the pmap.
	 */
	PMAP_READ_UNLOCK(pmap, s);

	pmap_expand(pmap, va);

	PMAP_READ_LOCK(pmap, s);
    }

    /*
     * Special case if the physical page is already mapped
     * at this address.
     */
    old_pa = pfn_to_phys(pte->pfn);
    if (pte->valid && old_pa == pa) {
	/*
	 * May be changing its wired attribute or protection
	 */
	if (wired && !pte->wired)
	    pmap_wire_mapping(pmap, va);
	else
	if (!wired && pte->wired)
	    pmap_unwire_mapping(pmap, va);

	pte_prot(pmap, prot, 1, &template);
	template.pfn = phys_to_pfn(pa);
	if (wired)
	    template.wired = 1;
	    
	if (caching == cache_disable)
	    template.cachedis = 1;
	else
	if (caching == cache_writethrough)
	    template.cachewrt = 1;

	PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);
	
	for (i = ptes_per_vm_page; i-- > 0; pte++) {
	    if (pte->dirty)
		template.dirty = 1;
	    *pte = template;
	    template.pfn++;
	}
    }
    else {

	/*
	 * Remove old mapping from the PV list if necessary.
	 */
	if (pte->valid) {
	    /*
	     * Invalidate the translation buffer,
	     * then remove the mapping.
	     */
	    PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);
	    
	    pmap_remove_range(pmap, va, va + PAGE_SIZE, FALSE);
	}
	
	if (managed_page(pa)) {

	    /*
	     * Enter the mapping in the PV list for this
	     * physical page.
	     */
	    pd = pg_desc(pa);
	    LOCK_PVH(pd);
	    pv_h = pg_desc_pvh(pd);

	    if (pv_h->pmap == PMAP_NULL) {

		/*
		 * No mappings yet
		 */
		pv_h->va = va;
		pv_h->pmap = pmap;
		pv_h->next = PV_ENTRY_NULL;
	    }
	    else {
		    
		/*
		 * Add new pv_entry after header.
		 */
		if (pv_e == PV_ENTRY_NULL) {
		    UNLOCK_PVH(pd);
		    PMAP_READ_UNLOCK(pmap, s);
		    pv_e = (pv_entry_t) zalloc(pv_entry_zone);
		    goto Retry;
		}
		pv_e->va = va;
		pv_e->pmap = pmap;
		pv_e->next = pv_h->next;
		pv_h->next = pv_e;
		/*
		 * Remember that we used the pvlist entry.
		 */
		pv_e = PV_ENTRY_NULL;
	    }
	    
	    UNLOCK_PVH(pd);
	}
	    
	pmap_allocate_mapping(pmap, va, wired);

	/*
	 * Build a template to speed up entering -
	 * only the pfn changes.
	 */
	pte_prot(pmap, prot, 1, &template);
	template.pfn = phys_to_pfn(pa);
	if (wired)
	    template.wired = 1;
	    
	if (caching == cache_disable)
	    template.cachedis = 1;
	else
	if (caching == cache_writethrough)
	    template.cachewrt = 1;
	
	for (i = ptes_per_vm_page; i-- > 0; pte++) {
	    *pte = template;
	    template.pfn++;
	}
    }

    PMAP_READ_UNLOCK(pmap, s);

    if (pv_e != PV_ENTRY_NULL)
	zfree(pv_entry_zone, (vm_offset_t) pv_e);
}

void
pmap_enter(
    pmap_t		pmap,
    vm_offset_t		va,
    vm_offset_t		pa,
    vm_prot_t		prot,
    boolean_t		wired
)
{
    pmap_enter_cache_spec(
			pmap,
			va,
			pa,
			prot,
			wired,
			cache_default);
}

void
pmap_enter_shared_range(
    pmap_t		pmap,
    vm_offset_t		va,
    vm_size_t		size,
    vm_offset_t		kern
)
{
    vm_offset_t		end = round_page(va + size);
    
    while (va < end) {
    	pmap_enter(
		    pmap,
		    va,
		    pmap_resident_extract(kernel_pmap, kern),
		    VM_PROT_READ|VM_PROT_WRITE,
		    TRUE);
		    
	va += PAGE_SIZE; kern += PAGE_SIZE;
    }
}

/*
 * Change the wiring attribute for a
 * pmap/virtual-address	pair.
 *
 * The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(
    pmap_t		pmap,
    vm_offset_t		va,
    boolean_t		wired
)
{
    pt_entry_t		*pte;
    int			i, s;
    
    /*
     * We must grab the pmap system lock because we may
     * change a pte_page queue.
     */
    PMAP_READ_LOCK(pmap, s);
    
    if ((pte = pmap_pt_entry(pmap, va)) == PT_ENTRY_NULL)
	panic("pmap_change_wiring");

    if (wired && !pte->wired) {
	/*
	 * wiring down mapping
	 */
	pmap_wire_mapping(pmap, va);
    }
    else
    if (!wired && pte->wired) {
	/*
	 * unwiring mapping
	 */
	pmap_unwire_mapping(pmap, va);
    }
    
    for (i = ptes_per_vm_page; i-- > 0; pte++)
	pte->wired = wired;

    PMAP_READ_UNLOCK(map, s);
}

vm_offset_t
pmap_extract(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    vm_offset_t		pa;
    int			s;
    
    SPLVM(s);
    simple_lock(&pmap->lock);
    
    pa = pmap_phys(pmap, va);
    
    simple_unlock(&pmap->lock);
    SPLX(s);
    
    return (pa);
}

vm_offset_t
pmap_resident_extract(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    return ((vm_offset_t) pmap_phys(pmap, va));
}

/*
 * Expand a pmap to be able to map the
 * specified virtual address by allocating
 * a single page table either from the list 
 * of free page tables or directly from kernel
 * memory.
 */
static
void
pmap_expand(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    pd_entry_t		template, *pde;
    pg_exten_t		*pe;
    vm_offset_t		pt;
    int			i, s;

    if (pmap == kernel_pmap)
	panic("pmap_expand");
	    
    if (pe = pt_free_obtain())
	pt = pe->pg_desc->pv_list.va;
    else {
	if (kmem_alloc_wired(kernel_map, &pt, PAGE_SIZE) != KERN_SUCCESS)
	    return;

	pt_alloc_count++;
	
	pe = pg_exten_alloc(pt);
    }

    PMAP_READ_LOCK(pmap, s);	

    /*
     * See if someone else expanded us first.
     */
    if (pmap_pt_entry(pmap, va) != PT_ENTRY_NULL) {
	PMAP_READ_UNLOCK(pmap, s);
	
	pg_exten_free(pe);

	kmem_free(kernel_map, pt, PAGE_SIZE);
	
	pt_alloc_count--;

	return;
    }

    /*
     * What virtual memory does this
     * page table map ?
     */
    pe->offset = trunc_section(va);
    pe->pmap = pmap;

    /*
     * Clear the reference aging
     * tick count.
     */
    pe->unrefd_age = 0;

    /*
     * Add this page table
     * to the active queue.
     */	
    pt_active_add(pe);

    /*
     * Setup entry template.
     */
    template = (pd_entry_t) { 0 };
    template.valid = 1;
    template.prot = PT_PROT_URW;
    template.pfn = phys_to_pfn(pe->phys);

    /*
     * Set the page directory entries for this page table
     */
    pde = pmap_pd_entry(pmap, pe->offset);
    for (i = ptes_per_vm_page; i-- > 0; pde++) {
	*pde = template;
	template.pfn++;
    }
    
    PMAP_READ_UNLOCK(pmap, s);
}

/*
 * Allocate one additional mapping
 * in a page table.  Note: pmap is
 * already locked.
 */
static
void
pmap_allocate_mapping(
    pmap_t		pmap,
    vm_offset_t		va,
    boolean_t		wired
)
{
    pd_entry_t		*pde;
    pg_exten_t		*pe;
    
    pmap->stats.resident_count++;
    if (wired)
    	pmap->stats.wired_count++;
    
    if (pmap == kernel_pmap)
	return;
	
    pde = pmap_pd_entry(pmap, trunc_section(va));
    if (pde->valid) {
	pe = pg_desc(pfn_to_phys(pde->pfn))->pg_exten;
	pe->alloc_count++;
	if (wired)
	    pe->wired_count++;
    }
}

/*
 * Deallocate one or more mappings
 * in a page table.  The page table
 * is freed if no mappings remain.
 * Note: pmap is already locked.
 */
static
void
pmap_deallocate_mappings(
    pmap_t		pmap,
    vm_offset_t		va,
    int			count,
    int			unwire_count,
    boolean_t		free_table
)
{
    pd_entry_t		*pde;
    pg_exten_t		*pe;
    int			i;
    
    pmap->stats.wired_count -= unwire_count;
    pmap->stats.resident_count -= count;

    if (pmap == kernel_pmap)
	return;
	
    pde = pmap_pd_entry(pmap, trunc_section(va));
    if (pde->valid) {	
	pe = pg_desc(pfn_to_phys(pde->pfn))->pg_exten;
	if (pe->wired_count < unwire_count)
	    panic("pmap_deallocate_mappings unwire");
	pe->wired_count -= unwire_count;
	if (pe->alloc_count < count)
	    panic("pmap_deallocate_mappings");
	pe->alloc_count -= count;

	if (free_table && pe->alloc_count == 0) {
	    for (i = ptes_per_vm_page; i-- > 0; pde++)
		pde->valid = 0;
		
	    pt_active_remove(pe);
	    
	    pt_free_add(pe);
	}
    }
}

static
void
pmap_wire_mapping(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    pd_entry_t		*pde;
    pg_exten_t		*pe;
    
    pmap->stats.wired_count++;
    
    if (pmap == kernel_pmap)
	return;
	
    pde = pmap_pd_entry(pmap, trunc_section(va));
    if (pde->valid) {
	pe = pg_desc(pfn_to_phys(pde->pfn))->pg_exten;
	pe->wired_count++;
    }
}

static
void
pmap_unwire_mapping(
    pmap_t		pmap,
    vm_offset_t		va
)
{
    pd_entry_t		*pde;
    pg_exten_t		*pe;
    
    pmap->stats.wired_count--;
    
    if (pmap == kernel_pmap)
	return;
	
    pde = pmap_pd_entry(pmap, trunc_section(va));
    if (pde->valid) {
	pe = pg_desc(pfn_to_phys(pde->pfn))->pg_exten;
	pe->wired_count--;
    }
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(
    pmap_t		dst_pmap,
    pmap_t		src_pmap,
    vm_offset_t		dst_addr,
    vm_size_t		len,
    vm_offset_t		src_addr
)
{
    /* OPTIONAL */
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.  [This update includes
 * forcing updates of any address map caching.]
 *
 * Generally used to insure that a thread about
 * to run will see a semantically correct world.
 */
void
pmap_update(
    void
)
{
    pg_exten_t		*pe, *npe;
    vm_offset_t		page;
    pd_entry_t		*pde;
    int			tick_delta;
    static unsigned int	last_tick;

    if (!last_tick)
    	last_tick = sched_tick;	/* initialization */

    tick_delta = sched_tick - last_tick;

    if (tick_delta > 1) {
	/*
	 * Free all of the pages in
	 * the free page table list.
	 */	
	while (pe = pt_free_obtain()) {
	    page = pe->pg_desc->pv_list.va;
    
	    pg_exten_free(pe);
    
	    kmem_free(kernel_map, page, PAGE_SIZE);
	    pt_alloc_count--;
	}

	/*
	 * Run through the page directory
	 * free list, freeing any that are
	 * not in use.
	 */
	(queue_entry_t)pe = queue_first(&pd_free_queue);
	while (!queue_end(&pd_free_queue, (queue_entry_t)pe)) {
	    if (pe->alloc_count == 0) {
		remqueue(&pd_free_queue, (queue_entry_t)pe);
		
		pd_free_count--;
		
		page = pe->pg_desc->pv_list.va;
		
		pg_exten_free(pe);
		
		kmem_free(kernel_map, page, PAGE_SIZE);
		pd_alloc_count--;
		
		(queue_entry_t)pe = queue_first(&pd_free_queue);
	    }
	    else
		(queue_entry_t)pe = queue_next((queue_entry_t)pe);
	}
    }

    /*
     * Age the active page tables.  Remove
     * all of the mappings from the tables
     * which have not been used for address
     * translation recently (excluding those
     * with wired mappings).  Removing all of
     * a table's mappings causes the table to
     * be removed from the pmap and added to
     * the free page table list.
     */
    {
	static int	pt_aging_k[] = { 8, 12, 16, 24, 32 },
			pt_aging_n = sizeof pt_aging_k / sizeof pt_aging_k[0];
	int		tick_age;
	
	tick_age = ((tick_age = tick_delta >> 3) > (pt_aging_n - 1)) ?
						pt_aging_k[pt_aging_n - 1] :
						pt_aging_k[tick_age];	    

	(queue_entry_t)pe = queue_first(&pt_active_queue);
	while (!queue_end(&pt_active_queue, (queue_entry_t)pe)) {
	    pde = pmap_pd_entry(pe->pmap, pe->offset);
	    if (pe->wired_count == 0 && pde->valid) {
		(queue_entry_t)npe = queue_next((queue_entry_t)pe);
    
		if (tick_delta > 0 && !pde->refer) {
		    int		old_age = pe->unrefd_age;
    
		    pe->unrefd_age += tick_delta;
		    if (old_age > pe->unrefd_age || pe->unrefd_age > tick_age)
			pmap_remove(
				pe->pmap,
				pe->offset,
				pe->offset + section_size);
		}
		else if (pde->refer) {
		    pde->refer = FALSE;
		    pe->unrefd_age = 0;
		}
		    
		pe = npe;
	    }
	    else {
		pe->unrefd_age = 0;
		(queue_entry_t)pe = queue_next((queue_entry_t)pe);
	    }
	}
    }

    last_tick += tick_delta;
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 *
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(
    pmap_t		pmap
)
{
    /* OPTIONAL */
}

/*
 * Bind the given pmap to the given
 * processor.
 */
void
pmap_activate(
    pmap_t		pmap,
    thread_t		thread,
    int			cpu
)
{
    PMAP_ACTIVATE(pmap, thread, cpu);
}


/*
 * Indicates that the given physical map is no longer
 * in use on the specified processor.
 */
void
pmap_deactivate(
    pmap_t		pmap,
    thread_t		thread,
    int			cpu
)
{
    PMAP_DEACTIVATE(pmap, thread, cpu);
}

/*
 * Return the pmap handle for the kernel.
 */
pmap_t
pmap_kernel(
    void
)
{
    return (kernel_pmap);
}

/*
 * pmap_zero_page zeros the specified (machine independent)
 * page.
 */
void
pmap_zero_page(
    vm_offset_t		pa
)
{
    page_set(pmap_phys_to_kern(pa), 0, PAGE_SIZE);
}

/*
 * pmap_copy_page copies the specified (machine independent)
 * pages.
 */
void
pmap_copy_page(
    vm_offset_t		src,
    vm_offset_t		dst
)
{
    page_copy(pmap_phys_to_kern(dst), pmap_phys_to_kern(src), PAGE_SIZE);
}

void
copy_to_phys(
    vm_offset_t		src,
    vm_offset_t		dst,
    vm_size_t		count
)
{
    bcopy(src, pmap_phys_to_kern(dst), count);
}

void
copy_from_phys(
    vm_offset_t		src,
    vm_offset_t		dst,
    vm_size_t		count
)
{
    bcopy(pmap_phys_to_kern(src), dst, count);
}

/*
 * Make the specified pages (by pmap, offset)
 * pageable (or not) as requested.
 *
 * A page which is not pageable may not take
 * a fault; therefore, its page table entry
 * must remain valid for the duration.
 *
 * This routine is merely advisory; pmap_enter
 * will specify that these pages are to be wired
 * down (or not) as appropriate.
 */
pmap_pageable(
    pmap_t		pmap,
    vm_offset_t		start,
    vm_offset_t		end,
    boolean_t		pageable
)
{
    /* OPTIONAL */
}

kern_return_t
pmap_attribute(
    pmap_t			pmap,
    vm_offset_t			address,
    vm_size_t			size,
    vm_machine_attribute_t	attribute,
    vm_machine_attribute_val_t	*value
)
{
	kern_return_t ret;

	if (attribute != MATTR_CACHE)
		return KERN_INVALID_ARGUMENT;

	/*
	** We can't get the caching attribute for more than one page
	** at a time
	*/
	if ((*value == MATTR_VAL_GET) &&
	    (trunc_page(address) != trunc_page(address+size-1)))
		return KERN_INVALID_ARGUMENT;

	if (pmap == PMAP_NULL)
		return KERN_SUCCESS;

	ret = KERN_SUCCESS;

        simple_lock(&pmap->lock);

	switch (*value) {
	case MATTR_VAL_CACHE_FLUSH:     /* flush from all caches */
	case MATTR_VAL_DCACHE_FLUSH:    /* flush from data cache(s) */
	case MATTR_VAL_ICACHE_FLUSH:    /* flush from instr cache(s) */
		break;

	case MATTR_VAL_GET:             /* return current value */
	case MATTR_VAL_OFF:             /* turn attribute off */
	case MATTR_VAL_ON:              /* turn attribute on */
	default:
		ret = KERN_INVALID_ARGUMENT;
		break;
	}
	simple_unlock(&pmap->lock);

	return ret;

}

/*
 * Clear the modify bits on the specified physical page.
 */
void
pmap_clear_modify(
    vm_offset_t		pa
)
{
    if (managed_page(pa))
	pmap_clear_page_attrib(pa, PG_DIRTY);
}

/*
 * Return whether or not the specified physical page is modified
 * by any pmaps.
 */
boolean_t
pmap_is_modified(
    vm_offset_t		pa
)
{
    if (managed_page(pa))
	return (pmap_check_page_attrib(pa, PG_DIRTY));
    else
	return (FALSE);
}

/*
 * Clear the reference bit on the specified physical page.
 */
void
pmap_clear_reference(
    vm_offset_t		pa
)
{
    if (managed_page(pa))
	pmap_clear_page_attrib(pa, PG_REFER);
}

/*
 * Return whether or not the specified physical page is referenced
 * by any physical maps.
 */
boolean_t
pmap_is_referenced(
    vm_offset_t		pa
)
{
    if (managed_page(pa))
	return (pmap_check_page_attrib(pa, PG_REFER));
    else
	return (FALSE);
}

/*
 * Clear the specified page attributes both in the
 * pmap_page_attributes table and the address translation
 * tables.  Note that we DO have to flush the entries from
 * the TLB since the processor uses the bits in the TLB to
 * determine whether it has to write the bits out to memory.
 */
static
void
pmap_clear_page_attrib(
    vm_offset_t			pa,
    int				attrib
)
{
    pt_entry_t			*pte;
    pv_entry_t			pv_h;
    pmap_t			pmap;
    vm_offset_t			va;
    pg_desc_t			*pd;
    int				i, s;

    pd = pg_desc(pa);

    PMAP_WRITE_LOCK(s);

    pd->page_attrib &= ~attrib;

    pv_h = pg_desc_pvh(pd);

    while ((pmap = pv_h->pmap) != PMAP_NULL) {
	va = pv_h->va;

	simple_lock(&pmap->lock);

	PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);

	pte = pmap_pt_entry(pmap, va);
	if (pte != PT_ENTRY_NULL) {
	    for (i = ptes_per_vm_page; i-- > 0; pte++) {
		if (attrib & PG_DIRTY)
		    pte->dirty = 0;
		if (attrib & PG_REFER)
		    pte->refer = 0;
	    }
	}

	simple_unlock(&pmap->lock);

	if ((pv_h = pv_h->next) == PV_ENTRY_NULL)
	    break;
    }

    PMAP_WRITE_UNLOCK(s);
}

/*
 * Check for the specified attributes for the
 * physical page.  if all bits are true in
 * the pmap_page_attributes table, we can trust
 * it.  otherwise, we must check the address
 * translation tables ourselves.  Note that we
 * DO NOT have to flush the entry from the TLB
 * before looking at the address translation
 * table since the TLB is write-through for the bits.
 */
static
boolean_t
pmap_check_page_attrib(
    vm_offset_t			pa,
    int				attrib
)
{
    pt_entry_t			*pte;
    pv_entry_t			pv_h;
    pmap_t			pmap;
    vm_offset_t			va;
    pg_desc_t			*pd;
    int				i, s;

    pd = pg_desc(pa);

    if ((pd->page_attrib & attrib) == attrib)
	return (TRUE);

    pv_h = pg_desc_pvh(pd);

    PMAP_WRITE_LOCK(s);

    while ((pmap = pv_h->pmap) != PMAP_NULL) {
	va = pv_h->va;

	simple_lock(&pmap->lock);

	pte = pmap_pt_entry(pmap, va);
	if (pte != PT_ENTRY_NULL) {
	    for (i = ptes_per_vm_page; i-- > 0; pte++) {
		if (pte->dirty)
		    pd->page_attrib |= PG_DIRTY;
		if (pte->refer)
		    pd->page_attrib |= PG_REFER;
	    }

	    if ((pd->page_attrib & attrib) == attrib) {
		simple_unlock(&pmap->lock);

		PMAP_WRITE_UNLOCK(s);

		return (TRUE);
	    }
	}

	simple_unlock(&pmap->lock);

	if ((pv_h = pv_h->next) == PV_ENTRY_NULL)
	    break;
    }

    PMAP_WRITE_UNLOCK(s);

    return (FALSE);
}

/*
 * Dummy routine to satisfy external reference.
 */
void
pmap_update_interrupt(
    void
)
{
    /* should never be called. */
    panic("pmap_update_interrupt");
}

/*
 * Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(
    vm_offset_t			pa,
    vm_prot_t			prot
)
{
    switch (prot) {
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
	    pmap_copy_on_write(pa);
	    break;

	case VM_PROT_ALL:
	    break;

	default:
	    pmap_remove_all(pa);
	    break;
    }
}
