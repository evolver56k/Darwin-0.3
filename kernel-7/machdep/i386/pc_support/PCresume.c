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
 * PC support primatives.
 *
 * HISTORY
 *
 * 22 Mar 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <machdep/i386/sel_inline.h>
#import <machdep/i386/cpu_inline.h>

#import "PCprivate.h"

kern_return_t
PCresume(void)
{
    thread_t			thread = current_thread();
    thread_saved_state_t	*saved_state = USER_REGS(thread);
    PCshared_t			shared = threadPCShared(thread);
    PCcontext_t			context;
    _PCcpuState_t *		cpuState;
    
    if (shared == 0)
    	return (KERN_FAILURE);
	
    cpuState = &shared->cpuState;
    
    if (cpuState->cr0.pe && cpuState->cs.ti != SEL_LDT)
    	return (KERN_FAILURE);
	
    context = currentContext(shared);
	
    context->IF_flag = (cpuState->eflags & EFL_IF) != 0;
    context->EM_bit = cpuState->cr0.em;
    context->X_flags = (cpuState->eflags & (EFL_NT | EFL_IOPL));
	
    PCscheduleTimers(context);
	
    context->running = TRUE;
    
    setts();
	
    saved_state->regs.eax = cpuState->eax;
    saved_state->regs.ebx = cpuState->ebx;
    saved_state->regs.ecx = cpuState->ecx;
    saved_state->regs.edx = cpuState->edx;
    saved_state->regs.edi = cpuState->edi;
    saved_state->regs.esi = cpuState->esi;
    saved_state->regs.ebp = cpuState->ebp;
    saved_state->frame.esp = cpuState->esp;
    saved_state->frame.ss = cpuState->ss;
    saved_state->frame.eflags = cpuState->eflags;
    saved_state->frame.eflags &= ~( EFL_VM | EFL_NT | EFL_IOPL | EFL_CLR );
    saved_state->frame.eflags |=  ( EFL_IF | EFL_SET );
    saved_state->frame.eip = cpuState->eip;
    saved_state->frame.cs = cpuState->cs;
    
    if (cpuState->cr0.pe) {
	saved_state->regs.ds = cpuState->ds;
	saved_state->regs.es = cpuState->es;
	saved_state->regs.fs = cpuState->fs;
	saved_state->regs.gs = cpuState->gs;
    }
    else {    
	saved_state->frame.eflags |= EFL_VM;
    
	saved_state->regs.ds = NULL_SEL;
	saved_state->regs.es = NULL_SEL;
	saved_state->regs.fs = NULL_SEL;
	saved_state->regs.gs = NULL_SEL;
	saved_state->frame.v_ds = sel_to_selector(cpuState->ds);
	saved_state->frame.v_es = sel_to_selector(cpuState->es);
	saved_state->frame.v_fs = sel_to_selector(cpuState->fs);
	saved_state->frame.v_gs = sel_to_selector(cpuState->gs);
    }
    
    thread_exception_return();
    /* NOTREACHED */
}

void
PCcallMonitor(
    thread_t			thread,
    thread_saved_state_t	*saved_state
)
{
    PCshared_t			shared = threadPCShared(thread);
    PCcontext_t			context = currentContext(shared);
    _PCcpuState_t *		cpuState;
    
    cpuState = &shared->cpuState;
    
    cpuState->cr0 = (cr0_t) { 0 };	// XXX

    if (context->EM_bit)
    	cpuState->cr0.em = TRUE;
    
    if (saved_state->frame.eflags & EFL_VM) {
	cpuState->cr0.pe = FALSE;
	cpuState->ds = selector_to_sel(saved_state->frame.v_ds);
	cpuState->es = selector_to_sel(saved_state->frame.v_es);
	cpuState->fs = selector_to_sel(saved_state->frame.v_fs);
	cpuState->gs = selector_to_sel(saved_state->frame.v_gs);
    }
    else {
	cpuState->cr0.pe = TRUE;
	cpuState->ds = saved_state->regs.ds;
	cpuState->es = saved_state->regs.es;
	cpuState->fs = saved_state->regs.fs;
	cpuState->gs = saved_state->regs.gs;
    }
    
    cpuState->eflags = saved_state->frame.eflags;
    if (context->IF_flag)
    	cpuState->eflags |= EFL_IF;
    else
    	cpuState->eflags &= ~EFL_IF;
	
    cpuState->eflags |= (context->X_flags & (EFL_NT | EFL_IOPL));

    cpuState->cs = saved_state->frame.cs;
    cpuState->eip = saved_state->frame.eip;

    cpuState->ss = saved_state->frame.ss;
    cpuState->esp = saved_state->frame.esp;

    cpuState->ebp = saved_state->regs.ebp;
    cpuState->esi = saved_state->regs.esi;
    cpuState->edi = saved_state->regs.edi;
    cpuState->edx = saved_state->regs.edx;
    cpuState->ecx = saved_state->regs.ecx;
    cpuState->ebx = saved_state->regs.ebx;
    cpuState->eax = saved_state->regs.eax;
	
    context->running = FALSE;
    
    setts();
    
    PCdeliverTimers(context);
	
    saved_state->regs.ds = UDATA_SEL;
    saved_state->regs.es = UDATA_SEL;
    saved_state->regs.fs = NULL_SEL;
    saved_state->regs.gs = NULL_SEL;
    
    saved_state->frame.cs = UCODE_SEL;
    saved_state->frame.eip = (unsigned int)shared->callbackHelper;
    
    saved_state->frame.ss = UDATA_SEL;
    saved_state->frame.esp = context->runState.sc_esp;
    saved_state->regs.ebp = 0;
    
    saved_state->frame.eflags &= ~(EFL_VM | EFL_DF | EFL_TF);
    
    thread_exception_return();
    /* NOTREACHED */
}

boolean_t
PCbopFA(
    thread_t			thread,
    thread_saved_state_t	*state,
    PCbopFA_t			*args
)
{
    if (args->cs.ti != SEL_LDT)
    	return (FALSE);
    
    state->frame.eip = args->eip;
    state->frame.cs = args->cs;
    state->frame.eflags = args->eflags;
    state->frame.eflags &= ~( EFL_VM | EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_IF | EFL_SET );
    state->frame.esp = args->esp;
    state->frame.ss = args->ss;

    thread_exception_return();
    /* NOTREACHED*/
}

boolean_t
PCbopFC(
    thread_t			thread,
    thread_saved_state_t	*state,
    PCbopFC_t			*args
)
{
    if (args->cs.ti != SEL_LDT)
    	return (FALSE);
    
    state->frame.eip = args->ip;
    state->frame.cs = args->cs;
    state->frame.eflags = args->flags;
    state->frame.eflags &= ~( EFL_VM | EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_IF | EFL_SET );
    state->frame.esp = args->sp;
    state->frame.ss = args->ss;

    thread_exception_return();
    /* NOTREACHED*/
}

void
PCbopFD(
    thread_t			thread,
    thread_saved_state_t	*state,
    PCbopFD_t			*args
)
{
    state->frame.eip = args->eip;
    state->frame.cs = args->cs;
    state->frame.eflags = args->eflags;
    state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    state->frame.esp = args->esp;
    state->frame.ss = args->ss;
    
    state->regs.ds = NULL_SEL;
    state->regs.es = NULL_SEL;
    state->regs.fs = NULL_SEL;
    state->regs.gs = NULL_SEL;

    thread_exception_return();
    /* NOTREACHED*/
}
