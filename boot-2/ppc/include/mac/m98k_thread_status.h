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
 * Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	mach/m98k/thread_status.h
 *	Author:	Mike DeMoney, NeXT Computer, Inc.
 *
 *	This include file defines the per-thread state
 *	for NeXT 98K-based products.
 *
 * HISTORY
 *  5-Nov-92  Ben Fathi (benf@next.com)
 *	Ported to m98k.
 *
 * 23-Jan-91  Mike DeMoney (mike@next.com)
 *	Created.
 */

#ifndef	_MACH_M98K_THREAD_STATUS_
#define	_MACH_M98K_THREAD_STATUS_

/*
 * m98k_thread_state_grf -- basic thread state for NeXT 98K-based products
 */
typedef struct _m98k_thread_state_grf {
	unsigned	r0;		// zt (not for mem ref): caller-saved
	unsigned	r1;		// sp (stack pointer): callee-saved
	unsigned	r2;		// toc (tbl of contents): callee saved
	unsigned	r3;		// a0 (arg 0, retval 0): caller saved
	unsigned	r4;		// a1
	unsigned	r5;		// a2
	unsigned	r6;		// a3
	unsigned	r7;		// a4
	unsigned	r8;		// a5
	unsigned	r9;		// a6
	unsigned	r10;		// a7
	unsigned	r11;		// ep (environment ptr): caller saved
	unsigned	r12;		// at (assembler temp): caller saved
	unsigned	r13;		// s17: callee saved
	unsigned	r14;		// s16
	unsigned	r15;		// s15
	unsigned	r16;		// s14
	unsigned	r17;		// s13
	unsigned	r18;		// s12
	unsigned	r19;		// s11
	unsigned	r20;		// s10
	unsigned	r21;		// s9
	unsigned	r22;		// s8
	unsigned	r23;		// s7
	unsigned	r24;		// s6
	unsigned	r25;		// s5
	unsigned	r26;		// s4
	unsigned	r27;		// s3
	unsigned	r28;		// s2
	unsigned	r29;		// s1
	unsigned	r30;		// s0
	unsigned	r31;		// fp (frame pointer): callee saved
	unsigned	lr;		// link register
	unsigned	ctr;		// count register
	unsigned	cr;		// condition register
	unsigned	xer;		// fixed point exception register
	unsigned	pmr;		// program mode register
	unsigned	cia;		// current instruction address
} m98k_thread_state_grf_t;

#define	M98K_THREAD_STATE_GRF_COUNT 	\
	(sizeof(m98k_thread_state_grf_t)/sizeof(int))

#endif	_MACH_M98K_THREAD_STATUS_
