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

/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
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
 *	@(#)kern_exit.c	8.10 (Berkeley) 2/23/95
 */
 
#include <machine/reg.h>
#include <machine/psl.h>

#include "compat_43.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <mach/mach_types.h>

void cpu_exit __P((struct proc *));
void exit1 __P((struct proc *, int));

/*
 * exit --
 *	Death of process.
 */
struct exit_args {
	int	rval;
};
void
exit(p, uap, retval)
	struct proc *p;
	struct exit_args *uap;
	int *retval;
{
	exit1(p, W_EXITCODE(uap->rval, 0));
	/*NOTREACHED*/
}

/*
 * Exit: deallocate address space and other resources, change proc state
 * to zombie, and unlink proc from allproc and parent's lists.  Save exit
 * status and rusage for wait().  Check for child processes and orphan them.
 */
void
exit1(p, rv)
	register struct proc *p;
	int rv;
{
	register struct proc *q, *nq;
	thread_t self = current_thread();
	struct task *task = p->task;
	register int i;

	/*
	 * If a thread in this task has already
	 * called exit(), then halt any others
	 * right here.
	 */
	while (p->exit_thread != self) {
		if (sig_try_lock(p) <= 0) {
			if (self->task != task)
				return;
			while (TRUE) {
				thread_halt_self_with_continuation(
							(void (*)(void)) 0);
				/* NOTREACHED */
			}
		}
		sig_lock_to_exit(p);
	}

	/*
	 * Halt the task, except for the current thread.
	 */
	(void) task_halt(task);

	if (p->p_pid == 1) {
		printf("pid 1 exited (signal %d, exit %d)",
		    WTERMSIG(rv), WEXITSTATUS(rv));
		panic("init died");
	}

	/*
	 * Remove proc from allproc queue and from pidhash chain.
	 * Need to do this before we do anything that can block.
	 * Not doing causes things like mount() find this on allproc
	 * in partially cleaned state.
	 */
	LIST_REMOVE(p, p_list);
	LIST_REMOVE(p, p_hash);

#ifdef PGINPROF
	vmsizmon();
#endif
	MALLOC_ZONE(p->p_ru, struct rusage *,
			sizeof *p->p_ru, M_ZOMBIE, M_WAITOK);
	/*
	 * If parent is waiting for us to exit or exec,
	 * P_PPWAIT is set; we will wakeup the parent below.
	 */
	p->p_flag &= ~(P_TRACED | P_PPWAIT);
	p->p_flag |= P_WEXIT;
	p->p_sigignore = ~0;
	p->p_siglist = 0;
	self->_uthread->uu_sig = 0;
	untimeout(realitexpire, (caddr_t)p);

	/*
	 * Close open files and release open-file table.
	 * This may block!
	 */
	fdfree(p);
	
	if (SESS_LEADER(p)) {
		register struct session *sp = p->p_session;

		if (sp->s_ttyvp) {
			/*
			 * Controlling process.
			 * Signal foreground pgrp,
			 * drain controlling terminal
			 * and revoke access to controlling terminal.
			 */
			if (sp->s_ttyp->t_session == sp) {
				if (sp->s_ttyp->t_pgrp)
					pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);
				(void) ttywait(sp->s_ttyp);
				/*
				 * The tty could have been revoked
				 * if we blocked.
				 */
				if (sp->s_ttyvp)
					VOP_REVOKE(sp->s_ttyvp, REVOKEALL);
			}
			if (sp->s_ttyvp)
				vrele(sp->s_ttyvp);
			sp->s_ttyvp = NULL;
			/*
			 * s_ttyp is not zero'd; we use this to indicate
			 * that the session once had a controlling terminal.
			 * (for logging and informational purposes)
			 */
		}
		sp->s_leader = NULL;
	}

	fixjobc(p, p->p_pgrp, 0);
	p->p_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
#if KTRACE
	/* 
	 * release trace file
	 */
	p->p_traceflag = 0;	/* don't trace the vrele() */
	if (p->p_tracep)
		vrele(p->p_tracep);
#endif

	/* Place onto zombproc. */
	LIST_INSERT_HEAD(&zombproc, p, p_list);
	p->p_stat = SZOMB;

	q = p->p_children.lh_first;
	if (q)		/* only need this if any child is S_ZOMB */
		wakeup((caddr_t) initproc);
	for (; q != 0; q = nq) {
		nq = q->p_sibling.le_next;
		proc_reparent(q, initproc);
		/*
		 * Traced processes are killed
		 * since their existence means someone is screwing up.
		 */
		if (q->p_flag & P_TRACED) {
			q->p_flag &= ~P_TRACED;
			if (q->sigwait_thread) {
				/*
				 * The sigwait_thread could be stopped at a
				 * breakpoint. Wake it up to kill.
				 * Need to do this as it could be a thread which is not
				 * the first thread in the task. So any attempts to kill
				 * the process would result into a deadlock on q->sigwait.
				 */
				thread_resume((struct thread *)q->sigwait_thread);
				clear_wait(q->sigwait_thread, THREAD_SHOULD_TERMINATE, FALSE);
				threadsignal(q->sigwait_thread, SIGKILL, 0);
			}
			psignal(q, SIGKILL);
		}
	}

	/*
	 * Save exit status and final rusage info, adding in child rusage
	 * info and self times.
	 */
	p->p_xstat = rv;
	*p->p_ru = p->p_stats->p_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(p->p_ru, &p->p_stats->p_cru);

	/*
	 * Free up profiling buffers.
	 */
	{
		struct uprof *p0 = &p->p_stats->p_prof, *p1, *pn;

		p1 = p0->pr_next;
		p0->pr_next = NULL;
		p0->pr_scale = 0;

		for (; p1 != NULL; p1 = pn) {
			pn = p1->pr_next;
			kfree((vm_offset_t)p1, sizeof *p1);
		}
	}

	/*
	 * Notify parent that we're gone.
	 */
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);

	/*
	 * Notify procfs debugger
	 */
	if (p->p_flag & P_FSTRACE)
		wakeup((caddr_t)p);

	task->proc = NULL;
	/*
	 * Other substructures are freed from wait().
	 */
	FREE_ZONE(p->p_stats, sizeof *p->p_stats, M_SUBPROC);
	p->p_stats = NULL;

	FREE_ZONE(p->p_sigacts, sizeof *p->p_sigacts, M_SUBPROC);
	p->p_sigacts = NULL;

	if (--p->p_limit->p_refcnt == 0)
		FREE_ZONE(p->p_limit, sizeof *p->p_limit, M_SUBPROC);
	p->p_limit = NULL;

	/*
	 * Finish up by terminating the task
	 * and halt this thread (only if a
	 * member of the task exiting).
	 */
	cpu_exit(p);
}

