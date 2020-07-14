/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1997-1998 by Apple Computer, Inc., All rights reserved.
 * Copyright 1994-1997 NeXT Software, Inc., All rights reserved.
 *
 * IdeDisk.h - Exported Interface for IDE Disk device class. 
 *
 * HISTORY 
 *
 * 04-Mar-1996	 Rakesh Dubey at NeXT
 *	Modified so that no memory is allocated at run-time.
 *
 * 07-Jul-1994	 Rakesh Dubey at NeXT
 *	Created from original driver written by David Somayajulu.
 */

#ifdef	DRIVER_PRIVATE

#ifndef	_BSD_DEV_IDEDISK_H
#define _BSD_DEV_IDEDISK_H 1

#import <driverkit/return.h>
#import <driverkit/driverTypes.h>
#import <driverkit/IODisk.h>
#import <driverkit/kernelDiskMethods.h>

#if (IO_DRIVERKIT_VERSION == 330)
#import <mach/time_stamp.h>
#else
#import <kern/queue.h>
#import <kern/time_stamp.h>
#endif

#import <kernserv/clock_timer.h>
#import "IdeCntPublic.h"

/*
 * We will preallocate command buffers for doing I/O. This avoids a deadlock
 * situation when we are running very low on memory. Comment out this #define
 * if this behavior is not desired.
 */
#define NO_ATA_RUNTIME_MEMORY_ALLOCATION

/*
 * Commands to be executed by I/O thread.
 */
typedef enum {
    IDEC_IOREQ,			/* process IdeIoReq_t */
    IDEC_READ,			/* normal read */
    IDEC_WRITE,			/* normal write */
    IDEC_ABORT,			/* abort pending "needsDisk" I/Os */
    IDEC_THREAD_ABORT,		/* thread abort */
    IDEC_INIT,			/* initialize the ide drive */
} IdeCmd_t;


/*
 * Command buffer. These are enqueued on one of the I/O queues by exported
 * methods and dequeued and processed by the I/O thread.
 */
typedef  struct {
    IdeCmd_t	command;	/* IDEC_IOREQ, etc. */
    ideIoReq_t	*ideIoReq;	/* command block passed to controller,
				   (IDEC_IOREQ only) */
    u_int 	block;		/* IDE_READ and IDE_WRITE only */
    u_int	blockCnt;	/* ditto */
    void	*buf;		/* where to r/w, for all except IDEC_IOREQ */
    vm_task_t	client;		/* address space containing *buf */
    id		waitLock;	/* NXConditionLock  */
    void	*pending;	/* if non-NULL, async request; this is */
				/* used to ioComplete: the operation. */

    unsigned 	needsDisk:1,
		oneWay:1;	/* 1 ==> no I/O complete */
    
    /*
     * Fields valid at I/O complete time.
     */
    u_int	bytesXfr;	/* bytes actually moved 
    				  (IDEC_{READ,WRITE} only) */
    IOReturn	status;		/* result */
    
    queue_chain_t link;
    queue_chain_t bufLink;
    
} ideBuf_t;



@interface IdeDisk:IODisk<IODiskReadingAndWriting, IOPhysicalDiskMethods>
{
@private
    /*
     * The queues on which all I/Os are enqueued by exported methods. 
     */
    queue_head_t	_ioQueueDisk;	/* for I/Os requiring disk */
    queue_head_t	_ioQueueNodisk;	/* for other I/O */

#ifdef NO_ATA_RUNTIME_MEMORY_ALLOCATION
    queue_head_t	_ideBufQueue;
    id			_ideBufLock;
#define MAX_NUM_ATABUF		128
    ideBuf_t		_ideBufPool[MAX_NUM_ATABUF];
#endif NO_ATA_RUNTIME_MEMORY_ALLOCATION
    
    /*
     * NXConditionLock - protects the queues; I/O thread sleeps on this. 
     */
    id 			_ioQLock;
    
    /*
     * The pseudo controller to streamline interrupts, resets and diagnostics
     * commands. 
     */
    id			_cntrlr;
    
    /*
     * Commands to read and write from the disk are cached here. 
     */
    unsigned 		_ideReadCommand;
    unsigned 		_ideWriteCommand;

    ideDriveInfo_t	_ideInfo;
    unsigned		_driveNum;
    unsigned char   	_ideDriveName[32];
}

/*
 * Probe is invoked at load time. It determines what devices are on the
 * bus and alloc's and init:'s an instance of this class for each one.
 */
+ (BOOL)probe:deviceDescription;
+ (IODeviceStyle) deviceStyle;
+ (Protocol **) requiredProtocols;
+ (BOOL)hd_devsw_init:deviceDescription;

- (ideDriveInfo_t)ideGetDriveInfo;
- (id)cntrlr;
- (unsigned)driveNum;

@end

#endif /* _BSD_DEV_IDEDISK_H */

#endif	/* DRIVER_PRIVATE */
