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
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/* 
 * HISTORY
 * Revision 1.1.1.1  1997/09/30 02:45:12  wsanchez
 * Import of kernel from umeshv/kernel
 *
 *
 * 4-Jan-95 Curtis Galloway (galloway) at NeXT
 *	Created cross-buildable version from old genassym.c.
 *
 * 16-Mar-93  Curtis Galloway (galloway) at NeXT
 *      Remove obsolete 68k timer references.
 *
 * Revision 1.4.1.3  91/03/28  08:43:49  rvb
 * 	Flush THREAD_EXIT & THREAD_EXIT_CODE for X134.
 * 	[91/03/23            rvb]
 * 
 * Revision 1.4.1.2  90/02/09  17:23:20  rvb
 * 	Constants for Mach emulation support.
 * 	[90/02/09            rvb]
 * 
 * Revision 1.4.1.1  90/01/02  13:50:28  rvb
 * 	Flush MACH_TIME.
 * 
 * Revision 1.4  89/04/05  12:57:30  rvb
 * 	X78: no more vmmeter
 * 	[89/03/24            rvb]
 * 
 * Revision 1.3  89/02/26  12:31:18  gm0w
 * 	Changes for cleanup.
 * 
 * 31-Dec-88  Robert Baron (rvb) at Carnegie-Mellon University
 *	Derived from MACH2.0 vax release.
 *	Still a lot of dead wood to cleanup.
 *
 * 11-Dec-87  Stephen Schwab (schwab) at Carnegie-Mellon University
 *	For 650, define ssc timer symbolic offsets.
 *
 */

#import <confdep.h>

#import <stddef.h>
#import <mach/mach_types.h>

#import <sys/param.h>
#import <sys/buf.h>
#import <sys/vmparam.h>
#import <sys/dir.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <sys/mbuf.h>
#import <sys/msgbuf.h>

#import <kern/lock.h>

#import <kern/thread.h>
#import <kern/task.h>

#import <fp_emul.h>
#if	FP_EMUL
#import <machdep/i386/fp_emul/fp_emul.h>
#endif

int _ERR = offsetof(thread_saved_state_t, frame.err);
int _EFL = offsetof(thread_saved_state_t, frame.eflags);
int _EBP = offsetof(thread_saved_state_t, regs.ebp);
int _P_PRI = offsetof(struct proc, p_priority);
int _P_STAT = offsetof(struct proc, p_stat);
int _P_SIG = offsetof(struct proc, p_siglist);
int _P_FLAG = offsetof(struct proc, p_flag);
int _SSLEEP = SSLEEP;
int _SRUN = SRUN;
int _RU_MINFLT = offsetof(struct rusage, ru_minflt);
int _PR_BASE = offsetof(struct uprof, pr_base);
int _PR_SIZE = offsetof(struct uprof, pr_size);
int _PR_OFF = offsetof(struct uprof, pr_off);
int _PR_SCALE = offsetof(struct uprof, pr_scale);
int _U_AR0 = offsetof(struct uthread, uu_ar0);
int _THREAD_PCB = offsetof(struct thread, pcb);
int _THREAD_RECOVER = offsetof(struct thread, recover);
int _THREAD_TASK = offsetof(struct thread, task);
int _THREAD_AST = offsetof(struct thread, ast);
int _AST_ZILCH = AST_ZILCH;
//	sel_conv.sel = NULL_SEL;
//int _NULLSEL = sel_conv.sel_data);
sel_t _NULLSEL = NULL_SEL;
//	sel_conv.sel = KCS_SEL;
//int _KCSSEL = sel_conv.sel_data);
sel_t _KCSSEL = KCS_SEL;
//	sel_conv.sel = KDS_SEL;
//int _KDSSEL = sel_conv.sel_data);
sel_t _KDSSEL = KDS_SEL;
//	sel_conv.sel = LCODE_SEL;
//int _LCODESEL = sel_conv.sel_data);
sel_t _LCODESEL = LCODE_SEL;
//	sel_conv.sel = LDATA_SEL;
//int _LDATASEL = sel_conv.sel_data);
sel_t _LDATASEL = LDATA_SEL;
int _VM_MIN_KERNEL_ADDRESS  = VM_MIN_KERNEL_ADDRESS;
int _KERNEL_LINEAR_BASE  = KERNEL_LINEAR_BASE;
int _KERN_FAILURE = KERN_FAILURE;

