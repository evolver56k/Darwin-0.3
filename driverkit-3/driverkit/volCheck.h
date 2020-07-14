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
 * volCheck.h - interface to common volume check logic.
 *
 * HISTORY
 * 07-May-91    Doug Mitchell at NeXT
 *      Created.
 *
 * This module performs the following functions for the IODisk class:
 *
 * -- it detects disk insertion events for removable media drives.
 *
 * -- it makes requests of the WS to put up various panels like "Please insert
 *    disk so-and-so". In the kernel this communication with the WS is handled
 *    via the vol driver; IODisk subclasses need not concern themselves
 *    with this mechanism.
 *
 * -- it detects "disk not ejected" condition for drives which do not 
 *    support autoeject. This condition will cause WS to put up a 
 *    "please eject disk so-and-so". 
 *
 * There is one volCheck thread in the kernel for all IODisk-based
 * drivers. There will also be one in user space when we get around to 
 * moving disk drivers out to user space.
 *
 * To use the volCheck module, the following must be performed:
 *
 * -- volCheckInit() must be called once at system initialization. 
 * 
 * -- the IODisk corresponding to the physDevice for each removable
 *    media drive must be registered with volCheck via volCheckRegister().
 *    The IODisk's idMapArray must have already been initialized with
 *    rawDev and blockDev.
 * 
 * -- IODisk subclasses using volCheck should implement I/O request
 *    queueing in the following manner:
 *
 *    generic exported method (readAsync:, etc.) 
 *    {
 *        if this I/O requires no disk
 *	      enqueue on queue_(always);
 *	  else
 *	      enqueue on queue_(needs_disk);
 *	  wakeup I/O thread via [some_queue_lock unlockWith:WORK_TO_DO];
 *    }
 *     
 *    generic I/O thread (floppy I/O thread or SCSI disk thread...) 
 *    {
 *        while(1) {
 *	      sleep via [some_queue_lock lockUntil:WORK_TO_DO];
 *	      process everything on queue_(always);
 *	      if lastReadyState != IO_RS_NODISK or IO_RS_EJECTING
 *	    	  process everything in queue_(needs_disk);
 *	      else {
 *	          if queue_(needs_disk) non-empty
 *		      volCheckRequest();
 *	      }
 *	      [some_queue_lock unlockWith:<current "work to do" state>];
 *        }
 *    }
 *
 *    Notes: 
 *        IODisk contains an instance variable called _lastReadyState.
 *        This is init'd by IODisk to IO_RS_NOTREADY or IO_RS_NODISK by 
 *        the IODisk subclass; it is subsequently only changed by 
 *        volCheck. The state transitions are as follows:
 *
 *      current   	change
 *       state      	  to    	when
 *	--------------	-----------	-----------------------------------
 *	IO_RS_NOTREADY or	
 *	IO_RS_NODISK	IO_Ready	volCheck determines via 
 *					  -updateReadyState that hardware is
 *					   ready
 *	IO_Ready	IO_RS_NOTREADY	subclass calls volCheckNotReady()
 *					  on error detection
 *	IO_Ready	IO_RS_EJECTING	subclass calls volCheckEjecting()
 *					  when executing eject command
 *   	IO_RS_EJECTING	IO_RS_NODISK	volCheck gets IO_RS_NODISK or
 *					  IO_RS_NOTREADY from updateReadyState.
 *
 *        When updateReadyState: is called and the current state is ready, 
 *	  no I/O should be performed - the driver should just return IO_Ready.
 *	  This avoids unnecessary I/O on drives which are known to have a disk
 *        present. 
 *
 *	  volCheckRequest() causes volCheck to make a request of the WS to 
 *	  put up a "Please insert disk" panel.
 *
 *        The key to the above psuedocode for I/O threads is that the thread
 *        will refrain from attempting any I/O requiring a disk when it is
 *        known that the drive has no disk, or when an eject is in progress.
 *
 *        The timing of the call to volCheckEjecting() is critical. The 
 *        lastReadyState instance variable will be set to IO_RS_EJECTING during
 *	  this call, disabling further I/Os thru the "queue_(needs_disk)" 
 *	  I/O queue. Thus volCheckEjecting must be called by the I/O thread. 
 *        If there are multiple I/O threads (as there are in the SCSI Disk 
 *	  driver), the thread which fetches the command buffer containing the
 *        eject command must call volCheckEjecting() while queue_(needs_disk)
 *        is still locked, so that no other I/O threads will attempt to 
 *	  perform an I/O which requires a disk.
 *
 * 	  NOTE:  in addition to checking "disk ejecting" via
 *		 [self getLastReadyState], a driver must also keep a local
 *		 instance variable like "ejectPending". It sets this at 
 *		 same time it calls volCheckEjecting() and it checks this
 *		 at the same time it checks [self getLastReadyState]. The 
 *		 reason for this is that volCheckEjecting() does not 
 * 		 update lastReadyState synchronously - it enqueues a request
 * 		 for this operation for the volCheck thread to handle. This
 *		 keeps the volCheck thread's operations in sync, but it
 *		 forces the driver to maintain this extra state.
 *
 * -- the driver must implement the following methods:
 *
 *    (readyState_t)updateReadyState	
 *	   This causes the driver to determine current state of drive
 *	   (ready, not ready, no disk, etc.). volCheck will not call this 
 *	   method when the disk's lastReadyState is IO_Ready.
 *
 *    (void)abortRequest
 *         Called when user hits "cancel" on a "Please Insert Disk" panel.
 *         This should be passed down to the I/O thread (on the
 *         queue_(always) I/O queue); the I/O thread should ioComplete 
 *         all of the I/O requests in the queue_{needs_disk} I/O queue
 *	   with the status IO_R_NO_DISK (maps to ENXIO).
 *
 *    (ioReturn_t)updatePhysicalParameters
 *	   This is called on detection of disk insertion. The driver 
 * 	   should update its physical disk parameters (in particular, 
 * 	   devSize, blockSize, and formatted). This should be a "no disk
 * 	   required" operation, since the volCheck thread might call this 
 * 	   when the driver's I/O thread is waiting for disk insertion.
 *
 *    (void)diskBecameReady
 *	   This is also called on detection of disk insertion. This should 
 *         cause the I/O thread to be woken up. The device's
 *         readyState will be IO_Ready; I/O requests in the 
 *	   queue_(needs_disk) I/O queue can proceed. 
 */
 
