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
 * AtapiCntInternal.m - Implementation of ATAPI controller class.
 *
 *
 * HISTORY
 *
 * 05-Mar-1996	 Rakesh Dubey at NeXT
 *	Modified so that no memory is allocated at run-time.
 *
 * 21-Mar-1995 	Rakesh Dubey at NeXT 
 *	Created. 
 */

#import "IdeCnt.h"
#import "AtapiCntInternal.h"
#import <kern/assert.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <driverkit/IODevice.h>
#import <machkit/NXLock.h>
#import <kernserv/prototypes.h>

/*
 * Top-level I/O thread.
 */
static volatile void atapiThread(AtapiController *atapiCnt);

@implementation AtapiController(Internal)

- initResources:direct
{
#ifdef NO_ATAPI_RUNTIME_MEMORY_ALLOCATION
    int i;
    atapiBuf_t *atapiBuf;
#endif NO_ATAPI_RUNTIME_MEMORY_ALLOCATION
	
    _ataController = direct;
    
    queue_init(&_ioQueueNodisk);
    
#ifdef NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

    /* Set up a queue of ideBufs */
    queue_init(&_atapiBufQueue);
    _atapiBufLock = [NXLock new];
    [_atapiBufLock lock];
    
    for (i = 0; i < MAX_NUM_ATAPIBUF; i++)	{
	atapiBuf = &_atapiBufPool[i];
	atapiBuf->waitLock = [NXConditionLock alloc];
	[atapiBuf->waitLock initWith:NO];
	queue_enter(&_atapiBufQueue, atapiBuf, atapiBuf_t *, bufLink);
    }
    [_atapiBufLock unlock];
    
    if (_ide_debug)	{
	IOLog("NO_ATAPI_RUNTIME_MEMORY_ALLOCATION\n");
    }

#endif NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

    _ioQLock = [NXConditionLock alloc];
    [_ioQLock initWith:NO_WORK_AVAILABLE];
    
    IOForkThread((IOThreadFunc)atapiThread, self);
    
    return self;
}

/*
 * Free up local resources. 
 */
- free
{
    /*
     * First kill the I/O thread, then free alloc'd instance variables. 
     */
    atapiBuf_t *atapiBuf;
    int i;

    atapiBuf = [self allocAtapiBuf];
    [self enqueueAtapiBuf:atapiBuf];
    
    [self freeAtapiBuf:atapiBuf];
    [_ioQLock free];

#ifdef NO_ATAPI_RUNTIME_MEMORY_ALLOCATION
    if (_atapiBufLock)
	[_atapiBufLock free];
    
    for (i = 0; i < MAX_NUM_ATAPIBUF; i++) {
	if (_atapiBufPool[i].waitLock)
	    [_atapiBufPool[i].waitLock free];
    }
#endif NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

    return ([super free]);
}

/*
 * Allocate and free AtapiBuf_t's.
 */

#ifdef NO_ATAPI_RUNTIME_MEMORY_ALLOCATION
- (atapiBuf_t *) allocAtapiBuf
{
    atapiBuf_t *atapiBuf;
    id waitLock;

    while (1)	{
    	[_atapiBufLock lock];
	if (!queue_empty(&_atapiBufQueue))
	    break;
	[_atapiBufLock unlock];
	IOSleep(20);		// the system is overloaded
    }
    
    ASSERT(queue_empty(&_atapiBufQueue) != 0);
    atapiBuf = (atapiBuf_t *) queue_first(&_atapiBufQueue);
    ASSERT(atapiBuf != 0);
    queue_remove(&_atapiBufQueue, atapiBuf, atapiBuf_t *, bufLink);

    waitLock = atapiBuf->waitLock;
    bzero(atapiBuf, sizeof(atapiBuf_t));
    atapiBuf->waitLock = waitLock;
    [atapiBuf->waitLock initWith:NO];
    
    [_atapiBufLock unlock];
    return (atapiBuf);
}

- (void)freeAtapiBuf:(atapiBuf_t *) atapiBuf
{
    [_atapiBufLock lock];
    queue_enter(&_atapiBufQueue, atapiBuf, atapiBuf_t *, bufLink);
    ASSERT(queue_empty(&_atapiBufQueue) != 0);
    [_atapiBufLock unlock];
}

#else NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

