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

/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	mach/ppc/syscall_sw.h
 *	Author:	Matt Watson, NeXT Computer, Inc.
 *
 *	This header file defines system call macros
 *	for the PowerPC family processors.
 *
 * HISTORY
 *
 *  4-Apr-97  Umesh Vaishampayan (umeshv@NeXT.com)
 *	Fixed the system call calling convention.
 *
 *  28-Mar-97  Matt Watson (mwatson@next.com)
 *	Ported to ppc.
 *
 *  6-Nov-92  Ben Fathi (benf@next.com)
 *	Ported to m98k.
 *
 * 23-Jan-91  Mike DeMoney (mike@next.com)
 *	Created.
 */

#ifndef	_MACH_PPC_SYSCALL_SW_H_
#define	_MACH_PPC_SYSCALL_SW_H_

#import <architecture/ppc/asm_help.h>
#import <architecture/ppc/pseudo_inst.h>

/*
 * Mach system call traps.
 *
 * The first 8 system call args are passed by user-code in
 * a0 - a7. (Should there ever need to be more
 * than 8 args, those should be passed in s0 - s17.)
 *
 * The system call number is in r0.
 *
 * NOTE: (Like the C calling convention), the kernel does not
 * preserve caller-saves across system calls.
 */

/*
 * kernel_trap_args_N -- Put the arguments in the right places.
 * Args are passed in a0 - a7, system call number in ep.
 */
#define	kernel_trap_args_0
#define	kernel_trap_args_1
#define	kernel_trap_args_2
#define	kernel_trap_args_3
#define	kernel_trap_args_4
#define	kernel_trap_args_5
#define	kernel_trap_args_6
#define	kernel_trap_args_7

/*
 * simple_kernel_trap -- Mach system calls with 8 or less args
 * Args are passed in a0 - a7, system call number in r0.
 * Do a "sc" instruction to enter kernel.
 */	
#define simple_kernel_trap(trap_name, trap_number)	\
	.globl	_##trap_name				@\
_##trap_name:						@\
	li	r0,trap_number				 @\
	sc						 @\
	blr						 @\
	END(trap_name)

#define kernel_trap_0(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_1(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_2(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_3(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_4(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_5(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_6(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_7(trap_name,trap_number)		 \
	simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_8(trap_name,trap_number)		 \
        simple_kernel_trap(trap_name,trap_number)

#define kernel_trap_9(trap_name,trap_number)		 \
        simple_kernel_trap(trap_name,trap_number)

#define kernel_trap(trap_name,trap_number,nargs)	 \
	kernel_trap_##nargs(trap_name,trap_number)

#endif	/* _MACH_PPC_SYSCALL_SW_H_ */
