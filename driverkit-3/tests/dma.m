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
 * dma.m - kern_dev stub for IODevice testing.
 *
 * HISTORY
 * 08-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <objc/objc.h>
#import <driverkit/driverServer.h>
#import <driverkit/return.h>
#import <driverkit/Device_ddm.h>
#import <architecture/nrw/io.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <machkit/NXLock.h>
#import <bsd/sys/printf.h>
#import <driverkit/IODeviceParams.h>
#import <libc.h>

#ifndef	KERNEL
#define vm_map_t	vm_task_t
#endif	KERNEL

/*
 * Struct for "enqueueing" dma requests.
 */
typedef struct {
	unsigned	dma_id;
	vm_offset_t	address;
	unsigned	byte_count;
	unsigned	completed:1;
	queue_chain_t	link;
} dma_frame_t;

/*
 * Info for one mapped dma channel.
 */
typedef struct {
	queue_head_t 	frame_list;	// queue of dma frames
	id		channel_lock;
	unsigned	mapped:1,	// true if channel is mapped
			running:1;
	IODmaDirection 	dir;
} dma_chan_t;

/*
 * Static variables for "registering" one (and only one) device.
 * If we want to get fancy, we can make these one-per-dev_port.
 */
BOOL		dev_port_exists = NO;	// dev_port_create() called
BOOL		dev_is_mapped = NO;		// dev_reg_mapped() called
port_t 		interrupt_port = PORT_NULL;
BOOL 		streamMode;
dma_chan_t 	channel[CHAN_PER_DEV];
IODevicePage	*dev_page = NULL;
BOOL		int_enabled;
BOOL		int_pending;

static dma_frame_t *dma_frame_alloc();
static void dma_frame_free(dma_frame_t *frame);
static void set_eor(int channel);
static void set_error_int(int channel);

