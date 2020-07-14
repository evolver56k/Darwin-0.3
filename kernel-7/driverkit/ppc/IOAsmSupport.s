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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 *
 * HISTORY
 *
 * Simon Douglas  22 Oct 97
 * - first checked in. From DriverServices
 */


#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <mach/ppc/vm_param.h>
#include <assym.h>


/*
Boolean _eCompareAndSwap(
	    UInt32 old, UInt32 new, UInt32 * result )
*/


ENTRY(_eCompareAndSwap, TAG_NO_FRAME_USED)

.L_retry:
	lwarx	r6,	0,r5
	cmpw	r6,	r3
	bne-	.L_fail
	sync				/* bug fix for 3.2 processors */
	stwcx.	r4,	0,r5
	bne-	.L_retry
	isync
	li	r3,	1
	blr
.L_fail:
	li	r3,	0
	blr

ENTRY(_eSynchronizeIO, TAG_NO_FRAME_USED)

	li	r0,	0
	eieio
	li	r3,	0
	blr

/*

OSStatus CallTVector_NoRecover(
	    void * p1, void * p2, void * p3, void * p4, void * p5, void * p6,	// r3-8
	    LogicalAddress entry )						// r9

*/

#define PARAM_SIZE	24

#if 0
0x5231204:  lwz     r12,0x000c(r2)
0x5231208:  stw     r2,0x0014(r1)
0x523120c:  lwz     r0,0x0000(r12)
0x5231210:  lwz     r2,0x0004(r12)
0x5231214:  mtspr   ctr,r0
0x5231218:  bctr
#endif

ENTRY(CallTVector_NoRecover, TAG_NO_FRAME_USED)

	mflr	r0
	stw	r0,	FM_LR_SAVE(r1)
	stw	r2,	FM_TOC_SAVE(r1)

	stwu	r1,	-(PARAM_SIZE+FM_SIZE)(r1)

	lwz	r2,	4(r9)
	lwz	r0,	0(r9)
	mtspr	lr,	r0
	mfspr	r12,	lr
	blrl

	addi	r1,	r1,(PARAM_SIZE+FM_SIZE)
	lwz	r2,	FM_TOC_SAVE(r1)
	lwz	r0,	FM_LR_SAVE(r1)
	mtlr	r0
	blr



