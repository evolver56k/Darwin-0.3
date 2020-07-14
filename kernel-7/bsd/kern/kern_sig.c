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

/* Copyright (c) 1995-1998 Apple Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_sig.c	8.7 (Berkeley) 4/18/94
 */

#define	SIGPROP		/* include signal properties table */
#include <sys/param.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <sys/syslog.h>
#include <sys/stat.h>

#include <sys/mount.h>

#include <machine/cpu.h>
#include <machine/spl.h>

#include <sys/vm.h>
#include <sys/user.h>		/* for coredump */

void stop __P((struct proc *p));
int cansignal __P((struct proc *, struct pcred *, struct proc *, int));
int killpg1 __P((struct proc *, int, int, int));
void sigexit __P((struct proc *, int));
void setsigvec __P((struct proc *, int, struct sigaction *));
void exit1 __P((struct proc *, int));

/*
 * Can process p, with pcred pc, send the signal signum to process q?
 */
int
cansignal(p, pc, q, signum)
	struct proc *p;
	struct pcred *pc;
	struct proc *q;
	int signum;
{
	if (pc->pc_ucred->cr_uid == 0)
		return (1);		/* root can always signal */

	if (signum == SIGCONT && q->p_session == p->p_session)
		return (1);		/* SIGCONT in session */

	/*
	 * Using kill(), only certain signals can be sent to setugid
	 * child processes
	 */
	if (q->p_flag & P_SUGID) {
		switch (signum) {
		case 0:
		case SIGKILL:
		case SIGINT:
		case SIGTERM:
		case SIGSTOP:
		case SIGTTIN:
		case SIGTTOU:
		case SIGTSTP:
		case SIGHUP:
		case SIGUSR1:
		case SIGUSR2:
			if (pc->p_ruid == q->p_cred->p_ruid ||
			    pc->pc_ucred->cr_uid == q->p_cred->p_ruid ||
			    pc->p_ruid == q->p_ucred->cr_uid ||
			    pc->pc_ucred->cr_uid == q->p_ucred->cr_uid)
				return (1);
		}
		return (0);
	}

	/* XXX
	 * because the P_SUGID test exists, this has extra tests which
	 * could be removed.
	 */
	if (pc->p_ruid == q->p_cred->p_ruid ||
	    pc->p_ruid == q->p_cred->p_svuid ||
	    pc->pc_ucred->cr_uid == q->p_cred->p_ruid ||
	    pc->pc_ucred->cr_uid == q->p_cred->p_svuid ||
	    pc->p_ruid == q->p_ucred->cr_uid ||
	    pc->pc_ucred->cr_uid == q->p_ucred->cr_uid)
		return (1);
	return (0);
}

struct sigaction_args {
	int	signum;
	struct sigaction *nsa;
	struct sigaction *osa;
};
/* ARGSUSED */
int
sigaction(p, uap, retval)
	struct proc *p;
	register struct sigaction_args *uap;
	register_t *retval;
{
	struct sigaction vec;
	register struct sigaction *sa;
	register struct sigacts *ps = p->p_sigacts;
	register int signum;
	int bit, error;

	signum = uap->signum;
	if (signum <= 0 || signum >= NSIG ||
	    signum == SIGKILL || signum == SIGSTOP)
		return (EINVAL);
	sa = &vec;
	if (uap->osa) {
		sa->sa_handler = ps->ps_sigact[signum];
		sa->sa_mask = ps->ps_catchmask[signum];
		bit = sigmask(signum);
		sa->sa_flags = 0;
		if ((ps->ps_sigonstack & bit) != 0)
			sa->sa_flags |= SA_ONSTACK;
		if ((ps->ps_sigintr & bit) == 0)
			sa->sa_flags |= SA_RESTART;
		if (p->p_flag & P_NOCLDSTOP)
			sa->sa_flags |= SA_NOCLDSTOP;
		if (error = copyout((caddr_t)sa, (caddr_t)uap->osa,
		    sizeof (vec)))
			return (error);
	}
	if (uap->nsa) {
		if (error = copyin((caddr_t)uap->nsa, (caddr_t)sa,
		    sizeof (vec)))
			return (error);
		setsigvec(p, signum, sa);
	}
	return (0);
}

