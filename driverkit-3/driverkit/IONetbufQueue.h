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
 * Class which implements a fifo queue of Netbufs.
 *
 * HISTORY
 *
 * 2 Feb 1993 David E. Bohman at NeXT
 *	Created.
 */

#ifdef	KERNEL

#import <objc/Object.h>
#import <bsd/net/netbuf.h>

/*
 * A request to enqueue a Netbuf
 * when there are already maxCount
 * Netbufs queued, causes the new
 * Netbuf to be freed without notice.
 *
 * Freeing an IONetbufQueue instance
 * causes any queued Netbufs to be freed.
 */
@interface IONetbufQueue:Object
{
@private
    struct _queueEntry {
	struct _queueEntry
				*_next;
    }		*_queueHead,
    		*_queueTail;
    unsigned	_queueCount;
    unsigned	_maxCount;
}

- initWithMaxCount:(unsigned)maxCount;

- (unsigned)count;		// returns number of Netbufs queued

- (unsigned)maxCount;

- (void)enqueue:(netbuf_t)nb;

- (netbuf_t)dequeue;

@end

#endif	KERNEL
