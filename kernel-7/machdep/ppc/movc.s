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
 * HISTORY
 * Revision 1.2  1997/10/29 02:14:00  tmason
 * Fixed oodles of bugs related to pmap issues as well as bcopy, FLOAT!, and cached accesses.
 * Radar Bug ID:
 *
 * Revision 1.1.1.1  1997/09/30 02:45:23  wsanchez
 * Import of kernel from umeshv/kernel
 *
 * Revision 1.1.??.1  1997/06/28  10:58:00  rvega
 *	Radar #1665906
 * 	Restore comments, alignment, and white space lost during integration.
 * 	[1997/06/28  10:58:00  rvega]
 *
 * Revision 1.1.??.1  1997/06/17  14:54:28  rvega
 * 	pmap_page_copy needs to flush the i-cache *and*
 *	it needs to work without using floating point registers.
 * 	[1997/06/17  14:54:28  rvega]
 *
 * Revision 1.1.??.1  1997/05/14  18:55:55  rvega
 * 	Cleanup sync/isync usage.
 * 	[1997/05/14  18:55:55  rvega]
 *
 */
 
/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

#include <debug.h>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <assym.h>

#ifndef USE_FLOATING_POINT_IN_KERNEL
#define	USE_FLOATING_POINT_IN_KERNEL	0
#else
#undef USE_FLOATING_POINT_IN_KERNEL
#define	USE_FLOATING_POINT_IN_KERNEL	0
#endif


/*
 * void pmap_zero_page(vm_offset_t pa)
 *
 * zero a page of physical memory.
 */

#if DEBUG
	/* C debug stub in pmap.c calls this */
ENTRY(pmap_zero_page_assembler, TAG_NO_FRAME_USED)
#else
ENTRY(pmap_zero_page, TAG_NO_FRAME_USED)
#endif /* DEBUG */

	/* Switch off data translations */
	mfmsr	r6
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync			/* Ensure data translations are off */

#ifndef UNCACHED_DATA_604
	li	r4, (PPC_PGBYTES/CACHE_LINE_SIZE)
#else	/* UNCACHED_DATA_604 */
	li	r4, (PPC_PGBYTES/4)
#endif	/* UNCACHED_DATA_604 */
	mtctr	r4
.L_phys_zero_loop:	
#ifndef UNCACHED_DATA_604
	dcbz	0, ARG0
	addi	ARG0, ARG0, CACHE_LINE_SIZE
#else	/* UNCACHED_DATA_604 */
	li	r7, 0
	stwx	r7, 0, ARG0
	addi	ARG0, ARG0, 4
#endif	/* UNCACHED_DATA_604 */
	bdnz+	.L_phys_zero_loop

	sync			/* Finish any outstanding writes */
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */

	blr

/* void
 * phys_copy(src, dst, bytecount)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *      int             bytecount
 *
 * This routine will copy bytecount bytes from physical address src to physical
 * address dst. 
 */

ENTRY(phys_copy, TAG_NO_FRAME_USED)

	/* Switch off data relocation.
	*/
	mfmsr	r6
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync			/* Ensure data translations are off */
	mtctr	ARG2
.L_phys_copy_byte_loop:	
	lbz	r7, 0(ARG0)
	stb	r7, 0(ARG1)
	addi	ARG0, ARG0, 1
	addi	ARG1, ARG1, 1
	bdnz+	.L_phys_copy_byte_loop

.L_phys_copy_done:
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are off */

	blr




/* void
 * pmap_copy_page(src, dst)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *
 * This routine will copy the physical page src to physical page dst
 * 
 * This routine assumes that the src and dst are page aligned and that the
 * destination is cached.
 *
 * This routine is called in a myriad places to copy pages containing
 * instructions to be executed later, therefore it has to invalidate the
 * i-cache entries of the destination page.
 */

#if DEBUG
	/* if debug, we have a little piece of C around this
	 * in pmap.c that gives some trace ability
	 */
ENTRY(pmap_copy_page_assembler, TAG_NO_FRAME_USED)
#else
ENTRY(pmap_copy_page, TAG_NO_FRAME_USED)
#endif /* DEBUG */

#if USE_FLOATING_POINT_IN_KERNEL
	/* Save off the link register, we want to call fpu_save. We assume
	 * that fpu_save leaves ARG0-5 intact and that it doesn't need
	 * a frame, otherwise we'd need one too.
	 */
	
	mflr	ARG3
	bl	EXT(fpu_save)
	mtlr	ARG3
#endif /* USE_FLOATING_POINT_IN_KERNEL */

	/* Switch off data relocation.
	*/
	mfmsr	r6
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync			/* Ensure data translations are off */

#if USE_FLOATING_POINT_IN_KERNEL
	li	r7,	(PPC_PGBYTES/(2*32))
	mtctr	r7
	li	r7, 32
#else /* USE_FLOATING_POINT_IN_KERNEL */
	li	r7,	(PPC_PGBYTES/CACHE_LINE_SIZE)
	mtctr	r7
#endif /* USE_FLOATING_POINT_IN_KERNEL */

.L_pmap_copy_page_loop:
#ifndef	UNCACHED_DATA_604
	dcbz	0,	ARG1
#if USE_FLOATING_POINT_IN_KERNEL
	dcbz	r7,	ARG1
#endif /* USE_FLOATING_POINT_IN_KERNEL */
#endif	/* UNCACHED_DATA_604 */

