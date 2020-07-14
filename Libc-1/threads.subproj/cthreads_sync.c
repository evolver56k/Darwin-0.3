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
#include "cthreads.h"
#include "cthread_internals.h"

/*
 * Mutex objects.
 *
 * Mutex_wait_lock is implemented in terms of mutex_try_lock().
 * Mutex_try_lock() and mutex_unlock() are machine-dependent.
 */

extern int mutex_spin_limit;


void
mutex_wait_lock(m)
	register mutex_t m;
{
	register int i;

	TRACE(printf("[%s] lock(%s)\n", cthread_name(cthread_self()), mutex_name(m)));
	for (i = 0; i < mutex_spin_limit; i += 1)
		if (m->lock == 0 && mutex_try_lock(m))
			return;
		else
			/* spin */;
	for (;;)
		if (m->lock == 0 && mutex_try_lock(m))
			return;
		else
			cthread_yield();
}
