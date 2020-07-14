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

/* Things that don't need to be exported from pmap. Putting
 * them here and not in pmap.h avoids major recompiles when
 * modifying something either here or in proc_reg.h
 */

#ifndef _PMAP_INTERNALS_H_
#define _PMAP_INTERNALS_H_

/* #include <mach/vm_types.h> */
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <kern/queue.h>
#include <machdep/ppc/proc_reg.h>

/* Page table entries are stored in groups (PTEGS) in a hash table */

#if __PPC__
#if _BIG_ENDIAN == 0
error - bitfield structures are not checked for bit ordering in words
#endif /* _BIG_ENDIAN */
#endif /* __PPC__ */

/*
 * Don't change these structures unless you change the assembly code in
 * locore.s
 */
struct mapping {
	queue_head_t	phys_link;	/* for mappings of a given PA */

	union {
		unsigned int word;
		struct {
			vm_offset_t	page:28;/* virtual page number */
			unsigned int    wired:1;  /* boolean */
			unsigned int    phys:1;   /* boolean */
			unsigned int	:0;
		} bits;
	} vm_info;

	pte_t	        *pte;		/* pointer to pte in hash table */
	struct pmap	*pmap;		/* pmap mapping belongs to */
};

#define MAPPING_NULL	((struct mapping *) 0)

struct phys_entry {
	queue_head_t	phys_link;	/* head of mappings of a given PA */
	pte1_t		pte1;		/* referenced/changed/wimg info */
};

#define PHYS_NULL	((struct phys_entry *)0)

/* Memory may be non-contiguous. This data structure contains info
 * for mapping this non-contiguous space into the contiguous
 * physical->virtual mapping tables. An array of this type is
 * provided to the pmap system at bootstrap by ppc_vm_init.
 *
 * NB : regions must be in order in this structure.
 */

typedef struct pmap_mem_region {
	vm_offset_t start;	/* Address of base of region */
	struct phys_entry *phys_table; /* base of region's table */
	unsigned int end;       /* End address+1 */
} pmap_mem_region_t;

/* MEM_REGION_MAX has a PowerMac dependancy - at least the value of
 * kMaxRAMBanks in ppc/POWERMAC/nkinfo.h
 */
#define MEM_REGION_MAX 26

extern pmap_mem_region_t pmap_mem_regions[MEM_REGION_MAX];
extern int          pmap_mem_regions_count;

/* keep track of free regions of physical memory so that we can offer
 * them up via pmap_next_page later on
 */

#define FREE_REGION_MAX 8
extern pmap_mem_region_t free_regions[FREE_REGION_MAX];
extern int          free_regions_count;

/* Prototypes */

struct phys_entry *pmap_find_physentry(vm_offset_t pa);

extern pte_t *pmap_pteg_overflow(pte_t *primary_hash, pte0_t primary_match,
				 pte_t *secondary_hash, pte0_t secondary_key);

#endif /* _PMAP_INTERNALS_H_ */
