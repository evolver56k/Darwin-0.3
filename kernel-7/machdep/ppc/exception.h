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
 * Revision 1.3  1997/11/06 21:07:55  umeshv
 * Cleaned up incorrect use of __PPC__, and __I386__
 * Fixed the feature inclusion to be #ifdef KERNEL_BUILD
 *
 * Revision 1.2  1997/10/29 02:13:46  tmason
 * Fixed oodles of bugs related to pmap issues as well as bcopy, FLOAT!, and cached accesses.
 * Radar Bug ID:
 *
 * Revision 1.1.1.1  1997/09/30 02:45:20  wsanchez
 * Import of kernel from umeshv/kernel
 *
 * Radar #1660195
 * Revision 1.1.??.?  1997/06/29  01:52:00  rvega
 *	Radar #1660195
 * 	Add LR plus volatile regs in PP area. This is
 *	needed for the MMU support (PTEG overflow)
 * 	[1997/06/29  01:52:00  rvega]
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

/* Miscellaneous constants and structures used by the exception
 * handlers
 */

#ifndef _PPC_EXCEPTION_H_
#define _PPC_EXCEPTION_H_

#ifndef __ASSEMBLER__

#include <machdep/ppc/mem.h>
#if defined(KERNEL_BUILD)
#include <cpus.h>
#endif /* KERNEL_BUILD */
#include <machdep/ppc/thread.h>

/* When an exception is taken, this info is accessed via sprg0 */

struct per_proc_info {
	unsigned int save_cr;
	unsigned int save_lr;
	unsigned int save_srr0;
	unsigned int save_srr1;
	unsigned int save_dar;
	unsigned int save_dsisr;
	unsigned int save_sprg0;
	unsigned int save_sprg1;
	unsigned int save_sprg2;
	unsigned int save_sprg3;
	unsigned int save_exception_type;
	
	unsigned int phys_exception_handlers;
	unsigned int virt_per_proc_info; /* virt addr for our CPU */
	
	/*	This save area is for the volatile registers used by the
		firmware functions.
	*/
	unsigned int save_r0;
	unsigned int save_r1;
	unsigned int save_r2;
	unsigned int save_r3;
	unsigned int save_r4;
	unsigned int save_r5;
	unsigned int save_r6;
	unsigned int save_r7;
	unsigned int save_r8;
	unsigned int save_r9;
	unsigned int save_r10;
	unsigned int save_r11;
	unsigned int save_r12;
#if TRUE
	unsigned int save_r13;
	unsigned int save_r14;
	unsigned int save_r15;
	unsigned int save_r16;
	unsigned int save_r17;
	unsigned int save_r18;
	unsigned int save_r19;
	unsigned int save_r20;
	unsigned int save_r21;
	unsigned int save_r22;
	unsigned int save_r23;
	unsigned int save_r24;
	unsigned int save_r25;
	unsigned int save_r26;
	unsigned int save_r27;
	unsigned int save_r28;
	unsigned int save_r29;
	unsigned int save_r30;
	unsigned int save_r31;
#endif
	
	unsigned int active_kloaded;	/* pointer to active_kloaded[CPU_NO] */
	unsigned int cpu_data;		/* pointer to cpu_data[CPU_NO] */
	unsigned int active_stacks;	/* pointer to active_stacks[CPU_NO] */
	unsigned int need_ast;		/* pointer to need_ast[CPU_NO] */
	pcb_t	     fpu_pcb;		/* pcb owning the fpu on this cpu */
};

extern struct per_proc_info per_proc_info[NCPUS];

extern char *trap_type[];

#ifdef DEBUG
extern int kdp_trap_codes[];
#define kdp_code(x) kdp_trap_codes[((x)==EXC_AST?0x30:(x)/EXC_VECTOR_SIZE)]
#endif /* DEBUG */

#endif /* ! __ASSEMBLER__ */

#define EXC_VECTOR_SIZE		4		/* function pointer size */

/* Hardware exceptions */

#define EXC_INVALID		(0x00 * EXC_VECTOR_SIZE)
#define EXC_RESET		(0x01 * EXC_VECTOR_SIZE)
#define EXC_MACHINE_CHECK	(0x02 * EXC_VECTOR_SIZE)
#define EXC_DATA_ACCESS		(0x03 * EXC_VECTOR_SIZE)
#define EXC_INSTRUCTION_ACCESS	(0x04 * EXC_VECTOR_SIZE)
#define EXC_INTERRUPT		(0x05 * EXC_VECTOR_SIZE)
#define EXC_ALIGNMENT		(0x06 * EXC_VECTOR_SIZE)
#define EXC_PROGRAM		(0x07 * EXC_VECTOR_SIZE)
#define EXC_FP_UNAVAILABLE	(0x08 * EXC_VECTOR_SIZE)
#define EXC_DECREMENTER		(0x09 * EXC_VECTOR_SIZE)
#define EXC_IO_ERROR		(0x0a * EXC_VECTOR_SIZE)
#define EXC_RESERVED_0B		(0x0b * EXC_VECTOR_SIZE)
#define EXC_SYSTEM_CALL		(0x0c * EXC_VECTOR_SIZE)
#define EXC_TRACE		(0x0d * EXC_VECTOR_SIZE)
#define EXC_FP_ASSIST		(0x0e * EXC_VECTOR_SIZE)
#define EXC_PERFORMANCE_MON	(0x0f * EXC_VECTOR_SIZE)
#define EXC_603_IT_MISS		(0x10 * EXC_VECTOR_SIZE)
#define EXC_603_DLT_MISS	(0x11 * EXC_VECTOR_SIZE)
#define EXC_603_DST_MISS	(0x12 * EXC_VECTOR_SIZE)
#define EXC_INSTRUCTION_BKPT	(0x13 * EXC_VECTOR_SIZE)
#define EXC_SYSTEM_MANAGEMENT	(0x14 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_15		(0x15 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_16		(0x16 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_17		(0x17 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_18		(0x18 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_19		(0x19 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1A		(0x1a * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1B		(0x1b * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1C		(0x1c * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1D		(0x1d * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1E		(0x1e * EXC_VECTOR_SIZE)
#define EXC_RESERVED_1F		(0x1f * EXC_VECTOR_SIZE)
#define EXC_RUNMODE_TRACE	(0x20 * EXC_VECTOR_SIZE) /* 601 only */
#define EXC_RESERVED_21		(0x21 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_22		(0x22 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_23		(0x23 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_24		(0x24 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_25		(0x25 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_26		(0x26 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_27		(0x27 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_28		(0x28 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_29		(0x29 * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2A		(0x2a * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2B		(0x2b * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2C		(0x2c * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2D		(0x2d * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2E		(0x2e * EXC_VECTOR_SIZE)
#define EXC_RESERVED_2F		(0x2f * EXC_VECTOR_SIZE)


/* software exceptions */

#define EXC_AST			(0x100 * EXC_VECTOR_SIZE) 
#define EXC_MAX			EXC_RESERVED_2F	/* Maximum exception no */

#endif /* _PPC_EXCEPTION_H_ */
