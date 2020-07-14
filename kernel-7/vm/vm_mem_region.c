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

/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	vm/vm_mem_region.c
 *
 *	This file contains machine independent code for managing
 *	physical memory regions.
 *
 * 18-Mar-91  Mike DeMoney and Peter King (mike@next.com)
 *	Created.
 */
#import	<kern/assert.h>
#import <mach/vm_param.h>
#import	<vm/vm_page.h>
#import <kernserv/macro_help.h>
#import <kern/assert.h>

/*
 * vm_mem_ppi -- convert a physical page number into a phys page index.
 * Returns the page ordinal in the set of "populated" pages.
 */
unsigned vm_mem_ppi(vm_offset_t pa)
{
	mem_region_t	region;
	unsigned	pages_so_far;

	pages_so_far = 0;
	for (region = &mem_region[0]; region < &mem_region[num_regions];
	     region++) {
		if (pa >= region->first_phys_addr
		    && pa < region->last_phys_addr) {
			return (pages_so_far +
				atop(pa - region->first_phys_addr));
		}
		ASSERT(region->num_pages == atop(region->last_phys_addr
						- region->first_phys_addr));
		pages_so_far += region->num_pages;
	}
	panic("mem_ppi");
	/*NOTREACHED*/
}

boolean_t vm_valid_page(
	vm_offset_t	pa
) {
	mem_region_t	region;

	for (region = &mem_region[0]; region < &mem_region[num_regions];
	     region++) {
		if (pa >= region->first_phys_addr
		    && pa < region->last_phys_addr) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *	vm_phys_to_vm_page:
 *
 *	Translates physical address to vm_page pointer.
 */
vm_page_t vm_phys_to_vm_page (vm_offset_t pa)
{
	mem_region_t	rp;
	
	for (rp = mem_region; rp < &mem_region[num_regions]; rp += 1) {
		if (pa >= rp->first_phys_addr && pa < rp->last_phys_addr) {
			return &rp->vm_page_array[atop(pa) - rp->first_page];
		}
	}
	return 0;
}

/*
 *	vm_region_to_vm_page:
 *
 *	Translates physical address to vm_page pointer.
 */
vm_page_t
vm_region_to_vm_page (pa)
	register vm_offset_t	pa;
{
	return (vm_phys_to_vm_page(pa));
}

vm_offset_t vm_alloc_from_regions(
	vm_size_t size,
	vm_size_t align
) {
	vm_offset_t	first_addr;
	mem_region_t	rp;
	extern
	    vm_offset_t	virtual_avail;

	/* Alignment must be power of 2 */
	ASSERT((align & (align - 1)) == 0);

	/*
	 * Search for region with enough space.
	 * Simple first fit for now.
	 */
	for (rp = mem_region; rp < &mem_region[num_regions]; rp += 1) {

		/* Align current first_addr to requested alignment */
		first_addr = ((rp->first_phys_addr + (align - 1))
			      & ~(align - 1));

		if (first_addr + size <= rp->last_phys_addr) {
			rp->first_phys_addr = first_addr + size;
#ifdef ppc
			if (round_page(rp->first_phys_addr) > virtual_avail)
			    virtual_avail = round_page(rp->first_phys_addr);
#endif

			return (vm_offset_t) first_addr;
		}
	}
	panic("vm_mem_alloc_from_regions");
	/*NOTREACHED*/
}
