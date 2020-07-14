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
 
#import <driverkit/return.h>
#import <mach/mach.h>
#import <mach/vm_param.h>
#import <driverkit/IODisk.h>
#import <driverkit/IODiskRwDistributed.h>
#import <driverkit/generalFuncs.h>
#import <kernserv/queue.h>

#define	SUPPORT_ASYNC	0

@interface IOStub: IODisk<IODiskReadingAndWriting, DiskRwDistributed>
{
	char	 	*stub_data;	// read/write data
	id		ioQLock;	// NXConditionLock. Protects ioQueue;
					//    I/O thread sleeps on this.
	queue_head_t	ioQueue;	// queue of IOBuf's
}

/* 
 * Condition variable states for  ioQLock.
 */
#define NO_WORK_AVAILABLE	0
#define WORK_AVAILABLE		1

/*
 * Init and free instances.
 */
- initStub : (int)unitNum;
- free;

@end
