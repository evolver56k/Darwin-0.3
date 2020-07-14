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
 * SCSIDisk.h - Exported Interface for SCSI Disk device class. 
 *
 * HISTORY
 * 11-Feb-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import <driverkit/scsiTypes.h>
#import <driverkit/generalFuncs.h>

/*
 * FIXME - there should only be one thread if target does not 
 * implement command queueing, and more (at least two, max TBD)
 * if target does implement command queueing.
 */
#define NUM_SD_THREADS_MIN	1
#define NUM_SD_THREADS_MAX	6

@interface SCSIDisk:IODisk<IODiskReadingAndWriting,IOPhysicalDiskMethods>
{
@private
	id 		_controller;	// the SCSIController object which does
					// our SCSI transactions
					
	/*
	 * FIXME - does SCSIDisk need SCSI3-style target and LUN?
	 */
	u_char		_target;	// target/lun of this device
	u_char		_lun;
	unsigned	_isReserved:1,	 // we hold a target/lun reservation
			_isRegistered:1, // registered w/IODisk
			_allowLoans:1;   // allow a free disk to be reserved
			
	/*
	 * I/O thread info.
	 */
	int		_numThreads;
	IOThread	_thread[NUM_SD_THREADS_MAX];
	
	/*
	 * The queues on which all I/Os are enqueued by exported methods. 
	 */
	queue_head_t	_ioQueueDisk;	// for I/Os requiring disk 
	queue_head_t	_ioQueueNodisk;	// for other I/O
	id 		_ioQLock;	// NXConditionLock - protects the 
					//   ioQueues; I/O thread sleep
					//   on this.
	unsigned char	_inquiryDeviceType;
	
	/*
	 * Eject command locking stuff. A thread about to execute an eject 
	 * command must wait until it is the only thread executing a "disk
	 * needed" type of I/O before preceeding; other such I/Os will be
	 * inhibited by either the ejectPending flag (set by us) or by the
	 * lastReadyState instance (maintained by volCheck and set 
	 * asynchronously to RS_EJECTING after an I/O thread gets an eject
	 * command and calls volCheckEjecting()).
	 *
	 * Implementation: ejectLock is an NXConditionLock;
	 * its condition variable is the number of threads currently 
	 * executing commands from IOqueue.q_disk. An eject can only occur
	 * when this is 1.
	 */
	id		_ejectLock;		
	int		_numDiskIos;	// #of threads executing 	
					// commands from IOqueue.q_disk
	BOOL		_ejectPending;	
	
}

+ (BOOL)probe : deviceDescription;
+ (IODeviceStyle)deviceStyle;
+ (Protocol **)requiredProtocols;

- (IOReturn) sdCdbRead 		: (IOSCSIRequest *)scsiReq
			  	   buffer:(void *)buffer /* data destination */
				   client:(vm_task_t)client;
			  
- (IOReturn) sdCdbWrite		: (IOSCSIRequest *)scsiReq
			  	   buffer:(void *)buffer /* data destination */
				   client:(vm_task_t)client;
				   
- (int)target;
- (int)lun;
- (unsigned char)inquiryDeviceType;
- controller;
- (void) synchronizeCache;

@end