#import <driverkit/IODisk.h>
#import <kernserv/insertmsg.h>

/*
 * One-time only initialization.
 */
void volCheckInit();

/*
 * Register a removable media drive.
 */
void volCheckRegister(id diskObj,
	dev_t blockDev,
	dev_t rawDev);
	
/*
 * Unregister a removable media drive. This probably should never happen,
 * at least not in the kernel...
 */
void volCheckUnregister(id diskObj);

/*
 * Request a "please insert disk" panel.
 */
void volCheckRequest(id diskObj,		// requestor
	int diskType);				// PR_DRIVE_FLOPPY, etc.
	

/*
 * Notify volCheck thread that specified device is in a "disk ejecting" state.
 */
void volCheckEjecting(id diskObj,
	int diskType);				// PR_DRIVE_FLOPPY, etc.

/*
 * Notify volCheck that disk has gone not ready. Typically called on
 * gross error detection.
 */
void volCheckNotReady(id diskObj);

/*
 * Some #defines from voldev.h and insertmsg.h, files which we can't import
 * in user mode...
 */

/*
 * p2 values for PR_PT_DISK_NUM / PR_PT_DISK_LABEL
 */
#define PR_DRIVE_FLOPPY         0       	/* floppy disk */
#define PR_DRIVE_OPTICAL        1       	/* OMD-1 (5.25") optical */
#define PR_DRIVE_SCSI           2       	/* removable SCSI disk */

