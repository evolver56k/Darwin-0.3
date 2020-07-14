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
 * ioThread.h  
 */
 
#import <mach/mach.h>
#import <kernserv/queue.h>
#import <machkit/NXLock.h>
#import <libc.h>
#import <driverkit/IODevice.h>
#import <driverkit/generalFuncs.h>

/*
 * Commands to be passed to I/O thread.
 */
typedef enum {
	IOC_GETNUM,		// get a number from stdin
	IOC_PRINTNUM,		// print a number
	IOC_QUIT		// exit
} ioCmd_t;

/*
 * This struct is the means by which exported methods pass commands to the
 * I/O thread.
 */

typedef struct {
	id		cmdLock;	// NXConditionLock. Clients await
					//   completion notification via 
					//   this lock.
	ioCmd_t		cmd;		// operation to perform
	int		aNumber;	// some data
	queue_chain_t	link;		// for enqueueing on ioQueue
} cmdBuf_t;

/*
 * Condition values for cmdLock.
 */
#define CMD_BUSY	0
#define CMD_COMPLETE	1

/*
 * Simple driver class.
 */
@interface MyDevice:IODevice
{
	id		ioQueueLock;	// NXConditionLock. Protects ioQueue;
					//   the ioThread awaits input via 
					//   this lock.
	queue_head_t	ioQueue;	// queue of cmdBuf_t's
	IOThread	ioThread;
}	

/*
 * Condition values for ioQueueLock.
 */
#define QUEUE_EMPTY	0
#define QUEUE_FULL	1		// at least one element in queue

/*
 * Initialization.
 */
- myDeviceInit;

/*
 * Exported run-time methods.
 */
- (int)getNumber;			// have I/O thread get a number 
					//    from user
- (void)printNumber : (int)ack;	// have I/O thread print a number
- free;

@end