#if USE_FLOATING_POINT_IN_KERNEL
	lfd	f0,  0(ARG0)
	lfd	f1,  8(ARG0)
	lfd	f2, 16(ARG0)
	lfd	f3, 24(ARG0)
	lfd	f4, 32(ARG0)
	lfd	f5, 40(ARG0)
	lfd	f6, 48(ARG0)
	lfd	f7, 56(ARG0)
	addi	ARG0,	ARG0,	(2*32)

	stfd	f0,  0(ARG1)
	stfd	f1,  8(ARG1)
	stfd	f2, 16(ARG1)
	stfd	f3, 24(ARG1)
#ifndef	UNCACHED_DATA_604
	dcbst	0,	ARG1
	sync
#endif	/* UNCACHED_DATA_604 */
#ifndef	UNCACHED_INST_604
	icbi	0,	ARG1
	sync
	isync
#endif	/* UNCACHED_INST_604 */

	addi	ARG1,	ARG1,	CACHE_LINE_SIZE

	stfd	f4,  0(ARG1)
	stfd	f5,  8(ARG1)
	stfd	f6, 16(ARG1)
	stfd	f7, 24(ARG1)
#else USE_FLOATING_POINT_IN_KERNEL
	lwz	r0,   0(ARG0)
	lwz	r5,   4(ARG0)
	lwz	r7,   8(ARG0)
	lwz	r8,  12(ARG0)
	lwz	r9,  16(ARG0)
	lwz	r10, 20(ARG0)
	lwz	r11, 24(ARG0)
	lwz	r12, 28(ARG0)
	addi	ARG0,	ARG0,	CACHE_LINE_SIZE

	stw	r0,   0(ARG1)
	stw	r5,   4(ARG1)
	stw	r7,   8(ARG1)
	stw	r8,  12(ARG1)
	stw	r9,  16(ARG1)
	stw	r10, 20(ARG1)
	stw	r11, 24(ARG1)
	stw	r12, 28(ARG1)

#endif /* USE_FLOATING_POINT_IN_KERNEL */

	/*
	**	Invalidate the i-cache entries.
	*/
#ifndef	UNCACHED_DATA_604
	dcbst	0,	ARG1
	sync
#endif	/* UNCACHED_DATA_604 */
#ifndef	UNCACHED_INST_604
	icbi	0,	ARG1
	sync
	isync
#endif	/* UNCACHED_INST_604 */

	addi	ARG1,	ARG1,	CACHE_LINE_SIZE
	bdnz+	.L_pmap_copy_page_loop

#if	USE_FLOATING_POINT_IN_KERNEL
	rlwinm	r6,	r6,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
#endif /* USE_FLOATING_POINT_IN_KERNEL */
	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */

	blr



/*
 * vm_offset_t
 * view_user_address(addr)
 *	vm_offset_t	addr;
 *
 * Set SR_COPYIN to point to segmentt containing address
 * and return modified address.
 * 
 */

ENTRY(view_user_address, TAG_NO_FRAME_USED)
	addis	ARG1,	0,	ha16(EXT(kdp_space))
	addi	ARG1,	ARG1,	lo16(EXT(kdp_space))
	lwz	ARG1,	0(ARG1)
	cmpwi	ARG1,	0		; if kdp_space is set use that
	bne	1f
	mfsprg	ARG1,	0	/* get per-proc_info */
	lwz	ARG1,	PP_CPU_DATA(ARG1)
	lwz	ARG1,	CPU_ACTIVE_THREAD(ARG1)
	lwz	ARG1,	THREAD_TASK(ARG1)
	lwz	ARG1,	TASK_VMMAP(ARG1)
	lwz	ARG1,	VMMAP_PMAP(ARG1)
	lwz	ARG1,	PMAP_SPACE(ARG1)
1:
	lis	r0,	SEG_REG_PROT>>16	/* Top byte of SR value */
	rlwimi	r0,	ARG1,	4,	4,	31 /* Insert space<<4   */
	rlwimi	r0,	ARG0,	4,	28,	31 /* Insert seg number */

	isync
	mtsr	SR_COPYIN_NAME,	r0
	isync

	/* ARG0 = adjusted user pointer mapping into SR_COPYIN */
	rlwinm	ARG0,	ARG0,	0,	4,	31
	oris	ARG0,	ARG0,	(SR_COPYIN << (28-16))

	blr

ENTRY(cioseg, TAG_NO_FRAME_USED)
	// build seg reg value
	lis	r7,	SEG_REG_PROT>>16	/* Top byte of SR value */
	rlwimi	r7,	r3,	4,	4,	31 /* Insert space<<4   */
	rlwimi	r7,	r4,	4,	28,	31 /* Insert seg number */

	// update seg reg
	isync
	mtsr	SR_COPYIN_NAME,	r7
	isync

	// build and return mapped addr for caller
	rlwinm	r3,	r4,	0,	4,	31
	oris	r3,	r3,	(SR_COPYIN << (28-16))

	blr

ENTRY(find_phys, TAG_NO_FRAME_USED)
        /* Switch off data translations */
        mfmsr   r11
        rlwinm  r10, r11, 0, MSR_DR_BIT+1, MSR_DR_BIT-1
        mtmsr   r10
        isync

	mr	r10, r3
	li	r3, 0
L_loop_find:
	cmpw	r4, r5
	bge	L_find_exit
	lwz	r9, 0(r4)
	addi	r4, r4, 4
	cmpw	r9, r10
	bne	L_loop_find
	subi	r3, r4, 4
L_find_exit:
	mtmsr	r11		/* Restore original translations */
	isync

	blr
