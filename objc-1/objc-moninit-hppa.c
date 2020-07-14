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

#ifndef __hppa__
#error "this is hppa machine dependent"
#endif

#include <mach/mach.h>

/*
 * objc_exitPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables to write
 * instructions which will cause the objective-C messager to then call
 * moncount for each message it dispatches.  The instruction at each of these
 * text lables is a "bv,n 0(%r19)" instruction.  The objective-C messager has
 * allocated space after each of these instructions for moninitobjc to write
 * the instructions to call moncount for each message it dispatches.
 * 
 * The instructions written over the "bv,n 0(%r19)" and the allocated space 
 * after it are:
 * exitPoint1:	bv,n 0(%r19)
 * | replace with the following instructions
 *	copy	%r30,%r20
 *	ldo  	128(%r30),%r30		; Allocate space on stack
 *	stwm	%r2,4(0,%r20)		; Save return pointer
 *	stwm	%r19,4(0,%r20)		; Save imp
 *	stwm	%r23,4(0,%r20)		; Save old args
 *	stwm	%r24,4(0,%r20)		;
 *	stwm	%r25,4(0,%r20)		;
 *	stwm	%r26,4(0,%r20)		;
 *	fstds,ma  %fr4,8(0,%r20)	; Save floating point args
 *	fstds,ma  %fr5,8(0,%r20)	;   (could actually save singles
 *	fstds,ma  %fr6,8(0,%r20)	;    for fr4 & fr6, but so what?)
 *	fstds,ma  %fr7,8(0,%r20)	;   
 *	copy	%r2,%r26		; moncount frompc param.
 *	copy	%r19,%r25		; moncount selfpc param.
 *	ldil	L`_moncount,%r1		;
 *      ble	R`_moncount(%sr4,%r1)	;
 *	copy	%r31,%r2		; <delay slot> mrp->rp
 *	ldo	-128(%r30),%r30		;   deallocate
 *	copy	%r30,%r20		;
 *	ldwm	4(0,%r20),%r2		; restore everything
 *	ldwm	4(0,%r20),%r19		; 
 *	ldwm	4(0,%r20),%r23		; 
 *	ldwm	4(0,%r20),%r24		;
 *	ldwm	4(0,%r20),%r25		;
 *	ldwm	4(0,%r20),%r26		;
 *	fldds,ma  8(0,%r20),%fr4	;
 *	fldds,ma  8(0,%r20),%fr5	;
 *	fldds,ma  8(0,%r20),%fr6	;
 *	fldds,ma  8(0,%r20),%fr7	;
 *	bv,n	0(%r19)			; goto *imp
 *	
 */
extern unsigned long objc_exitPoints[];

/*
 * objc_entryPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables that should not
 * have a call inserted to moncount in their shared library branch table slot.
 */
extern unsigned long objc_entryPoints[];

/* _moncode contains the assembled version of the code listed above.
 * moninit_objc will bcopy it to the right places and adjust the
 * (relative) branch instruction appropriately.
 */

static unsigned long _moncode[] = {
  0x081e0254,
  0x37de0100,
  0x6e820008,
  0x6e930008,
  0x6e970008,
  0x6e980008,
  0x6e990008,
  0x6e9a0008,
  0x2e901224,
  0x2e901225,
  0x2e901226,
  0x2e901227,
  0x0802025a,
  0x08130259,
  0x20200000, /* ldil L`_moncount,%r1 */
  0xe4202000, /* ble  R`_moncount(%sr4,%r1) */
  0x081f0242,
  0x37de3f01,
  0x081e0254,
  0x4e820008,
  0x4e930008,
  0x4e970008,
  0x4e980008,
  0x4e990008,
  0x4e9a0008,
  0x2e901024,
  0x2e901025,
  0x2e901026,
  0x2e901027,
  0xea60c002
  };
#define LDIL_INSTRUCTION_INDEX 14
#define BLE_INSTRUCTION_INDEX 15

static unsigned long dis_assemble_21(as21)
     unsigned int as21;
{
  unsigned long temp;


  temp  = ( as21 & 0x100000 ) >> 20;
  temp |= ( as21 & 0x0ffe00 ) >> 8;
  temp |= ( as21 & 0x000180 ) << 7;
  temp |= ( as21 & 0x00007c ) << 14; 
  temp |= ( as21 & 0x000003 ) << 12;
  return temp ;
}

static void dis_assemble_17(as17,x,y,z)
     unsigned int as17;
     unsigned int *x,*y,*z;
{

  *z =   ( as17 & 0x10000 ) >> 16;
  *x =   ( as17 & 0x0f800 ) >> 11;
  *y = ( ( as17 & 0x00400 ) >> 10 ) | ( ( as17 & 0x3ff ) << 1 );
}

/*
 * moninitobjc() is a machine dependent routine that causes objective-C
 * messager to call moncount() for each message it sends.
 */

unsigned long *
moninitobjc(
unsigned long moncount_addr)
{
    unsigned long i, min, max;
    unsigned long *p;
    kern_return_t r;
    unsigned w,w1,w2;
    
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
	max += 80;

	if((r = vm_protect(task_self(), (vm_address_t)min, (vm_size_t)(max-min),
			   FALSE, VM_PROT_READ | VM_PROT_WRITE |
			   VM_PROT_EXECUTE)) != KERN_SUCCESS)
	    return(objc_entryPoints);

	/*
	 * Write in the code to call moncount.
	 */
	for(i = 0; objc_exitPoints[i] != 0; i++){
        
	    p = (unsigned long *)(objc_exitPoints[i]);
	    memcpy(p,_moncode,sizeof(_moncode));
	    
	    /* Calculate the displacements for the branch to _moncount. */
	    p[LDIL_INSTRUCTION_INDEX] |= dis_assemble_21(moncount_addr>>11);
	    dis_assemble_17((moncount_addr & 0x7ff)>>2,&w1,&w2,&w);
	    p[BLE_INSTRUCTION_INDEX] |= (w1<<16) | (w2<<2) | w;
	}
	/*
	 * The text cache for the this code now needs to be flushed since
	 * it was just written on so that future calls will get the new
	 * instructions.
	 */
#warning objc-moninit-hppa not doing text cache flush!!
//	user_cache_flush(min, max-min);

	return(objc_entryPoints);
}
