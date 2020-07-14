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
 * NXLock.m. Standard lock object, user version.
 *
 * HISTORY
 * 01-Aug-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <machkit/NXLock.h>
#import <mach/cthreads.h>

@implementation NXLock 

/*
 * _priv instance variable points to one of these.
 */
typedef struct {
	struct mutex		m;
	struct condition	c;
	BOOL			locked;
} lock_priv_t;

- init
{	
	lock_priv_t *priv;

	[super init];
	priv = _priv = malloc(sizeof(lock_priv_t));
	mutex_init(&priv->m);
	condition_init(&priv->c);
	priv->locked = FALSE;
	return self;
}

- free 
{
	lock_priv_t *priv = _priv;

	mutex_clear(&priv->m);
	condition_clear(&priv->c);
	free(priv);
	return [super free];
}

- lock 
{
	lock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	while(priv->locked)
		condition_wait(&priv->c, &priv->m);
	priv->locked = TRUE;
	mutex_unlock(&priv->m);	// allow others to acquire mutex & wait
	return self;
}

/*
 * acquire the mutex (quickly) and signal anyone in lock code above.
 * Note that we do not want or need condition_broadcast, since the one 
 * party to awake will reset locked and cause any other parties to
 * block in condition_wait again.
 */
- unlock 
{
	lock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	priv->locked = FALSE;
	condition_signal(&priv->c);
	mutex_unlock(&priv->m);	// assume that signal delivery occurred
	return self;
}
@end
