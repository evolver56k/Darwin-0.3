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

/*	HISTORY

	1997/05/16	Rene Vega-- Cleanup sync/isync usage.
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
#include <debug.h>
#include <mach/ppc/vm_param.h>
	
	.text
	.align 2
	
/*
 * void     load_context(thread_t        thread)
 *
 * Load the context for the first kernel thread, and go.
 * TODO NMGS - assumes single CPU.
 *
 * NOTE - if DEBUG is set, the former routine is a piece
 * of C capable of printing out debug info before calling the latter,
 * otherwise both entry points are identical.
 */

#if DEBUG
ENTRY(Load_context, TAG_NO_FRAME_USED)
#else
ENTRY2(load_context, Load_context, TAG_NO_FRAME_USED)
#endif /* DEBUG */	

	/*
	 * Since this is the first thread, we came in on the interrupt
	 * stack. The first thread never returns, so there is no need to
	 * worry about saving its frame, hence we can reset the istackptr
	 * back to the saved_state structure at it's top
	 */

	addis	r11,	0,	ha16(EXT(intstack_top_ss))
	addi	r11,	r11,	lo16(EXT(intstack_top_ss))
	lwz	r0,	0(r11)

	addis	r11,	0,	ha16(EXT(istackptr))
	addi	r11,	r11,	lo16(EXT(istackptr))
	stw	r0,	0(r11)

        /*
         * get new thread pointer and set it into the active_threads pointer
         *
         * Never set any break points in these instructions since it
         * may cause a kernel stack overflow. (warning from HP code)
         */

	mfsprg	r2,	0	/* HACK - need to get around r2 problem */
	lwz	r11,	PP_CPU_DATA(r2)
	stw	ARG0,	CPU_ACTIVE_THREAD(r11)

	/* Find the new stack and store it in active_stacks */

	lwz	r1,	THREAD_KERNEL_STACK(ARG0)
	lwz	r12,	PP_ACTIVE_STACKS(r2)
	stw	r1,	0(r12)

        /*
         * This is the end of the no break points region.
         */

	/* Restore the callee save registers. The KS constants are
	 * from the bottom of the stack, pointing to a structure
	 * at the top of the stack
	 */
	lwz	r13,	KS_R13(r1)
	lwz	r14,	KS_R13+1*4(r1)
	lwz	r15,	KS_R13+2*4(r1)
	lwz	r16,	KS_R13+3*4(r1)
	lwz	r17,	KS_R13+4*4(r1)
	lwz	r18,	KS_R13+5*4(r1)
	lwz	r19,	KS_R13+6*4(r1)
	lwz	r20,	KS_R13+7*4(r1)
	lwz	r21,	KS_R13+8*4(r1)
	lwz	r22,	KS_R13+9*4(r1)
	lwz	r23,	KS_R13+10*4(r1)
	lwz	r24,	KS_R13+11*4(r1)
	lwz	r25,	KS_R13+12*4(r1)
	lwz	r26,	KS_R13+13*4(r1)
	lwz	r27,	KS_R13+14*4(r1)
	lwz	r28,	KS_R13+15*4(r1)
	lwz	r29,	KS_R13+16*4(r1)
	lwz	r30,	KS_R13+17*4(r1)
	lwz	r31,	KS_R13+18*4(r1)
	
        /*
         * Since this is the first thread, we make sure that thread_continue
         * gets a zero as its argument.
         */

	li	ARG0,	0

	lwz	r0,	KS_CR(r1)
	mtcrf	0xFF,r0
	lwz	r0,	KS_LR(r1)
	mtlr	r0
	lwz	r1,	KS_R1(r1)		/* Load new stack pointer */
	stw	ARG0,	FM_BACKPTR(r1)		/* zero backptr */
	blr			/* Jump to the continuation */
	
/* void Call_continuation( void (*continuation)(void),  vm_offset_t stack_ptr)
 */

ENTRY(Call_continuation, TAG_NO_FRAME_USED)
	mtlr	r3
	mr	r1, r4		/* Load new stack pointer */
	blr			/* Jump to the continuation */



/* struct thread_t Switch_context(thread_t	old,
 * 				  void		(*cont)(void),
 *				  thread_t	*new)
 *
 * Switch from one thread to another. If a continuation is supplied, then
 * we do not need to save callee save registers.
 *
 * TODO NMGS Assumes single CPU
 */

