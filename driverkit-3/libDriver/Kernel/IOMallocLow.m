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
 * IOMallocLow.m - IOMallocLow(), IOFreeLow() for i386. 
 *
 * HISTORY
 * 01-Apr-93    Doug Mitchell at NeXT
 *      Created.
 */

#import <driverkit/i386/kernelDriver.h> 
#import <mach/vm_param.h>
#import <kernserv/queue.h>
#import <machdep/i386/dma_exported.h>
#import <driverkit/generalFuncs.h>

queue_head_t dmaBufQueue;

/*
 * Need to keep dma_buf_t's around for IOFreeLow().
 */
typedef struct {
	dma_buf_t 	*buf;
	queue_chain_t	link;
} low16Buf;


void *IOMallocLow(int size)
{
	boolean_t brtn;
	dma_buf_t *buf;
	low16Buf *lowBuf;
	
	buf = IOMalloc(sizeof(*buf));
	brtn = dma_buf_alloc(buf, size);
	if(brtn == FALSE) {
		IOFree(buf, sizeof(*buf));
		return 0;
	}
	
	/*
	 * Enqueue this on dmaBufQueue.
	 */
	lowBuf = IOMalloc(sizeof(*lowBuf));
	lowBuf->buf = buf;
	queue_enter(&dmaBufQueue, lowBuf, low16Buf *, link);
	return buf->_ptr;
}

void IOFreeLow(void *p, int size)
{
	low16Buf *lowBuf;

	/*
	 * Find the associated low16Buf.
	 */
	lowBuf = (low16Buf *)queue_first(&dmaBufQueue);
	while(!queue_end(&dmaBufQueue, (queue_t)lowBuf)) {
		if(lowBuf->buf->_ptr == p) {
			queue_remove(&dmaBufQueue, lowBuf, low16Buf *, link);
			dma_buf_free(lowBuf->buf);
			IOFree(lowBuf->buf, sizeof(*lowBuf->buf));
			IOFree(lowBuf, sizeof(*lowBuf));
			return;
		}
		lowBuf = (low16Buf *)lowBuf->link.next;
	}
	IOLog("IOFreeLow: buf 0x%x not found\n", (unsigned)p);
}

