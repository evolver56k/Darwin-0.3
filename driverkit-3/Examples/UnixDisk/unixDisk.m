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
/*	unixDisk.m	1.0	01/31/91	(c) 1991 NeXT   
 *
 * unixDisk.m - Implementation for unix Disk superclass. Provides IODevice
 *	        interface for current "/dev" disks.
 *
 * HISTORY
 * 31-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <bsd/sys/types.h>
#import <mach/mach.h>
#import <mach/vm_param.h>
#import <mach/mach_error.h>
#import <bsd/libc.h>
#import <bsd/sys/file.h>
#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import "unixDisk.h"
#import "unixDiskPrivate.h"
#import "unixThread.h"
#import <errno.h>
#import "unixDiskUxpr.h"
#import <bsd/dev/disk.h>
#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>
#import <remote/NXConnection.h>
#import <objc/error.h>

#define FAKE_LIVE_PARTITION	1
#define NUM_API_THREADS		2

@implementation unixDisk

+initialize
{
	[super registerClass:self];
	return self;
}

/*
 * Common initialization. Device-specific parameters (dev_size, block_size)
 * must have been initialized before we're called by subclass's *init:
 * routines.
 */
- unixInit:(int)numThreads 
{
	int thread_num;
	struct drive_info drive_info;
	
	xpr_ud("unixInit: numThreads %d\n", numThreads, 2,3,4,5);
	
	/*
	 * Set up our instance's IOQueue.
	 */
	IOQueue.qlock = [NXConditionLock alloc];
	[IOQueue.qlock initWith:NO_WORK_AVAILABLE];
	queue_init(&IOQueue.q_disk);
	queue_init(&IOQueue.q_nodisk);
	IOQueue.device = self;	
	IOQueue.ejectLock = [NXConditionLock alloc];
	[IOQueue.ejectLock initWith:0];
	IOQueue.numDiskIos = 0;
	IOQueue.ejectPending = FALSE;
	
	/*
 	 * Log drive name.
	 */
	if(ioctl(unix_fd[0], DKIOCINFO, &drive_info)) {
		xpr_err("%s: unixInit: DKIOCINFO error\n", [self name],
			2,3,4,5);
	}
	else {
		IOLog("%s: %s\n", [self name], drive_info.di_name);
	}
	
	/*
	 * Start up some I/O threads to perform the actual work of this device.
	 */
	for(thread_num=0; thread_num<numThreads; thread_num++) {
		IOForkThread((IOThreadFunc)unix_thread, &IOQueue);
	}
	
	/*
	 * Init other instance variables.
	 */
	[self setLastReadyState:IO_Ready];
	[self setIsPhysical:YES];
	[super init];
	return self;
}

/*
 * Overrides IODisk method of same name; we do this to start up
 * NXConnections for IODiskPartition partitions attached to this physDevice.
 */
- (void) setLogicalDisk	: diskId
{
	if([self registerPartition:diskId] == nil) {
		return;
	}
	[super setLogicalDisk : diskId];
}

/*
 * Set up an NXConnection for ourself and start up threads to handle 
 * incoming I/O requests.
 * Called both from main(), for physical disk, and from setLogicalDisk:     
 * for IODiskPartition instances.
 */
- registerPartition: partId
{
	id roserver;
	int threadNum;
	id rtn = self;
	
	NX_DURING {
		roserver = [NXConnection registerRoot:partId
			    withName:[partId name]];
	} NX_HANDLER {
		IOLog("registerRoot raised %d\n", 
			NXLocalHandler.code);
		rtn = nil;
	} NX_ENDHANDLER

	if(rtn == nil) {
		return rtn;
	}
	if(roserver == nil) {
		IOLog("registerRoot returned nil\n");
		return nil;
	}
	
	/*
	 * Start up "API threads".
	 */
	for(threadNum=0; threadNum<NUM_API_THREADS; threadNum++) {
		NX_DURING {
			[roserver runInNewThread];
		} NX_HANDLER {
			IOLog("runInNewThread raised %d\n", 
				NXLocalHandler.code);
			rtn = nil;
		} NX_ENDHANDLER;
		if(rtn == nil) {
			break;
		}
	}
	return rtn;
}

/* 
 * IODiskDeviceReadingAndWriting protocol implementation. The main task of 
 * these methods is:
 * 
 * -- verify valid input parameters (can we skip this?)
 * -- set up and enqueue an IOBuf for servicing by the I/O thread.
 * -- Wait for I/O complete if synchronous request.
 */

