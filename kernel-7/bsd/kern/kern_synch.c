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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#import <cputypes.h>
#import <cpus.h>

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <sys/file.h>
#import <sys/vnode.h>
#import <sys/kernel.h>
#import <sys/buf.h>

#import <machine/spl.h>

#import <kern/ast.h>
#import <sys/callout.h>
#import <kern/queue.h>
#import <kern/lock.h>
#import <kern/thread.h>
#import <kern/sched.h>
#import <kern/sched_prim.h>
#import <mach/machine.h>
#import <kern/parallel.h>
#import <kern/processor.h>

#import <machine/cpu.h>
#import <vm/pmap.h>
#import <vm/vm_kern.h>

#import <kern/task.h>
#import <mach/time_value.h>

/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * If pri&PCATCH is set, signals will cause sleep
 * to return 1, rather than longjmp.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 */

static __inline__
int _sleep(chan, pri, wmsg, timo)
	caddr_t chan;
	int pri;
	char *wmsg;
	int timo;
{
	register struct proc *p;
	register thread_t thread = current_thread();
	int sig, catch = pri & PCATCH;
	int error = 0;
	spl_t	s;

	s = splhigh();

	p = current_proc();
#if KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p->p_tracep, 1, 0);
#endif	
	p->p_priority = pri & PRIMASK;
		
	if (chan)
		assert_wait(chan, (catch ? TRUE : FALSE));
		
	if (timo)
		thread_set_timeout(timo);
	/*
	 * We start our timeout
	 * before calling CURSIG, as we could stop there, and a wakeup
	 * or a SIGCONT (or both) could occur while we were stopped.
	 * A SIGCONT would cause us to be marked as SSLEEP
	 * without resuming us, thus we must be ready for sleep
	 * when CURSIG is called.  If the wakeup happens while we're
	 * stopped, p->p_wchan will be 0 upon return from CURSIG.
	 */

	if (catch) {
		unix_master();
		if (SHOULDissignal(p,thread->_uthread)) {
			if (sig = CURSIG(p)) {
				clear_wait(thread, THREAD_INTERRUPTED, TRUE);
				if (p->p_sigacts->ps_sigintr & sigmask(sig))
					error = EINTR;
				else
					error = ERESTART;
				unix_release();
				goto out;
			}
		}
		if (thread_should_halt(thread)) {
			clear_wait(thread, THREAD_SHOULD_TERMINATE, TRUE);
			error = EINTR;
			unix_release();
			goto out;
		}
		if (thread->wait_event == 0) {  // already happened
			unix_release();
			goto out;
		}
		unix_release();
	}

	thread->wait_mesg = wmsg;
	(void) spl0();
	p->p_stats->p_ru.ru_nvcsw++;

	thread_block();

	thread->wait_mesg = NULL;
	switch (thread->wait_result) {
		case THREAD_TIMED_OUT:
			error = EWOULDBLOCK;
			break;
		case THREAD_AWAKENED:
			/*
			 * Posix implies any signal should be delivered
			 * first, regardless of whether awakened due
			 * to receiving event.
			 */
			if (!catch)
				break;
			/* else fall through */
		case THREAD_INTERRUPTED:
		case THREAD_SHOULD_TERMINATE:
			if (catch) {
				unix_master();
				if (thread_should_halt(thread)) {
					error = EINTR;
				} else if (SHOULDissignal(p,thread->_uthread)) {
					if (sig = CURSIG(p)) {
						if (p->p_sigacts->ps_sigintr & sigmask(sig))
							error = EINTR;
						else
							error = ERESTART;
					}
					if (thread_should_halt(thread)) {
						error = EINTR;
					}
				}
				unix_release();
			} 
			break;
	}
out:
	(void) splx(s);
	return (error);
}

int sleep(chan, pri)
	void *chan;
	int pri;
{

	return (_sleep((caddr_t)chan, pri, (char *)NULL, 0));
	
}

int	tsleep(chan, pri, wmsg, timo)
	void *chan;
	int pri;
	char * wmsg;
	int	timo;
{			
	return(_sleep((caddr_t)chan, pri, wmsg, timo));
}

/*
 * Wake up all processes sleeping on chan.
 */
void
wakeup(chan)
	register void *chan;
{
	int s;

	s = splhigh();
	thread_wakeup((caddr_t)chan);
	splx(s);
}

/*
 * Wake up the first process sleeping on chan.
 *
 * Be very sure that the first process is really
 * the right one to wakeup.
 */
wakeup_one(chan)
	register caddr_t chan;
{
	int s;

	s = splhigh();
	thread_wakeup_one(chan);
	splx(s);
}

/*
 * Compute the priority of a process when running in user mode.
 * Arrange to reschedule if the resulting priority is better
 * than that of the current process.
 */
void
resetpriority(p)
	register struct proc *p;
{
	int newpri;

	if (p->p_nice < 0)
	    newpri = BASEPRI_USER +
		(p->p_nice * (MAXPRI_USER - BASEPRI_USER)) / PRIO_MIN;
	else
	    newpri = BASEPRI_USER -
		(p->p_nice * BASEPRI_USER) / PRIO_MAX;

	(void)task_priority(p->task, newpri, TRUE);
}

#if	NCPUS > 1

slave_start()
{
	register struct thread	*th;
	register int		mycpu;

	/*	Find a thread to execute */

	mycpu = cpu_number();

	splhigh();
	th = choose_thread(current_processor());
	if (th == NULL) {
		printf("Slave %d failed to find any threads.\n", mycpu);
		printf("Should have at least found idle thread.\n");
		halt_cpu();
	}

	/*
	 *	Show that this cpu is using the kernel pmap
	 */
	PMAP_ACTIVATE(kernel_pmap, th, mycpu);

	active_threads[mycpu] = th;

	if (th->task->kernel_vm_space == FALSE) {
		PMAP_ACTIVATE(vm_map_pmap(th->task->map), th, mycpu);
	}

	/*
	 *	Clock interrupt requires that this cpu have an active
	 *	thread, hence it can't be done before this.
	 */
#if	NeXT
#else	NeXT
	startrtclock();
#endif	/* NeXT */
	ast_context(th, mycpu);
	load_context(th);
	/*NOTREACHED*/
}
#endif	/* NCPUS > 1 */
