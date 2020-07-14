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
 * Copyright (c) 1990 NeXT, Inc.
 *
 * HISTORY
 *  2-Jul-90  Morris Meyer (mmeyer) at NeXT
 *	Created.
 */

#import <kern/time_stamp.h>
#if defined(KERNEL_BUILD)
#import <mallocdebug.h>
#endif /* KERNEL_BUILD */

#if	MALLOCDEBUG

#define	MTYPE_KALLOC		0
#define MTYPE_ZALLOC		1
#define	MTYPE_KMEM_ALLOC	2

#define	ALLOC_TYPE_ALLOC	0
#define ALLOC_TYPE_FREE		1

void malloc_debug (void *addr, void *pc, int size, int which, int type);
void *get_return_pc (void);

struct malloc_info {
	struct tsval time;	/* Time stamp    */
	short type;		/* Alloc or free */
	short which;		/* kalloc, zalloc or kmem_alloc */
	void *addr;		/* Allocated or free'd address */
	void *pc;		/* Caller of kalloc, kfree, etc */
	int size;		/* Size of allocated or free'd address */
};

#endif	/* MALLOCDEBUG */
