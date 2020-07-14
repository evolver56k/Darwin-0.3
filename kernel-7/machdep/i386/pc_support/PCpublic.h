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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PC support public declarations.
 *
 * HISTORY
 *
 * 9 Mar 1993 ? at NeXT
 *	Created.
 */
 
#import <mach/mach_types.h>
#if	!KERNEL
#import <mach/vm_param.h>
#endif

#import <architecture/i386/sel.h>
#import <architecture/i386/cpu.h>
#import <architecture/i386/fpu.h>

#import <bsd/i386/signal.h>

/*
 * PCVERSIONNUMBER is incremented whenever
 * a change is made that is not backwards
 * compatible.  This should never be done
 * after the first release, since it will
 * break the existing libraries.
 *
 * PCRELEASENUMBER is incremented whenever
 * a change is made that is backwards compatible
 * but must be distingished from earlier releases.
 *
 * NEXTSTEP for Intel Processors Release 3.1 is
 * defined to be PCVERSIONNUMBER 15, PCRELEASENUMBER 0.
 */

#define PCVERSIONNUMBER		15
#define PCRELEASENUMBER		1

typedef struct _PCcpuState {
    unsigned int	eax;
    unsigned int	ebx;
    unsigned int	ecx;
    unsigned int	edx;
    unsigned int	edi;
    unsigned int	esi;
    unsigned int	ebp;
    unsigned int	esp;
    sel_t		ss;
    unsigned short		:16;
    unsigned int	eflags;
    unsigned int	eip;
    sel_t		cs;
    unsigned short		:16;
    sel_t		ds;
    unsigned short		:16;
    sel_t		es;
    unsigned short		:16;
    sel_t		fs;
    unsigned short		:16;
    sel_t		gs;
    unsigned short		:16;
    cr0_t		cr0;
} _PCcpuState_t;

typedef struct fp_state _PCfpuState_t;

/*
 * There are a total of PCMAXCONTEXT - 1
 * contexts, since the initial context is
 * not really a context.
 *
 * Each time a new context is created,
 * the following fields are copied from
 * the previous one:
 *	runOptions
 *	timeout
 *	tick
 */

#define PCMAXCONTEXT		8

typedef struct PCcontext {
    // Saved context of call to PCrun()
    struct sigcontext	runState;
    
    // True when running PC code (not in monitor)
    boolean_t		running;

    // Last exception or interrupt info
    unsigned int	trapNum;
    unsigned int	errCode;
    kern_return_t	exceptionResult;
    unsigned int	callHandler;
#define PC_HANDLE_NONE	0
#define PC_HANDLE_FAULT	1
#define PC_HANDLE_INTN	2
#define PC_HANDLE_BOP	3
#define PC_HANDLE_EXIT	4

    // Monitor callback conditions	
    unsigned int	runOptions;
#define PC_RUN_IF_SET		1
#define PC_RUN_TIMEOUT		2
#define PC_RUN_TICK		4
    unsigned int	timeout;
    unsigned int	tick;
    
    // Interrupt flag for instruction emulation
    boolean_t		IF_flag;

    // TRUE means all FP instructions should trap 
    boolean_t		EM_bit;

    // Extra flag bits for which we must simulate set/clear	
    unsigned short	X_flags;
    
    // Mask of pending callbacks, updated synchronously
    unsigned int	pendingCallbacks;
#define PC_CALL_IF_SET	PC_RUN_IF_SET
#define PC_CALL_TIMEOUT	PC_RUN_TIMEOUT
#define PC_CALL_TICK	PC_RUN_TICK
#define PC_CALL_WOKEN	8

    // Mask of timer callbacks to be delivered
    unsigned int	pendingTimers;

    // Mask of timers that are scheduled
    unsigned int	expectedTimers;

    // Debugging features
    unsigned int	debugOptions;
#define PC_DEBUG_PRESERVE_TRACE		1
} *PCcontext_t;

typedef unsigned char	PCfaultMask_t[32/BYTE_SIZE];
typedef unsigned char	PCintNMask_t[256/BYTE_SIZE];
typedef struct {
    vm_offset_t		address;
    vm_size_t		length;
} PCdescTbl_t;

/*
 * Top level structure of the memory
 * region shared by the kernel and the
 * library.  The 'versionNumber' field
 * must come first.
 */

typedef struct PCshared {
    unsigned int	versionNumber;
    PCfaultMask_t	faultMask;
    PCintNMask_t	intNMask;
    void		(*callbackHelper)(void);
    PCfaultMask_t	machMask;
    PCdescTbl_t		IDT;
    PCdescTbl_t		LDT;
    _PCcpuState_t	cpuState;
    int			currentContext;
    struct PCcontext	contexts[PCMAXCONTEXT];
    unsigned int	releaseNumber;
    _PCfpuState_t	fpuState;
    boolean_t
    			fpuOwned	:1,
    			fpuStateValid	:1;
} *PCshared_t;

/*
 * Arguments to mode switch
 * BOPs (passed on stack).
 */

typedef struct {
    unsigned int	eip;
    sel_t		cs;
    unsigned short		:16;
    unsigned int	eflags;
    unsigned int	esp;
    sel_t		ss;
    unsigned short		:16;
} PCbopFD_t, PCbopFA_t;

typedef struct {
    unsigned short	ip;
    sel_t		cs;
    unsigned short	flags;
    unsigned short	sp;
    sel_t		ss;
} PCbopFC_t;
