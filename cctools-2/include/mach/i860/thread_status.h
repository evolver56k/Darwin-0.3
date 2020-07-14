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
 * Copyright (c) 1987, 1988 NeXT, Inc.
 */ 

#ifndef	_I860_THREAD_STATE_
#define	_I860_THREAD_STATE_

/*
 * I860_thread_state_regs		this is the structure that is exported
 *					to user threads for use in set/get
 *					status calls.  This structure should
 *					never change.
 */

#define	I860_THREAD_STATE_REGS	(4)	/* normal registers */

struct i860_thread_state_regs {
	int	ireg[31];  /* core registers (incl stack pointer, but not r0) */
	int	freg[30];  /* FPU registers, except f0 and f1 */
	int	psr;	   /* user's processor status register */
	int	epsr;	   /* user's extended processor status register */
	int	db;	   /* user's data breakpoint register */
	int	pc;	   /* user's program counter */
	int	_padding_; /* not used */
	/* Pipeline state for FPU */
	double	Mres3;
	double	Ares3;
	double	Mres2;
	double	Ares2;
	double	Mres1;
	double	Ares1;
	double	Ires1;
	double	Lres3m;
	double	Lres2m;
	double	Lres1m;
	double	KR;
	double	KI;
	double	T;
	int	Fsr3;
	int 	Fsr2;
	int	Fsr1;
	int	Mergelo32;
	int	Mergehi32;
};

#define	I860_THREAD_STATE_REGS_COUNT \
	(sizeof (struct i860_thread_state_regs) / sizeof (int))

#endif	_I860_THREAD_STATE_
