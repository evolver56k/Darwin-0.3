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
 * IOStubThread.m - IOStub Device Thread support.
 *
 * HISTORY
 * 04-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <mach/cthreads.h>
#import <driverkit/IODevice.h>
#import "IOStub.h"
#import "IOStubThread.h"
#import "IOStubDevice.h"
#import "IOStubUxpr.h"
#import <bsd/libc.h>
#import <machkit/NXLock.h>

@implementation IOStub(Thread)

/*
 * Enqueue an IOBuf on IOQueue and wake up anyone who might be waiting 
 * for something to do.
 */
- (void)enqueueIoBuf : (IOBuf_t *)buf
{
	xpr_stub("enqueueIoBuf: IOBuf 0x%x\n", buf, 2,3,4,5);
	[ioQLock lock];
	queue_enter(&ioQueue, buf, IOBuf_t *, link);
	[ioQLock unlockWith:WORK_AVAILABLE];
}

/*
 * Get an IOBuf from ioQueue. Blocks if necessary.
 */
- (IOBuf_t *)dequeueIoBuf
{
	IOBuf_t *buf;
	
	xpr_stub("dequeueIoBuf: entry\n", 1,2,3,4,5);
	[ioQLock lockWhen:WORK_AVAILABLE];
	buf = (IOBuf_t *)queue_first(&ioQueue);
	queue_remove(&ioQueue, buf, IOBuf_t *, link);
	if(queue_empty(&ioQueue))
		[ioQLock unlockWith:NO_WORK_AVAILABLE];
	else
		[ioQLock unlockWith:WORK_AVAILABLE];
	return buf;
}

@end

/*
 * I/O thread. This handles IOBufs which have been enqueued by 
 * exported methods (like read: and write:).
 *
 * This thread merely loops doing the  following:
 *    -- get an IOBuf off of the IOQueue.
 *    -- perform the task specified. 
 *    -- if async request, ioComplete: the result, else 
 *	 condition_signal() the waiting thread.
 */
 
volatile void IOStub_thread(IOStub *stubId)
{
	IOBuf_t *IOBuf;
	
	xpr_stub("IOStub_thread: starting\n", 1,2,3,4,5);
	while(1) {
		IOBuf = [stubId dequeueIoBuf];	
		xpr_stub("IOStub_thread: IOBuf 0x%x received\n", IOBuf,
			2,3,4,5); 
	
		switch(IOBuf->cmd) {	
		    case STUB_READ:
			[stubId deviceRead:IOBuf];
			break;	    
		    case STUB_WRITE:
			[stubId deviceWrite:IOBuf];
			break;	    
		    case STUB_ZERO:
			[stubId deviceZero:IOBuf];
			break;	    
		    case STUB_ABORT:
		    	/*
			 * I/O complete this request before we die.
			 */
			[IOBuf->waitLock unlockWith:YES];
			IOExitThread();
			break;
		}			
	}
	/* NOT REACHED */
}

/* end of IOThread.m */
