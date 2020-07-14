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

#include <debug.h>
#include <cpus.h>
#include <kern/thread.h>
#include <mach/exception.h>
#include <kern/syscall_sw.h>
#include <machdep/ppc/cpu_data.h>
#include <mach/thread_status.h>
#include <vm/vm_fault.h>
#include <vm/vm_kern.h> 	/* For kernel_map */
#include <mach/vm_param.h>
#include <machdep/ppc/trap.h>
#include <machdep/ppc/exception.h>
#include <machdep/ppc/proc_reg.h>	/* for SR_xxx definitions */
#include <machdep/ppc/pmap.h>
#include <machine/label_t.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>

#include <machdep/ppc/ast.h>
extern void fpu_save(void);
#define NULL 0

#if DEBUG
#define TRAP_ALL 		-1
#define TRAP_ALIGNMENT		0x01
#define TRAP_DATA		0x02
#define TRAP_INSTRUCTION 	0x04
#define TRAP_AST		0x08
#define TRAP_TRACE		0x10
#define TRAP_PROGRAM		0x20
#define TRAP_EXCEPTION		0x40
#define TRAP_UNRESOLVED		0x80
#define TRAP_SYSCALL		0x100	/* all syscalls */
#define TRAP_HW			0x200	/* all in hw_exception.s */
#define TRAP_MACH_SYSCALL	0x400
#define TRAP_SERVER_SYSCALL	0x800

int trapdebug=0;

#define TRAP_DEBUG(LEVEL, A) {if ((trapdebug & LEVEL)==LEVEL){kprintf A;}}
#else
#define TRAP_DEBUG(LEVEL, A)
#endif

/*
 * XXX don't pass VM_PROT_EXECUTE to vm_fault(), execute permission is implied
 * in either R or RW (note: the pmap module knows this).  This is done for the
 * benefit of programs that execute out of their data space (ala lisp).
 * If we didn't do this in that scenerio, the ITLB miss code would call us
 * and we would call vm_fault() with RX permission.  However, the space was
 * probably vm_allocate()ed with just RW and vm_fault would fail.  The "right"
 * solution to me is to have the un*x server always allocate data with RWX for
 * compatibility with existing binaries.
 */

#define	PROT_EXEC	(VM_PROT_READ)
#define PROT_RO		(VM_PROT_READ)
#define PROT_RW		(VM_PROT_READ|VM_PROT_WRITE)

#define IS_KLOADED(pc) ((vm_offset_t)pc > VM_MIN_KERNEL_LOADED_ADDRESS)

/* A useful macro to update the ppc_exception_state in the PCB
 * before calling doexception
 */
#define UPDATE_PPC_EXCEPTION_STATE {					      \
	thread_t th = current_thread();				      \
	struct ppc_exception_state *es = &th->pcb->es;	      \
	es->dar = dar;							      \
	es->dsisr = dsisr;						      \
	es->exception = trapno / EXC_VECTOR_SIZE;	/* back to powerpc */ \
}

static void unresolved_kernel_trap(int trapno,
				   struct ppc_saved_state *ssp,
				   unsigned int dsisr,
				   unsigned int dar,
				   char *message);

