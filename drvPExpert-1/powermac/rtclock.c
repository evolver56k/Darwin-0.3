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
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * Copyright 1996 1995 by Apple Computer, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

/*
 *	File:		rtclock.c
 *	Purpose:	Routines for handling the machine dependent
 *			real-time clock.
 */

#include <kern/clock.h>
#include <machdep/ppc/proc_reg.h>
#include <mach/ppc/thread_status.h> /* for struct ppc_saved_state */
#include <powermac.h>

/* global data declarations */

boolean_t rtclock_initialised;

unsigned int rtclock_intr_clock_ticks;

#define RTC_MAXRES	(NSEC_PER_SEC / HZ)	/* max resolution nsec */

int
rtc_init(void)
{
	rtclock_intr_clock_ticks = nsec_to_processor_clock_ticks(RTC_MAXRES);

	/* Set decrementer */
	mtdec(rtclock_intr_clock_ticks);

	rtclock_initialised = TRUE;
}

/*
 * Real-time clock device interrupt. Called only on the
 * master processor. Updates the clock time and upcalls
 * into the higher level clock code to deliver alarms.
 */
int
rtclock_intr(int device, struct ppc_saved_state *ssp)
{
	tvalspec_t	clock_time;
	int		now,now2;

	/* We may receive interrupts too early, we must reject them. At
	 * startup, the decrementer interrupts every ~2 seconds without
	 * us doing anything
	 */
	if (rtclock_initialised == FALSE)
		return 0;

	while ((now = mfdec()) < 0) {

                /*
                 * Wait for the decrementer to change, then jump
                 * in and add decrementer_count to its value
                 * (quickly, before it changes again!)
                 */
                while ((now2 = mfdec()) == now)
                        ;
                mtdec(now2 + rtclock_intr_clock_ticks);

		//hertz_tick(USER_MODE(ssp->srr1), ssp->srr0);
		ppc_hardclock(ssp);
	}
}

/*
 * long long read_processor_clock(void)
 *
 * Read the processor's realtime clock, converting into
 * a 64 bit number
 */

long long read_processor_clock(void)
{
	union {
		long long 	time64;
		int		word[2];
	} now;

	if (PROCESSOR_VERSION == PROCESSOR_VERSION_601) {
		unsigned int nsec,sec;
		do {
			sec  = mfrtcu();
			nsec = mfrtcl();
		} while (sec != mfrtcu());
		return (long long) nsec + (NSEC_PER_SEC * (long long)sec);
	} else {
		do {
			now.word[0]  = mftbu();
			now.word[1]  = mftb();
		} while (now.word[0] != mftbu());
		return now.time64;
	}
}

/*
 * void read_processor_clock_tval(tvalspec_t *time)
 *
 * Read the processor's realtime clock, converting into
 * a tvalspec structure. Note that on the 601 processor
 * we have to take into consideration the possibility
 * that the clock doesn't tick at the correct frequency
 */

void read_processor_clock_tval(tvalspec_t *time)
{
        long long now, now_h, now_l, tmp;
	unsigned int dec_period;

	dec_period = powermac_info.dec_clock_period;

	/* We assume that now uses only 56 bits.
         * This allows for 117 years of uptime if the bus is 80 MHz
	 * A faster bus means less up time.
	 */
	now = read_processor_clock();
	now_l = now & 0x00000000FFFFFFFFULL;
	now_h = now >> 32;

	/* Do low 24 bits of big mult and round */
	tmp = now_l * dec_period + 0x00800000ULL;

	/* Add the high 8 bits to the shifted low part */
	tmp = ((now_h * dec_period) << 8) + (tmp >> 24);

	/* tmp now is the number of ns we want */
	time->tv_sec  = tmp / NSEC_PER_SEC;
	time->tv_nsec = tmp % NSEC_PER_SEC;

        return;
}

int nsec_to_processor_clock_ticks(int nsec)
{
	long long num;

	/* Since num used 32 bits or less this is safe (ie. no overflow) */
	num = ((long long)nsec << 24) / powermac_info.dec_clock_period;

	return (int) num;
}

/* nsec_delay may overflow for delays over around 2 seconds. */
void tick_delay(int ticks)
{
	long long time, time2;

	time = read_processor_clock();

	/* Add on delay we want */
	
	time += ticks;
		
	/* Busy loop */

	do {
		time2 = read_processor_clock();
	} while (time2 < time);
}

/*
 * Delay creates a busy-loop of a given number of usecs. Doesn't use
 * interrupts
 *
 * This uses the rtc registers or the time-base registers depending upon
 * processor type.
 */

void delay(int usec)
{
	long ticks;

	ticks = nsec_to_processor_clock_ticks(NSEC_PER_SEC);

	/* longer delays may require multiple calls to tick_delay */
	while (usec > USEC_PER_SEC) {
		tick_delay(ticks);
		usec -= USEC_PER_SEC;
	}
	tick_delay(nsec_to_processor_clock_ticks(usec * NSEC_PER_USEC));
}

void
us_spin(int usec)
{
	delay(usec);
}
