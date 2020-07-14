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

/* Low level routines dealing with exception entry and exit.
 * There are various types of exception:
 *
 *    Interrupt, trap, system call and debugger entry. Each has it's own
 *    handler since the state save routine is different for each. The
 *    code is very similar (a lot of cut and paste).
 *
 *    The code for the FPU disabled handler (lazy fpu) is in cswtch.s
 */

#include <debug.h>
#include <mach_assert.h>

#include <mach/exception.h>
#include <mach/ppc/vm_param.h>

#include <assym.h>

#include <machdep/ppc/asm.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/trap.h>
#include <machdep/ppc/exception.h>
#include <kernserv/ppc/spl.h>
#include <machdep/ppc/machspl.h>
#include <kern/ast.h>
	
/*
 * thandler(type)
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info 
 *		r3 = exception type (one of EXC_...)
 */

/*
 * If pcb.ksp == 0 then the kernel stack is already busy,
 *                 we save ppc_saved state below the current stack pointer,
 *		   leaving enough space for the `red zone' in case the
 *		   trapped thread was in the middle of saving state below
 *		   its stack pointer.
 *
 * otherwise       we save a ppc_saved_state in the pcb, and switch to
 * 		   the kernel stack (setting pcb.ksp to 0)
 *
 * on return, we do the reverse, the last state is popped from the pcb
 * and pcb.ksp is set to the top of stack below the kernel state + frame
 * TODO NMGS - is this frame used? I don't think so
 *
 *                    Diagram of a thread's kernel stack
 *
 *               --------------- 	TOP OF STACK
 *              |kernel_state   |
 *              |---------------|
 *              |backpointer FM |
 *              |---------------|                         	   	
 *              |... C usage ...|                         	   	
 *              |               |                         	   	
 *              |---------------|                       TRAP IN KERNEL CODE
 *              |ppc_saved_state|                       STATE SAVED HERE   
 *              |---------------|                         		
 *              |backpointer FM |                                       
 *              |---------------|                         
 *              |... C usage ...|                         
 *              |               |                         
 *              |               |                         
 *              |               |                         
 *              |               |                         
 */


#if DEBUG

/* TRAP_SPACE_NEEDED is the space assumed free on the kernel stack when
 * another trap is taken. We need at least enough space for a saved state
 * structure plus two small backpointer frames, and we add a few
 * hundred bytes for the space needed by the C (which may be less but
 * may be much more). We're trying to catch kernel stack overflows :-)
 */

#define TRAP_SPACE_NEEDED	FM_REDZONE+SS_SIZE+(2*FM_SIZE)+256

#endif /* DEBUG */

	.text

ENTRY(thandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

		/* If we're on the gdb stack, there has probably been
		 * a fault reading user memory or something like that,
		 * so we should pass this to the gdb handler. NOTE
		 * we may have entered gdb through an interrupt handler
		 * (keyboard or serial line, for example), so interrupt
		 * stack may be busy too.
		 */
	addis	r1,	0,	ha16(EXT(gdbstackptr))
	addi	r1,	r1,	lo16(EXT(gdbstackptr))	/* TODO assumes 1 CPU */
	lwz	r1,	0(r1)
	cmpwi	CR0,	r1,	0
	beq-	CR0,	EXT(gdbhandler)
	
#if DEBUG
		/* Make sure we're not on the interrupt stack */
	addis	r1,	0,	ha16(EXT(istackptr))
	addi	r1,	r1,	lo16(EXT(istackptr))
	lwz	r1,	0(r1)
	cmpwi	CR0,	r1,	0

	/* If we are on the interrupt stack, treat as an interrupt,
	 * the interrupt handler will panic with useful info.
	 */

	beq-	CR0,	EXT(ihandler)
	
#endif /* DEBUG */

	lwz	r3,	PP_CPU_DATA(r2)

	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_PCB(r3)
	lwz	r1,	PCB_KSP(r3)

	cmpwi	CR1,	r1,	0	/* zero implies already on kstack */
	bne	CR1,	.L_kstackfree	/* This test is also used below */

	mfsprg	r1,	1		/* recover previous stack ptr */

	/* On kernel stack, allocate stack frame and check for overflow */

	/* Move stack pointer below redzone + reserve a saved_state */

	subi	r1,	r1,	FM_REDZONE+SS_SIZE 

	b	.L_kstack_save_state

.L_kstackfree:
	mr	r1,	r3		/* r1 points to save area of pcb */

.L_kstack_save_state:	

	/* Once we reach here, r1 contains the place
         * where we can store a ppc_saved_state structure. This may
	 * or may not be part of a pcb, we test that again once
	 * we've saved state. (CR1 still holds test done on ksp)
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)

	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 * pointed to by r2
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r0,	PP_SAVE_SRR0(r2)
	stw	r0,	SS_SRR0(r1)

	/* WARNING - r0 from the following instruction is used
	 * further below
	 */

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)


	/* WARNING! These two instructions assume that we didn't take
	 * any type of exception whilst saving state, it's a bit late
	 * for that!
	 * TODO NMGS move these up the code somehow, put in PROC_REG?
	 */

	mfdsisr	ARG2			/* r4	*/
	mfdar	ARG3			/* r5	*/

	/* work out if we will reenable interrupts or not depending
	 * upon the state which we came from, store as tmp in ARG5
	 */
	li	ARG5,	MSR_SUPERVISOR_INT_OFF
	rlwimi	ARG5,	r0,	0,	MSR_EE_BIT,	MSR_EE_BIT

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	mfctr	r0
	stw	r0,	SS_CTR(r1)

	/* Don't save MQ, we don't bother for now */
	
	/* Free the reservation whilst saving SR_COPYIN */

	mfsr	r0,	SR_COPYIN_NAME
	li	ARG7,	SS_SR_COPYIN
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	ARG7,	r1
	stw	r0,	SS_SR_COPYIN(r1)
	
	/* r3 still holds our pcb, CR1 still holds test to see if we're
	 * in the pcb or have saved state on the kernel stack */

	mr	ARG1,	r1		/* Preserve saved_state ptr in ARG1 */

	beq	CR1,	.L_state_on_kstack/* using above test for pcb/stack */

	/* We saved state in the pcb, recover the stack pointer */
	lwz	r1,	PCB_KSP(r3)

	/* Mark that we're occupying the kernel stack for sure now */	
	li	r0,	0
	stw	r0,	PCB_KSP(r3)

.L_state_on_kstack:	
		
	/* Phew!
	 *
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure either in the pcb or on
	 * the kernel stack, and the stack is marked as busy.
	 * r4 holds a pointer to this state, r1 is now the stack
	 * pointer no matter where the state was savd.
	 * We now generate a small stack frame with backpointers
	 * to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 */

