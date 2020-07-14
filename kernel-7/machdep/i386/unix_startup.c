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
 * Unix data structure initialization.
 *
 * HISTORY
 *
 * 26 May 1992 ? at NeXT
 *	Created from 68k version.
 */

#import <mach/mach_types.h>

#import <vm/vm_kern.h>
#import <vm/vm_page.h>

#import <kernserv/ns_timer.h>

#import <sys/param.h>
#import <sys/buf.h>
#import <sys/callout.h>
#import <sys/clist.h>
#import <sys/mbuf.h>
#import <sys/systm.h>
#import <sys/tty.h>

extern struct tty	cons;

#import <kern/assert.h>

/*
 * Declare these as initialized data so we can patch them.
 */
int	niobuf = 0;

#ifdef	NBUF
int		nbuf = NBUF;
#else
int		nbuf = 0;
#endif
#ifdef	NMFSBUF
int	nmfsbuf = NMFSBUF
#else
int	nmfsbuf = 0;
#endif
#ifdef	BUFPAGES
int		bufpages = BUFPAGES;
#else
int		bufpages = 0;
#endif
int		show_space = 0;
int  		  srv;        /* Flag indicates a server boot when set */
int             ncl = 0;
vm_map_t	buffer_map;

/*
 * Machine-dependent startup code
 */
/*
 * Machine-dependent early startup code
 */
#if     SHOW_SPACE

#define valloc(name, type, num)						\
MACRO_BEGIN								\
    (name) = (type *)(v); (v) = (vm_offset_t)((name)+(num));		\
    if (show_space) 							\
	printf(#name " = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",	\
	    num*sizeof(type),						\
	    num*sizeof(type),						\
	    name, num, sizeof(type));					\
MACRO_END

#define valloclim(name, type, num, lim) 				\
MACRO_BEGIN								\
    (name) = (type *)(v); (v) = (vm_offset_t)((lim) = ((name)+(num)));	\
    if (show_space) 							\
	printf(#name " = %d(0x%x) bytes @%x, %d cells @ %d bytes\n",	\
	    num*sizeof(type),						\
	    num*sizeof(type),						\
	    name, num, sizeof(type));					\
MACRO_END

#else   SHOW_SPACE

#define valloc(name, type, num) 					\
    (name) = (type *)(v); (v) = (vm_offset_t)((name)+(num))

#define valloclim(name, type, num, lim) 				\
    (name) = (type *)(v); (v) = (vm_offset_t)((lim) = ((name)+(num)));

#endif  SHOW_SPACE

vm_size_t
buffer_map_sizer(void)
{

    /*
     *      Since these pages are virtual-size pages (larger
     *      than physical page size), use only one page
     *      per buffer.
     */
    if (bufpages == 0) {
		bufpages = atop(mem_size / 50);
    }

    if (nbuf == 0) {
#if     PRIVATE_BUFS
	    nbuf = 100;
#else   PRIVATE_BUFS
	/* Go for a 1-1 correspondence between the number of buffer
	 * headers and bufpages.  Then add some extra (empty) buffer
	 * headers to aid clustering.
	 */
	if ((nbuf = bufpages) < 16)
			nbuf = 16;
	nbuf += 64;
#endif  PRIVATE_BUFS
    }

    if (bufpages > nbuf * (MAXBSIZE / page_size))
	bufpages = nbuf * (MAXBSIZE / page_size);

    if (niobuf == 0) {
		if ((niobuf = bufpages / (MAXPHYSIO / page_size)) > 1024)
			niobuf = 1024;
		if (niobuf < 32)
			niobuf = 32;
	}

    return (round_page(((vm_size_t)nbuf * MAXBSIZE)) + ((vm_size_t)niobuf
			* MAXPHYSIO));
	}

void
startup_early(void)
{
    vm_offset_t		firstaddr, v;
    mem_region_t	rp  = mem_region;

    v = firstaddr = pmap_phys_to_kern(rp->first_phys_addr);
    (void) buffer_map_sizer();

    valloc(buf, struct buf, nbuf + niobuf);

	/*
	 * Unless set at the boot command line, mfs gets no more than
	 * half of the system's bufs.  Hack to prevent buf starvation
	 * and system hang.
	 */
	if (nmfsbuf == 0)
		nmfsbuf = nbuf / 2;

    /*
     * Clear space allocated thus far, and make r/w entries
     * for the space in the kernel map.
     */

    bzero(firstaddr, v - firstaddr);
    rp->first_phys_addr = pmap_resident_extract(kernel_pmap, v);

    if (mem_size > (64 * 1024 * 1024)) {
            int scale;
	    extern u_long tcp_sendspace;
	    extern u_long tcp_recvspace;

	    if ((nmbclusters = ncl) == 0) {
	            if ((nmbclusters = ((mem_size / 16) / MCLBYTES)) > 4096)
		            nmbclusters = 8192;
	    }
	    if ((scale = nmbclusters / NMBCLUSTERS) > 1) {
	            tcp_sendspace *= scale;
		    tcp_recvspace *= scale;

		    if (tcp_sendspace > (32 * 1024))
		            tcp_sendspace = 32 * 1024;
		    if (tcp_recvspace > (32 * 1024))
		            tcp_recvspace = 32 * 1024;
	    }
    }
}


