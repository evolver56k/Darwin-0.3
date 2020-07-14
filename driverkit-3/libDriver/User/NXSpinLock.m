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
 * NXSpinLock.m. Cheap lock object, user version.
 *
 * HISTORY
 * 22-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */


#import <machkit/NXLock.h>
#import <mach/cthreads.h>

@implementation NXSpinLock  

/*
 * _priv instance variable points to one of these.
 */
typedef struct {
	struct mutex		m;
} spinlock_priv_t;

- init
{
	spinlock_priv_t *priv = _priv = malloc(sizeof(spinlock_priv_t));

	[super init];
	mutex_init(&priv->m);
	return self;
}

- free {
	spinlock_priv_t *priv = _priv;

	mutex_clear(&priv->m);
	free(priv);
	return [super free];
}

- lock {
	spinlock_priv_t *priv = _priv;

	mutex_lock(&priv->m);
	return self;
}

- unlock {
	spinlock_priv_t *priv = _priv;

	mutex_unlock(&priv->m);
	return self;
}
@end

