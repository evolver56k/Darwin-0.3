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

#include <cpus.h>
#include <assym.h>
#include <debug.h>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>

	
	.text
/*
 * void fpu_save(void)
 *
 * Called when there's an exception or when get_state is called 
 * for the FPU state. Puts a copy of the current thread's state
 * into the PCB pointed to by fpu_pcb, and leaves the FPU enabled.
 *
 * video_scroll.s assumes that ARG0-5 won't be trampled on, and
 * that fpu_save doesn't need a frame, otherwise it would need
 * a frame itself. Why not oblige!
 *
 * NOTE:	This routine is meant for kernel Floating Point
 *	use.  It is called to flush the current context to the correct
 *	pcb and leaves with MSR_FP on so the kernel can use the FPU.
 *
 *	IT IS THE CALLERS RESPONSIBILITY TO TURN THE FPU OFF AFTER USE!!!
 *
 *	The function fpu_disable is used to accomplish this from C language
 *	programs or is can be done at the end of an assembly routine.
 */

ENTRY(fpu_save, TAG_NO_FRAME_USED)
	/*
	 * Turn the FPU back on (should this be a separate routine?)
	 */
	mfmsr	r0
	andi.	r0,	r0,	MASK(MSR_FP)
	bne	.L_fpu_save_no_owner

	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_FP)	/* bit is in low-order 16 */
	mtmsr	r0
	isync

	mfsprg	ARG7,	0	/* HACK - need to get around r2 problem */

	lwz	ARG6,	PP_FPU_PCB(ARG7)
	
	/*
	 * See if any thread owns the FPU. If not, we can skip the state save.
	 */
	cmpwi	CR0,	ARG6,	0
	beq-	CR0,	.L_fpu_save_no_owner

	/*
	 * Save the current FPU state into the PCB of the thread that owns it.
	 */
        stfd    f0,   PCB_FS_F0(ARG6)
        stfd    f1,   PCB_FS_F1(ARG6)
        stfd    f2,   PCB_FS_F2(ARG6)
        stfd    f3,   PCB_FS_F3(ARG6)
        stfd    f4,   PCB_FS_F4(ARG6)
        stfd    f5,   PCB_FS_F5(ARG6)
        stfd    f6,   PCB_FS_F6(ARG6)
        stfd    f7,   PCB_FS_F7(ARG6)
		mffs    f0			/* fpscr in f0 low 32 bits*/
        stfd    f8,   PCB_FS_F8(ARG6)
        stfd    f9,   PCB_FS_F9(ARG6)
        stfd    f10,  PCB_FS_F10(ARG6)
        stfd    f11,  PCB_FS_F11(ARG6)
        stfd    f12,  PCB_FS_F12(ARG6)
        stfd    f13,  PCB_FS_F13(ARG6)
        stfd    f14,  PCB_FS_F14(ARG6)
        stfd    f15,  PCB_FS_F15(ARG6)
                stfd    f0,  PCB_FS_FPSCR(ARG6)	/* Store junk 32 bits+fpscr */
        stfd    f16,  PCB_FS_F16(ARG6)
        stfd    f17,  PCB_FS_F17(ARG6)
        stfd    f18,  PCB_FS_F18(ARG6)
        stfd    f19,  PCB_FS_F19(ARG6)
        stfd    f20,  PCB_FS_F20(ARG6)
        stfd    f21,  PCB_FS_F21(ARG6)
        stfd    f22,  PCB_FS_F22(ARG6)
        stfd    f23,  PCB_FS_F23(ARG6)
        stfd    f24,  PCB_FS_F24(ARG6)
        stfd    f25,  PCB_FS_F25(ARG6)
        stfd    f26,  PCB_FS_F26(ARG6)
        stfd    f27,  PCB_FS_F27(ARG6)
        stfd    f28,  PCB_FS_F28(ARG6)
        stfd    f29,  PCB_FS_F29(ARG6)
        stfd    f30,  PCB_FS_F30(ARG6)
        stfd    f31,  PCB_FS_F31(ARG6)

	/* Mark the FPU as having no owner now */
	li	r0,	0
	stw	r0,	PP_FPU_PCB(ARG7)

	/*
	 * Turn off the FPU for the old owner
	 */
	lwz	r0,	SS_SRR1(ARG6)
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r0,	SS_SRR1(ARG6)
	
