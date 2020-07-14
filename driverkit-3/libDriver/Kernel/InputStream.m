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
 * InputStream.m
 *
 * Copyright (c) 1991, NeXT Computer, Inc.  All rights reserved.
 *
 *      Audio driver input stream object.
 *
 * HISTORY
 *      07/14/92/mtm    Original coding.
 */

#import <mach/mach_types.h>
#import <kern/lock.h>
#import <mach/mach_user_internal.h>
#import <mach/mach_interface.h>
#import <kernserv/kern_server_types.h>
#import <kernserv/prototypes.h>
#import "InputStream.h"
#import <driverkit/IOAudioPrivate.h>
#import "snd_reply.h"
#import "audioLog.h"
#import "audioReply.h"
#import <mach/vm_param.h>		// PAGE_SIZE
#import <architecture/byte_order.h>
#import <driverkit/kernelDriver.h>

@implementation InputStream

/*
 * Enqueue buffer to region queue.
 */
- (BOOL)recordSize:(u_int)byteCount tag:(int)aTag
                          replyTo:(port_t)replyPort
                        replyMsgs:(ASMsgRequest)messages
{
    region_t *region;
    kern_return_t krtn;
    port_t kernReplyPort;
    vm_address_t kmem;
    vm_task_t kernelTask = (vm_task_t)kern_serv_kernel_task_port();

    kernReplyPort = IOConvertPort(replyPort, IO_CurrentTask, IO_Kernel);

    /*
     * Truncate count if necessary.
     */
    if (dataFormat == IOAudioDataFormatLinear16)
	byteCount &= (channelCount == 1 ? ~(2-1) : ~(4-1));

    /*
     * Allocate memory in the kernel map.
     */
    krtn = vm_allocate(kernelTask, &kmem, byteCount, TRUE);
    if (krtn != KERN_SUCCESS) {
	IOLog("Audio: record request (%d bytes) too large\n", byteCount);
	return NO;
    }

    region = [self newRegion];
    region->enq_ptr = region->record_ptr = region->data = kmem;
    region->end = region->data + byteCount;
    region->count = byteCount;
    region->tag = aTag;
    region->reply_port = kernReplyPort;
    region->messages = messages;

    /*
     * Some reply messages are per stream in NXSound interface.
     * FIXME: these only get set if data sent to stream.
     */
    userReplyMessages = messages;
    userReplyPort = kernReplyPort;

    xpr_audio_stream("IS: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    queue_enter(&regionQueue, region, region_t *, link);
    [regionQueueLock unlock];
    xpr_audio_stream("IS: unlocked regionQueue\n", 1,2,3,4,5);
    [device _dataPendingForChannel: [self channel]];
    return YES;
}

/*
 * Return count of bytes required for this region.
 */
- (u_int)mixRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            buffer:(vm_address_t)data maxCount:(u_int)max
            virgin:(BOOL)isVirgin rate:(u_int)srate format:(IOAudioDataFormat)format
	    channelCount:(u_int)chans
{
    u_int count = region->end - region->enq_ptr;

    /* FIXME
     * Crystal chip sends 4 bytes per sample, even in mono 8-bit mode.
     */
//    max /= 4;
    if (count > max)
	count = max;
    region->enq_ptr += count;
    xpr_audio_stream("IS: requested %d bytes\n", count, 2,3,4,5);
    return count;
}

/*
 * Read kernel memory into our task.
 */
static vm_address_t readFromKernel(vm_address_t kmem, u_int size)
{
    vm_address_t data;
    kern_return_t kerr;
    unsigned int count;
    vm_address_t kmem_page = trunc_page(kmem);
    unsigned int offset = kmem - kmem_page;
    vm_task_t kernelTask = (vm_task_t)kern_serv_kernel_task_port();

    /*
     * Note: kmem is normally on a page boundry because we get
     * it from vm_allocate.  This function, however, handles the
     * more general case of an offset.
     */

    /*
     * Read data from kernel map
     * (vm_read allocates memory for us).
     */
    kerr = vm_read(kernelTask, kmem_page, round_page(size + offset),
		   (pointer_t *)&data, &count);
    if (kerr != KERN_SUCCESS) {
	IOLog("Audio: vm_read returned %d\n", kerr);
	//IOPanic("Audio: vm_read\n");
    }
    /*
     * Return offset of the data in our task.
     */
    return data + offset;
}

/*
 * Send recorded data to user.
 */
- sendRecordedDataForRegion:(region_t *)region
{
    kern_return_t krtn;
    u_int count;
    vm_address_t data;
    port_t ourReplyPort, ourUserPort;
    
    count = region->record_ptr - region->data;

    data = readFromKernel(region->data, count);
    xpr_audio_stream("IS: sendRecordedDataForRegion 0x%x, count=%d\n",
		     region, count, 3,4,5);
    if (count == 0) {
	log_debug(("Audio: no data recorded for region!\n"));
	return self;
    }

    ourReplyPort = IOConvertPort(region->reply_port, IO_Kernel,
				 IO_KernelIOTask);
    ourUserPort = IOConvertPort(kernUserPort, IO_Kernel, IO_KernelIOTask);

    if (type == AS_TypeUser) {
	krtn = _NXAudioReplyRecordedData(ourReplyPort, ourUserPort,
					 ourReplyPort, tag,
					 region->tag,
					 data, count);
	if (krtn != KERN_SUCCESS) {
	    xpr_audio_stream("IS: sendRecData replyRecData error %d\n",
			     krtn, 2,3,4,5);
	}
    } else {
	if (!sndReplyMsg) {
	    sndReplyMsg = (msg_header_t *)IOMalloc(MSG_SIZE_MAX);
	    sndReplyMsg->msg_simple = TRUE;
	    sndReplyMsg->msg_size = sizeof(msg_header_t);
	    sndReplyMsg->msg_type = MSG_TYPE_NORMAL;
	    sndReplyMsg->msg_local_port = PORT_NULL;
	    sndReplyMsg->msg_remote_port = PORT_NULL;
	    sndReplyMsg->msg_id = 0;
	}
	audio_snd_reply_recorded_data(sndReplyMsg, ourReplyPort, region->tag,
				data, count);
	krtn = msg_send(sndReplyMsg, STREAM_SEND_OPTIONS, STREAM_SEND_TIMEOUT);
	if (krtn != KERN_SUCCESS) {
	    xpr_audio_stream("IS: sendRecData msg_send error %s (%d)\n",
			     mach_error_string(krtn), krtn, 3,4,5);
	}
    }
    return self;
}

/*
 * Copy recorded data to region.
 */
- completeRegion:(region_t *)region descriptor:(dma_desc_t *)ddp
            size:(u_int)xfer used:(u_int *)used
{
    u_int needed = region->enq_ptr - region->record_ptr;
    int i;
    short *src, *dest;

#ifdef DEBUG
    if (xfer < ddp->size) {
	xpr_audio_stream("IS: xfer (%d) < ddp->size (%d)\n", xfer, ddp->size,
			 3,4,5);
	log_debug(("Audio: recorded %d bytes, wanted %d\n", xfer, ddp->size));
    }
#endif 	DEBUG

    if ((xfer > 0) && (needed > 0)) {
	xpr_audio_stream("IS: completeRegion needs %d bytes\n", needed,
			 2,3,4,5);
	if (*used + needed > xfer)
	    needed = xfer - *used;
	xpr_audio_stream("IS: completeRegion copies %d bytes from 0x%x "
			 "to 0x%x\n", needed, ddp->mem + *used,
			 region->record_ptr, 4,5);
	/*
	 * Swap in place.
	 */
	src = (short *)(ddp->mem + *used);
	dest = (short *)region->record_ptr;
	if (dataFormat == IOAudioDataFormatLinear16) {
	    for (i = 0; i < needed / 2; i++)
		*dest++ = NXSwapHostShortToBig(*src++);
	} else if (dataFormat == IOAudioDataFormatLinear8) { /* && isUnary */
	    char *csrc = (char *)src;
	    char *cdest = (char *)dest;
	    for (i = 0; i < needed; i++) {
		/* convert to 2's comp. by xor'ing the high bit */
		*cdest++ = (*csrc ^ 0x80) | (*csrc & 0x7f);
		csrc++;
	    }
	} else if (dataFormat == IOAudioDataFormatMulaw8)
	    bcopy ((char *)src, (char *)dest, needed);
	*used += needed;
	region->record_ptr += needed;
	bytesProcessed += needed;
    }
    /*
     * Return recorded data on last ddp even if not all data was recorded.
     */
    if ((region->end_ddp == ddp) || region->flags.split)
	[self sendRecordedDataForRegion:region];
    return self;
}

/*
 * Split current region and return data recorded so far.
 */
- returnRecordedData
{
    region_t *region = 0, *newRegion;
    BOOL found = NO;
    kern_return_t krtn;
    int completedSize, remainingSize, enqueuedSize;
    vm_task_t kernelTask = (vm_task_t)kern_serv_kernel_task_port();

    xpr_audio_stream("IS: locking regionQueue\n", 1,2,3,4,5);
    [regionQueueLock lock];
    /*
     * Find the first non-completed region.
     */
    if (!queue_empty(&regionQueue)) {
	region = (region_t *)queue_first(&regionQueue);
	while (!queue_end(&regionQueue, (queue_entry_t)region)) {
	    if ((region->record_ptr > region->data) &&
		(region->record_ptr < region->end)) {
		xpr_audio_stream("IS: returnRecData: region 0x%x\n", region,
				 2,3,4,5);
		found = YES;
		break;
	    }
	    region = (region_t *)queue_next(&region->link);
	}
    }
    if (!found) {
	[regionQueueLock unlock];
	xpr_audio_stream("IS: unlocked regionQueue\n", 1,2,3,4,5);
	xpr_audio_stream("IS: returnRecData: no region to split\n", 1,2,3,4,5);
	wantsRecordedData = YES;
	return self;
    }

    newRegion = (region_t *)IOMalloc(sizeof(region_t));
    *newRegion = *region;
    completedSize = region->record_ptr - region->data;
    remainingSize = region->count - completedSize;
    enqueuedSize = region->enq_ptr - region->record_ptr;
    xpr_audio_stream("IS: returnRecData: complete=%d, remain=%d "
		     "enq=%d\n", completedSize, remainingSize,
		     enqueuedSize, 4,5);

    /*
     * Copy recorded data to a new region, free original data,
     * and allocate new data for original region.
     */
    krtn = vm_allocate(kernelTask, &newRegion->data, completedSize, TRUE);
    if (krtn != KERN_SUCCESS) {
	IOLog("Audio: cannot allocate record memory\n");
	IOFree(newRegion, sizeof(region_t));
	[regionQueueLock unlock];
	wantsRecordedData = YES;
	return self;
    }

    region->start_ddp = 0;
    region->flags.started = FALSE;

    bcopy((char *)region->data, (char *)newRegion->data, completedSize);

    krtn = vm_deallocate(kernelTask, region->data, region->count);
    if (krtn != KERN_SUCCESS)
	log_error(("Audio: vm_deallocate: %s\n", mach_error_string(krtn)));

    krtn = vm_allocate(kernelTask, &region->data, remainingSize, TRUE);
    if (krtn != KERN_SUCCESS) {
	IOLog("Audio: cannot allocate record memory\n");
	remainingSize = enqueuedSize = 0;
    }

    region->count = remainingSize;
    region->end = region->data + region->count;
    region->record_ptr = region->data;
    region->enq_ptr = region->data + enqueuedSize;

    /*
     * Enter the new, completed region at the front of the queue.
     */
    newRegion->count = completedSize;
    newRegion->end = newRegion->data + newRegion->count;
    newRegion->record_ptr = newRegion->enq_ptr = newRegion->end;
    newRegion->flags.ended = newRegion->flags.split = TRUE;
    queue_enter_first(&regionQueue, newRegion, region_t *, link);

    [regionQueueLock unlock];
    xpr_audio_stream("IS: unlocked regionQueue\n", 1,2,3,4,5);
    wantsRecordedData = NO;
    return self;
}

/*
 * Override from superclass to see if we need to return recorded data.
 */
- dmaCompleteDescriptor:(dma_desc_t *)ddp transfered:(u_int)count
{
    [super dmaCompleteDescriptor:ddp transfered:count];

    if (wantsRecordedData)
	[self returnRecordedData];
    return self;
}

/*
 * Free a region.
 */
- freeRegion:(region_t *)region
{
    kern_return_t krtn;

    log_debug(("IS: freeRegion dealloc %d bytes at 0x%x\n", region->count,
	       region->data));
    /*
     * Must get a fresh kernel task port here since
     * we are in the IO task.
     */
    krtn = vm_deallocate((vm_task_t)kern_serv_kernel_task_port(),
			 region->data, region->count);
    if (krtn != KERN_SUCCESS)
	log_error(("Audio: stream vm_deallocate error %s\n",
		   mach_error_string(krtn)));
    return [super freeRegion:region];
}

@end