/* WARNING!! Using mfsprg below assumes interrupts are still off here */
	
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */
	lwz	r0,	SS_SRR0(r1)
	stw	r0,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */

	//stwu	r1,	-FM_SIZE(r1)	/* and make new frame */
	stw	r1,	-FM_SIZE(r1)	/* and make new frame */
	subi	r1,	r1,	FM_SIZE
#endif /* DEBUG */


	/* call trap handler proper, with
	 *   ARG0 = type		(not yet, holds pcb ptr)
	 *   ARG1 = saved_state ptr	(already there)
	 *   ARG2 = dsisr		(already there)
	 *   ARG3 = dar			(already there)
	 */

	/* This assumes that no (non-tlb) exception/interrupt has occured
	 * since PP_SAVE_* get clobbered by an exception...
	 */
	lwz	ARG0,	PP_SAVE_EXCEPTION_TYPE(r2)

	/* Reenable interrupts if they were enabled before we came here */
	mtmsr	ARG5
	isync

	/* syscall exception might warp here if there's nothing left
	 * to do except generate a trap
	 */
.L_call_trap:	
	bl	EXT(trap)

	/*
	 * Ok, return from C function
	 *
	 * This is also the point where new threads come when they are created.
	 * The new thread is setup to look like a thread that took an 
	 * interrupt and went immediatly into trap.
	 *
	 * r3 must hold the pointer to the saved state, either on the
	 * stack or in the pcb.
	 */

thread_return:
	/* Reload the saved state */

	/* r0-3 will be restored last, use as temp for now */

	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_XER(r3)
	mtxer	r0
	lwz	r0,	SS_LR(r3)
	mtlr	r0
	lwz	r0,	SS_CTR(r3)
	mtctr	r0
	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN_NAME,	r0
	isync
	

	/* TODO NMGS don't restore mq since we're not 601-specific enough */

	/* Disable interrupts */
	li	r0, MSR_SUPERVISOR_INT_OFF
	mtmsr	r0


	/* Is this the last saved state, found in the pcb? */
	/* TODO NMGS optimise this by spreading it through the code above? */

	/* After this we no longer to keep &per_proc_info in r2 */
	
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r0,	THREAD_PCB(r1)

	cmp	CR0,0,	r0,	r3
	bne	CR0,	.L_notthelast_trap

	/* our saved state is actually part of the thread's pcb so
	 * we need to mark that we're leaving the kernel stack and
	 * jump into user space
	 */

	/* Mark the kernel stack as free */

	/* There may be a critical region here for traps(interrupts?)
	 * once the stack is marked as free, PCB_SR0 may be trampled on
	 * so interrupts should be switched off
	 */
	/* Release any processor reservation we may have had too */

	lwz	r2,	THREAD_KERNEL_STACK(r1)
	addi	r0,	r2,	KSTK_SIZE-KS_SIZE-FM_SIZE
	li	r2,	PCB_KSP
/* we have to use an indirect store to clear reservation */
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	r2,	r3		/* clear reservation */
	stw	r0,	PCB_KSP(r3)		/* mark stack as free */
	
	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_trap_ret_to_kspace
	
	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 */
		
	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r1,	0,	ha16(EXT(exception_exit))
	addi	r1,	r1,	lo16(EXT(exception_exit))
	lwz	r1,	0(r1)
	mtsrr0	r1
	li	r1,	MSR_VM_OFF
	mtsrr1	r1

	
	lwz	r1,	PCB_SR0(r3)	/* For trampoline */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */

	rfi

.L_trap_ret_to_kspace:	
.L_notthelast_trap:
	/* If we're not the last trap on the kernel stack life is easier,
	 * we don't need to switch back into the user's segment. we can
	 * simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	/* critical region for traps(interrupt?) since r1 no longer points
	 * to bottom of stack. Could be fixed. But interrupts are off (?).
	 */
	lwz	r2,	SS_R2(r3)	/* r2 is a constant (&per_proc_info) */
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */



/*QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ
 * void			CallPseudoKernel	( void )
 *
 * This op provides a means of invoking the BlueBox PseudoKernel from a
 * system (68k) or native (PPC) context while changing BlueBox interruption
 * state atomically. As an added bonus, this op clobbers only r0 while leaving
 * the rest of PPC user state registers intact.
 *
 * This op is invoked as follows:
 *	li r0, kCallPseudoKernelNumber	// load this op's firmware call number
 *	sc				// invoke CallPseudoKernel
 *	dc.l	CallPseudoKernelDescriptorPtr	// static pointer to CallPseudoKernelDescriptor
 *
 * NOTE: The CallPseudoKernelDescriptor and the word pointed to by
 * intControlAddr must be locked, else this op will crash the kernel.
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
	.align	5
					
__fcCallPseudoKernel:

	// Needed to save some state so this code matches NuKernel Support
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r3)

	stw	r0,	SS_R0(r1)

	lwz	r0,	PP_SAVE_SRR0(r2)
	stw	r0,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)

	// word following the sc is the descriptor's address
	lwz	r3,	SS_SRR0(r1)

	lwz	r10,	SS_CR(r1)	// setup r10 with CR

	lwz	r3,	0(r3)		// get descriptor's address

	lwz	r11,	CPKD_INTCONTROLADDR(r3)
	lwz	r4,	CPKD_PC(r3)
	lwz	r6,	CPKD_NEWSTATE(r3)
	lwz	r7,	CPKD_INTSTATEMASK(r3)
	lwz	r8,	0(r11)		// get current interruption control word
	lwz	r5,	CPKD_GPR0(r3)
	lwz	r12,	CPKD_SYSCONTEXTSTATE(r3)
	andc	r9, r8, r7		// remove current state
	and	r8, r8, r7		// extract current state
	cmplw	r8, r12			// test for entry from system context
	or	r9, r9, r6		// insert new state
	bne	CallFromAlternateContext		

CallFromSystemContext:
	lwz	r6,	CPKD_INTCR2SHIFT(r3)
	lwz	r7,	CPKD_INTCR2MASK(r3)
	srw	r10, r10, r6		// position live CR2 from cr register as required
	andc	r9, r9, r7		// remove old backup CR2
	and	r10, r10, r7		// mask live CR2
	or	r9, r9, r10		// insert CR2 into backup CR2
	b	CallContinue

CallFromAlternateContext:
CallContinue:
	stw	r9,	0(r11)	// update interruption control word 

	// introduce new pc and gr0 contents
	lwz	r6,	SS_SRR1(r1)
	stw	r4,	SS_SRR0(r1)
	stw	r5,	SS_R0(r1)

	// insert updated fe0, fe1, se, and be bits into user msr
	rlwimi	r6, r6, 0, MSR_FE1_BIT, MSR_FE0_BIT
	/* Turn off FPU */
	rlwinm	r6,	r6,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1

	// zero single step and branch step control in user msr
	stw	r6,	SS_SRR1(r1) // update user msr

	/*
	** Restore state for exit
	*/
	lwz	r0,	SS_CR(r1)
	mtcrf	0xFF,	r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r1)
	mtsprg	1,	r0

	lwz	r0,	SS_R2(r1)
	mtsprg	2,	r0

	lwz	r0,	SS_R3(r1)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r1)

	lwz	r4,	SS_R4(r1)
	lwz	r5,	SS_R5(r1)
	lwz	r6,	SS_R6(r1)
	lwz	r7,	SS_R7(r1)
	lwz	r8,	SS_R8(r1)
	lwz	r9,	SS_R9(r1)
	lwz	r10,	SS_R10(r1)
	lwz	r11,	SS_R11(r1)
	lwz	r12,	SS_R12(r1)

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r3,	0,	ha16(EXT(exception_exit))
	addi	r3,	r3,	lo16(EXT(exception_exit))
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3

	lwz	r2,	SS_SRR0(r1)	/* For trampoline */
	lwz	r3,	SS_SRR1(r1)	/* For trampoline */
	lwz	r1,	PCB_SR0(r1)	/* load the last register... */

	rfi



