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

// Copyright 1997 by Apple Computer, Inc., all rights reserved.
/*
 * Copyright (c) 1994-1997 NeXT Software, Inc.  All rights reserved. 
 *
 * AtapiCnt.h - Interface for ATAPI controller class. 
 *
 * HISTORY 
 * 31-Aug-1994 	Rakesh Dubey at NeXT 
 *	Created.
 */

#import <driverkit/scsiTypes.h>
#import <driverkit/IOSCSIController.h>
#import "AtapiCntCmds.h"

/*
 * We will preallocate command buffers for doing I/O. This avoids a deadlock
 * situation when we are running very low on memory. Comment out this #define
 * if this behavior is not desired.
 */
#define NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

/*
 * Commands to be executed by I/O thread.
 */
typedef enum {
    ATAPI_CNT_IOREQ,			/* process IdeIoReq_t */
    ATAPI_CNT_ABORT,			/* abort pending "needsDisk" I/Os */
    ATAPI_CNT_THREAD_ABORT,		/* thread abort */
} atapiCmd_t;

/*
 * Command buffer. These are enqueued on one of the I/O queues by exported
 * methods and dequeued and processed by the I/O thread.
 */
typedef  struct {
    atapiCmd_t	command;	/* IOREQ or abort */
    atapiIoReq_t *atapiIoReq;	/* command block passed to controller */
    void 	*buffer;
    vm_task_t	client;
    id		waitLock;	/* NXConditionLock  */
    sc_status_t	status;
    queue_chain_t link;
    queue_chain_t bufLink;
} atapiBuf_t;


@interface AtapiController:IOSCSIController
{
@private
    /*
     * The queues on which all I/Os are enqueued by exported methods. 
     */
    queue_head_t	_ioQueueNodisk;	/* for all I/O */

#ifdef NO_ATAPI_RUNTIME_MEMORY_ALLOCATION
    queue_head_t	_atapiBufQueue;
    id			_atapiBufLock;
#define MAX_NUM_ATAPIBUF		64
    atapiBuf_t		_atapiBufPool[MAX_NUM_ATAPIBUF];
#endif NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

    /*
     * NXConditionLock - protects the queues; I/O thread sleeps on this. 
     */
    id 			_ioQLock;
    
    id	_ataController;			/* ATA controller */
}

+ (BOOL)probe : deviceDescription;
+ (IODeviceStyle)deviceStyle;		/* override IOSCSIController	*/
+ (Protocol **)requiredProtocols;

- (sc_status_t) executeRequest 	: (IOSCSIRequest *)scsiReq 
			buffer : (void *)buffer 
			client : (vm_task_t)client;
			 
- (BOOL) maptoAtapiCmd:(atapiIoReq_t *)atapiIoReq	/* map SCSI -> ATAPI */
		 buffer:(void *)buffer;	

- (BOOL) emulateSCSICmd:(atapiIoReq_t *)atapiIoReq 
		 buffer:(void *)buffer;		/* fake these */

- (sc_status_t)resetSCSIBus;

- (void)getDMAAlignment:(IODMAAlignment *)alignment;

- (unsigned) maxTransfer;
@end

/*
 * So that we can use ATAPI naming convention and still use the ATA
 * registers.
 */
#define InterruptReason		sectCnt
#define byteCountLow		cylLow
#define byteCountHigh		cylHigh
#define driveSelect		drHead

#define ATAPI_NLUNS		1
