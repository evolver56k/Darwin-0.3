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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * NXSpinLock.m. Cheap lock object, kernel version.
 *
 * HISTORY
 * 22-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#define KERNEL		1
#define KERNEL_PRIVATE	1
#define ARCH_PRIVATE	1 

#import <machkit/NXLock.h>
#import <kernserv/lock.h>

/*
 * _priv instance variable points to one of these.
 */
struct _priv {
    simple_lock_t		spin_lock;
};

@implementation NXSpinLock 

- init
{
	struct _priv	*p = _priv;
	
	[super init];

	if (p == 0) {	
	    p = (struct _priv *)kalloc(sizeof (struct _priv));
	    p->spin_lock = simple_lock_alloc();
	
	    _priv = p;
	}

	simple_lock_init(p->spin_lock);

	return self;
}

- free
{
	struct _priv	*p = _priv;

	if (p) {
	    simple_lock_free(p->spin_lock);
	    kfree(p, sizeof (struct _priv));
	}

	return [super free];
}


- lock
{
	struct _priv	*p = _priv;

	simple_lock(p->spin_lock);

	return self;
}

- unlock
{
	struct _priv	*p = _priv;

	simple_unlock(p->spin_lock);

	return self;
}

@end