/*QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ
 * void ExitPseudoKernel ( ExitPseudoKernelDescriptorPtr exitDescriptor )
 *
 * This op provides a means of exiting from the BlueBox PseudoKernel to a
 * user context while changing the BlueBox interruption state atomically.
 * It also allows all of the user state PPC registers to be loaded.
 *
 * This op is invoked as follows:
 *	lwz r3, ExitPseudoKernelDescriptorPtr
 *	li r0, kCallPseudoKernelNumber	// load this op's firmware call number
 *	sc				// invoke CallPseudoKernel
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
	.align	5
__fcExitPseudoKernel:
	// Needed to save some state so this code matches NuKernel Support
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)

	stw	r0,	SS_R0(r1)

	lwz	r0,	PP_SAVE_SRR0(r2)
	stw	r0,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)

	/* start of actual routine */

	lwz	r9, SS_SRR1(r1)

	lwz	r3, SS_R3(r1)		// restore r3, it is exitdescptr

	lwz	r8, EPKD_CR(r3)

	lwz	r11, EPKD_INTCONTROLADDR(r3)
	lwz	r4, EPKD_PC(r3)
	lwz	r7, EPKD_NEWSTATE(r3)
	lwz	r10, EPKD_INTSTATEMASK(r3)
	lwz	r5, 0(r11)	// get current interruption control word
	lwz	r0, EPKD_SYSCONTEXTSTATE(r3)
	andc	r12, r5, r10		// remove current state
	cmplw	r7, r0			// test for exit to system context
	or	r12, r12, r7		// insert new state
	lwz	r0, EPKD_MSRUPDATE(r3)
	beq	ExitToSystemContext

ExitToAlternateContext:
	lwz	r5, EPKD_INTPENDINGMASK(r3)
	lwz	r6, EPKD_INTPENDINGPC(r3)
	and.	r7, r12, r5		// test for pending 'rupt in backup cr2
	beq	ExitUpdateRuptControlWord	//   and enter alternate context if none pending
	mr	r4, r6			// otherwise, introduce entry abort pc
	b	ExitNoUpdateRuptControlWord	//   and prepare to reenter pseudokernel

ExitToSystemContext:
	lwz	r5, EPKD_INTCR2SHIFT(r3)
	lwz	r6, EPKD_INTCR2MASK(r3)		
	slw	r7, r12, r5		// position backup cr2
	and	r7, r7, r6		//   and mask it
	or	r8, r8, r7		//   then or it into the live cr2
											// ...fall through into system context

ExitUpdateRuptControlWord:
	// insert updated fe0, fe1, se, and be bits into user msr
	rlwimi	r9, r0, 0, MSR_FE0_BIT, MSR_FE1_BIT
	/* Turn off FPU */
	rlwinm	r9,	r9,	0,	MSR_FP_BIT+1,	MSR_FP_BIT-1
	stw	r12, 0(r11)		// update interruption control word 
ExitNoUpdateRuptControlWord:
	lwz	r5, EPKD_GPR0(r3)
	lwz	r6, EPKD_SP(r3)
	lwz	r7, EPKD_GPR3(r3)
											// load caller's new register contents

	stw	r4, SS_SRR0(r1)
	stw	r5, SS_R0(r1)
	stw	r6, SS_R1(r1)
	stw	r7, SS_R3(r1)
	stw	r8, SS_CR(r1)
	stw	r9, SS_SRR1(r1)


	lwz	r0,	SS_CR(r1)
	mtcrf	0xFF,r0			/* update cr, it is live */

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r1)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r1)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r1)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r1)

	lwz	r4,	SS_R4(r1)
	lwz	r5,	SS_R5(r1)
	lwz	r6,	SS_R6(r1)
	lwz	r7,	SS_R7(r1)
	lwz	r8,	SS_R8(r1)
	lwz	r9,	SS_R9(r1)
	lwz	r10,	SS_R10(r1)
	lwz	r11,	SS_R11(r1)
	lwz	r12,	SS_R12(r1)

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r3,	0,	ha16(EXT(exception_exit))
	addi	r3,	r3,	lo16(EXT(exception_exit))
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3

	lwz	r2,	SS_SRR0(r1)	/* For trampoline */
	lwz	r3,	SS_SRR1(r1)	/* load the last register... */
	lwz	r1,	PCB_SR0(r1)	/* For trampoline... */

	rfi


/*
 * void cthread_set_self(cproc_t p)
 *
 * set's thread state "user_value"
 *
 * This op is invoked as follows:
 *	li r0, CthreadSetSelfNumber	// load the fast-trap number
 *	sc				// invoke fast-trap
 *	blr
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
 */
 	.text
	.align	5
__fcCthreadSetSelfNumber:
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)

	mfsprg	r3,	3
	stw	r3,	CTHREAD_SELF(r1)

	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r3,	0,	ha16(EXT(exception_exit))
	addi	r3,	r3,	lo16(EXT(exception_exit))
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3

	lwz	r3,	PP_SAVE_SRR1(r2)	/* load the last register... */
	lwz	r2,	PP_SAVE_SRR0(r2)	/* For trampoline */
	lwz	r1,	PCB_SR0(r1)		/* For trampoline... */

	rfi


/*
 * ur_cthread_t ur_cthread_self(void)
 *
 * return thread state "user_value"
 *
 * This op is invoked as follows:
 *	li r0, UrCthreadSelfNumber	// load the fast-trap number
 *	sc				// invoke fast-trap
 *	blr
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 *
 */
 	.text
	.align	5
