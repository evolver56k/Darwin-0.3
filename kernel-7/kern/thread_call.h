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
 * Copyright (c) 1993, 1994, 1995 NeXT Computer, Inc.
 *
 * Declarations for thread-based callouts.
 *
 * HISTORY
 *
 * 8 June 1995 ? at NeXT
 *	Converted from old API naming and
 *	changed the time representation to
 *	tvalspec_t.
 *
 * 7 April 1994 ? at NeXT
 *	Added support for external and delayed
 *	entries.  Cleaned up somewhat.
 * 3 July 1993 ? at NeXT
 *	Created.
 */

#ifndef	_KERN_THREAD_CALL_H_
#define	_KERN_THREAD_CALL_H_

#import <mach/clock_types.h>

typedef void	*thread_call_spec_t;
typedef void	*thread_call_t;
typedef void	(*thread_call_func_t)(
		    thread_call_spec_t		spec,
		    thread_call_t		call
);

void
thread_call_init(void);
				
tvalspec_t
deadline_from_interval(
	tvalspec_t	interval
);

void
thread_call_func(
	thread_call_func_t	func,
	thread_call_spec_t	spec,
	boolean_t		unique_call
);
void
thread_call_func_delayed(
	thread_call_func_t	func,
	thread_call_spec_t	spec,
	tvalspec_t		deadline
);

void
thread_call_func_cancel(
	thread_call_func_t	func,
	thread_call_spec_t	spec,
	boolean_t		cancel_all
);

thread_call_t
thread_call_allocate(
	thread_call_func_t	func,
	thread_call_spec_t	spec
);
void
thread_call_free(
	thread_call_t		call
);

void
thread_call_enter(
	thread_call_t		call
);
void
thread_call_enter_spec(
	thread_call_t		call,
	thread_call_spec_t	spec
);
void
thread_call_enter_delayed(
	thread_call_t		call,
	tvalspec_t		deadline
);
void
thread_call_enter_spec_delayed(
	thread_call_t		call,
	thread_call_spec_t	spec,
	tvalspec_t		deadline
);
void
thread_call_cancel(
	thread_call_t		call
);

#endif	/* _KERN_THREAD_CALL_H_ */
