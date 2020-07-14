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
 * NXLock.m. Standard lock object, kernel version.
 *
 * HISTORY
 * 01-Aug-91    Doug Mitchell at NeXT
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
    lock_t		sleep_lock;
};

@implementation NXLock 

- init
{
	struct _priv	*p = _priv;
	
	[super init];

	if (p == 0) {
	    p = (struct _priv *)kalloc(sizeof (struct _priv));
	    p->sleep_lock = lock_alloc();	

	    _priv = p;
	}
	
	lock_init(p->sleep_lock, TRUE);

	return self;
}

- free
{
	struct _priv	*p = _priv;

	if (p) {
	    lock_free(p->sleep_lock);
	    kfree(p, sizeof (struct _priv));
	}

	return [super free];
}


- lock 
{
	struct _priv	*p = _priv;

	lock_write(p->sleep_lock);

	return self;
}

- unlock 
{
	struct _priv	*p = _priv;

	lock_done(p->sleep_lock); 

	return self;
}

@end
