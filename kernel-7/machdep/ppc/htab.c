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
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

/* A marvelous selection of support routines for virtual memory */


#include <debug.h>
#include <mach_vm_debug.h>

#include <kern/thread.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <kernserv/machine/spl.h>

/*#include <kern/misc_protos.h>*/
/*#include <ppc/misc_protos.h>*/
/*#include <ppc/gdb_defs.h> /* for kgdb_kernel_in_pmap used in assert */
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/mem.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>

/* lion@apple.com 2/12/97 */
#define	NULL	((void *) 0)

/* These refer to physical addresses and are set and referenced elsewhere */

unsigned int hash_table_base;
unsigned int hash_table_size;

unsigned int hash_function_mask;

/* Prototypes for static functions */

static pte_t *find_pte_in_pteg(pte_t* pte, unsigned int match);

#if	MACH_VM_DEBUG
#define	DPRINTF(x)	kprintf(x)
#else	/* MACH_VM_DEBUG */
#define	DPRINTF(x)
#endif	/* MACH_VM_DEBUG */

/* gather statistics about hash table usage */

#if	DEBUG
#define MEM_STATS 1
#else
#define MEM_STATS 0
#endif /* DEBUG */

#if MEM_STATS
/* hash table usage information */
struct hash_table_stats {
	int find_pte_in_pteg_calls;
	int find_pte_in_pteg_not_found;
	int find_pte_in_pteg_location[8];
	struct find_or_alloc_calls {
		int found_primary;
		int found_secondary;
		int alloc_primary;
		int alloc_secondary;
		int overflow;
		int not_found;
	} find_or_alloc_calls[2];
	
} hash_table_stats;
#endif /* MEM_STATS */

/* Set up the machine registers for the given hash table.
 * The table has already been zeroed.
 */

void hash_table_init(unsigned int base, unsigned int size)
{
	register pte_t* pte;
	unsigned int mask;
	int i;

#if MACH_VM_DEBUG
	if (size % (64*1024)) {
		DPRINTF(("ERROR! size = 0x%x\n",size));
		enterDebugger("FATAL");
	}
	if (base % size) {
		DPRINTF(("ERROR! base not multiple of size\n"));
		DPRINTF(("base = 0x%08x, size=%dK \n",base,size/1024));
		enterDebugger("FATAL");
	}
#endif

#if PTE_EMPTY != 0
	/* We do not use zero values for marking empty PTE slots. This
	 * initialisation could be sped up, but it's only called once.
	 * the hash table has already been bzeroed.
	 */
	for (pte = (pte_t *) base; (unsigned int)pte < (base+size); pte++) {
		pte->pte0.word = PTE_EMPTY;
	}
#endif /* PTE_EMPTY != 0 */

	mask = 0;
	for (i = 1; i < (size >> 16); i *= 2)
		mask = (mask << 1) | 1;
	
	mtsdr1(hash_table_base | mask);
	
	isync();

	hash_function_mask = (mask << 16) | 0xFFC0;
}

/* Given the address of a pteg and the pte0 to match, this returns
 * the address of the matching pte or PTE_NULL. Can be used to search
 * for an empty pte by using PTE_EMPTY as the matching parameter.
 */

static __inline__ pte_t *
find_pte_in_pteg(pte_t* pte, unsigned int match)
{
	register int i;

#if	MEM_STATS
	hash_table_stats.find_pte_in_pteg_calls++;
#define INC_STAT(LOC) hash_table_stats.find_pte_in_pteg_location[LOC]++
#else	/* MEM_STATS */
#define INC_STAT(LOC)
#endif	/* MEM_STATS */

	/* unrolled for speed */
	if (pte[0].pte0.word == match) { INC_STAT(0); return &pte[0]; }
	if (pte[1].pte0.word == match) { INC_STAT(1); return &pte[1]; }
	if (pte[2].pte0.word == match) { INC_STAT(2); return &pte[2]; }
	if (pte[3].pte0.word == match) { INC_STAT(3); return &pte[3]; }
	if (pte[4].pte0.word == match) { INC_STAT(4); return &pte[4]; }
	if (pte[5].pte0.word == match) { INC_STAT(5); return &pte[5]; }
	if (pte[6].pte0.word == match) { INC_STAT(6); return &pte[6]; }
	if (pte[7].pte0.word == match) { INC_STAT(7); return &pte[7]; }

#if MEM_STATS
	hash_table_stats.find_pte_in_pteg_not_found++;
#endif /* MEM_STATS */
	return PTE_NULL;
}

