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

#ifndef __i386__
#error "this is i386 machine dependent"
#endif

#include <mach/mach.h>

/*
 * objc_exitPoints is a private_extern defined in the objective-C messager
 * which is a zero terminated table of a list of text lables to write
 * instructions which will cause the objective-C messager to then call
 * moncount for each message it dispatches.  The instruction at each of these
 * text lables is a "jmp *%eax@" instruction.  The objective-C messager has
 * allocated space after each of these instructions for moninitobjc to write
 * the instructions to call moncount for each message it dispatches.
 * 
 * The instructions written over the "jmp *%eax@" and the allocated space after
 * it are:
 * exitPoint1:	jmp	*%eax@
 *		| replace with the following instructions
 *		pushl   %eax
 *		pushl	%eax
 *		pushl   8(%esp)
 *		call	_moncount
 *		addl    $8,%esp
 *		popl    %eax
 *		jmp	*%eax@
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
    unsigned long i, min, max, addr;
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
	    /* pushl %eax */
	    *p++ = 0x50;
	    /* pushl %eax */
	    *p++ = 0x50;
	    /* pushl 8(%esp) */
	    *p++ = 0xff;
	    *p++ = 0x74;
	    *p++ = 0x24;
	    *p++ = 0x08;
	    /* call moncount */
	    addr = moncount_addr - ((unsigned long)p + 5);
	    *p++ = 0xe8;
	    *p++ = (addr) & 0xff;
	    *p++ = (addr >>  8) & 0xff;
	    *p++ = (addr >> 16) & 0xff;
	    *p++ = (addr >> 24) & 0xff;
	    /* addl $0x08,%esp */
	    *p++ = 0x83;
	    *p++ = 0xc4;
	    *p++ = 0x08;
	    /* popl %eax */
	    *p++ = 0x58;
	    /* jmp *%eax@ */
	    *p++ = 0xff;
	    *p++ = 0xe0;
	}
	/*
	 * The text cache for the this code now needs to be flushed since
	 * it was just written on so that future calls will get the new
	 * instructions.
	cache_flush(min, max-min);
	 */
	asm("jmp 1f");
	asm("1: nop");

	return(objc_entryPoints);
}
