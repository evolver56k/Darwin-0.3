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
 * Revision 1.3  1997/11/06 00:15:22  tmason
 * Fixed spl bug in pmap.c as well as fixes for bcopy
 *
 * Revision 1.2  1997/10/29 02:13:43  tmason
 * Fixed oodles of bugs related to pmap issues as well as bcopy, FLOAT!, and cached accesses.
 *
 * Revision 1.1.1.1  1997/09/30 02:45:28  wsanchez
 * Import of kernel from umeshv/kernel
 *
 * Revision 1.1.1.1  1997/06/28  10:47:00  rvega
 *	Radar #1665906
 * 	Add integer load/store method to avoid use of floating
 *	point load/stores in kernel until full support is completed.
 *	Note: this restores code that was regressed during integration.
 * 	[1997/06/28  10:47:00 rvega]
 *
 * Revision 1.1.1.1  1997/04/30  15:00:00  rvega
 * 	Use the Tim Olson fast copy function.
 * 	[1997/04/30  15:00:00  rvega]
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

#include <ppc/asm.h>
#include <assym.h>
#include <ppc/bcopy.h>
#include <ppc/proc_reg.h>	/* For CACHE_LINE_SIZE */

#ifndef USE_FLOATING_POINT_IN_KERNEL
#define	USE_FLOATING_POINT_IN_KERNEL	0
#else
#undef USE_FLOATING_POINT_IN_KERNEL
#define	USE_FLOATING_POINT_IN_KERNEL	0
#endif

/* If we are permitted to use floating point registers, this goes faster
 * for cache-aligned copies
 */


/* registers used: */

/* IMPORTANT - copyin/copyout assumes that bcopy won't trash r10 */

#define src		r4
#define byteCount	r5
#define wordCount	r6
#define dst		r7
#define bufA		r8
#define bufB		r9
#define bufC		r0
#define fbufA		f8
#define fbufB		f9
#define fbufC		f0

#define	rs		r3
#define	rd		r4
#define	rc		r5

#define	CACHE_FLAG	4
#define	crCached	4

/*
 * bcopy(const char *from, chat *to, vm_size_t nbytes)
 *
 * bcopy_nc is simply bcopy except the dcbz & dcbst
 * instructions are not to be used. This is for non-cached
 * areas of memory.
 * 
 * Various device drivers heavily use this routine.
 */

ENTRY(bcopy_nc, TAG_NO_FRAME_USED)
	mfcr	r0
	stw	r0,	FM_CR_SAVE(r1)
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)

	crxor	CACHE_FLAG,CACHE_FLAG,CACHE_FLAG
	b	.L_bcopy_common

/*	
 * void bcopy(const char *from, char *to, vm_size_t nbytes)
 *
 * bcopy uses memcpy, shouldn't this be done with a #define??
 */

#if USE_FAST_BCOPY

#if ORG_BCOPY
ENTRY(fast_bcopy, TAG_NO_FRAME_USED)
#else	/* ORG_BCOPY */
ENTRY2(bcopy,fast_bcopy, TAG_NO_FRAME_USED)
#endif	/* ORG_BCOPY */

#endif /* USE_FAST_BCOPY */
	mfcr	r0
	stw	r0,	FM_CR_SAVE(r1)
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)

	creqv	CACHE_FLAG,CACHE_FLAG,CACHE_FLAG

.L_bcopy_common:
	cmpwi	CR0, byteCount, 0
	beqlr-	CR0

	/* Convert to memcpy style arguments */
	mr	dst,	ARG1	/* Move dst from arg to 'dst' */
	mr	src,	ARG0	/* put src in expected register too */
	b	.L_memcpy_bcopy

#if	0	/* ndef	PPC604 */
/* The 601 and 603 cope well with unaligned word accesses, so we can
 * forget about worrying about word alignment issues - the only exception
 * to this is on a page boundary. This means that we can go faster than
 * on a 604
 */