.L_fpu_save_no_owner:
	blr


ENTRY(fpu_save_thread, TAG_NO_FRAME_USED)

	/*
	 * Turn the FPU back on (should this be a separate routine?)
	 */

	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_FP)	/* bit is in low-order 16 */
	mtmsr	r0
	isync

	mfsprg	ARG7,	0	/* HACK - need to get around r2 problem */

	lwz	ARG6,	PP_FPU_PCB(ARG7)
	/*
	 * See if any thread owns the FPU. If not, we can skip the state save.
	 */
	cmpwi	CR0,	ARG6,	0
	beq-	CR0,	.L_fpu_save_thread_no_owner

	/*
	 * Save the current FPU state into the PCB of the thread that owns it.
	 */
        stfd    f0,   PCB_FS_F0(ARG6)
        stfd    f1,   PCB_FS_F1(ARG6)
        stfd    f2,   PCB_FS_F2(ARG6)
        stfd    f3,   PCB_FS_F3(ARG6)
        stfd    f4,   PCB_FS_F4(ARG6)
        stfd    f5,   PCB_FS_F5(ARG6)
        stfd    f6,   PCB_FS_F6(ARG6)
        stfd    f7,   PCB_FS_F7(ARG6)
		mffs    f0			/* fpscr in f0 low 32 bits*/
        stfd    f8,   PCB_FS_F8(ARG6)
        stfd    f9,   PCB_FS_F9(ARG6)
        stfd    f10,  PCB_FS_F10(ARG6)
        stfd    f11,  PCB_FS_F11(ARG6)
        stfd    f12,  PCB_FS_F12(ARG6)
        stfd    f13,  PCB_FS_F13(ARG6)
        stfd    f14,  PCB_FS_F14(ARG6)
        stfd    f15,  PCB_FS_F15(ARG6)
                stfd    f0,  PCB_FS_FPSCR(ARG6)	/* Store junk 32 bits+fpscr */
        stfd    f16,  PCB_FS_F16(ARG6)
        stfd    f17,  PCB_FS_F17(ARG6)
        stfd    f18,  PCB_FS_F18(ARG6)
        stfd    f19,  PCB_FS_F19(ARG6)
        stfd    f20,  PCB_FS_F20(ARG6)
        stfd    f21,  PCB_FS_F21(ARG6)
        stfd    f22,  PCB_FS_F22(ARG6)
        stfd    f23,  PCB_FS_F23(ARG6)
        stfd    f24,  PCB_FS_F24(ARG6)
        stfd    f25,  PCB_FS_F25(ARG6)
        stfd    f26,  PCB_FS_F26(ARG6)
        stfd    f27,  PCB_FS_F27(ARG6)
        stfd    f28,  PCB_FS_F28(ARG6)
        stfd    f29,  PCB_FS_F29(ARG6)
        stfd    f30,  PCB_FS_F30(ARG6)
        stfd    f31,  PCB_FS_F31(ARG6)

	/*
	 * Turn off the FPU for the old owner
	 */
	lwz	r0,	SS_SRR1(ARG6)
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r0,	SS_SRR1(ARG6)

	/* Store current PCB address in fpu_pcb to claim fpu for thread */
	lwz	ARG6,	PP_CPU_DATA(ARG7)
	lwz	ARG6,	CPU_ACTIVE_THREAD(ARG6)
	lwz	ARG6,	THREAD_PCB(ARG6)
	stw	ARG6,	PP_FPU_PCB(ARG7)
	
.L_fpu_save_thread_no_owner:
	blr

