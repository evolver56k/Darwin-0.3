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
/*	unixThread.m		1.0	02/07/91	(c) 1991 NeXT   
 *
 * unixThread.m - unixDisk Device Thread support.
 *
 * HISTORY
 * 07-Feb-91    Doug Mitchell at NeXT
 *      Created.
 */

#import <bsd/sys/types.h>
#import <mach/cthreads.h>
#import <driverkit/IODevice.h>
#import "unixDisk.h"
#import "unixThread.h"
#import "unixDiskUxpr.h"
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>

static void unixThreadDequeue(IOQueue_t *ioQ, 
	BOOL needs_disk,
	int threadNum);

@implementation unixDisk(Thread)

/*
 * Enqueue an IOBuf on an IOQueue and wake up anyone (i.e., an I/O thread) 
 * who might be waiting for the IOBuf.
 */
- (void) enqueueIoBuf : (IOBuf_t *)buf
		         needs_disk : (BOOL)needs_disk
{
	queue_head_t *q;
	
	if(needs_disk)
		q = &IOQueue.q_disk;
	else
		q = &IOQueue.q_nodisk;
	[IOQueue.qlock lock];
	queue_enter(q, buf, IOBuf_t *, ioChain);
	[IOQueue.qlock unlockWith:WORK_AVAILABLE];
	xpr_ud("enqueueIoBuf: exiting. IOBuf 0x%x needs_disk %s\n", 
		buf, needs_disk ? (int)"TRUE" : (int)"FALSE", 3,4,5);
}

/*
 * Wakeup up I/O threads. Used for 'diskBecameReady' notification.
 */
- (void)ioThreadWakeup
{
	[IOQueue.qlock lock];
	[IOQueue.qlock unlockWith:WORK_AVAILABLE];
}

/*
 * Unlock IOQueue.qlock, updating condition variable as appropriate.
 */
- (void)unlockIOQueue
{
	int queue_state;
	IODiskReadyState lastReady = [self lastReadyState];
	
	/*
	 * There's still work to do when:
	 *    -- q_nodisk non-empty, or
	 *    -- q_disk non-empty and we "really" have a disk.
	 */

	if((!queue_empty(&IOQueue.q_nodisk)) ||
	   	((!queue_empty(&IOQueue.q_disk)) &&
	    		(lastReady != IO_NoDisk) &&
	    		(lastReady != IO_Ejecting) &&
      	    		(!IOQueue.ejectPending)
		)
	   ) {
		queue_state = WORK_AVAILABLE;
	}
	else	
		queue_state = NO_WORK_AVAILABLE;
	[IOQueue.qlock unlockWith:queue_state];
}

@end

/*
 * I/O thread. 'n' copies of this are IOForkThread()'d in the init:sender: 
 * method. This handles IOBufs which have been enqueued by exported methods 
 * (like read: and write:).
 *
 * This thread merely loops doing the  following:
 *    -- get an IOBuf off of the IOQueue.
 *    -- perform the task specified in IOBuf->command.
 *    -- if async request, ioComplete: the result, else 
 *	 wakeup the waiting thread.
 */
 
