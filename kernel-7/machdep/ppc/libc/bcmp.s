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

/* Copyright (c) 1998 Apple Computer, Inc.  All rights reserved.
 *
 *      File:  machdep/ppc/libc/bcmp.s
 *
 *	int bcmp(const void *b1, const void *b2, size_t len)
 *
 *	DESCRIPTION
 *		The bcmp() function compares byte string b1 against byte string
 *		b2, returning zero if they are identical, non-zero otherwise.
 *		Both strings are assumed to be len bytes long.
 *		Zero-length strings are always identical.
 *
 *		The strings may overlap.
 *
 * HISTORY
 *	23-Sep-1998	Umesh Vaishampayan	(umeshv@apple.com)
 *		Created.
 */

.text
.align 4
.globl _bcmp
_bcmp:
/* Optimized the 6 byte case to improve the networking performance */
	cmpwi   r5, 6
	bne-    L_not_six
	lwz     r6, 0(r3)
	lwz     r7, 0(r4)
	lhz     r8, 4(r3)
	xor     r10, r6, r7
	lhz     r9, 4(r4)
	xor     r11, r8, r9
	or      r3, r10, r11
	blr
	nop						; To align L_not_six on 4 word boundary
	nop						; To align L_not_six on 4 word boundary

L_not_six:
	mr.     r5,r5
	beq-    L_len_zero
L_len_not_zero:
	lbz     r0,0(r3)
	lbz     r9,0(r4)
	addi    r4,r4,1
	cmpw    cr1,r0,r9
	bne-    cr1,L_done
	subi	r5, r5, 1		; addic.  r5,r5,-1 touches xer
	addi    r3,r3,1
	mr.		r5, r5
	bne     L_len_not_zero
L_done:
	mr      r3,r5
	blr

L_len_zero:
	li      r3,0			; Zero-length strings are always identical.
	blr
