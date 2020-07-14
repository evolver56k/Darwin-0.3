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
 * Intel386 Family:	Definitions for pmap.
 *
 * HISTORY
 *
 * 15 December 1992 ? at NeXT
 *	Removed ldt from pmap.
 *
 * 22 August 1992 ? at NeXT
 *	Moved 'section' definition over to pmap_private.h.
 *
 * 9 April 1992 ? at NeXT
 *	Created.
 */

#import <mach/vm_param.h>
#import <mach/vm_prot.h>
#import <mach/vm_statistics.h>

#import <kern/lock.h>

typedef struct pt_entry {
    unsigned int	valid	:1,
    			prot	:2,
#define PT_PROT_KR	0
#define PT_PROT_KRW	1
#define PT_PROT_UR	2
#define PT_PROT_URW	3
			cachewrt:1,	/* 486 specific */
			cachedis:1,	/* 486 specific */
			refer	:1,
			dirty	:1,
				:2,
			wired	:1,
				:1,
				:1,
			pfn	:20;
} pt_entry_t;

#define PT_ENTRY_NULL	((pt_entry_t *) 0)

typedef struct pd_entry {
    unsigned int	valid	:1,
    			prot	:2,
				:2,
			refer	:1,
				:3,
				:3,
			pfn	:20;
} pd_entry_t;

#define PD_ENTRY_NULL	((pd_entry_t *) 0)

/*
 * This is the offset of kernel memory
 * in the address translation trees.
 */
#define KERNEL_LINEAR_BASE	VM_MAX_ADDRESS

typedef struct pmap {
    pd_entry_t *	root;	/* virtual address used for walking tree */
    unsigned int	cr3;	/* actual value for cr3 */
    int			ref_count;
    simple_lock_data_t	lock;
    struct pmap_statistics
    			stats;
    unsigned int	cpus_using;
} *pmap_t;

#define PMAP_NULL	((pmap_t) 0)

/*
 * With only one CPU, we just have to indicate whether the pmap is
 * in use.
 */
#define PMAP_ACTIVATE(pmap, thread, my_cpu)	\
MACRO_BEGIN					\
    (pmap)->cpus_using = TRUE;			\
MACRO_END

#define PMAP_DEACTIVATE(pmap, thread, cpu)	\
MACRO_BEGIN					\
    (pmap)->cpus_using = FALSE;			\
MACRO_END

/*
 * PMAP_CONTEXT: update any thread-specific hardware context that
 * is managed by pmap module.
 */
#define PMAP_CONTEXT(pmap, thread)

#define pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_phys_address(frame)	((vm_offset_t) (i386_ptob(frame)))
#define pmap_phys_to_frame(phys)	((unsigned int) (i386_btop(phys)))
#define pmap_phys_to_kern(phys)		((phys) + VM_MIN_KERNEL_ADDRESS)

typedef enum {
    cache_default,
    cache_writethrough,
    cache_disable
} cache_spec_t;

extern inline
    pt_entry_t *
	pmap_pt_entry(
		pmap_t		pmap,
		vm_offset_t	va);

extern inline
    pd_entry_t *
	pmap_pd_entry(
		pmap_t		pmap,
		vm_offset_t	va);

extern inline
    void
	pmap_enter_cache_spec(
		pmap_t		pmap,
		vm_offset_t	va,
		vm_offset_t	pa,
		vm_prot_t	prot,
		boolean_t	wired,
		cache_spec_t	caching);

extern
    void
    	pmap_enter_shared_range(
		pmap_t		pmap,
		vm_offset_t	va,
		vm_size_t	size,
		vm_offset_t	kern);
