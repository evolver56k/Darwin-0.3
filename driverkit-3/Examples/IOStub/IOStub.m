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
 
#import <bsd/sys/types.h>
#import <mach/mach.h>
#import <mach/vm_param.h>
#import <mach/mach_error.h>
#import <bsd/libc.h>
#import <driverkit/return.h>
#import <driverkit/IODevice.h>
#import "IOStub.h"
#import "IOStubThread.h"
#import "IOStubDevice.h"
#import "IOStubUxpr.h"
#import <machkit/NXLock.h>

@implementation IOStub

/*
 * Initialize current instance. Returns non-zero on error.
 */
- initStub : (int)unitNum
{
	int data_size;
	int i;
	char *cp;
	kern_return_t krtn;
	u_int block_size, dev_size;
	
	xpr_stub("IOStub : init: unitNum %d\n", unitNum, 2,3,4,5);
	

	[self setUnit:unitNum];
	if(unitNum == 0) {
		/*
		 * read/write.
		 */
		block_size = 0x20;
		dev_size = 0x1000;
		[self setName : "IOStub0"];
		[self setWriteProtected:NO];
	}
	else {
		/*
		 * Make this one read-only, non-queuing.
		 */
		block_size = 0x40;
		dev_size = 0x800;
		[self setName : "IOStub1"];
		[self setWriteProtected:YES];
	}
	[self setDeviceKind:"Stub Device"];
	[self setLocation:NULL];
	[self setDiskSize:dev_size];
	[self setBlockSize:block_size];
	[self setFormattedInternal:1];
	
	/* 
	 * allocate and initialize data for writing and reading.
	 */
	data_size = block_size * dev_size;
	krtn = vm_allocate(task_self(),
		(vm_address_t *)&stub_data,
		(vm_size_t)round_page(data_size), 
		YES);
	if(krtn != KERN_SUCCESS) {
		mach_error("***IOStub init: vm_allocate", krtn);
		return(self);
	}
	
	/*
	 * initial data: incrementing bytes for unit 0, decrementing
	 * bytes for others.
	 */
	cp = (char *)stub_data;
	for(i=0; i<data_size; i++) {
		if(unitNum)
			*cp++ = 0 - (char)i;
		else
			*cp++ = i;
	}
	 
	/*
	 * Set up our instance's IOQueue.
	 */
	ioQLock = [NXConditionLock alloc];
	[ioQLock initWith:NO_WORK_AVAILABLE];
	queue_init(&ioQueue);

	/*
	 * Start up an I/O thread to perform the actual work of this device.
	 */
	IOForkThread((IOThreadFunc)IOStub_thread, self);
			
	[self registerDevice];
	
	/*
	 * Let superclass take care of initializing inherited instance
	 * variables.
	 */
	[super init];
	return self;
}

/*
 * Free resources consumed by this instance. 
 */
- free
{
	int data_size;
	kern_return_t krtn;
	u_int block_size, dev_size;
	IOBuf_t *IOBuf;
		
	/*
	 * Kill the I/O thread.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_ABORT;
	IOBuf->pending   = 0;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	[self enqueueIoBuf:IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->waitLock lockWhen:YES];
	[IOBuf->waitLock unlock];
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(IOBuf_t));
	
	/*
	 *  Free up our VM.
	 */
	block_size = [self blockSize];
	dev_size = [self diskSize];
	data_size = block_size * dev_size;
	krtn = vm_deallocate(task_self(),
		(vm_address_t)stub_data,
		(vm_size_t)round_page(data_size));
	if(krtn != KERN_SUCCESS) {
		mach_error("***IOStub free: vm_deallocate", krtn);
	}
	return([super free]);
}

/*
 * These override the superclass's methods which are no-op stubs.
 */
 
- (int)deviceOpen:(u_int)intentions
{
	xpr_stub("IOStub deviceOpen: unit %d\n", [self unit], 2,3,4,5);
	return(IO_R_SUCCESS);
}

- (void)deviceClose
{
	xpr_stub("IOStub deviceClose: unit %d\n", [self unit], 2,3,4,5);
}

/* 
 * These are the exported I/O methods, from the 
 * IODiskDeviceReadingAndWriting category. Their main task is:
 * 
 * -- verify valid input parameters.
 * -- set up and enqueue an IOBuf for servicing by the I/O thread.
 * -- Wait for I/O complete if synchronous request.
 */

