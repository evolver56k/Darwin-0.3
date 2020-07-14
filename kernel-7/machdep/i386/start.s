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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Kernel startup routine.
 *
 * HISTORY
 *
 * 29 August 1992 ? at NeXT
 *	Created.
 */
 
#import <architecture/i386/asm_help.h>

#import <assym.h>

	TEXT
	/*
	 * Kernel entry point.
	 * Control is transferred here
	 * from the boot loader.  Machine
	 * is in protected mode, with paging
	 * turned off.
	 */
LEAF(_start, 0)

// BIOS/DOS hack			// this says don''t test memory
//	movw	$0x1234, %ax		// on reboot/reset
//	movw	%ax, 0x472		// pretty obscure
// BIOS/DOS hack
	cld

	// Initialize GDT & IDT
	call	_gdt_init
	call	_idt_init

	// switch to new GDT
	ljmp	$ LCODESEL,$start1

	/*
	 * The GDT and IDT descriptors
	 * can't be defined in C due to
	 * alignment problems.
	 */	
	DATA
	.align	3
EXPORT(_gdt_limit)
	.word	0
EXPORT(_gdt_base)
	.long	0

	.align	3
EXPORT(_idt_limit)
	.word	0
EXPORT(_idt_base)
	.long	0
	
	TEXT	
	/*
	 * Running off our GDT
	 */
start1:
	// switch to new GDT
	mov	$ LDATASEL,%ax	
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%fs
	mov	%ax,%gs
	mov	%ax,%ss

	/*
	 * Low level
	 * initialization.
	 * Size memory, set vm_page_size,
	 * setup kernel pmap and enable paging.
	 */
	call	_i386_init

	/*
	 * Paging is now enabled.
	 * Start using the kernel
	 * segments.
	 */
	ljmp	$ KCSSEL,$vstart
	
vstart:
	mov	$ KDSSEL,%ax
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%ss

	// High level initialization
	call	_startup_early
	call	_setup_main

	// Task switch to the first thread.
	push	%eax
	call	_start_initial_context
	// NOTREACHED
	hlt
