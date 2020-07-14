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
 * Copyright (c) 1992, 1993 NeXT Computer, Inc.
 *
 * Floating point support.
 *
 * HISTORY
 *
 * 21 June 1993 ? at NeXT
 *	Major rewrite to add special support for SoftPC.
 * 10 September 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <mach/exception.h>

#import <machdep/i386/cpu_inline.h>
#import <machdep/i386/fp_inline.h>
#import <machdep/i386/fp_exported.h>
#import <machdep/i386/configure.h>

#import <fp_emul.h>
#if	FP_EMUL
#import <machdep/i386/table_inline.h>
#import <machdep/i386/desc_inline.h>
#endif

#import <pc_support.h>
#if	PC_SUPPORT
#import <machdep/i386/pc_support/PCprivate.h>
#endif

static thread_t	fp_thread;

static
inline void	fp_init(fp_state_t		*fpstate),
		fp_init_default(fp_state_t	*fpstate),
		fp_unowned(void),
		fp_save(void),
		fp_restore(void);

static void	fp_switch(void);

#import <bsd/i386/reboot.h>
extern int	boothowto;

/*
 * Determine whether this machine
 * contains a hardware FPU and configure
 * the CPU accordingly.
 */

void
fp_configure(void)
{
    cr0_t		_cr0 = cr0();
    volatile
	unsigned short	status_word;

    /*
     * Make sure that floating
     * point operations do not
     * cause a trap.
     */ 
    _cr0.em = _cr0.ts = 0;
    set_cr0(_cr0);

    /*
     * Machines with a hardware
     * FPU will return a status
     * word of zero after an finit,
     * Machines without an FPU will
     * not modify the destination
     * operand.
     */
    status_word = 0x5a5a;

    asm volatile(
    	"fninit;
	fnstsw %0"
	    : "=m" (status_word));
    
    if (status_word != 0 || (boothowto&RB_NOFP) != 0) {
	/*
	 * Either this machine
	 * does not have an FPU,
	 * or we want to test the
	 * floating point emulator.
	 * Turn on the EM bit in CR0
	 * so that all floating point
	 * opcodes cause a trap.
	 */
	_cr0.em = 1;	// cause floating point instructions to trap INT7
	_cr0.mp = 0;	// cause WAIT instructions to trap INT7
	set_cr0(_cr0);
#if	FP_EMUL
    	cpu_config.fpu_type = FPU_EMUL;
#endif
    }
    else {
    	_cr0.ne = 1; // cause unmasked floating point exceptions to trap INT16
	_cr0.mp = 1;
	set_cr0(_cr0);
    	cpu_config.fpu_type = FPU_HDW;
    }
}

/*
 * Called on occurance of INT7 from
 * user mode.
 */

void
fp_noextension(
    thread_saved_state_t	*state
)
{
    if (cpu_config.fpu_type == FPU_NONE) {
	/*
	 * If we are not providing
	 * floating point support,
	 * just send an exception.
	 */
	exception(EXC_EMULATION, EXC_I386_NOEXTENSION, 0);
	/* NOTREACHED */
    }
    
    /*
     * Switch the fpu context
     * if necessary.
     */
    fp_switch();

#if	FP_EMUL
    if (cpu_config.fpu_type ==  FPU_EMUL) {
	/*
	 * If we are using the
	 * kernel floating point
	 * emulator, call it.
	 */
	e80387(state);
    }
#endif
}

/*
 * Called on occurance of INT16 from
 * user mode.
 */

void
fp_extension_fault(
    thread_saved_state_t	*state
)
{
    thread_t		exception_thread = fp_thread;

    /*
     * Stop the floating point unit
     * dead in its tracks.  We do not
     * want another exception to occur.
     */
    fp_save();		// sets fp_thread = THREAD_NULL
    
    if (exception_thread == current_thread()) {
    	exception(EXC_ARITHMETIC, EXC_I386_EXTENSION_FAULT, 0);
	/* NOTREACHED */
    }
    else
    	thread_ast_set(exception_thread, AST_FP_EXTEN);
}

/*
 * Called on occurance of INT16 from
 * kernel mode.
 */
void
fp_kernel_extension_fault(
    thread_saved_state_t	*state
)
{
    thread_t		exception_thread = fp_thread;
    
    /*
     * Stop the floating point unit.
     */
    fp_save();		// sets fp_thread = THREAD_NULL
    
    thread_ast_set(exception_thread, AST_FP_EXTEN);
    if (exception_thread == current_thread())
    	ast_propagate(exception_thread, cpu_number());
}

