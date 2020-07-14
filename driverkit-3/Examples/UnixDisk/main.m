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
/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * main.m - top-level module for IOStub example, Distributed Objects
 *	    version.
 *
 * HISTORY
 * 09-Jun-92    Doug Mitchell at NeXT
 *      Created. 
 */

#import "FloppyDisk.h"
#import "SCSIDisk.h"
#import "unixDiskUxpr.h"
#import <driverkit/generalFuncs.h>
#import <objc/error.h>
#import <mach/cthreads.h>
#import <mach/mach.h>
#import <driverkit/IODiskPartition.h>

#define SCSI_UNIT_MIN		0
#define SCSI_UNIT_MAX		1
#define FLOPPY_UNIT_MIN		0
#define FLOPPY_UNIT_MAX		0

int main(int arcg, char **argv)
{
	id diskId = nil;
	int unit;
	
	IOInitDDM(200, "udXpr");
	IOSetDDMMask(XPR_IODEVICE_INDEX,
		XPR_UNIXDISK | XPR_UDTHREAD | XPR_VC);
	volCheckInit();
	IOInitGeneralFuncs();
	
	/*
	 * Make sure IODiskPartition is ready for indirect device
	 * attachment.
	 */
	[IODiskPartition name];
	
	for(unit=0; unit<=SCSI_UNIT_MIN; unit++) {
		if(diskId == nil) {
			diskId = [SCSIDisk alloc];
		}
		
		/*
		 * Avoid free/alloc if this init fails.
		 */
		if([diskId SCSIInit:unit] == nil) {
			continue;
		}
		else {
			/*
			 * Success. Have DiskObject superclass take care of 
			 * the rest.
			 */
			if([diskId registerDevice] == nil) {
				exit(1);
			}
			[diskId registerPartition:diskId];
			diskId = nil;
		}
		
	} 
	if(diskId) {
		[diskId free];
		diskId = nil;
	}
	
	for(unit=0; unit<=FLOPPY_UNIT_MIN; unit++) {
		if(diskId == nil) {
			diskId = [FloppyDisk alloc];
		}
		
		/*
		 * Avoid free/alloc if this init fails.
		 */
		if([diskId floppyInit:unit] == nil) {
			continue;
		}
		else {
			/*
			 * Success. Have DiskObject superclass take care of 
			 * the rest.
			 */
			if([diskId registerDevice] == nil) {
				exit(1);
			}
			[diskId registerPartition:diskId];
			diskId = nil;
		}
		
	} 
	if(diskId) {
		[diskId free];
	}
	
	/*
	 * Success. This thread is done; all subsequent work is done by 
	 * the IOStub objects' I/O threads (started in initStub:) and
	 * by the NXProxy threads.
	 */
	cthread_exit(0);
	return 0;	
}