struct ppc_saved_state *trap(int trapno,
			     struct ppc_saved_state *ssp,
			     unsigned int dsisr,
			     unsigned int dar)
{
	int exception=0;
	int code;
	int subcode;
	vm_map_t map;
    unsigned int sp;
	unsigned int space,space2;
	unsigned int offset;
	thread_t th = current_thread();
	int error;
    struct proc			*p = th->task->proc;
    struct timeval		syst;

	TRAP_DEBUG(TRAP_ALL,("NMGS TRAP %d srr0=0x%08x, srr1=0x%08x\n",
		     trapno/4,ssp->srr0,ssp->srr1));

	/* Handle kernel traps first */

	if (!USER_MODE(ssp->srr1)) {
		/*
		 * Trap came from system task, ie kernel or collocated server
		 */
	      	switch (trapno) {
			/*
			 * These trap types should never be seen by trap()
			 * in kernel mode, anyway.
			 * Some are interrupts that should be seen by
			 * interrupt() others just don't happen because they
			 * are handled elsewhere. Some could happen but are
			 * considered to be fatal in kernel mode.
			 */
		case EXC_DECREMENTER:
		case EXC_INVALID:
		case EXC_RESET:
		case EXC_MACHINE_CHECK:
		case EXC_SYSTEM_MANAGEMENT:
		case EXC_INTERRUPT:
		case EXC_FP_UNAVAILABLE:
		case EXC_IO_ERROR:
		default:
			unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			break;
			
		case EXC_TRACE:
#if DEBUG
			enter_debugger(trapno, dsisr, dar, ssp, 0);
#else
			unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif
			break;

		case EXC_RUNMODE_TRACE:
		case EXC_INSTRUCTION_BKPT:
			unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			break;

		case EXC_PROGRAM:
			if (ssp->srr1 & MASK(SRR1_PRG_TRAP)) {
#if DEBUG
				enter_debugger(trapno, dsisr, dar, ssp, 0);
#else
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif /* DEBUG */
			} else {
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
			break;
		case EXC_ALIGNMENT:
			TRAP_DEBUG(TRAP_ALIGNMENT,
				   ("NMGS KERNEL ALIGNMENT_ACCESS, "
				     "DAR=0x%08x, DSISR = 0x%08X\n",
				     dar, dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			if (alignment(dsisr, dar, ssp)) {
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
			}
			break;
		case EXC_DATA_ACCESS:
			TRAP_DEBUG(TRAP_DATA,
				   ("NMGS KERNEL DATA_ACCESS, DAR=0x%08x,"
				     "DSISR = 0x%08X\n", dar, dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

			if (dsisr & MASK(DSISR_WATCH)) {
				printf("dabr ");
#if DEBUG
				enter_debugger(trapno, dsisr,dar,ssp, 1);
#else
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif
				break;
			}
			/* simple case : not SR_COPYIN segment, from kernel */
			if ((dar >> 28) != SR_COPYIN) {
				/* if from kloaded task, use task mapping */
#if notdef_next  /* NeXT kernloaded modules use the kernel map */
				if(IS_KLOADED(ssp->srr0))
					map = th->task->map;
				else 
#endif /* notdef_next */
					map = kernel_map;

				offset = dar;
				TRAP_DEBUG(TRAP_DATA,
					   ("SYSTEM FAULT FROM 0x%08x\n",
					    offset));
				code = vm_fault(map,
						trunc_page(offset),
						dsisr & MASK(DSISR_WRITE) ?
							PROT_RW : PROT_RO,
						FALSE,
						&error);
				if (code != KERN_SUCCESS) {
#if DEBUG
					if (th->recover) {
						label_t *l = (label_t *)th->recover;
						th->recover = (vm_offset_t)NULL;
						longjmp(l,1);
					} else {
					unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
					}
#else
					unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif
				}
				break;
			}

			/* If we get here, the fault was due to a copyin/out */

			map = th->task->map;

			/* Mask out SR_COPYIN and mask in original segment */

			offset = (dar & 0x0fffffff) |
				((mfsrin(dar) & 0xF) << 28);

			TRAP_DEBUG(TRAP_DATA,
				   ("sr=0x%08x, dar=0x%08x, sp=0x%x\n",
				    mfsrin(dar), dar,map->pmap->space));
			assert(((mfsrin(dar) & 0x0FFFFFFF) >> 4) ==
			       map->pmap->space);
			TRAP_DEBUG(TRAP_DATA,
				   ("KERNEL FAULT FROM 0x%x,0x%08x\n",
				    map->pmap->space, offset));

			code = vm_fault(map,
					trunc_page(offset),
					dsisr & MASK(DSISR_WRITE) ?
					PROT_RW : PROT_RO,
					FALSE,
					&error);

			/* If we failed, there should be a recovery
			 * spot to rfi to.
			 */
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_DATA,
					   ("FAULT FAILED- srr0=0x%08x,"
					     "pcb-srr0=0x%08x\n",
					     ssp->srr0,
					     th->pcb->ss.srr0));

				if (th->recover) {
					label_t *l = (label_t *)th->recover;
					th->recover = (vm_offset_t)NULL;
					longjmp(l, 1);
				} else {
					unresolved_kernel_trap(trapno, ssp, dsisr, dar, "copyin/out has no recovery point");
				}
			}
			break;
			
		case EXC_INSTRUCTION_ACCESS:
			/* Colocated tasks aren't necessarily wired and
			 * may page fault
			 */
			TRAP_DEBUG(TRAP_INSTRUCTION,
				   ("NMGS KERNEL INSTRUCTION ACCESS,"
				     "DSISR = 0x%B\n", dsisr,
			     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

#if DEBUG
				enter_debugger(trapno, dsisr,dar,ssp, 1);
#else
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif
#if 0
			/* Make sure it's not the kernel itself faulting */
			assert ((ssp->srr0 >= VM_MIN_KERNEL_LOADED_ADDRESS) &&
				(ssp->srr0 <  VM_MAX_KERNEL_LOADED_ADDRESS));

			/* Same as for data access, except fault type
			 * is PROT_EXEC and addr comes from srr0
			 */
			map = th->task->map;
			space = PPC_SID_KERNEL;
			
			code = vm_fault(map, trunc_page(ssp->srr0),
					PROT_EXEC, FALSE,
					&error);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_INSTRUCTION,
					   ("INSTR FAULT FAILED!\n"));
				exception = EXC_BAD_ACCESS;
				subcode = ssp->srr0;
			}
#endif
			break;

		case EXC_AST:
			TRAP_DEBUG(TRAP_AST,
				   ("EXC_AST FROM KERNEL MODE\n"));
			check_for_ast();
			break;
		}
	} else {
		/*
		 * Trap came from user task
		 */

		if (p)
			syst = p->p_stats->p_ru.ru_stime;


	      	switch (trapno) {
			/*
			 * These trap types should never be seen by trap()
			 * Some are interrupts that should be seen by
			 * interrupt() others just don't happen because they
			 * are handled elsewhere.
			 */
		case EXC_DECREMENTER:
		case EXC_INVALID:
		case EXC_MACHINE_CHECK:
		case EXC_INTERRUPT:
		case EXC_FP_UNAVAILABLE:
		case EXC_SYSTEM_MANAGEMENT:
		case EXC_IO_ERROR:
			
		default:
			panic("Unexpected exception type, %d 0x%x", trapno, trapno);
			break;

		case EXC_RESET:
			powermac_reboot(); 	     /* never returns */

		case EXC_ALIGNMENT:
			TRAP_DEBUG(TRAP_ALIGNMENT,
				   ("NMGS USER ALIGNMENT_ACCESS, "
				     "DAR=0x%08x, DSISR = 0x%B\n", dar, dsisr,
				     "\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			if (alignment(dsisr, dar, ssp)) {
				code = EXC_PPC_UNALIGNED;
				exception = EXC_BAD_ACCESS;
				subcode = dar;
			}
			break;

		case EXC_TRACE:			/* Real PPC chips */
		case EXC_INSTRUCTION_BKPT:	/* 603  PPC chips */
		case EXC_RUNMODE_TRACE:		/* 601  PPC chips */
			TRAP_DEBUG(TRAP_TRACE,("NMGS TRACE TRAP\n"));
			exception = EXC_BREAKPOINT;
			code = EXC_PPC_TRACE;
			subcode = ssp->srr0;
			break;

		case EXC_PROGRAM:
			TRAP_DEBUG(TRAP_PROGRAM,
				   ("NMGS PROGRAM TRAP\n"));
			if (ssp->srr1 & MASK(SRR1_PRG_FE)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("FP EXCEPTION\n"));
				fpu_save();
				fpu_disable();
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_ARITHMETIC;
				code = EXC_ARITHMETIC;
				subcode = per_proc_info[cpu_number()].
					fpu_pcb->fs.fpscr;
			} else if (ssp->srr1 & MASK(SRR1_PRG_ILL_INS)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("ILLEGAL INSTRUCTION\n"));

				UPDATE_PPC_EXCEPTION_STATE
				exception = EXC_BAD_INSTRUCTION;
				code = EXC_PPC_UNIPL_INST;
				subcode = ssp->srr0;
			} else if (ssp->srr1 & MASK(SRR1_PRG_PRV_INS)) {
				TRAP_DEBUG(TRAP_PROGRAM,
					   ("PRIVILEGED INSTRUCTION\n"));

				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_INSTRUCTION;
				code = EXC_PPC_PRIVINST;
				subcode = ssp->srr0;
			} else if (ssp->srr1 & MASK(SRR1_PRG_TRAP)) {
#if MACH_KGDB
				/* Give kernel debugger a chance to
				 * claim the breakpoint before passing
				 * it up as an exception
				 */
				if (kgdb_user_breakpoint(ssp)) {
					call_kgdb_with_ctx(EXC_PROGRAM,
							   0,
							   ssp);
					break;
				}
#endif /* MACH_KGDB */

				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BREAKPOINT;
				code = EXC_PPC_BREAKPOINT;
				subcode = ssp->srr0;
			}
			break;

		case EXC_DATA_ACCESS:
			TRAP_DEBUG(TRAP_DATA,
				   ("NMGS USER DATA_ACCESS, DAR=0x%08x, DSISR = 0x%B\n", dar, dsisr,"\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));
			
			if (dsisr & MASK(DSISR_WATCH)) {
				printf("dabr ");
#if DEBUG
				enter_debugger(trapno, dsisr,dar,ssp, 1);
#else
				unresolved_kernel_trap(trapno, ssp, dsisr, dar, NULL);
#endif
				break;
			}
			map = th->task->map;
			space = map->pmap->space;
			assert(space != 0);
#if DEBUG
			mfsr(space2, SR_UNUSED_BY_KERN);
			space2 = (space2 >> 4) & 0x00FFFFFF;

			/* Do a check that at least SR_UNUSED_BY_KERN
			 * is in the correct user space.
			 * TODO NMGS may not be true as ints are on?.
			 */
			assert(space2 == space);
#if 0
			TRAP_DEBUG(TRAP_DATA,("map = 0x%08x, space=0x%08x, addr=0x%08x\n",map, space, trunc_page(dar));
#endif
#endif /* DEBUG */
			
			code = vm_fault(map, trunc_page(dar),
				 dsisr & MASK(DSISR_WRITE) ? PROT_RW : PROT_RO,
				 FALSE,
				 &error);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_DATA,("FAULT FAILED!\n"));
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_ACCESS;
				subcode = dar;
			}
			break;
			
		case EXC_INSTRUCTION_ACCESS:
			TRAP_DEBUG(TRAP_INSTRUCTION,("NMGS USER INSTRUCTION ACCESS, DSISR = 0x%B\n", dsisr,"\20\2HASH\5PROT\6IO_SPC\7WRITE\12WATCH\14EIO"));

			/* Same as for data access, except fault type
			 * is PROT_EXEC and addr comes from srr0
			 */
			map = th->task->map;
			space = map->pmap->space;
			assert(space != 0);
#if DEBUG
			mfsr(space2, SR_UNUSED_BY_KERN);
			space2 = (space2 >> 4) & 0x00FFFFFF;

			/* Do a check that at least SR_UNUSED_BY_KERN
			 * is in the correct user space.
			 * TODO NMGS is this always true now ints are on?
			 */
			assert(space2 == space);
#endif /* DEBUG */
			
			code = vm_fault(map, trunc_page(ssp->srr0),
					PROT_EXEC, FALSE,
					&error);
			if (code != KERN_SUCCESS) {
				TRAP_DEBUG(TRAP_INSTRUCTION,
					   ("INSTR FAULT FAILED!\n"));
				UPDATE_PPC_EXCEPTION_STATE;
				exception = EXC_BAD_ACCESS;
				subcode = ssp->srr0;
			}
			break;

		case EXC_AST:
			TRAP_DEBUG(TRAP_AST,("EXC_AST FROM USER MODE\n"));
			check_for_ast();
			break;
			
		}
	}

	if (exception) {
		TRAP_DEBUG(TRAP_EXCEPTION,
		   ("doexception (0x%x, 0x%x, 0x%x, 0x%x, pc = 0x%x in %s)\n",
		   exception,code,subcode, th, th->pcb->ss.srr0,
		   &th->task->proc->p_comm));
		doexception(exception, code, subcode, th);
		return ssp;
	}

    if (USER_MODE(ssp->srr1)) {
		if (p && (p->p_flag & P_PROFIL)) {
			int		ticks;
			struct timeval	*tv = &p->p_stats->p_ru.ru_stime;
					
			ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
					(tv->tv_usec - syst.tv_usec) / 1000) /
					(tick / 1000);
			if (ticks)
				addupc_task(p, ssp->srr0, ticks);
		}
	}
	return ssp;
}

#if MACH_ASSERT
/* This routine is called from assembly before each and every system call
 * iff MACH_ASSERT is defined. It must preserve r3.
 */

extern int syscall_trace(int, struct ppc_saved_state *);

int syscall_trace(int retval, struct ppc_saved_state *ssp)
{
	int i, argc;

	if (trapdebug & TRAP_SYSCALL)
		trapdebug |= (TRAP_MACH_SYSCALL|TRAP_SERVER_SYSCALL);

	if (!(trapdebug & (TRAP_MACH_SYSCALL|TRAP_SERVER_SYSCALL)))
		return retval;
	if (ssp->r0 & 0x80000000) {
		/* Mach trap */
		if (!(trapdebug & TRAP_MACH_SYSCALL))
			return retval;

		printf("0x%08x : %30s (",
		       ssp->srr0, mach_trap_table[-(ssp->r0)].mach_trap_name);
		argc = mach_trap_table[-(ssp->r0)].mach_trap_arg_count;
		for (i = 0; i < argc; i++)
			printf("%08x ",*(&ssp->r3 + i));
		printf(")\n");
	} else {
		if (!(trapdebug & TRAP_SERVER_SYSCALL))
			return retval;
		printf("0x%08x : RHAPSODY %3d (", ssp->srr0, ssp->r0);
		argc = 4; /* print 4 of 'em! */
		for (i = 0; i < argc; i++)
			printf("%08x ",*(&ssp->r3 + i));
		printf(")\n");
	}
	return retval;
}
#endif /* MACH_ASSERT */

/*
 * called from syscall if there is an error
 */

int syscall_error(
	int exception,
	int code,
	int subcode,
	struct ppc_saved_state *ssp)
{
	register thread_t thread;

	thread = current_thread();

	if (thread == 0)
	    panic("syscall error in boot phase");

	if (!USER_MODE(ssp->srr1))
		panic("system call called from kernel");

	doexception(exception, code, subcode, thread);

	return 0;
}

/* Pass up a server syscall/exception */
void
doexception(
	    int exc,
	    int code,
	    int sub,
	    thread_t th)
{
	exception(exc, code, sub);
}

char *trap_type[] = {
	"0x0000  INVALID EXCEPTION",
	"0x0100  System reset",
	"0x0200  Machine check",
	"0x0300  Data access",
	"0x0400  Instruction access",
	"0x0500  External interrupt",
	"0x0600  Alignment",
	"0x0700  Program",
	"0x0800  Floating point",
	"0x0900  Decrementer",
	"0x0A00  I/O controller interface",
	"0x0B00  INVALID EXCEPTION",
	"0x0C00  System call exception",
	"0x0D00  Trace",
	"0x0E00  FP assist",
	"0x0F00  Performance monitoring",
	"0x1000  Instruction PTE miss",
	"0x1100  Data load PTE miss",
	"0x1200  Data store PTE miss",
	"0x1300  Instruction Breakpoint",
	"0x1400  System Management",
	"0x1500  INVALID EXCEPTION",
	"0x1600  INVALID EXCEPTION",
	"0x1700  INVALID EXCEPTION",
	"0x1800  INVALID EXCEPTION",
	"0x1900  INVALID EXCEPTION",
	"0x1A00  INVALID EXCEPTION",
	"0x1B00  INVALID EXCEPTION",
	"0x1C00  INVALID EXCEPTION",
	"0x1D00  INVALID EXCEPTION",
	"0x1E00  INVALID EXCEPTION",
	"0x1F00  INVALID EXCEPTION",
	"0x2000  Run Mode/Trace"
	"0x2100  INVALID EXCEPTION",
	"0x2200  INVALID EXCEPTION",
	"0x2300  INVALID EXCEPTION",
	"0x2400  INVALID EXCEPTION",
	"0x2500  INVALID EXCEPTION",
	"0x2600  INVALID EXCEPTION",
	"0x2700  INVALID EXCEPTION",
	"0x2800  INVALID EXCEPTION",
	"0x2900  INVALID EXCEPTION",
	"0x2A00  INVALID EXCEPTION",
	"0x2B00  INVALID EXCEPTION",
	"0x2C00  INVALID EXCEPTION",
	"0x2D00  INVALID EXCEPTION",
	"0x2E00  INVALID EXCEPTION",
	"0x2F00  INVALID EXCEPTION",
};

void unresolved_kernel_trap(int trapno,
			    struct ppc_saved_state *ssp,
			    unsigned int dsisr,
			    unsigned int dar,
			    char *message)
{
	char *trap_name;

#if notdef_next
	if(IS_KLOADED(ssp->srr0)) {
		int exception;

		TRAP_DEBUG(TRAP_UNRESOLVED,
			   ("unresolved trap %d in kernel loaded task %d\n",trapno));
		TRAP_DEBUG(TRAP_UNRESOLVED,
			   ("%s: sending exception message\n", message));
		switch(trapno) {
		case EXC_INVALID:
		case EXC_RESET:
		case EXC_MACHINE_CHECK:
		case EXC_INTERRUPT:
		case EXC_DECREMENTER:
		case EXC_IO_ERROR:
		case EXC_SYSTEM_CALL:
		case EXC_RUNMODE_TRACE:
		default:
			exception = EXC_BREAKPOINT;
			break;
		case EXC_ALIGNMENT:
		case EXC_INSTRUCTION_ACCESS:
		case EXC_DATA_ACCESS:
			exception = EXC_BAD_ACCESS;
			break;
		case EXC_PROGRAM:
			exception = EXC_BAD_INSTRUCTION;
			break;
		case EXC_FP_UNAVAILABLE:
			/* TODO NMGS - should this generate EXC_ARITHMETIC?*/
			exception = EXC_ARITHMETIC;
			break;
		}
		UPDATE_PPC_EXCEPTION_STATE;
		doexception(exception, 0, 0, current_thread());
		return;
	}
#if DEBUG
	regDump(ssp);
#endif /* DEBUG */
#endif /* notdef_next */

	if ((unsigned)trapno <= EXC_MAX)
		trap_name = trap_type[trapno / EXC_VECTOR_SIZE];
	else
		trap_name = "???? unrecognised exception";
	if (message == NULL)
		message = trap_name;

	/* no KGDB so display a backtrace */
	printf("\n\nUnresolved kernel trap: %s DSISR=0x%08x DAR=0x%08x\n",
	       trap_name, dsisr, dar);

#ifdef DEBUG
	enter_debugger(trapno, dsisr, dar, ssp, 1);
#else
	{
	unsigned int* stackptr;
	int i;
	printf("generating stack backtrace prior to panic:\n\n");

	printf("backtrace: 0x%08x ", ssp->srr0);

	stackptr = (unsigned int *)(ssp->r1);

	for (i = 0; i < 8; i++) {

		/* Avoid causing page fault */
		if (pmap_extract(kernel_pmap, (vm_offset_t)stackptr) == 0)
			break;

		stackptr = (unsigned int*)*stackptr;
		/*
		 * TODO NMGS - use FM_LR_SAVE constant, but asm.h defines
		 * too much (such as r1, used above)
		 */

		/* Avoid causing page fault */
		if (pmap_extract(kernel_pmap, (vm_offset_t)stackptr+2) == 0)
			break;

		printf("0x%08x ",*(stackptr + 2));
	}
	printf("\n\n");
	}
#endif

	panic(message);
}
