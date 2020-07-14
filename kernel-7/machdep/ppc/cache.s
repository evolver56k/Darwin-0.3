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
 * 25-Feb-1998	Umesh Vaishampayan (umeshv@apple.com)
 *	Optimized flush_cache(), flush_cache_v().
 *	Moved kdp_flush_icache() to this file.
 *
 * Revision 1.1.1.1  1997/09/30 02:45:20  wsanchez
 * Import of kernel from umeshv/kernel
 *
 * Revision 1.1.1.1  1997/05/15  22:25:00  rvega
 * 	Remove invalidate_tlb. cleanupuse of sync/isync
 * 	[1997/05/15  22:25:00  rvega]
 *
 * Revision 1.1.1.1  1997/04/30  15:55:55  rvega
 * 	Add required context synchronization around tlbie. Remove unnecessary
 *	sr diddling.
 * 	[1997/04/30  16:55:55  rvega]
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

#include <machdep/ppc/asm.h>
#include <machdep/ppc/proc_reg.h>
#include <cpus.h>
#include <assym.h>
#include <mach/ppc/vm_param.h>

/*
 * extern void flush_cache(vm_offset_t pa, unsigned count);
 *
 * flush_cache takes a physical address and count to flush, thus
 * must not be called for multiple virtual pages.
 *
 * it flushes the data cache and invalidates the instruction
 * cache for the address range in question
 */

ENTRY(flush_cache, TAG_NO_FRAME_USED)

	/* Switch off data translations */
	mfmsr	r6
	rlwinm	r7,	r6,	0,	MSR_DR_BIT+1,	MSR_DR_BIT-1
	mtmsr	r7
	isync

	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	L_flush_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE

L_flush_check:
	/* Make ctr hold count of how many times we should loop */
	srwi.	r8,	ARG1,	CACHE_LINE_POW2
	/* If less than 32 to do, jump over loop */
	beq- L_flush_cache_done
	mr	r7, r8		/* remember r8 for the icbi loop */
	mr	r9, ARG1	/* remember ARG1 for icbi loop */
	mtctr	r8			/* the loop count */

L_flush_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_DATA_604
	//dcbf	ARG0,	ARG1
	dcbst	ARG0,	ARG1
#endif	/* UNCACHED_DATA_604 */
	bdnz	L_flush_loop
	sync

	mr	ARG1, r9	/* restore ARG1 */
	mtctr	r7		/* restore the loop count */
L_invalidate_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_INST_604
	icbi	ARG0,	ARG1
#endif	/* UNCACHED_INST_604 */
	bdnz	L_invalidate_loop
	sync
	isync

L_flush_cache_done:
#ifndef	UNCACHED_DATA_604
	//dcbf	0,ARG0
	dcbst	0,ARG0
	sync
#endif	/* UNCACHED_DATA_604 */
#ifndef	UNCACHED_INST_604
	icbi	0,ARG0

	sync			/* Finish physical writes */
	isync			/* Ensure data translations are on */
#endif	/* UNCACHED_INST_604 */

	mtmsr	r6		/* Restore original translations */
	isync			/* Ensure data translations are on */

	blr

/*
 * extern void flush_cache_v(vm_offset_t pa, unsigned count);
 *
 * flush_cache_v takes a virtual address and count to flush, thus
 * can be called for multiple virtual pages.
 *
 * it flushes the data cache and invalidates the instruction
 * cache for the address range in question
 */

ENTRY(flush_cache_v, TAG_NO_FRAME_USED)
	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	L_flushv_check
	addi	ARG1,ARG1,CACHE_LINE_SIZE

L_flushv_check:
	/* Make ctr hold count of how many times we should loop */
	srwi.	r8,	ARG1,	CACHE_LINE_POW2
	/* If less than 32 to do, jump over loop */
	beq- L_flush_cache_v_done
	mr	r7, r8		/* remember r8 for the icbi loop */
	mr	r9, ARG1	/* remember ARG1 for icbi loop */
	mtctr	r8

L_flushv_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_DATA_604
	//dcbf	ARG0,	ARG1
	dcbst	ARG0,	ARG1
#endif	/* UNCACHED_DATA_604 */
	bdnz	L_flushv_loop
	sync

	mr	ARG1, r9	/* restore ARG1 */
	mtctr	r7		/* restore the loop count */
L_cachev_invalidate_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_INST_604
	icbi	ARG0,	ARG1
#endif	/* UNCACHED_INST_604 */
	bdnz	L_cachev_invalidate_loop
	sync
	isync

L_flush_cache_v_done:
#ifndef	UNCACHED_DATA_604
	//dcbf	0,ARG0
	dcbst	0,ARG0
	sync
