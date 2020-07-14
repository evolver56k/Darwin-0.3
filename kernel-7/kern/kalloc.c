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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	kern/kalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	General kernel memory allocator.  This allocator is designed
 *	to be used by the kernel to manage dynamic memory fast.
 */

#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>

#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>

vm_map_t kalloc_map;

/*
 *	All allocations of size less than kalloc_max are rounded to the
 *	next highest power of 2.  This allocator is built on top of
 *	the zone allocator.  A zone is created for each potential size
 *	that we are willing to get in small blocks.
 *
 *	We assume that kalloc_max is not greater than 64K;
 *	thus 16 is a safe array size for k_zone and k_zone_name.
 */

#define	NKSIZE	16
vm_size_t k_zone_maxsize;
struct zone *k_zone[NKSIZE];
static char k_zone_name[NKSIZE][16];
vm_size_t k_zone_elemsize[NKSIZE] = {
	16, 32, 48, 64, 80, 128, 256, 384, 512,
	1024, 2048, 3072, 4096, 8192, 12288, 16384
};

#if DIAGNOSTIC
/*
 * Be careful with ZALLOC0!  It seems that kdp now has a subtle dependency
 * on allocated memory not being initialized.  If you do use it you'll need
 * to enter gdb with cmd-Power and even then you may see a "SIGTRAP".
 */
#if ZALLOC0
void    *memset __P((void *, int, size_t));
#endif
#endif


/*
 *	Initialize the memory allocator.  This should be called only
 *	once on a system wide basis (i.e. first processor to get here
 *	does the initialization).
 *
 *	This initializes all of the zones.
 */

void kalloc_init(void)
{
	vm_size_t size;
	register int i;
	
	kalloc_map = kernel_map;

	/*
	 *	Allocate a zone for each size we are going to handle.
	 *	We specify non-paged memory.
	 */
	for (i = 0; i < NKSIZE; i++) {
		if ((size = k_zone_elemsize[i]) >= PAGE_SIZE)
			break;
		sprintf (k_zone_name[i], "kalloc.%d", size);
		k_zone[i] = zinit(size, 1024*1024, PAGE_SIZE,
			FALSE, k_zone_name[i]);
		k_zone_maxsize = size;
	}
}

vm_offset_t kalloc_noblock(size)
	vm_size_t size;
{
	register int zindex = 0;
	register vm_size_t allocsize;
	vm_offset_t addr;

	/* compute the size of the block that we will actually allocate */

	allocsize = size;
	if (size <= k_zone_maxsize) {
		allocsize = k_zone_elemsize[0];
		zindex = 0;
		while (allocsize < size) {
			allocsize = k_zone_elemsize[++zindex];
		}
	}

	/*
	 * If our size is still small enough, check the queue for that size
	 * and allocate.
	 */

	if (allocsize <= k_zone_maxsize) {
		addr = zalloc_noblock(k_zone[zindex]);
#if DIAGNOSTIC
#if ZALLOC0
		(void) memset((void *)addr, 0, (size_t) size);
#endif
#endif
	} else {
		if (kmem_alloc_zone(kalloc_map, &addr, allocsize, FALSE)
							    != KERN_SUCCESS)
			addr = 0;
	}

	return(addr);
}

vm_offset_t kalloc(size)
	vm_size_t size;
{
	register int zindex = 0;
	register vm_size_t allocsize;
	vm_offset_t addr;

	/* compute the size of the block that we will actually allocate */
	allocsize = size;
	if (size <= k_zone_maxsize) {
		allocsize = k_zone_elemsize[0];
		zindex = 0;
		while (allocsize < size) {
			allocsize = k_zone_elemsize[++zindex];
		}
	}

	/*
	 * If our size is still small enough, check the queue for that size
	 * and allocate.
	 */

	if (allocsize <= k_zone_maxsize) {
		addr = zalloc(k_zone[zindex]);
#if DIAGNOSTIC
#if ZALLOC0
		(void) memset((void *)addr, 0, (size_t) size);
#endif
#endif
	} else {
		if (kmem_alloc_wired(kalloc_map, &addr, allocsize)
							!= KERN_SUCCESS)
			addr = 0;
	}
	return(addr);
}

vm_offset_t kget(size)
	vm_size_t size;
{
	register int zindex = 0;
	register vm_size_t allocsize;
	vm_offset_t addr = 0;

	/* compute the size of the block that we will actually allocate */
	allocsize = size;
	if (size <= k_zone_maxsize) {
		allocsize = k_zone_elemsize[0];
		zindex = 0;
		while (allocsize < size) {
			allocsize = k_zone_elemsize[++zindex];
		}
	}

	/*
	 * If our size is still small enough, check the queue for that size
	 * and allocate.
	 */

	if (allocsize <= k_zone_maxsize) {
		addr = zget(k_zone[zindex]);
	} else {
		/* This will never work, so we might as well panic */
		panic("kget");
	}
	return(addr);
}

void 
kfree(data, size)
	vm_offset_t data;
	vm_size_t size;
{
	register int zindex = 0;
	register vm_size_t freesize;

	freesize = size;
	if (size <= k_zone_maxsize) {
		freesize = k_zone_elemsize[0];
		zindex = 0;
		while (freesize < size) {
			freesize = k_zone_elemsize[++zindex];
		}
	}

	if (freesize <= k_zone_maxsize) {
		zfree(k_zone[zindex], data);
	} else {
		kmem_free(kalloc_map, data, freesize);
	}
}

struct zone *kalloc_zone(
	vm_size_t	size)
{
	register int zindex = 0;
	register vm_size_t allocsize;

	/* compute the size of the block that we will actually allocate */

	allocsize = size;
	if (size <= k_zone_maxsize) {
		allocsize = k_zone_elemsize[0];
		zindex = 0;
		while (allocsize < size) {
			allocsize = k_zone_elemsize[++zindex];
		}

		return (k_zone[zindex]);
	}

	return (0);
}
