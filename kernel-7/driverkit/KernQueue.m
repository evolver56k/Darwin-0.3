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

#import <driverkit/KernQueue.h>
#import <driverkit/KernQueuePrivate.h>

@implementation KernQueue

- init
{
    [super init];
    
    _first = _last = self;
    
    return self;
}

- free
{
    if (_first != self)		/* !isEmpty */
    	return self;

    return [super free];
}

- first
{
    if (_first == self)		/* isEmpty */
    	return nil;
	
    return _first;
}

- last
{
    if (_last == self)		/* isEmpty */
    	return nil;
	
    return _last;
}

- dequeueFirst
{
    id		entry;

    if (_first == self)		/* isEmpty */
    	return nil;

    entry = _first;
    QEPrev(QENext(entry)) = self;
    _first = QENext(entry);
   
    QENext(entry) = QEPrev(entry) = nil;
   
    return entry;
}

- dequeueLast
{
    id		entry;
    
    if (_last == self)		/* isEmpty */
    	return nil;

    entry = _last;
    QENext(QEPrev(entry)) = self;
    _last = QEPrev(entry);
   
    QENext(entry) = QEPrev(entry) = nil;
    
    return entry;
}

- dequeueFirstObject
{
    id		entry;

    if (_first == self)		/* isEmpty */
    	return nil;

    entry = _first;
    QEPrev(QENext(entry)) = self;
    _first = QENext(entry);
   
    QENext(entry) = QEPrev(entry) = nil;
   
    return QEObject(entry);
}

- dequeueLastObject
{
    id		entry;
    
    if (_last == self)		/* isEmpty */
    	return nil;

    entry = _last;
    QENext(QEPrev(entry)) = self;
    _last = QEPrev(entry);
   
    QENext(entry) = QEPrev(entry) = nil;
    
    return QEObject(entry);
}

- enqueueObjectAsFirst:		object
{
}

- enqueueObjectAsLast:		object
{
}

- (BOOL)isEmpty
{
    return _first == self;
}

@end

@implementation KernQueueEntry

- initForQueue:		queue
    withObject:		object
{
    [super init];
    
    _queue = queue;
    _object = object;
    
    _next = _prev = nil;
    
    return self;
}

- free
{
    if (_next != nil)		/* isQueued */
    	return self;
	
    return [super free];
}

- freeObject
{
    return [_object free];
}

- enterAsFirst
{
    return [self enterAsFirstOnQueue:_queue];
}

- enterAsFirstOnQueue:	queue
{
    if (_next != nil || queue == nil)	/* isQueued || noQueue */
    	return nil;

    _queue = queue;

    _next = QFirst(queue);
    _prev = queue;
    QEPrev(_next) = self;
    QFirst(queue) = self;
    
    return self;
}

- enterAsLast
{
    return [self enterAsLastOnQueue:_queue];
}

- enterAsLastOnQueue:	queue
{
    if (_next != nil || queue == nil)	/* isQueued || noQueue */
    	return nil;

    _queue = queue;

    _next = queue;
    _prev = QLast(queue);
    QENext(_prev) = self;
    QLast(queue) = self;
    
    return self;
}

- removeFromQueue
{
    if (_next != nil) {			/* isQueued */
    	QEPrev(_next) = _prev;
	QENext(_prev) = _next;
	_next = _prev = nil;
    }
    
    return self;
}

- insertAfterEntry:	prevEntry
{
    if (_prev != nil || QENext(prevEntry) == nil)	/* isQueued ||
    							prevEntry !isQueued */
    	return nil;
	
    _queue = QEQueue(prevEntry);

    _next = QENext(prevEntry);
    _prev = prevEntry;
    QEPrev(QENext(prevEntry)) = self;
    QENext(prevEntry) = self;
    
    return self;
}

- insertBeforeEntry:	nextEntry
{
    if (_next != nil || QEPrev(nextEntry) == nil)	/* isQueued ||
    							nextEntry !isQueued */
    	return nil;

    _queue = QEQueue(nextEntry);

    _next = nextEntry;
    _prev = QEPrev(nextEntry);
    QENext(QEPrev(nextEntry)) = self;
    QEPrev(nextEntry) = self;
    
    return self;
}

- next
{
    if (_next == _queue)	/* endOfQueue */
    	return nil;

    return _next;
}

- prev
{
    if (_prev == _queue)	/* endOfQueue */
    	return nil;

    return _prev;
}

- nextObject:	(id *)nextEntry
{
    if (_next == nil || _next == _queue) {	/* endOfQueue */
    	*nextEntry = nil;
	return nil;
    }
    
    *nextEntry = _next;
    
    return QEObject(_next);
}

- prevObject:	(id *)prevEntry
{
    if (_prev == nil || _prev == _queue) {	/* endOfQueue */
    	*prevEntry = nil;
	return nil;
    }
    
    *prevEntry = _prev;
    
    return QEObject(_prev);
}

- object
{
    return _object;
}

- queue
{
    return _queue;
}

- (BOOL)isQueued
{
    return _next != nil;
}

@end

KernQueueEntry *
KernQueueFirst(
    KernQueue		*queue
)
{
    if (QFirst(queue) == queue)
    	return nil;
	
    return QFirst(queue);
}

KernQueueEntry *
KernQueueLast(
    KernQueue		*queue
)
{
    if (QLast(queue) == queue)
    	return nil;
	
    return QLast(queue);
}

KernQueueEntry *
KernQueueNext(
    KernQueueEntry	*entry
)
{
    if (QENext(entry) == QEQueue(entry))
    	return nil;
	
    return QENext(entry);
}

KernQueueEntry *
KernQueuePrev(
    KernQueueEntry	*entry
)
{
    if (QEPrev(entry) == QEQueue(entry))
    	return nil;

    return QEPrev(entry);
}

void *
KernQueueEntryObject(
    KernQueueEntry	*entry
)
{
    return QEObject(entry);
}

void *
KernQueueNextObject(
    KernQueueEntry	**entry
)
{
    KernQueueEntry	*next = QENext(*entry);

    if (next == nil || next == QEQueue(*entry))
    	return nil;
	
    *entry = next;
    
    return QEObject(next);
}

void *
KernQueuePrevObject(
    KernQueueEntry	**entry
)
{
    KernQueueEntry	*prev = QEPrev(*entry);

    if (prev == nil || prev == QEQueue(*entry))
    	return nil;

    *entry = prev;
    
    return QEObject(prev);
}

BOOL
KernQueueIsEmpty(
    KernQueue		*queue
)
{
    return QFirst(queue) == queue;
}

BOOL
KernQueueEntryIsQueued(
    KernQueueEntry	*entry
)
{
    return QENext(entry) != nil;
}
