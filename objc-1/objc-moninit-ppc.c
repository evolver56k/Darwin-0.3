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
#ifdef SHLIB
#import "shlib.h"
#undef moninitobjc
#endif

#ifndef __ppc__
#error "this is ppc machine dependent"
#endif

#include <mach/mach.h>

/*
 * objc_exitPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables to write
 * instructions which will cause the objective-C messager to then call
 * moncount for each message it dispatches.  The instruction at each of these
 * text lables is a "bctr" instruction.  The objective-C messager has
 * allocated space after each of these instructions for moninitobjc to write
 * the instructions to call moncount for each message it dispatches.
 * 
 * The instructions written over the "bctr" and the allocated space after
 * it are:
 * exitPoint1:	bctr
 *		; replace with the following instructions
 *		stw	 r3, 24(r1)	; save register parameters
 *		stw	 r4, 28(r1)	;
 *		stw	 r5, 32(r1)	;
 *		stw	 r6, 36(r1)	;
 *		stw	 r7, 40(r1)	;
 *		stw	 r8, 44(r1)	;
 *		stw	 r9, 48(r1)	;
 *		stw	 r10,52(r1)	;
 *		mflr	 r0		; move LR to r0
 *		stw	 r0,8(r1)	; save LR (return addr)
 *		mfcr	 r12		; move CTR to r12
 *		stw	 r12,4(r1)	; save CTR (imp) 
 *		stwu	 r1,-64(r1)	; grow the stack
 *
 *		ori	 r3,r0,0	; first arg is frompc (return addr)
 *		ori	 r4,r12,0	; second arg is selfpc (imp)
 *		bl	 _moncount
 *
 *		lwz	 r1,0(r1)	; restore the stack pointer
 *		lwz	 r12,4(r1)	; 
 *		mtctr	 r12		; restore CTR (imp)
 *		lwz	 r0,8(r1)	;
 *		mtlr	 r0		; restore LR (return addr)
 *		lwz	 r3, 24(r1)	;
 *		lwz	 r4, 28(r1)	;
 *		lwz	 r5, 32(r1)	;
 *		lwz	 r6, 36(r1)	;
 *		lwz	 r7, 40(r1)	;
 *		lwz	 r8, 44(r1)	;
 *		lwz	 r9, 48(r1)	;
 *		lwz	 r10,52(r1)	;
 *		bctr			; goto *imp;
 */
extern unsigned long objc_exitPoints[];

/*
 * objc_entryPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables that should not
 * have a call inserted to moncount in their shared library branch table slot.
 */
extern unsigned long objc_entryPoints[];

/*
 * moninitobjc() is a machine dependent routine that causes objective-C
 * messager to call moncount() for each message it sends.
 */
unsigned long *
moninitobjc(
unsigned long moncount_addr)
{
    unsigned long i, min, max;
    unsigned long *p, disp;
    kern_return_t r;

	if(objc_exitPoints[0] == 0)
	    return(objc_entryPoints);

	/*
	 * Determine the area to vm_protect() for writing the code.
	 */
	min = 0xffffffff;
	max = 0;
	for(i = 0; objc_exitPoints[i] != 0; i++){
	    if(objc_exitPoints[i] < min)
		min = objc_exitPoints[i];
	    if(objc_exitPoints[i] > max)
		max = objc_exitPoints[i];
	}
	max += 116;

	if((r = vm_protect(task_self(), (vm_address_t)min, (vm_size_t)(max-min),
			   FALSE, VM_PROT_READ | VM_PROT_WRITE |
			   VM_PROT_EXECUTE)) != KERN_SUCCESS)
	    return(objc_entryPoints);

	/*
	 * Write in the code to call moncount.
	 */
	for(i = 0; objc_exitPoints[i] != 0; i++){
	    p = (unsigned long *)(objc_exitPoints[i]);
	    /* stw r3,24(r1) */
	    *p++ = 0x90610018;
	    /* stw r4,28(r1) */
	    *p++ = 0x9081001c;
	    /* stw r5,32(r1) */
	    *p++ = 0x90a10020;
	    /* stw r6,36(r1) */
	    *p++ = 0x90c10024;
	    /* stw r7,40(r1) */
	    *p++ = 0x90e10028;
	    /* stw r8,44(r1) */
	    *p++ = 0x9101002c;
	    /* stw r9,48(r1) */
	    *p++ = 0x91210030;
	    /* stw r10,52(r1) */
	    *p++ = 0x91410034;
	    /* mflr r0 */
	    *p++ = 0x7c0802a6;
	    /* stw r0,8(r1) */
	    *p++ = 0x90010008;
	    /* mfcr r12 */
	    *p++ = 0x7d800026;
	    /* stw r12,4(r1) */
	    *p++ = 0x91810004;
	    /* stwu r1,-64(r1) */
	    *p++ = 0x9421ffc0;
	    /* ori r3,r0,0 */
	    *p++ = 0x60030000;
	    /* ori r4,r12,0 */
	    *p++ = 0x61840000;
	    /* bl _moncount */
	    disp = moncount_addr - (unsigned long)(p);
	    *p++ = 0x48000001 | (disp & 0x03fffffc);
	    /* lwz r1,0(r1) */
	    *p++ = 0x80210000;
	    /* lwz r12,4(r1) */
	    *p++ = 0x81810004;
	    /* mtctr r12 */
	    *p++ = 0x7d8903a6;
	    /* lwz r0,8(r1) */
	    *p++ = 0x80010008;
	    /* mtlr r0 */
	    *p++ = 0x7c0803a6;
	    /* lwz r3,24(r1) */
	    *p++ = 0x80610018;
	    /* lwz r4,28(r1) */
	    *p++ = 0x8081001c;
	    /* lwz r5,32(r1) */
	    *p++ = 0x80a10020;
	    /* lwz r6,36(r1) */
	    *p++ = 0x80c10024;
	    /* lwz r7,40(r1) */
	    *p++ = 0x80e10028;
	    /* lwz r8,44(r1) */
	    *p++ = 0x8101002c;
	    /* lwz r9,48(r1) */
	    *p++ = 0x81210030;
	    /* lwz r10,52(r1) */
	    *p++ = 0x81410034;
	    /* bctr */
	    *p++ = 0x4e800420;
	}
	/*
	 * The text cache for the this code now needs to be flushed since
	 * it was just written on so that future calls will get the new
	 * instructions.
	 */
	user_cache_flush(min, max-min);

	return(objc_entryPoints);
}
