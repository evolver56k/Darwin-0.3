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
 * Copyright (c) 1994 NeXT Computer, Inc.
 *
 * Kernel generic lock object.
 *
 * HISTORY
 *
 * 4 July 1994 ? at NeXT
 *	Created.
 */

#import <cpus.h>

#import <mach/mach_types.h>

#import <driverkit/KernLock.h>

@implementation KernLock

- initWithLevel:	(int)level
{
#if	NCPUS > 1
    _slock = simple_lock_alloc();
    simple_lock_init(_slock);
#else
    if (level == 0)
    	return [super free];
#endif

    _lockLevel = level;
    
    return self;
}

- init
{
    return [self initWithLevel:0];
}

- free
{
#if	NCPUS > 1
    simple_lock_free(_slock);
#endif
    
    return [super free];
}

- (int)level
{
    return _lockLevel;
}

- (void)acquire
{
    int		oldLevel = curipl();
    
    if (oldLevel < _lockLevel)
    	spln(ipltospl(_lockLevel));

#if	NCPUS > 1	
    simple_lock(_slock);
#endif
    _savedLevel = oldLevel;
}

- (void)release
{
    int		oldLevel = _savedLevel;

#if	NCPUS > 1    
    simple_unlock(_slock);
#endif
    spln(ipltospl(oldLevel));
}

@end

typedef struct KernLock_ {
    @defs(KernLock)
} KernLock_;

void
KernLockAcquire(
    KernLock		*_lock
)
{
    KernLock_		*lock = (KernLock_ *)_lock;
    int			oldLevel = curipl();
    
    if (_lock == nil)
    	return;
    
    if (oldLevel < lock->_lockLevel)
    	spln(ipltospl(lock->_lockLevel));

#if	NCPUS > 1	
    simple_lock(lock->_slock);
#endif
    lock->_savedLevel = oldLevel;
}

void
KernLockRelease(
    KernLock		*_lock
)
{
    KernLock_		*lock = (KernLock_ *)_lock;
    int			oldLevel;
    
    if (_lock == nil)
    	return;

    oldLevel = lock->_savedLevel;

#if	NCPUS > 1    
    simple_unlock(lock->_slock);
#endif
    spln(ipltospl(oldLevel));
}
