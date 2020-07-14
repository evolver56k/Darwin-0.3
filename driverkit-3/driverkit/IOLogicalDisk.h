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
 * IOLogicalDisk.h - Public interface for Logical Disk Object.
 *
 * HISTORY
 * 05-Mar-91    Doug Mitchell at NeXT
 *      Created. 
 *
 * IOLogicalDisk is an abstract superclass which conceptually overlays 
 * the IODiskDevice class and provides "logical partition" functionality. 
 * Unix File System I/O goes thru a subclass of IOLogicalDisk; other I/O 
 * usually goes thru a subclass of IODiskDevice (like SCSIDisk).
 *
 * The main functionality provided by an IOLogicalDisk is to map read and write
 * requests to a physical device which has diffferent physical 
 * characteristics (block size, offset of partitions, etc.) than the 
 * IOLogicalDisk.
 *
 * An IOLogicalDisk is always associated with an IODiskDevice which actually 
 * performs all of IOLogicalDisk's I/O; this IODiskDevice's id is kept in 
 * _physDevice. An IOLogicalDisk is always in the same task as its associated
 * _physDevice.
 * 
 * IODiskDevice's _logicalDisk instance variable is used to chain together all
 * of the IOLogicalDisks attached to a particular physDevice. In an 
 * IOLogicalDisk, logicalDisk points to the next IOLogicalDisk in a chain -
 * e.g., partition a's logicalDisk variable points to the IOLogicalDisk
 * instance for partition b. The chain is nil-terminated.
 */
 
#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#ifdef	KERNEL
#import <mach/mach_interface.h>
#else	KERNEL
#import <mach/mach.h>
#endif	KERNEL

@interface IOLogicalDisk:IODisk <IODiskReadingAndWriting>

/*
 * Some notes on IODisk instance variables:
 *
 * -- The formatted flag is a don't care for LogicalDisk and its subclasses.
 *    This flag is only for use by other device-specific subclasses of 
 *    IODiskDevice.
 *
 * -- The removable flag is always a copy of a LogicalDisk's physicalDisk's
 *    removable flag. This is set in the connectToPhysicalDisk method.
 */
{
@private
	id		_physicalDisk;		/* a pointer to the
						 * IODiskDevice which actually
						 * does our work */
	unsigned	_partitionBase;		/* offset from start of
						 * physDevice in
						 * blocSize blocks */
	unsigned	_physicalBlockSize;	/* blockSize of physDevice,
						 * cached here for efficiency
						 */
	BOOL		_instanceOpen;
	int		_IOLogicalDisk_reserved[4];
}

/*
 * Other methods Implemented by IOLogicalDisk.
 */
/*
 * Determine if this LogicalDisk (or any other subsequent logical disks in the
 * _nextLogicalDisk list) are currently open. Returns 0 if none open, else
 * 1.
 */
- (BOOL)isOpen;

/*
 * Determine if any logical disks in the _physicalDisk's entire 
 * _nextLogicalDisk chain except for self are open.
 */
- (BOOL)isAnyOtherOpen;

/*
 * Free self and any objects in logicalDisk chain.
 */
- free;
	
/*
 * Open a connection to physical device.
 */
- (IOReturn)connectToPhysicalDisk : physicalDisk;

/*
 * Get/set local instance variables.
 */
- (void)setPartitionBase	: (unsigned)partBase;
- physicalDisk;
- (void)setPhysicalBlockSize 	: (unsigned)size;
- (u_int)physicalBlockSize;
- (BOOL)isInstanceOpen;
- (void)setInstanceOpen		: (BOOL)isOpen;

/*
 * Passed on to _physDevice.
 */
- (IOReturn)isDiskReady		: (BOOL)prompt;

/* A read/write routine that reblocks if the physical device blocksize is larger than requested count. */
/* This must be a class method since it's called by a class method in DiskPartition. */

+ (IOReturn) commonReadWrite	: (id)physicalDisk
				: (BOOL)read		/* YES if read, NO if write */
				: (unsigned long long)offset 	/* byte offset */
				: (unsigned)length 		/* bytes */
				: (unsigned char *)buffer
				: (vm_task_t)client
				: (void *)pending
				: (u_int *)actualLength;

@end
