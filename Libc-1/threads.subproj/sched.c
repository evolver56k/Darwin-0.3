/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Copyright (c) 1990 NeXT Computer, Inc.  All rights reserved.
 *
 * Cthreads priority interface.
 */
/*
 * HISTORY
 * 14-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Created.
 */
/*
 * cthreads_sched.c - by Gregg Kellogg
 *
 * Cthreads layer on top of mach scheduling primitives.
 */
#include "cthreads.h"

kern_return_t
cthread_priority(cthread_t t, int priority, boolean_t set_max)
{
	return thread_priority(cthread_thread(t), priority, set_max);
}

kern_return_t
cthread_max_priority(cthread_t t, processor_set_t pset, int max_priority)
{
	return thread_max_priority(cthread_thread(t), pset, max_priority);
}

kern_return_t
cthread_abort(cthread_t t)
{
	return thread_abort(cthread_thread(t));
}
