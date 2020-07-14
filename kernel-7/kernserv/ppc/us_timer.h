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
 * Copyright (c) 1991 NeXT Computer, Inc.
 *
 * HISTORY
 * 14-May-90  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

/*
 *  NOTE:
 *
 *	This file specifies obsolete API.  It is provided for compatibility
 *	only.
 */

#ifndef _KERN_INTERNAL_US_TIMER_H
#define	_KERN_INTERNAL_US_TIMER_H

#import <mach/boolean.h>
#import <sys/time.h>
#import <mach/mach_types.h>
#import <kernserv/ns_timer.h>

#if	KERNEL
void us_timer_init(void);
void us_timeout(func proc, vm_address_t arg, struct timeval *tvp, int pri);
void us_abstimeout(func proc, vm_address_t arg, struct timeval *tvp, int pri);
boolean_t us_untimeout(func proc, vm_address_t arg);
void microtime(struct timeval * tvp);
void microboot(struct timeval * tvp);
void delay(unsigned int n);
void us_delay(unsigned usecs);
#endif	/* KERNEL */
#endif	/* _KERN_INTERNAL_US_TIMER_H */
