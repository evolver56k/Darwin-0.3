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
 * Copyright (c) 1990 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: pmap.h 1.13 91/09/25$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSS, 9/90
 */

/*
 *	Pmap header for ppc
 */

#ifndef	_PPC_PMAP_H_
#define	_PPC_PMAP_H_

/*#include <mach/vm_types.h>*/
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/machine/kern_return.h>
#include <kern/lock.h>
/*#include <kern/queue.h>*/

/* The size of the hash table and mapping entries and the size
 * of the VM needed by the linux server is proportional
 * to the amount of physical memory. The hash table and mapping
 * entries must be allocated in contiguous physical memory.
 * Currently, the limits are set up so that 128M is the ceiling
 */

#define MAX_SUPPORTED_PHYSICAL_MEMORY	(128*1024*1024)

struct pmap {
	decl_simple_lock_data(,	lock)	     /* Lock on map */
	int			ref_count;   /* reference count */
	space_t			space;	     /* space for this pmap */
	struct pmap		*next;	     /* linked list of free pmaps */
	struct pmap_statistics	stats;	     /* statistics */
};

typedef struct pmap *pmap_t;

#define PMAP_NULL  ((pmap_t) 0)

extern pmap_t	kernel_pmap;			/* The kernel's map */

#define	PMAP_SWITCH_USER(th, map, my_cpu) th->map = map;	

#define PMAP_ACTIVATE(pmap, th, cpu)
#define PMAP_DEACTIVATE(pmap, th, cpu)
#define PMAP_CONTEXT(pmap,th)

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= VM_MAX_KERNEL_ADDRESS))

#define	PPC_SID_KERNEL  0       /* Must change KERNEL_SEG_REG0_VALUE if !0 */
#define SID_MAX	((1<<20) - 1)	/* Space ID=20 bits, segment_id=SID + 4 bits */
#define PPC_SID_PRIME 356299	/* generate non-repetative "different" sids */
#define PPC_SID_MASK  0xfffff   /* within a 20-bit field */

#define pmap_kernel()			(kernel_pmap)
#define	pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_remove_attributes(pmap,start,end)
#define pmap_copy(dpmap,spmap,da,len,sa)

#define pmap_phys_address(x)	((x) << PPC_PGSHIFT)
#define pmap_phys_to_frame(x)	((x) >> PPC_PGSHIFT)

#define PMAP_PTOB(x)		((x) << PPC_PGSHIFT)

/* 
 * prototypes.
 */
extern void 		ppc_protection_init(void);
extern vm_offset_t	kvtophys(vm_offset_t addr);
extern vm_offset_t	phystokv(vm_offset_t addr);
extern vm_offset_t	pmap_map(vm_offset_t va,
				 vm_offset_t spa,
				 vm_offset_t epa,
				 vm_prot_t prot);
extern kern_return_t    pmap_add_physical_memory(vm_offset_t spa,
						 vm_offset_t epa,
						 boolean_t available,
						 unsigned int attr);
extern vm_offset_t	pmap_map_bd(vm_offset_t va,
				    vm_offset_t spa,
				    vm_offset_t epa,
				    vm_prot_t prot);
extern void		pmap_bootstrap(unsigned int mem_size,
				       vm_offset_t *first_avail);
extern void		pmap_block_map(vm_offset_t pa,
				       vm_size_t size,
				       vm_prot_t prot,
				       int entry, 
				       int dtlb);
extern void		pmap_switch(pmap_t);

extern vm_offset_t pmap_extract(pmap_t pmap,
				vm_offset_t va);

extern void pmap_remove_all(vm_offset_t pa);

extern boolean_t pmap_verify_free(vm_offset_t pa);

extern void flush_cache(vm_offset_t pa, unsigned length);
extern void flush_cache_v(vm_offset_t pa, unsigned length);
extern void invalidate_cache_v(vm_offset_t pa, unsigned length);
typedef enum {
    cache_default,
    cache_writethrough,
    cache_disable
} cache_spec_t;

void pmap_enter_cache_spec(
		pmap_t		pmap,
		vm_offset_t	va,
		vm_offset_t	pa,
		vm_prot_t	prot,
		boolean_t	wired,
		cache_spec_t	caching);

#endif /* _PPC_PMAP_H_ */

