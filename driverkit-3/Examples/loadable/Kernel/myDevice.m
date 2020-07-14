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
/*	IOStub.m	1.0	01/29/91	(c) 1991 NeXT   
 *
 * IOStub.m - Implementation for trivial I/O device subclass.
 *
 * HISTORY
 * 29-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import "myDevice.h"
#import <kernserv/queue.h>
#import <kernserv/prototypes.h>
#import <machkit/NXLock.h>
#import <architecture/nrw/io.h>
#import <architecture/nrw/dma_macros.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/driverServer.h>

static volatile void myDeviceThread(id deviceId);

/*
 * Communication between exported threads and I/O thread via a queue
 * of these structs.
 */
typedef struct {
	NXConditionLock		*lock;		// caller sleeps on this
	myDeviceOp		op;		// operation to perform
	IOReturn		status;		// status on completion
	queue_chain_t		link;
} cmdBuf;

/*
 * States of cmdBuf.lock and of qLock.
 */
typedef enum {
	LS_IDLE,
	LS_BUSY
} lockState;

@implementation myDevice

/*
 * Set device port. devicePort must be a port_t in the IOTask IPC space.
 * This is called from myHandler, which is NOT in the IOTask context, hence
 * convertPortToIOTask() was used to generate the devicePort passed in here.
 */
- (void)setDevicePort : (port_t)devicePort
{
	myDevicePort = devicePort;
}

/*
 * Initialize. We assume that myDevicePort is valid at this time.
 * Again, this is called from myHandler's context, so we start up an I/O 
 * thread in the IOTask to enable performing actual I/O using the hardware
 * represented by myDevicePort. 
 */
- init
{
	/*
	 * Initialize resources.
	 */
	queue_init(&ioQueue);
	qLock = [NXConditionLock alloc];
	[qLock initWith:LS_IDLE];
	IOForkThread((IOThreadFunc)myDeviceThread, self);
	
	/*
	 * Send an initialize command to the I/O thread.
	 */
	[self enqueueCmd:OP_INIT];
	
	return self;
}

/*
 * Shut down, free resources (but not ourself).
 */
- (void)shutDown
{
	[self enqueueCmd:OP_SHUTDOWN];
	[qLock free];
}

/*
 * Private methods.
 *
 * Send a cmdBuf to the I/O thread and wait for completion. This is the
 * means by which exported methods (-init, -shutDown) pass commands to
 * the I/O thread.
 */
- (IOReturn)enqueueCmd : (myDeviceOp)op
{	
	cmdBuf cmd;
	
	/*
	 * Create a cmdBuf.
	 */
	cmd.lock = [NXConditionLock alloc];
	[cmd.lock initWith:LS_BUSY];
	cmd.op = op;
	
	/*
	 * Enqueue on ioQueue.
	 */
	[qLock lock];
	queue_enter(&ioQueue, &cmd, cmdBuf *, link);
	[qLock unlockWith:LS_BUSY];
	
	/*
	 * Wait for completion.
	 */
	[cmd.lock lockWhen:LS_IDLE];
	[cmd.lock free];
	return cmd.status;
}

/*
 * Initialize device, called from within the I/O thread.
 */
- (IOReturn)deviceInit
{
	IODevicePage *devicePage;
	IOReturn drtn;
	unsigned intr;
	
	/*
	 * Map in device Page.
	 */
	drtn = _IOMapDevicePage(myDevicePort,
		task_self(),
		(vm_offset_t *)&devicePage,
		YES,				// anywhere
		IO_CacheOff);
	if(drtn) {
		IOLog("myDevice: _IOMapDevicePage() returned %s\n", 
		    	[self stringFromReturn: drtn]);
		return IO_R_NO_MEMORY;
	}
	
	/*
	 * Make sure we can access device page.
	 */
	set_chan_intr_set(devicePage, 0, 0x5a);
	intr = chan_intr_cause(devicePage, 0);
	IOLog("myDevice: channel interrrupt cause 0x%x\n", intr);
		
	/*
	 * This should clear all cause bits.
	 */
	set_chan_intr_cause(devicePage, 0, 0xffff);
	intr = chan_intr_cause(devicePage, 0);
	IOLog("myDevice: channel interrrupt cause 0x%x\n", intr);
	set_chan_intr_mask(devicePage, 0, 0x55aa);
	intr = chan_intr_mask(devicePage, 0);
	IOLog("myDevice: channel interrrupt mask  0x%x\n", intr);
	return IO_R_SUCCESS;
}


/*
 * The I/O thread which performs all of the useful work of this example.
 */ 
static volatile void myDeviceThread(myDevice *deviceId)
{
	cmdBuf *cmdp;
	BOOL die = NO;
	queue_head_t *ioQ = &deviceId->ioQueue;
	
	IOLog("myDevice: I/O thread starting\n");
	
	while(1) {
	
		/* 
		 * Wait for something to do.
		 */
		[deviceId->qLock lockWhen:LS_BUSY];
		cmdp = (cmdBuf *)queue_first(ioQ);
		queue_remove(ioQ, cmdp, cmdBuf *, link);
		
		/*
		 * Leave qLock in appropriate state, depending on contents
		 * of queue.
		 */
		if(queue_empty(ioQ)) {
			[deviceId->qLock unlockWith:LS_IDLE];
		}
		else {
			[deviceId->qLock unlockWith:LS_BUSY];
		}
		
		/*
		 * Execute command.
		 */
		switch(cmdp->op) {
		    case OP_INIT:
		    	cmdp->status = [deviceId deviceInit];
			break;
			
		    case OP_SHUTDOWN:
		    	/*
			 * Kill this thread, but return good status first.
			 */
			die = YES;
			cmdp->status = IO_R_SUCCESS;
		}
		
		/*
		 * Return cmdBuf to caller.
		 */
		[cmdp->lock lock];
		[cmdp->lock unlockWith:LS_IDLE];
		
		/*
		 * Terminate if necessary.
		 */
		if(die) {
			IOLog("myDriver; I/O thread terminating\n");
			IOExitThread();
		}
	}
	
	/* Not reached */
}