volatile void unix_thread(IOQueue_t *ioQ)
{
	int threadNum;
	IODiskReadyState lastReady;
	
	/*
	 * First assign ourself a thread number.
	 */
	[ioQ->qlock lock];
	threadNum = ioQ->numThreads++;
	[ioQ->qlock unlock];
	xpr_uth("unix_thread %d: starting\n", threadNum, 2,3,4,5);
	while(1) {
		
		/*
		 * Wait for something to do. Keep the lock until we
		 * dequeue something.
		 */
		[ioQ->qlock lockWhen:WORK_AVAILABLE];
		
		/*
		 * Service all requests which do not need a disk.
		 */
		xpr_uth("unix_thread: servicing q_nodisk\n", 1,2,3,4,5);
		while(!queue_empty(&ioQ->q_nodisk))
			unixThreadDequeue(ioQ, NO, threadNum);
		
		/*
		 * Now service all requests which need a disk, if our disk
		 * is present. 
		 */
		xpr_uth("unix_thread: servicing q_disk\n", 1,2,3,4,5);
		while((!queue_empty(&ioQ->q_disk)) &&
		      ([ioQ->device lastReadyState] != IO_NoDisk) &&
		      ([ioQ->device lastReadyState] != IO_Ejecting) &&
      		      (!ioQ->ejectPending)) { 
			unixThreadDequeue(ioQ, YES, threadNum);
		}
		
		/*
		 * If we have work to do in q_disk but we don't have a disk, 
		 * ask volCheck to put up a panel. In either case, when we 
		 * unlock ioQ.qlock for the last time, update its
		 * condition variable as appropriate so we and the other 
		 * I/O threads working on this IOQueue know whether or not
		 * to sleep.
		 */
		lastReady = [ioQ->device lastReadyState];
		if((!queue_empty(&ioQ->q_disk)) && 
		   ((lastReady == IO_NoDisk) || ioQ->ejectPending)) {
			[ioQ->device unlockIOQueue];
			xpr_uth("unix_thread: volCheckRequest()\n", 1,2,3,4,5);
			volCheckRequest(ioQ->device, 
				[ioQ->device diskType]);
		}
		else {
			[ioQ->device unlockIOQueue];
		}
	}
	/* NOT REACHED */
}

/*
 * Process a request at the head of one of the queues in *ioQ. 
 * ioQ->qlock must be held on entry; it will still be held on exit (though we
 * release it when doing a command dispatch).
 */
static void unixThreadDequeue(IOQueue_t *ioQ, 
	BOOL needs_disk,
	int threadNum)
{
	queue_head_t *q;
	IOBuf_t *IOBuf;
	
	if(needs_disk)
		q = &ioQ->q_disk;
	else
		q = &ioQ->q_nodisk;
	if(queue_empty(q)) {
		IOLog("unixThreadDequeue: Empty queue!\n");
		return;
	}
	IOBuf = (IOBuf_t *)queue_first(q);
	queue_remove(q, 
		IOBuf,
		IOBuf_t *,
		ioChain);
	
	/*
	 * For proper timing of the call to volCheckEjecting, we have to 
	 * determine right now - while the queue is locked - whether this
	 * is an eject command. If so, the call to volCheckEjecting() will
	 * prevent other I/O threads from attempting to perform I/Os from
	 * q_disk.
	 *
	 * The numDiskIos counter allows the thread doing an eject command
	 * to wait for all other pending Disk I/Os to complete before doing
	 * the eject.
	 *
	 * We also have to keep that cruft ejectPending flag around, since 
	 * the call to volCheckEjecting() doesn't result in an immediate 
	 * update of lastReadyState...
	 */
	if(needs_disk) {
		[ioQ->ejectLock lock];
		ioQ->numDiskIos++;
		if(IOBuf->command == @selector(deviceEject:threadNum:)) {
			ioQ->ejectPending = TRUE;
			volCheckEjecting(IOBuf->device, 
				[IOBuf->device diskType]);   
		}
		[ioQ->ejectLock unlockWith:ioQ->numDiskIos];
	}
	
	[ioQ->qlock unlock];
	xpr_uth("unix_thread %d: IOBuf 0x%x received\n", 
		threadNum, IOBuf, 3,4,5); 
	[IOBuf->device perform:IOBuf->command 
		with:(id)IOBuf
		with:(id)threadNum];
	[ioQ->qlock lock];
	if(needs_disk) {
		/*
		 * Enable possible waiting eject command.
		 */
		[ioQ->ejectLock lock];
		ioQ->numDiskIos--;
		[ioQ->ejectLock unlockWith:ioQ->numDiskIos];
	}
}	

/* end of IOThread.m */
