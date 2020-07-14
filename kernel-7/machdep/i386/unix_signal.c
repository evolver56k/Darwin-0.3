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
 * Copyright (c) 1992 NeXT, Inc.
 *
 * HISTORY
 * 13 May 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <mach/exception.h>

#import <sys/param.h>
#import <sys/proc.h>
#import <sys/user.h>

#import <i386/psl.h>

#import <machdep/i386/sel_inline.h>

#import <pc_support.h>
#if	PC_SUPPORT
#import <machdep/i386/pc_support/PCmiscInline.h>
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by chmk
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, the frame 
 * pointer, and the argument pointer, it returns
 * to the user specified pc, psl.
 */

void
sendsig(p, catcher, sig, mask, code)
 	struct proc *p;
	sig_t catcher;
	int sig, mask;
	u_long code;
{
    struct sigframe {
	int			sig;
	int			code;
	struct sigcontext *	scp;
    }				frame, *fp;
    struct sigcontext		context, *scp;
	struct sigacts *ps = p->p_sigacts;
    int				oonstack;
    thread_t			thread = current_thread();
    thread_saved_state_t *	saved_state = USER_REGS(thread);
    
	oonstack = ps->ps_sigstk.ss_flags & SA_ONSTACK;
    if ((ps->ps_flags & SAS_ALTSTACK) && !oonstack &&
		(ps->ps_sigonstack & sigmask(sig))) {
	    	scp = (struct sigcontext *)(ps->ps_sigstk.ss_sp - 1);
			ps->ps_sigstk.ss_flags |= SA_ONSTACK;
    } else
	    scp = (struct sigcontext *)saved_state->frame.esp - 1;
    fp = (struct sigframe *)scp - 1;

    /* 
     * Build the argument list for the signal handler.
     */
    frame.sig = sig;
    if (sig == SIGILL || sig == SIGFPE) {
	    frame.code = code;
    } else
	    frame.code = 0;
    frame.scp = scp;
    if (copyout((caddr_t)&frame, (caddr_t)fp, sizeof (frame)))
	goto bad;
#if	PC_SUPPORT
    {
	PCcontext_t	context = threadPCContext(thread);
	
	if (context && context->running) {
	    oonstack |= 02;
	    context->running = FALSE;
	}
    }
#endif
    /*
     * Build the signal context to be used by sigreturn.
     */
    context.sc_onstack = oonstack;
    context.sc_mask = mask;
    context.sc_eax = saved_state->regs.eax;
    context.sc_ebx = saved_state->regs.ebx;
    context.sc_ecx = saved_state->regs.ecx;
    context.sc_edx = saved_state->regs.edx;
    context.sc_edi = saved_state->regs.edi;
    context.sc_esi = saved_state->regs.esi;
    context.sc_ebp = saved_state->regs.ebp;
    context.sc_esp = saved_state->frame.esp;
    context.sc_ss = sel_to_selector(saved_state->frame.ss);
    context.sc_eflags = saved_state->frame.eflags;
    context.sc_eip = saved_state->frame.eip;
    context.sc_cs = sel_to_selector(saved_state->frame.cs);
    if (saved_state->frame.eflags & EFL_VM) {
	context.sc_ds = saved_state->frame.v_ds;
	context.sc_es = saved_state->frame.v_es;
	context.sc_fs = saved_state->frame.v_fs;
	context.sc_gs = saved_state->frame.v_gs;

	saved_state->frame.eflags &= ~EFL_VM;
    }
    else {
	context.sc_ds = sel_to_selector(saved_state->regs.ds);
	context.sc_es = sel_to_selector(saved_state->regs.es);
	context.sc_fs = sel_to_selector(saved_state->regs.fs);
	context.sc_gs = sel_to_selector(saved_state->regs.gs);
    }
    if (copyout((caddr_t)&context, (caddr_t)scp, sizeof (context)))
	goto bad;
    saved_state->frame.eip = (unsigned int)catcher;
    saved_state->frame.cs = UCODE_SEL;

    saved_state->frame.esp = (unsigned int)fp;
    saved_state->frame.ss = UDATA_SEL;

    saved_state->regs.ds = UDATA_SEL;
    saved_state->regs.es = UDATA_SEL;
    saved_state->regs.fs = NULL_SEL;
    saved_state->regs.gs = NULL_SEL;
    return;

bad:
	SIGACTION(p, SIGILL) = SIG_DFL;
	sig = sigmask(SIGILL);
	p->p_sigignore &= ~sig;
	p->p_sigcatch &= ~sig;
	p->p_sigmask &= ~sig;
	psignal(p, SIGILL);
	return;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
struct sigreturn_args {
	struct sigcontext *sigcntxp;
};
/* ARGSUSED */
int
sigreturn(p, uap, retval)
	struct proc *p;
	struct sigreturn_args *uap;
	int *retval;
{
    struct sigcontext		context;
    thread_t			thread = current_thread();
	int error;
    thread_saved_state_t *	saved_state = USER_REGS(thread);	
    
    if (error = copyin((caddr_t)uap->sigcntxp, (caddr_t)&context, 
						sizeof (context)))
					return(error);
    if ((context.sc_eflags & EFL_VM) == 0 &&
	   (!valid_user_code_selector(context.sc_cs) ||
	    !valid_user_data_selector(context.sc_ds) ||
	    !valid_user_data_selector(context.sc_es) ||
	    !valid_user_data_selector(context.sc_fs) ||
	    !valid_user_data_selector(context.sc_gs) ||
	    !valid_user_stack_selector(context.sc_ss))
	)
	return(EINVAL);

    if (context.sc_onstack & 01)
			p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = context.sc_mask &~ sigcantmask;
    saved_state->regs.eax = context.sc_eax;
    saved_state->regs.ebx = context.sc_ebx;
    saved_state->regs.ecx = context.sc_ecx;
    saved_state->regs.edx = context.sc_edx;
    saved_state->regs.edi = context.sc_edi;
    saved_state->regs.esi = context.sc_esi;
    saved_state->regs.ebp = context.sc_ebp;
    saved_state->frame.esp = context.sc_esp;
    saved_state->frame.ss = selector_to_sel(context.sc_ss);
    saved_state->frame.eflags = context.sc_eflags;
    saved_state->frame.eflags &= ~EFL_USERCLR;
    saved_state->frame.eflags |= EFL_USERSET;
    saved_state->frame.eip = context.sc_eip;
    saved_state->frame.cs = selector_to_sel(context.sc_cs);
    if (context.sc_eflags & EFL_VM) {
    	saved_state->regs.ds = NULL_SEL;
    	saved_state->regs.es = NULL_SEL;
    	saved_state->regs.fs = NULL_SEL;
    	saved_state->regs.gs = NULL_SEL;
	saved_state->frame.v_ds = context.sc_ds;
	saved_state->frame.v_es = context.sc_es;
	saved_state->frame.v_fs = context.sc_fs;
	saved_state->frame.v_gs = context.sc_gs;

	saved_state->frame.eflags |= EFL_VM;
    }
    else {
	saved_state->regs.ds = selector_to_sel(context.sc_ds);
	saved_state->regs.es = selector_to_sel(context.sc_es);
	saved_state->regs.fs = selector_to_sel(context.sc_fs);
	saved_state->regs.gs = selector_to_sel(context.sc_gs);
    }
#if	PC_SUPPORT
    if (context.sc_onstack & 02) {
	PCcontext_t	context = threadPCContext(thread);
	
	if (context)
	    context->running = TRUE;
    }
#endif
	return (EJUSTRETURN);
}

/*
 * machine_exception() performs MD translation
 * of a mach exception to a unix signal and code.
 */

boolean_t
machine_exception(
    int		exception,
    int		code,
    int		subcode,
    int		*unix_signal,
    int		*unix_code
)
{
    switch(exception) {

    case EXC_BAD_INSTRUCTION:
	*unix_signal = SIGILL;
	*unix_code = code;
	break;

    case EXC_ARITHMETIC:
	*unix_signal = SIGFPE;
	*unix_code = code;
	break;

    default:
	return(FALSE);
    }
   
    return(TRUE);
}