/* For the moment, just use the 604 version, TODO NMGS optimise */

#else	/* PPC604 */

/*
 * Copyright (C) 1993, 1994, 1995  Tim Olson
 *
 * This software is distributed absolutely without warranty. You are
 * free to use and modify the software as you wish.  You are also free
 * to distribute the software as long as you retain the above copyright
 * notice, and you make clear what your modifications were.
 *
 * Send comments and bug reports to tim@apple.com
 */

	.align 2
LmemcpyAlignVector:
	.long	.mm0s0c0
	.long	.mm0s0c1
	.long	.mm0s0c2
	.long	.mm0s0c3
	.long	.mm0s1c0
	.long	.mm0s1c1
	.long	.mm0s1c2
	.long	.mm0s1c3
	.long	.mm0s2c0
	.long	.mm0s2c1
	.long	.mm0s2c2
	.long	.mm0s2c3
	.long	.mm0s3c0
	.long	.mm0s3c1
	.long	.mm0s3c2
	.long	.mm0s3c3
	.long	.mm1s0c0
	.long	.mm1s0c1
	.long	.mm1s0c2
	.long	.mm1s0c3
	.long	.mm1s1c0
	.long	.mm1s1c1
	.long	.mm1s1c2
	.long	.mm1s1c3
	.long	.mm1s2c0
	.long	.mm1s2c1
	.long	.mm1s2c2
	.long	.mm1s2c3
	.long	.mm1s3c0
	.long	.mm1s3c1
	.long	.mm1s3c2
	.long	.mm1s3c3
	.long	.mm2s0c0
	.long	.mm2s0c1
	.long	.mm2s0c2
	.long	.mm2s0c3
	.long	.mm2s1c0
	.long	.mm2s1c1
	.long	.mm2s1c2
	.long	.mm2s1c3
	.long	.mm2s2c0
	.long	.mm2s2c1
	.long	.mm2s2c2
	.long	.mm2s2c3
	.long	.mm2s3c0
	.long	.mm2s3c1
	.long	.mm2s3c2
	.long	.mm2s3c3
	.long	.mm3s0c0
	.long	.mm3s0c1
	.long	.mm3s0c2
	.long	.mm3s0c3
	.long	.mm3s1c0
	.long	.mm3s1c1
	.long	.mm3s1c2
	.long	.mm3s1c3
	.long	.mm3s2c0
	.long	.mm3s2c1
	.long	.mm3s2c2
	.long	.mm3s2c3
	.long	.mm3s3c0
	.long	.mm3s3c1
	.long	.mm3s3c2
	.long	.mm3s3c3

/*
 * high-performance memcpy implementation for 604
 * uses aligned transfers plus alignment shuffling code
 */

	
#if USE_FAST_BCOPY
ENTRY(memcpy, TAG_NO_FRAME_USED)
#endif /* USE_FAST_BCOPY */
	mfcr	r0
	stw	r0,	FM_CR_SAVE(r1)
	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)

	cmpwi	CR0, byteCount, 0
	beqlr-	CR0
	mr	dst,	ARG0		/* Move dst from retval to 'dst' */

	creqv	CACHE_FLAG,CACHE_FLAG,CACHE_FLAG

.L_memcpy_bcopy:
		/* (jumped to) entry point for fast_bcopy */

	cmplw	src, dst
	beqlr				/* they are equal, so exit */

	stwu	r1, -48(r1)
	stw	r3,  12(r1)

	/* disable ints */
	mfmsr	r12
	rlwinm	r8, r12, 0 , MSR_EE_BIT+1, MSR_EE_BIT-1
	mtmsr	r8

#if USE_FLOATING_POINT_IN_KERNEL
	bl	EXT(fpu_save)		// save fpu state if in use
#endif /* USE_FLOATING_POINT_IN_KERNEL */

	//bgt+	awm0			/* src > dst */

	sub	r0,dst,src
	cmplw	r0,byteCount
	bge+	awm0

	add	r8, dst, byteCount
	add	r6, src, byteCount
	mtctr	byteCount
