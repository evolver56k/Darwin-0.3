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
/*	unixDisk.h	1.0	01/31/91	(c) 1991 NeXT   
 *
 * unixDisk.h - Exported Interface for unix Disk superclass. Provides IODevice
 *	        interface for current "/dev" disks.
 *
 * HISTORY
 * 31-Jan-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <bsd/sys/types.h>
#import <driverkit/return.h>
#import <driverkit/IODevice.h>
#import <driverkit/IODisk.h>
#import <driverkit/IODiskRwDistributed.h>
#import "unixDiskTypes.h"
#import <driverkit/volCheck.h>

#define MAX_IO_SIZE	(64 * 1024)
#define MAX_IO_THREADS	8

@interface unixDisk:IODisk<IODiskReadingAndWriting,
	IOPhysicalDiskMethods, DiskRwDistributed>
{
	int 		unix_fd[MAX_IO_THREADS];// unix file descriptor for
						// each thread
	IOQueue_t	IOQueue;		// queue of IOBuf's; this is
						// work to be done by 
						// unix_thread 
	int		_diskType;		// PR_DRIVE_FLOPPY, etc.
	char 		unix_name[20];		// "/dev/rsd0h", etc.
	
}

- unixInit			: (int)numThreads;
- (int)diskType;
- (void)setDiskType		: (int)type;
- (void) setLogicalDisk	: diskId;
- registerPartition		: partId;

@end

extern IOReturn errno_to_mio(int Errno);
