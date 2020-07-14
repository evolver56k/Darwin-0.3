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
 * Locore module for NEXTSTEP/Intel Kernel.
 *
 * HISTORY
 *
 * 17 July 1994 ? at NeXT
 *	Major mods for user state save optimizations.
 * 29 August 1992 ? at NeXT
 *	Created.
 */

#import <architecture/i386/asm_help.h>

#import <assym.h>

	TEXT

#define EXCEPTION(name, number)						\
LEAF(_##name, 0)			;				\
		pushl	$0x00		;				\
		pushl	$##number	;				\
		jmp	trap_handler

#define EXCEPTERR(name, number)						\
LEAF(_##name, 0)			;				\
		pushl	$##number	;				\
		jmp	trap_handler

#define INTERRUPT(name, number)						\
LEAF(_##name, 0)			;				\
		pushl	$0x00		;				\
		pushl	$##number	;				\
		jmp	interrupt

#define		RETURN			 				\
		popl	%gs		;				\
		popl	%fs		;				\
		popl	%es		;				\
		popl	%ds		;				\
		popa			;				\
		addl	$8, %esp	;				\
		iret

EXCEPTION(trp_divr,0x00)
EXCEPTION(trp_debg,0x01)
EXCEPTION(int__nmi,0x02)
EXCEPTION(trp_brpt,0x03)
EXCEPTION(trp_over,0x04)
EXCEPTION(flt_bnds,0x05)
EXCEPTION(flt_opcd,0x06)
EXCEPTION(flt_ncpr,0x07)
EXCEPTERR(abt_dblf,0x08)
EXCEPTION(abt_over,0x09)
EXCEPTERR(flt_btss,0x0A)
EXCEPTERR(flt_bseg,0x0B)
EXCEPTERR(flt_bstk,0x0C)
EXCEPTERR(flt_prot,0x0D)
EXCEPTERR(flt_page,0x0E)
EXCEPTION(trp_0x0F,0x0F)
EXCEPTION(flt_bcpr,0x10)
EXCEPTION(trp_0x11,0x11)
EXCEPTION(trp_0x12,0x12)
EXCEPTION(trp_0x13,0x13)
EXCEPTION(trp_0x14,0x14)
EXCEPTION(trp_0x15,0x15)
EXCEPTION(trp_0x16,0x16)
EXCEPTION(trp_0x17,0x17)
EXCEPTION(trp_0x18,0x18)
EXCEPTION(trp_0x19,0x19)
EXCEPTION(trp_0x1A,0x1A)
EXCEPTION(trp_0x1B,0x1B)
EXCEPTION(trp_0x1C,0x1C)
EXCEPTION(trp_0x1D,0x1D)
EXCEPTION(trp_0x1E,0x1E)
EXCEPTION(trp_0x1F,0x1F)
EXCEPTION(trp_0x20,0x20)
EXCEPTION(trp_0x21,0x21)
EXCEPTION(trp_0x22,0x22)
EXCEPTION(trp_0x23,0x23)
EXCEPTION(trp_0x24,0x24)
EXCEPTION(trp_0x25,0x25)
EXCEPTION(trp_0x26,0x26)
EXCEPTION(trp_0x27,0x27)
EXCEPTION(trp_0x28,0x28)
EXCEPTION(trp_0x29,0x29)
EXCEPTION(trp_0x2A,0x2A)
EXCEPTION(trp_0x2B,0x2B)
EXCEPTION(trp_0x2C,0x2C)
EXCEPTION(trp_0x2D,0x2D)
EXCEPTION(trp_0x2E,0x2E)
EXCEPTION(trp_0x2F,0x2F)
EXCEPTION(trp_0x30,0x30)
EXCEPTION(trp_0x31,0x31)
EXCEPTION(trp_0x32,0x32)
EXCEPTION(trp_0x33,0x33)
EXCEPTION(trp_0x34,0x34)
EXCEPTION(trp_0x35,0x35)
EXCEPTION(trp_0x36,0x36)
EXCEPTION(trp_0x37,0x37)
EXCEPTION(trp_0x38,0x38)
EXCEPTION(trp_0x39,0x39)
EXCEPTION(trp_0x3A,0x3A)
EXCEPTION(trp_0x3B,0x3B)
EXCEPTION(trp_0x3C,0x3C)
EXCEPTION(trp_0x3D,0x3D)
EXCEPTION(trp_0x3E,0x3E)
EXCEPTION(trp_0x3F,0x3F)
//
INTERRUPT(int_0x40,0x40)
INTERRUPT(int_0x41,0x41)
INTERRUPT(int_0x42,0x42)
INTERRUPT(int_0x43,0x43)
INTERRUPT(int_0x44,0x44)
INTERRUPT(int_0x45,0x45)
INTERRUPT(int_0x46,0x46)
INTERRUPT(int_0x47,0x47)
INTERRUPT(int_0x48,0x48)
INTERRUPT(int_0x49,0x49)
INTERRUPT(int_0x4A,0x4A)
INTERRUPT(int_0x4B,0x4B)
INTERRUPT(int_0x4C,0x4C)
INTERRUPT(int_0x4D,0x4D)
INTERRUPT(int_0x4E,0x4E)
INTERRUPT(int_0x4F,0x4F)
INTERRUPT(int_0x50,0x50)
INTERRUPT(int_0x51,0x51)
INTERRUPT(int_0x52,0x52)
INTERRUPT(int_0x53,0x53)
INTERRUPT(int_0x54,0x54)
INTERRUPT(int_0x55,0x55)
INTERRUPT(int_0x56,0x56)
INTERRUPT(int_0x57,0x57)
INTERRUPT(int_0x58,0x58)
INTERRUPT(int_0x59,0x59)
INTERRUPT(int_0x5A,0x5A)
INTERRUPT(int_0x5B,0x5B)
INTERRUPT(int_0x5C,0x5C)
INTERRUPT(int_0x5D,0x5D)
INTERRUPT(int_0x5E,0x5E)
INTERRUPT(int_0x5F,0x5F)
INTERRUPT(int_0x60,0x60)
INTERRUPT(int_0x61,0x61)
INTERRUPT(int_0x62,0x62)
INTERRUPT(int_0x63,0x63)
INTERRUPT(int_0x64,0x64)
INTERRUPT(int_0x65,0x65)
INTERRUPT(int_0x66,0x66)
INTERRUPT(int_0x67,0x67)
INTERRUPT(int_0x68,0x68)
INTERRUPT(int_0x69,0x69)
INTERRUPT(int_0x6A,0x6A)
INTERRUPT(int_0x6B,0x6B)
INTERRUPT(int_0x6C,0x6C)
INTERRUPT(int_0x6D,0x6D)
INTERRUPT(int_0x6E,0x6E)
INTERRUPT(int_0x6F,0x6F)
INTERRUPT(int_0x70,0x70)
INTERRUPT(int_0x71,0x71)
INTERRUPT(int_0x72,0x72)
INTERRUPT(int_0x73,0x73)
INTERRUPT(int_0x74,0x74)
INTERRUPT(int_0x75,0x75)
INTERRUPT(int_0x76,0x76)
INTERRUPT(int_0x77,0x77)
INTERRUPT(int_0x78,0x78)
INTERRUPT(int_0x79,0x79)
INTERRUPT(int_0x7A,0x7A)
INTERRUPT(int_0x7B,0x7B)
INTERRUPT(int_0x7C,0x7C)
INTERRUPT(int_0x7D,0x7D)
INTERRUPT(int_0x7E,0x7E)
INTERRUPT(int_0x7F,0x7F)
INTERRUPT(int_0x80,0x80)
INTERRUPT(int_0x81,0x81)
INTERRUPT(int_0x82,0x82)
INTERRUPT(int_0x83,0x83)
INTERRUPT(int_0x84,0x84)
INTERRUPT(int_0x85,0x85)
INTERRUPT(int_0x86,0x86)
INTERRUPT(int_0x87,0x87)
INTERRUPT(int_0x88,0x88)
INTERRUPT(int_0x89,0x89)
INTERRUPT(int_0x8A,0x8A)
INTERRUPT(int_0x8B,0x8B)
INTERRUPT(int_0x8C,0x8C)
INTERRUPT(int_0x8D,0x8D)
INTERRUPT(int_0x8E,0x8E)
INTERRUPT(int_0x8F,0x8F)
INTERRUPT(int_0x90,0x90)
INTERRUPT(int_0x91,0x91)
INTERRUPT(int_0x92,0x92)
INTERRUPT(int_0x93,0x93)
INTERRUPT(int_0x94,0x94)
INTERRUPT(int_0x95,0x95)
INTERRUPT(int_0x96,0x96)
INTERRUPT(int_0x97,0x97)
INTERRUPT(int_0x98,0x98)
INTERRUPT(int_0x99,0x99)
INTERRUPT(int_0x9A,0x9A)
INTERRUPT(int_0x9B,0x9B)
INTERRUPT(int_0x9C,0x9C)
INTERRUPT(int_0x9D,0x9D)
INTERRUPT(int_0x9E,0x9E)
INTERRUPT(int_0x9F,0x9F)
INTERRUPT(int_0xA0,0xA0)
INTERRUPT(int_0xA1,0xA1)
INTERRUPT(int_0xA2,0xA2)
INTERRUPT(int_0xA3,0xA3)
INTERRUPT(int_0xA4,0xA4)
INTERRUPT(int_0xA5,0xA5)
INTERRUPT(int_0xA6,0xA6)
INTERRUPT(int_0xA7,0xA7)
INTERRUPT(int_0xA8,0xA8)
INTERRUPT(int_0xA9,0xA9)
INTERRUPT(int_0xAA,0xAA)
INTERRUPT(int_0xAB,0xAB)
INTERRUPT(int_0xAC,0xAC)
INTERRUPT(int_0xAD,0xAD)
INTERRUPT(int_0xAE,0xAE)
INTERRUPT(int_0xAF,0xAF)
INTERRUPT(int_0xB0,0xB0)
INTERRUPT(int_0xB1,0xB1)
INTERRUPT(int_0xB2,0xB2)
INTERRUPT(int_0xB3,0xB3)
INTERRUPT(int_0xB4,0xB4)
INTERRUPT(int_0xB5,0xB5)
INTERRUPT(int_0xB6,0xB6)
INTERRUPT(int_0xB7,0xB7)
INTERRUPT(int_0xB8,0xB8)
INTERRUPT(int_0xB9,0xB9)
INTERRUPT(int_0xBA,0xBA)
INTERRUPT(int_0xBB,0xBB)
INTERRUPT(int_0xBC,0xBC)
INTERRUPT(int_0xBD,0xBD)
INTERRUPT(int_0xBE,0xBE)
INTERRUPT(int_0xBF,0xBF)
INTERRUPT(int_0xC0,0xC0)
INTERRUPT(int_0xC1,0xC1)
INTERRUPT(int_0xC2,0xC2)
INTERRUPT(int_0xC3,0xC3)
INTERRUPT(int_0xC4,0xC4)
INTERRUPT(int_0xC5,0xC5)
INTERRUPT(int_0xC6,0xC6)
INTERRUPT(int_0xC7,0xC7)
INTERRUPT(int_0xC8,0xC8)
INTERRUPT(int_0xC9,0xC9)
INTERRUPT(int_0xCA,0xCA)
INTERRUPT(int_0xCB,0xCB)
INTERRUPT(int_0xCC,0xCC)
INTERRUPT(int_0xCD,0xCD)
INTERRUPT(int_0xCE,0xCE)
INTERRUPT(int_0xCF,0xCF)
INTERRUPT(int_0xD0,0xD0)
INTERRUPT(int_0xD1,0xD1)
INTERRUPT(int_0xD2,0xD2)
INTERRUPT(int_0xD3,0xD3)
INTERRUPT(int_0xD4,0xD4)
INTERRUPT(int_0xD5,0xD5)
INTERRUPT(int_0xD6,0xD6)
INTERRUPT(int_0xD7,0xD7)
INTERRUPT(int_0xD8,0xD8)
INTERRUPT(int_0xD9,0xD9)
INTERRUPT(int_0xDA,0xDA)
INTERRUPT(int_0xDB,0xDB)
INTERRUPT(int_0xDC,0xDC)
INTERRUPT(int_0xDD,0xDD)
INTERRUPT(int_0xDE,0xDE)
INTERRUPT(int_0xDF,0xDF)
INTERRUPT(int_0xE0,0xE0)
INTERRUPT(int_0xE1,0xE1)
INTERRUPT(int_0xE2,0xE2)
INTERRUPT(int_0xE3,0xE3)
INTERRUPT(int_0xE4,0xE4)
INTERRUPT(int_0xE5,0xE5)
INTERRUPT(int_0xE6,0xE6)
INTERRUPT(int_0xE7,0xE7)
INTERRUPT(int_0xE8,0xE8)
INTERRUPT(int_0xE9,0xE9)
INTERRUPT(int_0xEA,0xEA)
INTERRUPT(int_0xEB,0xEB)
INTERRUPT(int_0xEC,0xEC)
INTERRUPT(int_0xED,0xED)
INTERRUPT(int_0xEE,0xEE)
INTERRUPT(int_0xEF,0xEF)
INTERRUPT(int_0xF0,0xF0)
INTERRUPT(int_0xF1,0xF1)
INTERRUPT(int_0xF2,0xF2)
INTERRUPT(int_0xF3,0xF3)
INTERRUPT(int_0xF4,0xF4)
INTERRUPT(int_0xF5,0xF5)
INTERRUPT(int_0xF6,0xF6)
INTERRUPT(int_0xF7,0xF7)
INTERRUPT(int_0xF8,0xF8)
INTERRUPT(int_0xF9,0xF9)
INTERRUPT(int_0xFA,0xFA)
INTERRUPT(int_0xFB,0xFB)
INTERRUPT(int_0xFC,0xFC)
INTERRUPT(int_0xFD,0xFD)
INTERRUPT(int_0xFE,0xFE)
INTERRUPT(int_0xFF,0xFF)

	ALIGN
trap_handler:
	// save general registers
	pusha

	// save segment registers
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	// reinitialize segment registers
	movw	$ KDSSEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$ LDATASEL,%ax
	movw	%ax,%fs
	movw	$ NULLSEL,%ax
	movw	%ax,%gs

	// clear direction flag
	cld

	// set frame pointer
	lea	EBP(%esp),%ebp

	// is our stack the save area?
	movl	_empty_stacks,%esi
	movl	%esp,%ebx
	andl	%esi,%esi
	je	0f
	// yes, switch to kernel stack
	// need to do this atomically
	cli
	movl	_stack_pointers,%esp
	movl	$0,_empty_stacks
	sti
0:
	// ptr to saved state
	pushl	%ebx

	call	_catch_trap
	// user mode trap never returns
	// kernel mode trap always returns

	// restore old stack pointer
	// and old value of _empty_stacks
	// need to do this atomically
	cli
	movl	%ebx,%esp
	movl	%esi,_empty_stacks
	sti

	RETURN

	ALIGN
interrupt:
	// save general registers
	pusha

	// save segment registers
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	// reinitialize segment registers
	movw	$ KDSSEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$ LDATASEL,%ax
	movw	%ax,%fs
	movw	$ NULLSEL,%ax
	movw	%ax,%gs

	// clear direction flag
	cld

	// set frame pointer
	lea	EBP(%esp),%ebp

	// is our stack the save area?
	movl	_empty_stacks,%esi
	movl	%esp,%ebx
	andl	%esi,%esi
	je	0f
	// yes, switch to kernel stack
	// interrupts are already disabled
	movl	_stack_pointers,%esp
	movl	$0,_empty_stacks
0:
	// ptr to saved state
	pushl	%ebx

	call	_catch_interrupt
	// user mode interrupt may not return
	// kernel mode interrupt always returns

	// restore old stack pointer	
	movl	%ebx,%esp
	
	// restore old value of _empty_stacks
	movl	%esi,_empty_stacks

	RETURN
	
LEAF(_machdep_call_, 0)
	// save eflags
	pushf
	
	// dummy trapno
	pushl	$0
	
	// save general registers
	pusha
	
	// save segment registers
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	// copy eflags to correct offset
	movl	ERR(%esp),%eax
	movl	%eax,EFL(%esp)

	// reinitialize segment registers
	movw	$ KDSSEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$ LDATASEL,%ax
	movw	%ax,%fs
	movw	$ NULLSEL,%ax
	movw	%ax,%gs
	
	// clear direction flag
	cld

	// set frame pointer
	lea	EBP(%esp),%ebp

	// is our stack the save area?
	movl	_empty_stacks,%esi
	movl	%esp,%ebx
	andl	%esi,%esi
	je	0f
	// yes, switch to kernel stack
	// need to do this atomically
	cli
	movl	_stack_pointers,%esp
	movl	$0,_empty_stacks
	sti
0:
	// ptr to saved state
	pushl	%ebx

	call	_machdep_call
	// call from user mode never returns
	// call from kernel mode always returns (error)

	movl	%ebx,%esp
	
	RETURN
	
LEAF(_mach_kernel_trap_, 0)
	// save eflags
	pushf
	
	// dummy trapno
	pushl	$0
	
	// save general registers
	pusha
	
	// save segment registers
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	// copy eflags to correct offset
	movl	ERR(%esp),%eax
	movl	%eax,EFL(%esp)

	// reinitialize segment registers
	movw	$ KDSSEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$ LDATASEL,%ax
	movw	%ax,%fs
	movw	$ NULLSEL,%ax
	movw	%ax,%gs
	
	// clear direction flag
	cld

	// set frame pointer
	lea	EBP(%esp),%ebp

	// is our stack the save area?
	movl	_empty_stacks,%esi
	movl	%esp,%ebx
	andl	%esi,%esi
	je	0f
	// yes, switch to kernel stack
	// need to do this atomically
	cli
	movl	_stack_pointers,%esp
	movl	$0,_empty_stacks
	sti
0:
	// ptr to saved state
	pushl	%ebx

	call	_mach_kernel_trap
	// call from user mode never returns
	// call from kernel mode always returns (error)

	movl	%ebx,%esp
	
	RETURN
	
LEAF(_unix_syscall_, 0)
	// save eflags
	pushf
	
	// dummy trapno
	pushl	$0
	
	// save general registers
	pusha
	
	// save segment registers
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	// copy eflags to correct offset
	movl	ERR(%esp),%eax
	movl	%eax,EFL(%esp)

	// reinitialize segment registers
	movw	$ KDSSEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	$ LDATASEL,%ax
	movw	%ax,%fs
	movw	$ NULLSEL,%ax
	movw	%ax,%gs
	
	// clear direction flag
	cld

	// set frame pointer
	lea	EBP(%esp),%ebp

	// is our stack the save area?
	movl	_empty_stacks,%esi
	movl	%esp,%ebx
	andl	%esi,%esi
	je	0f
	// yes, switch to kernel stack
	// need to do this atomically
	cli
	movl	_stack_pointers,%esp
	movl	$0,_empty_stacks
	sti
0:
	// ptr to saved state
	pushl	%ebx

	call	_unix_syscall
	// call from user mode never returns
	// call from kernel mode always returns (error)

	movl	%ebx,%esp
	
	RETURN
	
LEAF(_dbf_handler_, 0)
0:	call	_dbf_handler
	jmp	0b

	/*
	 * Return directly to
	 * a *user* context
	 * using a specific return
	 * frame.  Typically, the
	 * frame is in the thread
	 * save area, but it can
	 * also be somewhere on the
	 * kernel stack.  N.B. If the
	 * resumed context is executing
	 * in kernel mode, the system
	 * will crash and burn.
	 */
LEAF(__return_with_state, 0)
	// discard return address
	popl	%eax
	
	// retrieve state ptr
	popl	%ebx

	// switch stack back to save area
	// need to do this atomically
	cli
	movl	%ebx,%esp
	movl	$1,_empty_stacks
	sti
	
	RETURN

	/*
	 * Perform a coroutine
	 * switch between threads.
	 */
LEAF(__switch_tss, 0)
	movl	4(%esp),%eax
	andl	%eax,%eax
	je	0f
	movl	%edi,0x44(%eax)
	movl	%esi,0x40(%eax)
	movl	%ebx,0x34(%eax)
	movl	%ebp,0x3c(%eax)
	popl	%edx
	movl	%edx,0x20(%eax)
	movl	%esp,0x38(%eax)

	movl	4(%esp),%edx
	movl	0x44(%edx),%edi
	movl	0x40(%edx),%esi
	movl	0x34(%edx),%ebx
	movl	0x3c(%edx),%ebp
	movl	8(%esp),%eax
	movl	0x38(%edx),%esp
	movl	0x20(%edx),%edx
	jmp	*%edx

0:
	popl	%edx
	
	movl	4(%esp),%edx
	movl	0x44(%edx),%edi
	movl	0x40(%edx),%esi
	movl	0x34(%edx),%ebx
	movl	0x3c(%edx),%ebp
	movl	8(%esp),%eax
	movl	0x38(%edx),%esp
	movl	0x20(%edx),%edx
	jmp	*%edx
	
LEAF(__stack_attach, 0)
	pushl	%eax
	call	*%ebx
0:	hlt
	jmp	0b
	
LEAF(__call_with_stack, 0)
	movl	4(%esp),%eax
	movl	8(%esp),%esp
	movl	%esp,%ebp
	call	*%eax	
0:	hlt
	jmp	0b

#ifdef	GPROF
LEAF(mcount, 0)
	movl	0(%esp),%eax		// fetch self address
	pushl	%eax
	movl	4(%ebp),%eax		// fetch parent address
	pushl	%eax
	pushf
	popl	%eax
	andl	$0x200,%eax
	je	0f
	cli
	call	_mcount
	sti
	addl	$8,%esp
	ret
	
0:	call	_mcount
	addl	$8,%esp
	ret
#endif	GPROF

#define	O_EDI	0
#define	O_ESI	4
#define	O_EBX	8
#define	O_EBP	12
#define	O_ESP	16
#define O_EIP	20
#define	O_IPL	24

LEAF(_setjmp, 0)
X_LEAF(_set_label, _setjmp)
	call	_curipl
	movl	4(%esp),%edx		// address of save area
	movl	%eax,O_IPL(%edx)	// save ipl level for longjmp
	movl	%edi,O_EDI(%edx)
	movl	%esi,O_ESI(%edx)
	movl	%ebx,O_EBX(%edx)
	movl	%ebp,O_EBP(%edx)
	movl	%esp,O_ESP(%edx)
	movl	0(%esp),%eax		// %eip (return address)
	movl	%eax,O_EIP(%edx)
	xorl	%eax,%eax		// retval <- 0
	ret

LEAF(_longjmp, 0)
X_LEAF(_jump_label, _longjmp)
	movl	4(%esp),%edx		// address of save area
	movl	O_EDI(%edx),%edi
	movl	O_ESI(%edx),%esi
	movl	O_EBX(%edx),%ebx
	movl	O_EBP(%edx),%ebp
	movl	O_ESP(%edx),%esp
	movl	O_EIP(%edx),%eax	// %eip (return address)
	movl	%eax,0(%esp)
	pushl	O_IPL(%edx)
	call	_splx			// restore ipl level
	addl	$4,%esp
	movl	$1,%eax
	ret
