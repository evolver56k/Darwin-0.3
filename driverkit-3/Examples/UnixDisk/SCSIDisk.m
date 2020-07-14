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
/*	SCSIDisk.m	1.0	01/02/91	(c) 1991 NeXT   
 *
 * SCSIDisk.m - Implementation for SCSI Disk class. 
 *
 * HISTORY
 * 01-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import <bsd/sys/file.h>
#import "SCSIDisk.h"
#import <bsd/dev/scsireg.h>
#import <bsd/dev/disk.h>
#import <errno.h>
#import "unixDiskUxpr.h"
#import <driverkit/volCheck.h>

#define NUM_SCSI_THREADS	2
#define SCSI_RAW_NAME		"sd"

@implementation SCSIDisk

/*
 * Probe hardware to determine how many instances to alloc and init. This 
 * is a hard coded kludge for now.
 */
#if	0
+ (void)IOProbe:serverId
{
	int 	scsi_unit;
	id	scsiId = nil;
	
	/*
	 * Attempt to instantiate and init each possible scsi disk.
	 */
	for(scsi_unit=SCSI_UNIT_MIN; scsi_unit<=SCSI_UNIT_MAX; scsi_unit++) {
		if(scsiId == nil)
			scsiId = [self alloc];
		
		/*
		 * Avoid free/alloc if this init fails.
		 */
		if([scsiId SCSIInit:scsi_unit 
		    sender:serverId] == nil) 
			continue;
		else {
			/*
			 * Success. Have DiskObject superclass take care of 
			 * the rest.
			 */
			[scsiId registerDevice];
			[scsiId registerDisk];
			scsiId = nil;
		}
	}
	if(scsiId)
		[scsiId free];
}

#endif	0

/*
 * init one instance for specified disk.
 */
- SCSIInit:(int)diskNum
{
	char *name;
	int thread_num;
	
	xpr_ud("SCSIDisk : init: diskNum %d\n", diskNum, 2,3,4,5);
	
	[self setUnit:diskNum];
	name = malloc(10);
	sprintf(name, "%s%d", SCSI_RAW_NAME, diskNum);
	[self setName:(const char *)name];
	[self setDeviceKind:"Unix SCSI Disk"];
	[self setLocation:NULL];
	[self setDriveName:"Unix SCSI Disk"];
	[self setDiskType:PR_DRIVE_SCSI];
	[self setWriteProtected:0];
	
	/*
	 * Do a unix open  for each thread. All I/O here goes thru the live 
	 * partition.
	 */
	sprintf(unix_name, "/dev/rsd%dh", diskNum);
	for(thread_num=0; thread_num<NUM_SCSI_THREADS; thread_num++) {
		unix_fd[thread_num] = open(unix_name, O_RDWR, 0);
		if(unix_fd[thread_num] < 0) {
		
			/*
			 * Maybe it's read-only.
			 */
			unix_fd[thread_num] = open(unix_name, O_RDONLY, 0);
			if(unix_fd[thread_num] < 0) {
				perror("SCSIDisk open()");
				return(nil);
			}
			[self setWriteProtected:1];
		}
	}
	
	/*
	 * Let superclass take care of initializing inherited instance
	 * variables.
	 */
	[self unixInit:NUM_SCSI_THREADS];
	if([self updatePhysicalParameters])
		return nil;	
	return self;
}

/*
 * Get physical parameters (dev_size, block_size, etc.) from new disk.
 */
- (IOReturn)updatePhysicalParameters
{
#if	i386
	return(IO_R_IO);
#else
	int rtn;
	int formattedFlag;
	struct capacity_reply capacity;
	
	/*
	 * Is it formatted?
	 */
	[self setRemovable:0];		// fixme 
	rtn = ioctl(unix_fd[0], DKIOCGFORMAT, &formattedFlag);
	if(rtn) {
		xpr_ud("SCSIDisk get format: errno %d\n", errno, 2,3,4,5);
		return(IO_R_IO);
	}
	if(formattedFlag) {
		[self setFormattedInternal:1];
		rtn = ioctl(unix_fd[0], SDIOCGETCAP, &capacity);
		if(rtn) {
			xpr_ud("SCSIDisk get capacity: errno %d\n", errno, 
				2,3,4,5);
			return(IO_R_IO);
		}
		[self setBlockSize:capacity.cr_blklen];
		[self setDiskSize:capacity.cr_lastlba + 1];
	}
	else {
		[self setBlockSize:-1];
		[self setDiskSize:0];	
		[self setFormatted:0];
	}
	
	/*
	 * FIXME - get writeProtected via Inquiry command...
	 */
	return(IO_R_SUCCESS);
#endif
}

