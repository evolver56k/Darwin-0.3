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
 * Kernel generic queue object(s).
 *
 * HISTORY
 *
 * 24 June 1994 ? at NeXT
 *	Created.
 */
 
#ifdef	KERNEL_PRIVATE

#import <driverkit/KernQueue.h>

typedef struct KernQueue_ {
    @defs(KernQueue)
} KernQueue_;

#define First		_head.next
#define Last		_head.prev

#define	_first		((id)First)
#define _last		((id)Last)

#define QFirst(q)	((KernQueue_ *)(q))->First
#define QLast(q)	((KernQueue_ *)(q))->Last

typedef struct KernQueueEntry_ {
    @defs(KernQueueEntry)
} KernQueueEntry_;

#define	Next		_link.next
#define Prev		_link.prev

#define _next		((id)Next)
#define _prev		((id)Prev)

#define QENext(e)	((KernQueueEntry_ *)(e))->Next
#define QEPrev(e)	((KernQueueEntry_ *)(e))->Prev
#define QEQueue(e)	((KernQueueEntry_ *)(e))->_queue
#define QEObject(e)	((KernQueueEntry_ *)(e))->_object

#endif
