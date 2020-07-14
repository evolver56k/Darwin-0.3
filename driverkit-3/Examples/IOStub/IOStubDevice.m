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
/*	IOStubDevice.m	1.0	02/07/91	(c) 1991 NeXT   
 *
 * IOStubDevice.m - Implementation of "Device-specific" IOStub functions.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
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

@implementation IOStub(Device)

/*
 * These are the methods which actually do the work of this device. All
 * of these methods run in the IOStub_thread thread.
 */
- (void)deviceRead : (IOBuf_t *)IOBuf
{
	char *source;
	u_int block_size;
	
	xpr_stub("deviceRead:\n", 1,2,3,4,5);
	block_size = [self blockSize];
	source = stub_data + IOBuf->offset * block_size;
	bcopy(source, IOBuf->buf, IOBuf->bytesReq);
	IOBuf->status = IO_R_SUCCESS;
	if(IOBuf->pending) {
		/*
		 * Async I/O. Notify client.
		 */
#if		SUPPORT_ASYNC
		[self ioComplete:IOBuf->pending
			dirRead : 1
			status : IO_R_SUCCESS
			bytesXfr : IOBuf->bytesReq];
		IOFree(IOBuf, sizeof(*IOBuf));
#else		SUPPORT_ASYNC
		IOLog("Bogus async request in deviceRead\n");
		exit(1);
#endif		SUPPORT_ASYNC
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		*IOBuf->bytesXfr = IOBuf->bytesReq;
		[IOBuf->waitLock unlockWith:YES];
	}
}

- (void)deviceWrite : (IOBuf_t *)IOBuf
{
	char *dest;
	u_int block_size;
	
	xpr_stub("deviceWrite:\n", 1,2,3,4,5);
	block_size = [self blockSize];
	dest = stub_data + IOBuf->offset * block_size;
	bcopy(IOBuf->buf, dest, IOBuf->bytesReq);
	IOBuf->status = IO_R_SUCCESS;
	if(IOBuf->pending) {
#if		SUPPORT_ASYNC
		/*
		 * Async I/O. Notify client.
		 */
		[self ioComplete:IOBuf->pending
			dirRead : 0
			status : IO_R_SUCCESS
			bytesXfr : IOBuf->bytesReq];
		IOFree(IOBuf, sizeof(*IOBuf));
#else		SUPPORT_ASYNC
		IOLog("Bogus async request in deviceZero\n");
#endif		SUPPORT_ASYNC
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		*IOBuf->bytesXfr = IOBuf->bytesReq;
		[IOBuf->waitLock unlockWith:YES];
	}
}

/*
 * Zero all of memory. No dispatch for this one...
 */
- (void)deviceZero : (IOBuf_t *)IOBuf
{
	u_int block_size;
	u_int dev_size;
	
	xpr_stub("deviceZero:\n", 1,2,3,4,5);
	block_size = [self blockSize];
	dev_size = [self diskSize];
	bzero(stub_data, block_size * dev_size);
	IOBuf->status = IO_R_SUCCESS;
	if(IOBuf->pending) {
#if		SUPPORT_ASYNC
		/*
		 * Async I/O. Notify client.
		 */
		[self ioComplete:IOBuf->pending
			dirRead : 1
			status : IO_R_SUCCESS
			bytesXfr : 0];
		IOFree(IOBuf, sizeof(*IOBuf));
#else		SUPPORT_ASYNC
		IOLog("Bogus async request in deviceZero\n");
#endif		SUPPORT_ASYNC
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		[IOBuf->waitLock unlockWith:YES];
	}
}


@end