void
fp_ast(
    thread_t		thread
)
{
    thread_ast_clear(thread, AST_FP_EXTEN);
    
    exception(EXC_ARITHMETIC, EXC_I386_EXTENSION_FAULT, 0);
    /* NOTREACHED */
}    

/*
 * Initialize a saved floating point
 * context as if an FINIT was performed.
 * Used to initialize a context for the
 * software emulator, as well as to 
 * initialize an unused context so that
 * thread_get_state() returns something
 * sensible for a thread that has not
 * performed any floating point operations.
 */

/*
 * Initialize the floating point
 * state to the hardware default.
 */ 
static inline
void
fp_init_default(
    fp_state_t		*fpstate
)
{
    *(unsigned short *)&fpstate->environ.control	= 0x037F;
    *(unsigned short *)&fpstate->environ.status		= 0x0000;
    *(unsigned short *)&fpstate->environ.tag		= 0xFFFF;
    
    fpstate->environ.ip					= 0x00000000;
    fpstate->environ.opcode				= 0x0000;
    fpstate->environ.cs					= NULL_SEL;
    fpstate->environ.dp					= 0x00000000;
    fpstate->environ.ds					= NULL_SEL;
}

/*
 * Initialize the floating point
 * state to that needed by the
 * NEXTSTEP floating point model:
 * double precision, round to
 * nearest or even.
 */
static inline
void
fp_init(
    fp_state_t		*fpstate
)
{
    *(unsigned short *)&fpstate->environ.control	= 0x027F;
    *(unsigned short *)&fpstate->environ.status		= 0x0000;
    *(unsigned short *)&fpstate->environ.tag		= 0xFFFF;
    
    fpstate->environ.ip					= 0x00000000;
    fpstate->environ.opcode				= 0x0000;
    fpstate->environ.cs					= NULL_SEL;
    fpstate->environ.dp					= 0x00000000;
    fpstate->environ.ds					= NULL_SEL;
}

/*
 * Save the floating point context
 * for the thread which owns the FPU
 * and mark it unowned.  N.B. This might
 * not be the current thread.
 */
 
static inline
void
fp_save(void)
{
    thread_t	thread = fp_thread;

    if (thread) {
#if	PC_SUPPORT
	PCshared_t	shared = threadPCShared(thread);
	
	if (shared && shared->fpuOwned) {
	    if (cpu_config.fpu_type == FPU_HDW)
		fnsave(&shared->fpuState);	// clears TS flag
		
	    shared->fpuStateValid = TRUE;
	}
	else {
#endif
	    if (cpu_config.fpu_type == FPU_HDW)
		fnsave(&thread->pcb->fpstate);	// clears TS flag
	
	    thread->pcb->fpvalid = TRUE;
#if	PC_SUPPORT
	}
#endif

	fp_unowned();
    }
}

/*
 * Restore the floating point context of the
 * current thread.  The main assumption here
 * is that the FPU is currently NOT owned by
 * any thread.
 */

static inline
void  
fp_restore(void)
{
    thread_t	thread = current_thread();
#if	PC_SUPPORT
    PCshared_t	shared = threadPCShared(thread);
    
    if (shared) {
    	PCcontext_t	context = currentContext(shared);
	
	if (context->running) {
	    if (shared->fpuStateValid) {
		if (cpu_config.fpu_type == FPU_HDW)
		    frstor(&shared->fpuState);	// clears TS flag
		    
		shared->fpuStateValid = FALSE;
	    }
	    else {
		fp_init_default(&shared->fpuState);

		if (cpu_config.fpu_type == FPU_HDW)
		    frstor(&shared->fpuState);	// clears TS flag
	    }

#if	FP_EMUL
	    if (cpu_config.fpu_type == FPU_EMUL)
		map_data(sel_to_gdt_entry(FPSTATE_SEL),
			    (vm_offset_t) &shared->fpuState
				+ KERNEL_LINEAR_BASE,
			    (vm_size_t) sizeof (shared->fpuState),
					    KERN_PRIV, FALSE);
#endif

	    shared->fpuOwned = TRUE;
	}
	else {
	    shared->fpuOwned = FALSE;
	    goto restore_thread;
	}
    }
    else {
restore_thread:
#endif
	if (thread->pcb->fpvalid) {
	    if (cpu_config.fpu_type == FPU_HDW)
		frstor(&thread->pcb->fpstate);	// clears TS flag
    
	    thread->pcb->fpvalid = FALSE;
	}
	else {
	    fp_init(&thread->pcb->fpstate);
	    
	    if (cpu_config.fpu_type == FPU_HDW)
		frstor(&thread->pcb->fpstate);	// clears TS flag
	}

#if	FP_EMUL
	if (cpu_config.fpu_type == FPU_EMUL)
	    map_data(sel_to_gdt_entry(FPSTATE_SEL),
			(vm_offset_t) &thread->pcb->fpstate
			    + KERNEL_LINEAR_BASE,
			(vm_size_t) sizeof (thread->pcb->fpstate),
					KERN_PRIV, FALSE);
#endif
#if	PC_SUPPORT
    }
#endif
}