startup(
    vm_offset_t		firstaddr
)
{
    unsigned int	i;
    vm_size_t		map_size;
    kern_return_t	ret;
    vm_offset_t		buffer_max;
    int			base, residual;
    extern int		vm_page_free_count;
    mem_region_t	rp = mem_region;

    cons.t_dev = makedev(12, 0);

    kminit();

    /*
     * Good {morning,afternoon,evening,night}.
     */
    panic_init();

    printf(version);

#define MEG	(1024*1024)
    printf("physical memory = %d.%d%d megabytes.\n",
	mem_size/MEG,
	((mem_size%MEG)*10)/MEG,
	((mem_size%(MEG/10))*100)/MEG);

    /*
     * Allocate space for system data structures.
     * The first available real memory address is in "firstaddr".
     * The first available kernel virtual address is in "v".
     * As pages of kernel virtual memory are allocated, "v" is incremented.
     * As pages of memory are allocated and cleared,
     * "firstaddr" is incremented.
     * An index into the kernel page table corresponding to the
     * virtual memory address maintained in "v" is kept in "mapaddr".
     */

    /*
     * Since the virtual memory system has already been set up,
     * we cannot bypass it to allocate memory as the old code
     * DOES.  we therefore make two passes over the table
     * allocation code.  The first pass merely calculates the
     * size needed for the various data structures.  The
     * second pass allocates the memory and then sets the
     * actual addresses.  The code must not change any of
     * the allocated sizes between the two passes.
     */
    firstaddr = round_page(firstaddr);
    map_size = buffer_map_sizer();

    /*
     * Between the following find, and the next one below
     * we can't cause any other memory to be allocated.  Since
     * below is the first place we really need an object, it
     * will cause the object zone to be expanded, and will
     * use our memory!  Therefore we allocate a dummy object
     * here.  This is all a hack of course.
     */
    ret = vm_map_find(kernel_map, vm_object_allocate(0), (vm_offset_t) 0,
		&firstaddr, map_size, TRUE);
    ASSERT(ret == KERN_SUCCESS);
    vm_map_remove(kernel_map, firstaddr, firstaddr + map_size);

    /*
     * Now allocate buffers proper.  They are different than the above
     * in that they usually occupy more virtual memory than physical.
     */
    buffers = (void *)firstaddr;
    base = bufpages / nbuf;
    residual = bufpages % nbuf;

    /*
     * Allocate virtual memory for buffer pool.
     */
    buffer_map = kmem_suballoc(kernel_map,
			       &firstaddr, &buffer_max, map_size, TRUE);
    ret = vm_map_find(buffer_map, 
		      vm_object_allocate(map_size), (vm_offset_t) 0,
		      &firstaddr, map_size, FALSE);

    ASSERT(ret == KERN_SUCCESS);

    for (i = 0; i < nbuf; i++) {
	vm_size_t	thisbsize;
	vm_offset_t	curbuf;

	/*
	 * First <residual> buffers get (base+1) physical pages
	 * allocated for them.  The rest get (base) physical pages.
	 *
	 * The rest of each buffer occupies virtual space,
	 * but has no physical memory allocated for it.
	 */

	thisbsize = page_size*(i < residual ? base+1 : base);
	curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
	vm_map_pageable(buffer_map, curbuf, curbuf+thisbsize, FALSE);
    }

    {
	register int	nbytes;

	nbytes = ptoa(bufpages);
	printf("using %d buffers containing %d.%d%d megabytes of memory\n",
		nbuf,
		nbytes/MEG,
		((nbytes%MEG)*10)/MEG,
		((nbytes%(MEG/10))*100)/MEG);

	nbytes = ptoa(vm_page_free_count);
	printf("available memory = %d.%d%d megabytes. vm_page_free_count = %x\n",
		nbytes/MEG,
		((nbytes%MEG)*10)/MEG,
		((nbytes%(MEG/10))*100)/MEG,
		vm_page_free_count);
    }

    /*
     * Initialize memory allocator and swap
     * and user page table maps.
     */
    mb_map = kmem_suballoc(kernel_map,
		(vm_offset_t *) &mbutl,
		(vm_offset_t *) &embutl,
		(vm_size_t) (nmbclusters * MCLBYTES),
		FALSE);

    /*
     * Set up buffers, so they can be used to read disk labels.
     */
    bufinit();
}
