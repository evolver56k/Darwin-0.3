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
 * Copyright (c) 1995 NeXT Computer, Inc.
 *
 * Interim clock/timer control API.
 *
 * HISTORY
 *
 * 4 June 1995 ? at NeXT
 *	Created.
 */

#ifndef	_KERN_CLOCK_H_
#define	_KERN_CLOCK_H_

#import <mach/clock_types.h>
#import <machdep/machine/mach_param.h>
#import <kern/macro_help.h>

#define TICKS_PER_SEC	HZ
#define NSEC_PER_TICK	(NSEC_PER_SEC/TICKS_PER_SEC)

#define TVALSPEC_SEC_MAX	(0 - 1)
#define TVALSPEC_NSEC_MAX	(NSEC_PER_SEC - 1)

#define TVALSPEC_MAX	((tvalspec_t) { TVALSPEC_SEC_MAX, TVALSPEC_NSEC_MAX } )
#define TVALSPEC_ZERO	((tvalspec_t) { 0, 0 } )

#define ADD_TVALSPEC_NSEC(t1, nsec)					\
MACRO_BEGIN								\
	(t1)->tv_nsec += (clock_res_t)(nsec);				\
	if ((clock_res_t)(nsec) > 0 && (t1)->tv_nsec >= NSEC_PER_SEC) {	\
		(t1)->tv_nsec -= NSEC_PER_SEC;				\
		(t1)->tv_sec += 1;					\
	}								\
	else if ((clock_res_t)(nsec) < 0 && (t1)->tv_nsec < 0) {	\
		(t1)->tv_nsec += NSEC_PER_SEC;				\
		(t1)->tv_sec -= 1;					\
	}								\
MACRO_END

#define UPDATE_MAPPED_TVALSPEC(mtv, tv)				\
MACRO_BEGIN							\
	if ((mtv) != 0) {					\
		(mtv)->mtv_csec = (tv)->tv_sec;			\
		(mtv)->mtv_nsec = (tv)->tv_nsec;		\
		(mtv)->mtv_sec = (tv)->tv_sec;			\
	}							\
MACRO_END

extern tvalspec_t ticks_to_tvalspec(
	unsigned int	ticks);

typedef enum {
	Calendar,	/* assumed battery back-up */
	System		/* assumed "highest resolution available" */
} clock_type_t;

extern tvalspec_t clock_get_counter(
	clock_type_t	which_clock);

extern void clock_set_counter(
	clock_type_t	which_clock,
	tvalspec_t	value);

extern void clock_adjust_counter(
	clock_type_t	which_clock,
	clock_res_t	nsec);

extern mapped_tvalspec_t *clock_map_counter(
	clock_type_t	which_clock);

typedef enum {
	SystemWide	/* system-wide event timer */
} timer_type_t;

extern void timer_set_deadline(
	timer_type_t	which_timer,
	tvalspec_t	deadline);

typedef void	(*timer_func_t)(
	tvalspec_t	timestamp);

extern void timer_set_expire_func(
	timer_type_t	which_timer,
	timer_func_t	func);						

#endif	/* _KERN_CLOCK_H_ */
