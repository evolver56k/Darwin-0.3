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
 * myDevice.h - Interface for trivial I/O device subclass.
 *
 * HISTORY
 * 18-Dec-92    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <driverkit/return.h>
#import <driverkit/IODevice.h>
#import <kernserv/queue.h>
#import <machkit/NXLock.h>

/*
 * Operations performed by the I/O thread.
 */
typedef enum {
	OP_INIT,
	OP_SHUTDOWN
} myDeviceOp;


@interface myDevice : IODevice
{
	port_t		myDevicePort;
	
	/*
	 * Commands are passed to the I/O thread via ioQueue, which is 
	 * protected with qLock. 
	 */
	queue_head_t	ioQueue;
	NXConditionLock	*qLock;
}

/*
 * Public methods, called from myHandler.
 */

/*
 * Set device port.
 */
- (void)setDevicePort : (port_t)devicePort;

/*
 * Initialize.
 */
- init;

/*
 * Shut down, free resources.
 */
- (void)shutDown;

/*
 * Private methods.
 *
 * Send a cmdBuf to the I/O thread and wait for completion.
 */
- (IOReturn)enqueueCmd : (myDeviceOp)op;

/*
 * Initialize device, called from within the I/O thread.
 */
- (IOReturn)deviceInit;

@end
