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
 *  Copyright (c) 1994 by Sun Microsystems, Inc
 */

#ifndef	_MACH_SPARC_THREAD_STATUS_H_
#define	_MACH_SPARC_THREAD_STATUS_H_

#include <architecture/sparc/reg.h>

/*
 *	sparc_thread_state_regs
 *		This is the structure that is exported
 *      to user threads for use in set/get status
 *      calls.  This structure should never change.
 *		The "local" and "in" registers of the corresponding 
 *		register window	are saved in the stack frame pointed
 *		to by sp -> %o6.
 *
 *	sparc_thread_state_fpu
 *		This is the structure that is exported
 *      to user threads for use in set/get FPU register 
 *		status calls.
 *
 */

#define	SPARC_THREAD_STATE_REGS	1

struct sparc_thread_state_regs {
	struct regs regs;
};

#define	SPARC_THREAD_STATE_REGS_COUNT \
			(sizeof(struct sparc_thread_state_regs) / sizeof(int))

/*
 *	Floating point unit registers
 */

#define SPARC_THREAD_STATE_FPU	2


struct sparc_thread_state_fpu {
	struct fpu fpu;	/* floating point registers/status */
};

#define	SPARC_THREAD_STATE_FPU_COUNT \
			(sizeof(struct sparc_thread_state_fpu) / sizeof(int))

#define	SPARC_THREAD_STATE_FLAVOR_COUNT  2

#define SPARC_THREAD_STATE_FLAVOR_LIST_COUNT         \
	( SPARC_THREAD_STATE_FLAVOR_COUNT *              \
		(sizeof (struct thread_state_flavor) / sizeof(int)))

#endif	_MACH_SPARC_THREAD_STATUS_H_
