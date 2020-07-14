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
/* 	Copyright (c) 1989 NeXT Computer, Inc.  All rights reserved. 
 * 
 * return.h - driverkit return codes.
 *
 * HISTORY
 * 14-Nov-89    Doug Mitchell at NeXT
 *      Created.
 */
 
/*
 * IOReturn values. Subclasses of IODevice may define more of these.
 */
typedef	int	IOReturn;

#define IO_R_SUCCESS		0		// OK 	
#define IO_R_NO_MEMORY		(-701)		// couldn't allocate memory 
#define IO_R_RESOURCE		(-702)		// resource shortage 
#define IO_R_IPC_FAILURE	(-703)		// error during IPC 
#define IO_R_NO_DEVICE		(-704)		// no such device 
#define IO_R_PRIVILEGE		(-705)		// privilege/access violation 
#define IO_R_INVALID_ARG	(-706)		// invalid argument 
#define IO_R_LOCKED_READ	(-707)		// device read locked 
#define IO_R_LOCKED_WRITE	(-708)		// device write locked 
#define IO_R_EXCLUSIVE_ACCESS	(-709)		// exclusive access device &&
						// already open 
#define IO_R_BAD_MSG_ID		(-710)		// sent/received messages had 
						// different msg_id's 
#define IO_R_UNSUPPORTED 	(-711)		// unsupported function 
#define IO_R_VM_FAILURE		(-712)		// misc. VM failure 
#define IO_R_INTERNAL		(-713)		// internal library error 
#define IO_R_IO			(-714)		// General I/O error 
#define IO_R_CANT_LOCK		(-716)		// can't acquire requested lock
#define IO_R_NOT_OPEN		(-717)		// device not open 
#define IO_R_NOT_READABLE	(-718)		// read not supported 
#define IO_R_NOT_WRITABLE	(-719)		// write not supported 
#define IO_R_ALIGN		(-720)		// DMA alignment error 
#define IO_R_MEDIA		(-721)		// Media Error 
#define IO_R_OPEN		(-722)		// device(s) still open 
#define IO_R_RLD		(-723)		// rld failure 
#define IO_R_DMA		(-724)		// DMA failure 
#define IO_R_BUSY		(-725)		// Device Busy 
#define IO_R_TIMEOUT		(-726)		// I/O Timeout 
#define IO_R_OFFLINE		(-727)		// device offline 
#define IO_R_NOT_READY		(-728)		// not ready 
#define IO_R_NOT_ATTACHED	(-729)		// device/channel not attached 
#define IO_R_NO_CHANNELS 	(-730)		// no DMA channels available 
#define IO_R_NO_SPACE 		(-731)		// no address space available 
						//   for mapping 
#define IO_R_PORT_EXISTS 	(-733)		// devicePort already exists
#define	IO_R_CANT_WIRE 		(-734)		// Can't wire down physical 
						//   memory
#define	IO_R_NO_INTERRUPT 	(-735)		// no interrupt port attached
#define IO_R_NO_FRAMES		(-736)		// no DMA frames enqueued
#define	IO_R_MSG_TOO_LARGE	(-737)		// oversized message received
						//   on interrupt port

#define IO_R_INVALID		(-1)		// should never be seen 
