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
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifndef	_MACHINE_MACHSPL_H_
#define	_MACHINE_MACHSPL_H_ 1


/*
 *	This file defines the interrupt priority levels used by
 *	machine-dependent code.
 *
 *      The powerpc has only primitive interrupt masking (all/none),
 *      spl behaviour is done in software
 */

/*
 * These are the bits in the eirr register that are assigned for various 
 * devices
 */

#define SPL_CLOCK_BIT		0
#define SPL_POWER_BIT		1
#define SPL_VM_BIT		4
#define SPL_CIO_BIT		8
#define SPL_HPIB_BIT		9
#define SPL_BIO_BIT		9
#define SPL_IMP_BIT		9
#define SPL_TTY_BIT		16
#define SPL_NET_BIT		24

/*
 * The powerpc only has a single level of hardware interrupt. There is
 * no processor support for disabling certain interrupts whilst allowing
 * others - this must be done in software.
 * 
 * The mechanism chosen is fairly standard - a primary interrupt fielder
 * is used which detects the source of the incoming interrupt and thus its
 * priority. This routine will either allow the interrupt to pass to
 * its handler if it is above the current priority, or queue it if it is
 * not. A word of 32 bits is used for queue flags, thus determining the
 * number of interrupt levels available.
 */


/*
 * Convert interrupt levels into machine-independent SPLs as follows:
 *
 *                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
 *  0 1 2 3 4 5 6 7  8  9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |c|p| | |v| | | |b i| | | | | | | |t| | | | | | | |n| | |s| | | | |
 * |l|w| | |m| | | |i m| | | | | | | |t| | | | | | | |e| | |c| | | | |
 * |k|r| | | | | | |o p| | | | | | | |y| | | | | | | |t| | |l| | | | |
 * | | | | | | | | |   | | | | | | | | | | | | | | | | | | |k| | | | |
 * +-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Notes:
 *	- software prohibits more than one machine-dependent SPL per bit on
 *	  a given architecture (e.g. ppc).  In cases where there
 *	  are multiple equivalent devices which interrupt at the same level
 *	  (e.g. ASP RS232 #1 and #2), the interrupt table insertion routine
 *	  will always add in the unit number (arg0) to offset the entry.
 *	- hard clock must be the first bit (i.e. 0x80000000).
 *	- SPL7 is any non-zero value (since the PSW I-bit is off).
 *	- SPLIMP serves two purposes: blocks network interfaces and blocks
 *	  memory allocation via malloc.  In theory, SPLLAN would be high
 *TODONMGS	  enough.  However, on the hp700, the SCSI driver uses malloc at
 *TODONMGS	  interrupt time requiring SPLIMP >= SPLBIO.  On the 800, we are
 *	  still using HP-UX drivers which make the assumption that
 *	  SPLIMP >= SPLCIO.  New drivers would address both problems.
 */
 /* Note also : if any new SPL's are introduced, please add to debugging list*/
#define SPLOFF          (32 - 0)        /* all interrupts disabled TODO NMGS  */
#define SPLHIGH         (32 - 2)        /* TODO NMGS any non-zero, non-INTPRI value */
#define SPLSCHED        SPLHIGH
#define SPLCLOCK        (32 - 0)        /* hard clock */
#define SPLPOWER        (32 - 1)        /* power failure (unused) */
#define SPLVM           (32 - 4)        /* TLB shootdown (unused) */
#define SPLBIO          (32 - 8)        /* block I/O */
#define SPLIMP          (32 - 8)        /* network & malloc */
#define SPLTTY          (32 - 16)       /* TTY */
#define SPLNET          (32 - 24)       /* soft net */
#define SPLSCLK         (32 - 27)       /* soft clock */
#define SPLLO           (32 - 32)       /* no interrupts masked */

#define SPL_CMP_GT(a, b)        ((unsigned)(a) >  (unsigned)(b))
#define SPL_CMP_LT(a, b)        ((unsigned)(a) <  (unsigned)(b))
#define SPL_CMP_GE(a, b)        ((unsigned)(a) >= (unsigned)(b))
#define SPL_CMP_LE(a, b)        ((unsigned)(a) <= (unsigned)(b))

#define IPLHIGH         SPLHIGH
#define IPLSCHED        SPLSCHED
#define IPLDEVICE		SPLBIO


#ifndef __ASSEMBLER__

typedef unsigned	spl_t;

/*
 * This is a generic interrupt switch table.  It may be used by various
 * interrupt systems.  For each interrupt, it holds a handler and an
 * TODO NMGS interrupt mask (selected from SPL* or, more generally, INTPRI*).
 *
 * So that these tables can be easily found, please prefix them with
 * the label "itab_" (e.g. "itab_proc").
 */

#if defined(KERNEL_BUILD)
#include <mach_debug.h>
#endif /* KERNEL_BUILD */

struct intrtab {
	void (*handler)(int); /* ptr to routine to call */
	unsigned int intpri;	/* INTPRI (SPL) with which to call it */
	int arg;		/* 1 arguments to handler: arg0 is unit */
};

#endif /* __ASSEMBLER__ */

#endif	/* _MACHINE_MACHSPL_H_ */
