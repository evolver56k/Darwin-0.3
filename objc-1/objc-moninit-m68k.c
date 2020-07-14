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

#ifndef __m68k__
#error "this is m68k machine dependent"
#endif

#include <mach/mach.h>

/*
 * objc_exitPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables to write
 * instructions which will cause the objective-C messager to then call
 * moncount for each message it dispatches.  The instruction at each of these
 * text lables is a "jmp a0@" instruction.  The objective-C messager has
 * allocated space after each of these instructions for moninitobjc to write
 * the instructions to call moncount for each message it dispatches.
 * 
 * The instructions written over the "jmp a0@" and the allocated space after
 * it are:
 * exitPoint1:	jmp	a0@
 *		| replace with the following instructions
 *		movel	a0,sp@-
 *		movel	a1,sp@-
 *		movel	a0,sp@-
 *		movel	sp@(12),sp@-
 *		jsr	moncount
 *		addl	#8,sp
 *		movel	sp@+,a1
 *		movel	sp@+,a0
 *		jmp	a0@
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
    char *p;
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
	max += 24;

	if((r = vm_protect(task_self(), (vm_address_t)min, (vm_size_t)(max-min),
			   FALSE, VM_PROT_READ | VM_PROT_WRITE |
			   VM_PROT_EXECUTE)) != KERN_SUCCESS)
	    return(objc_entryPoints);

	/*
	 * Write in the code to call moncount.
	 */
	for(i = 0; objc_exitPoints[i] != 0; i++){
	    p = (char *)(objc_exitPoints[i]);
	    /* movel a0,sp@- */
	    *p++ = 0x2f;
	    *p++ = 0x08;
	    /* movel a1,sp@- */
	    *p++ = 0x2f;
	    *p++ = 0x09;
	    /* movel a0,sp@- */
	    *p++ = 0x2f;
	    *p++ = 0x08;
	    /* movel sp@(12),sp@- */
	    *p++ = 0x2f;
	    *p++ = 0x2f;
	    *p++ = 0x00;
	    *p++ = 0x0c;
	    /* jsr	moncount */
	    *p++ = 0x4e;
	    *p++ = 0xb9;
	    *p++ = (moncount_addr >> 24) & 0xff;
	    *p++ = (moncount_addr >> 16) & 0xff;
	    *p++ = (moncount_addr >>  8) & 0xff;
	    *p++ = (moncount_addr) & 0xff;
	    /* addl #8,sp */
	    *p++ = 0x50;
	    *p++ = 0x8f;
	    /* movel sp@+,a1 */
	    *p++ = 0x22;
	    *p++ = 0x5f;
	    /* movel sp@+,a0 */
	    *p++ = 0x20;
	    *p++ = 0x5f;
	    /* jmp a0@ */
	    *p++ = 0x4e;
	    *p++ = 0xd0;
	}
	/*
	 * The text cache for the this code now needs to be flushed since
	 * it was just written on so that future calls will get the new
	 * instructions.
	cache_flush(min, max-min);
	 */
	asm("trap #2");

	return(objc_entryPoints);
}
