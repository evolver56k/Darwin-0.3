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

#import <objc/Object.h>

typedef struct _QueueChain {
    void	*next;
    void	*prev;
} _QueueChain;

@interface KernQueue : Object
{
@private
    _QueueChain	_head;
}

- first;
- last;

- dequeueFirst;
- dequeueLast;

- dequeueFirstObject;
- dequeueLastObject;

- enqueueObjectAsFirst: object;
- enqueueObjectAsLast: object;

- (BOOL)isEmpty;

@end

@interface KernQueueEntry : Object
{
@private
    _QueueChain	_link;
    id		_queue;
    id		_object;
}

- initForQueue: queue
    withObject: object;

- freeObject;

- enterAsFirst;
- enterAsFirstOnQueue: queue;
- enterAsLast;
- enterAsLastOnQueue: queue;

- removeFromQueue;
- insertAfterEntry: prevEntry;
- insertBeforeEntry: nextEntry;

- next;
- prev;
- nextObject: (id *)nextEntry;
- prevObject: (id *)prevEntry;

- object;
- queue;

- (BOOL)isQueued;

@end

KernQueueEntry *
KernQueueFirst(
	KernQueue	*queue
);
KernQueueEntry *
KernQueueLast(
	KernQueue	*queue
);
KernQueueEntry *
KernQueueNext(
	KernQueueEntry	*entry
);
KernQueueEntry *
KernQueuePrev(
	KernQueueEntry	*entry
);

void *
KernQueueEntryObject(
	KernQueueEntry	*entry
);
void *
KernQueueNextObject(
	KernQueueEntry	**entry		/* IN/OUT */
);
void *
KernQueuePrevObject(
	KernQueueEntry	**entry		/* IN/OUT */
);

BOOL
KernQueueIsEmpty(
	KernQueue	*queue
);
BOOL
KernQueueEntryIsQueued(
	KernQueueEntry	*entry
);

#endif
