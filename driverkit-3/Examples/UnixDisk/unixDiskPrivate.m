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
/*	unixDiskPrivate.m	1.0	02/07/91	(c) 1991 NeXT   
 *
 * unixDiskPrivate.m - unixDisk Private methods.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <mach/cthreads.h>
#import <bsd/libc.h>
#import <bsd/sys/file.h>
#import <driverkit/IODevice.h>
#import "unixDisk.h"
#import "unixThread.h"
#import "unixDiskPrivate.h"
#import <bsd/dev/disk.h>
#import "unixDiskUxpr.h"
#import <errno.h>
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>

@implementation unixDisk(Private)

/*
 * readCommon: and writeCommon: are invoked by the exported flavors of
 * read: and write:. 
 */
- (IOReturn) readCommon : (u_int)block 
	 	length : (u_int)length 
		buffer : (void *)bufp
		actualLength : (u_int *)actualLength 		/* returned */
		pending : (void *)pending
		caller : (id)caller
		IOBufRtn : (IOBuf_t **)IOBufRtn
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	u_int block_size, dev_size;
	
	/*
	 * We have to 'fault in' a possible non-present disk in 
	 * order to get its physical parameters...
	 */
	mrtn = [self isDiskReady:TRUE];
	switch(mrtn) {
	    case IO_R_SUCCESS:
	    	break;
	    case IO_R_NO_DISK:
		xpr_err("%s readCommon: disk not present\n", 
			[self name], 2,3,4,5);
		return(mrtn);
	    default:
	    	IOLog("%s readCommon: bogus return from isDiskReady"
			" (%s)\n",
			[self name], [self stringFromReturn:mrtn]);
		return(mrtn);
	}
	if(![self isFormatted])
		return(IO_R_UNFORMATTED);

	/*
	 * Verify legal parameters.
	 */
	block_size = [self blockSize];
	dev_size = [self diskSize];
	if(block >= dev_size) {
	    	xpr_ud("unixDisk readCommon: invalid "
			"deviceBlock/byte count\n", 1,2,3,4,5);
		return(IO_R_INVALID_ARG);
	}

	if(length % block_size) {
		xpr_ud("unixDisk readCommon: byteCount not block_size "
			"multiple\n", 1,2,3,4,5);
		return(IO_R_INVALID_ARG); 
	}
	
	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(unixDeviceRead:threadNum:);
	IOBuf->offset    = block;
	IOBuf->bytesReq  = length;
	IOBuf->buf       = bufp;
	IOBuf->bytesXfr  = actualLength;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = pending;
	IOBuf->caller 	 = caller;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	*IOBufRtn	 = IOBuf;
	if(!pending) {
		IOBuf->wait_lock = [NXConditionLock alloc];
		[IOBuf->wait_lock initWith:FALSE];
	}
	else {
		IOBuf->wait_lock = nil;
	}
	[self enqueueIoBuf:IOBuf needs_disk:TRUE];
	return(IO_R_SUCCESS);
}

- (IOReturn) writeCommon : (u_int)block 
	 	length : (u_int)length 
		buffer : (void *)bufp
		actualLength : (u_int *)actualLength 		/* returned */
		pending : (void *)pending
		caller : (id)caller			// for DO only
		IOBufRtn : (IOBuf_t **)IOBufRtn
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	u_int block_size, dev_size;
	
	/*
	 * We have to 'fault in' a possible non-present disk in 
	 * order to get its physical parameters...
	 */
	mrtn = [self isDiskReady:TRUE];
	switch(mrtn) {
	    case IO_R_SUCCESS:
	    	break;
	    case IO_R_NO_DISK:
		xpr_err("%s writeCommon: disk not present\n", 
			[self name], 2,3,4,5);
		return(mrtn);
	    default:
	    	IOLog("%s writeCommon: bogus return from isDiskReady"
			" (%s)\n",
			[self name], [self stringFromReturn:mrtn]);
		return(mrtn);
	}

	/*
	 * Verify access and legal parameters.
	 */
	if([self isWriteProtected]) {
		return IO_R_NOT_WRITABLE;
	}
	block_size = [self blockSize];
	dev_size = [self diskSize];
	if(block >= dev_size) {
	    	xpr_ud("unixDisk writeCommon: invalid "
			"deviceBlock/byte count\n", 1,2,3,4,5);
		return(IO_R_INVALID_ARG);
	}
	if(length % block_size) {
		xpr_ud("unixDisk writeCommon: byteCount not block_size "
			"multiple\n", 1,2,3,4,5);
		return(IO_R_INVALID_ARG);
	}

	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(unixDeviceWrite:threadNum:);
	IOBuf->offset    = block;
	IOBuf->bytesReq  = length;
	IOBuf->buf       = bufp;
	IOBuf->bytesXfr  = actualLength;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = pending;
	IOBuf->caller 	 = caller;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	*IOBufRtn	 = IOBuf;
	if(!pending) {
		IOBuf->wait_lock = [NXConditionLock alloc];
		[IOBuf->wait_lock initWith:FALSE];
	}
	else {
		IOBuf->wait_lock = nil;
	}
	[self enqueueIoBuf:IOBuf needs_disk:TRUE];
	return(IO_R_SUCCESS);
}

