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
/*	FloppyDisk.m	1.0	01/02/91	(c) 1991 NeXT   
 *
 * FloppyDisk.m - Implementation for Floppy Disk class. 
 *
 * HISTORY
 * 01-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */
 
#import "FloppyDisk.h"
#import <bsd/sys/types.h>
#import <bsd/dev/fd_extern.h>
#import <bsd/libc.h>
#import <bsd/sys/file.h>
#import <bsd/dev/disk.h>
#import <errno.h>
#import "unixDiskUxpr.h"
#import <driverkit/volCheck.h>

#define NUM_FLOPPY_THREADS	2
#define FLOPPY_RAW_NAME		"fd"

@implementation FloppyDisk

/*
 * Probe hardware to determine how many instances to alloc and init. This 
 * is a hard coded kludge for now.
 */
#if	0
+ (void)IOProbe:serverId
{
	int 	floppy_unit;
	id	floppyId = nil;
	
	/*
	 * Attempt to instantiate and init each possible floppy disk.
	 */
	for(floppy_unit=FLOPPY_UNIT_MIN; 
	    floppy_unit<=FLOPPY_UNIT_MAX; 
	    floppy_unit++) {
		if(floppyId == nil)
			floppyId = [self alloc];
		
		/*
		 * Avoid free/alloc if this init fails.
		 */
		if([floppyId floppyInit:floppy_unit 
		    sender:serverId] == nil) 
			continue;
		else {
			/*
			 * Success. Have DiskObject superclass take care of 
			 * the rest.
			 */
			[floppyId registerDevice];
			[floppyId registerDisk];
			floppyId = nil;
		}
	}
	if(floppyId)
		[floppyId free];
}
#endif	0

/*
 * init one instance for specified disk.
 */
- floppyInit:(int)diskNum
{
	char *name;
	int thread_num;
	
	xpr_ud("FloppyDisk : init: unit %d\n", diskNum, 2,3,4,5);
	
	[self setUnit:diskNum];
	name = malloc(10);
	sprintf(name, "%s%d", FLOPPY_RAW_NAME, diskNum);
	[self setName:(const char *)name];
	[self setDeviceKind:"Unix Floppy Disk"];
	[self setLocation:NULL];
	[self setDriveName:"Sony Floppy"];
	[self setDiskType:PR_DRIVE_FLOPPY];
	[self setWriteProtected:0];
	
	/*
	 * Do a unix open  for each thread. All I/O here goes thru the live 
	 * partition.
	 */
	sprintf(unix_name, "/dev/rfd%db", diskNum);
	for(thread_num=0; thread_num<NUM_FLOPPY_THREADS; thread_num++) {
		unix_fd[thread_num] = open(unix_name, O_RDWR, 0);
		if(unix_fd[thread_num] < 0) {
		
			/*
			 * Maybe it's read-only.
			 */
			unix_fd[thread_num] = open(unix_name, O_RDONLY, 0);
			if(unix_fd[thread_num] < 0) {
				perror("floppyInit open()");
				return(nil);
			}
			[self setWriteProtected:1];
		}
	}
	
	/*
	 * Let superclass take care of initializing inherited instance
	 * variables.
	 */
	[self unixInit:NUM_FLOPPY_THREADS];
	if([self updatePhysicalParameters])
		return nil;
	return self;
}

/*
 * Get physical parameters (dev_size, block_size, etc.) from new disk.
 */
- (IOReturn)updatePhysicalParameters
{
	struct fd_format_info format_info;
	int rtn;
		
	/*
	 * try to get format info of the drive.
	 */
	[self setFormattedInternal:0];
	[self setRemovable:1];
	rtn = ioctl(unix_fd[0], FDIOCGFORM, &format_info);
	if(rtn) {
		xpr_ud("FloppyDisk FDIOCGFORM: errno %d\n", errno, 2,3,4,5);
		return(IO_R_IO);
	}
	if(!(format_info.flags & FFI_FORMATTED)) {
		xpr_ud("FloppyDisk Unformatted\n", errno, 2,3,4,5);
		[self setBlockSize:-1];
		[self setDiskSize:0];	
	}
	else {
		[self setFormattedInternal:1];
		[self setBlockSize:format_info.sectsize_info.sect_size];
		[self setDiskSize:format_info.total_sects];
	}
	if(format_info.flags & FFI_WRITEPROTECT) {
		[self setWriteProtected:1];
	}
	else {
		[self setWriteProtected:0];
	}
	return(IO_R_SUCCESS);
}