/*
 * fpu_restore()
 *
 * restore the current user thread Floating-Point context in the FPU and
 * set the FPU ownership to the current user thread.
 *
 * This code runs in virtual address mode with interrupts off.
 *
 */     

        .text
ENTRY(fpu_restore, TAG_NO_FRAME_USED) 

	mfsprg	r4, 0	/* load per_proc_info pointer */

	/* See if the current thread owns the FPU, if so, just return */
	lwz	r3, PP_FPU_PCB(r4)
 
	lwz	r5, PP_CPU_DATA(r4)
	lwz	r5, CPU_ACTIVE_THREAD(r5)
	lwz	r5, THREAD_PCB(r5)

	cmpw	cr0, r5, r3
	beq	cr0, .L_fpu_restore_ret

	/* Store current PCB address in fpu_pcb to claim fpu for thread */
	stw	r5, PP_FPU_PCB(r4)

	/* restore current user thread Floating-Point context */
	lfd	f31, PCB_FS_FPSCR(r5)	/* Load junk 32 bits+fpscr */
	lfd	f0, PCB_FS_F0(r5)
	lfd	f1, PCB_FS_F1(r5)
	lfd	f2, PCB_FS_F2(r5)
	lfd	f3, PCB_FS_F3(r5)
	lfd	f4, PCB_FS_F4(r5)
	lfd	f5, PCB_FS_F5(r5)
	lfd	f6, PCB_FS_F6(r5)
	lfd	f7, PCB_FS_F7(r5)
	mtfsf	0xff, f31		/* fpscr in f0 low 32 bits*/
	lfd	f8, PCB_FS_F8(r5)
	lfd	f9, PCB_FS_F9(r5)
	lfd	f10, PCB_FS_F10(r5)
	lfd	f11, PCB_FS_F11(r5)
	lfd	f12, PCB_FS_F12(r5)
	lfd	f13, PCB_FS_F13(r5)
	lfd	f14, PCB_FS_F14(r5)
	lfd	f15, PCB_FS_F15(r5)
	lfd	f16, PCB_FS_F16(r5)
	lfd	f17, PCB_FS_F17(r5)
	lfd	f18, PCB_FS_F18(r5)
	lfd	f19, PCB_FS_F19(r5)
	lfd	f20, PCB_FS_F20(r5)
	lfd	f21, PCB_FS_F21(r5)
	lfd	f22, PCB_FS_F22(r5)
	lfd	f23, PCB_FS_F23(r5)
	lfd	f24, PCB_FS_F24(r5)
	lfd	f25, PCB_FS_F25(r5)
	lfd	f26, PCB_FS_F26(r5)
	lfd	f27, PCB_FS_F27(r5)
	lfd	f28, PCB_FS_F28(r5)
	lfd	f29, PCB_FS_F29(r5)
	lfd	f30, PCB_FS_F30(r5)
	lfd	f31, PCB_FS_F31(r5)

	/* Turn back on the FPU in the current user thread saved context */
	lwz	r4, SS_SRR1(r5)
	ori	r4, r4, MASK(MSR_FP)    /* bit is in low-order 16 */
	stw	r4, SS_SRR1(r5)

.L_fpu_restore_ret:
	blr


/*
 * fpu_switch()
 *
 * Jumped to by the floating-point unavailable exception handler to
 * switch fpu context in a lazy manner.
 *
 * This code is run in virtual address mode with interrupts off.
 * It is assumed that the pcb in question is in memory
 *
 * Upon exit, the code returns to the users context with the MSR_FP for the
 * previous FPU owner disabled.
 *
 * ENTRY:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
 * r1 is used as a temporary register throught this code, not as a stack
 * pointer.
 * 
 */

	.text
ENTRY(fpu_switch, TAG_NO_FRAME_USED)

#if DEBUG
	lwz	r3,	PP_SAVE_SRR1(r2)
	andi.	r1,	r3,	MASK(MSR_PR)
	bne+	.L_fpu_switch_user
	bl	EXT(fpu_panic)
