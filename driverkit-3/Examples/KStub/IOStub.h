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
/*	IOStub.h	1.0	01/29/91	(c) 1991 NeXT   
 *
 * IOStub.h - Interface for trivial I/O device subclass.
 *
 * HISTORY
 * 29-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/deviceCommon.h>
#import <driverkit/libIO.h>
#import <driverkit/IODiskDevice.h>
#import <kernserv/queue.h>

#define NUM_IOSTUBS	2

@interface IOStub : IODiskDevice<DiskDeviceRw>
{
	char	 	*stub_data;		/* read/write data */
	queue_head_t	ioQueue;		/* queue of IOBuf_t's for
						 * I/O thread */
	id		queueLock;		/* NXConditionLock. I/O thread
						 *    waits on this for work
						 *    to do. 
						 */
}

/*
 * Probe 'hardware' for specified unit. 
 */
+ stubProbe			: (int)Unit;

/*
 * Init and free instances.
 */
- (int)stubInit			: (int)Unit;

- free;

- (int)initBufs;


@end
