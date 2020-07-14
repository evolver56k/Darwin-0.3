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

#ifndef _PPC_MEM_H_
#define _PPC_MEM_H_

/* Various prototypes for things in/used by the low level memory
 * routines
 */

#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/pmap_internals.h>
#include <mach/machine/vm_types.h>

/* These statics are initialised by pmap.c */

extern vm_offset_t hash_table_base;
extern vm_offset_t hash_table_size;
extern vm_offset_t hash_table_mask;

void hash_table_init(vm_offset_t base, vm_offset_t size);
pte_t *find_or_allocate_pte(space_t	sid,
			    vm_offset_t v_addr,
			    boolean_t	allocate);

#endif /* _PPC_MEM_H_ */