void
setsigvec(p, signum, sa)
	register struct proc *p;
	int signum;
	register struct sigaction *sa;
{
	register struct sigacts *ps = p->p_sigacts;
	register int bit;

	bit = sigmask(signum);
	/*
	 * Change setting atomically.
	 */
	(void) splhigh();
	simple_lock(&p->siglock);
	ps->ps_sigact[signum] = sa->sa_handler;
	ps->ps_catchmask[signum] = sa->sa_mask &~ sigcantmask;
	if ((sa->sa_flags & SA_RESTART) == 0)
		ps->ps_sigintr |= bit;
	else
		ps->ps_sigintr &= ~bit;
	if (sa->sa_flags & SA_ONSTACK)
		ps->ps_sigonstack |= bit;
	else
		ps->ps_sigonstack &= ~bit;
	if (sa->sa_flags & SA_USERTRAMP)
		ps->ps_usertramp |= bit;
	else
		ps->ps_usertramp &= ~bit;
	if (signum == SIGCHLD) {
		if (sa->sa_flags & SA_NOCLDSTOP)
			p->p_flag |= P_NOCLDSTOP;
		else
			p->p_flag &= ~P_NOCLDSTOP;
	}
	/*
	 * Set bit in p_sigignore for signals that are set to SIG_IGN,
	 * and for signals set to SIG_DFL where the default is to ignore.
	 * However, don't put SIGCONT in p_sigignore,
	 * as we have to restart the process.
	 */
	if (sa->sa_handler == SIG_IGN ||
	    (sigprop[signum] & SA_IGNORE && sa->sa_handler == SIG_DFL)) {
		p->p_siglist &= ~bit;		/* never to be seen again */
		/*
		 *	If this is a thread signal, clean out the
		 *	threads as well.
		 */
		if (bit & threadmask) {
			register	task_t		task = p->task;
			register	queue_head_t	*list;
			register	thread_t	thread;

			list = &(task->thread_list);
			task_lock(task);
			thread = (thread_t) queue_first(list);
			while (!queue_end(list, (queue_entry_t) thread)) {
				thread->_uthread->uu_sig &= ~bit;
				thread = (thread_t)
					queue_next(&thread->thread_list);
			}
			task_unlock(task);
		}
		if (signum != SIGCONT)
			p->p_sigignore |= bit;	/* easier in psignal */
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sa->sa_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	simple_unlock(&p->siglock);
	(void) spl0();
}

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void
siginit(p)
	struct proc *p;
{
	register int i;

	for (i = 0; i < NSIG; i++)
		if (sigprop[i] & SA_IGNORE && i != SIGCONT)
			p->p_sigignore |= sigmask(i);
}

/*
 * Reset signals for an exec of the specified process.
 */
void
execsigs(p)
	register struct proc *p;
{
	register struct sigacts *ps = p->p_sigacts;
	register int nc, mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs((long)p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_siglist &= ~mask;
		}
		ps->ps_sigact[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	ps->ps_sigstk.ss_flags = SA_DISABLE;
	ps->ps_sigstk.ss_size = 0;
	ps->ps_sigstk.ss_sp = 0;
	ps->ps_flags = 0;
}

/*
 * Manipulate signal mask.
 * Note that we receive new mask, not pointer,
 * and return old mask as return value;
 * the library stub does the rest.
 */
struct sigprocmask_args {
	int	how;
	sigset_t mask;
};
int
sigprocmask(p, uap, retval)
	register struct proc *p;
	struct sigprocmask_args *uap;
	register_t *retval;
{
	int error = 0;

	*retval = p->p_sigmask;
	(void) splhigh();

	switch (uap->how) {
	case SIG_BLOCK:
		p->p_sigmask |= uap->mask &~ sigcantmask;
		break;

	case SIG_UNBLOCK:
		p->p_sigmask &= ~(uap->mask);
		break;

	case SIG_SETMASK:
		p->p_sigmask = uap->mask &~ sigcantmask;
		break;
	
	default:
		error = EINVAL;
		break;
	}
	(void) spl0();
	return (error);
}

/* ARGSUSED */
int
sigpending(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{

	*retval = p->p_siglist;
	return (0);
}

#if COMPAT_43
/*
 * Generalized interface signal handler, 4.3-compatible.
 */
struct osigvec_args {
	int signum;
	struct sigvec *nsv;
	struct sigvec *osv;
};
/* ARGSUSED */
int
osigvec(p, uap, retval)
	struct proc *p;
	register struct osigvec_args *uap;
	register_t *retval;
{
	struct sigvec vec;
	register struct sigacts *ps = p->p_sigacts;
	register struct sigvec *sv;
	register int signum;
	int bit, error;

	signum = uap->signum;
	if (signum <= 0 || signum >= NSIG ||
	    signum == SIGKILL || signum == SIGSTOP)
		return (EINVAL);
	sv = &vec;
	if (uap->osv) {
		*(sig_t *)&sv->sv_handler = ps->ps_sigact[signum];
		sv->sv_mask = ps->ps_catchmask[signum];
		bit = sigmask(signum);
		sv->sv_flags = 0;
		if ((ps->ps_sigonstack & bit) != 0)
			sv->sv_flags |= SV_ONSTACK;
		if ((ps->ps_sigintr & bit) != 0)
			sv->sv_flags |= SV_INTERRUPT;
			if (p->p_flag & P_NOCLDSTOP)
				sv->sv_flags |= SA_NOCLDSTOP;
		if (error = copyout((caddr_t)sv, (caddr_t)uap->osv,
		    sizeof (vec)))
			return (error);
	}
	if (uap->nsv) {
		if (error = copyin((caddr_t)uap->nsv, (caddr_t)sv,
		    sizeof (vec)))
			return (error);
		sv->sv_flags ^= SA_RESTART;	/* opposite of SV_INTERRUPT */
		setsigvec(p, signum, (struct sigaction *)sv);
	}
	return (0);
}

struct osigblock_args {
	int mask;
};
int
osigblock(p, uap, retval)
	register struct proc *p;
	struct osigblock_args *uap;
	register_t *retval;
{

	(void) splhigh();
	*retval = p->p_sigmask;
	p->p_sigmask |= uap->mask &~ sigcantmask;
	(void) spl0();
	return (0);
}

struct osigsetmask_args {
	int mask;
};
int
osigsetmask(p, uap, retval)
	struct proc *p;
	struct osigsetmask_args *uap;
	register_t *retval;
{

	(void) splhigh();
	*retval = p->p_sigmask;
	p->p_sigmask = uap->mask &~ sigcantmask;
	(void) spl0();
	return (0);
}
#endif /* COMPAT_43 */

/*
 * Suspend process until signal, providing mask to be set
 * in the meantime.  Note nonstandard calling convention:
 * libc stub passes mask, not pointer, to save a copyin.
 */
struct sigsuspend_args {
	int mask;
};
/* ARGSUSED */
int
sigsuspend(p, uap, retval)
	register struct proc *p;
	struct sigsuspend_args *uap;
	register_t *retval;
{
	register struct sigacts *ps = p->p_sigacts;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the sigacts structure
	 * to indicate this.
	 */
	ps->ps_oldmask = p->p_sigmask;
	ps->ps_flags |= SAS_OLDMASK;
	p->p_sigmask = uap->mask &~ sigcantmask;
	(void) tsleep((caddr_t) ps, PPAUSE|PCATCH, "pause", 0);
	/* always return EINTR rather than ERESTART... */
	return (EINTR);
}

#if COMPAT_43
struct osigstack_args {
	struct sigstack *nss;
	struct sigstack *oss;
};
/* ARGSUSED */
int
osigstack(p, uap, retval)
	struct proc *p;
	register struct osigstack_args *uap;
	register_t *retval;
{
	struct sigstack ss;
	struct sigacts *psp;
	int error = 0;

	psp = p->p_sigacts;
	ss.ss_sp = psp->ps_sigstk.ss_sp;
	ss.ss_onstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;
	if (uap->oss && (error = copyout((caddr_t)&ss,
	    (caddr_t)uap->oss, sizeof (struct sigstack))))
		return (error);
	if (uap->nss && (error = copyin((caddr_t)uap->nss,
	    (caddr_t)&ss, sizeof (ss))) == 0) {
		psp->ps_sigstk.ss_sp = ss.ss_sp;
		psp->ps_sigstk.ss_size = 0;
		psp->ps_sigstk.ss_flags |= ss.ss_onstack & SA_ONSTACK;
		psp->ps_flags |= SAS_ALTSTACK;
	}
	return (error);
}
#endif /* COMPAT_43 */

struct sigaltstack_args {
	struct sigaltstack *nss;
	struct sigaltstack *oss;
};
/* ARGSUSED */
int
sigaltstack(p, uap, retval)
	struct proc *p;
	register struct sigaltstack_args *uap;
	register_t *retval;
{
	struct sigacts *psp;
	struct sigaltstack ss;
	int error;

	psp = p->p_sigacts;
	if ((psp->ps_flags & SAS_ALTSTACK) == 0)
		psp->ps_sigstk.ss_flags |= SA_DISABLE;
	if (uap->oss && (error = copyout((caddr_t)&psp->ps_sigstk,
	    (caddr_t)uap->oss, sizeof (struct sigaltstack))))
		return (error);
	if (uap->nss == 0)
		return (0);
	if (error = copyin((caddr_t)uap->nss, (caddr_t)&ss,
	    sizeof (ss)))
		return (error);
	if (ss.ss_flags & SA_DISABLE) {
		if (psp->ps_sigstk.ss_flags & SA_ONSTACK)
			return (EINVAL);
		psp->ps_flags &= ~SAS_ALTSTACK;
		psp->ps_sigstk.ss_flags = ss.ss_flags;
		return (0);
	}
	if (ss.ss_size < MINSIGSTKSZ)
		return (ENOMEM);
	psp->ps_flags |= SAS_ALTSTACK;
	psp->ps_sigstk= ss;
	return (0);
}

struct kill_args {
	int pid;
	int signum;
};
/* ARGSUSED */
int
kill(cp, uap, retval)
	register struct proc *cp;
	register struct kill_args *uap;
	register_t *retval;
{
	register struct proc *p;
	register struct pcred *pc = cp->p_cred;

	if ((u_int)uap->signum >= NSIG)
		return (EINVAL);
	if (uap->pid > 0) {
		/* kill single process */
		if ((p = pfind(uap->pid)) == NULL)
			return (ESRCH);
		if (!cansignal(cp, pc, p, uap->signum))
			return (EPERM);
		if (uap->signum)
			psignal(p, uap->signum);
		return (0);
	}
	switch (uap->pid) {
	case -1:		/* broadcast signal */
		return (killpg1(cp, uap->signum, 0, 1));
	case 0:			/* signal own process group */
		return (killpg1(cp, uap->signum, 0, 0));
	default:		/* negative explicit process group */
		return (killpg1(cp, uap->signum, -(uap->pid), 0));
	}
	/* NOTREACHED */
}

#if COMPAT_43
struct okillpg_args {
	int pgid;
	int signum;
};
/* ARGSUSED */
int
okillpg(p, uap, retval)
	struct proc *p;
	register struct okillpg_args *uap;
	register_t *retval;
{

	if ((u_int)uap->signum >= NSIG)
		return (EINVAL);
	return (killpg1(p, uap->signum, uap->pgid, 0));
}
#endif /* COMPAT_43 */

/*
 * Common code for kill process group/broadcast kill.
 * cp is calling process.
 */
int
killpg1(cp, signum, pgid, all)
	register struct proc *cp;
	int signum, pgid, all;
{
	register struct proc *p;
	register struct pcred *pc = cp->p_cred;
	struct pgrp *pgrp;
	int nfound = 0;
	
	if (all)	
		/* 
		 * broadcast 
		 */
		for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM || 
			    p == cp || !cansignal(cp, pc, p, signum))
				continue;
			nfound++;
			if (signum)
				psignal(p, signum);
		}
	else {
		if (pgid == 0)		
			/* 
			 * zero pgid means send to my process group.
			 */
			pgrp = cp->p_pgrp;
		else {
			pgrp = pgfind(pgid);
			if (pgrp == NULL)
				return (ESRCH);
		}
		for (p = pgrp->pg_members.lh_first; p != 0;
		     p = p->p_pglist.le_next) {
			if (p->p_pid <= 1 || p->p_flag & P_SYSTEM ||
			    p->p_stat == SZOMB ||
			    !cansignal(cp, pc, p, signum))
				continue;
			nfound++;
			if (signum)
				psignal(p, signum);
		}
	}
	return (nfound ? 0 : ESRCH);
}

/*
 * Send a signal to a process group.
 */
void
gsignal(pgid, signum)
	int pgid, signum;
{
	struct pgrp *pgrp;

	if (pgid && (pgrp = pgfind(pgid)))
		pgsignal(pgrp, signum, 0);
}

/*
 * Send a signal to a process group.  If checktty is 1,
 * limit to members which have a controlling terminal.
 */
void
pgsignal(pgrp, signum, checkctty)
	struct pgrp *pgrp;
	int signum, checkctty;
{
	register struct proc *p;

	if (pgrp)
		for (p = pgrp->pg_members.lh_first; p != 0;
		     p = p->p_pglist.le_next)
			if (checkctty == 0 || p->p_flag & P_CONTROLT)
				psignal(p, signum);
}

/*
 * Send a signal caused by a trap to a specific thread.
 */
void
threadsignal(thread, signum, code)
	struct thread *thread;
	register int signum;
	u_long code;
{
	register struct uthread *uth = thread->_uthread;
	register struct proc *p = thread->task->proc;
	int mask;

	if ((u_int)signum >= NSIG || signum == 0)
		return;

	mask = sigmask(signum);
	if ((mask & threadmask) == 0)
		return;

	if (!(p->p_flag & P_TRACED) && (p->p_sigignore & mask))
		return;

	simple_lock(&p->siglock);
	uth->uu_sig |= mask;
	uth->uu_code = code;
	simple_unlock(&p->siglock);
}

/*
 * Send the signal to the process.  If the signal has an action, the action
 * is usually performed by the target process rather than the caller; we add
 * the signal to the set of pending signals for the process.
 *
 * Exceptions:
 *   o When a stop signal is sent to a sleeping process that takes the
 *     default action, the process is stopped without awakening it.
 *   o SIGCONT restarts stopped processes (or puts them back to sleep)
 *     regardless of the signal action (eg, blocked or ignored).
 *
 * Other ignored signals are discarded immediately.
 */
void
psignal(p, signum)
	register struct proc *p;
	register int signum;
{
	register int s, prop;
	register sig_t action;
	register thread_t	sig_thread;
	register task_t		sig_task;
	register thread_t	cur_thread;
	int mask;

	if ((u_int)signum >= NSIG || signum == 0)
		panic("psignal signal number");
	mask = sigmask(signum);
	prop = sigprop[signum];

	/*
	 *	We will need the task pointer later.  Grab it now to
	 *	check for a zombie process.  Also don't send signals
	 *	to kernel internal tasks.
	 */
	if (((sig_task = p->task) == TASK_NULL) || sig_task->kernel_vm_space)
		return;

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & P_TRACED)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 * (Note: we don't set SIGCONT in p_sigignore,
		 * and if it is set to SIG_IGN,
		 * action will be SIG_DFL here.)
		 */
		if (p->p_sigignore & mask)
			return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}

	if (p->p_nice > NZERO && action == SIG_DFL && (prop & SA_KILL) &&
		(p->p_flag & P_TRACED) == 0)
		p->p_nice = NZERO;

	if (prop & SA_CONT)
		p->p_siglist &= ~stopsigmask;

	if (prop & SA_STOP) {
		/*
		 * If sending a tty stop signal to a member of an orphaned
		 * process group, discard the signal here if the action
		 * is default; don't stop the process below if sleeping,
		 * and don't clear any pending SIGCONT.
		 */
		if (prop & SA_TTYSTOP && p->p_pgrp->pg_jobc == 0 &&
			action == SIG_DFL)
			return;
		p->p_siglist &= ~contsigmask;
	}
	p->p_siglist |= mask;

	/*
	 * Defer further processing for signals which are held,
	 * except that stopped processes must be continued by SIGCONT.
	 */
	if (action == SIG_HOLD && ((prop & SA_CONT) == 0 || p->p_stat != SSTOP))
		return;
		
	/*
	 *	Deliver the signal to the first thread in the task. This
	 *	allows single threaded applications which use signals to
	 *	be able to be linked with multithreaded libraries.  We have
	 *	an implicit reference to the current_thread, but need
	 *	an explicit one otherwise.  The thread reference keeps
	 *	the corresponding task data structures around too.  This
	 *	reference is released by thread_deallocate_interrupt
	 *	because psignal() can be called from interrupt level).
	 */
	
	s = splhigh();
	
	cur_thread = current_thread();
	
	/*
	 *	This is a mess.  The thread_list_lock is a special
	 *	lock that excludes insert and delete operations
	 *	on the task's thread list for our benefit (can't
	 *	grab task lock because we might be at interrupt
	 *	level).  Check if there are any threads in the
	 *	task.  If there aren't, sending it a signal
	 *	isn't going to work very well, so just return.
	 */
	simple_lock(&sig_task->thread_list_lock);
	if (queue_empty(&sig_task->thread_list)) {
		simple_unlock(&sig_task->thread_list_lock);
		(void) splx(s);
		return;
	}
	sig_thread = (thread_t) queue_first(&sig_task->thread_list);
	if (sig_thread != cur_thread)
		thread_reference(sig_thread);
	simple_unlock(&sig_task->thread_list_lock);

	/*
	 *	SIGKILL priority twiddling moved here from above because
	 *	it needs sig_thread.  Could merge it into large switch
	 *	below if we didn't care about priority for tracing
	 *	as SIGKILL's action is always SIG_DFL.
	 */
	if ((signum == SIGKILL) && (p->p_nice > NZERO)) {
		p->p_nice = NZERO;
		thread_max_priority(sig_thread, sig_thread->processor_set,
			BASEPRI_USER);
		thread_priority(sig_thread, BASEPRI_USER, FALSE);
	}

	/*
	 *	Process is traced - wake it up (if not already
	 *	stopped) so that it can discover the signal in
	 *	issig() and stop for the parent.
	 */
	if (p->p_flag & P_TRACED) {
		if (p->p_stat != SSTOP)
			goto run;
		else
			goto out;
	}

	if (action != SIG_DFL) {
		/*
		 *	User wants to catch the signal.
		 *	Wake up the thread, but don't un-suspend it
		 *	(except for SIGCONT).
		 */
		if (prop & SA_CONT)
			(void) task_resume(sig_task);
		goto run;
	} else {
		/*	Default action - varies */

		if (mask & stopsigmask) {
			/*
			 * These are the signals which by default
			 * stop a process.
			 *
			 * Don't clog system with children of init
			 * stopped from the keyboard.
			 */
			if (!(prop & SA_STOP) && p->p_pptr == initproc) {
				psignal(p, SIGKILL);
				p->p_siglist &= ~mask;
				goto out;
			}
			/*
			 *	Stop the task.
			 */
			if ((sig_thread->state & TH_RUN) == 0) {
				/*
				 *	If task hasn't already been stopped by
				 *	a signal, stop it.
				 */
				p->p_siglist &= ~mask;
				if (sig_task->user_stop_count == 0) {
					/*
					 * p_cursig must not be set, because
					 * it will be psig()'d if it is not
					 * zero, and the signal is being
					 * handled here.  But save the signal
					 * in p_stopsig so WUNTRACED
					 * option to wait can find it.
					 */
					p->p_xstat = signum;
					if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
						psignal(p->p_pptr, SIGCHLD);
					stop(p);
				}
				goto out;
			}
			else {
				if ((p == current_proc()) && (p->p_stat != SZOMB))
					ast_on(cpu_number(), AST_UNIX);
				goto out;
			}
		}

		switch (signum) {
			/*
			 * Signals ignored by default have been dealt
			 * with already, since their bits are on in
			 * p_sigignore.
			 */

		case SIGKILL:
			/*
			 * Kill signal always sets process running and
			 * unsuspends it.
			 */
			while (sig_task->user_stop_count > 0)
				(void) task_resume(sig_task);
			/*
			 *	Process will be running after 'run'
			 */
			p->p_stat = SRUN;

			/*
			 * Break it out of user wait, as well.
			 */
			while (sig_thread->user_stop_count > 0)
				(void) thread_resume(sig_thread);

			/*
			 * Clear system wait if possible.  The
			 * THREAD_SHOULD_TERMINATE is overkill, but
			 * saves us from potentially buggy code elsewhere.
			 */
			clear_wait(sig_thread, THREAD_SHOULD_TERMINATE, FALSE);
#if	MACH_HOST
			/*
			 * Make sure it can run.
			 */
			if (sig_thread->processor_set->empty)
				thread_assign(sig_thread, &default_pset);
#endif
			/*
			 * If we're delivering the signal to some other
			 * thread, that thread might be stuck in an
			 * exception.  Break it out.  Can't call
			 * thread_exception_abort from high spl, but
			 * SIGKILL can't be sent from interrupt level, so
			 * it's ok to drop spl.  Can call thread_deallocate
			 * for same reason.
			 */
			splx(s);
			if (sig_thread != cur_thread) {
				mach_msg_abort_rpc(sig_thread);
				thread_deallocate(sig_thread);
			}
			return;

		case SIGCONT:
			/*
			 * Let the process run.  If it's sleeping on an
			 * event, it remains so.
			 */
			(void) task_resume(sig_task);
			p->p_stat = SRUN;
			goto out;

		default:
			/*
			 * All other signals wake up the process, but don't
			 * resume it.
			 */
			goto run;
		}
	}
	/*NOTREACHED*/
