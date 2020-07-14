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
 * genassym.c is used to produce an
 * assembly file which, intermingled with unuseful assembly code,
 * has all the necessary definitions emitted. This assembly file is
 * then postprocessed with sed to extract only these definitions
 * and thus the final assyms.s is created.
 *
 * This convoluted means is necessary since the structure alignment
 * and packing may be different between the host machine and the
 * target so we are forced into using the cross compiler to generate
 * the values, but we cannot run anything on the target machine.
 */
#include <mach/machine/thread_status.h>
#include <stddef.h>
#include <sys/types.h>
#include <kern/thread.h>
#include <kern/host.h>
#include <mach/machine/thread_status.h>
#include <machdep/ppc/frame.h>
#include <machdep/ppc/exception.h>
#include <kern/syscall_sw.h>
#include <machdep/ppc/PseudoKernel.h>

#ifdef notdef_next
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE)0)->MEMBER)

#define DECLARE(SYM,VAL) \
	__asm("#define\t" SYM "\t%0" : : "n" ((u_int)(VAL)))

void main(void)
{
#endif /* notdef_next */

	/* Process Control Block */

	int _PCB_FLOAT_STATE =  offsetof(struct pcb, fs);

	/* Floating point state */

	int _PCB_FS_F0 =	offsetof(struct pcb, fs.fpregs[0]);
	int _PCB_FS_F1 = 	offsetof(struct pcb, fs.fpregs[1]);
	int _PCB_FS_F2 = 	offsetof(struct pcb, fs.fpregs[2]);
	int _PCB_FS_F3 = 	offsetof(struct pcb, fs.fpregs[3]);
	int _PCB_FS_F4 = 	offsetof(struct pcb, fs.fpregs[4]);
	int _PCB_FS_F5 = 	offsetof(struct pcb, fs.fpregs[5]);
	int _PCB_FS_F6 = 	offsetof(struct pcb, fs.fpregs[6]);
	int _PCB_FS_F7 = 	offsetof(struct pcb, fs.fpregs[7]);
	int _PCB_FS_F8 = 	offsetof(struct pcb, fs.fpregs[8]);
	int _PCB_FS_F9 = 	offsetof(struct pcb, fs.fpregs[9]);
	int _PCB_FS_F10 = 	offsetof(struct pcb, fs.fpregs[10]);
	int _PCB_FS_F11 = 	offsetof(struct pcb, fs.fpregs[11]);
	int _PCB_FS_F12 = 	offsetof(struct pcb, fs.fpregs[12]);
	int _PCB_FS_F13 = 	offsetof(struct pcb, fs.fpregs[13]);
	int _PCB_FS_F14 = 	offsetof(struct pcb, fs.fpregs[14]);
	int _PCB_FS_F15 = 	offsetof(struct pcb, fs.fpregs[15]);
	int _PCB_FS_F16 = 	offsetof(struct pcb, fs.fpregs[16]);
	int _PCB_FS_F17 = 	offsetof(struct pcb, fs.fpregs[17]);
	int _PCB_FS_F18 = 	offsetof(struct pcb, fs.fpregs[18]);
	int _PCB_FS_F19 = 	offsetof(struct pcb, fs.fpregs[19]);
	int _PCB_FS_F20 = 	offsetof(struct pcb, fs.fpregs[20]);
	int _PCB_FS_F21 = 	offsetof(struct pcb, fs.fpregs[21]);
	int _PCB_FS_F22 = 	offsetof(struct pcb, fs.fpregs[22]);
	int _PCB_FS_F23 = 	offsetof(struct pcb, fs.fpregs[23]);
	int _PCB_FS_F24 = 	offsetof(struct pcb, fs.fpregs[24]);
	int _PCB_FS_F25 = 	offsetof(struct pcb, fs.fpregs[25]);
	int _PCB_FS_F26 = 	offsetof(struct pcb, fs.fpregs[26]);
	int _PCB_FS_F27 = 	offsetof(struct pcb, fs.fpregs[27]);
	int _PCB_FS_F28 = 	offsetof(struct pcb, fs.fpregs[28]);
	int _PCB_FS_F29 = 	offsetof(struct pcb, fs.fpregs[29]);
	int _PCB_FS_F30 = 	offsetof(struct pcb, fs.fpregs[30]);
	int _PCB_FS_F31 = 	offsetof(struct pcb, fs.fpregs[31]);
	int _PCB_FS_FPSCR = 	offsetof(struct pcb, fs.fpscr_pad);


	int _PCB_SAVED_STATE =offsetof(struct pcb, ss);
	int _PCB_KSP =	offsetof(struct pcb, ksp);
	int _PCB_SR0 =	offsetof(struct pcb, sr0);

	int _PCB_SIZE =	sizeof(struct pcb);

	/* Save State Structure */
	int _SS_R0 =	offsetof(struct ppc_saved_state, r0);
	int _SS_R1 =	offsetof(struct ppc_saved_state, r1);
	int _SS_R2 =	offsetof(struct ppc_saved_state, r2);
	int _SS_R3 =	offsetof(struct ppc_saved_state, r3);
	int _SS_R4 =	offsetof(struct ppc_saved_state, r4);
	int _SS_R5 =	offsetof(struct ppc_saved_state, r5);
	int _SS_R6 =	offsetof(struct ppc_saved_state, r6);
	int _SS_R7 =	offsetof(struct ppc_saved_state, r7);
	int _SS_R8 =	offsetof(struct ppc_saved_state, r8);
	int _SS_R9 =	offsetof(struct ppc_saved_state, r9);
	int _SS_R10 =	offsetof(struct ppc_saved_state, r10);
	int _SS_R11 =	offsetof(struct ppc_saved_state, r11);
	int _SS_R12 =	offsetof(struct ppc_saved_state, r12);
	int _SS_R13 =	offsetof(struct ppc_saved_state, r13);
	int _SS_R14 =	offsetof(struct ppc_saved_state, r14);
	int _SS_R15 =	offsetof(struct ppc_saved_state, r15);
	int _SS_R16 =	offsetof(struct ppc_saved_state, r16);
	int _SS_R17 =	offsetof(struct ppc_saved_state, r17);
	int _SS_R18 =	offsetof(struct ppc_saved_state, r18);
	int _SS_R19 =	offsetof(struct ppc_saved_state, r19);
	int _SS_R20 =	offsetof(struct ppc_saved_state, r20);
	int _SS_R21 =	offsetof(struct ppc_saved_state, r21);
	int _SS_R22 =	offsetof(struct ppc_saved_state, r22);
	int _SS_R23 =	offsetof(struct ppc_saved_state, r23);
	int _SS_R24 =	offsetof(struct ppc_saved_state, r24);
	int _SS_R25 =	offsetof(struct ppc_saved_state, r25);
	int _SS_R26 =	offsetof(struct ppc_saved_state, r26);
	int _SS_R27 =	offsetof(struct ppc_saved_state, r27);
	int _SS_R28 =	offsetof(struct ppc_saved_state, r28);
	int _SS_R29 =	offsetof(struct ppc_saved_state, r29);
	int _SS_R30 =	offsetof(struct ppc_saved_state, r30);
	int _SS_R31 =	offsetof(struct ppc_saved_state, r31);
	int _SS_CR =	offsetof(struct ppc_saved_state, cr);
	int _SS_XER =	offsetof(struct ppc_saved_state, xer);
	int _SS_LR =	offsetof(struct ppc_saved_state, lr);
	int _SS_CTR =	offsetof(struct ppc_saved_state, ctr);
	int _SS_SRR0 =	offsetof(struct ppc_saved_state, srr0);
	int _SS_SRR1 =	offsetof(struct ppc_saved_state, srr1);
	int _SS_MQ =	offsetof(struct ppc_saved_state, mq);
	int _SS_SR_COPYIN =	offsetof(struct ppc_saved_state, sr_copyin);
	int _CTHREAD_SELF =	offsetof(struct pcb, cthread_self);
	int _PCB_FLAGS = offsetof(struct pcb, flags);
	int _SS_SIZE = sizeof(struct ppc_saved_state);

	/* Exception State structure */
	int _PCB_ES_DAR = 	offsetof(struct pcb, es.dar);
	int _PCB_ES_DSISR = 	offsetof(struct pcb, es.dsisr);

	/* Per Proc info structure */
	int _PP_SAVE_CR = offsetof(struct per_proc_info, save_cr);
	int _PP_SAVE_LR = offsetof(struct per_proc_info, save_lr);
	int _PP_SAVE_SRR0 = offsetof(struct per_proc_info, save_srr0);
	int _PP_SAVE_SRR1 = offsetof(struct per_proc_info, save_srr1);
	int _PP_SAVE_DAR = offsetof(struct per_proc_info, save_dar);
	int _PP_SAVE_DSISR = offsetof(struct per_proc_info, save_dsisr);
	int _PP_SAVE_SPRG0 = offsetof(struct per_proc_info, save_sprg0);
	int _PP_SAVE_SPRG1 = offsetof(struct per_proc_info, save_sprg1);
	int _PP_SAVE_SPRG2 = offsetof(struct per_proc_info, save_sprg2);
	int _PP_SAVE_SPRG3 = offsetof(struct per_proc_info, save_sprg3);

	int _PP_SAVE_R0	   = offsetof(struct per_proc_info, save_r0);
	int _PP_SAVE_R1	   = offsetof(struct per_proc_info, save_r1);
	int _PP_SAVE_R2	   = offsetof(struct per_proc_info, save_r2);
	int _PP_SAVE_R3	   = offsetof(struct per_proc_info, save_r3);
	int _PP_SAVE_R4	   = offsetof(struct per_proc_info, save_r4);
	int _PP_SAVE_R5	   = offsetof(struct per_proc_info, save_r5);
	int _PP_SAVE_R6	   = offsetof(struct per_proc_info, save_r6);
	int _PP_SAVE_R7	   = offsetof(struct per_proc_info, save_r7);
	int _PP_SAVE_R8	   = offsetof(struct per_proc_info, save_r8);
	int _PP_SAVE_R9	   = offsetof(struct per_proc_info, save_r9);
	int _PP_SAVE_R10   = offsetof(struct per_proc_info, save_r10);
	int _PP_SAVE_R11   = offsetof(struct per_proc_info, save_r11);
	int _PP_SAVE_R12   = offsetof(struct per_proc_info, save_r12);

	int _PP_SAVE_EXCEPTION_TYPE = 
			offsetof(struct per_proc_info, save_exception_type);
	int _PP_CPU_DATA = offsetof(struct per_proc_info, cpu_data);
	int _PP_PHYS_EXCEPTION_HANDLERS =
		offsetof(struct per_proc_info, phys_exception_handlers);
	int _PP_VIRT_PER_PROC =
		offsetof(struct per_proc_info, virt_per_proc_info);
#ifdef notdef_next
	int _PP_ACTIVE_KLOADED =
		offsetof(struct per_proc_info, active_kloaded);
#endif /* notdef_next */
	int _PP_ACTIVE_STACKS =
		offsetof(struct per_proc_info, active_stacks);
	int _PP_NEED_AST =
		offsetof(struct per_proc_info, need_ast);
	int _PP_FPU_PCB =
		offsetof(struct per_proc_info, fpu_pcb);

	/* Kernel State Structure - special case as we want offset from
	 * bottom of kernel stack, not offset into structure
	 */
#define IKSBASE (u_int)STACK_IKS(0)

	int _KS_PCB = IKSBASE + offsetof(struct ppc_kernel_state, pcb);
	int _KS_R1 =  IKSBASE + offsetof(struct ppc_kernel_state, r1);
	int _KS_R2 =  IKSBASE + offsetof(struct ppc_kernel_state, r2);
	int _KS_R13 = IKSBASE + offsetof(struct ppc_kernel_state, r13);
	int _KS_LR =  IKSBASE + offsetof(struct ppc_kernel_state, lr);
	int _KS_CR =  IKSBASE + offsetof(struct ppc_kernel_state, cr);
	int _KS_SIZE = sizeof(struct ppc_kernel_state);
	int _KSTK_SIZE = KERNEL_STACK_SIZE;

	/* values from kern/thread.h */
	int _THREAD_PCB = offsetof(struct thread, pcb);
	int _THREAD_KERNEL_STACK = offsetof(struct thread, kernel_stack);
	int _THREAD_SWAPFUNC = offsetof(struct thread, swap_func);
	int _THREAD_RECOVER = offsetof(struct thread, recover);
	int _THREAD_TASK = offsetof(struct thread, task);
	int _THREAD_AST = offsetof(struct thread, ast);
#ifdef notdef_next
	DECLARE("THREAD_TOP_ACT",
		offsetof(struct thread_shuttle *, top_act));
	DECLARE("THREAD_CONTINUATION",
		offsetof(struct thread_shuttle *, continuation));

	/* values from kern/thread_act.h */
	DECLARE("ACT_TASK",    offsetof(struct thread_activation *, task));
	DECLARE("ACT_MACT_PCB",offsetof(struct thread_activation *, mact.pcb));
	DECLARE("ACT_AST",     offsetof(struct thread_activation *, ast));
	DECLARE("ACT_VMMAP",   offsetof(struct thread_activation *, map));
	DECLARE("ACT_KLOADED",
		offsetof(struct thread_activation *, kernel_loaded));
	DECLARE("ACT_KLOADING",
		offsetof(struct thread_activation *, kernel_loading));
	DECLARE("ACT_MACH_EXC_PORT",
		offsetof(struct thread_activation *,
		exc_actions[EXC_MACH_SYSCALL].port));
#endif

	/* values from kern/task.h */
#ifdef notdef_next
	DECLARE("TASK_MACH_EXC_PORT",
		offsetof(struct task *, exc_actions[EXC_MACH_SYSCALL].port));
#else /* notdef_next */
	int _TASK_VMMAP = offsetof(struct task, map);
	int _TASK_MACH_EXC_PORT = offsetof(struct task, itk_exception);
#endif /* notdef_next */

	/* values from vm/vm_map.h */
	int _VMMAP_PMAP = offsetof(struct vm_map, pmap);

	/* values from machine/pmap.h */
	int _PMAP_SPACE = offsetof(struct pmap, space);

#ifdef notdef_next
	/* Constants from pmap.h */
	DECLARE("PPC_SID_KERNEL", PPC_SID_KERNEL);
#endif /* notdef_next */

	/* values for accessing mach_trap table */
	int _MACH_TRAP_OFFSET_POW2 = 4;

	int _MACH_TRAP_ARGC =
		offsetof(mach_trap_t, mach_trap_arg_count);
	int _MACH_TRAP_FUNCTION =
		offsetof(mach_trap_t, mach_trap_function);

	int _HOST_SELF = offsetof(struct host, host_self);

	/* values from cpu_data.h */
	int _CPU_ACTIVE_THREAD = offsetof(cpu_data_t, active_thread);
	int _CPU_FLAGS = offsetof(cpu_data_t, flags);

#ifdef notdef_next
	/* Misc values used by assembler */
	int _AST_ALL = 0;
}
#endif /* notdef_next */


	/* Frame Structure */
	int _FM_SIZE	=	sizeof(struct frame_area);