mmc00:					/* move it all, backwards */
	subi	r6, r6, 1
	subi	r8, r8, 1
	lbz	r0, 0(r6)
	stb	r0, 0(r8)
	bdnz+	mmc00


bcopy_exit:
#if USE_FLOATING_POINT_IN_KERNEL
	bl	EXT(fpu_disable)
#endif
	mtmsr	r12

	lwz	r3,  12(r1)
	addi	r1, r1, 48

	lwz	r0, FM_LR_SAVE(r1)
	mtlr	r0
	lwz	r0, FM_CR_SAVE(r1)
	mtcr	r0

	blr

awm0:
	cmpwi	CR0, byteCount, 8
	bge	.awm2
	
	/* handle a byte at a time for short moves */
	mtctr	byteCount
.awm1:	
	lbz	r0, 0(src)
	stb	r0, 0(dst)
	addi	src, src, 1
	addi	dst, dst, 1

	bdnz	.awm1

	b	bcopy_exit


.awm2:
	/* special case long, cache-block aligned transfers */
	andi.	r0, src, CACHE_LINE_SIZE-1	/* cache block aligned? */
	bne	.awm3
	andi.	r0, dst, CACHE_LINE_SIZE-1
	bne	.awm3
	andi.	r0, byteCount, CACHE_LINE_SIZE-1
	bne	.awm3

	srwi	wordCount, byteCount, 5	    /* compute blocks to transfer */

#if USE_FLOATING_POINT_IN_KERNEL

	cmpwi	wordCount, 1
	beq	L_bcopy_one_fp

	srwi	bufA, byteCount, 6	    /* compute blocks to transfer */
	mtctr	bufA

	li	bufB,	CACHE_LINE_SIZE
#else /* USE_FLOATING_POINT_IN_KERNEL */

	mtctr	wordCount

#endif /* USE_FLOATING_POINT_IN_KERNEL */

.mmc0:

#if CACHE_LINE_SIZE < 32
#error code assumes CACHE_LINE_SIZE >= 32, and prefers it at 32 exactly
#endif

#ifndef	UNCACHED_DATA_604
	bf      CACHE_FLAG,.L_bcopy_skip_dcbz
	dcbz	0, dst
#if USE_FLOATING_POINT_IN_KERNEL
	dcbz	bufB, dst
#endif /* USE_FLOATING_POINT_IN_KERNEL */
.L_bcopy_skip_dcbz:
#endif	/* UNCACHED_DATA_604 */

#if USE_FLOATING_POINT_IN_KERNEL
		/* We can use floating point regs, this zooms */
	lfd	f0,  0(src)
	lfd	f1,  8(src)
	lfd	f2, 16(src)
	lfd	f3, 24(src)
	lfd	f4, 32(src)
	lfd	f5, 40(src)
	lfd	f6, 48(src)
	lfd	f7, 56(src)
	addi	src, src, (2*CACHE_LINE_SIZE)

	stfd	f0,  0(dst)
	stfd	f1,  8(dst)
	stfd	f2, 16(dst)
	stfd	f3, 24(dst)
	stfd	f4, 32(dst)
	stfd	f5, 40(dst)
	stfd	f6, 48(dst)
	stfd	f7, 56(dst)
	addi	dst, dst, (2*CACHE_LINE_SIZE)

#else	/* USE_FLOATING_POINT_IN_KERNEL */
	lwz	r0,   0(src)
	lwz	r3,   4(src)
	lwz	r5,   8(src)
	lwz	r6,  12(src)
	lwz	r8,  16(src)
	lwz	r9,  20(src)
	lwz	r10, 24(src)
	lwz	r11, 28(src)
	addi	src, src, CACHE_LINE_SIZE

	stw	r0,   0(dst)
	stw	r3,   4(dst)
	stw	r5,   8(dst)
	stw	r6,  12(dst)
	stw	r8,  16(dst)
	stw	r9,  20(dst)
	stw	r10, 24(dst)
	stw	r11, 28(dst)
	addi	dst, dst, CACHE_LINE_SIZE