- (IOBuf_t *)allocIOBuf
{
	return(IOMalloc(sizeof(IOBuf_t)));
}

- (void)freeIOBuf:(IOBuf_t *)IOBuf
{
	if(IOBuf->wait_lock != nil) {
		/* 
		 * This was used for async I/O; free these resources...
		 */
		[IOBuf->wait_lock free];
	}
	IOFree(IOBuf, sizeof(*IOBuf));
}

/*
 * Wakeup thread waiting on IOBuf.
 */
- (void)IOBufDone : (IOBuf_t *)IOBuf
		    status : (IOReturn)status
{
	[IOBuf->wait_lock lock];
	IOBuf->status = status;
	[IOBuf->wait_lock unlockWith:TRUE];
}	

/*
 * These methods are invoked by a unix_thread; they do the actual work of
 * reading and writing.
 */
- (void)unixDeviceRead : (IOBuf_t *)IOBuf
	threadNum:(int)threadNum
{
	int rtn;
	int position;
	IOReturn status;
	u_int block_size;
	
	xpr_uth("unix deviceRead: thread %d offset %d\n", 
		threadNum, IOBuf->offset, 3,4,5);
	
	/*
	 * lseek to appropriate place.
	 */
	block_size = [self blockSize];
	position = IOBuf->offset * block_size;
	rtn = lseek(unix_fd[threadNum], position, L_SET);
	if(rtn != position) {
		xpr_uth("unixDisk deviceRead lseek(): errno %d\n", errno,
			2,3,4,5);
		status = errno_to_mio(errno);
		goto done;
	}

	/* 
	 * Go for it.
	 */
	xpr_uth("unix deviceRead read(): thread %d offset %d\n", 
		threadNum, IOBuf->offset, 3,4,5);
	rtn = read(unix_fd[threadNum], IOBuf->buf, IOBuf->bytesReq);
	if(rtn > 0) {
		*IOBuf->bytesXfr = rtn;
		status = IO_R_SUCCESS;
	}
	else 
		status = errno_to_mio(errno);
done:
	/*
	 * Any time we successfully complete a read or a write, we go to 
	 * "formatted" state.
	 */
	if(status == IO_R_SUCCESS)
		[self setFormattedInternal:1];
	if(IOBuf->pending) {
		/*
		 * Async I/O. Notify client.
		 */
		IOData *rdata = [[IOData alloc] initWithData:IOBuf->buf
			size:*IOBuf->bytesXfr
			dealloc:NO];
		
		[rdata setFreeFlag:NO];
		[IOBuf->caller readIoComplete:(unsigned)IOBuf->pending	
					// ioKey
			device:self
			data:rdata
			status:IOBuf->status];
		[rdata free];
		[self freeIOBuf:IOBuf];
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		[self IOBufDone:IOBuf status:status];
	}
	xpr_uth("unixDeviceRead thread %d: DONE, status %s\n",
		threadNum, [self stringFromReturn:IOBuf->status], 3,4,5);
}

