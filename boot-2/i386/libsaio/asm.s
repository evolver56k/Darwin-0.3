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
/*
 * HISTORY
 * $Log: asm.s,v $
 * Revision 1.1.1.2  1999/08/04 21:16:57  wsanchez
 * Impoort of boot-66
 *
 * Revision 1.3  1999/08/04 21:12:12  wsanchez
 * Update APSL
 *
 * Revision 1.2  1999/03/25 05:48:30  wsanchez
 * Add APL.
 * Remove unused gzip code.
 * Remove unused Adobe fonts.
 *
 * Revision 1.1.1.1.66.2  1999/03/16 16:08:54  wsanchez
 * Substitute License
 *
 * Revision 1.1.1.1.66.1  1999/03/16 07:33:21  wsanchez
 * Add APL
 *
 * Revision 1.1.1.1  1997/12/05 21:57:57  wsanchez
 * Import of boot-25 (~mwatson)
 *
 * Revision 2.1.1.2  90//03//22  17:59:50  rvb
 * 	Added _sp() => where is the stack at. [kupfer]
 * 
 * Revision 2.1.1.1  90//02//09  17:25:04  rvb
 * 	Add Intel copyright
 * 	[90//02//09            rvb]
 * 
 */


//			INTEL CORPORATION PROPRIETARY INFORMATION
//
//	This software is supplied under the terms of a license  agreement or 
//	nondisclosure agreement with Intel Corporation and may not be copied 
//	nor disclosed except in accordance with the terms of that agreement.
//
//	Copyright 1988 Intel Corporation
// Copyright 1988, 1989 by Intel Corporation
//

#include <architecture/i386/asm_help.h>
#include "memory.h"

#define data32	.byte 0x66
#define addr32	.byte 0x67

	.file "asm.s"

//BOOTSEG		=	0x100
BOOTSEG		=	BASE_SEG

CR0_PE_ON	=	0x1
CR0_PE_OFF	=	0xfffffffe

.globl _Gdtr
	.data
	.align 2,0x90
_Gdtr:
	.word 0x2F
//	.long _Gdt+4096
	.long vtop(_Gdt)
	
	.text

//
// real_to_prot()
// 	transfer from real mode to protected mode.
//	preserves all registers except eax
//

LABEL(__real_to_prot)
	// guarantee that interrupt is disabled when in prot mode
	cli

	// load the gdtr
	addr32
	data32
	lgdt	_Gdtr

	// set the PE bit of CR0 to go to protected mode
	mov	%cr0, %eax
	data32
	or	$CR0_PE_ON, %eax
	mov	%eax, %cr0 

	// make intrasegment jump to flush the processor pipeline and
	// reload CS register
	data32
	ljmp	$0x08, $xprot

xprot:
	// we are in USE32 mode now
	// set up the protected mode segment registers : DS, SS, ES
	mov	$0x10, %eax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es
	
	// clear garbage from upper word of esp
	xorl	%eax, %eax
	movw	%sp, %ax
	movl	%eax, %esp

	ret

//
// prot_to_real()
// 	transfer from protected mode to real mode
//	preserves all registers except eax
// 

LABEL(__prot_to_real)

	// change to USE16 mode
	ljmp	$0x18, $x16

x16:
	// clear the PE bit of CR0
	mov	%cr0, %eax
	data32
	and 	$CR0_PE_OFF, %eax
	mov	%eax, %cr0


	// make intersegment jmp to flush the processor pipeline
	// and reload CS register
	data32
	ljmp $BOOTSEG, $xreal


xreal:
	// we are in real mode now
	// set up the real mode segment registers : DS, SS, ES
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	data32
	ret

#if defined(DEFINE_INLINE_FUNCTIONS)

//
// outb(port, byte)
//

LABEL(_outb)
	push	%ebp
	mov	%esp, %ebp
	push	%edx

	movw	8(%ebp), %dx
	movb	12(%ebp), %al
	outb	%al, %dx

	pop	%edx
	pop	%ebp
	ret

//
// inb(port)
//

LABEL(_inb)
	push	%ebp
	mov	%esp, %ebp
	push	%edx

	movw	8(%ebp), %dx
	subw	%ax, %ax
	inb	%dx, %al

	pop	%edx
	pop	%ebp
	ret

#endif

//
// halt()
//

LABEL(_halt)
//	call	_getchar
	hlt
	jmp	_halt



//
// startprog(phyaddr)
//	start the program on protected mode where phyaddr is the entry point
//

LABEL(_startprog)
	push	%ebp
	mov	%esp, %ebp

	mov	0x8(%ebp), %ecx		// entry offset 
	mov	$0x28, %ebx		// segment
	push	%ebx
	push	%ecx

	// set up %ds and %es
	mov	$0x20, %ebx
	movw	%bx, %ds
	movw	%bx, %es

	lret


#if 0
//
// pcpy(src, dst, cnt)
//	where src is a virtual address and dst is a physical address
//

LABEL(_pcpy)
	push	%ebp
	mov	%esp, %ebp
	push	%es
	push	%esi
	push	%edi
	push	%ecx

	cld

	// set %es to point at the flat segment
	mov	$0x20, %eax
	movw	%ax , %es

	mov	0x8(%ebp), %esi		// source
	mov	0xc(%ebp), %edi		// destination
	mov	0x10(%ebp), %ecx	// count

	rep
	movsb

	pop	%ecx
	pop	%edi
	pop	%esi
	pop	%es
	pop	%ebp

	ret
#endif

LABEL(__sp)
	mov	%esp, %eax
	ret