#if	FP_EMUL
//	sel_conv.sel = FPSTATE_SEL;
//int _FPSTATESEL = sel_conv.sel_data);
int _sr_masks  = offsetof(fp_state_t, environ.control);
int _sr_controls  = (offsetof(fp_state_t, environ.control)+1);
int _sr_errors  =offsetof(fp_state_t, environ.status);
int _sr_flags  = (offsetof(fp_state_t, environ.status)+1);
int _sr_tags  = offsetof(fp_state_t, environ.tag);
int _sr_instr_offset  = offsetof(fp_state_t, environ.ip);
int _sr_instr_base  = offsetof(fp_state_t, environ.cs);
int _sr_mem_offset  = offsetof(fp_state_t, environ.dp);
int _sr_mem_base  = offsetof(fp_state_t, environ.ds);
int _sr_regstack  = offsetof(fp_state_t, stack);

int _work_area  = offsetof(fp_emul_state_t, emul_work_area);
int _mem_operand_pointer  = offsetof(fp_emul_state_t, emul_work_area.mem_operand_pointer);
int _instruction_pointer  = offsetof(fp_emul_state_t, emul_work_area.instruction_pointer);
int _operation_type  = offsetof(fp_emul_state_t, emul_work_area.operation_type);
int _reg_num  = offsetof(fp_emul_state_t, emul_work_area.reg_num);
int _op1_format  = offsetof(fp_emul_state_t, emul_work_area.op1_format);
int _op1_location  = offsetof(fp_emul_state_t, emul_work_area.op1_location);
int _op1_use_up  = offsetof(fp_emul_state_t, emul_work_area.op1_use_up);
int _op2_format  = offsetof(fp_emul_state_t, emul_work_area.op2_format);
int _op2_location  = offsetof(fp_emul_state_t, emul_work_area.op2_location);
int _op2_use_up  = offsetof(fp_emul_state_t, emul_work_area.op2_use_up);
int _result_format  = offsetof(fp_emul_state_t, emul_work_area.result_format);
int _result_location  = offsetof(fp_emul_state_t, emul_work_area.result_location);
int _result2_format  = offsetof(fp_emul_state_t, emul_work_area.result2_format);
int _dword_frac1  = offsetof(fp_emul_state_t, emul_work_area.dword_frac1);
int _sign1  = offsetof(fp_emul_state_t, emul_work_area.sign1);
int _sign1_ext  = offsetof(fp_emul_state_t, emul_work_area.sign1_ext);
int _tag1  = offsetof(fp_emul_state_t, emul_work_area.tag1);
int _tag1_ext  = offsetof(fp_emul_state_t, emul_work_area.tag1_ext);
int _expon1  = offsetof(fp_emul_state_t, emul_work_area.expon1);
int _expon1_ext  = offsetof(fp_emul_state_t, emul_work_area.expon1_ext);
int _dword_frac2  = offsetof(fp_emul_state_t, emul_work_area.dword_frac2);
int _sign2  = offsetof(fp_emul_state_t, emul_work_area.sign2);
int _sign2_ext  = offsetof(fp_emul_state_t, emul_work_area.sign2_ext);
int _tag2  = offsetof(fp_emul_state_t, emul_work_area.tag2);
int _tag2_ext  = offsetof(fp_emul_state_t, emul_work_area.tag2_ext);
int _expon2  = offsetof(fp_emul_state_t, emul_work_area.expon2);
int _expon2_ext  = offsetof(fp_emul_state_t, emul_work_area.expon2_ext);
int _before_error_signals  = offsetof(fp_emul_state_t, emul_work_area.before_error_signals);
int _extra_dword_reg  = offsetof(fp_emul_state_t, emul_work_area.extra_dword_reg);
int _result_dword_frac  = offsetof(fp_emul_state_t, emul_work_area.result_dword_frac);
int _result_sign  = offsetof(fp_emul_state_t, emul_work_area.result_sign);
int _result_sign_ext  = offsetof(fp_emul_state_t, emul_work_area.result_sign_ext);
int _result_tag  = offsetof(fp_emul_state_t, emul_work_area.result_tag);
int _result_tag_ext  = offsetof(fp_emul_state_t, emul_work_area.result_tag_ext);
int _result_expon  = offsetof(fp_emul_state_t, emul_work_area.result_expon);
int _result_expon_ext  = offsetof(fp_emul_state_t, emul_work_area.result_expon_ext);
int _result2_dword_frac  = offsetof(fp_emul_state_t, emul_work_area.result2_dword_frac);
int _result2_sign  = offsetof(fp_emul_state_t, emul_work_area.result2_sign);
int _result2_sign_ext  = offsetof(fp_emul_state_t, emul_work_area.result2_sign_ext);
int _result2_tag  = offsetof(fp_emul_state_t, emul_work_area.result2_tag);
int _result2_tag_ext  = offsetof(fp_emul_state_t, emul_work_area.result2_tag_ext);
int _result2_expon  = offsetof(fp_emul_state_t, emul_work_area.result2_expon);
int _result2_expon_ext  = offsetof(fp_emul_state_t, emul_work_area.result2_expon_ext);
int _dword_cop  = offsetof(fp_emul_state_t, emul_work_area.dword_cop);
int _dword_dop  = offsetof(fp_emul_state_t, emul_work_area.dword_dop);
int _oprnd_siz32  = offsetof(fp_emul_state_t, emul_work_area.oprnd_siz32);
int _addrs_siz32  = offsetof(fp_emul_state_t, emul_work_area.addrs_siz32);
int _fpstate_ptr  = offsetof(fp_emul_state_t, emul_work_area.fpstate_ptr);
int _is16bit  = offsetof(fp_emul_state_t, emul_work_area.is16bit);
int _inst_counter  = offsetof(fp_emul_state_t, emul_work_area.inst_counter);
int _trapno  = offsetof(fp_emul_state_t, trapno);

