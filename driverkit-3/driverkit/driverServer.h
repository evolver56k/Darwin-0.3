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
 * driverServer.h - Private RPC Interface to kernel's driverServer module.
 *
 * HISTORY
 * 13-Jan-98	Martin Minow at Apple
 *	Added createMachPort
 * 05-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <bsd/sys/types.h> 
#import <mach/port.h>
#import <mach/mach_types.h>
#import <driverkit/driverTypes.h>
#import <driverkit/driverTypesPrivate.h>

extern port_t device_master_self();

/*
 * Obtain IOSlotId_t and dev_type for specified IODeviceNumber. 
 * Normally executed by Config only.
 */
IOReturn _IOLookupByDeviceNumber(
	port_t deviceMaster,
	IODeviceNumber deviceNumber,
	IOSlotId *slotId,		// returned
	IODeviceType *deviceType,	// returned
	BOOL *inUse);			// returned
	
/*
 * Obtain IOSlotId and IODeviceType for specified devicePort.
 */
IOReturn _IOLookupByDevicePort(
	port_t devicePort,
	IOSlotId *slotId,		// returned
	IODeviceType *deviceType);	// returned

/*
 * Obtain physical address of a devicePage. The devicePage pointer 
 * returned by this RPC can not be derefenced by the caller.
 */
IOReturn _IOGetPhysicalAddressOfDevicePage(
	port_t devicePort,
	unsigned *devicePage);	// returned

/*
 * Create device port for specified deviceNumber.
 */
IOReturn _IOCreateDevicePort(
	port_t deviceMaster, 
	task_t targetTask, 
	IODeviceNumber deviceNumber,
	port_t *devicePort);
	
/*
 * Destroy specified devicePort.
 */
IOReturn _IODestroyDevicePort(
	port_t deviceMaster,
	port_t devicePort);
	
/*
 * Request that interrupt notification messages for the device represented 
 * by devicePort be sent to the port interruptPort.
 */
IOReturn _IOAttachInterrupt(
	port_t devicePort, 
	port_t interruptPort);
	
/*
 * Disassociate interrupt notification messages for the device represented 
 * by devicePort from being sent to intr_port.
 */
IOReturn _IODetachInterrupt(
	port_t devicePort, 
	port_t interruptPort);

/*
 * Associate a hardware dma channel with the channel channelNumber of the 
 * device represented by devicePort.
 */
IOReturn _IOAttachChannel(
	port_t devicePort, 
	int channelNumber,
	BOOL streamMode,
	int bufferSize);
	
/*
 * Disassociate the dma channel channelNumber from the device represented by 
 * devicePort. Frees up all resources associated with specified DMA channel.
 */
IOReturn _IODetachChannel(
	port_t devicePort, 
	int channelNumber);
	
/*
 * Map the device register page of the device associated with devicePort 
 * into the target task at addr.
 */
IOReturn _IOMapDevicePage(
	port_t devicePort,
	task_t targetTask,
	vm_address_t *addr,	/* in/out */
	BOOL anywhere,
	IOCache cache);		/* IO_CacheOff, IO_WriteThrough, etc. */

/*
 * Unmap the device register page of the device associated with devicePort.
 */
IOReturn _IOUnmapDevicePage(
	port_t devicePort,
	task_t target_task,
	vm_address_t addr);

/*
 * Map the portion of the slot space of the NeXTbus device 
 * associated with devicePort, specified by slotOffset and length, into 
 * the target task at addr.
 */
IOReturn _IOMapSlotSpace(
	port_t devicePort,
	task_t targetTask,
	vm_offset_t slotOffset,
	vm_size_t length,
	vm_address_t *addr,		// in/out
	BOOL anywhere,
	IOCache cache);
/*
 * Unmap the slot space of the NeXTbus device associated with devicePort.
 * 'len' and 'addr' must match similar fields from a previous dev_slot_map().
 */
IOReturn _IOUnmapSlotSpace(
	port_t devicePort,
	task_t targetTask,
	vm_address_t addr,
	vm_size_t length);

/*
 * Map the portion of the board space of the NeXTbus device 
 * associated with devicePort, specified by boardOffset and length, into 
 * the targetTask at addr.
 */
IOReturn _IOMapBoardSpace(
	port_t devicePort,
	task_t targetTask,
	vm_offset_t boardOffset,
	vm_size_t length,
	vm_address_t *addr,		// in/out
	BOOL anywhere,
	IOCache cache);

/*
 * Unmap the board space of the NeXTbus device associated with devicePort.
 * 'len' and 'addr' must match similar fields from a previous IOMapBoard().
 */
IOReturn _IOUnmapBoardSpace(
	port_t devicePort,
	task_t targetTask,
	vm_address_t addr,
	vm_size_t length);

/*
 * Issue command on the dma channel identified by devicePort and 
 * channelNumber. 
 * 
 * IO_CHAN_NONE can be specified for channelNumber if no channels are attached; 
 * in this case the only legal IOChannelCommand bits are 
 * IO_CC_ENABLE_INTERRUPTS and IO_CC_DISABLE_INTERRUPTS.
 * See <driverkit/types.h> for bit definitions for IOChannelCommand.
 */
IOReturn _IOSendChannelCommand(
	port_t devicePort,
	int channelNumber,
	IOChannelCommand command);

