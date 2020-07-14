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
 *
 * HISTORY
 * 15-May-91  Gregg Kellogg (gk) at NeXT
 *	Use m68k_saved_state instead of NeXT_saved_state.
 *	Use m68k_thread_state_regs NeXT_regs.
 *	Use m68k_thread_state_68882 NeXT_thread_state_68882.
 *	Use m68k_thread_state_user_reg NeXT_thread_state_user_reg.
 *	Moved m68k_saved_state and USER_REGS to pcb.h.
 *
 */ 

#ifndef	_MACH_M68K_THREAD_STATUS_
#define	_MACH_M68K_THREAD_STATUS_

/*
 *	m68k_thread_state_regs	this is the structure that is exported
 *				to user threads for use in set/get status
 *				calls.  This structure should never
 *				change.
 *
 *	m68k_thread_state_68882	this structure is exported to user threads
 *				to allow the to set/get 68882 floating
 *				pointer register state.
 *
 *	m68k_saved_state	this structure corresponds to the state
 *				of the user registers as saved on the
 *				stack upon kernel entry.  This structure
 *				is used internally only.  Since this
 *				structure may change from version to
 *				version, it is hidden from the user.
 */

#define	M68K_THREAD_STATE_REGS	(1)	/* normal registers */
#define M68K_THREAD_STATE_68882	(2)	/* 68882 registers */
#define M68K_THREAD_STATE_USER_REG (3)	/* additional user register */

#define M68K_THREAD_STATE_MAXFLAVOR (3)

struct m68k_thread_state_regs {
	int	dreg[8];	/* data registers */
	int	areg[8];	/* address registers (incl stack pointer) */
	short	pad0;		/* not used */
	short	sr;		/* user's status register */
	int	pc;		/* user's program counter */
};

#define	M68K_THREAD_STATE_REGS_COUNT \
	(sizeof (struct m68k_thread_state_regs) / sizeof (int))

struct m68k_thread_state_68882 {
	struct {
		int	fp[3];		/* 96-bit extended format */
	} regs[8];
	int	cr;			/* control */
	int	sr;			/* status */
	int	iar;			/* instruction address */
	int	state;			/* execution state */
};

#define	M68K_THREAD_STATE_68882_COUNT \
	(sizeof (struct m68k_thread_state_68882) / sizeof (int))

struct m68k_thread_state_user_reg {
	int	user_reg;		/* user register (used by cthreads) */
};

#define M68K_THREAD_STATE_USER_REG_COUNT \
	(sizeof (struct m68k_thread_state_user_reg) / sizeof (int))

#endif	_MACH_M68K_THREAD_STATUS_
