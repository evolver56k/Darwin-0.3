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
 * Copyright (c) 1995, 1997 Apple Computer, Inc.
 *
 * HISTORY
 *
 * 23 June 1995 ? at NeXT
 *	Pulled over from CMU (MK83), added local
 * 	mods: freespace suballocator and sorted
 *	zone free lists to reduce fragmentation.
 */
/*
 *	File:	zalloc.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	 1985
 *
 */

#ifndef	_KERN_ZALLOC_H_
#define _KERN_ZALLOC_H_

#include <mach/machine/vm_types.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <machdep/machine/machspl.h>

/*
 *	A zone is a collection of fixed size blocks for which there
 *	is fast allocation/deallocation access.  Kernel routines can
 *	use zones to manage data structures dynamically, creating a zone
 *	for each type of data structure to be managed.
 *
 */

typedef struct zone {
	decl_simple_lock_data(,lock)	/* generic lock */
	spl_t		lock_ipl;
	int		count;		/* Number of elements used now */
	vm_offset_t	last_insert;
	vm_offset_t	free_elements;
	vm_size_t	cur_size;	/* current memory utilization */
	vm_size_t	max_size;	/* how large can this zone grow */
	vm_size_t	elem_size;	/* size of an element */
	vm_size_t	alloc_size;	/* size used for more memory */
	boolean_t	doing_alloc;	/* is zone expanding now? */
	char		*zone_name;	/* a name for the zone */
	unsigned int
	/* boolean_t */	pageable :1,	/* zone pageable? */
	/* boolean_t */	sleepable :1,	/* sleep if empty? */
	/* boolean_t */ exhaustible :1,	/* merely return if empty? */
	/* boolean_t */	expandable :1;	/* expand zone (with message)? */
	lock_data_t	complex_lock;	/* Lock for pageable zones */
	struct zone_free_space *
			free_space;	/* where to get new elements from */
	struct zone *	next_zone;	/* Link for all-zones list */
#if DIAGNOSTIC
	vm_offset_t	lowest, highest;
#endif
} *zone_t;

#define		ZONE_NULL	((zone_t) 0)

			/* allocate a zone entry and block
			 * if no space available */
extern vm_offset_t	zalloc(	zone_t		zone);

			/* allocate a zone entry and fail
			 * if zone free list is empty */
extern vm_offset_t	zget(	zone_t		zone);

			/* create a new zone */
extern zone_t		zinit(	vm_size_t	size,
				vm_size_t	max,
				vm_size_t	alloc,
				boolean_t	pageable,
				char		*zone_name);

			/* return a zone entry to the
			 * zone free list */
extern void		zfree(	zone_t		zone,
				vm_offset_t	elem);

			/* modify the characteristics of
			 * an existing zone */
extern void		zchange(zone_t		zone,
				boolean_t	pageable,
				boolean_t	sleepable,
				boolean_t	exhaustible,
				boolean_t	collectable);

			/* zones are collectable by default
			 * and cannot later be changed back to collectable */
extern void		zcollectable(zone_t	zone);

			/* exported to vm_resident module
			 * to acquire space for bootstrapping */
extern vm_offset_t	zdata;
extern vm_offset_t	zdata_size;

			/* supply memory directly by freeing
			 * it into the specified zone */
extern void		zcram(	zone_t		zone,
				vm_offset_t	newmem,
				vm_size_t	size);

			/* bootstrap the zone system to allow
			 * static allocation before vm is up */
extern void		zone_bootstrap(void);

			/* create the submap of the kernel_map
			 * used by this module for dynamic allocation */
extern void		zone_init(void);

#endif	/* _KERN_ZALLOC_H_ */
