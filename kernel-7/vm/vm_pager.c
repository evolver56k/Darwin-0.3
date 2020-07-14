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
 *	File:	vm/vm_pager.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Paging space routine stubs.  Emulates a matchmaker-like interface
 *	for builtin pagers.
 */

#import <mach/kern_return.h>
#import <vm/vm_pager.h>
#import <vm/vm_page.h>
#import <mach/vm_prot.h>
#import <mach/boolean.h>
#import <vm/vnode_pager.h>

#import <vm/pmap.h>

/*
 *	Important note:
 *		All of these routines gain a reference to the
 *		object (first argument) as part of the automatic
 *		argument conversion. Explicit deallocation is necessary.
 */

extern
boolean_t	vm_page_zero_fill();

pager_return_t vm_pager_get(pager, m, error)
	vm_pager_t	pager;
	vm_page_t	m;
	int		*error;
{
	if (pager == vm_pager_null) {
		(void) vm_page_zero_fill(m);
		return(PAGER_SUCCESS);
	}
	if (pager->is_device)
		panic("vm_pager_get device");
	return(vnode_pagein(m, error));
}

pager_return_t vm_pager_put(pager, m)
	vm_pager_t	pager;
	vm_page_t	m;
{
	if (pager == vm_pager_null)
		panic("vm_pager_put: null pager");
	if (pager->is_device)
	    panic("vm_pager_put device");
	return(vnode_pageout(m));
}

void vm_pager_deallocate(pager)
	vm_pager_t	pager;
{
	if (pager == vm_pager_null)
		panic("vm_pager_deallocate: null pager");
	if (pager->is_device)
		panic("vm_pager_deallocate device");
	vnode_dealloc(pager);
}

vm_pager_t vm_pager_allocate(size)
	vm_size_t	size;
{
	return((vm_pager_t)vnode_alloc(size));
}

boolean_t vm_pager_has_page(pager, offset)
	vm_pager_t	pager;
	vm_offset_t	offset;
{
	if ((pager == vm_pager_null) || pager->is_device)
		panic("vm_pager_has_page");
	return(vnode_has_page(pager,offset));
}
