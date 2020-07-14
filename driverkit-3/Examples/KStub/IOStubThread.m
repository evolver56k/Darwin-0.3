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
/*	IOStubThread.m		1.0	02/04/91	(c) 1991 NeXT   
 *
 * IOStubThread.m - IOStub I/O Thread and associated methods.
 *
 * HISTORY
 * 04-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <driverkit/IODevice.h>
#import "IOStub.h"
#import "IOStubThread.h"
#import "IOStubPrivate.h"
#import <driverkit/DiskDeviceKern.h>
#import <bsd/dev/ldd.h>
#import <machkit/NXLock.h>

@implementation  IOStub(Thread)

/*
 * Enqueue an IOBuf on IOQueue, wake up I/O thread. Called by exported
 * methods in order to make I/O requests of the I/O thread.
 */
- (void)enqueueIoBuf : (IOBuf *)buf
{
	xpr_stub("enqueueIoBuf: IOBuf 0x%x\n", buf, 2,3,4,5);
	[queueLock lock];
	queue_enter(&ioQueue, buf, IOBuf *, ioChain);
	[queueLock unlockWith:WORK_AVAILABLE];
}

/*
 * The rest of the methods in this module run in the I/O thread.
 */
 
/*
 * Get an IOBuf from IOQueue. Blocks if necessary.
 */
- (IOBuf *)dequeueIoBuf
{
	IOBuf *buf;
	
	xpr_stub("dequeueIoBuf\n", 1,2,3,4,5);
	[queueLock lockWhen:WORK_AVAILABLE];
	buf = (IOBuf *)queue_first(&ioQueue);
	queue_remove(&ioQueue, buf, IOBuf *, ioChain);
	if(queue_empty(&ioQueue))
		[queueLock unlockWith:NO_WORK_AVAILABLE];
	else
		[queueLock unlockWith:WORK_AVAILABLE];
	return buf;
}

/*
 * Methods which do the actual "I/O" of this device.
 */
/*
 * These are the methods which actually do the work of this device. All
 * of these methods run in IOStub_thread.
 */
- (void)deviceRead : (IOBuf *)IOBuf
{
	char *source;
		
	xpr_stub("deviceRead:\n", 1,2,3,4,5);
	source = stub_data + (IOBuf->offset * [self blockSize]);
	bcopy(source, IOBuf->buf, IOBuf->bytesReq);
	IOBuf->bytesXfr = IOBuf->bytesReq;
	IOBuf->status = IO_R_SUCCESS;
	if(IOBuf->pending) {
		/*
		 * Async I/O. Notify client.
		 */
		[self diskIoComplete:IOBuf->pending
			status:IO_R_SUCCESS
			bytesXfr:IOBuf->bytesXfr];
		IOFree(IOBuf, sizeof(*IOBuf));
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		[IOBuf->waitLock lock];
		[IOBuf->waitLock unlockWith:YES];
	}
}

- (void)deviceWrite : (IOBuf *)IOBuf
{
	char *dest;
	
	xpr_stub("deviceWrite:\n", 1,2,3,4,5);
	dest = stub_data + IOBuf->offset * [self blockSize];
	
	bcopy(IOBuf->buf, dest, IOBuf->bytesReq);
	IOBuf->bytesXfr = IOBuf->bytesReq;
	IOBuf->status = IO_R_SUCCESS;
	if(IOBuf->pending) {
		/*
		 * Async I/O. Notify client.
		 */
		[self diskIoComplete:IOBuf->pending
			status:IO_R_SUCCESS
			bytesXfr:IOBuf->bytesXfr];
		IOFree(IOBuf, sizeof(*IOBuf));
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		[IOBuf->waitLock lock];
		[IOBuf->waitLock unlockWith:YES];
	}
}


/*
 * I/O thread. This is forked off in the stubInit: method. This handles
 * IOBufs which have been enqueued by exported methods.
 *
 * This thread merely loops doing the following:
 *    -- get an IOBuf off of the IOQueue.
 *    -- perform the task specified. 
 *    -- if async request, ioComplete: the result, else 
 *	 notify the waiting thread via IOBuf.waitLock.
 */
volatile void IOStub_thread(id stubp)
{
	IOStub *stub = stubp;
	IOBuf *IOBuf;
	
	xpr_stub("IOStub_thread: starting\n", 1,2,3,4,5);
	while(1) {
		IOBuf = [stub dequeueIoBuf];	
		xpr_stub("IOStub_thread: IOBuf 0x%x received\n", IOBuf,
			2,3,4,5); 
	
		switch(IOBuf->cmd) {	
		    case STUB_READ:
			[stub deviceRead:IOBuf];
			break;	    
		    case STUB_WRITE:
			[stub deviceWrite:IOBuf];
			break;	    
		    case STUB_ABORT:
		    	/* 
			 * Time for us to exit. First I/O complete
			 * this request, then exit. 
			 */
			IOBuf->status = IO_R_SUCCESS;
			[IOBuf->waitLock lock];
			[IOBuf->waitLock unlockWith:YES];
			IOExitThread();

		    default:
		    	printf("IOStub_thread: BOGUS IOBuf\n");
		    	break;
		}			
	}
	/* NOT REACHED */
}

@end

/* end of IOStubThread.m */
