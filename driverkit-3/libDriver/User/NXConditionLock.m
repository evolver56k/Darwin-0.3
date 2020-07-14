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
 * NXConditionLock.m. Lock object with exported condition variable, 
 *	user version.
 *
 * HISTORY
 * 01-Aug-91    Doug Mitchell at NeXT
 *      Created. 
 */


#import <machkit/NXLock.h>
#import <mach/cthreads.h>

@implementation NXConditionLock 

/*
 * _priv instance variable points to one of these.
 */
typedef struct {
	struct mutex		m;
	struct condition	c;
	int				locked;
	int 			conditionVar;
} conditionlock_priv_t;

- init
{
	conditionlock_priv_t *priv;

	[super init];
	priv = _priv = malloc(sizeof(conditionlock_priv_t));
	mutex_init(&priv->m);
	condition_init(&priv->c);
	priv->locked = 0;
	priv->conditionVar = 0;
	return self;
}

- initWith : (int)condition
{
	conditionlock_priv_t *priv;

	[self init];
	priv = _priv;
	priv->conditionVar = condition;
	return self;
}

- (int) condition
{
	conditionlock_priv_t *priv = _priv;

	return priv->conditionVar;
}

- free 
{
	conditionlock_priv_t *priv = _priv;

	mutex_clear(&priv->m);
	condition_clear(&priv->c);
	free(priv);
	return [super free];
}

// enter critical region, regardless of conditionVar
- lock
{
	conditionlock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	while (priv->locked)
		condition_wait(&priv->c, &priv->m);	// block nicely
	priv->locked = TRUE;
	mutex_unlock(&priv->m);
	return self;
}

// leave critical region, regardless of conditionVar
- unlock	// leave critical region, regardless of conditionVar
{
	conditionlock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	priv->locked = FALSE;
	mutex_unlock(&priv->m);
	condition_broadcast(&priv->c);	// let everyone check conditionVar
	return self;
}

// enter critical section only upon condition
- lockWhen : (int)value	// enter critical section only upon condition
{
	conditionlock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	while(priv->locked || priv->conditionVar != value)
		condition_wait(&priv->c, &priv->m);
	priv->locked = TRUE;
	mutex_unlock(&priv->m);
	return self;
}

// leave critical section, establishing condition 'value'
- unlockWith : (int)value 
{
	conditionlock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	priv->conditionVar = value;
	priv->locked = FALSE;
	mutex_unlock(&priv->m);
	condition_broadcast(&priv->c);	// let everyone check condition
	return self;
}

@end