void
cpu_exit(p)
	register struct proc *p;
{
	struct thread *self = current_thread();
	struct task *task = p->task;

	p->task = TASK_NULL;
	task_deallocate(task);

	(void) task_terminate(task);
	while (task == self->task) {
		thread_halt_self_with_continuation((void (*)()) 0);
		/*NOTREACHED*/
	}
}

struct wait4_args {
	int	pid;
	int *status;
	int options;
	struct rusage *rusage;
};

#if COMPAT_43
int
owait(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	struct wait4_args a;

	a.options = 0;
	a.rusage = NULL;
	a.pid = WAIT_ANY;
	a.status = NULL;
	return (wait1(p, &a, retval, 1));
}

int
wait4(p, uap, retval)
	struct proc *p;
	struct wait4_args *uap;
	int *retval;
{
	return (wait1(p, uap, retval, 0));
}

struct owait3_args {
	int *status;
	int options;
	struct rusage *rusage;
};

int
owait3(p, uap, retval)
	struct proc *p;
	struct owait3_args *uap;
	int *retval;
{
	struct wait4_args a;

	a.options = uap->options;
	a.rusage = uap->rusage;
	a.pid = WAIT_ANY;
	a.status = uap->status;

	return (wait1(p, &a, retval, 1));
}

#else
#define	wait1	wait4
#endif

int
wait1(q, uap, retval, compat)
	register struct proc *q;
	register struct wait4_args *uap;
	register_t *retval;
#if COMPAT_43
	int compat;
