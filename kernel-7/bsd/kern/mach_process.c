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

/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*-
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */
/*
 * HISTORY
 *
 *  10-Jun-97  Umesh Vaishampayan (umeshv@apple.com)
 *	Ported to PPC. Cleaned up the architecture specific code.
 */

#import <machine/reg.h>
#import <machine/psl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/sysctl.h>

#include <sys/mount.h>

#import <kern/task.h>
#import <kern/thread.h>
#import <kern/sched_prim.h>

#import <vm/vm_map.h>
#import <mach/vm_param.h>
#import <mach/vm_prot.h>
#import <vm/vm_kern.h>
#if defined(ppc)
#include <machdep/ppc/proc_reg.h>
#endif /* ppc */
/* Macros to clear/set/test flags. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

/*
 * sys-trace system call.
 */
struct ptrace_args {
	int	req;
	pid_t pid;
	caddr_t	addr;
	int	data;
};
int
ptrace(p, uap, retval)
	struct proc *p;
	struct ptrace_args *uap;
	register_t *retval;
{
	struct proc *t = current_proc();	/* target process */
	vm_map_t	victim_map;
	vm_offset_t	start_addr, end_addr,
			kern_addr, offset;
	vm_size_t	size;
	boolean_t	change_protection;
	task_t		task;
	thread_t	thread;
	int		*locr0;
	int error = 0;

	/*
	 *	Intercept and deal with "please trace me" request.
	 */	 
	if (uap->req == PT_TRACE_ME) {
		SET(p->p_flag, P_TRACED);
		/* Non-attached case, our tracer is our parent. */
		t->p_oppid = t->p_pptr->p_pid;
		return(0);
	}

	/*
	 *	Locate victim, and make sure it is traceable.
	 */
	if ((t = pfind(uap->pid)) == NULL)
			return (ESRCH);
		
	task = t->task;
	if (uap->req == PT_ATTACH) {

		/*
		 * You can't attach to a process if:
		 *	(1) it's the process that's doing the attaching,
		 */
		if (t->p_pid == p->p_pid)
			return (EINVAL);

		/*
		 *	(2) it's already being traced, or
		 */
		if (ISSET(t->p_flag, P_TRACED))
			return (EBUSY);

		/*
		 *	(3) it's not owned by you, or is set-id on exec
		 *	    (unless you're root).
		 */
		if ((t->p_cred->p_ruid != p->p_cred->p_ruid ||
			ISSET(t->p_flag, P_SUGID)) &&
		    (error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);

		SET(t->p_flag, P_TRACED);
		t->p_oppid = t->p_pptr->p_pid;
		if (t->p_pptr != p)
			proc_reparent(t, p);
		t->p_xstat = 0;         /* XXX ? */
		/* Halt it dead in its tracks. */
		psignal(t, SIGSTOP);
		return(0);
	}

	/*
	 * You can't do what you want to the process if:
	 *	(1) It's not being traced at all,
	 */
	if (!ISSET(t->p_flag, P_TRACED))
		return (EPERM);

	/*
	 *	(2) it's not being traced by _you_, or
	 */
	if (t->p_pptr != p)
		return (EBUSY);

	/*
	 *	(3) it's not currently stopped.
	 */
	if (task->user_stop_count == 0 || t->p_stat != SSTOP)
		return (EBUSY);

	/*
	 *	Mach version of ptrace executes request directly here,
	 *	thus simplifying the interaction of ptrace and signals.
	 */
	switch (uap->req) {

	case PT_DETACH:
		if (t->p_oppid != t->p_pptr->p_pid) {
			struct proc *pp;

			pp = pfind(t->p_oppid);
			proc_reparent(t, pp ? pp : initproc);
		}

		t->p_oppid = 0;
		CLR(t->p_flag, P_TRACED);
		goto resume;
		
	case PT_KILL:
		/*
		 *	Tell child process to kill itself after it
		 *	is resumed by adding NSIG to p_cursig. [see issig]
		 */
		psignal(t, SIGKILL);
		goto resume;

	case PT_STEP:			/* single step the child */
	case PT_CONTINUE:		/* continue the child */
		thread = (thread_t) queue_first(&task->thread_list);
		locr0 = thread->_uthread->uu_ar0;
		if ((int)uap->addr != 1) {
#if	defined(i386)
			locr0[PC] = (int)uap->addr;
#elif	defined(ppc)
#define ALIGNED(addr,size)	(((unsigned)(addr)&((size)-1))==0)
		if (!ALIGNED((int)uap->addr, sizeof(int)))
			return (ERESTART);

#if defined(ppc)
		thread->pcb->ss.srr0 = (int)uap->addr;
#endif /* ppc */
#undef 	ALIGNED
#else
#error architecture not implemented!
#endif
		} /* (int)uap->addr != 1 */

		if ((unsigned)uap->data < 0 || (unsigned)uap->data >= NSIG)
			goto errorLabel;

		if (uap->data != 0)
			psignal(t, uap->data);


		if (uap->req == PT_STEP) {
#if	defined(i386)
			locr0[PS] |= PSL_T;
#elif 	defined(ppc)
			thread->pcb->ss.srr1 |= MASK(MSR_SE);
#else
#error architecture not implemented!
#endif
		} /* uap->req == PT_STEP */
		else {  /* PT_CONTINUE - clear trace bit if set */
#if defined(i386)
			locr0[PS] &= ~PSL_T;
#elif defined(ppc)
			thread->pcb->ss.srr1 &= ~MASK(MSR_SE);
#endif
		}

	resume:
		t->p_stat = SRUN;
		t->p_xstat = uap->data;
		task_resume(task);
		break;
		
	default:
	errorLabel:
		return(EINVAL);
	}
	return(0);
}