/* Given a space identifier and a virtual address, this returns the
 * address of the pte describing this virtual page.
 *
 * If allocate is FALSE, the function will return PTE_NULL if not found
 *
 * If allocate is TRUE the function will create a new PTE entry marked
 *                 as invalid with only pte0 set up (ie no physical info or
 *                 protection/change info)
 */

pte_t *
find_or_allocate_pte(space_t sid, vm_offset_t v_addr, boolean_t allocate)
{
	register vm_offset_t	primary_hash,  secondary_hash;
	register pte0_t		primary_match, secondary_match;
	register pte_t		*pte;
	register unsigned int    segment_id;

	/* Horrible hack "(union *)&var-> " to retype to union */
	segment_id = (sid << 4) | (v_addr >> 28);

	primary_hash = segment_id ^ ((va_full_t *)&v_addr)->page_index;

	primary_hash = hash_table_base +
		((primary_hash << 6) & hash_function_mask);

	/* Firstly check out the primary pteg */

#if 0 && MACH_VM_DEBUG
	if (sid) {
		DPRINTF(("PRIMARY HASH OF SID 0x%x and addr 0x%x = 0x%x**********\n",
			 sid, v_addr, primary_hash));
	}
#endif

	/* TODO NMGS could be faster if all put on one line with shifts etc,
	 * does -O2 optimise this?
	 */

	primary_match.bits.valid 	= 1;
	/* virtual segment id = concat of space id + seg reg no */
	primary_match.bits.segment_id	= segment_id;
	primary_match.bits.hash_id	= 0;
	primary_match.bits.page_index	= ((va_abbrev_t *)&v_addr)->page_index;


	pte = find_pte_in_pteg((pte_t *) primary_hash, primary_match.word);

	if (pte != PTE_NULL) {
#if MEM_STATS
		hash_table_stats.find_or_alloc_calls[allocate].found_primary++;
#endif /* MEM_STATS */
		assert (pte->pte0.word != PTE_EMPTY);
		return pte;
	}

#if 0 && MACH_VM_DEBUG
	DPRINTF(("find_pte : searching secondary hash\n"));
#endif

	/* pte wasn't found in primary hash location, check secondary */

	secondary_match = primary_match;
	secondary_match.bits.hash_id = 1; /* Indicate secondary hash */
	
	secondary_hash = primary_hash ^ hash_function_mask;

	pte = find_pte_in_pteg((pte_t *) secondary_hash, secondary_match.word);

#if MEM_STATS
	if (pte != PTE_NULL)
		hash_table_stats.find_or_alloc_calls[allocate].found_secondary++;
	if ((pte == PTE_NULL) && !allocate)
		hash_table_stats.find_or_alloc_calls[allocate].not_found++;
#endif /* MEM_STATS */

	if ((pte == PTE_NULL) && allocate) {
		/* pte wasn't found - find one ourselves and fill in
		 * the pte0 entry (mark as invalid though?)
		 */
		pte = find_pte_in_pteg((pte_t *) primary_hash, PTE_EMPTY);
		if (pte != PTE_NULL) {
				/* Found in primary location, set up pte0 */

			primary_match.bits.valid = 0;
			pte->pte0.word = primary_match.word;
#if MEM_STATS
			hash_table_stats.find_or_alloc_calls[allocate].alloc_primary++;
#endif /* MEM_STATS */

		} else {
			pte = find_pte_in_pteg((pte_t *) secondary_hash,
					       PTE_EMPTY);
			if (pte != PTE_NULL) {
				/* Found in secondary */
				secondary_match.bits.valid = 0;
				pte->pte0.word = secondary_match.word;
#if MEM_STATS
				hash_table_stats.find_or_alloc_calls[allocate].alloc_secondary++;
#endif /* MEM_STATS */
			} else {
#if MEM_STATS
				hash_table_stats.find_or_alloc_calls[allocate].overflow++;
#endif /* MEM_STATS */
				/* Both PTEGs are full - we need to upcall
				 * to pmap to free up an entry, it will
				 * return the entry that has been freed up
				 */
#if DEBUG
				/* DPRINTF(("find_or_allocate : BOTH PTEGS ARE FULL\n")); */
#endif /* DEBUG */
				pte = pmap_pteg_overflow(
					       (pte_t *)primary_hash,
							primary_match,
					       (pte_t *)secondary_hash,
						        secondary_match);
				assert (pte != PTE_NULL);
			}
		}
		/* Set up the new entry - pmap code assumes:
		 * pte0 contains good translation but valid bit is reset
		 * pte1 has been initialised to zero
		 */
		assert(pte->pte0.word != PTE_EMPTY);
		pte->pte1.word = 0;

	}

	return pte; /* Either the address or PTE_NULL */
}

