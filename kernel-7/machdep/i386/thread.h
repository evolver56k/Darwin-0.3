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
 * Intel386 Family:	Machine dependent thread state.
 *
 * HISTORY
 *
 * 30 September 1992 ? at NeXT
 *	Moved tss out of pcb.
 *
 * 6 April 1992 ? at NeXT
 *	Created.
 */

#ifndef	_MACHDEP_I386_PCB_H_
#define _MACHDEP_I386_PCB_H_

#import <kern/kernel_stack.h>

#import <architecture/i386/frame.h>
#import <architecture/i386/fpu.h>

#import <machdep/i386/seg.h>
#import <machdep/i386/regs.h>

/*
 * Context information which is common
 * to all threads in a task.  Each
 * thread has its own copy of this
 * information.
 */	
typedef struct pcb_common {
    vm_offset_t		ldt;	/* a LINEAR address */
    vm_size_t		ldt_size;
    struct {
	vm_offset_t		base;
	vm_size_t		length;
    }			io_bmap;
    lock_data_t		lock;
} pcb_common_t;

/*
 * Storage definition for a threads
 * hardware task state segment.
 * Threads which need an IO space
 * bitmap for access to device
 * registers from user mode need
 * an externally allocated TSS.
 */
typedef union {
    tss_t		internal;
    struct tss_alloc {
	vm_offset_t		base;
	vm_size_t		length;
    }			external;
} pcb_tss_t;

/*
 * Thread state is saved in
 * this format whenever an
 * interrupt/trap/system call
 * occurs.
 */
typedef struct thread_saved_state {
    regs_t		regs;
    unsigned int	trapno;
    except_frame_t	frame;
} thread_saved_state_t;

/*
 * A thread uses a private save
 * area as its kernel stack while
 * executing in user mode.  After
 * the user state is saved, it
 * switches to the active kernel
 * stack.  The save area must be
 * large enough to hold the entire
 * user state, plus enough for a
 * kernel mode interrupt in case
 * one occurs before the stack is
 * switched.  In addition, if a
 * call gate gets invoked with
 * the TF (single step) flag set,
 * the debug trap gets taken from
 * kernel mode after the call
 * frame gets saved but before the
 * first instruction of the call
 * gate handler gets executed.  Thus,
 * an extra exception frame is needed
 * for the worst case.
 */
typedef struct thread_save_area {
    except_frame_t		sa_x;
    thread_saved_state_t	sa_k;
    thread_saved_state_t	sa_u;
} thread_save_area_t;

/*
 * Per-thread machine dependent
 * context information.
 */
typedef struct pcb {
    /*
     * i386 Hardware Task State.
     */
    tss_t *		tss;
    vm_size_t		tss_size;
    pcb_tss_t		tss_store;

    /*
     * The private area for saved user
     * state is external to the pcb,
     * and is created lazily.  This
     * avoids wasting the space for
     * kernel mode-only threads.
     */
    thread_save_area_t	*save_area;

    /*
     * Most threads use a common, static
     * LDT.  Some threads have their own,
     * which resides in the task virtual
     * address space.
     */	
    vm_offset_t		ldt;	/* a LINEAR address */
    vm_size_t		ldt_size;
    
    /*
     * Floating point state is
     * identical to the format
     * written by the FSAVE instruction.
     */
    fp_state_t		fpstate;

    /*
     * cthreads 'self' id
     */ 
    unsigned int	cthread_self;
    
    /*
     * Private data structure for
     * PC support.
     */
    struct PCprivate	*PCpriv;

    /*
     * Boolean flags.
     */
    unsigned int
    /* boolean_t */	trace		:1,
    			fpvalid		:1,
			extern_tss	:1,
					:0;
} *pcb_t;

#define current_stack_pointer()		(stack_pointers[cpu_number()])

#define USER_REGS(thread)						\
    ((thread)->pcb->save_area ?						\
	(&(thread)->pcb->save_area->sa_u) :				\
	    (void *)(thread_user_state(thread)))
 
#endif	/* _MACHDEP_I386_PCB_H_ */
