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
//#include <mach_debug.h>
//#include <debug.h>

#include <mach/machine/vm_types.h>
//#include <mach/vm_param.h>
#include <kern/thread.h>
//#include <mach/thread_status.h>
//#include <kern/mach_header.h>
//#include <mach-o/loader.h>
//#include <kern/assert.h>

#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/boot.h>
#include <vm/pmap.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>
#include <kern/ast.h>
#include <mach/ppc/exception.h>
#include <machdep/ppc/exception.h>
#import <machdep/ppc/kernBootStruct.h>

#include <machdep/ppc/cpu_data.h>

#include "powermac_exceptions.h"
#include "powermac.h"
#import "boot.h"

/* struct ppc_thread_state boot_task_thread_state; */

cpu_data_t cpu_data[NCPUS];

/* per_proc_info is accessed with VM switched off via sprg0 */

struct per_proc_info per_proc_info[NCPUS];

vm_offset_t mem_size;  /* Size of actual physical memory present in bytes */

pmap_mem_region_t pmap_mem_regions[MEM_REGION_MAX];
int	 pmap_mem_regions_count = 0;	/* No non-contiguous memory regions */

pmap_mem_region_t free_regions[FREE_REGION_MAX];
int	     free_regions_count;

struct mem_region       mem_region[MEM_REGION_MAX];
int                     num_regions;

vm_offset_t virtual_avail, virtual_end;

extern struct mach_header _mh_execute_header;

/* It is called with VM
 * switched ON, with the bottom 2M of memory mapped into KERNELBASE_TEXT
 * via BAT0, and the region in 2-4M mapped 1-1 (KERNELBASE_DATA) via BAT1
 *
 * The IO space is mapped 1-1 either via a segment register or via
 * BAT2, depending upon the processor type.
 *
 * printf already works
 * 
 * First initialisation of memory.
 *  - zero bss,
 *  - invalidate some seg regs,
 *  - set up hash tables
 */

void ppc_vm_init(unsigned int mem_limit, boot_args* args)
{
	vm_offset_t	first_avail;
	unsigned int	i;

	/* Go through the list of memory regions passed in via the args
	 * and copy valid entries into the pmap_mem_regions table, adding
	 * further calculated entries.
	 */
	
	pmap_mem_regions_count = 0;
	mem_size = 0;	/* Will use to total memory found so far */

#warning use the kernboot stucture equivalent here
	for (i = 0; i < kMaxDRAMBanks; i++) {
	    	if (mem_limit > 0 && mem_size >= mem_limit)
		    	break;

		if (args->PhysicalDRAM[i].size == 0)
			continue;

		/* The following should only happen if memory size has
		   been artificially reduced with -m */
		if (mem_limit > 0 &&
		    mem_size + args->PhysicalDRAM[i].size > mem_limit)
			args->PhysicalDRAM[i].size = mem_limit - mem_size;

		/* We've found a region, tally memory */

		pmap_mem_regions[pmap_mem_regions_count].start =
			args->PhysicalDRAM[i].base;
		pmap_mem_regions[pmap_mem_regions_count].end =
			args->PhysicalDRAM[i].base +
			args->PhysicalDRAM[i].size;

		/* Regions must be provided in ascending order */
		assert ((pmap_mem_regions_count == 0) ||
			pmap_mem_regions[pmap_mem_regions_count].start >
			pmap_mem_regions[pmap_mem_regions_count-1].start);
		
		/* Keep track of how much memory we've found */

		mem_size += args->PhysicalDRAM[i].size;

		/* incremement number of regions found */
		pmap_mem_regions_count++;
	}

	printf("mem_size = %d M\n",mem_size / (1024 * 1024));

	/* Set up the various globals describing memory usage */
	free_regions_count = 0;

	if (args->deviceTreeP != 0)
		first_avail = round_page(args->topOfKernelData);
	else
		first_avail = round_page(getlastaddr());

#ifdef notdef
	printf("I. first_avail %x mem_size %x\n", first_avail, mem_size);
	dump_regions();
#endif

	startup_early(&first_avail);

#ifdef notdef
	printf("Ia. first_avail %x\n", first_avail);
#endif

	pmap_bootstrap(mem_size, &first_avail);

	/*
	 * Copy our memory description into vm's version
	 */
	for (i=0; i < free_regions_count; i++ ) {
		mem_region[i].base_phys_addr = free_regions[i].start;
		mem_region[i].first_phys_addr = free_regions[i].start;
		mem_region[i].last_phys_addr = free_regions[i].end;
	}
	num_regions = free_regions_count;

#ifdef notdef
	printf("II. first_avail %x mem_size %x\n", first_avail, mem_size);
	dump_regions();
#endif

	virtual_avail = round_page(first_avail);
	virtual_end   = VM_MAX_KERNEL_ADDRESS;

	/* Set up per_proc info */
	per_proc_info[cpu_number()].phys_exception_handlers = (unsigned int)
		&exception_handlers;
	per_proc_info[cpu_number()].virt_per_proc_info = (unsigned int)
		&per_proc_info[cpu_number()];
	per_proc_info[cpu_number()].cpu_data = (unsigned int)
		&cpu_data[cpu_number()];
	per_proc_info[cpu_number()].active_stacks = (unsigned int)
		&active_stacks[cpu_number()];
	per_proc_info[cpu_number()].need_ast = (unsigned int)
		&need_ast[cpu_number()];
	per_proc_info[cpu_number()].fpu_pcb = (pcb_t) 0;

	/*
	 * save per_proc_info in SPRG0 so interrupt handler can see it!
	 */
	mtsprg(0, ((unsigned int)&per_proc_info));

#if MACH_KGDB
	kgdb_kernel_in_pmap = TRUE;
#endif /* MACH_KGDB */

	sync();isync();
	if (PROCESSOR_VERSION != PROCESSOR_VERSION_601)
		flush_cache(hash_table_base, hash_table_size);
	else {
		mtibatl(3,BAT_INVALID);
		isync();
		mtibatl(2,BAT_INVALID);
		isync();
		mtibatl(1,BAT_INVALID);
		isync();
		mtibatl(0,BAT_INVALID);
		isync();
	}
	isync();
}