- (void)unixDeviceWrite : (IOBuf_t *)IOBuf
	threadNum:(int)threadNum
{
	int rtn;
	int position;
	IOReturn status;
	u_int block_size;
	
	xpr_uth("unix deviceWrite: thread %d offset %d\n", 
		threadNum, IOBuf->offset, 3,4,5);

	/*
	 * lseek to appropriate place.
	 */
	block_size = [self blockSize];
	position = IOBuf->offset * block_size;
	rtn = lseek(unix_fd[threadNum], position, L_SET);
	if(rtn != position) {
		xpr_uth("unixDisk deviceWrite lseek(): errno %d\n", errno,
			2,3,4,5);
		status = errno_to_mio(errno);
		goto done;
	}
	
	/*
	 * Go for it.
	 */
	rtn = write(unix_fd[threadNum], IOBuf->buf, IOBuf->bytesReq);
	if(rtn > 0) {
		*IOBuf->bytesXfr = rtn;
		status = IO_R_SUCCESS;
	}
	else 
		status = errno_to_mio(errno);
done:
	/*
	 * Any time we successfully complete a read or a write, we go to 
	 * "formatted" state.
	 */
	if(status == IO_R_SUCCESS)
		[self setFormattedInternal:1];
	if(IOBuf->pending) {
		/*
		 * Async I/O. Notify client.
		 */
		[IOBuf->caller writeIoComplete:(unsigned)IOBuf->pending
			device:self
			actualLength:*IOBuf->bytesXfr
			status:IOBuf->status];
		[self freeIOBuf:IOBuf];
	}
	else {
		/*
		 * Synchronous I/O. Notify sleeping thread.
		 */
		[self IOBufDone:IOBuf status:status];
	}		
	xpr_uth("unixDeviceWrite thread %d: DONE, status %s\n",
		threadNum, [self stringFromReturn:IOBuf->status], 3,4,5);
}

- (void)deviceEject	: (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum
{
	int rtn;
	IOReturn status;
	
	xpr_uth("unixDisk deviceEject: thread %d\n", threadNum, 2,3,4,5);
	
	/*
	 * Wait until we're the only thread doing disk access.
	 */
	[IOQueue.ejectLock lockWhen:1];
	[IOQueue.ejectLock unlock];
	
	rtn = ioctl(unix_fd[threadNum], DKIOCEJECT, NULL);
	if(rtn)
		status = errno_to_mio(errno);
	else
		status = IO_R_SUCCESS;
	
	/*
	 * We go to unformatted state until we successfully complete a 
	 * read or a write.
	 */
	[self setFormattedInternal:0];
	IOQueue.ejectPending = FALSE;
	[self IOBufDone:IOBuf status:status];
	xpr_uth("unixDeviceEject thread %d: DONE, status %s\n",
		threadNum, [self stringFromReturn:IOBuf->status], 3,4,5);
}

/*
 * Requested disk is not available. ioComplete everything in this
 * device's q_disk queue with IO_R_NO_DISK status.
 */
- (void)unixDeviceAbort : (IOBuf_t *)IOBuf
{
	IOBuf_t *abortBuf;
	queue_head_t *q = &IOQueue.q_disk;
	
	xpr_uth("unixDeviceAbort\n", 1,2,3,4,5);
	[IOQueue.qlock lock];
	while(!queue_empty(q)) {
		abortBuf = (IOBuf_t *)queue_first(q);
		queue_remove(q, abortBuf, IOBuf_t *, ioChain);
		
		/*
		 * Note we don't change condition variable of qlock...
		 */
		[IOQueue.qlock unlock];
		xpr_uth("unixDeviceAbort: ABORTING IOBuf 0x%x\n", abortBuf,
			2,3,4,5);
		[self IOBufDone:abortBuf status:IO_R_NO_DISK];
		[IOQueue.qlock lock];
	}
	[IOQueue.qlock unlock];
	
	/* 
	 * One more thing - I/O complete the abort command itself.
	 */
	[self IOBufDone:IOBuf status:IO_R_SUCCESS];
}

/*
 * See if disk is present. If we got this far, it is; otherwise this request
 * would have been ioComplete'd in unixDeviceAbort:.
 */
- (void)deviceProbe	: (IOBuf_t *)IOBuf
{
	[self IOBufDone:IOBuf status:IO_R_SUCCESS];
}

/*
 * Test ready. We do this by trying to do a new open , with O_NDELAY. If that
 * succeeds, it's ready.
 */
- (void)deviceCheckReady : (IOBuf_t *)IOBuf
			   threadNum:(int)threadNum
{
	int newfd;
	IODiskReadyState *readyState = (IODiskReadyState *)IOBuf->buf;
	
	newfd = open(unix_name, O_RDONLY|O_NDELAY, 0);
	if(newfd <=0) {
		*readyState = IO_NotReady;
		xpr_uth("deviceCheckReady: NOT READY\n", 1,2,3,4,5);
	}
	else {
		*readyState = IO_Ready;
		close(newfd);
		xpr_uth("deviceCheckReady: READY\n", 1,2,3,4,5);
	}
	[self IOBufDone:IOBuf status:IO_R_SUCCESS];
}

@end
