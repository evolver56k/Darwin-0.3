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

#ifndef _TIMER_
#define _TIMER_

#include <mach/boolean.h>

#ifdef WIN32
#include <winnt-pdo.h>
#include <sys/types.h>
#include <winsock.h>
#else
#include <sys/time.h>
#endif WIN32

#include "mem.h"

/*
 * TIMEVAL_CMP
 *	Compare two timeval structures.
 *	Result is <, ==, or > 0, according to whether t1 is <, ==, or > t2.
 */
#define TIMEVAL_CMP(t1, t2)		\
	((t1).tv_sec == (t2).tv_sec ? (t1).tv_usec - (t2).tv_usec : (t1).tv_sec - (t2).tv_sec)

/*
 * Timer structure.
 */
typedef struct timer {
	struct timer	*link;		/* used by queue package */
	struct timeval	interval;	/* timer interval */
	void		(*action)();	/* action on timer expiration */
	char 		*info;		/* arbitrary client information */
	struct timeval	deadline;	/* used by timer package */
} *nmtimer_t;

#define TIMER_NULL	((nmtimer_t)0)


/*
 * TIMER_WAKE_UP
 *	wakes up the timer thread immediately.
 *
 */
extern void timer_wake_up();
/*
*/


/*
 * TIMER_START:
 *	If timer t is not already on the timer queue
 *	then the absolute deadline of t is computed and t is inserted in the timer queue.
 *
 *	Assumes that the timer is not on the timer queue - timer_restart should be used if it is.
 */
extern void timer_start(/*t*/);
/*
nmtimer_t		t;
*/


/*
 * TIMER_STOP:
 *	If timer t is present on the queue then it is removed from the queue.
 *	Returns whether there was a timer to be removed or not.
 */
extern boolean_t timer_stop(/*t*/);
/*
nmtimer_t		t;
*/


/*
 * TIMER_RESTART:
 *	Starts timer T Whether it was already queued on the timer queue or not.
 */
extern void timer_restart(/*t*/);
/*
nmtimer_t		t;
*/


/*
 * TIMER_ALLOC:
 *	allocates space for a timer and initialises it.
 */
extern nmtimer_t timer_alloc();


/*
 * TIMER_INIT:
 *	Initilizes the timer package.
 *	Creates a thread which waits for timers to expire.
 */
extern boolean_t timer_init();


/*
 * TIMER_KILL:
 *	Terminates the background timer thread by tricking it into suicide.
 *	Note that the timer thread may not be terminated upon return --
 *	the purpose of this routine is to do cleanup when a program is
 *	ready to terminate so that the threads package will not dump core.
 */
extern void timer_kill();


/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_TIMER;


#endif _TIMER_