.L_fpu_switch_user:
#endif /* DEBUG */

	/*
	 * Turn the FPU back on
	 */
	mfmsr	r3
	ori	r3,	r3,	MASK(MSR_FP)	/* bit is in low-order 16 */
	mtmsr	r3
	isync

	/* See if the current thread owns the FPU, if so, just return */
	lwz	r3,	PP_FPU_PCB(r2)

	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)

	cmpw	CR0,	r1,	r3	/* If we own FPU, just return */
	beq	CR0,	.L_fpu_switch_return
	cmpwi	CR1,	r3,	0	/* If no owner, skip save */
	beq	CR1,	.L_fpu_switch_load_state

/*
 * Identical code to that of fpu_save but is inlined
 * to avoid creating a stack frame etc.
 */
        stfd    f0,   PCB_FS_F0(r3)
        stfd    f1,   PCB_FS_F1(r3)
        stfd    f2,   PCB_FS_F2(r3)
        stfd    f3,   PCB_FS_F3(r3)
        stfd    f4,   PCB_FS_F4(r3)
        stfd    f5,   PCB_FS_F5(r3)
        stfd    f6,   PCB_FS_F6(r3)
        stfd    f7,   PCB_FS_F7(r3)
                mffs    f0			/* fpscr in f0 low 32 bits*/
        stfd    f8,   PCB_FS_F8(r3)
        stfd    f9,   PCB_FS_F9(r3)
        stfd    f10,  PCB_FS_F10(r3)
        stfd    f11,  PCB_FS_F11(r3)
        stfd    f12,  PCB_FS_F12(r3)
        stfd    f13,  PCB_FS_F13(r3)
        stfd    f14,  PCB_FS_F14(r3)
        stfd    f15,  PCB_FS_F15(r3)
                stfd    f0,  PCB_FS_FPSCR(r3)	/* Store junk 32 bits+fpscr */
        stfd    f16,  PCB_FS_F16(r3)
        stfd    f17,  PCB_FS_F17(r3)
        stfd    f18,  PCB_FS_F18(r3)
        stfd    f19,  PCB_FS_F19(r3)
        stfd    f20,  PCB_FS_F20(r3)
        stfd    f21,  PCB_FS_F21(r3)
        stfd    f22,  PCB_FS_F22(r3)
        stfd    f23,  PCB_FS_F23(r3)
        stfd    f24,  PCB_FS_F24(r3)
        stfd    f25,  PCB_FS_F25(r3)
        stfd    f26,  PCB_FS_F26(r3)
        stfd    f27,  PCB_FS_F27(r3)
        stfd    f28,  PCB_FS_F28(r3)
        stfd    f29,  PCB_FS_F29(r3)
        stfd    f30,  PCB_FS_F30(r3)
        stfd    f31,  PCB_FS_F31(r3)

	stw	r0,	SS_R0(r1)
	/*
	 * Turn off the FPU for the old owner
	 */
	lwz	r0,	SS_SRR1(r3)
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r0,	SS_SRR1(r3)

	lwz	r0,	SS_R0(r1)
	
	/* Now load in the current threads state */