__fcUrCthreadSelfNumber:
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)

	lwz	r3,	CTHREAD_SELF(r1)
	mtsprg	3,	r3


	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r3,	0,	ha16(EXT(exception_exit))
	addi	r3,	r3,	lo16(EXT(exception_exit))
	lwz	r3,	0(r3)
	mtsrr0	r3
	li	r3,	MSR_VM_OFF
	mtsrr1	r3

	lwz	r3,	PP_SAVE_SRR1(r2)	/* load the last register... */
	lwz	r2,	PP_SAVE_SRR0(r2)	/* For trampoline */
	lwz	r1,	PCB_SR0(r1)		/* For trampoline... */

	rfi



/*
 * shandler(type)
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc_info structure
 *              original cr            saved in per_proc_info structure
 *              exception type         saved in per_proc_info structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...)
 */

/*
 * If pcb.ksp == 0 then the kernel stack is already busy,
 *                 this is an error - jump to the debugger entry
 * 
 * otherwise       we save a (partial - TODO ) ppc_saved_state
 *                 in the pcb, and, depending upon the type of
 *                 syscall, look it up in the kernel table
 *		   or pass it to the server.
 *
 * on return, we do the reverse, the state is popped from the pcb
 * and pcb.ksp is set to the top of stack.
 */

