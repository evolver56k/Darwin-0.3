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
#import <driverkit/i386/directDevice.h>

@implementation ExampleController

/*
 *  Probe, configure board and init new instance.
 */
+ probe:deviceDescription
{
	ExampleController	*example = [self alloc];

	/*
	 *  Probe this instance at the possible addresses for the Example
	 *  controller.  In this case, we ignore the configuration info,
	 *  which we should really check to be completely consistent.
	 */
	if (![example probeAtPortBase:0x1f0] && 
	    ![example probeAtPortBase:0x170]) {
	    	[example free];
		return nil;
	}

	return [example initFromDeviceDescription:deviceDescription];
}


- initFromDeviceDescription:deviceDescription
{
	/*
	 *  The call to [super init...] reserves all of our resources, or it
	 *  returns nil.
	 */
	if ([super initFromDeviceDescription:deviceDescription] == nil)
		return [super free];

	/* a bunch of driver specific stuff here... */

	/*
	 *  Now we're all set!  Just register ourselves, set some driverkit
	 *  stuff driverkit stuff, enable interrupts and go...
	 */
	[self setUnit:some unique number];
	[self setDeviceName:a unique name];
	[self setDeviceKind:"sc"];		/* a scsi controller */
	
	[self registerDevice];

	[self enableAllInterrupts];

	return self;
}


/*
 *  Can be called if init runs to completion.
 */
- free
{
	/* device specific stuff here */
	return [super free];
}


/*
 *  Override IODevice's attachInterrupt method so we can start the interrupt
 *  thread after our interrupt port has been allocated.
 */
- (void) attachInterrupt
{
	[super attachInterrupt];
	IOForkThread((IOThreadFcn)InterruptHanderThread, self);
}


/*
 *  Handle an interrupt.  Called in a thread context from the handler thread.
 */
- interrupt
{
	/* clear interrupts and do something meaningful */
	return self;
}

@end

static volatile void
InterruptHandlerThread(id self)
{
	kern_return_t krtn;
	msg_header_t *msgp = (msg_header_t *) IOMalloc(MSG_SIZE_MAX);

	while(1) {
		/*
		 * Wait for something to do.
		 */
		msgp->msg_local_port = [self interruptPort];
		msgp->msg_size = MSG_SIZE_MAX;	
		krtn = msg_receive(msgp, RCV_TIMEOUT, 5000);	/* XXX */
		if (krtn != KERN_SUCCESS)
			continue;
		
		/*
		 *  Hardware interrupt.  Dispatch.
		 */
		switch(msgp->msg_id) {
		    case INT_MSG_DEVICE:
			    [self interrupt];
			    break;
		    case INT_MSG_TIMEOUT:
		    	    [self timeout];
			    break;
		    default:
			    break;
		}
	}
}

