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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * Class which implements queues of Netbufs.
 *
 * HISTORY
 *
 * 2 Feb 1993 David E. Bohman at NeXT
 *	Created.
 */
 
#ifdef	KERNEL

#import <driverkit/IONetbufQueue.h>

@implementation IONetbufQueue

- init
{
    return [self initWithMaxCount:16];
}

- initWithMaxCount:(unsigned)maxCount
{
    [super init];

    _queueHead = _queueTail = 0;
    _queueCount = 0; _maxCount = maxCount;
    
    return self;
}

- free
{
    netbuf_t	nb;
    
    while (nb = [self dequeue])
    	nb_free(nb);
	
    return [super free];
}

- (unsigned)count
{
    return (_queueCount);
}

- (unsigned)maxCount
{
    return (_maxCount);
}

- (void)enqueue:(netbuf_t)nb
{
    struct _queueEntry	*qe = (struct _queueEntry *)nb;

    if (_queueCount < _maxCount) {
    	if (_queueCount++ > 0) {
	    _queueTail->_next = qe; _queueTail = qe;
	}
	else
	    _queueHead = _queueTail = qe;

    	qe->_next = 0;
    }
    else
    	nb_free(nb);
}

- (netbuf_t)dequeue
{
    struct _queueEntry	*qe;

    if (_queueCount > 0) {
    	qe = _queueHead; _queueHead = qe->_next;
	if (--_queueCount == 0)
	    _queueHead = _queueTail = 0;
	    
	qe->_next = 0;
    }
    else
    	qe = 0;
	
    return ((netbuf_t)qe);
}

@end

#endif	KERNEL