ENTRY(Switch_context, TAG_NO_FRAME_USED)

	/*
	 * Get the old kernel stack, and store into the thread structure.
	 * See if a continuation is supplied, and skip state save if so.
	 * NB. Continuations are no longer used, so this test is omitted,
	 * as should the second argument, but it is in generic code.
	 * We always save state. This does not hurt even if continuations
	 * are put back in.
	 */

	stw	r2,	FM_TOC_SAVE(r1)
	mfsprg	r2,	0	/* HACK - need to get around r2 problem */
	lwz	r12,	PP_ACTIVE_STACKS(r2)
	cmpwi   CR0,    ARG1,   0               /* Continuation? */
	lwz	r11,	0(r12)
	stw	ARG1,	THREAD_SWAPFUNC(ARG0)
	stw	r11,	THREAD_KERNEL_STACK(ARG0)
	bne     CR0,    .L_sw_ctx_no_save       /* Yes, skip save */
	/*
	 * Save all the callee save registers plus state.
	 */

	/* the KS_ constants point to the offset from the bottom of the
	 * stack to the ppc_kernel_state structure at the top, thus
	 * r11 can be used directly
	 */	
	stw	r1,	KS_R1(r11)
	lwz	r0,	FM_TOC_SAVE(r1)
	stw	r0,	KS_R2(r11)
	stw	r13,	KS_R13(r11)
	stw	r14,	KS_R13+1*4(r11)
	stw	r15,	KS_R13+2*4(r11)
	stw	r16,	KS_R13+3*4(r11)
	stw	r17,	KS_R13+4*4(r11)
	stw	r18,	KS_R13+5*4(r11)
	stw	r19,	KS_R13+6*4(r11)
	stw	r20,	KS_R13+7*4(r11)
	stw	r21,	KS_R13+8*4(r11)
	stw	r22,	KS_R13+9*4(r11)
	stw	r23,	KS_R13+10*4(r11)
	stw	r24,	KS_R13+11*4(r11)
	stw	r25,	KS_R13+12*4(r11)
	stw	r26,	KS_R13+13*4(r11)
	stw	r27,	KS_R13+14*4(r11)
	stw	r28,	KS_R13+15*4(r11)
	stw	r29,	KS_R13+16*4(r11)
	stw	r30,	KS_R13+17*4(r11)
	stw	r31,	KS_R13+18*4(r11)
	
	mfcr	r0
	stw	r0,	KS_CR(r11)
	mflr	r0
	stw	r0,	KS_LR(r11)

	/*
	 * Make the new thread the current thread.
	 */
.L_sw_ctx_no_save:
	
	lwz	r11,	PP_CPU_DATA(r2)
	stw	ARG2,	CPU_ACTIVE_THREAD(r11)
	
	lwz	r11,	THREAD_KERNEL_STACK(ARG2)

	lwz	r10,	PP_ACTIVE_STACKS(r2)
	stw	r11,	0(r10)

	/*
	 * Restore all the callee save registers.
	 */
	lwz	r1,	KS_R1(r11)	/* Restore stack pointer */

	/* Restore the callee save registers */
	lwz	r13,	KS_R13(r11)
	lwz	r14,	KS_R13+1*4(r11)
	lwz	r15,	KS_R13+2*4(r11)
	lwz	r16,	KS_R13+3*4(r11)
	lwz	r17,	KS_R13+4*4(r11)
	lwz	r18,	KS_R13+5*4(r11)
	lwz	r19,	KS_R13+6*4(r11)
	lwz	r20,	KS_R13+7*4(r11)
	lwz	r21,	KS_R13+8*4(r11)
	lwz	r22,	KS_R13+9*4(r11)
	lwz	r23,	KS_R13+10*4(r11)
	lwz	r24,	KS_R13+11*4(r11)
	lwz	r25,	KS_R13+12*4(r11)
	lwz	r26,	KS_R13+13*4(r11)
	lwz	r27,	KS_R13+14*4(r11)
	lwz	r28,	KS_R13+15*4(r11)
	lwz	r29,	KS_R13+16*4(r11)
	lwz	r30,	KS_R13+17*4(r11)
	lwz	r31,	KS_R13+18*4(r11)

	lwz	r2,	KS_R2(r11)
	lwz	r0,	KS_CR(r11)
	mtcrf	0xFF,	r0
	lwz	r0,	KS_LR(r11)
	mtlr	r0
	
	/* ARG0 still holds old thread pointer, we return this */
	
	blr		/* Jump into the new thread */

