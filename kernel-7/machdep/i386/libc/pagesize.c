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

/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *      File:   machdep/i386/libc/pagesize.c
 *      Author: Bruce Martin, NeXT Computer, Inc.
 *
 *      This file contains machine dependent code for block compares
 *      on NeXT i386-based products.  Currently tuned for the i486.
 *
 * HISTORY
 * 27-Sep-92  Bruce Martin (Bruce_Martin@NeXT.COM)
 *	Created.
 */

#include <mach/mach_types.h>

#if defined(NX_CURRENT_COMPILER_RELEASE) && (NX_CURRENT_COMPILER_RELEASE < 320)
#define SIREG "e"
#else
#define SIREG "S"
#endif

/*
 * A memory set function which requires its input address to be
 * integer aligned, and its span to be an integer mulitple of
 * 32 bytes.
 *
 * NOTE: we force this routine to use %ecx because
 * we have to note that these arguments are clobbered.  There
 * doesn't seem to be any other way - short of hard-wiring
 * a specific register.
 */
static __inline__ vm_offset_t
mem_set_a4_l32(vm_offset_t addr, unsigned int val, int len)
{
    asm volatile ("
	.align 2, 0x90
	movl %2, 0x00(%0);	movl %2, 0x04(%0)
	movl %2, 0x08(%0);	movl %2, 0x0C(%0)
	movl %2, 0x10(%0);	movl %2, 0x14(%0)
	movl %2, 0x18(%0);	movl %2, 0x1C(%0)
	addl $0x20, %0;		addl $-0x20,%1
	jne .-30
	"
	: "=r" (addr)
	: "c" (len), "a" (val), "0" (addr)
	: "eax", "ecx", "cc");
    return(addr);
}


/*
 * A memory copy which requires its input addresses to be
 * integer aligned, and its span to be an integer multiple of
 * four bytes.
 *
 * Benchmarking has shown the use of the rep;movsl opcode to be
 * slightly slower than the optimal unrolled loop on the 486, but
 * only by a few percent.  On the P5, you can't beat rep;movsl.
 */
static __inline__ void
mem_copy_forward_a4_l4(vm_offset_t dst, vm_offset_t src, vm_size_t len)
{
    asm("rep; movsl"
        : /* no outputs */
        : "c" (len>>2), "D" (dst), SIREG (src)
        : "ecx", "esi", "edi");
    return;
}



void
page_set(vm_offset_t addr, int val, vm_size_t size)
{
    (void) mem_set_a4_l32(addr, val, size);
    return;
}


void
page_copy(vm_offset_t dst, vm_offset_t src, vm_size_t size)
{
    (void) mem_copy_forward_a4_l4(dst, src, size);
    return;
}