run:
	/*
	 * If we're being traced (possibly because someone attached us
	 * while we were stopped), check for a signal from the debugger.
	 */
    if (p->p_stat == SSTOP) {
        if ((p->p_flag & P_TRACED) != 0 && p->p_xstat != 0)
            p->p_siglist |= sigmask(p->p_xstat); 
    }

	/*
	 * setrunnable(p) in BSD
	 */

	p->p_stat = SRUN;

	/*
	 *	Wake up the thread if it is interruptible.
	 */
	clear_wait(sig_thread, THREAD_INTERRUPTED, TRUE);
out:
	splx(s);
	if (sig_thread != cur_thread)
		thread_deallocate_interrupt(sig_thread);
}

__inline__ void
sig_lock_to_exit(
	struct proc	*p)
{
	thread_t	self = current_thread();

	p->exit_thread = self;
	simple_unlock(&p->siglock);
	(void) task_hold(p->task);
	(void) task_dowait(p->task, FALSE);
}

__inline__ int
sig_try_lock(
	struct proc	*p)
{
	thread_t	self = current_thread();

	simple_lock(&p->siglock);
	while (p->sigwait || p->exit_thread) {
	   	 simple_unlock(&p->siglock);
	   	 if (p->exit_thread) {
			if (p->exit_thread == self) {
				/*
				 * Already exiting - no signals.
				 */
				return 0;
			}
			else {
				/*
				 * Another thread has called exit -
				 * stop (until terminate request).
				 */
				thread_hold(self);
			}
		}
		thread_block_with_continuation((void (*)(void)) 0);
		if (thread_should_halt(self)) {
			/*
			 * Terminate request - clean up.
			 */
			return -1;
		}
		simple_lock(&p->siglock);
	}

