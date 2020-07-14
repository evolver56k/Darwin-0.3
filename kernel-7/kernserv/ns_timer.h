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
 * The machine independent clock and timer definitions.
 *
 * Copyright (c) 1991 NeXT, Inc.
 *
 * HISTORY
 *
 *  6Dec91 Brian Pinkerton at NeXT
 *	Created.
 */
#import <kernserv/clock_timer.h>	/* for ns_time_t */
#import <bsd/sys/time.h>

void ns_timeout(func proc, void *arg, ns_time_t time, int pri);
void ns_abstimeout(func proc, void *arg, ns_time_t deadline, int pri);
boolean_t ns_untimeout(func proc, void *arg);
void ns_sleep(ns_time_t delay);

ns_time_t timeval_to_ns_time(struct timeval *tv);
void ns_time_to_timeval(ns_time_t nano_time, struct timeval *tvp);

#if	KERNEL_PRIVATE
#warning Obsolete header file: <kernserv/ns_timer.h>. use \
	<kern/time_stamp.h> instead; or better yet, just say no!!

#import <kern/time_stamp.h>

#endif	/* KERNEL_PRIVATE */
