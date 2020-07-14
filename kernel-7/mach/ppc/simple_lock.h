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

/* Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 * 
 *	File:	mach/ppc/simple_lock.h
 *
 *	This file contains machine dependent code for exclusion
 *	lock handling on PowerPC-based products.
 *
 */

#ifndef	_MACH_PPC_SIMPLE_LOCK_
#define _MACH_PPC_SIMPLE_LOCK_

#include <mach/ppc/boolean.h>

#define	_MACHINE_SIMPLE_LOCK_DATA_

struct slock{
	volatile unsigned int lock_data[4];/* in general 1 bit is sufficient */
};

typedef struct slock	simple_lock_data_t;
typedef struct slock	*simple_lock_t;

#if defined(_KERNEL)

#if	!defined(DEFINE_SIMPLE_LOCK_PRIMS)
extern __inline__
#endif
void
simple_lock_init(
    simple_lock_t	slock
)
{
	slock->lock_data[0] = 0;
	slock->lock_data[1] = 0;
	slock->lock_data[2] = 0;
	slock->lock_data[3] = 0;
}

#if	!defined(DEFINE_SIMPLE_LOCK_PRIMS)
extern __inline__
#endif
void
simple_unlock(
    simple_lock_t	slock
)
{
	__asm__ volatile ("sync");
    *((volatile int *)(slock)) = 0;
}

#if	!defined(DEFINE_SIMPLE_LOCK_PRIMS)
extern __inline__
#endif
boolean_t
simple_lock_try(
    simple_lock_t	slock
)
{
	extern boolean_t test_and_set();
    boolean_t		result;
	
    result = test_and_set(0, (int *)slock);
	
    return (!result);
}



#if	!defined(DEFINE_SIMPLE_LOCK_PRIMS)
extern __inline__
#endif
void
#if DIAGNOSTIC
real_simple_lock(
#else
simple_lock(
#endif
    simple_lock_t	slock
)
{    
	extern boolean_t test_and_set();
    while (test_and_set(0, (int *)slock))
		continue;
}

#endif /* _KERNEL */
#endif	/* _MACH_PPC_SIMPLE_LOCK_ */
