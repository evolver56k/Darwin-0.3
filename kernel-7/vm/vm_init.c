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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	vm/vm_init.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Initialize the Virtual Memory subsystem.
 */

#import <mach_xp.h>

#import <mach/machine/vm_types.h>
#import <kern/lock.h>
#import <vm/vm_object.h>
#import <vm/vm_map.h>
#import <vm/vm_page.h>
#import <vm/vm_kern.h>
#import <vm/vnode_pager.h>

/*
 *	vm_init initializes the virtual memory system.
 *	This is done only by the first cpu up.
 *
 *	The start and end address of physical memory is passed in.
 */

void vm_mem_init()
{
	extern vm_offset_t	virtual_avail, virtual_end;

	/*
	 *	Initializes resident memory structures.
	 *	From here on, all physical memory is accounted for,
	 *	and we use only virtual addresses.
	 */

	virtual_avail = vm_page_startup(
	    mem_region, num_regions, virtual_avail);

#ifdef ppc
	virtual_avail = adjust_bat_limit(virtual_avail, 0, TRUE, TRUE);
#endif

	/*
	 *	Initialize other VM packages
	 */

	zone_bootstrap();
	vm_object_init();
	vm_map_init();
	kmem_init(virtual_avail, virtual_end);
	pmap_init(mem_region, num_regions);

	zone_init();
	kalloc_init();

#if	MACH_XP
#else	MACH_XP
	vm_user_init();
#endif	MACH_XP
}