#endif /* USE_FLOATING_POINT_IN_KERNEL */
	
	bdnz	.mmc0
	
#if USE_FLOATING_POINT_IN_KERNEL
L_bcopy_one_fp:
		/* check just in case we fell through from above */
	andi.	r0, wordCount, 1
	beq	L_bcopy_fpu_end

#ifndef	UNCACHED_DATA_604
	bf      CACHE_FLAG,.L_bcopy_skip_dcbz1
	dcbz	0, dst
.L_bcopy_skip_dcbz1:
#endif	/* UNCACHED_DATA_604 */

		/* We can use floating point regs, this zooms */
	lfd	f0,  0(src)
	lfd	f1,  8(src)
	lfd	f2, 16(src)
	lfd	f3, 24(src)

	stfd	f0,  0(dst)
	stfd	f1,  8(dst)
	stfd	f2, 16(dst)
	stfd	f3, 24(dst)
L_bcopy_fpu_end:
#endif	/* USE_FLOATING_POINT_IN_KERNEL */
	b	bcopy_exit



.awm3:	
	/* compute alignment transfer vector */
	addis	bufB, 0,	ha16(LmemcpyAlignVector)
	addi	bufB, bufB,	lo16(LmemcpyAlignVector)

	srwi.	wordCount, byteCount, 2		/* compute words to transfer */
	rlwinm	bufA, dst, 6, 24, 25
	rlwimi	bufA, src, 4, 26, 27
	rlwimi	bufA, byteCount, 2, 28, 29
	lwzx	r0, bufA, bufB
	mtctr	r0
	bctr



/* forward copy destination aligned at 0, source aligned at 0, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = 0123 4567 89ab xxxx */
	.align	4
.mm0s0c0:
	srwi	r0,	wordCount,	1
	mtctr	r0

.mm0s0c0a:
	lwz	bufA, 0(src)
	lwz	bufB, 4(src)
	stw	bufA, 0(dst)
	stw	bufB, 4(dst)
	addi	src, src, 8
	addi	dst, dst, 8
	bdnz	.mm0s0c0a

	/* if even number of words, return */
	andi.	wordCount,	wordCount,	1
	beq	bcopy_exit

	/* otherwise copy last word */
.mm0s0c0a1:
	lwz	bufA, 0(src)
	stw	bufA, 0(dst)
		
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 0, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = 0123 4567 89ab cxxx */
	.align	4
.mm0s0c1:
	mtctr	wordCount