//	int _NARGS	=	(sizeof(struct param_area)/ sizeof(int));

//	int _MAXREGARGS	=	(sizeof(struct reg_param_area)/ sizeof(int));

	int _ARG_SIZE	=	sizeof(struct stack_param_area);

	int _LA_SIZE	=	sizeof(struct linkage_area);
	int _FM_BACKPTR	=	offsetof(struct stack_frame, la.saved_sp);
	int _FM_CR_SAVE	=	offsetof(struct stack_frame, la.saved_cr);
	int _FM_LR_SAVE	=	offsetof(struct stack_frame, la.saved_lr);
	int _FM_TOC_SAVE=	offsetof(struct stack_frame, la.saved_r2);

	int _RPA_SIZE	=	sizeof(struct reg_param_area);
	int _SPA_SIZE	=	sizeof(struct stack_param_area);

	int _FM_ARG0	=	offsetof(struct stack_frame, pa.spa.saved_p8);

	int _FM_REDZONE	=	sizeof (struct redzone);


	/* CallPseudoKernel info */
	int _CPKD_PC			= offsetof(CPKD_t, pc);
	int _CPKD_GPR0			= offsetof(CPKD_t, gpr0);
	int _CPKD_INTCONTROLADDR	= offsetof(CPKD_t, intControlAddr);
	int _CPKD_NEWSTATE		= offsetof(CPKD_t, newState);
	int _CPKD_INTSTATEMASK		= offsetof(CPKD_t, intStateMask);
	int _CPKD_INTCR2MASK		= offsetof(CPKD_t, intCR2Mask);
	int _CPKD_INTCR2SHIFT		= offsetof(CPKD_t, intCR2Shift);
	int _CPKD_SYSCONTEXTSTATE	= offsetof(CPKD_t, sysContextState);


	/* ExitPseudoKernel info */
	int _EPKD_PC			= offsetof(EPKD_t, pc);
	int _EPKD_SP			= offsetof(EPKD_t, sp);
	int _EPKD_GPR0			= offsetof(EPKD_t, gpr0);
	int _EPKD_GPR3			= offsetof(EPKD_t, gpr3);
	int _EPKD_CR			= offsetof(EPKD_t, cr);
	int _EPKD_INTCONTROLADDR	= offsetof(EPKD_t, intControlAddr);
	int _EPKD_NEWSTATE		= offsetof(EPKD_t, newState);
	int _EPKD_INTSTATEMASK		= offsetof(EPKD_t, intStateMask);
	int _EPKD_INTCR2MASK		= offsetof(EPKD_t, intCR2Mask);
	int _EPKD_INTCR2SHIFT		= offsetof(EPKD_t, intCR2Shift);
	int _EPKD_SYSCONTEXTSTATE	= offsetof(EPKD_t, sysContextState);
	int _EPKD_INTPENDINGMASK	= offsetof(EPKD_t, intPendingMask);
	int _EPKD_INTPENDINGPC		= offsetof(EPKD_t, intPendingPC);
	int _EPKD_MSRUPDATE		= offsetof(EPKD_t, msrUpdate);
	
