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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Simple spin locks.
 *
 * HISTORY
 *
 * 19 November 1992 ? at NeXT
 *	Created.
 */
 
#import <mach/boolean.h>

#ifndef	_MACH_I386_SIMPLE_LOCK_H_
#define _MACH_I386_SIMPLE_LOCK_H_

#define _MACHINE_SIMPLE_LOCK_DATA_

struct slock {
    boolean_t		locked;
};

typedef struct slock		simple_lock_data_t;
typedef simple_lock_data_t	*simple_lock_t;

static __inline__
void
simple_lock_init(
    simple_lock_t	slock
)
{
    slock->locked = FALSE;
}

static __inline__
boolean_t
simple_lock_try(
    simple_lock_t	slock
)
{
    boolean_t		result;
    
    asm volatile(
    	"xchgl %1,%0; xorl %3,%0"
	    : "=r" (result), "=m" (slock->locked)
	    : "0" (TRUE), "i" (TRUE));
	    
    return (result);
}

static __inline__
void
simple_lock(
    simple_lock_t	slock
)
{    
    do
    	{
	    while (slock->locked)
		continue;
	}
    while (!simple_lock_try(slock));
}

static __inline__
void
simple_unlock(
    simple_lock_t	slock
)
{
    boolean_t		result;
    
    asm volatile(
	"xchgl %1,%0"
	    : "=r" (result), "=m" (slock->locked)
	    : "0" (FALSE));
}

#endif	/* _MACH_I386_SIMPLE_LOCK_H_ */