- (atapiBuf_t *) allocAtapiBuf
{
    atapiBuf_t *atapiBuf = IOMalloc(sizeof(atapiBuf_t));

    bzero(atapiBuf, sizeof(atapiBuf_t));
    atapiBuf->waitLock = [NXConditionLock alloc];
    [atapiBuf->waitLock initWith:NO];
    
    return (atapiBuf);
}

- (void)freeAtapiBuf:(atapiBuf_t *) atapiBuf
{
    if (atapiBuf->waitLock) {
	[atapiBuf->waitLock free];
    }
    IOFree(atapiBuf, sizeof(atapiBuf_t));
}
#endif NO_ATAPI_RUNTIME_MEMORY_ALLOCATION

/*
 * -- Enqueue an AtapiBuf_t on ioQueue<Disk,Nodisk>
 * -- wake up the I/O thread
 * -- wait for I/O complete (if atapiBuf->pending == NULL)
 *
 * All I/O goes thru here; this is the last method called by exported methods
 * before the I/O thread takes over. 
 */
- (IOReturn) enqueueAtapiBuf:(atapiBuf_t *) atapiBuf
{
    queue_head_t *q;

    [_ioQLock lock];
    q = &_ioQueueNodisk;
    queue_enter(q, atapiBuf, atapiBuf_t *, link);
    [_ioQLock unlockWith:WORK_AVAILABLE];

    /*
     * Wait for I/O complete. 
     */
    [atapiBuf->waitLock lockWhen:YES];
    [atapiBuf->waitLock unlock];

    /* FIXME: What should this value be?? */
    return (atapiBuf->status);
}


/*
 * Either wake up the thread which is waiting on the ideCmdBuf, or send an 
 * ioComplete back to client. ideCmdBuf->status must be valid.
 */
- (void)atapiIoComplete:(atapiBuf_t *) atapiBuf
{
    /*
     * Sync I/O. Just wake up the waiting thread. 
     */
    [atapiBuf->waitLock lock];
    [atapiBuf->waitLock unlockWith:YES];
}

/*
 * Main command dispatch method. 
 */
- (void)atapiCmdDispatch:(atapiBuf_t *)atapiBuf
{
    switch (atapiBuf->command) {
    
		case ATAPI_CNT_IOREQ:
			atapiBuf->status = [_ataController
				atapiExecuteCmd:atapiBuf->atapiIoReq 
				buffer:atapiBuf->buffer client:atapiBuf->client];
			break;

		case ATAPI_CNT_THREAD_ABORT:

			/*
			 * First give I/O complete before we die. 
			 */
			atapiBuf->status = STAT_GOOD;
			[self atapiIoComplete:atapiBuf];
			IOExitThread();

		default:
			IOLog("%s: Bogus atapiBuf->command 0x%0x in atapiCmdDispatch\n", 
				[self name], atapiBuf->command);
			IOPanic("atapiThread");
	}

    [self atapiIoComplete:atapiBuf];
    return;
}

/*
 * Unlock ioQLock, updating condition variable as appropriate.
 */
- (void)unlockIoQLock
{
    int     queue_state;

    if (!queue_empty(&_ioQueueNodisk))
	queue_state = WORK_AVAILABLE;
    else
	queue_state = NO_WORK_AVAILABLE;
    [_ioQLock unlockWith:queue_state];
}


@end

/*
 * I/O thread. Each one of these sits around waiting for work to do on 
 * ioQueue; when something appears, the thread grabs it and disptahes it.
 */

static volatile void
atapiThread(AtapiController *atapiCnt)
{
    atapiBuf_t *atapiBuf;
    queue_head_t *q;

    while (1) {

	/*
	 * Wait for some work to do. 
	 */
	[atapiCnt->_ioQLock lockWhen:WORK_AVAILABLE];

	/*
	 * Service all requests which do not require a disk. 
	 */
	q = &atapiCnt->_ioQueueNodisk;
	while (!queue_empty(q)) {
	    atapiBuf = (atapiBuf_t *) queue_first(q);
	    queue_remove(q, atapiBuf, atapiBuf_t *, link);
	    [atapiCnt->_ioQLock unlock];
	    [atapiCnt atapiCmdDispatch:atapiBuf];
	    [atapiCnt->_ioQLock lock];
	}

	[atapiCnt unlockIoQLock];
    }

    /* NOT REACHED */
}