/*
 * Restore the floating point context
 * of the current thread, saving the
 * current context if necessary.
 */

static
void
fp_switch(void)
{
    thread_t	thread = current_thread();

    if (thread == fp_thread) {
#if	PC_SUPPORT
    	PCshared_t	shared = threadPCShared(thread);
	
	if (shared) {
	    PCcontext_t		context = currentContext(shared);
	    
	    if (context->running) {
		if (!shared->fpuOwned) {
		    if (cpu_config.fpu_type == FPU_HDW)
			fnsave(&thread->pcb->fpstate); // clears TS flag
		
		    thread->pcb->fpvalid = TRUE;

		    if (shared->fpuStateValid) {
			if (cpu_config.fpu_type == FPU_HDW)
			    frstor(&shared->fpuState);	// clears TS flag
			    
			shared->fpuStateValid = FALSE;
		    }
		    else {
			fp_init_default(&shared->fpuState);
			
			if (cpu_config.fpu_type == FPU_HDW)
			    frstor(&shared->fpuState);	// clears TS flag
		    }

#if	FP_EMUL
		    if (cpu_config.fpu_type == FPU_EMUL)
			map_data(sel_to_gdt_entry(FPSTATE_SEL),
				    (vm_offset_t) &shared->fpuState
					+ KERNEL_LINEAR_BASE,
				    (vm_size_t) sizeof (shared->fpuState),
						    KERN_PRIV, FALSE);
#endif

		    shared->fpuOwned = TRUE;
		}
	    }
	    else {
	    	if (shared->fpuOwned) {
		    if (cpu_config.fpu_type == FPU_HDW)
			fnsave(&shared->fpuState);	// clears TS flag
			
		    shared->fpuStateValid = TRUE;

		    if (thread->pcb->fpvalid) {
			if (cpu_config.fpu_type == FPU_HDW)
			    frstor(&thread->pcb->fpstate);// clears TS flag
		
			thread->pcb->fpvalid = FALSE;
		    }
		    else {
			fp_init(&thread->pcb->fpstate);
			
			if (cpu_config.fpu_type == FPU_HDW)
			    frstor(&thread->pcb->fpstate);// clears TS flag
		    }

#if	FP_EMUL
		    if (cpu_config.fpu_type == FPU_EMUL)
			map_data(sel_to_gdt_entry(FPSTATE_SEL),
				    (vm_offset_t) &thread->pcb->fpstate
					+ KERNEL_LINEAR_BASE,
				    (vm_size_t)
					sizeof (thread->pcb->fpstate),
						    KERN_PRIV, FALSE);
#endif

		    shared->fpuOwned = FALSE;
		}
	    }
	}
#endif
    }
    else {
	fp_save(); fp_thread = thread; fp_restore();
    }

    clts();	// make sure that the TS flag is cleared
}

/*
 * Mark the FPU as currently
 * unowned.
 */

static inline
void
fp_unowned(void)
{
    fp_thread = THREAD_NULL;
    setts();
}

/*
 * Mark the FPU unowned if the
 * thread currently owns it.  The
 * context is discarded.
 */

void
fp_terminate(
    thread_t	thread
)
{
    if (thread == fp_thread)
	fp_unowned();
}

/*
 * Update the saved floating point
 * context of the thread if necessary.
 * Perform a software FINIT if the
 * thread has not performed any floating
 * point operations yet.
 */

void
fp_synch(
    thread_t	thread
)
{
    if (thread == fp_thread)
	fp_save();
    else {
	if (!thread->pcb->fpvalid)
	    fp_init(&thread->pcb->fpstate);
    }
}