.mm0s0c1a:
	lwz	bufA, 0(src)
	stw	bufA, 0(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm0s0c1a

	lwz	bufC, 0(dst)
	lwz	bufA, 0(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 0(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 0, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = 0123 4567 89ab cdxx */
	.align	4
.mm0s0c2:
	mtctr	wordCount
.mm0s0c2a:
	lwz	bufA, 0(src)
	stw	bufA, 0(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm0s0c2a

	lwz	bufC, 0(dst)
	lwz	bufA, 0(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 0(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 0, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = 0123 4567 89ab cdex */
	.align	4
.mm0s0c3:
	mtctr	wordCount

.mm0s0c3a:
	lwz	bufA, 0(src)
	stw	bufA, 0(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm0s0c3a

	lwz	bufC, 0(dst)
	lwz	bufA, 0(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 0(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 1, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = x012 3456 789a bxxx */
	.align	4
.mm0s1c0:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src, src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 8

.mm0s1c0a:
	lwz	bufB, 4(src)
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	src, src, 4
	slwi	bufA, bufB, 8
	addi	dst, dst, 4
	bdnz	.mm0s1c0a

	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 1, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = x012 3456 789a bcxx */
	.align	4
.mm0s1c1:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src, src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 8

.mm0s1c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 1, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = x012 3456 789a bcdx */
	.align	4
.mm0s1c2:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src, src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 8

.mm0s1c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 1, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = x012 3456 789a bcde xxxx */
	.align	4
.mm0s1c3:
	lwz	bufA, -1(src)
	lwz	bufB, 3(src)
	addi	src, src, 3
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 0, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 8

.mm0s1c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm0s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 2, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = xx01 2345 6789 abxx */
	.align	4
.mm0s2c0:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 16

.mm0s2c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c0a

	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 2, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = xx01 2345 6789 abcx */
	.align	4
.mm0s2c1:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 16

.mm0s2c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 2, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	4
.mm0s2c2:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 16

.mm0s2c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 2, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = xx01 2345 6789 abcd exxx */
	.align	4
.mm0s2c3:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 0, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 16

.mm0s2c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm0s2c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 3, byte count 0 */
/* d = 0123 4567 89ab xxxx */
/* s = xxx0 1234 5678 9abx */
	.align	4
.mm0s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 24

.mm0s3c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c0a

	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 3, byte count 1 */
/* d = 0123 4567 89ab cxxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	4
.mm0s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 24

.mm0s3c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 3, byte count 2 */
/* d = 0123 4567 89ab cdxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	4
.mm0s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 24

.mm0s3c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 0, source aligned at 3, byte count 3 */
/* d = 0123 4567 89ab cdex */
/* s = xxx0 1234 5678 9abc dexx */
	.align	4
.mm0s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 0, 7
	rlwimi	bufC, bufB, 24, 8, 31
	stw	bufC, 0(dst)
	slwi	bufA, bufB, 24

.mm0s3c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm0s3c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 0, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = 0123 4567 89ab xxxx */
	.align	4
.mm1s0c0:
	lwz	bufA, 0(src)
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufA, 24

.mm1s0c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 0, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = 0123 4567 89ab cxxx */
	.align	4
.mm1s0c1:
	lwz	bufA, 0(src)
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufA, 24

.mm1s0c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 0, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = 0123 4567 89ab cdxx */
	.align	4
.mm1s0c2:
	lwz	bufA, 0(src)
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufA, 24

.mm1s0c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 0, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = 0123 4567 89ab cdex */
	.align	4
.mm1s0c3:
	lwz	bufA, 0(src)
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufA, 24

.mm1s0c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm1s0c3a

	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 1, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = x012 3456 789a bxxx */
	.align	4
.mm1s1c0:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1

.mm1s1c0a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm1s1c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 1, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = x012 3456 789a bcxx */
	.align	4
.mm1s1c1:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1

.mm1s1c1a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm1s1c1a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 1, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = x012 3456 789a bcdx */
	.align	4
.mm1s1c2:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1

.mm1s1c2a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm1s1c2a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 1, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = x012 3456 789a bcde xxxx */
	.align	4
.mm1s1c3:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 8, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1

.mm1s1c3a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm1s1c3a

	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 2, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = xx01 2345 6789 abxx */
	.align	4
.mm1s2c0:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 8

.mm1s2c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 2, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = xx01 2345 6789 abcx */
	.align	4
.mm1s2c1:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 8

.mm1s2c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 2, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	4
.mm1s2c2:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 8

.mm1s2c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 2, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = xx01 2345 6789 abcd exxx */
	.align	4
.mm1s2c3:
	lwz	bufA, -2(src)
	lwz	bufB, 2(src)
	addi	src, src, 2
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 8, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 8

.mm1s2c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm1s2c3a

	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 3, byte count 0 */
/* d = x012 3456 789a bxxx */
/* s = xxx0 1234 5678 9abx */
	.align	4
.mm1s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 16

.mm1s3c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 3, byte count 1 */
/* d = x012 3456 789a bcxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	4
.mm1s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 16

.mm1s3c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 3, byte count 2 */
/* d = x012 3456 789a bcdx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	4
.mm1s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -1(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 16

.mm1s3c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c2a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 1, source aligned at 3, byte count 3 */
/* d = x012 3456 789a bcde xxxx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	4
.mm1s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -1(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 8, 15
	rlwimi	bufC, bufB, 16, 16, 31
	stw	bufC, -1(dst)
	subi	dst, dst, 1
	slwi	bufA, bufB, 16

.mm1s3c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm1s3c3a

	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 0, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = 0123 4567 89ab xxxx */
	.align	4
.mm2s0c0:
	lwz	bufA, 0(src)
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 16

.mm2s0c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 0, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = 0123 4567 89ab cxxx */
	.align	4
.mm2s0c1:
	lwz	bufA, 0(src)
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 16

.mm2s0c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 0, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = 0123 4567 89ab cdxx */
	.align	4
.mm2s0c2:
	lwz	bufA, 0(src)
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 16

.mm2s0c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c2a

	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 0, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = 0123 4567 89ab cdex */
	.align	4
.mm2s0c3:
	lwz	bufA, 0(src)
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 16

.mm2s0c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm2s0c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 1, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = x012 3456 789a bxxx */
	.align	4
.mm2s1c0:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 24

.mm2s1c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 1, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = x012 3456 789a bcxx */
	.align	4
.mm2s1c1:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 24

.mm2s1c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c1a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 1, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = x012 3456 789a bcdx */
	.align	4
.mm2s1c2:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 24

.mm2s1c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c2a

	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 1, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = x012 3456 789a bcde xxxx */
	.align	4
.mm2s1c3:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufA, 24

.mm2s1c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm2s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 2, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = xx01 2345 6789 abxx */
	.align	4
.mm2s2c0:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2

.mm2s2c0a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm2s2c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 2, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = xx01 2345 6789 abcx */
	.align	4
.mm2s2c1:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2

.mm2s2c1a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm2s2c1a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 2, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	4
.mm2s2c2:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2

.mm2s2c2a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm2s2c2a

	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 2, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = xx01 2345 6789 abcd exxx */
	.align	4
.mm2s2c3:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 16, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2

.mm2s2c3a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm2s2c3a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 3, byte count 0 */
/* d = xx01 2345 6789 abxx */
/* s = xxx0 1234 5678 9abx */
	.align	4
.mm2s3c0:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufB, 8

.mm2s3c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 3, byte count 1 */
/* d = xx01 2345 6789 abcx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	4
.mm2s3c1:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -2(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufB, 8

.mm2s3c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c1a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 3, byte count 2 */
/* d = xx01 2345 6789 abcd xxxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	4
.mm2s3c2:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufB, 8

.mm2s3c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c2a

	b	bcopy_exit

/* forward copy destination aligned at 2, source aligned at 3, byte count 3 */
/* d = xx01 2345 6789 abcd exxx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	4
.mm2s3c3:
	lwz	bufA, -3(src)
	lwz	bufB, 1(src)
	addi	src, src, 1
	lwz	bufC, -2(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 16, 23
	rlwimi	bufC, bufB, 8, 24, 31
	stw	bufC, -2(dst)
	subi	dst, dst, 2
	slwi	bufA, bufB, 8

.mm2s3c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm2s3c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 0, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = 0123 4567 89ab xxxx */
	.align	4
.mm3s0c0:
	lwz	bufA, 0(src)
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 8

.mm3s0c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c0a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 0, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = 0123 4567 89ab cxxx */
	.align	4
.mm3s0c1:
	lwz	bufA, 0(src)
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 8

.mm3s0c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c1a

	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 0, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = 0123 4567 89ab cdxx */
	.align	4
.mm3s0c2:
	lwz	bufA, 0(src)
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 8

.mm3s0c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 0, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = 0123 4567 89ab cdex */
	.align	4
.mm3s0c3:
	lwz	bufA, 0(src)
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 8, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 8

.mm3s0c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 8, 24, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 8
	bdnz	.mm3s0c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 1, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = x012 3456 789a bxxx */
	.align	4
.mm3s1c0:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 16

.mm3s1c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 16, 16, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 1, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = x012 3456 789a bcxx */
	.align	4
.mm3s1c1:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 16

.mm3s1c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c1a

	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 1, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = x012 3456 789a bcdx */
	.align	4
.mm3s1c2:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 16

.mm3s1c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 1, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = x012 3456 789a bcde xxxx */
	.align	4
.mm3s1c3:
	lwz	bufA, -1(src)
	subi	src, src, 1
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 16, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 16

.mm3s1c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 16, 16, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 16
	bdnz	.mm3s1c3a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 2, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = xx01 2345 6789 abxx */
	.align	4
.mm3s2c0:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 24

.mm3s2c0a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c0a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 2, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = xx01 2345 6789 abcx */
	.align	4
.mm3s2c1:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 24

.mm3s2c1a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c1a

	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 2, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = xx01 2345 6789 abcd xxxx */
	.align	4
.mm3s2c2:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 24

.mm3s2c2a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c2a

	lwz	bufC, 4(dst)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 2, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = xx01 2345 6789 abcd exxx */
	.align	4
.mm3s2c3:
	lwz	bufA, -2(src)
	subi	src, src, 2
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 24, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3
	slwi	bufA, bufA, 24

.mm3s2c3a:
	lwz	bufB, 4(src)
	addi	src, src, 4
	rlwimi	bufA, bufB, 24, 8, 31
	stw	bufA, 4(dst)
	addi	dst, dst, 4
	slwi	bufA, bufB, 24
	bdnz	.mm3s2c3a

	lwz	bufB, 4(src)
	lwz	bufC, 4(dst)
	rlwimi	bufA, bufB, 24, 8, 31
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 3, byte count 0 */
/* d = xxx0 1234 5678 9abx */
/* s = xxx0 1234 5678 9abx */
	.align	4
.mm3s3c0:
	lwz	bufA, -3(src)
	subi	src, src, 3
	lwz	bufC, -3(dst)
	addi	wordCount, wordCount, -1
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3

.mm3s3c0a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm3s3c0a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 23
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 3, byte count 1 */
/* d = xxx0 1234 5678 9abc xxxx */
/* s = xxx0 1234 5678 9abc xxxx */
	.align	4
.mm3s3c1:
	lwz	bufA, -3(src)
	subi	src, src, 3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3

.mm3s3c1a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm3s3c1a

	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 3, byte count 2 */
/* d = xxx0 1234 5678 9abc dxxx */
/* s = xxx0 1234 5678 9abc dxxx */
	.align	4
.mm3s3c2:
	lwz	bufA, -3(src)
	subi	src, src, 3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3

.mm3s3c2a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm3s3c2a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 7
	stw	bufC, 4(dst)
	b	bcopy_exit

/* forward copy destination aligned at 3, source aligned at 3, byte count 3 */
/* d = xxx0 1234 5678 9abc dexx */
/* s = xxx0 1234 5678 9abc dexx */
	.align	4
.mm3s3c3:
	lwz	bufA, -3(src)
	subi	src, src, 3
	lwz	bufC, -3(dst)
	mtctr	wordCount
	rlwimi	bufC, bufA, 0, 24, 31
	stw	bufC, -3(dst)
	subi	dst, dst, 3

.mm3s3c3a:
	lwz	bufA, 4(src)
	stw	bufA, 4(dst)
	addi	src, src, 4
	addi	dst, dst, 4
	bdnz	.mm3s3c3a

	lwz	bufC, 4(dst)
	lwz	bufA, 4(src)
	rlwimi	bufC, bufA, 0, 0, 15
	stw	bufC, 4(dst)
	b	bcopy_exit
#endif	/* PPC604 */
