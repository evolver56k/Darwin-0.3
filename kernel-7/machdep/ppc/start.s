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


#include <machdep/ppc/asm.h>
#include <machdep/ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <assym.h>
	
/*
 * Interrupt and bootup stack for initial processor
 */

	.file	"start.s"
	
	.data
	.align  PPC_PGSHIFT		/* Align on page boundry */
	.set	., .+PPC_PGBYTES	/* Red zone for interrupt stack*/
        .globl  EXT(intstack)		/* intstack itself */
EXT(intstack):
	.set	., .+INTSTACK_SIZE

	/*
	 * intstack_top_ss points to the topmost saved
	 * state structure in the intstack
	 */
	.align	ALIGNMENT
        .globl  EXT(intstack_top_ss)
EXT(intstack_top_ss):
	.long	EXT(intstack)+INTSTACK_SIZE-SS_SIZE


	/* GDB stack - used by the debugger if present */
        .globl  EXT(gdbstack)
EXT(gdbstack):
	.set	., .+KERNSTACK_SIZE

	/*
	 * gdbstack_top_ss points to the topmost saved
	 * state structure in the gdbstack
	 */
	.align	ALIGNMENT
        .globl  EXT(gdbstack_top_ss)
EXT(gdbstack_top_ss):
	.long	EXT(gdbstack)+KERNSTACK_SIZE-SS_SIZE

        .globl  EXT(gdbstackptr)
EXT(gdbstackptr):
	.long	EXT(gdbstack)+KERNSTACK_SIZE-SS_SIZE
	

/*
 * All CPUs start here.
 *
 * How we got to here (as of today 1997.02.27) in execution:
 *
 * OpenFirmware loaded the 'SecondaryLoader' from a bootp/tftp server
 * on the same subnet as this machine.  The SecondaryLoader brought
 * this mach-o (MH_PRELOAD) image into memory and jumped to the
 * start address.
 *
 * We expect that:
 *	we are in supervisor mode (of course!)
 *	Translations are off
 *	ARG0 == pointer to startup parameters
 *
 * We do the following:
 *	Force the MSR to a known state
 *	Switch to our stack
 *	Call into the C code.
 *	
 */

	.data
	.align	3
EXT(FloatInit):
	.long	0xC24BC195		/* Initial value */
	.long	0x87859393		/* of floating point registers */
    .globl  EXT(FloatInit)

	.text
	.align 2
	
ASENTRY(start)


#ifndef UseOpenFirmware
	/* Make sure our MSR is valid */
	li	r0,	MSR_VM_OFF
	sync
	mtmsr	r0
	isync
#ifdef UNCACHED_DATA_604
	lis	r28, 0x4	/* flush 128K */
	lis	r0, 0x2
.L_loop_flush:
	lwzx	r29, 0, r28
	subic	r28, r28, 32
	cmpw	r0, r28
	bge+	.L_loop_flush
#endif	/* UNCACHED_DATA_604 */

/*	li	r28, 0x84 */ 	/* turn on branch prediction, turn of serial */
/*	li	r28, 0 */

	mfspr	r28,	hid0

#ifdef UNCACHED_DATA_604
	rlwinm	r28,	r28,	0,	21+1,	21-1 /* invalidate data cache */
	rlwinm	r28,	r28,	0,	17+1,	17-1 /* turn off data cache */
#endif	/* UNCACHED_DATA_604 */
#ifdef UNCACHED_INST_604
	rlwinm	r28,	r28,	0,	20+1,	20-1 /* invalidate inst cache */
	rlwinm	r28,	r28,	0,	16+1,	16-1 /* turn off inst cache */
#endif	/* UNCACHED_INST_604 */

	//rlwinm	r28,	r28,	0,	24+1,	24-1 /* turn on serial execution */

	mtspr	hid0,r28

#endif /* UseOpenFirmware */

	li	r0,	MSR_VM_OFF|MASK(MSR_FP)
	mtmsr	r0
	isync

	lis		r29,hi16(EXT(FloatInit))	/* Get top of floating point init value */
	ori		r29,r29,lo16(EXT(FloatInit))	/* Slam bottom */
	lfd		f0,0(r29)			/* Initialize FP0 */
	fmr		f1,f0				/* Ours is not */
	fmr		f2,f0				/* to wonder why, */
	fmr		f3,f0				/* ours is but to */
	fmr		f4,f0				/* do or die! */
	fmr		f5,f0
	fmr		f6,f0
	fmr		f7,f0
	fmr		f8,f0
	fmr		f9,f0
	fmr		f10,f0
	fmr		f11,f0
	fmr		f12,f0
	fmr		f13,f0
	fmr		f14,f0
	fmr		f15,f0
	fmr		f16,f0
	fmr		f17,f0
	fmr		f18,f0
	fmr		f19,f0
	fmr		f20,f0
	fmr		f21,f0
	fmr		f22,f0
	fmr		f23,f0
	fmr		f24,f0
	fmr		f25,f0
	fmr		f26,f0
	fmr		f27,f0
	fmr		f28,f0
	fmr		f29,f0
	fmr		f30,f0
	fmr		f31,f0

	li	r0,	MSR_VM_OFF
	mtmsr	r0
	isync
	
	addis	r29,	0,	hi16(EXT(intstack_top_ss))
	ori	r29,	r29,	lo16(EXT(intstack_top_ss))
	lwz	r29,	0(r29)

#ifdef GPROF
	subi    r29, r29, FM_REDZONE	; we save more state in ppc_init()
#endif

	li	r28,	0
	stw	r28,	FM_BACKPTR(r29) /* store a null frame backpointer */

	/* move onto new stack */
	
	mr	r1,	r29

	bl	EXT(ppc_init)

	/* Should never return */

	BREAKPOINT_TRAP
