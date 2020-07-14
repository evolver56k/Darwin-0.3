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
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 */

#include <simple_clock.h>
#include <cpus.h>

#include <machine/reg.h>
#include <machine/spl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dkstat.h>
#include <sys/callout.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#ifdef GPROF
#include <sys/gmon.h>
#endif

#include <bsd/machine/cpu.h>

#include <kern/thread.h>
#include <mach/machine.h>
#include <kern/sched.h>
#include <kern/thread_call.h>
#include <mach/time_value.h>
#include <kern/timer.h>
#include <kern/xpr.h>

#include <kern/assert.h>

#include <mach/boolean.h>

/*
 * Clock handling routines.
 *
 * This code is written to operate with two timers which run
 * independently of each other. The main clock, running at hz
 * times per second, is used to do scheduling and timeout calculations.
 * The second timer does resource utilization estimation statistically
 * based on the state of the machine phz times a second. Both functions
 * can be performed by a single clock (ie hz == phz), however the 
 * statistics will be much more prone to errors. Ideally a machine
 * would have separate clocks measuring time spent in user state, system
 * state, interrupt state, and idle state. These clocks would allow a non-
 * approximate measure of resource utilization.
 */
#define BUMPTIME(t, usec) { \
	register volatile struct timeval *tp = (t); \
 \
	tp->tv_usec += (usec); \
	if (tp->tv_usec >= 1000000) { \
		tp->tv_usec -= 1000000; \
		tp->tv_sec++; \
	} \
}

/*
 * The hz hardware interval timer.
 * We update the events relating to real time.
 * If this timer is also being used to gather statistics,
 * we run through the statistics gathering routine as well.
 */

#if	SIMPLE_CLOCK
tvalspec_t	last_hardclock[NCPUS];
#endif	/* SIMPLE_CLOCK */


