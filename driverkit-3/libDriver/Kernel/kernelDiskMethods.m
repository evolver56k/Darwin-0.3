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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * kernelDiskMethods.m - Kernel devsw glue for DiskObject class.
 *
 * HISTORY
 * 02-May-91    Doug Mitchell at NeXT
 *      Created. 
 */

/*
 * HACK ALERT
 * avoid including the code in signal.h to avoid the multiple definition of
 * __quad_word.
 */
#define _MACHINE_SIGNAL_ 1

#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import <driverkit/kernelDiskMethods.h>
#import <bsd/sys/buf.h>
#import <bsd/sys/errno.h>
#import <kern/assert.h>
#import <kernserv/prototypes.h>
#import <driverkit/generalFuncs.h>


#ifdef ppc //bknight - 12/3/97 - Radar #2004660
#define GROK_APPLE 1
#endif //bknight - 12/3/97 - Radar #2004660


@implementation IODisk(kernelDiskMethods)

/*
 * Async I/O complete function. Supercedes ioComplete method in 
 * libDriver/User/IODeviceDispatch.m. The void * argument is actually 
 * a struct buf *, but the rest of the code in DiskObject doesn't 
 * need to know that.
 */
- (void)completeTransfer		: (void *)iobuf 
			     withStatus : (IOReturn)status
			   actualLength : (unsigned)actualLength
{
	struct buf *bp = iobuf;
	
	if(status != IO_R_SUCCESS) {
		bp->b_flags |= B_ERROR;
	}
	bp->b_error = [self errnoFromReturn:status];
	bp->b_resid = bp->b_bcount - actualLength;
	biodone(bp);
}


/*
 * Get/set idMap pointer.
 */
- (IODevAndIdInfo *)devAndIdInfo
{
	return(_devAndIdInfo);
}

- (void)setDevAndIdInfo : (IODevAndIdInfo *)devAndIdInfo
{
	_devAndIdInfo = devAndIdInfo;
}

/*
 * Obtain dev_t associated with this instance. This does not take into 
 * account partitions, since they are a IODiskPartition construct.
 */
- (dev_t)blockDev
{
	return _devAndIdInfo->blockDev;
}

- (dev_t)rawDev
{
	return _devAndIdInfo->rawDev;
}

@end

@implementation IODisk(kernelDiskMethodsPrivate)

/*
 * Register either a LogicalDisk or a DiskObject subclass with the owner's
 * IODevAndIdInfo array.
 */
- (void)registerUnixDisk 	: (int) partition	
{
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( partition < ( 2 * NPART ) ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if(partition > (NPART-2)) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s registerUnixDisk: Bogus partition (%d)\n",
			[self name], partition);
		return;
	}
	if(_isPhysical) {
	    	_devAndIdInfo->liveId = self;
	}
	else {
	    	_devAndIdInfo->partitionId[partition] = self;
	}
}

/*
 * FIXME:
 * This needs some locking to prevent IO requests to nil IDs!
 */
- (void)unregisterUnixDisk 	:(int) partition	
{
#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if( ! ( partition < ( 2 * NPART ) ) ) {
#else GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	if(partition > (NPART-2)) {
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660
		IOLog("%s unregisterUnixDisk: Bogus partition (%d)\n",
			[self name], partition);
		return;
	}
	if(_isPhysical) {
	    	_devAndIdInfo->liveId = nil;
	}
	else {
	    	_devAndIdInfo->partitionId[partition] = nil;
	}
}

@end