	return 1;
}

/*
 * If the current process has received a signal (should be caught or cause
 * termination, should interrupt current syscall), return the signal number.
 * Stop signals with default action are processed immediately, then cleared;
 * they aren't returned.  This is checked after each entry to the system for
 * a syscall or trap (though this can usually be done without calling issignal
 * by checking the pending signal masks in the CURSIG macro.) The normal call
 * sequence is
 *
 *	while (signum = CURSIG(curproc))
 *		postsig(signum);
 */
int
issignal(p)
	register struct proc *p;
{
	register int signum, mask, prop, sigbits;
	task_t task = p->task;
	thread_t initial_thread;
	thread_t cur_thread;
	int	s;

#if DIAGNOSTIC
	/*
	 *	This must be called on master cpu
	 */
	if (cpu_number() != master_cpu)
		panic("issig not on master");
#endif /* DIAGNOSTIC */

	/*
	 * Try to grab the signal lock.
	 */
	if (sig_try_lock(p) <= 0)
		return (0);
	
	/*
	 * only allow delivery of process signals (asynchronous)
	 * to the initial thread. This is the first thread in
	 * the tasks thread list.
	 */
	s = splsched();
	simple_lock(&task->thread_list_lock);
	initial_thread = (thread_t)queue_first(&task->thread_list);
	simple_unlock(&task->thread_list_lock);
	splx(s);
	cur_thread = current_thread();

	for(;;) {
		sigbits = (cur_thread->_uthread->uu_sig |p->p_siglist) & ~p->p_sigmask;

		for (;;) {
			if (p->p_flag & P_PPWAIT)
				sigbits &= ~stopsigmask;
			if (sigbits == 0) {	 	/* no signal to send */
				simple_unlock(&p->siglock);
				return (0);
			}
			signum = ffs((long)sigbits);
			mask = sigmask(signum);
			prop = sigprop[signum];
		
			if (mask & threadmask) {
				/* we can take this signal */
				cur_thread->_uthread->uu_sig &= ~mask;
				break;
			} else {
				if (cur_thread != initial_thread)
					sigbits &= ~mask;
				else
					break;
			}
		}

		/*
		 * We should see pending but ignored signals
		 * only if P_TRACED was on when they were posted.
		 */
		if (mask & p->p_sigignore && (p->p_flag & P_TRACED) == 0) {
			p->p_siglist &= ~mask;		/* take the signal! */
			continue;
		}
		if (p->p_flag & P_TRACED && (p->p_flag & P_PPWAIT) == 0) {
			register int	hold;
			register task_t	task;
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the debugger.
			 */

			if (p->p_flag & P_FSTRACE) {
#if	PROCFS
				/* procfs debugging */
				p->p_stat = SSTOP;
				wakeup((caddr_t)p);
#error need to implement 
				panic("procfs debugging");
#endif
			} else {
				/* ptrace debugging */
				p->p_xstat = signum;
				psignal(p->p_pptr, SIGCHLD);
				pcb_synch(cur_thread);
				/*
				 *	XXX Have to really stop for debuggers;
				 *	XXX stop() doesn't do the right thing.
				 *	XXX Inline the task_suspend because we
				 *	XXX have to diddle Unix state in the
				 *	XXX middle of it.
				 */
				task = p->task;
				hold = FALSE;
				task_lock(task);
				if ((task->user_stop_count)++ == 0)
					hold = TRUE;
				task_unlock(task);
	
				if (hold) {
					(void) task_hold(task);
					thread_reference(cur_thread);
					p->sigwait = TRUE;
					p->sigwait_thread = cur_thread;
					simple_unlock(&p->siglock);
					(void) task_dowait(task, TRUE);
					thread_hold(current_thread());
				}
				else {
					thread_reference(cur_thread);
					p->sigwait = TRUE;
					p->sigwait_thread = cur_thread;
					simple_unlock(&p->siglock);
				}
				p->p_stat = SSTOP;
				p->p_flag &= ~P_WAITED;
				wakeup((caddr_t)p->p_pptr);
				thread_block();
				simple_lock(&p->siglock);
				p->sigwait = FALSE;
				p->sigwait_thread = NULL;
				thread_deallocate(cur_thread);

				/*
				 *	We get here only if task
				 *	is continued or killed.  Kill condition
				 *	is signalled by adding NSIG to p_cursig.
				 *	Pass original p_cursig as exit value in
				 *	this case.
				 */
				if (p->p_siglist & sigmask(SIGKILL)) {
					/*
					 *	Wait event may still be outstanding;
					 *	clear it, since sig_lock_to_exit will
					 *	wait.
					 */
					clear_wait(current_thread(),
						THREAD_INTERRUPTED,
						FALSE);
					sig_lock_to_exit(p);
					/*
					 * Since this thread will be resumed
					 * to allow the current syscall to
					 * be completed, must save u_qsave
					 * before calling exit().  (Since exit()
					 * calls closef() which can trash u_qsave.)
					 */
					exit1(p,signum);
				}

				/*
				 *	We may have to quit
				 */
				if (thread_should_halt(current_thread())) {
					simple_unlock(&p->siglock);
					return(0);
				}

				p->p_siglist &= ~mask;	/* clear the old signal */

				/*
				 * If the traced bit got turned off, go back up
				 * to the top to rescan signals.  This ensures
				 * that p_sig* and ps_sigact are consistent.
				 */
				if ((p->p_flag & P_TRACED) == 0) {
					if (mask & threadmask)
						cur_thread->_uthread->uu_sig |= mask;
					else
						p->p_siglist |= mask;
					continue;
				}

				/*
				 * If parent wants us to take the signal,
				 * then it will leave it in p->p_xstat;
				 * otherwise we just look for signals again.
				 */
				signum = p->p_xstat;
				if (signum == 0)
					continue;
				/*
				 * Put the new signal into p_siglist.  If the
				 * signal is being masked, look for other signals.
				 */
				mask = sigmask(signum);
				if (mask & threadmask)
					cur_thread->_uthread->uu_sig |= mask;
				else
					p->p_siglist |= mask;
				if (p->p_sigmask & mask)
					continue;
			}
		}

		/*
		 * Decide whether the signal should be returned.
		 * Return the signal's number, or fall through
		 * to clear it from the pending mask.
		 */

		switch ((long)p->p_sigacts->ps_sigact[signum]) {
		
		case (long)SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_pptr->p_pid == 0) {
#if DIAGNOSTIC
				/*
				 * Are you sure you want to ignore SIGSEGV
				 * in init? XXX
				 */
				printf("Process (pid %d) got signal %d\n",
					p->p_pid, signum);
#endif
				break; 				/* == ignore */
			}
			
			/*
			 * If there is a pending stop signal to process
			 * with default action, stop here,
			 * then clear the signal.  However,
			 * if process is member of an orphaned
			 * process group, ignore tty stop signals.
			 */
			if (prop & SA_STOP) {
				if (p->p_flag & P_TRACED ||
		    		    (p->p_pgrp->pg_jobc == 0 &&
				    prop & SA_TTYSTOP))
					break;	/* == ignore */
				p->p_xstat = signum;
				stop(p);
				if ((p->p_pptr->p_flag & P_NOCLDSTOP) == 0)
					psignal(p->p_pptr, SIGCHLD);
				thread_reference(cur_thread);
				p->sigwait = TRUE;
				p->sigwait_thread = cur_thread;
				simple_unlock(&p->siglock);
				thread_block();
				simple_lock(&p->siglock);
				p->sigwait = FALSE;
				p->sigwait_thread = NULL;
				thread_deallocate(cur_thread);
				/*
				 *	We may have to quit
				 */
				if (thread_should_halt(current_thread())) {
					simple_unlock(&p->siglock);
					return(0);
				}
				break;
			} else if (prop & SA_IGNORE) {
				/*
				 * Except for SIGCONT, shouldn't get here.
				 * Default action is to ignore; drop it.
				 */
				break;		/* == ignore */
			} else {
				simple_unlock(&p->siglock);
				return (signum);
			}
			/*NOTREACHED*/

		case (long)SIG_IGN:
			/*
			 * Masking above should prevent us ever trying
			 * to take action on an ignored signal other
			 * than SIGCONT, unless process is traced.
			 */
			if ((prop & SA_CONT) == 0 &&
			    (p->p_flag & P_TRACED) == 0)
				printf("issignal\n");
			break;		/* == ignore */

		default:
			/*
			 * This signal has an action, let
			 * postsig() process it.
			 */
			simple_unlock(&p->siglock);
			return (signum);
		}
		p->p_siglist &= ~mask;		/* take the signal! */
		}
	/* NOTREACHED */
}

