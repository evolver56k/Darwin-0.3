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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * ev.c - Machine dependent code needed to support the Event Driver.
 *
 * HISTORY
 * 22 Sep 92	Joe Pasqua 
 *      Created. 
 */
 
#import <mach/mach_types.h>
 
#import <mach/kern_return.h>
#import <kern/kern_port.h>
#import <vm/vm_kern.h>
#import <driverkit/return.h>

#ifndef	NULL
#define NULL ((void *)0)
#endif

/*
 * NULL terminated list of event source classes that should be attached
 * to the Event Driver when the Window Server starts up.
 * The list is machine dependent.
 */
static const char *defaultEventSrc[] = 
{
	"EventSrcPCPointer",
	"EventSrcPCKeyboard",
	NULL
};

/*
 * Return a NULL terminated list of event sources that should be attached
 * to the Event Driver when the Window Server starts up.
 */
const char **
defaultEventSources()
{
	return defaultEventSrc;
}

/*
 * Set up the shared memory area between the Window Server and the kernel.
 *
 *	Obtain page aligned wired kernel memory for 'size' bytes using
 *	kmem_alloc().
 *	Find a similar sized region in the Window Server task VM map using
 *	vm_map_find().  This function will find an appropriately sized region,
 *	create a memory object, and insert it in the VM map.
 *	For each physical page in the kernel's wired memory we got from
 *	kmem_alloc(), enter that page at the appropriate location in the page
 *	map for the Window Server, in the address range we allocated using
 *	vm_map_find().  
 */
 kern_return_t
createEventShmem(	port_t task,			// in
			vm_size_t size,			// in
			struct vm_map **owner,		// out
			vm_offset_t *owner_addr,	// out
			vm_offset_t *shmem_addr	)	// out
{
	vm_offset_t	off;
	vm_offset_t	phys_addr;
	kern_return_t	krtn;
	vm_map_t	task_map;
	kern_port_t	task_port;
	vm_size_t	shmem_size;
	extern vm_map_t convert_port_to_map(kern_port_t);

	*owner = VM_MAP_NULL;
	// Get the task map
	if ( (task_port = (kern_port_t)IOGetKernPort(task)) == KERN_PORT_NULL )
	    return KERN_INVALID_ARGUMENT;
	if ( (task_map = convert_port_to_map(task_port)) == VM_MAP_NULL )
	{
	    port_release(task_port);	// Decrement ref cnt
	    return KERN_INVALID_ARGUMENT;
	}
	port_release(task_port);	// Decrement ref cnt

	shmem_size = round_page(size);	// Round up to page boundry.

	/* Find some memory of a suitable size in the kernel VM map. */
	if ( kmem_alloc_wired( kernel_map, shmem_addr, shmem_size ) )
	{
	    vm_map_deallocate(task_map);	// discard our reference
	    return KERN_NO_SPACE;
	}
	/* Find some memory of the same size in 'task'.  We use vm_map_find()
	   to do this. vm_map_find inserts the found memory object in the
	   target task's map as a side effect. */
	*owner_addr = vm_map_min(task_map);
	krtn = vm_map_find( task_map,
		VM_OBJECT_NULL,
		0,				// offset
		owner_addr,
		shmem_size,
		TRUE );				// Find first fit
	if(krtn)
	{
	    IOLog("createEventShmem: vm_map_find() returned %d\n", krtn);
	    vm_map_deallocate(task_map);	// discard our reference
	    return KERN_NO_SPACE;
	}
	/* For each page in the area allocated from the kernel map,
	   	find the physical address of the page.
		Enter the page in the target task's pmap, at the
		appropriate target task virtual address. */
	for ( off = 0; off < shmem_size; off += PAGE_SIZE )
	{
	    phys_addr = pmap_extract( kernel_pmap, (*shmem_addr) + off );
	    if ( phys_addr == 0 )
	    {
		IOLog("createEventShmem: no paddr for vaddr 0x%x\n",
			(*shmem_addr) + off );
		kmem_free( kernel_map, *shmem_addr, size );
		vm_map_deallocate(task_map);	// discard our reference
		return KERN_NO_SPACE;
	    }
	    pmap_enter(
		task_map->pmap,
		*owner_addr + off,
		phys_addr,
		VM_PROT_READ|VM_PROT_WRITE,
		TRUE);
	}
	*owner = task_map;

	return KERN_SUCCESS;
}


// 
// Unmap the shared memory area and release the wired memory.
//
kern_return_t
destroyEventShmem(	port_t task,
			struct vm_map *owner,
			vm_size_t size,
			vm_offset_t owner_addr,
			vm_offset_t shmem_addr )
{
	vm_offset_t off;
	kern_return_t krtn;
	vm_size_t shmem_size;

	if ( owner == VM_MAP_NULL )
		return KERN_INVALID_ARGUMENT;
	shmem_size = round_page(size);

	// Pull the shared pages out of the task map
	for ( off = 0; off < shmem_size; off += PAGE_SIZE )
	{
		pmap_remove(
		    owner->pmap,
		    owner_addr + off,
		    owner_addr + off + PAGE_SIZE);
	}
	// Free the former shmem area in the task
	krtn = vm_map_remove(owner,
		owner_addr,
		owner_addr + shmem_size );
	if(krtn) {
		IOLog("destroyEventShmem: vm_map_remove() returned %d\n",
			krtn);
	}
	// finally, free the memory back to the kernel map.
	kmem_free( kernel_map, shmem_addr, shmem_size );
	
	vm_map_deallocate(owner);	// discard our reference
	return krtn;
}
