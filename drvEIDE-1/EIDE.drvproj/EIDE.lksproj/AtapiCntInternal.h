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
 * AtapiCntInternal.h - Definition of ATAPI controller class.
 *
 *
 * HISTORY 
 * 21-Mar-1995 	Rakesh Dubey at NeXT 
 *	Created. 
 */
 
#import "AtapiCnt.h"

/*
 * Condition variable states for ioQueueLock.
 */
#define NO_WORK_AVAILABLE	0
#define WORK_AVAILABLE		1

@interface AtapiController(Internal)

/*
 * One-time only initialization.
 */
- initResources:controller;

/*
 * Free up local resources. 
 */
- free;


/*
 * Alloc/free command buffers.
 */
- (atapiBuf_t *)allocAtapiBuf;
- (void)freeAtapiBuf:(atapiBuf_t *)atapiBuf;


/*
 * -- Enqueue an atapiBuf_t on ioQueue<Disk,Nodisk>
 * -- wake up the I/O thread
 * -- wait for I/O complete (if atapiBuf->pending == NULL)
 */
- (IOReturn)enqueueAtapiBuf	: (atapiBuf_t *)atapiBuf;

/*
 * Either wake up the thread which is waiting on the atapiCmdBuf, or send an 
 * ioComplete back to client. atapiCmdBuf->status must be valid.
 */
- (void)atapiIoComplete 		: (atapiBuf_t *)atapiBuf;

/*
 * Main command dispatch method.
 */
- (void)atapiCmdDispatch 		: (atapiBuf_t *)atapiBuf;

/*
 * Unlock ioQLock, updating condition variable as appropriate.
 */
- (void)unlockIoQLock;

@end	/* AtapiController(Internal) */

