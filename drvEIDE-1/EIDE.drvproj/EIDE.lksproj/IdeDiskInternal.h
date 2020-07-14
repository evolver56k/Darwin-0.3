/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdeDiskInt.h - private categories and typedefs for IdeDisk class.
 *
 * HISTORY 
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */
 
#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_IDEDISKINTERNAL_H
#define _BSD_DEV_IDEDISKINTERNAL_H

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import "IdeDisk.h"
#import "IdeCnt.h"
#import "IdeKernel.h"


/*
 * Condition variable states for ioQueueLock.
 */
#define NO_WORK_AVAILABLE	0
#define WORK_AVAILABLE		1

/*
 * Kernel-specific types.
 */
#define NUM_IDE_DEV		(MAX_IDE_DRIVES * MAX_IDE_CONTROLLERS)	// 4

#define NUM_IDE_PART		8
#define IDE_LIVE_PART		(NUM_IDE_PART-1)


typedef struct {
    /*
     * One per unit. Note that the physDevice (live partition), the raw
     * device, and the block devices for a given disk all share the same
     * physbuf. Block devices don't use physbuf; arbitration for access to
     * physbuf by the raw and live devices is done by physio(). 
     */
    struct buf *physbuf;   /* for phys I/O */

}       Ide_dev_t;

__private_extern__ void ide_init_idmap(id);
__private_extern__ IODevAndIdInfo *ide_idmap();

/*
 * General utility methods in IdeDiskInt.m.
 */
@interface IdeDisk(Internal)

/*
 * Print details about the drive. 
 */
-(void)printInfo:(ideIdentifyInfo_t *)ideIdentifyInfo unit:(unsigned int)unit;

/*
 * Probe for existing drive.
 */
- (BOOL)ideDiskInit:(unsigned int)diskUnit target:(unsigned int)unit;

- (IOReturn)initIdeDrive;

/*
 * One-time only initialization.
 */
- initResources:controller;

/*
 * Free up local resources. 
 */
- free;


/*
 * Alloc/free command buffers.
 */
- (ideBuf_t *)allocIdeBuf:(void *)pending;
- (void)freeIdeBuf:(ideBuf_t *)ideBuf;

/*
 * Common read/write routine.
 */
- (IOReturn) deviceRwCommon : (IdeCmd_t)command
		  block: (u_int) deviceBlock
		  length : (u_int)length
		  buffer : (void *)buffer
		  client : (vm_task_t)client
		  pending : (void *)pending
		  actualLength : (u_int *)actualLength;

/*
 * -- Enqueue an ideBuf_t on ioQueue<Disk,Nodisk>
 * -- wake up the I/O thread
 * -- wait for I/O complete (if ideBuf->pending == NULL)
 */
- (IOReturn)enqueueIdeBuf	: (ideBuf_t *)ideBuf;

/*
 * Either wake up the thread which is waiting on the ideCmdBuf, or send an 
 * ioComplete back to client. ideCmdBuf->status must be valid.
 */
- (void)ideIoComplete 		: (ideBuf_t *)ideBuf;

/*
 * Main command dispatch method.
 */
- (void)ideCmdDispatch 		: (ideBuf_t *)ideBuf;

- (IOReturn) ideXfrIoReq:(ideIoReq_t *)ideIoReq;
/*
 * Common read/write method.
 */
- (IOReturn)ideRwCommon	: (ideBuf_t *)ideBuf;

- (void)logRwErr 		: (const char *)errType	
			  block : (int)block
			 status : (ide_return_t)status
		       readFlag : (BOOL)readFlag;

/*
 * Unlock ioQLock, updating condition variable as appropriate.
 */
- (void)unlockIoQLock;

/*
 * Top-level I/O thread.
 */
volatile void ideThread(IdeDisk *idisk);

@end	/* IdeDisk(Internal) */

#endif	/* _BSD_DEV_IDEDISKINTERNAL_H */

#endif	/* DRIVER_PRIVATE */

