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

#ifdef	KERNEL_PRIVATE

#ifndef	_MACH_I386_SYSCALL_SW_H_
#define _MACH_I386_SYSCALL_SW_H_

#include <architecture/i386/asm_help.h>

/*
 * These defines are for future use (if we want to change the system call
 * argument passing conventions).
 */
#define kernel_trap_args_0	;
#define kernel_trap_args_1
#define kernel_trap_args_2
#define kernel_trap_args_3
#define kernel_trap_args_4
#define kernel_trap_args_5
#define kernel_trap_args_6
#define kernel_trap_args_7
#define kernel_trap_args_9
#define kernel_trap_args_9

#define save_registers_0
#define save_registers_1
#define save_registers_2
#define save_registers_3
#define save_registers_4
#define save_registers_5
#define save_registers_6
#define save_registers_7
#define save_registers_8
#define save_registers_9

#define restore_registers_0
#define restore_registers_1
#define restore_registers_2
#define restore_registers_3
#define restore_registers_4
#define restore_registers_5
#define restore_registers_6
#define restore_registers_7
#define restore_registers_8
#define restore_registers_9

#define MACHCALLSEL	0x33

#define kernel_trap(trap_name, trap_number, number_args)	\
LEAF(_##trap_name, 0)						;\
	save_registers_##number_args				;\
	kernel_trap_args_##number_args				;\
	movl	$##trap_number, %eax				;\
	lcall	$##MACHCALLSEL, $0				;\
	restore_registers_##number_args				;\
END(_##trap_name)


#endif	/* _MACH_I386_SYSCALL_SW_H_ */

#endif	/* KERNEL_PRIVATE */