/*
 * Build and enqueue a list of dma descriptors. The memory referenced by the 
 * descriptor will be locked. This returns IO_CR_ALIGNMENT if the channel has
 * been configured as a streaming mode channel, rw is IO_DMA_Read, and
 * the specified frame crosses a page boundary.
 * See <driverkit/types.h> for definitions of IOChannelEnqueueOption. 
 */
IOReturn _IOEnqueueDMA(
	port_t devicePort,
	int channelNumber,
	vm_task_t task,
	vm_address_t addr,
	vm_size_t len,
	IODMADirection rw,
	IODescriptorCommand cmd,	// descriptor command (m88k only)
	unsigned char index,		// descriptor command region/index  
					//    (m88k only)
	IOChannelEnqueueOption opts,
	unsigned dmaId,			// must be non-zero
	BOOL *running);			// returned 

#ifdef	KERNEL

/*
 * Kernel version of IOEnqueueDma(). Most kernel drivers don't have the
 * task port of their clients; they pass in a vm_task_t (actually, a
 * pointer to a kernel vm_map) directly here.
 * 
 * This is not an RPC; it's a direct function call to driverServer.
 */
IOReturn _IOEnqueueDMAInt(
	port_t devicePort,
	int chan_num,
	vm_task_t task_id,
	vm_offset_t addr,
	vm_size_t len,
	IODMADirection rw,
	IODescriptorCommand cmd,	// descriptor command - m88k only
	unsigned char index,		// desc cmd region/index - m88k only
	IOChannelEnqueueOption opts,
	unsigned dma_id,		// must be non-zero
	BOOL *running);			// returned - m88k only
	
#endif	KERNEL

/*
 * Dequeue a dma frame enqueued via chan_dma_enqueue.  
 * A dma_id of IO_NULL_DMA_ID indicates that no descriptors are available;
 * IO_CR_SUCCESS is returned in that case. IO_CR_BUSY is returned if the
 * channel is still enabled and no completed descriptors are available.
 * See <driverkit/types.h> for definitions of IOChannelDequeueOption. 
 */
IOReturn _IODequeueDMA(
	port_t devicePort,
	int channelNumber,
	IOChannelDequeueOption opts,
	vm_size_t *numBytes,		// RETURNED (except on m88k output 
					//    devices)
	IOUserStatus *userStatus,	// RETURNED, m88k only
	IODMAStatus *dmaStatus,		// RETURNED
	BOOL *eor,			// RETURNED
	unsigned *dmaId);		// RETURNED

/*
 * Determine deviceKind and deviceName of specified unit. Returns
 * IO_R_NOT_ATTACHED if specified unit is not currently registered and
 * IO_R_NO_DEVICE if specifed unit is greater than current maximum total
 * nmumber of units.
 */
IOReturn _IOLookupByObjectNumber(
	port_t deviceMaster,
	IOObjectNumber objectNumber,
	IOString *deviceKind,			// returned
	IOString *deviceName);			// returned
	
/*
 * Determine deviceKind and IOObjectNumber of specified deviceName. Returns
 * IO_R_NOTATTACHED if deviceName not found, else returns IO_R_SUCCESS.
 */
IOReturn _IOLookupByDeviceName(
	port_t deviceMaster,
	IOString deviceName,
	IOObjectNumber *objectNumber,		// returned
	IOString *deviceKind);			// returned

/*
 * Get/set parameter RPCs. 
 */
IOReturn _IOGetIntValues(
	port_t device_master,
	IOObjectNumber objectNumber,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned int *parameterArray,		// data returned here
	unsigned int *returnedCount);		// size returned here
	
IOReturn _IOGetCharValues(
	port_t device_master,
	IOObjectNumber objectNumber,
	IOParameterName parameterName,
	unsigned int maxCount,			// 0 means "as much as
						//    possible"
	unsigned char *parameterArray,		// data returned here
	unsigned int *returnedCount);		// size returned here
	
IOReturn _IOSetIntValues(
	port_t device_master,
	IOObjectNumber objectNumber,
	IOParameterName parameterName,
	unsigned int *parameterArray,
	unsigned int count);			// size of parameterArray

IOReturn _IOSetCharValues(
	port_t device_master,
	IOObjectNumber objectNumber,
	IOParameterName parameterName,
	unsigned char *parameterArray,
	unsigned int count);			// size of parameterArray

IOReturn _IOServerConnect(
	port_t device_master,
	IOObjectNumber objectNumber,
	port_t clientTask,
	port_t *serverPort);
/*
 * Pull in machine specific stuff.
 */

#import <driverkit/machine/driverServer.h>

/*
 * Create an IODeviceDescription for the config table data passed in
 * configData, and probe the appropriate device class. Returns IO_R_SUCCESS
 * if a driver was successfully instantiated, else returns IO_R_NO_DEVICE.
 */
IOReturn _IOProbeDriver(port_t device_master,
	IOConfigData configData,
	unsigned configDataSize);
	
/*
 * Obtain the current system config table data.
 */
IOReturn _IOGetSystemConfig(port_t deviceMaster,
	unsigned maxDataSize,
	IOConfigData configData,		// returned
	unsigned *configDataSize);		// returned

/*
 * Unload a driver.
 */
IOReturn _IOUnloadDriver(port_t deviceMaster,
	IOConfigData configData,
	unsigned configDataSize);

/*
 * Obtain the current config table data for driver 'n'.
 */
IOReturn _IOGetDriverConfig(port_t deviceMaster,
	unsigned driverNum,
	unsigned maxDataSize,
	IOConfigData configData,		// returned
	unsigned *configDataSize);		// returned

