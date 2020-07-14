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
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 *
 * Harness for calling protected-mode BIOS functions.
 */

#import <machdep/i386/asm.h>
#import <assym.h>

#define data32	.byte 0x66
#define addr32	.byte 0x67

#define O_INT	0
#define O_EAX	4
#define O_EBX	8
#define O_ECX	12
#define O_EDX	16
#define O_EDI	20
#define O_ESI	24
#define O_EBP	28
#define O_CS	32
#define O_DS	34
#define O_ES	36
#define O_FLG	40
#define O_ADDR	44

.data
.align 2
save_es:
	.long 0
save_eax:
	.long 0
save_edx:
	.long 0
save_flag:
	.long 0
new_eax:
	.long 0
new_edx:
	.long 0

.text	
ENTRY(_bios32)
	enter	$0,$0
	pushal
	push %es
	push %fs
	push %gs
	pushf
	
	movl	8(%ebp), %edx		// address of save area
	
	movw	O_CS(%edx), %ax
	movw	%ax, save_seg
	movl	O_ADDR(%edx), %eax
	movl	%eax, save_addr
	movl	O_EBX(%edx), %ebx
	movl	O_ECX(%edx), %ecx
	movl	O_EDI(%edx), %edi
	movl	O_ESI(%edx), %esi
	movl	O_EBP(%edx), %ebp
	movl	%edx, save_edx
	movl	O_EAX(%edx), %eax
	movl	%eax, new_eax
	movl	O_EDX(%edx), %eax
	movl	%eax, new_edx
	movw	O_DS(%edx), %ax
	pushw	%ax

	movl	new_eax, %eax
	mov	new_edx, %edx
	popw	%ds

	cli
	.byte	0x9A			// far jmp
save_addr:
	.long 0
save_seg:
	.word 0

	pushf
	mov	$KDSSEL, %eax
	mov	%ax, %ds


	movl	%eax, save_eax
	popl	%eax
	movw	%ax, save_flag
	movw	%es, %ax
	movw	%ax, save_es
	
	movl	%edx, new_edx		// save new edx before clobbering
	movl	save_edx, %edx
	movl	new_edx, %eax		// now move it into buffer
	movl	%eax, O_EDX(%edx)
	movl	save_eax, %eax
	movl	%eax, O_EAX(%edx)
	movw	save_es, %ax
	movw	%ax, O_ES(%edx)
	movw	save_flag, %ax
	movw	%ax, O_FLG(%edx)
	movl	%ebx, O_EBX(%edx)
	movl	%ecx, O_ECX(%edx)
	movl	%edi, O_EDI(%edx)
	movl	%esi, O_ESI(%edx)
	movl	%ebp, O_EBP(%edx)

	popf
	pop %gs
	pop %fs
	pop %es
	
	popal
	leave
	
	ret
	
	
