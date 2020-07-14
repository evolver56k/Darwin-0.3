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
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	slave.c
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Misc. slave routines.
 */

#import <cpus.h>

#if	NCPUS > 1

#import <simple_clock.h>

#import <bsd/machine/reg.h>
#ifdef	ibmrt
#import <ca/scr.h>
#endif	ibmrt
#if	!defined(ibmrt) && !defined(mips)
#import <bsd/machine/psl.h>
#endif	!defined(ibmrt) && !defined(mips)

#import <sys/param.h>
#import <sys/systm.h>
#import <sys/dk.h>
#import <sys/dir.h>
#import <sys/user.h>
#import <sys/kernel.h>
#import <sys/proc.h>
#import <bsd/machine/cpu.h>
#ifdef	vax
#import <vax/mtpr.h>
#endif	vax

#import <kern/timer.h>

#import <kern/sched.h>
#import <kern/thread.h>
#import <mach/machine.h>
#import <kernserv/clock_timer.h>

#ifdef	balance
#import <machine/intctl.h>
#endif	balance

#import	<kernserv/machine/us_timer.h>

slave_main()
{
	slave_config();
	cpu_up(cpu_number());

	ns_hardclock_init();
	timer_init(&kernel_timer[cpu_number()]);
	start_timer(&kernel_timer[cpu_number()]);

	slave_start();
	/*NOTREACHED*/
}

#endif	NCPUS > 1
