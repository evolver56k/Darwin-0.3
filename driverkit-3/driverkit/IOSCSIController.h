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
/*
 * Copyright 1997, 1998 Apple Computer Inc. All rights reserved.
 * Copyright (c) 1991, 1992, 1993 NeXT Computer, Inc.
 *
 * Abstract superclass for SCSI controllers.
 *
 * HISTORY
 *
 * 1998-3-11	gvdl@apple.com Removed IOSCSIUser stuff
 *
 * 12 December 1997 Martin Minow at Apple
 * 	Added IOSCISUser kernel support
 * 14 June 1995	Doug Mitchell at NeXT
 *	Added SCSI-3 support.
 * 25 June 1993 David E. Bohman at NeXT
 *	Cleaned up some (made machine independent).
 * 11/9/92	Brian Pinkerton at NeXT
 *      Created.
 */

#import <driverkit/IODirectDevice.h>
#import <driverkit/return.h>
#import <driverkit/scsiTypes.h>
#import <kernserv/queue.h>

#define IOSCSI_RSVD0_SIZE	(SCSI_NTARGETS * SCSI_NLUNS - 2)

@interface IOSCSIController : IODirectDevice<IOSCSIControllerExported>
{
@private
	/*
	 * Target/lun reservation list. One target is reserved by the 
	 * subclass for Host ID. We keep track of how many active 
	 * devices are on the bus in _reserveCount.
	 */
	queue_head_t	_reserveQ;
	
	/*
	 * Space used in 3.3 and previous versions, for backward compatibility.
	 */
	unsigned	_IOSCSIController_reserved0[IOSCSI_RSVD0_SIZE];
	
	unsigned int	_reserveCount;
	id		_reserveLock;	 // NXLock. Protects _reserveArray.
	unsigned 	_worstCaseAlign; // worst case DMA alignment.

	// Remove Martin's user stuff without breaking the fragile class
	unsigned int	_IOSCSIController_reserved[4];
}

- initFromDeviceDescription	: deviceDescription;

- (unsigned int)numReserved;

- (IOReturn)getIntValues	: (unsigned *)parameterArray
		   forParameter : (IOParameterName)parameterName
			  count : (unsigned int *)count;
	
- (IOReturn)setIntValues	: (unsigned *)parameterArray
		   forParameter : (IOParameterName)parameterName
			  count : (unsigned int)count;
- (unsigned int) numQueueSamples;		/* simple statistics */
- (unsigned int) sumQueueLengths;
- (unsigned int) maxQueueLength;

- (void)resetStats;				/* zero the above counters */

/*
 * Subclasses of SCSIController must implement the following two 
 * methods of the SCSIControllerExported protocol. 
 * SCSIController's versions of these return SR_IOST_INVALID.
 * Implementation of executeSCSI3Request:buffer:client is optional.
 *
 * - (sc_status_t) executeRequest : (IOSCSIRequest *)scsiReq
 *			   buffer : (void *)buffer 
 *			   client : (vm_task_t)client;
 *				   
 * - (sc_status_t) resetSCSIBus;			
 *
 *
 * May optionally be overridden by subclass.
 *
 * - (unsigned) maxTransfer;
 * - (void)getDMAAlignment		: (IODMAAlignment *)alignment;
 * - (int)numberOfTargets;		// IOSCSIController returns 8
 */

@end

#define IO_IS_A_SCSI_CONTROLLER		"IOIsASCSIController"
#define IO_SCSI_CONTROLLER_STATS	"IOSCSIControllerStatistics"

#define IO_SCSI_CONTROLLER_STAT_ARRAY_SIZE	3
#define IO_SCSI_CONTROLLER_MAX_QUEUE_LENGTH	0
#define IO_SCSI_CONTROLLER_QUEUE_SAMPLES	1
#define IO_SCSI_CONTROLLER_QUEUE_TOTAL		2
