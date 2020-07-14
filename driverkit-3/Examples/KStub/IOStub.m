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
 
#import "IOStub.h"
#import "IOStubThread.h"
#import "IOStubPrivate.h"
#import <bsd/sys/types.h>
#import <driverkit/deviceCommon.h> 
#import <driverkit/libIO.h>
#import <mach/mach_user_internal.h>
#import <bsd/sys/param.h>
#import <mach/vm_param.h>
#import <bsd/dev/ldd.h>
#import <mach/mach_interface.h>
#import <machkit/NXLock.h>

#define STUB_BLOCK_SIZE	1024

@implementation IOStub

/*
 * Probe hardware for specified unit. 
 */
+ stubProbe		:(int)Unit
{
	id	stubId;
	int 	rtn;
	
	xpr_stub("stubProbe unit %d\n", Unit, 2,3,4,5);
	if(Unit >= NUM_IOSTUBS) {
		IOLog("stubProbe: bogus Unit (%d)\n", Unit);
		return(nil);
	}
	stubId = [self alloc];
	if(rtn = [stubId stubInit:Unit])  {
		xpr_stub("stubProbe: stubInit returned %d; aborting\n", 
			rtn, 2,3,4,5);
		[stubId free];
		return nil;
	}
	
	[stubId registerDevice];
	return(stubId);
}

/*
 * Initialize current instance. Returns non-zero on error.
 */
- (int)stubInit		:(int)Unit
{
	u_int block_size, dev_size;
	
	xpr_stub("stubInit Unit %d\n", Unit, 2,3,4,5);
	[self setUnit:Unit];
	if(Unit == 0) {
		/*
		 * read/write.
		 */
		[self setWriteProtected:NO];
		block_size = STUB_BLOCK_SIZE;
		dev_size = 0x1000;
		[self setDeviceName : "IOStub0"];
	}
	else {
		/*
		 * Make this one read-only.
		 */
		[self setWriteProtected:YES];
		block_size = STUB_BLOCK_SIZE;
		dev_size = 0x800;
		[self setDeviceName : "IOStub1"];
	}
	[self setDeviceType:"Stub Device"];
	[self setDriveName:"Stub Device"];
	[self setLocation:NULL];
	[self setBlockSize:block_size];
	[self setDeviceSize:dev_size];
	[self setFormattedInt:1];
	
	 
	/*
	 * Set up our instance's IOQueue.
	 */
	queueLock = [NXConditionLock new];
	[queueLock initWith:NO_WORK_AVAILABLE];
	queue_init(&ioQueue);

	/*
	 * Start up an I/O thread to perform the actual work of this device.
	 */
	IOForkThread((IOThreadFcn)IOStub_thread, self);
		
	[self initBufs];
		
	/*
	 * Let superclass take care of initializing inherited instance
	 * variables.
	 */
	[super init];
	xpr_stub("stubInit returning success\n", 1,2,3,4,5);
	return(0);
}

/*
 * Free resources consumed by this instance. 
 */
- free
{
	int data_size;
	unsigned block_size, dev_size;
	IOBuf *IOBuf;
	
	/*
	 * Kill the I/O thread.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_ABORT;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	IOBuf->device	 = self;
	[self enqueueIoBuf:IOBuf];
	[IOBuf->waitLock lockWhen:YES];
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(*IOBuf));

	/*
	 * Free our "device" memory..
	 */
	block_size = [self blockSize];
	dev_size = [self deviceSize];
	data_size = block_size * dev_size;
	IOFree(stub_data, data_size);
	[queueLock free];
	return([super free]);
}

/*
 * Initialize "device memory".
 */
- (int)initBufs
{
	int 		data_size;
	char 		*cp;
	int		i;
	
	/*
	 * allocate and initialize data for writing and reading.
	 */
	data_size = [self blockSize] * [self deviceSize];
	stub_data = IOMalloc(data_size);
	if(stub_data == NULL) {
		IOLog("***IOStub init: IOMalloc returned NULL");
		return 1;
	}
	
	/*
	 * initial data: incrementing bytes for unit 0, decrementing
	 * bytes for others.
	 */
	cp = stub_data;
	for(i=0; i<data_size; i++) {
		if([self unit])
			*cp++ = 0 - (char)i;
		else
			*cp++ = i;
	}
	return 0;
}

/* 
 * These are the exported I/O methods, from the DiskDeviceRw category. 
 * Their main task is:
 * 
 * -- verify valid input parameters.
 * -- set up and enqueue an IOBuf for servicing by the I/O thread.
 * -- Wait for I/O complete if synchronous request.
 */

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (out unsigned char *)buffer
				  actualLength : (out unsigned *)actualLength 
				  client : (vm_task_t)client
{
	IOReturn mrtn;
	char *source;
	IOBuf *IOBuf;
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("IOStub read: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self deviceSize];
	
	/*
	 * Verify legal parameters.
	 */
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub read: invalid offset/byte count\n", 
			1,2,3,4,5);
		return(IO_R_INVALIDARG);
	}

	/*
	 * OK, queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_READ;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->bytesXfr  = 0;
	IOBuf->status 	 = IO_R_INVALID;
	IOBuf->pending   = NULL;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->client	 = IOVmTaskSelf();
	[self enqueueIoBuf:IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->waitLock lockWhen:YES];
	xpr_stub("stub read: DONE\n", 1,2,3,4,5);
	mrtn = IOBuf->status;
	*actualLength = IOBuf->bytesXfr;
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(*IOBuf));
	return(mrtn);
}

- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (out unsigned char *)buffer
				  pending : (unsigned)pending
				  client : (vm_task_t)client				  
{
	char *source;
	IOBuf *IOBuf;
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("IOStub readAsync: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self deviceSize];

	/*
	 * Verify legal parameters.
	 */
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub read: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALIDARG);
	}

	/*
	 * Just queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_READ;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = (void *)pending;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->client	 = client;
	[self enqueueIoBuf:IOBuf];
	
	xpr_stub("stub readAsync: DONE\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (in unsigned char *)buffer
				  actualLength : (out unsigned *)actualLength 
				  client : (vm_task_t)client
{
	IOReturn mrtn;
	char *dest;
	IOBuf *IOBuf;
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("IOStub write: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self deviceSize];

	/*
	 * Verify legal parameters.
	 */
	if([self writeProtected]) {
		return IO_R_NOTWRITEABLE;
	}
	dest = stub_data + offset * block_size;
	if((dest + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub dest: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALIDARG);
	}

	/*
	 * OK, queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_WRITE;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->status 	 = IO_R_INVALID;
	IOBuf->pending   = NULL;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 0;
	IOBuf->client	 = IOVmTaskSelf();
	[self enqueueIoBuf:IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->waitLock lockWhen:YES];
	xpr_stub("stub write: DONE\n", 1,2,3,4,5);
	mrtn = IOBuf->status;
	*actualLength = IOBuf->bytesXfr;
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(*IOBuf));
	return(mrtn);
}


- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (in unsigned char *)buffer
				  pending : (unsigned)pending
				  client : (vm_task_t)client
{
	char *source;
	IOBuf *IOBuf;
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("IOStub writeAsync: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self deviceSize];

	if([self writeProtected]) {
		return IO_R_NOTWRITEABLE;
	}
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub write: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALIDARG);
	}

	/*
	 * Just queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_WRITE;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = (void *)pending;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 0;
	IOBuf->client	 = client;
	[self enqueueIoBuf:IOBuf];
	
	xpr_stub("stub writeAsync: DONE\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

@end

