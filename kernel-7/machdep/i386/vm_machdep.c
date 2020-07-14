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
 * Intel386 Family:	MD VM crud.
 *
 * HISTORY
 *
 * 18 May 1992 ? at NeXT
 *	Created.
 */

#import <sys/param.h>
#import <sys/buf.h>
#import <sys/errno.h>

#import <kern/thread.h>

#import <vm/vm_kern.h>

#import <machdep/i386/cpu_inline.h>

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of the page size.
 */
pagemove(from, to, size)
    register caddr_t from, to;
    int size;
{
    register pt_entry_t *fpte, *tpte;

    if (size % I386_PGBYTES)
	panic("pagemove");

    while (size > 0) {
	fpte = pmap_pt_entry(kernel_pmap, (vm_offset_t)from);
	tpte = pmap_pt_entry(kernel_pmap, (vm_offset_t)to);
	*tpte++ = *fpte;
	*fpte++ = (pt_entry_t) { 0 };
	from += I386_PGBYTES;
	to += I386_PGBYTES;
	size -= I386_PGBYTES;
    }
    
    flush_tlb();
}

/* 
 * kernacc - check kernel access to kernel memory.
 *
 */
boolean_t
kernacc(
    vm_offset_t 	base,
    unsigned int	len,
    int			op
)
{
    vm_offset_t		end = base + len;
    pt_entry_t		*pte;
    
    base = trunc_page(base);

    while (base < end) {
	pte = pmap_pt_entry(kernel_pmap, base);
	if (pte == PT_ENTRY_NULL)
	    return (FALSE);

	if (!pte->valid)
	    return (FALSE);
	    
	if (op == B_WRITE && pte->prot == PT_PROT_KR)
	    return (FALSE);

	base += PAGE_SIZE;
    }   

    return (TRUE);
}
