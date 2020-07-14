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
 * for i386 architecture.
 *
 * HISTORY
 *
 * 11 June 1995 ? at NeXT
 *	Major rewrite for new clock API - banished
 *	all occurrances of ns_time_t in favor of
 *	tvalspec_t.
 *
 * 24 March 1992 ? at NeXT
 *	Created from m68k version.
 */
 
#import <mach/mach_types.h>

#import <bsd/sys/param.h>
#import <bsd/sys/time.h>

#import <kern/clock.h>
#import <vm/vm_kern.h>

#import	<machdep/i386/timer.h>
#import <machdep/i386/timer_inline.h>
#import <machdep/i386/intr_exported.h>

static tvalspec_t	time_of_boot;	/* rel to 1/1/70 (UNIX T[0]) */

/*
 * Nanosecond event counter ;-)
 */

static struct _system_clock {
    tvalspec_t		counter;
    timer_cnt_val_t	last_timer_count;
    mapped_tvalspec_t	*mapped_counter;
    timer_cnt_val_t	timer_const;
} system_clock;

static tvalspec_t	system_time_stamp(void);

static struct _system_timer {
    boolean_t		is_set;
    tvalspec_t		expire_time;
    timer_func_t	expire_func;
} system_timer;

struct _interrupt_location {
    unsigned int	eip;
    int			rpl;
    int			ipl;
};

static void machine_hardclock(
		struct _interrupt_location	*loc);

static boolean_t	hardclock_enabled;

void
system_timer_dispatch(
    unsigned int		irq,
    void			*_state,
    int				ipl
)
{
    thread_saved_state_t	*state = (thread_saved_state_t *)_state;
    tvalspec_t			now;
    struct _interrupt_location	loc;

    ADD_TVALSPEC_NSEC(&system_clock.counter, NSEC_PER_TICK);
    system_clock.last_timer_count = system_clock.timer_const;

    if (state) {
	loc.eip	= state->frame.eip;

	if (!(state->frame.eflags & EFL_VM))
	    loc.rpl = state->frame.cs.rpl;
	else
	    loc.rpl = USER_PRIV;

	loc.ipl = ipl;
    }
    else {
	loc.eip = 0x00000000;
	loc.rpl = KERN_PRIV;
	loc.ipl = ipl;
    }
    
    if (hardclock_enabled)
    	machine_hardclock(&loc);

    now = system_time_stamp();
    
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

static void
machine_hardclock(
    struct _interrupt_location	*loc
)
{
    clock_interrupt(tick, (loc->rpl == USER_PRIV), (loc->ipl == 0));

    hardclock(loc->eip, STATUS_WORD(loc->rpl, loc->ipl));
}

static unsigned int	us_spin_us_const = 0x2000;

void
us_spin(
    unsigned int	us
)
{
    unsigned int	constant;
    
    while (us-- > 0) {
	constant = us_spin_us_const;

	while (constant-- > 0)
	    continue;
    }
}

void
us_spin_calibrate(void)
{
    timer_ctl_reg_t	reg;
    timer_cnt_val_t	leftover;
    int			s;
    
    reg = (timer_ctl_reg_t) { 0 };

    reg.mode = TIMER_EVENTMODE;
    reg.rw = TIMER_CTL_RW_BOTH;
    timer_set_ctl(reg);

    s = splclock();
    timer_write(TIMER_CNT0_SEL, TIMER_COUNT_MAX);
    
    us_spin(1);
    
    timer_latch(TIMER_CNT0_SEL); leftover = timer_read(TIMER_CNT0_SEL);
    splx(s);

    /*
     * Formula for spin constant is :
     *  (loopcount * timer clock speed)/ counter ticks
     */
    us_spin_us_const =
	(us_spin_us_const *
	    (TIMER_CONSTANT / (TIMER_COUNT_MAX - leftover))) / 1000000;
}

static timer_cnt_val_t
system_timer_constant(void)
{
    unsigned int	constant;
    
    constant = TIMER_CONSTANT / TICKS_PER_SEC;
    if ((TIMER_CONSTANT % TICKS_PER_SEC) >= (TICKS_PER_SEC / 2))
    	constant++;
		
    return ((timer_cnt_val_t)constant);
}

/*
 *  Initialize the clock and timer interface.
 */
void
machine_clock_init(void)
{
    timer_ctl_reg_t		reg;
    int				s;
    extern void			readtodc(unsigned int *);

    (void) intr_register_irq(0, system_timer_dispatch, 0, INTR_IPL6);
    (void) intr_enable_irq(0);

    /*
     * Initialize the RTC, and set the system's idea of time.
     */
    readtodc(&time_of_boot.tv_sec);

    /*
     * Set up the timer.
     */
    s = splclock();

    reg = (timer_ctl_reg_t){ 0 };

    reg.mode = TIMER_NDIVMODE;
    reg.rw = TIMER_CTL_RW_BOTH;
    timer_set_ctl(reg);

    // save constant for later use
    system_clock.timer_const = system_timer_constant();
    
    timer_write(TIMER_CNT0_SEL, system_clock.timer_const);
    system_clock.last_timer_count = system_clock.timer_const;

    splx(s);
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
	int		s = splclock();

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
    clock_type_t 	which_clock,
    tvalspec_t 		value
)
{
    tvalspec_t		counter = system_time_stamp();

    switch (which_clock) {

    case Calendar: {
	int		s = splclock();

	time_of_boot = value;
	SUB_TVALSPEC(&time_of_boot, &counter);
	splx(s);

	writetodc(&value.tv_sec);
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
	int		s = splclock();

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
	    s = splclock();
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
	    int		s = splclock();

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
	    int		s = splclock();
    
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
    timer_cnt_val_t	current_timer_count, last_timer_count;
    clock_res_t		fraction;
    tvalspec_t		result;
    int			s = splclock();

    // take a snapshot of the whole counter state
    result = system_clock.counter;
    last_timer_count = system_clock.last_timer_count;
    timer_latch(TIMER_CNT0_SEL);
    current_timer_count = timer_read(TIMER_CNT0_SEL);
    system_clock.last_timer_count = current_timer_count;
    splx(s);

    // check for tick overflow
    if (current_timer_count > last_timer_count) {
	// we missed a tick
	ADD_TVALSPEC_NSEC(&result, NSEC_PER_TICK);
    }

    // convert fraction to ns
    fraction = (system_clock.timer_const - current_timer_count);
    fraction = fraction * (NSEC_PER_SEC / TIMER_CONSTANT);

    ADD_TVALSPEC_NSEC(&result, fraction);

    return (result);
}

unsigned int
event_get(void)
{
    return (system_time_stamp().tv_nsec);
}