.L_fpu_switch_load_state:	

	/* Store current PCB address in fpu_pcb to claim fpu for thread */
	stw	r1,	PP_FPU_PCB(r2)

	    lfd    f31,  PCB_FS_FPSCR(r1)	/* Load junk 32 bits+fpscr */
        lfd     f0,   PCB_FS_F0(r1)
        lfd     f1,   PCB_FS_F1(r1)
        lfd     f2,   PCB_FS_F2(r1)
        lfd     f3,   PCB_FS_F3(r1)
        lfd     f4,   PCB_FS_F4(r1)
        lfd     f5,   PCB_FS_F5(r1)
        lfd     f6,   PCB_FS_F6(r1)
        lfd     f7,   PCB_FS_F7(r1)
            mtfsf	0xff,	f31		/* fpscr in f0 low 32 bits*/
        lfd     f8,   PCB_FS_F8(r1)
        lfd     f9,   PCB_FS_F9(r1)
        lfd     f10,  PCB_FS_F10(r1)
        lfd     f11,  PCB_FS_F11(r1)
        lfd     f12,  PCB_FS_F12(r1)
        lfd     f13,  PCB_FS_F13(r1)
        lfd     f14,  PCB_FS_F14(r1)
        lfd     f15,  PCB_FS_F15(r1)
        lfd     f16,  PCB_FS_F16(r1)
        lfd     f17,  PCB_FS_F17(r1)
        lfd     f18,  PCB_FS_F18(r1)
        lfd     f19,  PCB_FS_F19(r1)
        lfd     f20,  PCB_FS_F20(r1)
        lfd     f21,  PCB_FS_F21(r1)
        lfd     f22,  PCB_FS_F22(r1)
        lfd     f23,  PCB_FS_F23(r1)
        lfd     f24,  PCB_FS_F24(r1)
        lfd     f25,  PCB_FS_F25(r1)
        lfd     f26,  PCB_FS_F26(r1)
        lfd     f27,  PCB_FS_F27(r1)
        lfd     f28,  PCB_FS_F28(r1)
        lfd     f29,  PCB_FS_F29(r1)
        lfd     f30,  PCB_FS_F30(r1)
        lfd     f31,  PCB_FS_F31(r1)

.L_fpu_switch_return:

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */

	addis	r3,	0,	ha16(EXT(exception_exit))
	addi	r3,	r3,	lo16(EXT(exception_exit))
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3
	

	lwz	r1,	PCB_SR0(r1)		/* restore current sr0 */

	lwz	r3,	PP_SAVE_CR(r2)
	mtcrf	0xFF,r3
	lwz	r3,	PP_SAVE_SRR1(r2)
	lwz	r2,	PP_SAVE_SRR0(r2)
	/* Enable floating point for the thread we're returning to */
	ori	r3,	r3,	MASK(MSR_FP)	/* bit is in low-order 16 */
	
	/* Return to the trapped context */
	rfi

/*
 * void fpu_disable(void)
 *
 * disable the fpu in the current msr
 *
 */

ENTRY(fpu_disable, TAG_NO_FRAME_USED)

	/* See if the current thread owns the FPU, if so, just return */
	mfsprg	r2,	0		/* HACK - need to get around r2 problem */
	lwz	r0,	PP_FPU_PCB(r2)
	cmpwi	CR1,	r0,	0	/* If no owner, skip save */
	bne	CR1,	.L_fpu_disable_not

	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	mtmsr	r0
	isync
.L_fpu_disable_not:
	blr

ENTRY(fpu_disable_thread, TAG_NO_FRAME_USED)

	/* Mark the FPU as having no owner now */
	mfsprg	r2,	0		/* HACK - need to get around r2 problem */

	lwz	r0,	PP_FPU_PCB(r2)
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_PCB(r3)

	cmpw	CR0,	r0,	r3	/* If we own FPU, disown it */
	bne	CR0,	.L_thread_not_own

	li	r0,	0
	stw	r0,	PP_FPU_PCB(r2)
.L_thread_not_own:

	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	mtmsr	r0
	isync
	blr

/*
 * void lfs(fpsp,fpdp)
 *
 * load the single precision float to the double
 *
 * This routine is used by the alignment handler.
 *
 */
ENTRY(lfs, TAG_NO_FRAME_USED)
        lfs     f1,	0(r3)
	stfd	f1,	0(r4)
	blr

/*
 * fpsp stfs(fpdp,fpsp)
 *
 * store the double precision float to the single
 *
 * This routine is used by the alignment handler.
 *
 */
ENTRY(stfs, TAG_NO_FRAME_USED)
	lfd	f1,	0(r3)
        stfs	f1,	0(r4)
	blr

