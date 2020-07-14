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
 * SCSIDiskPrivate.h - Private methods for SCSIDisk class.
 *
 * HISTORY
 * 11-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#ifndef	_BSD_DEV_M88K_SCSIDISKPRIVATE_H_
#define _BSD_DEV_M88K_SCSIDISKPRIVATE_H_

#import <driverkit/return.h>
#import <driverkit/IODisk.h>
#import <driverkit/SCSIDisk.h>
#import <driverkit/SCSIDiskTypes.h>
#import <driverkit/scsiTypes.h>
#import <bsd/dev/scsireg.h>
#import <kern/queue.h>

/* Radar 2005639: this should be in a common header */
#define APPLE_MAX_THREADS		"ClientThreads"

@interface SCSIDisk(Private)

- (sdInitReturn_t)SCSIDiskInit	:(int)iunit 
				  targetId : (u_char)Target
				  lun : (u_char)Lun
				  controller : controllerId;

/*
 * Radar 2005639: if the controller implements the APPLE_MAX_THREADS
 * getIntValues selector, use that value. Otherwise, call the default
 * initializer.
 */
- initResourcesWithThreadCount	: (int) threadCount;		  
- initResources;

/*
 * Free up local resources.
 */
- free;

/*
 * Internal I/O routines.
 */
- (sc_status_t)sdInquiry 	: (inquiry_reply_t *)inquiryReply;
- (sc_status_t)sdReadCapacity 	: (capacity_reply_t *)capacityReply;
- (sc_status_t)sdModeSense	: (mode_sel_data_t *)modeSenseReply;
- (IOReturn)sdRawRead		: (int)block
				  blockCnt:(int)blockCnt
				  buffer:(void *)buffer;
		
/*
 * Options for scsiStartStop:		  
 */
typedef enum {
	SS_START,
	SS_STOP,
	SS_EJECT,
} startStop_t;

- (IOReturn)scsiStartStop 	: (startStop_t)cmd
				  inhibitRetry : (BOOL)inhibitRetry;
			  
/*
 * Common deviceRead and deviceWrite routine.
 */
- (IOReturn) deviceRwCommon 	: (sdOp_t)command
		  		  block: (u_int) deviceBlock
		 		  length : (u_int)length
		  		  buffer : (void *)buffer
		  		  client : (vm_task_t)client
		 		  pending : (void *)pending
		  		  actualLength : (u_int *)actualLength;

/*
 * -- Enqueue an sdBuf on ioQueue 
 * -- wake up one of the I/O threads
 * -- wait for I/O complete (if sdBuf->pending != NULL)
 */
- (IOReturn)enqueueSdBuf:(sdBuf_t *)sdBuf;

/*
 * Allocate and free sdBuf_t's.
 */
- (sdBuf_t *)allocSdBuf 	: (void *)pending;
- (void)freeSdBuf 		: (sdBuf_t *)sdBuf;

#if 1  // Radar Fix #2260508
- (void) releaseNonZeroLunsOnTarget : (int) target;
- (void) reserveNonZeroLunsOnTarget : (int) target;
#endif // Radar Fix #2260508

@end

#endif	_BSD_DEV_M88K_SCSIDISKPRIVATE_H_