- (IOReturn) readAt		: (u_int)offset 
				  length : (u_int)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (u_int *)actualLength
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	
	xpr_ud("unixDisk read: unit %d  offset 0x%x "
		"length 0x%x\n", [self unit], offset, length, 4,5);
		
	mrtn = [self readCommon : offset 
	 	length : length 
		buffer : buffer
		actualLength : actualLength 		/* returned */
		pending : 0
		caller : nil
		IOBufRtn : &IOBuf];
	
	/*
	 * Wait for I/O complete. 
	 */
	if(!mrtn) {
		[IOBuf->wait_lock lockWhen:TRUE];
		[IOBuf->wait_lock unlock];
		mrtn = IOBuf->status;
		[self freeIOBuf:IOBuf];
	}
	xpr_ud("unixDisk read: returning %d\n", mrtn, 2,3,4,5);
	return(mrtn);
}

- (IOReturn) readAsyncAt	: (u_int)offset 
				  length : (u_int)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	return IO_R_UNSUPPORTED;
}

- (IOReturn) writeAt		: (u_int)offset 
				  length : (u_int)length 
				  buffer : (unsigned char *)buffer
				  actualLength : (u_int *)actualLength
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	
	xpr_ud("unixDisk write: unit %d  offset 0x%x  "
		"length 0x%x\n", [self unit], offset, length, 4,5);
		
	mrtn = [self writeCommon : offset 
	 	length : length 
		buffer : buffer
		actualLength : actualLength			/* returned */
		pending : 0
		caller : nil
		IOBufRtn : &IOBuf];
	
	/*
	 * Wait for I/O complete.
	 */
	if(!mrtn) {
		[IOBuf->wait_lock lockWhen:TRUE];
		mrtn = IOBuf->status;
		[self freeIOBuf:IOBuf];
	}
	xpr_ud("unixDisk write: returning %d\n", mrtn, 2,3,4,5);
	return(mrtn);
}
				  	
- (IOReturn) writeAsyncAt	: (u_int)offset 
				  length : (u_int)length 
				  buffer : (unsigned char *)buffer
				  pending : (void *)pending
{
	return IO_R_UNSUPPORTED;
}

- (IODiskReadyState)updateReadyState
{
	IOBuf_t *IOBuf;
	IODiskReadyState readyState;
	
	xpr_ud("unixDisk updateReadyState: top\n", 1,2,3,4,5);
	
	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(deviceCheckReady:threadNum:);
	IOBuf->offset    = 0;
	IOBuf->bytesReq	 = 0;
	IOBuf->buf       = &readyState;
	IOBuf->bytesXfr  = NULL;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = 0;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->wait_lock = [NXConditionLock alloc];
	[IOBuf->wait_lock initWith:FALSE];
	[self enqueueIoBuf:IOBuf needs_disk:FALSE];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->wait_lock lockWhen:TRUE];
	readyState = *(IODiskReadyState *)IOBuf->buf;
	[self freeIOBuf:IOBuf];

	xpr_ud("unixDisk updateReadyState: DONE; state = %s\n", 
		IOFindNameForValue(readyState, readyStateValues), 2,3,4,5);
	return(readyState);
}

/*
 * IOPhysicalDiskMethods protocol methods.
 */
/*
 * Device-specific eject. Called by DiskObject's eject:.
 */
- (IOReturn) ejectPhysical
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	
	xpr_ud("unixDisk ejectPhysical: top\n", 1,2,3,4,5);

	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(deviceEject:threadNum:);
	IOBuf->offset    = 0;
	IOBuf->bytesReq	 = 0;
	IOBuf->buf       = NULL;
	IOBuf->bytesXfr  = NULL;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = 0;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->wait_lock = [NXConditionLock alloc];
	[IOBuf->wait_lock initWith:FALSE];
	[self enqueueIoBuf:IOBuf needs_disk:TRUE];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->wait_lock lockWhen:TRUE];
	mrtn = IOBuf->status;
	[self freeIOBuf:IOBuf];

	xpr_ud("unixDisk eject: DONE; status = %d\n", mrtn, 2,3,4,5);
	return(mrtn);
}

/*
 * Removable media support.
 */
 
/*
 * Called by volCheck thread when WS has told us that a requested disk is
 * not present. Pending I/Os which require a disk to be present must be 
 * aborted.
 */
