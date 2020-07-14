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
 * interruptMsg.h - Interrupt message defintion.
 *
 * HISTORY
 * 04-Apr-91    Doug Mitchell at NeXT
 *      Created. 
 */

#import <mach/message.h>

/*
 * Interrupt message sent by kernel to device-specific handler.
 */
typedef struct {

	msg_header_t	header;		// standard message header 
	
} IOInterruptMsg;

/*
 * Values for IOInterruptMsg.header.
 */
#define IO_INTERRUPT_MSG_ID_BASE	0x232323

#define IO_TIMEOUT_MSG			(IO_INTERRUPT_MSG_ID_BASE + 0)
#define IO_COMMAND_MSG			(IO_INTERRUPT_MSG_ID_BASE + 1)
/*
 * Next 16 reserved for multiple interrupt sources per device.
 */
#define IO_DEVICE_INTERRUPT_MSG		(IO_INTERRUPT_MSG_ID_BASE + 2)
#define IO_DEVICE_INTERRUPT_MSG_FIRST	(IO_DEVICE_INTERRUPT_MSG)
#define IO_DEVICE_INTERRUPT_MSG_LAST	(IO_DEVICE_INTERRUPT_MSG_FIRST + 15)
#define IO_DMA_INTERRUPT_MSG		(IO_DEVICE_INTERRUPT_MSG_LAST + 1)
#define IO_FIRST_UNRESERVED_INTERRUPT_MSG	\
					(IO_DMA_INTERRUPT_MSG + 1)
