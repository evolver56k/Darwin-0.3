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
 * Copyright (c) 1993 NeXT, Inc.
 *
 * HISTORY
 * 23 Aug 93 Mac Gillon at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <mach/exception.h>

#import <sys/param.h>
#import <sys/proc.h>
#import <sys/user.h>

#import <ppc/signal.h>
#import <sys/signalvar.h>
#import <kern/thread.h>
#import <mach/ppc/thread_status.h>
#import <machdep/ppc/proc_reg.h>
#import <machdep/ppc/fpu.h>

#define	C_REDZONE_LEN		224
#define	C_STK_ALIGN			16
#define C_PARAMSAVE_LEN		64
#define	C_LINKAGE_LEN		48
#define TRUNC_DOWN(a,b,c)  (((((unsigned)a)-(b))/(c)) * (c))
#define NBITS (MASK(MSR_POW)|MASK(MSR_ILE)|MASK(MSR_IP)|MASK(MSR_LE))
#define RBITS (MASK(MSR_PR)|MASK(MSR_ME)|MASK(MSR_IR)|MASK(MSR_DR)|MASK(MSR_EE))


/*
 * Arrange for this process to run a signal handler
 */


struct sigregs {
	struct ppc_saved_state ss;
	struct ppc_float_state fs;
};

void
sendsig(p, catcher, sig, mask, code)
 	struct proc *p;
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	struct sigregs *p_regs;
	struct sigcontext context, *p_context;
	struct sigacts *ps = p->p_sigacts;
	int framesize;
	int oonstack;
	unsigned long sp;
	struct ppc_saved_state *statep;
	struct thread *thread;
	unsigned long paramp,linkp;

	thread = current_thread();

	statep = &(thread->pcb->ss);

	oonstack = ps->ps_sigstk.ss_flags & SA_ONSTACK;

	/* figure out where our new stack lives */
	if ((ps->ps_flags & SAS_ALTSTACK) && !oonstack &&
		(ps->ps_sigonstack & sigmask(sig))) {
		sp = (unsigned long)(ps->ps_sigstk.ss_sp);
		ps->ps_sigstk.ss_flags |= SA_ONSTACK;
	}
	else
		sp = statep->r1;

	// preserve RED ZONE area
	sp = TRUNC_DOWN(sp, C_REDZONE_LEN, C_STK_ALIGN);

	// context goes first on stack
	sp -= sizeof(*p_context);
	p_context = (struct sigcontext *) sp;

	// next are the saved registers
	sp -= sizeof(*p_regs);
	p_regs = (struct sigregs *)sp;

	// C calling conventions, create param save and linkage
	// areas

	sp = TRUNC_DOWN(sp, C_PARAMSAVE_LEN, C_STK_ALIGN);
	paramp = sp;
	sp -= C_LINKAGE_LEN;
	linkp = sp;

	/* flush fpu registers to pcb */
	fp_state_save(thread);

	/* fill out sigcontext */
	context.sc_onstack = oonstack;
	context.sc_mask = mask;
	context.sc_ir = statep->srr0;
	context.sc_psw = statep->srr1;
	context.sc_regs = p_regs;

	/* copy info out to user space */
	if (copyout((caddr_t)&context, (caddr_t)p_context, sizeof(context)))
		goto bad;
	if (copyout((caddr_t)statep, (caddr_t)&p_regs->ss, 
			sizeof(struct ppc_saved_state)))
		goto bad;
	if (copyout((caddr_t)&thread->pcb->fs, (caddr_t)&p_regs->fs,
			sizeof(struct ppc_float_state)))
		goto bad;

	/* Place our arguments in arg registers: rtm dependent */

	statep->r3 = (unsigned long)sig;
	statep->r4 = (unsigned long)code;
	statep->r5 = (unsigned long)p_context;

	statep->srr0 = (unsigned long)catcher;
	statep->srr1 = MSR_EXPORT_MASK_SET;
	statep->r1 = sp;

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
	struct sigcontext context;
	struct sigregs *p_regs;
	int error;
	struct thread *thread;

	thread = current_thread();

	if (error = copyin(uap->sigcntxp, &context, sizeof(context))) {
		return(error);
	}
	
	/* adjust the critical fields */
	/* make sure naughty bits are off */
	context.sc_psw &= ~(NBITS);
	/* make sure necessary bits are on */
	context.sc_psw |= (RBITS);

//	/* we return from sigreturns as if we faulted in */
//	entry->es_flags = (entry->es_flags & ~ES_GATEWAY) | ES_TRAP;

	if (context.sc_regs) {
		p_regs = (struct sigregs *)context.sc_regs;
		if (error = copyin(&p_regs->ss, &thread->pcb->ss,
				sizeof(struct ppc_saved_state)))
			return(error);

		if (error = copyin(&p_regs->fs, &thread->pcb->fs,
				sizeof(struct ppc_float_state)))
			return(error);

		}
	else {
		thread->pcb->ss.r1 = context.sc_sp;
	}
//		entry->es_general.saved.stack_pointer = context.sc_sp;

	if (context.sc_onstack & 01)
			p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = context.sc_mask &~ sigcantmask;
	thread->pcb->ss.srr0 = context.sc_ir;
	thread->pcb->ss.srr1 = context.sc_psw;

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