int _saved_gs  = offsetof(fp_emul_state_t, emul_regs.gs);
int _saved_fs  = offsetof(fp_emul_state_t, emul_regs.fs);
int _saved_es  = offsetof(fp_emul_state_t, emul_regs.es);
int _saved_ds  = offsetof(fp_emul_state_t, emul_regs.ds);
int _saved_edi  = offsetof(fp_emul_state_t, emul_regs.edi);
int _saved_esi  = offsetof(fp_emul_state_t, emul_regs.esi);
int _saved_ebp  = offsetof(fp_emul_state_t, emul_regs.ebp);
int _saved_ebx  = offsetof(fp_emul_state_t, emul_regs.ebx);
int _saved_edx  = offsetof(fp_emul_state_t, emul_regs.edx);
int _saved_ecx  = offsetof(fp_emul_state_t, emul_regs.ecx);
int _saved_eax  = offsetof(fp_emul_state_t, emul_regs.eax);

int _saved_eip  = offsetof(fp_emul_state_t, emul_frame.eip);
int _saved_cs  = offsetof(fp_emul_state_t, emul_frame.cs);
int _saved_flags  = offsetof(fp_emul_state_t, emul_frame.eflags);
int _old_esp  = offsetof(fp_emul_state_t, emul_frame.esp);
int _old_ss  = offsetof(fp_emul_state_t, emul_frame.ss);

int _WORK_AREA_SIZE  =sizeof(fp_emul_work_t);
int _WORK_AREA_WORDS  =sizeof(fp_emul_work_t)/4;

#endif FP_EMUL