- (void)abortRequest
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	
	xpr_ud("unixDisk abortRequest: top\n", 1,2,3,4,5);
	
	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(unixDeviceAbort:);
	IOBuf->offset    = 0;
	IOBuf->bytesReq	 = 0;
	IOBuf->buf       = NULL;
	IOBuf->bytesXfr  = NULL;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = 0;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->wait_lock = [NXConditionLock alloc];
	[IOBuf->wait_lock initWith:FALSE];
	[self enqueueIoBuf:IOBuf needs_disk:FALSE];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->wait_lock lockWhen:TRUE];
	mrtn = IOBuf->status;
	[self freeIOBuf:IOBuf];

	xpr_ud("unixDisk abortRequest: DONE; status = %d\n", mrtn, 2,3,4,5);
	return;
}

/*
 * Called by the volCheck thread when a transition to "ready" is detected.
 * Pending I/Os which require a disk may proceed.
 *
 * All we have to do is wake up the I/O threads.
 */
- (void)diskBecameReady
{
	[self ioThreadWakeup];
}

/*
 * Inquire if disk is present; if not, and 'prompt' is TRUE, ask for it. 
 * Returns IO_R_NO_DISK if:
 *    prompt TRUE, disk not present, and user cancels request for disk.
 *    prompt FALSE, disk not present.
 * Else returns IO_R_SUCCESS.
 */
- (IOReturn)isDiskReady	: (BOOL)prompt
{
	IOReturn mrtn;
	IOBuf_t *IOBuf;
	
	xpr_ud("%s: diskBecameReady\n", [self name], 2,3,4,5);
	if([self lastReadyState] == IO_Ready) {
		/*
		 * This one's easy...
		 */
		return(IO_R_SUCCESS);
	}
	if(prompt == NO) {
		return(IO_R_NO_DISK);
	}
	
	/*
	 * Queue up an IOBuf.
	 */
	IOBuf            = [self allocIOBuf];
	IOBuf->command   = @selector(deviceProbe:);
	IOBuf->offset    = 0;
	IOBuf->bytesReq	 = 0;
	IOBuf->buf       = NULL;
	IOBuf->bytesXfr  = NULL;
	IOBuf->status 	 = IO_R_INVALID;	/* must be filled in by
						 * I/O thread */
	IOBuf->pending   = 0;
	IOBuf->device	 = self;
	IOBuf->dirRead	 = 1;
	IOBuf->wait_lock = [NXConditionLock alloc];
	[IOBuf->wait_lock initWith:FALSE];
	[self enqueueIoBuf:IOBuf needs_disk:TRUE];
	
	/*
	 * Wait for I/O complete.
	 */
	[IOBuf->wait_lock lockWhen:TRUE];
	mrtn = IOBuf->status;
	[self freeIOBuf:IOBuf];

	xpr_ud("unixDisk isDiskReady: DONE; status = %d\n", mrtn, 2,3,4,5);
	return(mrtn);
}

/*
 * This must be implemented by subclass.
 */
- (IOReturn)updatePhysicalParameters
{
	IOLog("unixDisk updatePhysicalParameters: NOT IMPLEMENTED\n");
	return IO_R_UNSUPPORTED;
}

/*
 * unixDisk-specific Get/set methods.
 */
- (int)diskType
{
	return(_diskType);
}

- (void)setDiskType		: (int)type
{
	_diskType = type;
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
	 */
	[ldata setFreeFlag:NO];
	rtn = [self readAt:offset
		length:length
		buffer:[ldata data]
		actualLength:actualLength];
	if(*actualLength) {
		*data = ldata;
	}
	
	return rtn;
}

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
	/*
	 * TBD 
	 */
	return IO_R_UNSUPPORTED;
}
				  
			  
- (IOReturn) writeAt		: (unsigned)offset 		// in blocks
				  length : (unsigned)length 	// in bytes
				  data : (out IOData *)data
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

IOReturn errno_to_mio(int Errno) {

	IOReturn mrtn;
	
	switch(Errno) {
	    case 0:
	    	mrtn = IO_R_SUCCESS;
		break;
	    case EIO:
		mrtn = IO_R_IO;
		break;
	    case EPERM:
	    case EACCES:
		mrtn = IO_R_PRIVILEGE;
		break;
	    default:
	    	mrtn = IO_R_IO;
		break;
	}
	return(mrtn);
}

