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
 * Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
 * Open Software Foundation, Inc. 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of ("OSF") or Open Software 
 * Foundation not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
 * ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
 */
/*
 * OSF Research Institute MK6.1 (unencumbered) 1/31/1995
 */

/*
 *	File:		clock_types.h
 *	Purpose:	Clock facility header definitions. These
 *			definitons are needed by both kernel and
 *			user-level software.
 */

#ifndef	_MACH_CLOCK_TYPES_H_
#define	_MACH_CLOCK_TYPES_H_

/*
 * Reserved clock id values for default clocks.
 */
#define	REALTIME_CLOCK	0		/* required for all systems */
#define BATTERY_CLOCK	1		/* optional */
#define HIGHRES_CLOCK	2		/* optional */

/*
 * Type definitions.
 */
typedef	int	alarm_type_t;		/* alarm time type */
typedef int	sleep_type_t;		/* sleep time type */
typedef	int	clock_id_t;		/* clock identification type */
typedef int	clock_flavor_t;		/* clock flavor type */
typedef int	*clock_attr_t;		/* clock attribute type */
typedef int	clock_res_t;		/* clock resolution type */


/*
 * Attribute names.
 */
#define	CLOCK_GET_TIME_RES	1	/* get_time call resolution */
#define	CLOCK_MAP_TIME_RES	2	/* map_time call resolution */
#define CLOCK_ALARM_CURRES	3	/* current alarm resolution */
#define CLOCK_ALARM_MINRES	4	/* minimum alarm resolution */
#define CLOCK_ALARM_MAXRES	5	/* maximum alarm resolution */

/*
 * Normal time specification used by the kernel clock facility.
 */
struct tvalspec {
	unsigned int	tv_sec;			/* seconds */
	clock_res_t	tv_nsec;		/* nanoseconds */
};
typedef struct tvalspec	tvalspec_t;

#define NSEC_PER_USEC	1000		/* nanoseconds per microsecond */
#define USEC_PER_SEC	1000000		/* microseconds per second */
#define NSEC_PER_SEC	1000000000	/* nanoseconds per second */
#define BAD_TVALSPEC(t)							\
	((t)->tv_nsec < 0 || (t)->tv_nsec >= NSEC_PER_SEC)

/* t1 <=> t2 */
#define CMP_TVALSPEC(t1, t2)						\
	((t1)->tv_sec > (t2)->tv_sec ? +1 :				\
	((t1)->tv_sec < (t2)->tv_sec ? -1 : (t1)->tv_nsec - (t2)->tv_nsec))

/* t1  += t2 */
#define ADD_TVALSPEC(t1, t2)						\
{									\
	if (((t1)->tv_nsec += (t2)->tv_nsec) >= NSEC_PER_SEC) {		\
		(t1)->tv_nsec -= NSEC_PER_SEC;				\
		(t1)->tv_sec  += 1;					\
	}								\
	(t1)->tv_sec += (t2)->tv_sec;					\
}

/* t1  -= t2 */
#define SUB_TVALSPEC(t1, t2)						\
{									\
	if (((t1)->tv_nsec -= (t2)->tv_nsec) < 0) {			\
		(t1)->tv_nsec += NSEC_PER_SEC;				\
		(t1)->tv_sec  -= 1;					\
	}								\
	(t1)->tv_sec -= (t2)->tv_sec;					\
}

/*
 * Mapped time specification used by the kernel clock facility.
 */
struct	mapped_tvalspec {
	tvalspec_t	mtv_time;
#define	mtv_sec		mtv_time.tv_sec		/* seconds */
#define mtv_nsec	mtv_time.tv_nsec	/* nanoseconds */
	unsigned int	mtv_csec;		/* check seconds */
};
typedef struct mapped_tvalspec	mapped_tvalspec_t;

/*
 * Macro for reading a consistant tvalspec_t value "ts"
 * from a mapped time specification "mts". (On a multi
 * processor, it is assumed that processors see writes
 * in the "correct" order since the kernel updates the
 * mapped time in the inverse order it is read here.)
 */
#define MTS_TO_TS(mts, ts)				\
	do {						\
		(ts)->tv_sec  = (mts)->mtv_sec;		\
		(ts)->tv_nsec = (mts)->mtv_nsec;	\
	} while ((ts)->tv_sec != (mts)->mtv_csec)

/*
 * Alarm parameter defines.
 */
#define ALRMTYPE	0xff		/* type (8-bit field)	*/
#define TIME_ABSOLUTE	0x0		/* absolute time */
#define TIME_RELATIVE	0x1		/* relative time */
#define BAD_ALRMTYPE(t)	\
	(((t) & ALRMTYPE) > TIME_RELATIVE)

#endif /* _MACH_CLOCK_TYPES_H_ */
