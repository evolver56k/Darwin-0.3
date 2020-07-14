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
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	vax/thread.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1987, Avadis Tevanian, Jr.
 *
 *	This file defines machine specific, thread related structures,
 *	variables and macros.
 */

#ifndef	_MACHDEP_PPC_THREAD_
#define _MACHDEP_PPC_THREAD_

#include <kern/kernel_stack.h>
#include <mach/ppc/thread_status.h>

#define CURRENT_THREAD
#include <machdep/ppc/cpu_data.h>

/*
 * PPC process control block
 *
 * In the continuation model, the PCB holds the user context. It is used
 * on entry to the kernel from user mode, either by system call or trap,
 * to store the necessary user registers and other state.
 */
struct pcb
{
	struct ppc_saved_state ss;
	struct ppc_exception_state es;
	struct ppc_float_state fs;
	unsigned ksp;			/* points to TOP OF STACK or zero */
	unsigned sr0;		/* TODO NMGS hack?? sort out user space */
	unsigned long cthread_self;	/* for use of cthread package */
        unsigned flags;
};

typedef struct pcb *pcb_t;

struct ppc_kernel_state
{
        pcb_t    pcb;           /* Efficiency hack, I am told :-) */
        unsigned r1;            /* Stack pointer */
        unsigned r2;            /* TOC/GOT pointer */
        unsigned r13[32-13];    /* Callee saved `non-volatile' registers */
        unsigned lr;            /* `return address' used in continuation */
        unsigned cr;
                                /* Floating point not used, so not saved */
};

#include <machdep/ppc/fpu.h>
#define pcb_synch(thread) fp_state_save(thread)

#define USER_REGS(thread)       (&(thread)->pcb->ss)

#define STACK_IKS(stack)                \
        (((struct ppc_kernel_state *)(((vm_offset_t)stack)+KERNEL_STACK_SIZE))-1)

#ifndef __ASSEMBLER__
/*
 * These routines are called from from task_create() to 
 * manage any pcb state common to all threads in a task 
 * via the "task->pcb_common" pointer. Currently null for sparc.
 */
#define pcb_common_init(task)
#define pcb_common_terminate(task)
#endif  /* !__ASSEMBLER__ */

#endif	/* _MACHDEP_PPC_THREAD_ */
