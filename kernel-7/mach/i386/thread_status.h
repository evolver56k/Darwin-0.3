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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	External thread state.
 *
 * HISTORY
 *
 * 5 April 1992 ? at NeXT
 *	Created.
 */

#import <architecture/i386/frame.h>
#import <architecture/i386/fpu.h>

/*
 * Main thread state consists of
 * general registers, segment registers,
 * eip and eflags.
 */

#define i386_THREAD_STATE	-1

typedef struct {
    unsigned int	eax;
    unsigned int	ebx;
    unsigned int	ecx;
    unsigned int	edx;
    unsigned int	edi;
    unsigned int	esi;
    unsigned int	ebp;
    unsigned int	esp;
    unsigned int	ss;
    unsigned int	eflags;
    unsigned int	eip;
    unsigned int	cs;
    unsigned int	ds;
    unsigned int	es;
    unsigned int	fs;
    unsigned int	gs;
} i386_thread_state_t;

#define i386_THREAD_STATE	-1

#define i386_THREAD_STATE_COUNT		\
    ( sizeof (i386_thread_state_t) / sizeof (int) )

/*
 * Default segment register values.
 */
    
#define USER_CODE_SELECTOR	0x000f
#define USER_DATA_SELECTOR	0x0017
#define KERN_CODE_SELECTOR	0x0008
#define KERN_DATA_SELECTOR	0x0010

/*
 * Thread floating point state
 * includes FPU environment as
 * well as the register stack.
 */
 
#define i386_THREAD_FPSTATE	-2

typedef struct {
    fp_env_t		environ;
    fp_stack_t		stack;
} i386_thread_fpstate_t;

#define i386_THREAD_FPSTATE_COUNT 	\
    ( sizeof (i386_thread_fpstate_t) / sizeof (int) )

/*
 * Extra state that may be
 * useful to exception handlers.
 */

#define i386_THREAD_EXCEPTSTATE	-3

typedef struct {
    unsigned int	trapno;
    err_code_t		err;
} i386_thread_exceptstate_t;

#define i386_THREAD_EXCEPTSTATE_COUNT	\
    ( sizeof (i386_thread_exceptstate_t) / sizeof (int) )

/*
 * Per-thread variable used
 * to store 'self' id for cthreads.
 */
 
#define i386_THREAD_CTHREADSTATE	-4
 
typedef struct {
    unsigned int	self;
} i386_thread_cthreadstate_t;

#define i386_THREAD_CTHREADSTATE_COUNT	\
    ( sizeof (i386_thread_cthreadstate_t) / sizeof (int) )

/*
 * Machine-independent way for servers and Mach's exception mechanism to
 * choose the most efficient state flavor for exception RPC's:
 */
#define MACHINE_THREAD_STATE            i386_THREAD_STATE
#define MACHINE_THREAD_STATE_COUNT      i386_THREAD_STATE_COUNT
