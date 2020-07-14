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
 * Copyright (c) 1992, 1993, 1994, 1995 NeXT Computer, Inc.
 *
 * Machine dependent clock and timer routines
 * for ppc architecture.  Ported from hppa/sparc.
 *
 */

#include <mach/mach_types.h>

#include <bsd/sys/param.h>
#include <bsd/sys/time.h>

#import <kern/clock.h>
#import <vm/vm_kern.h>

/* Window server in action flag */
extern unsigned int	wserver_on;
extern unsigned int	get_unix_time_of_day( void );
extern void		set_unix_time_of_day( unsigned int unixSecs );

boolean_t		clock_initialized = FALSE;

tvalspec_t	time_of_boot;	/* rel to 1/1/70 (UNIX T[0]) */

/*
 * Nanosecond event counter ;-)
 */
 
static struct _system_clock {
    mapped_tvalspec_t	*mapped_counter;
} system_clock;

static tvalspec_t	system_time_stamp(void);

static struct _system_timer {
    boolean_t		is_set;
    tvalspec_t		expire_time;
    timer_func_t	expire_func;
} system_timer;

static boolean_t	hardclock_enabled;

void
ppc_hardclock(struct ppc_saved_state *ssp)
{
    tvalspec_t		now;
    
    if (hardclock_enabled) {
	clock_interrupt(tick, USERMODE(ssp->srr1), BASEPRI(ssp->srr1));
	hardclock(ssp->srr0, ssp->srr1);
    }

    read_processor_clock_tval(&now);

    if (system_timer.is_set &&
	CMP_TVALSPEC(&system_timer.expire_time, &now) <= 0) {
	    system_timer.is_set = FALSE;
	    (*system_timer.expire_func)(now);
    }
}

void
hardclock_init(void)
{
    hardclock_enabled = TRUE;
}

/*
 *  Initialize the clock and timer interface.
 */
void
machine_clock_init(void)
{
/*
 * Initialixe the RTC, and set the system's idea of time.
 */

    rtc_init();

    // Set the boot time to zero.  It will be updated when the
    // via (Cuda or PMU) driver is probed.
    time_of_boot.tv_sec = 0;

    clock_initialized = TRUE;
}

/*
 * Set the systems real boot time now that DriverKit has given us
 * access to a Time Of Day clock.
 */
void
set_boot_time(void)
{
      time_of_boot.tv_sec =
	get_unix_time_of_day() - clock_get_counter(System).tv_sec;
}

/*
 *  Clock functions
 */
tvalspec_t
clock_get_counter(
    clock_type_t 	which_clock
)
{
    tvalspec_t		result = system_time_stamp();

    switch (which_clock) {

    case System:
	break;

    case Calendar: {
	int		s = splusclock();

    	ADD_TVALSPEC(&result, &time_of_boot);
	splx(s);
	break;
    }

    default:
	result = TVALSPEC_ZERO;
	break;
    }
    
    return (result);
}

void
clock_set_counter(
    clock_type_t	which_clock,
    tvalspec_t		value
)
{
    tvalspec_t		counter = system_time_stamp();

    switch (which_clock) {

    case Calendar: {
    	int		s = splusclock();

	time_of_boot = value;
	SUB_TVALSPEC(&time_of_boot, &counter);
	splx(s);
	set_unix_time_of_day( value.tv_sec );
    	break;
    }

    default:
    	/* Can only set the calendar */
	break;
    }
}

void
clock_adjust_counter(
    clock_type_t	which_clock,
    clock_res_t		nsec
)
{
    switch (which_clock) {

    case Calendar: {
	int		s = splusclock();

	ADD_TVALSPEC_NSEC(&time_of_boot, nsec);
	splx(s);
	break;
    }

    default:
	/* Can only adjust the calendar */
	break;
    }
}

mapped_tvalspec_t *
clock_map_counter(
    clock_type_t	which_clock
)
{
    mapped_tvalspec_t	*mapped_clock;

    switch (which_clock) {
    
    case System:
    	if (!(mapped_clock = system_clock.mapped_counter)) {
	    int		s;

	    if (kmem_alloc_wired(kernel_map,
			(vm_offset_t *)&mapped_clock,
				PAGE_SIZE) != KERN_SUCCESS) {
		mapped_clock = 0;
		break;
	    }
	    s = splusclock();
	    system_clock.mapped_counter = mapped_clock;
	    splx(s);
	}
	break;
    
    default:
    	mapped_clock = 0;
	break;
    }
    
    return (mapped_clock);
}

void
timer_set_expire_func(
    timer_type_t	which_timer,
    timer_func_t	expire_func
)
{
    switch (which_timer) {

    case SystemWide:
	if (!expire_func || !system_timer.expire_func) {
	    int		s = splusclock();

	    system_timer.expire_func = expire_func;
	    system_timer.is_set = FALSE;
	    splx(s);
	}
	break;

    default:
    	break;
    }
}

void						
timer_set_deadline(
    timer_type_t	which_timer,
    tvalspec_t		deadline
)
{
    switch (which_timer) {

    case SystemWide:
	if (system_timer.expire_func) {
	    int		s = splusclock();
    
	    system_timer.expire_time = deadline;
	    system_timer.is_set = TRUE;
	    splx(s);
	}
	break;

    default:
	break;
    }
}

/*
 * Get current clock value plus fraction of a tick.
 */
static tvalspec_t
system_time_stamp(void)
{
    tvalspec_t		result;

    read_processor_clock_tval(&result);

    return (result);
}

/* Return 32 bit representation of event counter */
unsigned int
event_get(void)
{
    return (system_time_stamp().tv_nsec);
}