#endif	/* UNCACHED_DATA_604 */
#ifndef	UNCACHED_INST_604
	icbi	0,ARG0

	sync		/* make sure flushes have completed */
	isync
#endif	/* UNCACHED_INST_604 */

	blr

/*
 * extern void invalidate_cache_v(vm_offset_t pa, unsigned count);
 */

ENTRY(invalidate_cache_v, TAG_NO_FRAME_USED)
	/* Check to see if the address is aligned. */
	add	r8, ARG0,ARG1
	andi.	r8,r8,(CACHE_LINE_SIZE-1)
	beq-	L_invalidate_checkv
	addi	ARG1,ARG1,CACHE_LINE_SIZE

L_invalidate_checkv:
	/* Make ctr hold count of how many times we should loop */
	srwi.	r8,	ARG1,	CACHE_LINE_POW2
	/* If less than 32 to do, jump over loop */
	beq- L_invalidate_cache_v_done
	mr	r7, r8		/* remember r8 for the icbi loop */
	mr	r9, ARG1	/* remember ARG1 for icbi loop */
	mtctr	r8

L_invalidatev_loop:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_DATA_604
	dcbi	ARG0,	ARG1
	dcbi	ARG0,	ARG1	/* fix for dcbi bug */
#endif	/* UNCACHED_DATA_604 */
	bdnz	L_invalidatev_loop
	sync

	mr	ARG1, r9	/* restore ARG1 */
	mtctr	r7		/* restore the loop count */
L_invalidatev_loop2:
	subic	ARG1,	ARG1,	CACHE_LINE_SIZE
#ifndef	UNCACHED_INST_604
	icbi	ARG0,	ARG1
#endif	/* UNCACHED_INST_604 */
	bdnz	L_invalidatev_loop2
	sync
	isync

L_invalidate_cache_v_done:
#ifndef	UNCACHED_DATA_604
	dcbi	0,ARG0
	dcbi	0,ARG0		/* fix for dcbi bug */
	sync
#endif	/* UNCACHED_DATA_604 */
#ifndef	UNCACHED_INST_604
	icbi	0,ARG0

	sync		/* make sure invalidates have completed */
	isync
#endif	/* UNCACHED_INST_604 */

	blr


/*
 * void 	kdp_flush_icache(caddr_t addr,  unsigned count)
 * 
 * Flush the data cache, invalidate the code cache for all cache lines in the
 * specified range.  This enables execution of the instructions stored via the
 * data cache (e.g. code read in by noncoherent logical I/O or code generated
 * "on-the-fly").
 * 
 * NOTE: The dcbf instruction takes a logical address.  This means that addr
 * must be valid in the current address space.  Further, it "acts as a store
 * to the addressed byte with respect to address translation and protection. 
 * The reference and changed bits are set accordingly."  So, make sure the
 * page is not read-only!  Use the static mapping.
 */
.set kLog2CacheLineSize, 5
.set kCacheLineSize, 32

ENTRY(kdp_flush_icache, TAG_NO_FRAME_USED)
	cmpi	CR0,0,r4,0			// is this zero length?
	add	r4,r3,r4			// calculate last byte + 1
	subi	r4,r4,1				// calculate last byte

	srwi	r5,r3,kLog2CacheLineSize	// calc first cache line index
	srwi	r4,r4,kLog2CacheLineSize	// calc last cache line index
	beq	cr0, LdataToCodeDone		// done if zero length

	subf	r4,r5,r4			// calc diff (# lines minus 1)
	addi	r4,r4,1				// # of cache lines to flush
	slwi	r5,r5,kLog2CacheLineSize	// calc addr of first cache line

	// flush the data cache lines
	mr	r3,r5				// starting address for loop
	mtctr	r4				// loop count
LdataToCodeFlushLoop:
#ifndef UNCACHED_DATA_604
	//dcbf	0, r3				// flush the data cache line
	dcbst	0, r3				// flush the data cache line
#endif
	addi	r3,r3,kCacheLineSize		// advance to next cache line
	bdnz	LdataToCodeFlushLoop		// loop until count is zero
	sync					// wait until RAM is valid

	// invalidate the code cache lines
	mr	r3,r5				// starting address for loop
	mtctr	r4				// loop count
LdataToCodeInvalidateLoop:
#ifndef UNCACHED_INST_604
	icbi	0, r3				// invalidate code cache line
#endif
	addi	r3,r3,kCacheLineSize		// advance to next cache line
	bdnz	LdataToCodeInvalidateLoop	// loop until count is zero
	sync					// wait until last icbi completes
	isync					// discard prefetched instructions, too

LdataToCodeDone:
	blr					// return nothing
