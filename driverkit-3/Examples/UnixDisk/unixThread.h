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
/*	unixThread.h		1.0	02/07/91	(c) 1991 NeXT   
 *
 * unixThread.h - unixDisk device thread support.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#ifndef	_UNIX_THREAD_
#define _UNIX_THREAD_

#import <driverkit/return.h>
#import <objc/objc.h>
#import <mach/cthreads.h>
#import <kernserv/queue.h>
#import "unixDisk.h"


@interface unixDisk(Thread)

/*
 * Enqueue an IOBuf on an IOQueue and wake up anyone who might be waiting 
 * for the IOBuf.
 */
- (void) enqueueIoBuf : (IOBuf_t *)buf
		         needs_disk : (BOOL)needs_disk;

/*
 * Wakeup up I/O threads. Used for 'diskBecameReady' notification.
 */
- (void)ioThreadWakeup;

/*
 * Unlock IOQueue.qlock, updating condition variable as appropriate.
 */
- (void)unlockIOQueue;

@end

extern void unix_thread(IOQueue_t *queue);

#endif	_UNIX_THREAD_
