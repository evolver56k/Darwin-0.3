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

/*	@(#)ns_timer.c	1.0	12/8/87		(c) 1987 NeXT	*/

/* 
 * Copyright (c) 1992 NeXT, Inc.
 */ 

#import <sys/param.h>
#import <sys/kernel.h>
#import <bsd/sys/callout.h>
#import <kernserv/ns_timer.h>
#import <kernserv/clock_timer.h>
#import <bsd/machine/spl.h>
#import <kern/thread_call.h>

#if defined (__ppc__)
#import "machdep/ppc/longlong.h"
#elif defined (__i386__)
#import "machdep/i386/longlong.h"
#else
#error architecture not supported
#endif

/*
 * Micro-second Timer.
 */

/*
 *  Arrange that the function "proc" gets called in "time" nanoseconds.
 */
void ns_timeout(func proc, void *arg, ns_time_t time, int pri)
{
	int		s;
	tvalspec_t	deadline;
	extern
	    tvalspec_t	tick_stamp;

	s = splhigh();
	llp_div_l(&time, NSEC_PER_SEC, &deadline.tv_nsec);
	deadline.tv_sec = (unsigned int)time;
	ADD_TVALSPEC(&deadline, &tick_stamp);

	thread_call_func_delayed(
			(thread_call_func_t)proc,
			(thread_call_spec_t)arg, deadline);
	splx(s);
}

/*
 *  Arrange that the function "proc" gets called at "deadline" nanoseconds
 *  from boot.
 */
void ns_abstimeout(func proc, void *arg, ns_time_t time, int pri)
{
	int		s;
	tvalspec_t	deadline;

	s = splhigh();
	llp_div_l(&time, NSEC_PER_SEC, &deadline.tv_nsec);
	deadline.tv_sec = (unsigned int)time;

	thread_call_func_delayed(
			(thread_call_func_t)proc,
			(thread_call_spec_t)arg, deadline);
	splx(s);
}

/*
 * Remove something scheduled via ns_timeout or ns_abstimeout.
 */
boolean_t ns_untimeout(func proc, void *arg)
{
	thread_call_func_cancel(
			(thread_call_func_t)proc,
			(thread_call_spec_t)arg, FALSE);
		    
	return (TRUE);	/* XXX cheat */
}

void
ns_sleep(ns_time_t delay)
{
	int		s;
	extern void	wakeup(caddr_t chan);

	s = splhigh();
	ns_timeout((func)wakeup, &delay, delay, CALLOUT_PRI_SOFTINT1);
	/* make this uninterruptable */
	sleep((caddr_t)&delay, PZERO - 1);
	splx(s);
}

/*
 *  Conversion functions.  Loads o' fun.
 */
void
ns_time_to_timeval(ns_time_t nano_time, struct timeval *tvp)
{
	llp_div_l(&nano_time, 1000000000, (unsigned int *)&tvp->tv_usec);
	
	tvp->tv_sec = (unsigned long) nano_time;
	tvp->tv_usec /= 1000;
}

ns_time_t
timeval_to_ns_time(struct timeval *tvp)
{
	ns_time_t	a_time;

	a_time = (ns_time_t) tvp->tv_sec * 1000000ULL + tvp->tv_usec;
	a_time *= 1000ULL;

	return a_time;
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.
 */
void
microtime(struct timeval * tvp)
{
	tvalspec_t	now = clock_get_counter(Calendar);

	tvp->tv_sec = now.tv_sec;
	tvp->tv_usec = now.tv_nsec / NSEC_PER_USEC;
}

/*
 * microboot returns the number of microseconds since boot.
 */
void microboot(struct timeval *tvp)
{
	tvalspec_t	now = clock_get_counter(System);

	tvp->tv_sec = now.tv_sec;
	tvp->tv_usec = now.tv_nsec / NSEC_PER_USEC;
}
