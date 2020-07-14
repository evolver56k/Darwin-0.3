/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	SYS.h
 *
 *	Definition of the user side of the UNIX system call interface
 *	for M98K.
 *
 *	Errors are flagged by the location of the trap return (ie., which
 *	instruction is executed upon rfi):
 *
 *		SC PC + 4:	Error (typically branch to cerror())
 *		SC PC + 8:	Success
 *
 * HISTORY
 * 18-Nov-92	Ben Fathi (benf@next.com)
 *	Ported to m98k.
 *
 *  9-Jan-92	Peter King (king@next.com)
 *	Created.
 */

#define KERNEL_PRIVATE	1
/*
 * Header files.
 */
#import	<architecture/ppc/asm_help.h>
#import	<architecture/ppc/pseudo_inst.h>
#import	<mach/ppc/syscall_sw.h>
#import	<mach/ppc/exception.h>
#import	<sys/syscall.h>

/*
 * Macros.
 */

#define	SYSCALL(name, nargs)			\
	.globl	cerror				@\
LEAF(_##name)					@\
	kernel_trap_args_##nargs		@\
	li	r0,SYS_##name			@\
	sc					@\
	b	1f   				@\
	b	2f				@\
1:	BRANCH_EXTERN(cerror)			@\
.text						\
2:	nop

#define	SYSCALL_NONAME(name, nargs)		\
	.globl	cerror				@\
	kernel_trap_args_##nargs		@\
	li	r0,SYS_##name			@\
	sc					@\
	b	1f   				@\
	b	2f				@\
1:	BRANCH_EXTERN(cerror)			@\
.text						\
2:	nop

#define	PSEUDO(pseudo, name, nargs)		\
LEAF(_##pseudo)					@\
	SYSCALL_NONAME(name, nargs)

