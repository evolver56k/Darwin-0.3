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

/* HISTORY
 * 18-May-90  Avadis Tevanian (avie) at NeXT
 *	Changed to use sensible priorities (higher numbers -> higher pri).
 *
 *  1-Feb-88  David Golub (dbg) at Carnegie-Mellon University
 *	Goofed... netisr thread must run at splnet, because the routines
 *	it calls expect to be called from the softnet interrupt (at
 *	splnet).
 *
 * 19-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 *
 */

/*
 *	netisr.c
 *
 *	Kernel thread for network code.
 */

#include <machine/spl.h>
#include <net/netisr.h>

#include <kern/thread.h>
#include <kern/sched_prim.h>

volatile int netisr;
int   soft_net_wakeup;

void netisr_thread_continue(void)
{
	/*
	 *	All routines this thread calls expect to be called
	 *	at splnet.
	 */
	 	 
	(void) splnet();

	while (netisr != 0) {
#ifdef	NIMP
#if	NIMP > 0
		if (netisr & (1<<NETISR_IMP)){
			netisr &= ~(1<<NETISR_IMP);
			impintr();
		}
#endif	/* NIMP > 0 */
#endif	/* NIMP */

#if	INET
		if (netisr & (1<<NETISR_IP)){
			void ipintr(void);

			netisr &= ~(1<<NETISR_IP);
			ipintr();
		}
		if (netisr & (1<<NETISR_ARP)) {
			void arpintr(void);

			netisr &= ~(1<<NETISR_ARP);
			arpintr();
		}
#endif	/* INET */

#if defined(ppc)
		if (netisr & (1<<NETISR_BLUE)){
			void blue_notify(void);

			netisr &= ~(1<<NETISR_BLUE);
			blue_notify();
		}
#endif /* ppc */

#if	ISO
		if (netisr & (1<<NETISR_ISO)) {
	    	netisr &= ~(1<<NETISR_ISO);
	    	isointr();
		}
#endif	/* ISO */

#if	CCITT
		if (netisr & (1<<NETISR_CCITT)) {
	    	netisr &= ~(1<<NETISR_CCITT);
	    	ccittintr();
		}
#endif	CCITT

#if	NS
		if (netisr & (1<<NETISR_NS)){
			netisr &= ~(1<<NETISR_NS);
			nsintr();
		}
#endif	NS

#if NETAT
		if (netisr & (1<<NETISR_APPLETALK)){
			void atalkintr(void);

			netisr &= ~(1<<NETISR_APPLETALK);
			atalkintr();
		}
#endif NETAT
	}

	assert_wait(&soft_net_wakeup, FALSE);
	thread_block_with_continuation(netisr_thread_continue);
	/* NOTREACHED */
}

void netisr_thread(void)
{
	register thread_t self = current_thread();
	extern void stack_privilege(thread_t thread);
	extern void thread_bind(thread_t, processor_t);

	/*
	 *	Make sure that this thread 
	 *	always has a kernel stack, and
	 *	bind it to the master cpu.
	 */
	stack_privilege(self);
	thread_bind(self, master_processor);
	thread_block_with_continuation(netisr_thread_continue);
	/* NOTREACHED */
}
