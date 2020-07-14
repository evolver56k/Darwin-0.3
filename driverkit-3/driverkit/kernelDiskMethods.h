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
 * kernelDiskMethods.h - Kernel devsw glue for IODisk class.
 *
 * HISTORY
 * 02-May-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import <kernserv/prototypes.h>
#import <bsd/sys/disktab.h>

#define	IO_DISK_UNIT(dev)	(minor(dev) >> 3)
#define	IO_DISK_PART(dev)	(minor(dev) & 0x7)	

@interface IODisk(kernelDiskMethods)

/*
 * Async I/O complete function. The void * argument is actually 
 * a struct buf *, but the rest of the code in IODisk (or its subclasses)
 * doesn't need to know that.
 */
- (void)completeTransfer		: (void *)iobuf 
			     withStatus : (IOReturn)status
			   actualLength : (unsigned)actualLength;

/*
 * Get/set IODevAndIdInfo pointer.
 */
- (IODevAndIdInfo *)devAndIdInfo;
- (void)setDevAndIdInfo			: (IODevAndIdInfo *)devAndIdInfo;

/*
 * Obtain dev_t associated with this instance.
 */
- (dev_t)blockDev;
- (dev_t)rawDev;

@end

