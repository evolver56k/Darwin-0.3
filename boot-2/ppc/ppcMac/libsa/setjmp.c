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
__asm__("
	.text
	.globl	__setjmp
	.globl	setjmp
setjmp:
__setjmp:
	subi	3,3,4		/* Adjust pointer */
	stwu	1,4(3)
	stwu	2,4(3)
	mflr	0
	stwu	0,4(3)
	stwu	14,4(3)
	stwu	15,4(3)
	stwu	16,4(3)
	stwu	17,4(3)
	stwu	18,4(3)
	stwu	19,4(3)
	stwu	20,4(3)
	stwu	21,4(3)
	stwu	22,4(3)
	stwu	23,4(3)
	stwu	24,4(3)
	stwu	25,4(3)
	stwu	26,4(3)
	stwu	27,4(3)
	stwu	28,4(3)
	stwu	29,4(3)
	stwu	30,4(3)
	stwu	31,4(3)
	stfdu	14,4(3)
	stfdu	15,8(3)
	stfdu	16,8(3)
	stfdu	17,8(3)
	stfdu	18,8(3)
	stfdu	19,8(3)
	stfdu	20,8(3)
	stfdu	21,8(3)
	stfdu	22,8(3)
	stfdu	23,8(3)
	stfdu	24,8(3)
	stfdu	25,8(3)
	stfdu	26,8(3)
	stfdu	27,8(3)
	stfdu	28,8(3)
	stfdu	29,8(3)
	stfdu	30,8(3)
	stfdu	31,8(3)
	li	3,0
	blr
");