#endif
{
	register int nfound;
	register struct proc *p, *t;
	int status, error;

	if (uap->pid == 0)
		uap->pid = -q->p_pgid;

loop:
	nfound = 0;
	for (p = q->p_children.lh_first; p != 0; p = p->p_sibling.le_next) {
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid &&
		    p->p_pgid != -(uap->pid))
			continue;
		nfound++;
		if (p->p_stat == SZOMB) {
			retval[0] = p->p_pid;
#if COMPAT_43
			if (compat)
				retval[1] = p->p_xstat;
			else
#endif
			if (uap->status) {
				status = p->p_xstat;	/* convert to int */
				if (error = copyout((caddr_t)&status,
				    (caddr_t)uap->status,
				    sizeof(status)))
					return (error);
			}
			if (uap->rusage &&
			    (error = copyout((caddr_t)p->p_ru,
			    (caddr_t)uap->rusage,
			    sizeof (struct rusage))))
				return (error);
			/*
			 * If we got the child via a ptrace 'attach',
			 * we need to give it back to the old parent.
			 */
			if (p->p_oppid && (t = pfind(p->p_oppid))) {
				p->p_oppid = 0;
				proc_reparent(p, t);
				psignal(t, SIGCHLD);
				wakeup((caddr_t)t);
				return (0);
			}
			p->p_xstat = 0;
			if (p->p_ru) {
				ruadd(&q->p_stats->p_cru, p->p_ru);
				FREE_ZONE(p->p_ru, sizeof *p->p_ru, M_ZOMBIE);
				p->p_ru = NULL;
			} else {
				printf("Warning : lost p_ru for %s\n", p->p_comm);
			}

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(p->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--p->p_cred->p_refcnt == 0) {
				struct ucred *ucr = p->p_ucred;
				struct pcred *pcr;

				if (ucr != NOCRED) {
					p->p_ucred = NOCRED;
					crfree(ucr);
				}
				pcr = p->p_cred;
				p->p_cred = NULL;
				FREE_ZONE(pcr, sizeof *pcr, M_SUBPROC);
			}

			/*
			 * Release reference to text vnode
			 */
			if (p->p_textvp)
				vrele(p->p_textvp);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(p);
			LIST_REMOVE(p, p_list);	/* off zombproc */
			LIST_REMOVE(p, p_sibling);
			FREE_ZONE(p, sizeof *p, M_PROC);
			nprocs--;
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag & P_WAITED) == 0 &&
		    (p->p_flag & P_TRACED || uap->options & WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;
#if COMPAT_43
			if (compat) {
				retval[1] = W_STOPCODE(p->p_xstat);
				error = 0;
			} else
#endif
			if (uap->status) {
				status = W_STOPCODE(p->p_xstat);
				error = copyout((caddr_t)&status,
				    (caddr_t)uap->status,
				    sizeof(status));
			} else
				error = 0;
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (uap->options & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	if (error = tsleep((caddr_t)q, PWAIT | PCATCH, "wait", 0))
		return (error);
	goto loop;
}

/*
 * make process 'parent' the new parent of process 'child'.
 */
void
proc_reparent(child, parent)
	register struct proc *child;
	register struct proc *parent;
{

	if (child->p_pptr == parent)
		return;

	LIST_REMOVE(child, p_sibling);
	LIST_INSERT_HEAD(&parent->p_children, child, p_sibling);
	child->p_pptr = parent;
}

kern_return_t
init_process(void)
/*
 *	Make the current process an "init" process, meaning
 *	that it doesn't have a parent, and that it won't be
 *	gunned down by kill(-1, 0).
 */
{
	register struct proc *p = current_proc();

	if (suser(p->p_ucred, &p->p_acflag))
		return(KERN_NO_ACCESS);

	if (p->p_pid != 1 && p->p_pgid != p->p_pid)
		enterpgrp(p, p->p_pid, 0);
	p->p_flag |= P_SYSTEM;

	/*
	 *	Take us out of the sibling chain, and
	 *	out of our parent's child chain.
	 */
	LIST_REMOVE(p, p_sibling);
	p->p_sibling.le_prev = NULL;
	p->p_sibling.le_next = NULL;
	p->p_pptr = kernproc;

	return(KERN_SUCCESS);
}

void
process_terminate_self(void)
{
	struct thread *self = current_thread();
	struct task *task = self->task;
	struct proc *p = task->proc;

	if (p != NULL) {
		exit1(p, W_EXITCODE(0, SIGKILL));
		/*NOTREACHED*/
	}
}