/*ARGSUSED*/
void
hardclock(pc, ps)
	int ps;
	caddr_t pc;
{
	register struct callout *p1;
	register struct proc *p;
	register int s;
#if	SIMPLE_CLOCK
	tvalspec_t new_hardclock, delta_hardclock;
	unsigned int myticks;
#define	tick	myticks
#endif	/* SIMPLE_CLOCK */

	extern int tickdelta;
	extern long timedelta;
	register thread_t	thread;

	thread = current_thread();

#if	SIMPLE_CLOCK
	/*
	 *	Simple hardware timer does not restart on overflow, hence
	 *	interrupts do not happen at a constant rate.  Must call
	 *	machine-dependent routine to find out how much time has
	 *	elapsed since last interrupt.
	 *
	 *	On NeXT we use SIMPLE_CLOCK because hardclock is called
	 *	at softint0 level, and although time won't drift, there's
	 *	a fair amount of "jitter". --mike
	 */
	new_hardclock = clock_get_counter(System);
	delta_hardclock = new_hardclock;
	SUB_TVALSPEC(&delta_hardclock, &last_hardclock[cpu_number()]);
	myticks = (delta_hardclock.tv_sec * USEC_PER_SEC) +
			(delta_hardclock.tv_nsec / NSEC_PER_USEC);
	last_hardclock[cpu_number()] = new_hardclock;

	/*
	 *	NOTE: tick was #define'd to myticks above.
	 */
#endif	/* SIMPLE_CLOCK */
	
	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	p = current_proc();
	if (USERMODE(ps)) {		
		if (p) {
			if (p->p_stats->p_prof.pr_scale) {
				p->p_flag |= P_OWEUPC;
				ast_on(cpu_number(), AST_UNIX);
			}
		}

		/*
		 * CPU was in user state.  Increment
		 * user time counter, and process process-virtual time
		 * interval timer. 
		 */
		if (timerisset(&p->p_stats->p_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&p->p_stats->p_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(p, SIGVTALRM);
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */
	if (p && !(thread->state & TH_IDLE))
	{		
		if (p->p_limit->pl_rlimit[RLIMIT_CPU].rlim_cur != RLIM_INFINITY) {
		    time_value_t	sys_time, user_time;

		    thread_read_times(thread, &user_time, &sys_time);
		    if ((sys_time.seconds + user_time.seconds + 1) >
		        p->p_limit->pl_rlimit[RLIMIT_CPU].rlim_cur) {
			psignal(p, SIGXCPU);
			if (p->p_limit->pl_rlimit[RLIMIT_CPU].rlim_cur <
			    p->p_limit->pl_rlimit[RLIMIT_CPU].rlim_max)
				p->p_limit->pl_rlimit[RLIMIT_CPU].rlim_cur += 5;
			}
		}
		if (timerisset(&p->p_stats->p_timer[ITIMER_PROF].it_value) &&
		    itimerdecr(&p->p_stats->p_timer[ITIMER_PROF], tick) == 0)
			psignal(p, SIGPROF);
	}

	/*
	 * Increment the time-of-day, and schedule
	 * processing of the callouts at a very low cpu priority,
	 * so we don't keep the relatively high clock interrupt
	 * priority any longer than necessary.
	 */

	/*
	 * Gather the statistics.
	 */
	gatherstats(pc, ps);

#if	NCPUS > 1
	if (cpu_number() != master_cpu)
		return;
#endif	/* NCPUS > 1 */

	if (timedelta != 0) {
		register delta;
		clock_res_t nsdelta = tickdelta * NSEC_PER_USEC;

		if (timedelta < 0) {
			delta = tick - tickdelta;
			timedelta += tickdelta;
			nsdelta = -nsdelta;
		} else {
			delta = tick + tickdelta;
			timedelta -= tickdelta;
		}
		clock_adjust_counter(Calendar, nsdelta);
	}
	microtime(&time);
}

#if	SIMPLE_CLOCK
#undef	tick
#endif	/* SIMPLE_CLOCK */

/*
 * Gather statistics on resource utilization.
 *
 * We make a gross assumption: that the system has been in the
 * state it is in (user state, kernel state, interrupt state,
 * or idle state) for the entire last time interval, and
 * update statistics accordingly.
 */
/*ARGSUSED*/
void
gatherstats(pc, ps)
	caddr_t pc;
	int ps;
{
	register int cpstate, s;

#ifdef GPROF
    struct gmonparam *p = &_gmonparam;
#endif

	/*
	 * Determine what state the cpu is in.
	 */
	if (USERMODE(ps)) {
		/*
		 * CPU was in user state.
		 */
		if (current_proc()->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if ((current_thread()->state & TH_IDLE) && BASEPRI(ps))
			cpstate = CP_IDLE;
#ifdef GPROF
		if (p->state == GMON_PROF_ON) {
			s = pc - p->lowpc;
			if (s < p->textsize) {
				s /= (HISTFRACTION * sizeof(*p->kcount));
				p->kcount[s]++;
			}
		}
#endif
	}
	/*
	 * We maintain statistics shown by user-level statistics
	 * programs:  the amount of time in each cpu state, and
	 * the amount of time each of DK_NDRIVE ``drives'' is busy.
	 */
	cp_time[cpstate]++;
	for (s = 0; s < DK_NDRIVE; s++)
		if (dk_busy & (1 << s))
			dk_time[s]++;
}

/*
 * Arrange that (*fun)(arg) is called in t/hz seconds.
 */
void
timeout(ftn, arg, ticks)
	void (*ftn) __P((void *));
	void *arg;
	register int ticks;
{
	int		s;
	tvalspec_t	deadline;
	extern
	    tvalspec_t	tick_stamp;

	if (ticks <= 0)
		ticks = 1;

	s = splhigh();
	deadline = ticks_to_tvalspec(ticks);
	ADD_TVALSPEC(&deadline, &tick_stamp);

	thread_call_func_delayed(
	    		(thread_call_func_t)ftn,
			(thread_call_spec_t)arg, deadline);
	splx(s);
}

/*
 * untimeout is called to remove a function timeout call
 * from the callout structure.
 */
int
untimeout(ftn, arg)
	void (*ftn) __P((void *));
	void *arg;
{
	thread_call_func_cancel(
			(thread_call_func_t)ftn,
			(thread_call_spec_t)arg, FALSE);

	return TRUE;	/* XXX cheat */
}

/*
 * Compute number of hz until specified time.
 * Used to compute third argument to timeout() from an
 * absolute time.
 */
hzto(tv)
	struct timeval *tv;
{
	register long ticks;
	register long sec;
	int s = splhigh();
	
	/*
	 * If number of milliseconds will fit in 32 bit arithmetic,
	 * then compute number of milliseconds to time and scale to
	 * ticks.  Otherwise just compute number of hz in time, rounding
	 * times greater than representible to maximum value.
	 *
	 * Delta times less than 25 days can be computed ``exactly''.
	 * Maximum value for any timeout in 10ms ticks is 250 days.
	 */
	sec = tv->tv_sec - time.tv_sec;
	if (sec <= 0x7fffffff / 1000 - 1000)
		ticks = ((tv->tv_sec - time.tv_sec) * 1000 +
			(tv->tv_usec - time.tv_usec) / 1000)
				/ (tick / 1000);
	else if (sec <= 0x7fffffff / hz)
		ticks = sec * hz;
	else
		ticks = 0x7fffffff;
	splx(s);
	return (ticks);
}

/*
 * Convert ticks to a timeval
 */
ticks_to_timeval(ticks, tvp)
	register long ticks;
	struct timeval *tvp;
{
	tvp->tv_sec = ticks/hz;
	tvp->tv_usec = (ticks%hz) * tick;
	ASSERT(tvp->tv_usec < 1000000);
}

/*
 * Return information about system clocks.
 */
int
sysctl_clockrate(where, sizep)
	register char *where;
	size_t *sizep;
{
	struct clockinfo clkinfo;

	/*
	 * Construct clockinfo structure.
	 */
	clkinfo.hz = hz;
	clkinfo.tick = tick;
	clkinfo.profhz = hz;
	clkinfo.stathz = hz;
	return (sysctl_rdstruct(where, sizep, NULL, &clkinfo, sizeof(clkinfo)));
}

/*
 * Start profiling on a process.
 *
 * Kernel profiling passes kernel_proc which never exits and hence
 * keeps the profile clock running constantly.
 */
void
startprofclock(p)
	register struct proc *p;
{
	if ((p->p_flag & P_PROFIL) == 0)
		p->p_flag |= P_PROFIL;
}

/*
 * Stop profiling on a process.
 */
void
stopprofclock(p)
	register struct proc *p;
{
	if (p->p_flag & P_PROFIL)
		p->p_flag &= ~P_PROFIL;
}
