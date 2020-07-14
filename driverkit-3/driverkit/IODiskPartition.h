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
 * IODiskPartition.h - interface for NeXT-style LogicalDisk.
 *
 * HISTORY
 * 01-May-91    Doug Mitchell at NeXT
 *      Created.
 *
 *
 * This IOLogicalDisk class handles all NeXT/Unix File system specific 
 * operations pertaining to a physical disk.
 *
 * For a given physical disk, there are two types of DiskObjects:
 *
 * -- The physical disk. This is a subclass of DiskObject like SCSIDisk.
 *    I/O to the Unix "live partition" is performed directly on the
 *    physical disk, as are device-specific ioctl's. 
 *
 * -- IODiskPartition partition instances. There is one of these per valid 
 *    partition in physicalDisk's label. There is always at least one 
 *    IODiskPartition partition, corresponding to partition 0, even if 
 *    there is no valid label or if the label has no partition 0. Common 
 *    ioctl-type operations like eject are performed on IODiskPartition
 *    partitions; this allows common error checking before (possibly) 
 *    passing the method on to device-specific code. All normal Unix 
 *    reads and writes (other than thru the live partition) are performed 
 *    on an IODiskPartition partition.
 *
 *    Certain operations result in freeing all IODiskPartition partitions 
 *    other than partition 0. These operations are: eject, writeLabel, and
 *    setFormatted. Each of these force us to discard our current partition
 *    state since they basically obliterate the current label. All of these
 *    have the following restrictions:
 *
 *	-- they must be performed on partition 0, since all other 
 *	   IODiskPartition partitions wil be freed.
 *	-- No block devices can be open. (Open block devices represent open
 *  	   file systems, for which these operations are catastrophic).
 *	-- No other IODiskPartition partitions may be open. 
 */

#import <driverkit/IOLogicalDisk.h>
#import <bsd/dev/disk_label.h>
#ifdef	KERNEL
#import <driverkit/kernelDiskMethods.h>
#import <bsd/dev/ldd.h>
#endif	KERNEL

#ifdef ppc //bknight - 12/3/97 - Radar #2004660
#define GROK_APPLE 1
#endif //bknight - 12/3/97 - Radar #2004660

/*
 * Public NXDisk methods.
 */
@protocol IODiskPartitionExported

/*
 * Read disk label.
 */
- (IOReturn) readLabel		: (out disk_label_t *)label_p;

/*
 * Write disk label.
 */
- (IOReturn) writeLabel		: (in disk_label_t *)label_p;

/*
 * Get/set "device open" flags.
 */
- (BOOL)isBlockDeviceOpen;
- (void)setBlockDeviceOpen	: (BOOL)openFlag;
- (BOOL)isRawDeviceOpen;
- (void)setRawDeviceOpen	: (BOOL)openFlag;

@end

@interface IODiskPartition:IOLogicalDisk <IODiskPartitionExported>

{
@private
	int 		_partition;		// like 3 LSB's of the old
						// UNIX minor number

	BOOL		_labelValid;		// label is valid

#ifdef GROK_APPLE //bknight - 12/3/97 - Radar #2004660
	BOOL		_hfsValid;		// true for HFS partitions w/o labels
#endif GROK_APPLE //bknight - 12/3/97 - Radar #2004660

	BOOL		_blockDeviceOpen;	// block device is open
	BOOL		_rawDeviceOpen;		// raw device is open
	unsigned char  	_physicalPartition;	// partition index in real map

	ns_time_t      	_probeTime;
	id		_partitionWaitLock;	// condition lock to wait
						// for probe of label
	int		_IODiskPartition_reserved[4];
}

+ (IODeviceStyle)deviceStyle;
+ (Protocol **)requiredProtocols;

/*
 * Examine a physical disk; create necessary instances of NXDisk.
 * Returns non-nil if any instances were created. 
 */
+ (BOOL)probe : (id)deviceDescription;

/*
 * Free all attached logicalDisks.
 */
- free;

/*
 * Stall for disk ready after probe
 */
- waitForProbe:(int) seconds;

@end
