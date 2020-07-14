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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef	_RWLOCK_
#define	_RWLOCK_

#include <mach/mach.h>
#include <mach/cthreads.h>
#include "debug.h"

typedef enum {
	PERM_READ,
	PERM_READWRITE,
} rw_perm_t;

typedef enum {
	NOBLOCK = 0,
	BLOCK = 1,
} rw_block_t;

typedef struct lock {
	struct mutex		lk_mutex;
#ifdef hpux
    cthread_t		lk_owner;
#endif
} *nmlock_t;

static inline void
lk_init (nmlock_t l)
{
#ifdef hpux
    l->lk_owner = NULL;
#endif
    mutex_init(&l->lk_mutex);
}

static inline void
lk_lock (nmlock_t l, rw_perm_t permission, rw_block_t block)
{
    mutex_lock(&l->lk_mutex);
#ifdef hpux
    l->lk_owner = cthread_self();
#endif
}

static inline void
lk_unlock (nmlock_t l)
{
#ifdef hpux
    l->lk_owner = NULL;
#endif
    mutex_unlock(&l->lk_mutex);
}

static inline void
lk_clear (nmlock_t l)
{
#ifdef hpux
    l->lk_owner = NULL;
#endif
    mutex_clear(&l->lk_mutex);
}


#endif	_RWLOCK_
