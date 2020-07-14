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
 * Copyright (c) 1993 NeXT, Inc.
 */ 


#ifndef	_MACH_PPC_VM_PARAM_H_
#define _MACH_PPC_VM_PARAM_H_

#import <sys/types.h>

#define BYTE_SIZE	8	/* byte size in bits */

#define PPC_PGBYTES	4096	/* bytes per power pc page */
#define PPC_PGSHIFT	12	/* number of bits to shift for pages */
#define PPC_PGALIGN	12      /* power of two for page alignment */

#define VM_MIN_ADDRESS	((vm_offset_t) 0)
#define VM_MAX_ADDRESS	((vm_offset_t) 0xfffff000)

#define KERNELBASE_TEXT	0x0	/* see MACH3.0 mach/ppc/vm_param.h */

#define ppc_round_page(x)	((((unsigned)(x)) + PPC_PGBYTES - 1) & \
					~(PPC_PGBYTES-1))
#define ppc_trunc_page(x)	(((unsigned)(x)) & ~(PPC_PGBYTES-1))

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0x00004000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0x3ffff000)

/*
 * WARNING : If you make the stack bigger, you need to check the
 *	trap code to make sure things still work...
 */
 
#define KERNSTACK_SIZE		(4 * PPC_PGBYTES)
#define INTSTACK_SIZE		(10 * PPC_PGBYTES)

/*
 * Maximum alignment required by any data type for this architecture.
 */
#define MAX_DATA_ALIGNMENT      16  /* 16 byte alignment for LDCWS, LDCWX */

#endif	/* _MACH_PPC_VM_PARAM_H_ */
