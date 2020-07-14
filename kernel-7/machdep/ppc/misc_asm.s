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

/*	HISTORY
 *
 *  1998/02/25  Umesh Vaishampayan  (umeshv@apple.com)
 *  	Moved kdp_flush_icache() to cache.s
 *
 *	1997/10/14	Umesh Vaishampayan	(umeshv@apple.com)
 *		Added disable_ee() and restore_ee() for the use of mcount.
 *
 *	1997/05/16	Rene Vega -- Cleanup sync/isync usage.
 */

/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

#include <assym.h>
#include <machdep/ppc/asm.h>
#include <machdep/ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <machdep/ppc/powermac.h>

/* Disable external interrupts and return the previous value of msr */
/* These routines are used by mcount() */
ENTRY(disable_ee, TAG_NO_FRAME_USED)
	mfmsr	r3
	rlwinm	r0, r3, 0,  MSR_EE_BIT+1,   MSR_EE_BIT-1
	mtmsr	r0
	blr

/* /Restore the msr to the value passed in the first parameter */
ENTRY(restore_ee, TAG_NO_FRAME_USED)
	mtmsr	r3
	blr

/* Mask and unmask interrupts at the processor level */	
ENTRY(interrupt_disable, TAG_NO_FRAME_USED)
	mfmsr	r0
	rlwinm	r0,	r0,	0,	MSR_EE_BIT+1,	MSR_EE_BIT-1
	mtmsr	r0
	blr

ENTRY(interrupt_enable, TAG_NO_FRAME_USED)
	mfmsr	r0
	ori	r0,	r0,	MASK(MSR_EE)
	mtmsr	r0
	blr

	
/*
**	get_timebase()
**
**	Entry	- R3 contains pointer to 64 bit structure.
**
**	Exit	- 64 bit structure filled in.
**
*/
ENTRY(get_timebase, TAG_NO_FRAME_USED)
loop:
	mftbu	r4
	mftb	r5
	mftbu	r6
	cmpw	r6, r4
	bne	loop

	stw	r4, 0(r3)
	stw	r5, 4(r3)

	blr


/*
 * Boolean	test_and_set (UInt32 theBit, UInt8 *theAddress)
 *{
 *	UInt32		theValue;
 *	UInt32		theMask;
 *	
 *	theAddress += theBit / 8;
 *	theBit %= 8;
 *	
 *	theBit += (theAddress % 4) * 8;
 *	theAddress &= ~(0x00000003);		// long-align the address
 *	
 *	theMask = 1 << (31 - theBit);
 *	
 *	while (true) {
 *		theValue = __lwarx(theAddress);
 *		if ((theValue & theMask) != 0)
 *			return true;
 *		
 *		if (__stwcx(theValue | theMask, theAddress))
 *			return false;
 *	}
 *}
 */
ENTRY(test_and_set, TAG_NO_FRAME_USED)
	
	rlwinm	r7, r3, (32 - 3), 3,31		// r7 = theBit / 8
	rlwinm	r3, r3, 0, 29,31		// theBit %= 8
	add	r4, r4, r7			// theAddress += theBit / 8
	rlwinm	r7, r4, 3, 27,28		// r7 = (theAddress % 4) * 8
	add	r3, r3, r7			// theBit += (theAddress %4)*8
	rlwinm	r4, r4, 0, 0,29			// long-align the address
	li	r7, 1
	nor	r3, r3, r3			// theBit = 31 - theBit
	rlwnm	r7, r7, r3, 0,31		// theMask = 1 << (31 - theBit)
	
	li	r3, 1				// assume it is already set
	
Ltasl:
	sync
	lwarx	r5, 0, r4
	and.	r8, r5, r7			// was the masked bit set?
	bnelr					// it was already set, return

	or	r5, r5, r7			// set the bit
	sync
	stwcx.	r5, 0, r4
	bne-	Ltasl
	isync
	li	r3, 0
	
	blr