/*
 * Put the argument process into the stopped state and notify the parent
 * via wakeup.  Signals are handled elsewhere.  The process must not be
 * on the run queue.
 */
void
stop(p)
	register struct proc *p;
{

	/*
	 *	Call special task_suspend routine,
	 *	because this routine is called from interrupts
	 *	(psignal) and cannot sleep.
	 */
	(void) task_suspend_nowait(p->task);	/*XXX*/

	p->p_stat = SSTOP;
	p->p_flag &= ~P_WAITED;
	wakeup((caddr_t)p->p_pptr);
}

/*
 * Take the action for the specified signal
 * from the current set of pending signals.
 */
void
postsig(signum)
	register int signum;
{
	register struct proc *p = current_proc();
	register struct sigacts *ps = p->p_sigacts;
	register sig_t action;
	u_long code;
	int mask, returnmask;

#if DIAGNOSTIC
	if (signum == 0)
		panic("postsig");
	/*
	 *	This must be called on master cpu
	 */
	if (cpu_number() != master_cpu)
		panic("psig not on master");
#endif

	/*
	 * Try to grab the signal lock.
	 */
	if (sig_try_lock(p) <= 0)
		return;

	mask = sigmask(signum);
	p->p_siglist &= ~mask;
	action = ps->ps_sigact[signum];
#if KTRACE
	if (KTRPOINT(p, KTR_PSIG))
		ktrpsig(p->p_tracep,
		    signum, action, ps->ps_flags & SAS_OLDMASK ?
		    ps->ps_oldmask : p->p_sigmask, 0);
#endif
	if (action == SIG_DFL) {
		/*
		 * Default action, where the default is to kill
		 * the process.  (Other cases were ignored above.)
		 */
		 /* called with sig_lock() held */
		sigexit(p, signum);
		/* NOTREACHED */
	} else {
		/*
		 * If we get here, the signal must be caught.
		 */
#if DIAGNOSTIC
		if (action == SIG_IGN || (p->p_sigmask & mask))
		    log(LOG_WARNING,
		        "postsig: processing masked or ignored signal\n");
#endif
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal.
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want
		 * restored after the signal processing is completed.
		 */
		(void) splhigh();
		if (ps->ps_flags & SAS_OLDMASK) {
			returnmask = ps->ps_oldmask;
			ps->ps_flags &= ~SAS_OLDMASK;
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= ps->ps_catchmask[signum] | mask;
		if (ps->ps_sig != signum) {
			code = 0;
		} else {
			code = ps->ps_code;
			ps->ps_code = 0;
		}
		simple_unlock(&p->siglock);
		(void) spl0();
		p->p_stats->p_ru.ru_nsignals++;
		sendsig(p, action, signum, returnmask, code);
	}
}

/*
 * Force the current process to exit with the specified signal, dumping core
 * if appropriate.  We bypass the normal tests for masked and caught signals,
 * allowing unrecoverable failures to terminate the process without changing
 * signal state.  Mark the accounting record with the signal termination.
 * If dumping core, save the signal number for the debugger.  Calls exit and
 * does not return.
 */
void
sigexit(p, signum)
	register struct proc *p;
	int signum;
{

	sig_lock_to_exit(p);
	p->p_acflag |= AXSIG;
	if (sigprop[signum] & SA_CORE) {
		p->p_sigacts->ps_sig = signum;
		if (coredump(p) == 0)
			signum |= WCOREFLAG;
	}
	exit1(p, W_EXITCODE(0, signum));
	/* NOTREACHED */
}

