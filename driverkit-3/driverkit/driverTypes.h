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
 * driverTypes.h - Data types and #defines for driverServer interface.
 *
 * HISTORY
 * 05-June-91    Doug Mitchell at NeXT
 *      Created. 
 */

#ifndef	_DRIVERKIT_DRIVERTYPES_
#define _DRIVERKIT_DRIVERTYPES_

#import <bsd/sys/types.h>
#import <mach/port.h>
#import <mach/mach_types.h>
#import <architecture/arch_types.h>
#import <driverkit/return.h>
#import <objc/objc.h>

#ifndef	NULL
#define	NULL	0
#endif	NULL
	
/*
 * Simple data types.
 */
typedef unsigned int 	IOChannelCommand;
typedef unsigned int	IOChannelEnqueueOption;
typedef unsigned int 	IOChannelDequeueOption;
typedef unsigned char 	IODescriptorCommand;
typedef unsigned int	IODeviceNumber;
typedef unsigned int 	IOObjectNumber;

/*
 * Machine-independent DMA channel status.
 */
typedef enum {
	IO_None,			// no appropriate status
	IO_Complete,			// DMA channel idle
	IO_Running,			// DMA channel running 
	IO_Underrun,			// under/overrun
	IO_BusError,			// bus error
	IO_BufferError,			// DMA buffer error
} IODMAStatus;


typedef enum {
	IO_DMARead,			// device to memory
	IO_DMAWrite			// memory to device
} IODMADirection;

/*
 * Machine-independent caching specification.
 */
typedef	enum {
	IO_CacheOff,			// cache inhibit
	IO_WriteThrough,
	IO_CopyBack
} IOCache;

/*
 * A typoedef which distinguishes direct, indirect, and pseudo devices.
 */
typedef enum {
	IO_DirectDevice,
	IO_IndirectDevice,
	IO_PseudoDevice
} IODeviceStyle;

/*
 * Indicates a range of values.  Used for memory regions, port regions, etc.
 */
typedef struct range {
	unsigned int	start;
	unsigned int	size;
} IORange;

/*
 * Map between #defined or enum'd constants and text description.
 */
typedef struct {
	int value;
	const char *name;
} IONamedValue;

/*
 * Value-to-string conversion arrays in libDriver.
 */
extern const IONamedValue IODMAStatusStrings[];		// for IODMAStatus

/*
 * Specify DMA alignment.
 * A value of 0 means no restriction for associated parameter.
 */
typedef struct {
	unsigned	readStart;
	unsigned	writeStart;
	unsigned	readLength;
	unsigned	writeLength;
} IODMAAlignment;

/*
 * Machine-independent type for a machine-dependent DMA buffer.
 */
typedef void *IODMABuffer;

/*
 * Memory alignment -- specified as a power of two.
 */
typedef unsigned int	IOAlignment;

#define IO_NULL_VM_TASK		((vm_task_t)0)

/*
 * Hardcoded IOSlotId for native devices.
 */
#define IO_NATIVE_SLOT_ID	((IOSlotId)-1)

/*
 * Hardcoded IODeviceType for slot devices.
 */
#define IO_SLOT_DEVICE_TYPE	((IODeviceType)-1)

/*
 * IOSlotId and IODeviceType for non-existent device.
 */
#define IO_NULL_SLOT_ID		((IOSlotId)0)
#define IO_NULL_DEVICE_TYPE	((IODeviceType)0)
#define IO_NULL_DEVICE_INDEX	((IODeviceIndex)0)

/*
 * Maximum sizes for IOMapSlot() and IOMapBoard().
 */
#define IO_MAX_BOARD_SIZE	(256 * 1024 * 1024)
#define IO_MAX_NRW_SLOT_SIZE	(15 * 1024 * 1024)
#define IO_MAX_SLOT_SIZE	(16 * 1024 * 1024)

/*
 * Standard type for an ASCII name (e.g., deviceName, deviceType).
 */
#define IO_STRING_LENGTH	80
typedef char IOString[IO_STRING_LENGTH];

/*
 * Parameter name for get/set parameter RPCs.
 */
#define IO_MAX_PARAMETER_NAME_LENGTH	(64)
typedef char IOParameterName[IO_MAX_PARAMETER_NAME_LENGTH];

/* 
 * Actual data arrays passed in get/set parameter RPCs.
 */
#define IO_MAX_PARAMETER_ARRAY_LENGTH	(512)
typedef int IOIntParameter[IO_MAX_PARAMETER_ARRAY_LENGTH];
typedef char IOCharParameter[IO_MAX_PARAMETER_ARRAY_LENGTH];
typedef unsigned char IOByteParameter[IO_MAX_PARAMETER_ARRAY_LENGTH * 4];

/*
 * Pull in machine specific stuff.
 */

#import <driverkit/machine/driverTypes.h>

/* 
 * Values for IOChannelCommand.
 */
#define IO_CC_ENABLE_INTERRUPTS		0x00000001	// enable interrrupts
#define IO_CC_DISABLE_INTERRUPTS	0x00000002	// disable interrupts
#define IO_CC_CONNECT_FRAME_LOOP	0x00000004	// create DMA frame 
							//    loop
#define IO_CC_DISCONNECT_FRAME_LOOP	0x00000008	// disconnect DMA frame
							//    loop

/*
 * m68k only. 
 */
#define IO_CC_START_READ		0x00000008	// start DMA Read
#define IO_CC_START_WRITE		0x00000010	// start DMA Write
#define IO_CC_ABORT			0x00000020	// abort current 
							//   operation
#define IO_CC_ENABLE_DEVICE_INTERRUPTS	0x00000040	// enable ints, device 
							//   level
#define IO_CC_DISABLE_DEVICE_INTERRUPTS	0x00000080	// disable ints, device
							//   level

/*
 * Used as channelNumber argument for commands which do not refer to a
 * channel.
 */
#define IO_NO_CHANNEL	(-1)


/*
 * Values for IOChannelEnqueueOption.
 */
#define IO_CEO_END_OF_RECORD		0x00000001	// end of record
#define IO_CEO_DESCRIPTOR_INTERRUPT	0x00000002	// generate descriptor 
							//    interrupt
#define IO_CEO_ENABLE_INTERRUPTS	0x00000004	// enable interrupt
#define IO_CEO_ENABLE_CHANNEL		0x00000008	// enable channel (m68k
							//    only)
#define IO_CEO_DESCRIPTOR_COMMAND	0x00000010	// enable descriptor 
							// command (nrw only)

/*
 * Values for IOChannelDequeueOption.
 * IF IO_CDO_ALL is specified and the channel is running, CR_BUSY will be
 * returned and all other options will be ignored.
 */
#define IO_CDO_DONE			0x00000001	// dequeue completed
							//   descriptors
#define IO_CDO_ALL			0x00000002	// dequeue all 
							//   descriptors 
#define IO_CDO_ENABLE_INTERRUPTS	0x00000004	// enable interrupt
#define IO_CDO_ENABLE_INTERRUPTS_IF_EMPTY 0x00000008	// enable interrupt if
							// no more descriptors
							// can be dequeued

#define IO_NULL_DMA_ID	((unsigned) 0)

typedef void *IOAddressMap;
typedef unsigned int IOPhysicalAddress;
typedef unsigned int IOVirtualAddress;

typedef void 		(*IOInterruptHandler)(void *identity, 
				void *state, 
				unsigned int arg);

#endif	_DRIVERKIT_DRIVERTYPES_