IOReturn _IOLookupByObjectNumber(
	port_t device_master,
	IODeviceNumber dev_number,
	IOSlotId *slot_id,		// returned
	IODeviceType *dev_type,
	BOOL *in_use)
{
	xpr_dma("dev_get_type\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

IOReturn _IOLookupByObjectNumberFromPort(
	port_t dev_port,
	IOSlotId *slot_id,		// returned
	IODeviceType *dev_type)		// returned
{
	xpr_dma("dev_port_to_type\n", 1,2,3,4,5);
	return(IO_R_SUCCESS);
}

IOReturn _IOCreateDevicePort(
	port_t device_master, 
	task_t target_task,
	IODeviceNumber dev_num,
	port_name_t *dev_port)
{
	xpr_dma("dev_port_create\n", 1,2,3,4,5);
	dev_port_exists = YES;
	return(IO_R_SUCCESS);
}

IOReturn _IODestroyDevicePort(
	port_t device_master,
	port_t dev_port)
{
	xpr_dma("dev_port_destroy\n", 1,2,3,4,5);
	dev_port_exists = NO;
	return(IO_R_SUCCESS);
}

IOReturn _IOAttachInterrupt(
	port_t dev_port, 
	port_t intr_port)
{
	xpr_dma("dev_intr_attach\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_intr_attach: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	interrupt_port = intr_port;
	int_enabled = NO;
	return(IO_R_SUCCESS);
}
	
IOReturn _IODetachInterrupt(
	port_t dev_port, 
	port_t intr_port)
{
	xpr_dma("dev_intr_detach\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_intr_detach: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	if(interrupt_port == PORT_NULL) {
		IOLog("dev_intr_detach: no interrupt port\n");
		return IO_R_NOT_ATTACHED;
	}
	interrupt_port = PORT_NULL;
	return(IO_R_SUCCESS);
}
IOReturn _IOAttachChannel(
	port_t dev_port, 
	int chan_num,
	BOOL stream_mode,
	int buffer_size)
{
	dma_chan_t *chan;
	
	xpr_dma("dev_chan_attach\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_chan_attach: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	if(channel[chan_num].mapped) {
		IOLog("dev_chan_attach: chan already mapped\n");
		return IO_R_BUSY;
	}
	if(chan_num > CHAN_PER_DEV) {
		IOLog("dev_chan_attach: bogus channel number (%d)\n", 
			chan_num);
		return(IO_R_INVALID_ARG);
	}
	chan = &channel[chan_num];
	chan->mapped = 1;
	int_pending = 0;
	queue_init(&chan->frame_list);
	streamMode = stream_mode;
	chan->channel_lock = [NXSpinLock new];
	return(IO_R_SUCCESS);
}

IOReturn _IODetachChannel(
	port_t dev_port, 
	int chan_num)
{
	dma_chan_t *chan = &channel[chan_num];
	IOReturn rtn = IO_R_SUCCESS;
	
	xpr_dma("dev_chan_detach\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_chan_detach: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	[chan->channel_lock lock];
	if(!channel[chan_num].mapped) {
		IOLog("dev_chan_detach: chan not mapped\n");
		rtn = IO_R_NOT_ATTACHED;
		goto done;
	}
	channel[chan_num].mapped = 0;
done:
	[chan->channel_lock unlock];
	return rtn;
}

IOReturn _IOMapDevicePage(
	IODevicePort dev_port,
	task_t target_task,
	vm_offset_t *addr,	/* in/out */
	BOOL anywhere,
	IOCache cache)
{
	xpr_dma("dev_reg_map\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_reg_map: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	dev_page = IOMalloc(sizeof(IODevicePage));
	bzero(dev_page, sizeof(*dev_page));
	*addr = (vm_offset_t)dev_page;
	dev_is_mapped = YES;
	return(IO_R_SUCCESS);
}

IOReturn _IOUnmapDevicePage(
	port_t dev_port,
	task_t target_task,
	vm_offset_t addr)
{
	xpr_dma("dev_reg_unmap\n", 1,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("dev_reg_unmap: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	if(!dev_is_mapped) {
		IOLog("dev_reg_unmap: dev not mapped\n");
		return(IO_R_NOT_ATTACHED);
	}
	if(addr != (vm_offset_t)dev_page) {
		IOLog("dev_reg_unmap: bad address\n");
		return IO_R_PRIVILEGE;
	}
	IOFree(dev_page, sizeof(IODevicePage));
	dev_is_mapped = NO;
	return(IO_R_SUCCESS);
}

IOReturn _IOMapSlotSpace(
	port_t dev_port,
	task_t target_task,
	vm_offset_t slot_offset,
	vm_size_t length,
	vm_offset_t *addr,		// in/out
	BOOL anywhere,
	IOCache cache)
{
	xpr_dma("dev_slot_map\n", 1,2,3,4,5);
	if(!dev_port_exists) 
		return IO_R_PRIVILEGE;
	return(IO_R_SUCCESS);
}

IOReturn _IOUnmapSlotSpace(port_t dev_port,
	task_t target_task,
	vm_offset_t addr,
	vm_size_t len)
{
	xpr_dma("dev_slot_unmap\n", 1,2,3,4,5);
	if(!dev_port_exists) 
		return IO_R_PRIVILEGE;
	return(IO_R_SUCCESS);
}

IOReturn _IOSendChannelCommand(port_t dev_port,
	int chan_num,
	IOChannelCommand command)
{
	xpr_dma("chan_command cmd = 0x%x\n", command,2,3,4,5);
	if(!dev_port_exists) {
		IOLog("chan_command: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	if(command & IO_CC_ENABLE_INTERRUPTS)
		int_enabled = 1;
	else if(command & IO_CC_ENABLE_INTERRUPTS)
		int_enabled = 0;
	else {
		IOLog("chan_command: BOGUS COMMAND (0x%x)\n", command);
		return IO_R_INVALID_ARG;
	}
	return(IO_R_SUCCESS);
}

IOReturn _IOEnqueueDMA(port_t dev_port,
	int  chan_num,
	vm_task_t task_id,
	vm_offset_t addr,
	vm_size_t len,
	IODmaDirection rw,
	IODescriptorCommand cmd,
	u_char index,
	IOChannelEnqueueOption opts,
	unsigned dma_id,
	BOOL *running)
{
	IOReturn rtn = IO_R_SUCCESS;
	dma_chan_t *chan;
	dma_frame_t *frame;
	
	xpr_dma("chan_dma_enqueue dma_id 0x%x len %d chan %d\n", 
		dma_id, len, chan_num, 4,5);
	if(!dev_port_exists) {
		IOLog("chan_dma_enqueue: no dev_port\n");
		return IO_R_PRIVILEGE;
	}
	if(!dev_is_mapped) {
		IOLog("chan_dma_enqueue: no dev_page\n");
		return IO_R_PRIVILEGE;
	}
	chan = &channel[chan_num];
	[chan->channel_lock lock];
	if(!chan->mapped) {
		IOLog("chan_dma_enqueue: channel not mapped\n");
		rtn = IO_R_NOT_ATTACHED;
		goto done;
	}
	
	/*
	 * Take care of enabling interrupts now to simpify notification...
	 */
	if(opts & IO_CEO_ENABLE_INTERRUPTS)
		int_enabled = YES;
		
	/*
	 * Enqueue this on the end of this channel's pending DMAs.
	 */
	frame = dma_frame_alloc();
	frame->dma_id = dma_id;
	frame->address = addr;
	frame->byte_count = len;
	frame->completed = 0;
	if(!queue_empty(&chan->frame_list)) {
	
		/*
		 * Make sure we're not changing direction.
		 */
		if(chan->dir != rw) {
			IOLog("chan_dma_enqueue: DMA DIRECTION CHANGE\n");
			rtn = IO_R_INVALID_ARG;
			goto done;
		}
	}
	else {
		chan->dir = rw;
	}
	queue_enter(&chan->frame_list,
		frame,
		dma_frame_t *,
		link);
		
	/*
	 * Block devices:
	 * Mark this frame as done. Block devices do their own interrupt
	 * simulation for testing.
	 *
	 * Stream devices:
	 * Mark this frame as done for outbound frames only.
	 * If this is a IO_DMAWrite and there exists a IO_DMARead 
	 * channel on this device, mark the first non-complete frame
	 * on the read channel's queue as done, copy the data we just enqueued
	 * to that (DMA read) frame, and send an interrupt message to the
	 * driver. (Intended for ethernet debug).
	 */
	if(!streamMode) {
		frame->completed = 1;
		set_eor(chan_num);
	}
	else if(rw == IO_DMAWrite) {
		
		int read_chan_num;
		dma_chan_t *read_chan = &channel[0];
		dma_frame_t *read_frame;
		BOOL found = NO;
		IOInterruptMsg int_msg;
		kern_return_t krtn;
		
		frame->completed = 1;
		set_eor(chan_num);
		for(read_chan_num=0; 
		    read_chan_num<CHAN_PER_DEV; 
		    read_chan_num++) {
			if(read_chan->dir == IO_DMARead) 
				break;
			chan++;
		}
		if(read_chan_num == CHAN_PER_DEV) {
			xpr_dma("chan_dma_enqueue: NO READ CHANNEL\n", 
				1,2,3,4,5);
				
			/*
			 * for int_pending....
			 */
			read_chan = chan;
			goto send_intr;
		}
		
		/*
		 * OK, look for an uncompleted DMA frame on the Read channel 
		 * queue.
		 */
		read_frame = (dma_frame_t *)
			queue_first(&read_chan->frame_list);
		while(!queue_end(&read_chan->frame_list, 
		    (queue_t)read_frame)) {
		 	if(!read_frame->completed) {
				found = YES;
				break;
			}
			read_frame = (dma_frame_t *)read_frame->link.next;
		}
		if(!found) {
			/*
			 * DMA overrun! Let's call this a "channel error"
			 * interrupt.
			 */
			xpr_dma("chan_dma_enqueue: DMA Overrun on Read\n ",
				1,2,3,4,5);
			if(queue_empty(&read_chan->frame_list)) {
				xpr_dma("   (no frames)\n", 1,2,3,4,5);
			}
			else {
				xpr_dma("   (no incompl frames)\n", 1,2,3,4,5);
			}
			set_error_int(read_chan_num);
			goto send_intr;
		}
					
		/*
		 * Found an incomplete Read frame. Copy data over to it
		 * and make it complete.
		 */
		bcopy((void *)frame->address, (void *)read_frame->address,
			frame->byte_count);
		read_frame->completed = 1;
		read_frame->byte_count = frame->byte_count;
		set_eor(read_chan_num);
		
		/*
		 * Finally, send an interrupt message to the device if 
		 * we haven't already done so since the last "enable
		 * interrupts".
		 */
send_intr:
		if(int_enabled && !int_pending) {
			bzero(&int_msg, sizeof(int_msg));
			int_msg.header.msg_size = sizeof(int_msg);
			int_msg.header.msg_id = IO_DMA_INTERRUPT_MSG;
			int_msg.header.msg_remote_port = interrupt_port;
			int_msg.header.msg_local_port = PORT_NULL;
			
			/*
			 * Timeout is to handle port queue full. If that
			 * happens, it's a bug in this weird test simulation,
			 * not the driver. That can never happen on the 
			 * NRW.
			 */
			krtn = msg_send(&int_msg.header, SEND_TIMEOUT, 1);
			if(krtn) {
				IOLog("chan_dma_enqueue: msg_send returned "
					"%d\n", krtn);
			}
			int_pending = 1;
			int_enabled = 0;
		}
	}
	*running = NO;	// right???
done:
	[chan->channel_lock unlock];
	return rtn;
}

IOReturn _IOEnqueueDMAInt(
	IODevicePort dev_port,
	int chan_num,
	vm_map_t task_id,
	vm_offset_t addr,
	vm_size_t len,
	IODmaDirection rw,
	IODescriptorCommand cmd,	// descriptor command - m88k only
	u_char index,
	IOChannelEnqueueOption opts,
	unsigned dma_id,		// must be non-zero
	BOOL *running)			// returned - m88k only
{
	return _IOEnqueueDMA(dev_port,
		chan_num,
		task_id,
		addr,
		len,
		rw,
		cmd,
		index,
		opts,
		dma_id,
		running);
}

IOReturn _IODequeueDMA(port_t dev_port,
	int chan_num,
	IOChannelDequeueOption opts,
	vm_size_t *bcount,		// RETURNED
	IOUserStatus *userStatus,	// RETURNED, m88k only
	IODmaStatus *dmaStatus,		// RETURNED
	BOOL *eor,			// RETURNED
	unsigned *dma_id)		// RETURNED
{
	dma_frame_t *frame = NULL;
	dma_chan_t *chan;
	IOReturn rtn = IO_R_SUCCESS;
	BOOL found = NO;
	unsigned summary_bit = 1 << chan_num;
	
	if(!dev_port_exists) 
		return IO_R_PRIVILEGE;
	chan = &channel[chan_num];
	[chan->channel_lock lock];
	if(!chan->mapped) {
		IOLog("chan_dma_dequeue: channel not mapped\n");
		rtn = IO_R_NOT_ATTACHED;
		goto done;
	}
	
	/*
	 * See if we have a frame to dequeue. We can't tell if a channel
	 * is running; this'll be crude.
	 */
	if(!queue_empty(&chan->frame_list)) {
		frame = (dma_frame_t *)queue_first(&chan->frame_list);
		if(frame->completed || (opts & IO_CDO_ALL)) 
			found = YES;
	}
	if(found) {
		queue_remove(&chan->frame_list,
			frame,
			dma_frame_t *,
			link);
		*dma_id = frame->dma_id;
		*bcount = frame->byte_count;
		*dma_status = IO_Complete;
		*user_status = 0;
		*eor = frame->completed ? YES : NO;
		dma_frame_free(frame);
	}
	else
		*dma_id = IO_NULL_DMA_ID;
		
	/*
	 * Clear this channel's interrupt bit in the summary register.
	 */
	dev_page->int_dev.intr_summ.is_u.is_int &= ~summary_bit;
	int_pending = 0;
	
	/*
	 * Re-enable interrupts if appropriate.
	 */
	if((opts & IO_CDO_ENABLE_INTERRUPTS) ||
	   ((opts & IO_CDO_ENABLE_INTERRUPTS_IF_EMPTY) && (*dma_id == IO_NULL_DMA_ID))) {
	 	int_enabled = 1;  
	}
	
done:
	xpr_dma("chan_dma_dequeue dma_id 0x%x bcount %d chan %d\n", 
		*dma_id, *bcount ,chan_num, 4,5);
	[chan->channel_lock unlock];
	return rtn;
}

/*
 * Determine deviceType and name of specified unit.
 */
IOReturn _IOLookupByObjectNumber(
	port_t deviceMaster,
	IOObjectNumber unit,
	IOString *deviceType,			// returned
	IOString *name)			// returned
{
	return IODoInquire(unit, 
		deviceType, 
		name);
}

/*
 * Get/set parameter RPCs. 
 */
 
IOReturn _IOGetParameterInIntArray(
	port_t device_master,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned int *parameterArray,		// data returned here
	unsigned int *returnedCount)		// size returned here
{
	return IODoGetParameterInt(unit, 
		parameterName,
		maxCount,
		parameterArray,
		returnedCount);
}

IOReturn _IOGetParameterInCharArray(
	port_t device_master,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned char *parameterArray,		// data returned here
	unsigned int *returnedCount)		// size returned here
{
	return IODoGetParameterChar(unit, 
		parameterName,
		maxCount,
		parameterArray,
		returnedCount);
}

IOReturn _IOSetParameterFromIntArray(
	port_t device_master,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int count,			// size of parameterArray
	unsigned int *parameterArray)
{
	return IODoSetParameterInt(unit, 
		parameterName,
		count,
		parameterArray);
}

IOReturn _IOSetParameterFromCharArray(
	port_t device_master,
	IOObjectNumber unit,
	IOParameterName parameterName,
	unsigned int count,			// size of parameterArray
	unsigned char *parameterArray)
{
	return IODoSetParameterChar(unit, 
		parameterName,
		count,
		parameterArray);
}


static dma_frame_t *dma_frame_alloc()
{
	return(IOMalloc(sizeof(dma_frame_t)));
}

static void dma_frame_free(dma_frame_t *frame)
{
	IOFree(frame, sizeof(dma_frame_t));
}

/*
 * Set channel's EOR and descriptor interrupt bits in channel interrupt cause.
 * Also set the channel interrupt bit in the device interrupt summary 
 * register.
 */
static void set_eor(int channel)
{
	chan_t *io_chan = &dev_page->chan_regs[channel];
	unsigned summary_bit = 1 << channel;
	
	io_chan->chan_intr_cause.ci_u.ci_int |= (CI_DESC | CI_EIO);
	dev_page->int_dev.intr_summ.is_u.is_int |= summary_bit;
}

/*
 * Set channel's error interrupt.
 */
static void set_error_int(int channel)
{
	chan_t *io_chan = &dev_page->chan_regs[channel];
	unsigned summary_bit = 1 << channel;
	
	io_chan->chan_intr_cause.ci_u.ci_int |= CI_ERROR;
	dev_page->int_dev.intr_summ.is_u.is_int |= summary_bit;
}