- (IOReturn) readAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
{
	IOReturn mrtn;
	char *source;
	IOBuf_t *IOBuf;
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("IOStub read: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self diskSize];

	/*
	 * Verify legal parameters.
	 */
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub read: invalid offset/byte count\n", 
			1,2,3,4,5);
		return(IO_R_INVALID_ARG);
	}

	/*
	 * OK, queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_READ;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->bytesXfr  = actualLength;
	IOBuf->status 	 = IO_R_INVALID;
	IOBuf->pending   = 0;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	IOBuf->dirRead	 = 1;
	[self enqueueIoBuf:IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->waitLock lockWhen:YES];
	[IOBuf->waitLock unlock];
	xpr_stub("stub read: DONE\n", 1,2,3,4,5);
	mrtn = IOBuf->status;
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(*IOBuf));
	return(mrtn);
}

- (IOReturn) readAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	char *source;
	IOBuf_t *IOBuf;
	unsigned block_size;
	unsigned dev_size;
	
	xpr_stub("IOStub readAsync: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self diskSize];

	/*
	 * Verify legal parameters.
	 */
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub read: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALID_ARG);
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
	IOBuf->pending   = pending;
	IOBuf->dirRead	 = 1;
	[self enqueueIoBuf:IOBuf];
	
	xpr_stub("stub readAsync: DONE\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

- (IOReturn) writeAt		: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (unsigned *)actualLength
{
	IOReturn mrtn;
	char *dest;
	IOBuf_t *IOBuf;
	unsigned block_size;
	unsigned dev_size;
	
	xpr_stub("IOStub write: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self diskSize];

	/*
	 * Verify legal parameters.
	 */
	if([self isWriteProtected]) {
		return IO_R_NOT_WRITABLE;
	}
	dest = stub_data + offset * block_size;
	if((dest + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub dest: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALID_ARG);
	}

	/*
	 * OK, queue up an IOBuf.
	 */
	IOBuf            = IOMalloc(sizeof(*IOBuf));
	IOBuf->cmd       = STUB_WRITE;
	IOBuf->offset    = offset;
	IOBuf->bytesReq	 = length;
	IOBuf->buf       = buffer;
	IOBuf->bytesXfr  = actualLength;
	IOBuf->status 	 = IO_R_INVALID;
	IOBuf->pending   = 0;
	IOBuf->waitLock  = [NXConditionLock alloc];
	[IOBuf->waitLock initWith:NO];
	IOBuf->dirRead	 = 0;
	[self enqueueIoBuf:IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->waitLock lockWhen:YES];
	[IOBuf->waitLock unlock];
	xpr_stub("stub write: DONE\n", 1,2,3,4,5);
	mrtn = IOBuf->status;
	[IOBuf->waitLock free];
	IOFree(IOBuf, sizeof(*IOBuf));
	return(mrtn);
}

- (IOReturn) writeAsyncAt	: (unsigned)offset 
				  length : (unsigned)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending;
{
	char *source;
	IOBuf_t *IOBuf;
	unsigned block_size;
	unsigned dev_size;
	
	xpr_stub("IOStub writeAsync: unit %d  offset 0x%x  length 0x%x\n",
		[self unit], offset, length, 4,5);
		
	block_size = [self blockSize];
	dev_size = [self diskSize];

	/*
	 * Verify legal parameters.
	 */
	if([self isWriteProtected]) {
		return IO_R_NOT_WRITABLE;
	}
	source = stub_data + offset * block_size;
	if((source + length) > (stub_data + (block_size * dev_size))) {
	   
	    	xpr_stub("IOStub write: invalid offset/byte count\n",
			1,2,3,4,5);
		return(IO_R_INVALID_ARG);
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
	IOBuf->pending   = pending;
	IOBuf->dirRead	 = 0;
	[self enqueueIoBuf:IOBuf];
	
	xpr_stub("stub writeAsync: DONE\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

/*
 * Distributed Objects R/W protocol.
 */
- (IOReturn) readAt		: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (out IOData **)data
				  actualLength : (out unsigned *)actualLength
{
	IOReturn rtn;
	IOData *ldata = [[IOData alloc] initWithSize:length];
	
	/*
	 * FIXME - when freeFlag is set to YES (which is what we really 
	 * want), this method crashes with the following error messages
	 * when run with the DiskTest app:
	 *
	 *   finishEncoding: send error -101:invalid memory
	 *   uncaught exception : 11001
	 *   objc: FREED(id): message free sent to freed object=0x9c01c
	 * 
	 * When run with the test programs in ../tests, everything works 
	 * OK...
	 *
	 * When we set freeFlag to NO, everything works, but the IOStub
	 * server leaks memory.
	 *
	 * Update 15-Oct-92 - this error only happens when moving at least
	 * one entire page of data. Moving less than a page is OK.
	 */
	[ldata setFreeFlag:YES];
	rtn = [self readAt:offset
		length:length
		buffer:[ldata data]
		actualLength:actualLength];
	if(*actualLength) {
		*data = ldata;
	}
	
	/*
	 * FIXME - when does the IOData get deallocated??
	 */
	return rtn;
}

/*
 * DiskRwDistributed protocol.
 */
- (IOData *)rreadAt		: (unsigned)offset
				  length : (unsigned)length
				  rtn : (out IOReturn *)rtn
				  actualLength : (out unsigned *)actualLength
{
	IOReturn lrtn;
	IOData *ldata = [[IOData alloc] initWithSize:length];
	
	[ldata setFreeFlag:YES];
	lrtn = [self readAt:offset
		length:length
		buffer:[ldata data]
		actualLength:actualLength];
	*rtn = lrtn;
	return ldata;
}
	
- (IOReturn) readAsyncAt	: (unsigned)offset
				  length : (unsigned)length
				  ioKey : (unsigned)ioKey
				  caller : (id)caller
{
	/* TBD */
	return IO_R_UNSUPPORTED;
}
				  
			  
- (IOReturn) writeAt		: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (in IOData *)data
				  actualLength : (out unsigned *)actualLength
{
	IOReturn rtn;
	
	if([data size] < length) {
		IOLog("%s writeAt: insufficient memory in IOData\n",
			[self name]);
	}
	rtn = [self writeAt:offset
			length:length
			buffer:[data data]
			actualLength:actualLength];
	[data free]; 
	return rtn;
}

- (IOReturn)writeAsyncAt	: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (in IOData *)data
				  ioKey : (unsigned)ioKey
				  caller : (id)caller
{
	return IO_R_UNSUPPORTED;
}
@end

