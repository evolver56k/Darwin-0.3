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
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */


#include <architecture/i386/asm_help.h>
#include "memory.h"

#define data32	.byte 0x66

// boot2() -- second stage boot

	.file "boot2.s"

TEXT

LABEL(boot2)
//---------------------------
#if 0
	push	%edx
	data32
	mov	$1,%ebx			// bh=0, bl=1 (blue)
	mov	$0x41,%al		// bios int 10, function 0xe
	mov	$0xe,%ah
	int	$0x10			// bios display a byte in tty mode

//	xor	%ah,%ah
//	int	$0x16			// get key
	pop	%edx
	
	call	__real_to_prot
	call	__prot_to_real

	push	%edx
	data32
	mov	$1,%ebx			// bh=0, bl=1 (blue)
	mov	$0x42,%al		// bios int 10, function 0xe
	mov	$0xe,%ah
	int	$0x10			// bios display a byte in tty mode

//	xor	%ah,%ah
//	int	$0x16			// get key
	pop	%edx
	
#endif
//---------------------------
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %es

//	/* move booter to real location */
//	data32
//	movl	BOOTER_LOAD_ADDR,%esi
//	cld
//	data32
//	movl	BOOTER_ADDR,%edi
//	data32
//	movl	BOOTER_LEN, %ecx
//	repnz; movsb
	
	/* change to protected mode */
	data32
	call	__real_to_prot

	pushl	%edx			// bootdev
	call	_boot
	ret

//	. = SAIO_TABLE_PTR_OFFSET
//LABEL(_saio_table_pointer)
//	.value	_saio_functions
//	.value 0
	