ENTRY(shandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

#if DEBUG
		/* Make sure we're not on the interrupt stack */
	addis	r1,	0,	ha16(EXT(istackptr))
	addi	r1,	r1,	lo16(EXT(istackptr))
	lwz	r1,	0(r1)
	cmpwi	CR0,	r1,	0

	/* If we are on the interrupt stack, treat as an interrupt,
	 * the interrupt handler will panic with useful info.
	 */
	beq	EXT(ihandler)
	
#endif /* DEBUG */

	/*
	**	check for special BlueBox calls
	*/
	addis	r1,	0,	ha16(EXT(nsysent))
	addi	r1,	r1,	lo16(EXT(nsysent))
	lwz	r3,	0(r1)
	cmpw	CR0,	r0,	r3
	ble	L_shandler_syscall

	cmpwi	CR0,	r0,	0x7FFC
	beq-	__fcCallPseudoKernel

	cmpwi	CR0,	r0,	0x7FFE
	beq-	__fcExitPseudoKernel

	cmpwi	CR0,	r0,	0x7FF1	;CthreadSetSelfNumber
	beq-	__fcCthreadSetSelfNumber

	cmpwi	CR0,	r0,	0x7FF2	;UrCthreadSelfNumber
	beq-	__fcUrCthreadSelfNumber

L_shandler_syscall:
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r1,	THREAD_PCB(r3)
#if DEBUG
		/* Check that we're not on kernel stack already */
	lwz	r1,	PCB_KSP(r1)

	/* If we are on a kernel stack, treat as a interrupt
	 * the interrupt handler will panic with useful info.
	 */
	cmpwi	CR1,	r1,	0
	/* tell the handler that we performed a syscall from this loc */
	li	r3,	EXC_SYSTEM_CALL
	beq	CR1,	EXT(ihandler)

/* Reload active thread into r3 and PCB into r1 as before */
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r1,	THREAD_PCB(r3)
#endif /* DEBUG */

	/* Once we reach here, r1 contains the pcb
         * where we can store a partial ppc_saved_state structure,
	 * and r3 contains the active thread structure (used later)
	 */

	/* TODO NMGS - could only save callee saved regs for
	 * many(all?) Mach syscalls, not for unix since might be fork() etc
	 */
	stw	r0,	SS_R0(r1)    /* Save trap number for debugging */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r0,	PP_SAVE_SRR0(r2)  /* Save SRR0 in debug call frame */
	stw	r0,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	oris	r0,	r0,	MSR_SYSCALL_MASK >> 16 /* Mark syscall state */
	stw	r0,	SS_SRR1(r1)
	
	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	/* SAVE ARG REGISTERS? - YES, needed by server system calls */
	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)

	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)

		/*
		 * Callee saved state, need to save in case we
		 * are executing a 'fork' unix system call or similar
		 */

	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)

	/* We use these registers in the code below, save them */
	
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)
	
	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	mflr	r0
	stw	r0,	SS_LR(r1)

	/* Volatile state, still restoring for sensible corefiles */
	mfctr	r0
	stw	r0,	SS_CTR(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	/* Don't bother with MQ for now */
	
	/* Free the reservation whilst saving SR_COPYIN */

	mfsr	r0,	SR_COPYIN_NAME
	li	r31,	SS_SR_COPYIN
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	r31,	r1
	stw	r0,	SS_SR_COPYIN(r1)
	
	/* We saved state in the pcb, recover the stack pointer */
	lwz	r31,	PCB_KSP(r1)	/* Get ksp */

	li	r0,	0
	stw	r0,	PCB_KSP(r1)	/* Mark stack as busy with 0 val */
	
	/* Phew!
	 *
	 * To summarise, when we reach here, we have filled out
	 * a (partial) ppc_saved_state structure in the pcb, moved
	 * to kernel stack, and the stack is marked as busy.
	 * r1 holds a pointer to this state, and r3 holds a
	 * pointer to the active thread. r31 holds kernel stack ptr.
	 * We now move onto the kernel stack and generate a small
	 * stack frame to follow the calling
	 * conventions. We set up the backpointers to the calling
	 * routine allowing us to backtrace.
	 */

	mr	r30,	r1		/* Save pointer to state in r30 */
	mr	r1,	r31		/* move to kernel stack */
	mfsprg	r0,	1		/* get old stack pointer */
	stw	r0,	FM_BACKPTR(r1)	/* store as backpointer */
		
#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */
	stw	r29,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */
	//stwu	r1,	-FM_SIZE(r1)	/* point back to previous frame */
	stw	r1,	-FM_SIZE(r1)	/* point back to previous frame */
	subi	r1,	r1,	FM_SIZE
#endif /* DEBUG */

	//stwu	r1,	-(FM_SIZE+ARG_SIZE)(r1)
	//stwu	r1,	-(FM_SIZE+FM_REDZONE)(r1)
	stw	r1,	-(FM_SIZE+FM_REDZONE)(r1)
	subi	r1,	r1,	(FM_SIZE+FM_REDZONE)

	/* switch on interrupts now kernel stack is busy and valid */
	mfmsr	r0
	rlwimi	r0,	r0,	0,	MSR_EE_BIT,	MSR_EE_BIT
	mtmsr	r0


	/* we should still have r1=ksp, r3(ie ARG0)=current-thread,
	 * r30 = pointer to saved state (in pcb)
	 */
	
	/* Work out what kind of syscall we have to deal with. 
	 */

#if MACH_ASSERT
	/* Call a function that can print out our syscall info */
	mr	r4,	r30
	bl	syscall_trace
	/* restore those volatile argument registers */
	lwz	r4,	SS_R4(r30)
	lwz	r5,	SS_R5(r30)
	lwz	r6,	SS_R6(r30)
	lwz	r7,	SS_R7(r30)
	lwz	r8,	SS_R8(r30)
	lwz	r9,	SS_R9(r30)
	lwz	r10,	SS_R10(r30)
	
#endif /* MACH_ASSERT */
	mr	r3,	r30		/* put pcb in ARG0 */
	lwz	r0,	SS_R0(r30)
		
	cmpwi	CR0,	r0,	0
	blt-	.L_mach_kernel_syscall

	/* +ve syscall - go to server */

	b	EXT(unix_syscall)

.L_mach_kernel_syscall:

	b	EXT(mach_syscall)



.L_thread_syscall_return:

	li	r3, MSR_SUPERVISOR_INT_OFF
	mtmsr	r3
	
	/* thread_exception_return returns to here, almost all
	 * registers intact. It expects a full context restore
	 * of what it hasn't restored itself (ie. what we use).
	 *
	 * In particular for us,
	 * we still have     r31 points to the current thread,
	 *                   r30 points to the current pcb
	 */

	mr	r3,	r30
	mr	r1,	r31
	/* r0-2 will be restored last, use as temp for now */


	/* Callee saved state was saved and restored by the functions
	 * that we have called, assuming that we performed a standard
	 * function calling sequence. We only restore those that we
	 * are currently using.
	 *
	 * thread_exception_return arrives here, however, and it
	 * expects the full state to be restored
	 */
#if DEBUG
	/* the following callee-saved state should already be restored */
	
	lwz	r30,	SS_R14(r3)@ twne	r30,	r14
	lwz	r30,	SS_R15(r3)@ twne	r30,	r15
	lwz	r30,	SS_R16(r3)@ twne	r30,	r16
	lwz	r30,	SS_R17(r3)@ twne	r30,	r17
	lwz	r30,	SS_R18(r3)@ twne	r30,	r18
	lwz	r30,	SS_R19(r3)@ twne	r30,	r19
	lwz	r30,	SS_R20(r3)@ twne	r30,	r20
	lwz	r30,	SS_R21(r3)@ twne	r30,	r21
	lwz	r30,	SS_R22(r3)@ twne	r30,	r22
	lwz	r30,	SS_R23(r3)@ twne	r30,	r23
	lwz	r30,	SS_R24(r3)@ twne	r30,	r24
	lwz	r30,	SS_R25(r3)@ twne	r30,	r25
	lwz	r30,	SS_R26(r3)@ twne	r30,	r26
	lwz	r30,	SS_R27(r3)@ twne	r30,	r27
	lwz	r30,	SS_R28(r3)@ twne	r30,	r28
	lwz	r30,	SS_R29(r3)@ twne	r30,	r29
#endif /* DEBUG */
		
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_LR(r3)
	mtlr	r0

	/* Volatile state, still restoring for sensible corefiles */
	lwz	r0,	SS_CTR(r3)
	mtctr	r0

	lwz	r0,	SS_XER(r3)
	mtxer	r0

	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN_NAME,	r0
	isync

	/* mark kernel stack as free before restoring r30, r31 */

	/* we no longer need r2 pointer to per_proc_info at this point */
	
	/* There may be a critical region here for traps(interrupts?)
	 * once the stack is marked as free, PCB_SR0 may be trampled on
	 * so interrupts must be off.
	 */
	/* Clear reservation at the same time */
	lwz	r2,	THREAD_KERNEL_STACK(r1)
	addi	r0,	r2,	KSTK_SIZE-KS_SIZE-FM_SIZE
	li	r2,	PCB_KSP
/* we have to use an indirect store to clear reservation */
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	r2,	r3		/* clear reservation */
	stw	r0,	PCB_KSP(r3)		/* mark stack as free */

	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	bne+	.L_syscall_returns_to_user

	/* TODO NMGS - is this code in common with interrupts and traps?*/
	/* the syscall is returning to something in
	 * priviliged mode, can just rfi without modifying
	 * space registers
	 */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	/* critical region for traps(interrupt?) since r1 no longer points
	 * to bottom of stack. Could be fixed. But interrupts are off (?).
	 */

	lwz	r2,	SS_R2(r3)
	/* r3 restored last */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */

.L_syscall_returns_to_user:	

	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	/* Prepare to rfi to the exception exit routine, which is
	 * in physical address space */
	addis	r1,	0,	ha16(EXT(exception_exit))
	addi	r1,	r1,	lo16(EXT(exception_exit))
	lwz	r1,	0(r1)
	mtsrr0	r1
	li	r1,	MSR_VM_OFF
	mtsrr1	r1

	lwz	r1,	PCB_SR0(r3)	/* For trampoline */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */

	rfi



/*
 * thread_exception_return()
 *
 * Return to user mode directly from within a system call.
 */

ENTRY(thread_exception_return, TAG_NO_FRAME_USED)

.L_thread_exc_ret_check_ast:	

	/* Disable interrupts */
	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3

	/* Check to see if there's an outstanding AST */
	/* We don't bother establishing a call frame even though CHECK_AST
	   can invoke ast_taken(), because it can just borrow our caller's
	   frame, given that we're not going to return.  */
		
	bl	EXT(check_for_ast)
	
.L_exc_ret_no_ast:
	/* arriving here, interrupts should be disabled */

	mfsprg	r2,	0	/* HACK - need to get around r2 problem */
	/* Get the active thread's PCB pointer to restore regs
	 */
	
	lwz	r31,	PP_CPU_DATA(r2)
	lwz	r31,	CPU_ACTIVE_THREAD(r31)
	lwz	r30,	THREAD_PCB(r31)

	/* If the MSR_SYSCALL_MASK isn't set, then we came from a trap,
	 * so warp into the return_from_trap (thread_return) routine,
	 * which takes PCB pointer in ARG0, not in r30!
	 */
	lwz	r0,	SS_SRR1(r30)
	mr	ARG0,	r30	/* Copy pcb pointer into ARG0 in case */

	/* test top half of msr */
	srwi	r0,	r0,	16
	cmpwi	CR0,	r0,	MSR_SYSCALL_MASK >> 16
	bne-	CR0,	thread_return

	/* Otherwise, go to thread_syscall return, which requires
	 * r31 holding current thread, r30 holding current pcb
	 */

	/*
	 * restore saved state here
	 * except for r0-2, r3, r29, r30 and r31 used
	 * by thread_syscall_return,
	 */
	lwz	r4,	SS_R4(r30)
	lwz	r5,	SS_R5(r30)
	lwz	r6,	SS_R6(r30)
	lwz	r7,	SS_R7(r30)
	lwz	r8,	SS_R8(r30)
	lwz	r9,	SS_R9(r30)
	lwz	r10,	SS_R10(r30)
	lwz	r11,	SS_R11(r30)
	lwz	r12,	SS_R12(r30)
	lwz	r13,	SS_R13(r30)
	lwz	r14,	SS_R14(r30)
	lwz	r15,	SS_R15(r30)
	lwz	r16,	SS_R16(r30)
	lwz	r17,	SS_R17(r30)
	lwz	r18,	SS_R18(r30)
	lwz	r19,	SS_R19(r30)
	lwz	r20,	SS_R20(r30)
	lwz	r21,	SS_R21(r30)
	lwz	r22,	SS_R22(r30)
	lwz	r23,	SS_R23(r30)
	lwz	r24,	SS_R24(r30)
	lwz	r25,	SS_R25(r30)
	lwz	r26,	SS_R26(r30)
	lwz	r27,	SS_R27(r30)
	lwz	r28,	SS_R28(r30)
	lwz	r29,	SS_R29(r30)

	b	.L_thread_syscall_return



/*
 * thread_bootstrap_return()
 *
 */
ENTRY(thread_bootstrap_return, TAG_NO_FRAME_USED)

	/* Disable interrupts */
	li	r3,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r3

	/* Check for any outstanding ASTs and deal with them */

	bl	EXT(check_for_ast)
	
	/*
	** Restore from AST check
	*/
	mfsprg	r2,	0 	/* HACK - need to get around r2 problem */
	lwz	r3,	PP_CPU_DATA(r2)
	lwz	r3,	CPU_ACTIVE_THREAD(r3)
	lwz	r3,	THREAD_PCB(r3)

	/* Ok, we're all set, jump to thread_return as if we
	 * were just coming back from a trap (ie. r3 set up to point to pcb)
	 */	

	b	thread_return



/*
 * ihandler(type)
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc structure
 *              original cr            saved in per_proc structure
 *              exception type (r3)    saved in per_proc structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...) also in per_proc
 *
 * gdbhandler is a close derivative, bugfixes to one may apply to the other!
 */

/* Build a saved state structure on the interrupt stack and call
 * the routine interrupt()
 */

ENTRY(ihandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */

	/*
	 * get the value of istackptr, if it's zero then we're already on the
	 * interrupt stack, otherwise it points to a saved_state structure
	 * at the top of the interrupt stack.
	 */

	addis	r1,	0,	ha16(EXT(istackptr))
	addi	r1,	r1,	lo16(EXT(istackptr))	/* TODO assumes 1 CPU */
	lwz	r1,	0(r1)
	cmpwi	CR0,	r1,	0
	bne	CR0,	.L_istackfree

	/* We're already on the interrupt stack, get back the old
	 * stack pointer and make room for a frame
	 */

	mfsprg	r1,	1	/* recover old stack pointer */

	/* Move below the redzone where the interrupted thread may have
	 * been saving its state and make room for our saved state structure
	 */
	subi	r1,	r1,	FM_REDZONE+SS_SIZE

	
	
.L_istackfree:

	/* Once we reach here, r1 contains the adjusted stack pointer
         * where we can store a ppc_saved_state structure.
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)

	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r5,	PP_SAVE_SRR0(r2)	/* r5 holds srr0 used below */
	stw	r5,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	mfctr	r0
	stw	r0,	SS_CTR(r1)

	/* Don't bother with MQ for now */

	/* Free the reservation whilst saving SR_COPYIN */

	mfsr	r0,	SR_COPYIN_NAME
	li	r4,	SS_SR_COPYIN
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	r4,	r1
	stw	r0,	SS_SR_COPYIN(r1)
	
	/* Mark that we're occupying the interrupt stack for sure now */

	addis	r4,	0,	ha16(EXT(istackptr))
	addi	r4,	r4,	lo16(EXT(istackptr))
	li	r0,	0
	stw	r0,	0(r4)		/* TODO assumes 1 CPU */

	/*
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure on the interrupt stack, and
	 * the stack is marked as busy. We now generate a small
	 * stack frame with backpointers to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 */
	
	mr	r4,	r1		/* Preserve saved_state in ARG1 */
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 */

	stw	r5,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */

	//stwu	r1,	-FM_SIZE(r1)	/* Mak new frame for C routine */
	stw	r1,	-FM_SIZE(r1)	/* Mak new frame for C routine */
	subi	r1,	r1,	FM_SIZE
#endif /* DEBUG */

	/* r3 still holds the reason for the interrupt */
	/* and r4 holds a pointer to the saved state */
	mfdsisr	ARG2
	mfdar	ARG3
	
	bl	EXT(interrupt)

	/* interrupt() returns a pointer to the saved state in r3
	 *
	 * Ok, back from C. Disable interrupts while we restore things
	 */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0

	/* Reload the saved state */

	/* r0-2 will be restored last, use as temp for now */

	/* We don't restore r3-5, these are restored differently too.
	 * see trampoline code TODO NMGS evaluate need for this
	 */

	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_XER(r3)
	mtxer	r0
	lwz	r0,	SS_LR(r3)
	mtlr	r0
	lwz	r0,	SS_CTR(r3)
	mtctr	r0
	lwz	r0,	SS_SR_COPYIN(r3)
	isync
	mtsr	SR_COPYIN_NAME,	r0
	isync

	/* TODO NMGS don't restore mq since we're not 601-specific enough */

	/* Is this the first interrupt on the stack? */

	addis	r4,	0,	ha16(EXT(intstack_top_ss))
	addi	r4,	r4,	lo16(EXT(intstack_top_ss))  /* TODO assumes 1 CPU */
	lwz	r4,	0(r4)

	cmp	CR0,0,	r4,	r3
	bne	CR0,	.L_notthelast_interrupt

	/* We're the last frame on the stack. Indicate that
	 * we've freed up the stack by putting our save state ptr in
	 * istackptr. Clear reservation at same time.
	 */

	addis	r4,	0,	ha16(EXT(istackptr))
	addi	r4,	r4,	lo16(EXT(istackptr))	/* TODO assumes 1 CPU */
/* we have to use an indirect store to clear reservation */
	sync				/* bug fix for 3.2 processors */
	stwcx.	r3,	0,	r4		/* clear reservation */
	stw	r3,	0(r4)

	/* We're the last frame on the stack.
	 * Check for ASTs if one of the below is true:	
	 *    returning to user mode
	 *    returning to a kloaded server
	 */

	lwz	r4,	SS_SRR1(r3)
	andi.	r4,	r4,	MASK(MSR_PR)
	bne+	.L_check_int_ast	/* returning to user level, check */
	b	.L_no_int_ast	/* in kernel, no check */

.L_check_int_ast:

	/*
	 * There is a pending AST. Massage things to make it look like
	 * we took a trap and jump into the trap handler.  To do this
	 * we essentially pretend to return from the interrupt but
	 * at the last minute jump into the trap handler with an AST
	 * trap instead of performing an rfi.
	 */

	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_CR(r3)	/* store state in per_proc struct */
	stw	r0,	PP_SAVE_CR(r2)
	lwz	r0,	SS_SRR0(r3)
	stw	r0,	PP_SAVE_SRR0(r2)
	lwz	r0,	SS_SRR1(r3)
	stw	r0,	PP_SAVE_SRR1(r2)
	li	r0,	EXC_AST
	stw	r0,	PP_SAVE_EXCEPTION_TYPE(r2)
	
	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* r2 remains a constant - virt addr of per_proc_info */
	li	r3,	EXC_AST	/* TODO r3 isn't used by thandler -optimise? */
	b	EXT(thandler)		/* hyperspace into AST trap */

.L_no_int_ast:	

	/* We're committed to performing the rfi now.
	 * If returning to the user space, we should restore the user's
	 * segment registers. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */
	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_interrupt_returns_to_kspace

	/* TODO NMGS would it be better to store SR0 in saved_state
	 * rather than perform this expensive lookup?
	 */
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)
	lwz	r1,	PCB_SR0(r1)	/* For trampoline */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* Prepare to rfi to the exception exit routine */
	addis	r2,	0,	ha16(EXT(exception_exit))
	addi	r2,	r2,	lo16(EXT(exception_exit))
	lwz	r2,	0(r2)
	mtsrr0	r2
	li	r2,	MSR_VM_OFF
	mtsrr1	r2

	/* r1 already loaded above */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */
	
	rfi
.L_interrupt_returns_to_kspace:	
.L_notthelast_interrupt:
	/* If we're not the last interrupt on the interrupt stack
	 * life is easier, we don't need to switch back into the
	 * user's segment. we can simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	lwz	r2,	SS_R2(r3)	/* r2 is a constant - why restore?*/
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
	
/*
 * gdbhandler(type)
 *
 * Entry:	VM switched ON
 *		Interrupts  OFF
 *              original r1-3 saved in sprg1-3
 *              original srr0 and srr1 saved in per_proc structure
 *              original cr            saved in per_proc structure
 *              exception type (r3)    saved in per_proc structure
 *              r1 = scratch
 *              r2 = virt addr of per_proc_info
 *		r3 = exception type (one of EXC_...) also in per_proc
 *
 *  Closely based on ihandler - bugfixes to one may apply to the other!
 */

/* build a saved state structure on the debugger stack and call
 * the routine enterDebugger()
 */

ENTRY(gdbhandler, TAG_NO_FRAME_USED)	/* What tag should this have?! */
#if DEBUG
	/*
	 * get the value of gdbstackptr, if it's zero then we're already on the
	 * debugger stack, otherwise it points to a saved_state structure
	 * at the top of the debugger stack.
	 */

	addis	r1,	0,	ha16(EXT(gdbstackptr))
	addi	r1,	r1,	lo16(EXT(gdbstackptr))	/* TODO assumes 1 CPU */
	lwz	r1,	0(r1)
	cmpwi	CR0,	r1,	0
	bne	CR0,	.L_gdbstackfree

	/* We're already on the debugger stack, get back the old
	 * stack pointer and make room for a frame
	 */

	mfsprg	r1,	1	/* recover old stack pointer */

	/* Move below the redzone where the interrupted thread may have
	 * been saving its state and make room for our saved state structure
	 */
	subi	r1,	r1,	FM_REDZONE+SS_SIZE

	/* TODO NMGS - how about checking for stack overflow, huh?! */

.L_gdbstackfree:

	/* Once we reach here, r1 contains the adjusted stack pointer
         * where we can store a ppc_saved_state structure.
	 */

	stw	r0,	SS_R0(r1)

	mfsprg	r0,	1
	stw	r0,	SS_R1(r1)

	mfsprg	r0,	2
	stw	r0,	SS_R2(r1)

	mfsprg	r0,	3
	stw	r0,	SS_R3(r1)

	stw	r4,	SS_R4(r1)
	stw	r5,	SS_R5(r1)
	stw	r6,	SS_R6(r1)
	stw	r7,	SS_R7(r1)
	stw	r8,	SS_R8(r1)
	stw	r9,	SS_R9(r1)
	stw	r10,	SS_R10(r1)
	stw	r11,	SS_R11(r1)
	stw	r12,	SS_R12(r1)
	stw	r13,	SS_R13(r1)
	stw	r14,	SS_R14(r1)
	stw	r15,	SS_R15(r1)
	stw	r16,	SS_R16(r1)
	stw	r17,	SS_R17(r1)
	stw	r18,	SS_R18(r1)
	stw	r19,	SS_R19(r1)
	stw	r20,	SS_R20(r1)
	stw	r21,	SS_R21(r1)
	stw	r22,	SS_R22(r1)
	stw	r23,	SS_R23(r1)
	stw	r24,	SS_R24(r1)
	stw	r25,	SS_R25(r1)
	stw	r26,	SS_R26(r1)
	stw	r27,	SS_R27(r1)
	stw	r28,	SS_R28(r1)
	stw	r29,	SS_R29(r1)
	stw	r30,	SS_R30(r1)
	stw	r31,	SS_R31(r1)

	/* Save more state - cr,xer,lr,ctr,srr0,srr1,mq
	 * some of this comes back out from the per-processor structure
	 */

	lwz	r0,	PP_SAVE_CR(r2)
	stw	r0,	SS_CR(r1)
	
	lwz	r5,	PP_SAVE_SRR0(r2)	/* r5 holds srr0 used below */
	stw	r5,	SS_SRR0(r1)

	lwz	r0,	PP_SAVE_SRR1(r2)
	stw	r0,	SS_SRR1(r1)

	mfxer	r0
	stw	r0,	SS_XER(r1)

	mflr	r0
	stw	r0,	SS_LR(r1)

	mfctr	r0
	stw	r0,	SS_CTR(r1)

	/* Don't bother with MQ for now */

	/* Free the reservation whilst saving SR_COPYIN */

	mfsr	r0,	SR_COPYIN_NAME
	li	r4,	SS_SR_COPYIN
	sync				/* bug fix for 3.2 processors */
	stwcx.	r0,	r4,	r1
	stw	r0,	SS_SR_COPYIN(r1)
	
	/* Mark that we're occupying the gdb stack for sure now */

	addis	r4,	0,	ha16(EXT(gdbstackptr))
	addi	r4,	r4,	lo16(EXT(gdbstackptr))
	li	r0,	0
	stw	r0,	0(r4)		/* TODO assumes 1 CPU */

	/*
	 * To summarise, when we reach here, we have filled out
	 * a ppc_saved_state structure on the gdb stack, and
	 * the stack is marked as busy. We now generate a small
	 * stack frame with backpointers to follow the calling
	 * conventions. We set up the backpointers to the trapped
	 * routine allowing us to backtrace.
	 *
	 * This probably isn't needed in gdbhandler, but I've left
	 * it in place
	 */
	
	mr	r4,	r1		/* Preserve saved_state in ARG1 */
	subi	r1,	r1,	FM_SIZE
	mfsprg	r0,	1
	stw	r0,	FM_BACKPTR(r1)	/* point back to previous stackptr */

#if	DEBUG
	/* If debugging, we need two frames, the first being a dummy
	 * which links back to the trapped routine. The second is
	 * that which the C routine below will need
	 * TODO NMGS debugging call frame not correct yet
	 */
	stw	r5,	FM_LR_SAVE(r1)	/* save old instr ptr as LR value */
	//stwu	r1,	-FM_SIZE(r1)	/* point back to previous frame */
	stw	r1,	-FM_SIZE(r1)	/* point back to previous frame */
	subi	r1,	r1,	FM_SIZE
#endif /* DEBUG */

	/* r3 still holds the reason for the trap */
	/* and r4 holds a pointer to the saved state */

	mfdsisr	ARG2

	bl	EXT(enterDebugger)

	/* enterDebugger() returns a pointer to the saved state in r3
	 *
	 * Ok, back from C. Disable interrupts while we restore things
	 */

	li	r0,	MSR_SUPERVISOR_INT_OFF
	mtmsr	r0

	/* Reload the saved state */

	/* r0-2 will be restored last, use as temp for now */

	/* We do not restore r3-5, these are restored differently too.
	 * see trampoline code TODO NMGS evaluate need for this
	 */

	lwz	r6,	SS_R6(r3)
	lwz	r7,	SS_R7(r3)
	lwz	r8,	SS_R8(r3)
	lwz	r9,	SS_R9(r3)
	lwz	r10,	SS_R10(r3)
	lwz	r11,	SS_R11(r3)
	lwz	r12,	SS_R12(r3)
	lwz	r13,	SS_R13(r3)
	lwz	r14,	SS_R14(r3)
	lwz	r15,	SS_R15(r3)
	lwz	r16,	SS_R16(r3)
	lwz	r17,	SS_R17(r3)
	lwz	r18,	SS_R18(r3)
	lwz	r19,	SS_R19(r3)
	lwz	r20,	SS_R20(r3)
	lwz	r21,	SS_R21(r3)
	lwz	r22,	SS_R22(r3)
	lwz	r23,	SS_R23(r3)
	lwz	r24,	SS_R24(r3)
	lwz	r25,	SS_R25(r3)
	lwz	r26,	SS_R26(r3)
	lwz	r27,	SS_R27(r3)
	lwz	r28,	SS_R28(r3)
	lwz	r29,	SS_R29(r3)
	lwz	r30,	SS_R30(r3)
	lwz	r31,	SS_R31(r3)

	lwz	r0,	SS_XER(r3)
	lwz	r5,	SS_LR(r3)
	mtxer	r0
	mtlr	r5
	lwz	r0,	SS_CTR(r3)
	lwz	r5,	SS_SR_COPYIN(r3)
	mtctr	r0
	isync
	mtsr	SR_COPYIN_NAME,	r5
	isync

	/* TODO NMGS don't restore mq since we're not 601-specific enough */

	/* Is this the first frame on the stack? */

	addis	r4,	0,	ha16(EXT(gdbstack_top_ss))
	addi	r4,	r4,	lo16(EXT(gdbstack_top_ss))  /* TODO assumes 1 CPU */
	lwz	r4,	0(r4)

	cmp	CR0, 0,	r4,	r3
	bne	CR0,	.L_notthelast_gdbframe

	/* We're the last frame on the stack. Indicate that
	 * we've freed up the stack by putting our save state ptr in
	 * istackptr. Clear reservation at same time.
	 */
	addis	r4,	0,	ha16(EXT(gdbstackptr))
	addi	r4,	r4,	lo16(EXT(gdbstackptr))	/* TODO assumes 1 CPU */
/* we have to use an indirect store to clear reservation */
	sync				/* bug fix for 3.2 processors */
	stwcx.	r3,	0,	r4		/* clear reservation */
	stw	r3,	0(r4)

	/* We may be returning to something in the kernel space.
	 * If we are, we can skip the trampoline and just rfi,
	 * since we don't want to restore the user's space regs
	 */

	lwz	r0,	SS_SRR1(r3)
	andi.	r0,	r0,	MASK(MSR_PR)
	beq-	.L_gdb_ret_to_kspace

	/* If jumping into user space, we should restore the user's
	 * segment register 0. We jump via a trampoline in physical mode
	 * TODO NMGS this trampoline code probably isn't needed
	 */

	/* TODO NMGS would it be better to store SR0 in saved_state
	 * rather than perform this expensive lookup?
	 */
	lwz	r1,	PP_CPU_DATA(r2)
	lwz	r1,	CPU_ACTIVE_THREAD(r1)
	lwz	r1,	THREAD_PCB(r1)
	lwz	r1,	PCB_SR0(r1)	/* For trampoline */

#if DEBUG
	/* Assert that PCB_SR0 is not in kernel space */
	rlwinm.	r0,	r1,	0,	8,	27
	bne+	.Lbp_skip
	BREAKPOINT_TRAP
.Lbp_skip:
#endif /* DEBUG */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0

	/* the trampoline code takes r1-r3 from sprg1-3, and uses r1-3
	 * as arguments
	 */
	lwz	r0,	SS_R1(r3)
	mtsprg	1,	r0
	lwz	r0,	SS_R2(r3)
	mtsprg	2,	r0
	lwz	r0,	SS_R3(r3)
	mtsprg	3,	r0

	lwz	r0,	SS_R0(r3)
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)

	/* Prepare to rfi to the exception exit routine */
	addis	r2,	0,	ha16(EXT(exception_exit))
	addi	r2,	r2,	lo16(EXT(exception_exit))
	lwz	r2,	0(r2)
	mtsrr0	r2
	li	r2,	MSR_VM_OFF
	mtsrr1	r2

	/* r1 already loaded above */
	lwz	r2,	SS_SRR0(r3)	/* For trampoline */
	lwz	r3,	SS_SRR1(r3)	/* load the last register... */
	

	rfi
	
.L_gdb_ret_to_kspace:	
.L_notthelast_gdbframe:
	/* If we're not the last frame on the stack
	 * life is easier, we don't need to switch back into the
	 * user's segment. we can simply restore the last registers and rfi
	 */

	lwz	r0,	SS_CR(r3)
	mtcrf	0xFF,r0
	lwz	r0,	SS_SRR0(r3)
	mtsrr0	r0
	lwz	r0,	SS_SRR1(r3)
	mtsrr1	r0

	lwz	r0,	SS_R0(r3)
	lwz	r1,	SS_R1(r3)
	lwz	r2,	SS_R2(r3)	/* r2 is a constant - why restore?*/
	/* r3 restored last */
	lwz	r4,	SS_R4(r3)
	lwz	r5,	SS_R5(r3)
	/* and lastly... */
	lwz	r3,	SS_R3(r3)

	rfi				/* return to calling context */
#endif /* DEBUG */	
