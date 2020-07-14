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
 * Intel386 Family:	Inline expansions for pmap.
 *
 * HISTORY
 *
 * 9 April 1992 ? at NeXT
 *	Created.
 */

/*
 * Convert a physical address
 * into an absolute page index.
 */
static inline
unsigned int
phys_to_pfn(
    unsigned int	phys
)
{
    union {
	unsigned int	phys;
	struct {
	    unsigned int		:12,
	    			pfn	:20;
	}		fields;
    } conv;
    
    conv.phys = phys;
    
    return (conv.fields.pfn);
}

/*
 * Return the address corresponding
 * the an absolute physical page
 * index.
 */
static inline
unsigned int
pfn_to_phys(
    unsigned int	pfn
)
{
    union {
	unsigned int	phys;
	struct {
	    unsigned int		:12,
	    			pfn	:20;
	}		fields;
    } conv;
    
    conv.phys = 0;
    conv.fields.pfn = pfn;
    
    return (conv.phys);
}

/*
 * Return the page offset part
 * of an address, virtual or
 * physical.
 */
static inline
unsigned int
page_offset(
    unsigned int	addr	/* XXX vm or phys */
)
{
    union {
	unsigned int	addr;
	struct {
	    unsigned int	offset	:12,
	    				:20;
	}		fields;
    } conv;
    
    conv.addr = addr;
    
    return (conv.fields.offset);
}

/*
 * Given a ptr to a page
 * directory and a virtual
 * address, return a ptr to
 * the corresponding
 * page directory entry.
 */
static inline
pd_entry_t *
pd_to_pd_entry(
    pd_entry_t		p[],
    vm_offset_t		v
)
{
    union {
	vm_offset_t	virt;
	struct {
	    unsigned int		:12,
	    				:10,
				index	:10;
	}		fields;
    } conv;
    
    conv.virt = v;
    
    return (&p[conv.fields.index]);
}

/*
 * Given a ptr to a page
 * table and a virtual
 * address, return a ptr to
 * the corresponding
 * page table entry.
 */
static inline
pt_entry_t *
pt_to_pt_entry(
    pt_entry_t		p[],
    vm_offset_t		v
)
{
    union {
	vm_offset_t	virt;
	struct {
	    unsigned int		:12,
	    			index	:10,
					:10;
	}		fields;
    } conv;
    
    conv.virt = v;
    
    return (&p[conv.fields.index]);
}

/*
 * Return the virtual address
 * of the page table referenced
 * by a page directory entry.
 */
static inline
pt_entry_t *
pd_entry_to_pt(
    pd_entry_t		*p
)
{
    return ((pt_entry_t *)(pfn_to_phys(p->pfn) +
				    VM_MIN_KERNEL_ADDRESS));
}

/*
 * Given  a ptr to a valid page
 * directory entry and a virtual
 * address, walk to the corresponding
 * page table entry, and return a ptr
 * to it.
 */
static inline
pt_entry_t *
pd_entry_to_pt_entry(
    pd_entry_t		*p,
    vm_offset_t		v
)
{
    return (pt_to_pt_entry(pd_entry_to_pt(p), v));
}
