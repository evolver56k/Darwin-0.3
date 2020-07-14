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
 * Copyright (c) 1992,1991,1990,1989,1988,1987 Carnegie Mellon University
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
 *	File:	mach/vm_statistics.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *
 *	Virtual memory statistics structure.
 *
 */

#ifndef	_MACH_VM_STATISTICS_H_
#define	_MACH_VM_STATISTICS_H_

#import <mach/machine/vm_types.h>

struct vm_statistics {
	integer_t	pagesize;		/* page size in bytes */
	integer_t	free_count;		/* # of pages free */
	integer_t	active_count;		/* # of pages active */
	integer_t	inactive_count;		/* # of pages inactive */
	integer_t	wire_count;		/* # of pages wired down */
	integer_t	zero_fill_count;	/* # of zero fill pages */
	integer_t	reactivations;		/* # of pages reactivated */
	integer_t	pageins;		/* # of pageins */
	integer_t	pageouts;		/* # of pageouts */
	integer_t	faults;			/* # of faults */
	integer_t	cow_faults;		/* # of copy-on-writes */
	integer_t	lookups;		/* object cache lookups */
	integer_t	hits;			/* object cache hits */
};

typedef struct vm_statistics	*vm_statistics_t;
typedef struct vm_statistics	vm_statistics_data_t;

#ifdef	KERNEL
extern vm_statistics_data_t	vm_stat;
#endif	/* KERNEL */

/*
 *	Each machine dependent implementation is expected to
 *	keep certain statistics.  They may do this anyway they
 *	so choose, but are expected to return the statistics
 *	in the following structure.
 */

struct pmap_statistics {
	integer_t		resident_count;	/* # of pages mapped (total)*/
	integer_t		wired_count;	/* # of pages wired */
};

typedef struct pmap_statistics	*pmap_statistics_t;
#endif	/* _MACH_VM_STATISTICS_H_ */
