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
/*
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 */
#include "SYS.h"

// NeXT 386 signal handler

.set RSAVE_BYTES, 12		// space that saved regs need on stack

	.text
	.align 2

LEAF(__sigtramp, 0)
	// Save registers
	pushl	%eax
	pushl	%ecx
	pushl	%edx

	// Clear the EFLAGS direction bit (DF)
	cld

	// get signal handler
	movl	(RSAVE_BYTES)(%esp), %eax
	sall	$2, %eax
	EXTERN_TO_REG(_sigcatch, %ecx)
	addl	%ecx, %eax
	movl	(%eax), %eax

	// duplicate the signal handler's arguments
	pushl	(8 + 0 + RSAVE_BYTES)(%esp)
	pushl	(4 + 4 + RSAVE_BYTES)(%esp)
	pushl	(0 + 8 + RSAVE_BYTES)(%esp)

	// call signal handler
	call	*%eax

	// pop signal handler arguments off stack
	addl	$12, %esp

	// restore registers
	popl	%edx
	popl	%ecx
	popl	%eax

	// pop off all the kernel supplied arguments
	addl	$8, %esp

	CALL_EXTERN(_sigreturn)

	// if we get here, we have big trouble - the kernel has rejected the
	// stack frame.
	CALL_EXTERN(_abort)
END(__sigtramp)
